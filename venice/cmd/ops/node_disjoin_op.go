package ops

import (
	"fmt"

	cmd "github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/cmd/env"
	"github.com/pensando/sw/venice/utils/errors"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/log"
)

// NodeDisjoinOp contains state for Op for node leaving the cluster
type NodeDisjoinOp struct {
	cluster *cmd.Cluster
	node    *cmd.Node
}

// NewNodeDisjoinOp creates an op for a node leaving a cluster.
func NewNodeDisjoinOp(node *cmd.Node) *NodeDisjoinOp {
	return &NodeDisjoinOp{
		node: node,
	}
}

// Validate is a method to validate the op
func (o *NodeDisjoinOp) Validate() error {
	// Check if in cluster.
	if env.MasterService == nil {
		return errors.NewBadRequest(fmt.Sprintf("Not a master to delete node +%v from cluster", o.node.Name))
	}
	return nil
}

// Run executes the cluster leaving steps.
func (o *NodeDisjoinOp) Run() (interface{}, error) {
	_, err := sendDisjoins(nil, []string{o.node.Name})
	if err != nil {
		log.Errorf("error %v sending disjoins to node %v", err, o.node.Name)
	}
	err2 := env.K8sService.DeleteNode(o.node.Name)
	log.Infof("node %v disjoin from cluster. err %v", o.node.Name, err2)
	if err != nil || err2 != nil {
		recorder.Event(eventtypes.NODE_DISJOINED, fmt.Sprintf("Node %s disjoined from cluster", o.node.Name), o.node)
	}
	return o.node.Name, err
}
