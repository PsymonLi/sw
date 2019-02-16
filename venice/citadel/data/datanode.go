// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package data

// this file contains the datanode management code

import (
	"bytes"
	"errors"
	"fmt"
	"io"
	"os"
	"os/user"
	"sync"

	context "golang.org/x/net/context"

	"time"

	"github.com/cenkalti/backoff"

	"github.com/pensando/sw/venice/citadel/kstore"
	"github.com/pensando/sw/venice/citadel/meta"
	"github.com/pensando/sw/venice/citadel/tproto"
	"github.com/pensando/sw/venice/citadel/tstore"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
	"github.com/pensando/sw/venice/utils/safelist"
)

// TshardState is state of a tstore shard
type TshardState struct {
	syncLock    sync.Mutex            // lock for pending sync operations
	replicaID   uint64                // replica id
	shardID     uint64                // shard this replica belongs to
	isPrimary   bool                  // is this primary replica? // FIXME: who updates this when node is disconnected?
	syncPending bool                  // is sync pending on this shard?
	store       *tstore.Tstore        // data store
	syncBuffer  sync.Map              // sync buffer for failed writes
	replicas    []*tproto.ReplicaInfo // other replicas of this shard
	// FIXME: do we need to keep a write log for replicas that are temporarily offline?
}

// KshardState is state of a kstore shard
type KshardState struct {
	syncLock    sync.Mutex            // lock for pending sync operations
	replicaID   uint64                // replica id
	shardID     uint64                // shard this replica belongs to
	isPrimary   bool                  // is this primary replica? // FIXME: who updates this when node is disconnected?
	syncPending bool                  // is sync pending on this shard?
	kstore      kstore.Kstore         // key-value store
	syncBuffer  sync.Map              // sync buffer for failed writes
	replicas    []*tproto.ReplicaInfo // other replicas of this shard
	// FIXME: do we need to keep a write log for replicas that are temporarily offline?
}

// StoreAPI is common api provided by both kstore and tstore
type StoreAPI interface {
	GetShardInfo(sinfo *tproto.SyncShardInfoMsg) error
	RestoreShardInfo(sinfo *tproto.SyncShardInfoMsg) error
	BackupChunk(chunkID uint64, w io.Writer) error
	RestoreChunk(chunkID uint64, r io.Reader) error
}

// DNode represents a backend data node instance
type DNode struct {
	nodeUUID     string // unique id for the data node
	nodeURL      string // URL to reach the data node
	dbPath       string // data store path
	querydbPath  string // query data store path
	clusterCfg   *meta.ClusterConfig
	metaNode     *meta.Node        // pointer to metadata node instance
	watcher      *meta.Watcher     // metadata watcher
	rpcServer    *rpckit.RPCServer // grpc server
	tsQueryStore *tstore.Tstore    // ts query store
	tshards      sync.Map          // tstore shards
	kshards      sync.Map          // kstore shards
	rpcClients   sync.Map          // rpc connections

	isStopped bool // is the datanode stopped?
}

// syncBufferState is the common structure used by ts/kvstore to queue failed writes
// replica id is used as the key to access syncBufferState from the map
type syncBufferState struct {
	ctx         context.Context             // context used by the retry functions
	cancel      context.CancelFunc          // to cancel sync buffer
	wg          sync.WaitGroup              // for the syncbuffer go routine
	nodeUUID    string                      // unique id for the data node
	shardID     uint64                      // shard id
	replicaID   uint64                      // replica id
	clusterType string                      // ts or kv store
	queue       *safelist.SafeList          // pending queue
	backoff     *backoff.ExponentialBackOff // back-off config
}

// NewDataNode creates a new data node instance
func NewDataNode(cfg *meta.ClusterConfig, nodeUUID, nodeURL, dbPath string, querydbPath string) (*DNode, error) {
	// Start a rpc server
	rpcSrv, err := rpckit.NewRPCServer(globals.Citadel, nodeURL, rpckit.WithLoggerEnabled(false))
	if err != nil {
		log.Errorf("failed to listen to %s: Err %v", nodeURL, err)
		return nil, err
	}

	// create watcher
	watcher, err := meta.NewWatcher(nodeUUID, cfg)
	if err != nil {
		return nil, err
	}

	if querydbPath == "" {
		// use the tpmfs dir, /run/user/$uid/qstore
		if u, err := user.Current(); err == nil {
			querydbPath = fmt.Sprintf("/run/user/%s/qstore", u.Uid)
		} else { // pick one from /tmp
			querydbPath = "/tmp/qstore"
		}
	}

	// If nodeURL was passed with :0, then update the nodeURL to the real URL
	nodeURL = rpcSrv.GetListenURL()

	// create a data node
	dn := DNode{
		nodeUUID:    nodeUUID,
		nodeURL:     nodeURL,
		dbPath:      dbPath,
		querydbPath: querydbPath,
		clusterCfg:  cfg,
		watcher:     watcher,
		rpcServer:   rpcSrv,
	}

	// read all shard state from metadata store and restore it
	err = dn.readAllShards(cfg)
	if err != nil {
		log.Errorf("Error reading state from metadata store. Err: %v", err)
		return nil, err
	}

	// register RPC handlers
	tproto.RegisterDataNodeServer(rpcSrv.GrpcServer, &dn)
	rpcSrv.Start()
	log.Infof("Datanode RPC server is listening on: %s", nodeURL)

	// register the node metadata
	metaNode, err := meta.NewNode(cfg, nodeUUID, nodeURL)
	if err != nil {
		log.Errorf("Error creating metanode. Err: %v", err)
		return nil, err
	}
	dn.metaNode = metaNode

	return &dn, nil
}

// getDbPath returns the db path for store type
func (dn *DNode) getDbPath(clusterType string, replicaID uint64) string {
	return fmt.Sprintf("%s/%s/%d", dn.dbPath, clusterType, replicaID)
}

// readAllShards reads all shard state from metadata store and restores state
func (dn *DNode) readAllShards(cfg *meta.ClusterConfig) error {
	// read current state of the cluster and restore the shards
	// FIXME: if etcd was unreachable when we come up we need to handle it
	if cfg.EnableTstore {

		// query aggregator for this data node
		if err := dn.newQueryStore(); err != nil {
			return err
		}

		cluster, err := meta.GetClusterState(cfg, meta.ClusterTypeTstore)
		if err == nil {
			for _, shard := range cluster.ShardMap.Shards {
				for _, repl := range shard.Replicas {
					if repl.NodeUUID == dn.nodeUUID {
						dbPath := dn.getDbPath(meta.ClusterTypeTstore, repl.ReplicaID)
						ts, serr := tstore.NewTstore(dbPath)
						if serr != nil {
							log.Errorf("Error creating tstore at %s. Err: %v", dbPath, serr)
						} else {
							// create a shard instance
							tshard := TshardState{
								replicaID: repl.ReplicaID,
								shardID:   repl.ShardID,
								isPrimary: repl.IsPrimary,
								store:     ts,
							}

							log.Infof("Restored tstore replica %d shard %d", repl.ReplicaID, repl.ShardID)

							// collect all replicas in this shard
							var replicaList []*tproto.ReplicaInfo
							for _, sr := range shard.Replicas {
								// get node for url
								node, nerr := cluster.GetNode(sr.NodeUUID)
								if nerr != nil {
									return nerr
								}

								// build replica info
								rpinfo := tproto.ReplicaInfo{
									ClusterType: meta.ClusterTypeTstore,
									ReplicaID:   sr.ReplicaID,
									ShardID:     sr.ShardID,
									IsPrimary:   sr.IsPrimary,
									NodeUUID:    sr.NodeUUID,
									NodeURL:     node.NodeURL,
								}

								replicaList = append(replicaList, &rpinfo)
							}
							tshard.replicas = replicaList

							// save the datastore
							dn.tshards.Store(repl.ReplicaID, &tshard)
						}
					}
				}
			}
		}
	}
	// restore all kstore shards
	if cfg.EnableKstore {
		cluster, err := meta.GetClusterState(cfg, meta.ClusterTypeKstore)
		if err == nil {
			for _, shard := range cluster.ShardMap.Shards {
				for _, repl := range shard.Replicas {
					if repl.NodeUUID == dn.nodeUUID {
						dbPath := dn.getDbPath(meta.ClusterTypeKstore, repl.ReplicaID)
						ks, serr := kstore.NewKstore(kstore.BoltDBType, dbPath)
						if serr != nil {
							log.Errorf("Error creating kstore at %s. Err: %v", dbPath, serr)
						} else {
							// create a shard instance
							kshard := KshardState{
								replicaID: repl.ReplicaID,
								shardID:   repl.ShardID,
								isPrimary: repl.IsPrimary,
								kstore:    ks,
							}

							log.Infof("Restored kstore replica %d shard %d", repl.ReplicaID, repl.ShardID)

							// collect all replicas in this shard
							var replicaList []*tproto.ReplicaInfo
							for _, sr := range shard.Replicas {
								// get node for url
								node, nerr := cluster.GetNode(sr.NodeUUID)
								if nerr != nil {
									return nerr
								}

								// build replica info
								rpinfo := tproto.ReplicaInfo{
									ClusterType: meta.ClusterTypeKstore,
									ReplicaID:   sr.ReplicaID,
									ShardID:     sr.ShardID,
									IsPrimary:   sr.IsPrimary,
									NodeUUID:    sr.NodeUUID,
									NodeURL:     node.NodeURL,
								}

								replicaList = append(replicaList, &rpinfo)
							}
							kshard.replicas = replicaList

							// save the datastore
							dn.kshards.Store(repl.ReplicaID, &kshard)
						}
					}
				}
			}
		}
	}

	return nil
}

// CreateShard creates a shard
func (dn *DNode) CreateShard(ctx context.Context, req *tproto.ShardReq) (*tproto.StatusResp, error) {
	var resp tproto.StatusResp

	log.Infof("%s Received CreateShard req %+v", dn.nodeUUID, req)

	switch req.ClusterType {
	case meta.ClusterTypeTstore:
		if dn.clusterCfg.EnableTstore {
			// find the shard from replica id
			val, ok := dn.tshards.Load(req.ReplicaID)
			if ok && val.(*TshardState).store != nil {
				log.Warnf("Replica %d already exists", req.ReplicaID)
				return &resp, nil
			}

			// create the data store
			dbPath := dn.getDbPath(meta.ClusterTypeTstore, req.ReplicaID)
			ts, err := tstore.NewTstore(dbPath)
			if err != nil {
				log.Errorf("Error creating tstore at %s. Err: %v", dbPath, err)
				resp.Status = err.Error()
				return &resp, err
			}

			// create a shard instance
			shard := TshardState{
				replicaID: req.ReplicaID,
				shardID:   req.ShardID,
				isPrimary: req.IsPrimary,
				store:     ts,
				replicas:  req.Replicas,
			}

			// save the datastore
			dn.tshards.Store(req.ReplicaID, &shard)
		}
	case meta.ClusterTypeKstore:
		if dn.clusterCfg.EnableKstore {
			// find the shard from replica id
			val, ok := dn.kshards.Load(req.ReplicaID)
			if ok && val.(*KshardState).kstore != nil {
				log.Warnf("Replica %d already exists", req.ReplicaID)
				return &resp, nil
			}

			// create the shard
			dbPath := dn.getDbPath(meta.ClusterTypeKstore, req.ReplicaID)
			ks, err := kstore.NewKstore(kstore.BoltDBType, dbPath)
			if err != nil {
				log.Errorf("Error creating kstore %s. Err: %v", dbPath, err)
				resp.Status = err.Error()
				return &resp, err
			}

			// create a shard instance
			shard := KshardState{
				replicaID: req.ReplicaID,
				shardID:   req.ShardID,
				isPrimary: req.IsPrimary,
				kstore:    ks,
				replicas:  req.Replicas,
			}

			// save the datastore
			dn.kshards.Store(req.ReplicaID, &shard)
		}
	default:
		log.Fatalf("Unknown cluster type :%s.", req.ClusterType)
	}

	return &resp, nil
}

// UpdateShard updates shard info
func (dn *DNode) UpdateShard(ctx context.Context, req *tproto.ShardReq) (*tproto.StatusResp, error) {
	var resp tproto.StatusResp

	log.Infof("%s Received UpdateShard req %+v", dn.nodeUUID, req)

	switch req.ClusterType {
	case meta.ClusterTypeTstore:
		// find the shard from replica id
		val, ok := dn.tshards.Load(req.ReplicaID)
		if !ok || val.(*TshardState).store == nil {
			log.Errorf("Shard %d not found", req.ReplicaID)
			return &resp, errors.New("Shard not found")
		}
		shard := val.(*TshardState)

		// set it as primary
		shard.isPrimary = req.IsPrimary

		// set replicas
		shard.replicas = req.Replicas

		// update sync buffer
		dn.updateSyncBuffer(&shard.syncBuffer, req)

	case meta.ClusterTypeKstore:
		// find the shard from replica id
		val, ok := dn.kshards.Load(req.ReplicaID)
		if !ok || val.(*KshardState).kstore == nil {
			log.Errorf("Shard %d not found", req.ReplicaID)
			return &resp, errors.New("Shard not found")
		}
		shard := val.(*KshardState)

		// set it as primary
		shard.isPrimary = req.IsPrimary

		// set replicas
		shard.replicas = req.Replicas

		// update sync buffer
		dn.updateSyncBuffer(&shard.syncBuffer, req)

	default:
		log.Fatalf("Unknown cluster type :%s.", req.ClusterType)
	}

	return &resp, nil
}

// DeleteShard deletes a shard from the data node
func (dn *DNode) DeleteShard(ctx context.Context, req *tproto.ShardReq) (*tproto.StatusResp, error) {
	var resp tproto.StatusResp

	log.Infof("%s Received DeleteShard req %+v", dn.nodeUUID, req)

	switch req.ClusterType {
	case meta.ClusterTypeTstore:
		// find the shard from replica id
		val, ok := dn.tshards.Load(req.ReplicaID)
		if !ok || val.(*TshardState).store == nil {
			log.Errorf("Shard %d not found", req.ReplicaID)
			return &resp, errors.New("Shard not found")
		}
		shard := val.(*TshardState)
		dn.deleteShardSyncBuffer(&shard.syncBuffer)

		dn.tshards.Delete(req.ReplicaID)

		// acquire sync lock to make sure there are no outstanding sync
		shard.syncLock.Lock()
		defer shard.syncLock.Unlock()

		// close the databse
		err := shard.store.Close()
		if err != nil {
			log.Errorf("Error closing the database for shard %d. Err: %v", req.ReplicaID, err)
		}
		shard.store = nil

		// remove the db files
		dbPath := dn.getDbPath(meta.ClusterTypeTstore, req.ReplicaID)
		os.RemoveAll(dbPath)
	case meta.ClusterTypeKstore:
		// find the shard from replica id
		val, ok := dn.kshards.Load(req.ReplicaID)
		if !ok || val.(*KshardState).kstore == nil {
			log.Errorf("Shard %d not found", req.ReplicaID)
			return &resp, errors.New("Shard not found")
		}
		shard := val.(*KshardState)

		// delete sync buffer
		dn.deleteShardSyncBuffer(&shard.syncBuffer)

		dn.kshards.Delete(req.ReplicaID)

		// acquire sync lock to make sure there are no outstanding sync
		shard.syncLock.Lock()
		defer shard.syncLock.Unlock()

		// close the databse
		err := shard.kstore.Close()
		if err != nil {
			log.Errorf("Error closing the database for shard %d. Err: %v", req.ReplicaID, err)
		}
		shard.kstore = nil

		// remove the db files
		dbPath := dn.getDbPath(meta.ClusterTypeKstore, req.ReplicaID)
		os.RemoveAll(dbPath)
	default:
		log.Fatalf("Unknown cluster type :%s.", req.ClusterType)
	}

	return &resp, nil
}

// SyncShardReq is request message from metadata server to primary
func (dn *DNode) SyncShardReq(ctx context.Context, req *tproto.SyncShardMsg) (*tproto.StatusResp, error) {
	var resp tproto.StatusResp
	var store StoreAPI
	var lock *sync.Mutex

	log.Infof("%s Received SyncShardReq req %+v", dn.nodeUUID, req)

	// handle based on cluster type
	switch req.ClusterType {
	case meta.ClusterTypeTstore:
		// find the data store from shard id
		val, ok := dn.tshards.Load(req.SrcReplicaID)
		if !ok || val.(*TshardState).store == nil {
			log.Errorf("Replica %d not found for cluster %s", req.SrcReplicaID, req.ClusterType)
			return &resp, errors.New("Shard not found")
		}
		shard := val.(*TshardState)
		shard.isPrimary = req.SrcIsPrimary

		// verify we are the primary for this shard
		if !shard.isPrimary {
			log.Errorf("Non-primary received shard sync request. Shard: %+v", shard)
			return &resp, errors.New("Non-primary received shard sync req")
		}

		store = shard.store
		lock = &shard.syncLock

		// update the replicas list
		shard.replicas = req.Replicas
	case meta.ClusterTypeKstore:
		// find the data store from shard id
		val, ok := dn.kshards.Load(req.SrcReplicaID)
		if !ok || val.(*KshardState).kstore == nil {
			log.Errorf("Replica %d not found for cluster %s", req.SrcReplicaID, req.ClusterType)
			return &resp, errors.New("Shard not found")
		}
		shard := val.(*KshardState)
		shard.isPrimary = req.SrcIsPrimary

		// verify we are the primary for this shard
		if !shard.isPrimary {
			log.Errorf("Non-primary received shard sync request. Shard: %+v", shard)
			return &resp, errors.New("Non-primary received shard sync req")
		}
		store = shard.kstore
		lock = &shard.syncLock

		// update the replicas list
		shard.replicas = req.Replicas

	default:
		log.Fatalf("Unknown cluster type :%s.", req.ClusterType)
		return &resp, errors.New("Unknown cluster type")
	}

	// copy data in background
	go dn.syncShard(context.Background(), req, store, lock)

	return &resp, nil
}

// syncShard sync's contents of a shard from source replica to dest replica
func (dn *DNode) syncShard(ctx context.Context, req *tproto.SyncShardMsg, store StoreAPI, lock *sync.Mutex) {
	// lock the shard
	// FIXME: we need to remove this lock and keep sync buffer
	lock.Lock()
	defer lock.Unlock()

	// collect all local shard info
	sinfo := tproto.SyncShardInfoMsg{
		ClusterType:   req.ClusterType,
		SrcReplicaID:  req.SrcReplicaID,
		DestReplicaID: req.DestReplicaID,
		ShardID:       req.ShardID,
	}

	// get the shard info from the store
	err := store.GetShardInfo(&sinfo)
	if err != nil {
		log.Errorf("Error getting shard info from tstore. Err: %v", err)
		return
	}

	// dial a new connection for syncing data
	rclient, err := rpckit.NewRPCClient(fmt.Sprintf("datanode-%s", dn.nodeUUID), req.DestNodeURL, rpckit.WithLoggerEnabled(false), rpckit.WithRemoteServerName(globals.Citadel))
	if err != nil {
		log.Errorf("Error connecting to rpc server %s. err: %v", req.DestNodeURL, err)
		return
	}
	defer rclient.Close()

	// send shard info message to destination
	dnclient := tproto.NewDataNodeClient(rclient.ClientConn)
	_, err = dnclient.SyncShardInfo(ctx, &sinfo)
	if err != nil {
		log.Errorf("Error sending sync shard info message to %s. Err: %v", req.DestNodeURL, err)
		return
	}

	// send stream of sync messages
	stream, err := dnclient.SyncData(ctx)
	if err != nil {
		log.Errorf("Error syncing data to %s. Err: %v", req.DestNodeURL, err)
		return
	}

	// walk thru all chunks and send them over the stream
	for _, chunkInfo := range sinfo.Chunks {
		var buf bytes.Buffer

		// backup chunk
		err = store.BackupChunk(chunkInfo.ChunkID, &buf)
		if err != nil {
			log.Errorf("Error backing up chunk %d. Err: %v", chunkInfo.ChunkID, err)
			return
		}

		// create the message
		msg := tproto.SyncDataMsg{
			ClusterType:   req.ClusterType,
			SrcReplicaID:  req.SrcReplicaID,
			DestReplicaID: req.DestReplicaID,
			ShardID:       req.ShardID,
			ChunkID:       chunkInfo.ChunkID,
			Data:          buf.Bytes(),
		}

		log.Infof("Syncing data from replica %d chunk %d len: %d", req.SrcReplicaID, chunkInfo.ChunkID, len(msg.Data))

		// send the message
		err = stream.Send(&msg)
		if err != nil {
			log.Errorf("Error sending chunk %d. Err: %v", chunkInfo.ChunkID, err)
			return
		}
	}

	// close the stream
	_, err = stream.CloseAndRecv()
	if err != nil {
		log.Errorf("Error closing sync channel. Err: %v", err)
		return
	}
}

// SyncShardInfo contains the shard info about a shard thats about to be synced
func (dn *DNode) SyncShardInfo(ctx context.Context, req *tproto.SyncShardInfoMsg) (*tproto.StatusResp, error) {
	var resp tproto.StatusResp
	var store StoreAPI

	log.Infof("%s Received SyncShardInfo req %+v", dn.nodeUUID, req)

	switch req.ClusterType {
	case meta.ClusterTypeTstore:
		// find the data store from replica id
		val, ok := dn.tshards.Load(req.DestReplicaID)
		if !ok || val.(*TshardState).store == nil {
			log.Errorf("Shard %d not found", req.DestReplicaID)
			return nil, errors.New("Shard not found")
		}
		shard := val.(*TshardState)
		store = shard.store

		// if there are chunks to be synced, set the sync pending flag
		if len(req.Chunks) != 0 {
			shard.syncPending = true
		}
	case meta.ClusterTypeKstore:
		// find the data store from shard id
		val, ok := dn.kshards.Load(req.DestReplicaID)
		if !ok || val.(*KshardState).kstore == nil {
			log.Errorf("Shard %d not found", req.DestReplicaID)
			return nil, errors.New("Shard not found")
		}
		shard := val.(*KshardState)
		store = shard.kstore

		// if there are chunks to be synced, set the sync pending flag
		if len(req.Chunks) != 0 {
			shard.syncPending = true
		}
	default:
		log.Fatalf("Unknown cluster type :%s.", req.ClusterType)
	}

	// restore the local metadata
	err := store.RestoreShardInfo(req)
	if err != nil {
		log.Errorf("Error restoring local metadata. Err: %v", err)
		return nil, err
	}

	return &resp, nil
}

// SyncData is streaming sync message from primary to replica
func (dn *DNode) SyncData(stream tproto.DataNode_SyncDataServer) error {
	var store StoreAPI

	log.Infof("%s Received SyncData req", dn.nodeUUID)

	// loop till all the message are received
	for {
		// keep receiving
		req, err := stream.Recv()
		if err == io.EOF {
			// send success at the end of the message
			return stream.SendAndClose(&tproto.StatusResp{})
		}
		if err != nil {
			log.Errorf("Error receiving from stream. Err: %v", err)
			return err
		}

		log.Infof("%s Restoring %s Chunk %d on shard %d", dn.nodeUUID, req.ClusterType, req.ChunkID, req.ShardID)

		switch req.ClusterType {
		case meta.ClusterTypeTstore:
			// find the data store from shard id
			val, ok := dn.tshards.Load(req.DestReplicaID)
			if !ok || val.(*TshardState).store == nil {
				log.Errorf("Shard %d not found", req.SrcReplicaID)
				return errors.New("Shard not found")
			}
			shard := val.(*TshardState)
			store = shard.store

			// clear sync pending flag
			defer func() { shard.syncPending = false }()
		case meta.ClusterTypeKstore:
			// find the data store from shard id
			val, ok := dn.kshards.Load(req.DestReplicaID)
			if !ok || val.(*KshardState).kstore == nil {
				log.Errorf("Shard %d not found", req.SrcReplicaID)
				return errors.New("Shard not found")
			}
			shard := val.(*KshardState)
			store = shard.kstore

			// clear sync pending flag
			defer func() { shard.syncPending = false }()
		default:
			log.Fatalf("Unknown cluster type :%s.", req.ClusterType)
		}

		// restore
		err = store.RestoreChunk(req.ChunkID, bytes.NewBuffer(req.Data))
		if err != nil {
			log.Errorf("Error restoring chunk %d. Err: %v", req.ChunkID, err)
			return err
		}
	}
}

// GetCluster returns cluster state from the watcher
func (dn *DNode) GetCluster(clusterType string) *meta.TscaleCluster {
	return dn.watcher.GetCluster(clusterType)
}

// IsLeader returns true if this node is the leader node
func (dn *DNode) IsLeader() bool {
	return dn.metaNode.IsLeader()
}

// IsStopped returns true if data node is stopped
func (dn *DNode) IsStopped() bool {
	return dn.isStopped
}

// HasPendingSync checks if any shards on this node has sync pending
func (dn *DNode) HasPendingSync() bool {
	var retVal bool
	dn.tshards.Range(func(key interface{}, value interface{}) bool {
		shard := value.(*TshardState)
		shard.syncLock.Lock()
		defer shard.syncLock.Unlock()
		if shard.syncPending {
			log.Infof("Datanode %s tstore shard %d replica %d has sync pending", dn.nodeUUID, shard.shardID, shard.replicaID)
			retVal = true
		}
		return true
	})
	dn.kshards.Range(func(key interface{}, value interface{}) bool {
		shard := value.(*KshardState)
		shard.syncLock.Lock()
		defer shard.syncLock.Unlock()
		if shard.syncPending {
			log.Infof("Datanode %s kstore shard %d replica %d has sync pending", dn.nodeUUID, shard.shardID, shard.replicaID)
			retVal = true
		}
		return true
	})

	return retVal
}

// SoftRestart does a soft restart of the nodes without releasing the lease
// Note: this is to be used by testing purposes only
func (dn *DNode) SoftRestart() error {
	var err error

	// first stop everythig
	dn.isStopped = true
	dn.tsQueryStore.Close()
	dn.watcher.Stop()
	dn.tshards.Range(func(key interface{}, value interface{}) bool {
		shard := value.(*TshardState)
		// clean up syncbuffer
		dn.deleteShardSyncBuffer(&shard.syncBuffer)

		if shard.store != nil {
			shard.store.Close()
			shard.store = nil
		}
		return true
	})
	dn.kshards.Range(func(key interface{}, value interface{}) bool {
		shard := value.(*KshardState)
		// clean up syncbuffer
		dn.deleteShardSyncBuffer(&shard.syncBuffer)
		if shard.kstore != nil {
			shard.kstore.Close()
			shard.kstore = nil
		}
		return true
	})

	// close the rpc clients
	dn.rpcClients.Range(func(key interface{}, value interface{}) bool {
		rc, ok := value.(*rpckit.RPCClient)
		if ok {
			rc.Close()
			dn.rpcClients.Delete(key)
		}
		return true
	})

	// stop rpc server
	dn.rpcServer.Stop()

	// restart everything back up
	dn.isStopped = false

	// Start a rpc server
	dn.rpcServer, err = rpckit.NewRPCServer(globals.Citadel, dn.nodeURL, rpckit.WithLoggerEnabled(false))
	if err != nil {
		log.Errorf("failed to listen to %s: Err %v", dn.nodeURL, err)
		return err
	}

	// create watcher
	dn.watcher, err = meta.NewWatcher(dn.nodeUUID, dn.clusterCfg)
	if err != nil {
		return err
	}

	// register RPC handlers
	tproto.RegisterDataNodeServer(dn.rpcServer.GrpcServer, dn)
	dn.rpcServer.Start()
	log.Infof("Datanode RPC server is listening on: %s", dn.nodeURL)

	// read all shard state from metadata store and restore it
	err = dn.readAllShards(dn.clusterCfg)
	if err != nil {
		log.Errorf("Error reading state from metadata store. Err: %v", err)
		return err
	}

	return nil
}

// Stop stops the data node
func (dn *DNode) Stop() error {
	dn.isStopped = true
	dn.tsQueryStore.Close()
	dn.metaNode.Stop()
	dn.watcher.Stop()
	dn.tshards.Range(func(key interface{}, value interface{}) bool {
		shard := value.(*TshardState)
		// clean up sync buffer
		dn.deleteShardSyncBuffer(&shard.syncBuffer)

		if shard.store != nil {
			shard.store.Close()
			shard.store = nil
		}
		return true
	})
	dn.kshards.Range(func(key interface{}, value interface{}) bool {
		shard := value.(*KshardState)
		// clean up sync buffer
		dn.deleteShardSyncBuffer(&shard.syncBuffer)
		if shard.kstore != nil {
			shard.kstore.Close()
			shard.kstore = nil
		}
		return true
	})

	// close the rpc clients
	dn.rpcClients.Range(func(key interface{}, value interface{}) bool {
		rc, ok := value.(*rpckit.RPCClient)
		if ok {
			rc.Close()
			dn.rpcClients.Delete(key)
		}
		return true
	})

	dn.rpcServer.Stop()

	return nil
}

// ------------- internal functions --------

// getDnclient returns a grpc client
func (dn *DNode) getDnclient(clusterType, nodeUUID string) (tproto.DataNodeClient, error) {
	// if we already have a connection, just return it
	eclient, ok := dn.rpcClients.Load(nodeUUID)
	if ok {
		return tproto.NewDataNodeClient(eclient.(*rpckit.RPCClient).ClientConn), nil
	}

	// find the node
	cl := dn.watcher.GetCluster(clusterType)
	node, err := cl.GetNode(nodeUUID)
	if err != nil {
		return nil, err
	}

	// dial the connection
	rclient, err := rpckit.NewRPCClient(fmt.Sprintf("datanode-%s", dn.nodeUUID), node.NodeURL, rpckit.WithLoggerEnabled(false), rpckit.WithRemoteServerName(globals.Citadel))
	if err != nil {
		log.Errorf("Error connecting to rpc server %s. err: %v", node.NodeURL, err)
		return nil, err
	}
	dn.rpcClients.Store(nodeUUID, rclient)

	return tproto.NewDataNodeClient(rclient.ClientConn), nil
}

// reconnectDnclient close existing rpc client and try reconnecting
func (dn *DNode) reconnectDnclient(clusterType, nodeUUID string) (tproto.DataNodeClient, error) {
	// close and delete existing rpc client
	eclient, ok := dn.rpcClients.Load(nodeUUID)
	if ok {
		eclient.(*rpckit.RPCClient).Close()
		dn.rpcClients.Delete(nodeUUID)
	}

	return dn.getDnclient(clusterType, nodeUUID)
}

// retry ts/kv write based on the cluster type
func (dn *DNode) replicateFailedRequest(sb *syncBufferState) error {
	switch sb.clusterType {
	case meta.ClusterTypeTstore:
		return dn.replicateFailedPoints(sb)
	case meta.ClusterTypeKstore:
		return dn.replicateFailedWrite(sb)
	default:
		log.Fatalf("unknown cluster type: %+v in sync buffer", sb)
	}
	return nil
}

// go routine to process the pending requests
func (dn *DNode) processPendingQueue(sb *syncBufferState) {
	defer sb.wg.Done()

	// forever
	for {
		wait := sb.backoff.NextBackOff()
		// we don't stop the back off timer
		if wait == backoff.Stop {
			log.Errorf("abort processing sync buffer queue due to timeout, %+v", sb)
			return
		}

		select {
		case <-time.After(wait):
			if err := dn.replicateFailedRequest(sb); err != nil {
				sb.backoff.NextBackOff()

			} else { // reset
				sb.backoff.Reset()
			}

		case <-sb.ctx.Done():
			// try one more time
			if err := dn.replicateFailedRequest(sb); err != nil {
				log.Errorf("sync buffer failed to replicate writes, discard queue, length:%d, {%+v}", sb.queue.Len(), sb)
			}
			log.Infof("exit sync buffer processing, %+v", sb)
			return
		}
	}
}

// addSyncBuffer queues failed writes and triggers retry routine
func (dn *DNode) addSyncBuffer(sm *sync.Map, nodeUUID string, req interface{}) error {
	var sb *syncBufferState
	var clusterType string
	var replicaID uint64
	var shardID uint64

	switch v := req.(type) {
	case *tproto.PointsWriteReq:
		clusterType = meta.ClusterTypeTstore
		replicaID = v.ReplicaID
		shardID = v.ShardID

	case *tproto.KeyValueMsg:
		clusterType = meta.ClusterTypeKstore
		replicaID = v.ReplicaID
		shardID = v.ShardID
	default:
		log.Fatalf("invalid cluster type in sync buffer req %T", v)
	}

	if sbinter, ok := sm.Load(replicaID); ok {
		sb = sbinter.(*syncBufferState)
	} else {
		ctx, cancel := context.WithCancel(context.Background())
		// add new entry with exponential back off
		sb = &syncBufferState{
			ctx:         ctx,
			cancel:      cancel,
			queue:       safelist.New(),
			backoff:     backoff.NewExponentialBackOff(),
			nodeUUID:    nodeUUID,
			replicaID:   replicaID,
			shardID:     shardID,
			clusterType: clusterType,
		}
		sb.backoff.InitialInterval = 2 * time.Second
		sb.backoff.RandomizationFactor = 0.2
		sb.backoff.Multiplier = 2
		sb.backoff.MaxInterval = meta.DefaultNodeDeadInterval / 2
		sb.backoff.MaxElapsedTime = 0 // run forever
		sb.backoff.Reset()

		log.Infof("%s created sync buffer for cluster-type:%s, replica id:%v", dn.nodeUUID, sb.clusterType, replicaID)

		// store new entry
		sm.Store(replicaID, sb)
		sb.wg.Add(1)
		go dn.processPendingQueue(sb)
	}

	sb.queue.Insert(req)
	return nil
}

// updateSyncBuffer clean up the sync buffer based on the updated shard
func (dn *DNode) updateSyncBuffer(sm *sync.Map, req *tproto.ShardReq) error {
	log.Infof("%s sync buffer received update request: %+v", dn.nodeUUID, req)
	newReplicaMap := map[uint64]*tproto.ReplicaInfo{}

	// not primary ? delete all replica ids of this shard
	if req.IsPrimary == false {
		return dn.deleteShardSyncBuffer(sm)
	}

	// primary replica ? update the sync buffer
	for _, e := range req.Replicas {
		newReplicaMap[e.ReplicaID] = e
	}

	replicaToDel := []uint64{}
	sm.Range(func(key, val interface{}) bool {
		replicaID := key.(uint64)
		// select replica ids that got removed
		if _, ok := newReplicaMap[replicaID]; !ok {
			replicaToDel = append(replicaToDel, replicaID)
		}
		return true
	})

	log.Infof("delete replica ids from sync buffer %+v", replicaToDel)
	for _, r := range replicaToDel {
		dn.deleteSyncBuffer(sm, r)
	}

	return nil
}

// deleteSyncBuffer deletes replica id from sync buffer
func (dn *DNode) deleteSyncBuffer(sm *sync.Map, replicaID uint64) error {
	log.Infof("%s sync buffer  received delete request for replica %v", dn.nodeUUID, replicaID)
	sbinter, ok := sm.Load(replicaID)
	if !ok {
		return nil
	}
	sb := sbinter.(*syncBufferState)
	// stop the go routine
	sb.cancel()
	sb.wg.Wait()
	sm.Delete(replicaID)

	// empty the queue
	sb.queue.RemoveTill(func(i int, v interface{}) bool {
		return true
	})
	return nil
}

// deleteShardSyncBuffer deletes all sync buffer in the shard
func (dn *DNode) deleteShardSyncBuffer(sm *sync.Map) error {
	replicaList := []uint64{}

	sm.Range(func(k interface{}, v interface{}) bool {
		replicaID, ok := k.(uint64)
		if ok {
			replicaList = append(replicaList, replicaID)
		}
		return true
	})

	log.Infof("%s sync buffer received delete all request, replica-ids: %+v", dn.nodeUUID, replicaList)
	for _, k := range replicaList {
		dn.deleteSyncBuffer(sm, k)
	}
	return nil
}
