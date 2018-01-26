package ops

import (
	"context"
	"fmt"
	"path"
	"time"

	cmd "github.com/pensando/sw/api/generated/cmd"

	"github.com/gogo/protobuf/types"
	"github.com/satori/go.uuid"

	"github.com/pensando/sw/venice/cmd/env"
	"github.com/pensando/sw/venice/cmd/grpc"
	"github.com/pensando/sw/venice/cmd/grpc/server/auth"
	certutils "github.com/pensando/sw/venice/cmd/grpc/server/certificates/utils"
	"github.com/pensando/sw/venice/cmd/utils"
	"github.com/pensando/sw/venice/cmd/validation"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/certmgr"
	"github.com/pensando/sw/venice/utils/errors"
	"github.com/pensando/sw/venice/utils/log"
)

const (
	defaultNTPServer = "pool.ntp.org"
)

// clusterCreateOp contains state for creating a cluster.
type clusterCreateOp struct {
	cluster *cmd.Cluster
}

// NewClusterCreateOp creates an op for creating a cluster.
func NewClusterCreateOp(cluster *cmd.Cluster) Op {
	return &clusterCreateOp{
		cluster: cluster,
	}
}

// Validate is a method to validate the cluster object.
func (o *clusterCreateOp) Validate() error {
	// Check if in cluster.
	if cluster, err := utils.GetCluster(); err != nil {
		return errors.NewInternalError(err)
	} else if cluster != nil {
		return errors.NewBadRequest(fmt.Sprintf("Already part of cluster +%v", cluster))
	}

	// Validate arguments.
	if errs := validation.ValidateCluster(o.cluster); len(errs) != 0 {
		return errors.NewInvalid("cluster", "", errs)
	}
	return nil
}

// populateClusterDefaults fills in the defaults for cluster object.
func (o *clusterCreateOp) populateClusterDefaults() {
	o.cluster.Kind = "Cluster"
	o.cluster.UUID = uuid.NewV4().String()
	if len(o.cluster.Spec.NTPServers) == 0 {
		o.cluster.Spec.NTPServers = append(o.cluster.Spec.NTPServers, defaultNTPServer)
	}
}

// Run executes the cluster creation steps.
func (o *clusterCreateOp) Run() (interface{}, error) {
	// Populate defaults (UUID, NTP Servers etc)
	o.populateClusterDefaults()

	// Generate etcd quorum configuration.
	quorumConfig, err := makeQuorumConfig(o.cluster.UUID, o.cluster.Spec.QuorumNodes)
	if err != nil {
		return nil, errors.NewInternalError(err)
	}

	if env.CertMgr == nil {
		cm, err := certmgr.NewGoCryptoCertificateMgr(certutils.GetCertificateMgrDir())
		if err != nil {
			return nil, fmt.Errorf("Failed to instantiate certificate manager, error: %v", err)
		}
		env.CertMgr = cm
	}
	// Transport key is an asymmetric key that allows multiple CMD instances to securely agree on a
	// symmetric key that can be used to transport secrets across CMD instances.
	// For instance, the CertMgr CA signing key is generated by the CMD instance that initiates
	// cluster formation and transferred to other CMD instances as the join
	transportKey, err := env.CertMgr.GetKeyAgreementKey("QuorumNodes")
	if err == nil {
		defer env.CertMgr.DestroyKeyAgreementKey("QuorumNodes")
	}

	// Since this is the instance that initiates cluster formation, bootstrap the CA.
	// This generates the signing key if it is not already available.
	err = env.CertMgr.StartCa(true)
	if err != nil {
		return nil, errors.NewInternalError(fmt.Errorf("Error starting CertificatesMgr CA: %v", err))
	}
	if !env.CertMgr.IsReady() {
		return nil, errors.NewInternalError(fmt.Errorf("CertMgr not ready"))
	}

	// Send prejoin request to all nodes.
	preJoinReq := &grpc.ClusterPreJoinReq{
		Name:         o.cluster.Name,
		Uuid:         o.cluster.UUID,
		VirtualIp:    o.cluster.Spec.VirtualIP,
		TransportKey: env.CertMgr.MarshalKeyAgreementKey(transportKey),
	}

	// Each node that we invite to join the cluster will provide an individual key-agreement-key
	// Store the key in this map, indexed by node name, so that when later we send over the CertMgr
	// bundle to that particular node we can use the right key-agreement-key to wrap the CA key
	nodeTransportKeys := make(map[string][]byte)
	err = sendPreJoins(nil, preJoinReq, o.cluster.Spec.QuorumNodes, nodeTransportKeys)
	if err != nil {
		return nil, errors.NewBadRequest(err.Error())
	}

	// Send join request to all nodes.
	joinReq := &grpc.ClusterJoinReq{
		Name:         o.cluster.Name,
		Uuid:         o.cluster.UUID,
		VirtualIp:    o.cluster.Spec.VirtualIP,
		QuorumNodes:  o.cluster.Spec.QuorumNodes,
		QuorumConfig: quorumConfig,
		NTPServers:   o.cluster.Spec.NTPServers,
	}

	err = sendJoins(nil, joinReq, o.cluster.Spec.QuorumNodes, nodeTransportKeys)
	if err != nil {
		return nil, errors.NewInternalError(err)
	}
	ts, err := types.TimestampProto(time.Now())
	if err != nil {
		return nil, errors.NewInternalError(err)
	}
	o.cluster.CreationTime.Timestamp = *ts
	o.cluster.ModTime.Timestamp = *ts

	// Store Cluster and Node objects in kv store.
	txn := env.KVStore.NewTxn()
	err = txn.Create(path.Join(globals.ClusterKey, o.cluster.Name), o.cluster)
	if err != nil {
		log.Errorf("Failed to add cluster to txn, error: %v", err)
		sendDisjoins(nil, o.cluster.Spec.QuorumNodes)
		return nil, errors.NewInternalError(err)
	}

	for ii := range o.cluster.Spec.QuorumNodes {
		name := o.cluster.Spec.QuorumNodes[ii]
		node := makeNode(name)
		err = txn.Create(path.Join(globals.NodesKey, name), node)
		if err != nil {
			log.Errorf("Failed to add node %v to txn, error: %v", name, err)
			sendDisjoins(nil, o.cluster.Spec.QuorumNodes)
			return nil, errors.NewInternalError(err)
		}
	}

	if _, err := txn.Commit(context.Background()); err != nil {
		log.Errorf("Failed to commit cluster create txn to kvstore, error: %v", err)
		sendDisjoins(nil, o.cluster.Spec.QuorumNodes)
		return nil, errors.NewInternalError(err)
	}
	log.Infof("Wrote cluster %#v to kvstore", o.cluster)

	// Cluster is formed, we can start Resolver and other authenticated services
	if env.AuthRPCServer == nil {
		go auth.RunAuthServer(":"+env.Options.GRPCAuthPort, nil)
	}

	return o.cluster, nil
}
