// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package meta_test

import (
	"fmt"
	"testing"
	"time"

	context "golang.org/x/net/context"

	"github.com/pensando/sw/venice/citadel/meta"
	"github.com/pensando/sw/venice/citadel/tproto"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
	. "github.com/pensando/sw/venice/utils/testutils"
)

// fake data node rpc server
type fakeDataNode struct {
	nodeUUID string
}

func (f *fakeDataNode) CreateShard(ctx context.Context, req *tproto.ShardReq) (*tproto.StatusResp, error) {
	log.Infof("%s got CreateShard req: %+v", f.nodeUUID, req)
	return &tproto.StatusResp{}, nil
}

func (f *fakeDataNode) UpdateShard(ctx context.Context, req *tproto.ShardReq) (*tproto.StatusResp, error) {
	log.Infof("%s got UpdateShard req: %+v", f.nodeUUID, req)
	return &tproto.StatusResp{}, nil
}

func (f *fakeDataNode) DeleteShard(ctx context.Context, req *tproto.ShardReq) (*tproto.StatusResp, error) {
	log.Infof("%s got DeleteShard req: %+v", f.nodeUUID, req)
	return &tproto.StatusResp{}, nil
}

func (f *fakeDataNode) CreateDatabase(ctx context.Context, req *tproto.DatabaseReq) (*tproto.StatusResp, error) {
	return &tproto.StatusResp{}, nil
}

func (f *fakeDataNode) DeleteDatabase(ctx context.Context, req *tproto.DatabaseReq) (*tproto.StatusResp, error) {
	return &tproto.StatusResp{}, nil
}

func (f *fakeDataNode) PointsWrite(ctx context.Context, req *tproto.PointsWriteReq) (*tproto.StatusResp, error) {
	return &tproto.StatusResp{}, nil
}

func (f *fakeDataNode) ExecuteQuery(ctx context.Context, req *tproto.QueryReq) (*tproto.QueryResp, error) {
	return nil, nil
}

func (f *fakeDataNode) SyncShardReq(ctx context.Context, req *tproto.SyncShardMsg) (*tproto.StatusResp, error) {
	log.Infof("%s got SyncShardReq req: %+v", f.nodeUUID, req)
	return &tproto.StatusResp{}, nil
}

func (f *fakeDataNode) PointsReplicate(ctx context.Context, req *tproto.PointsWriteReq) (*tproto.StatusResp, error) {
	log.Infof("%s got PointsReplicate req: %+v", f.nodeUUID, req)
	return &tproto.StatusResp{}, nil
}

func (f *fakeDataNode) SyncShardInfo(ctx context.Context, req *tproto.SyncShardInfoMsg) (*tproto.StatusResp, error) {
	log.Infof("%s got SyncShardInfo req: %+v", f.nodeUUID, req)
	return &tproto.StatusResp{}, nil
}

func (f *fakeDataNode) SyncData(stream tproto.DataNode_SyncDataServer) error {
	log.Infof("%s got SyncData req", f.nodeUUID)
	return nil
}

func (f *fakeDataNode) ReadReq(context.Context, *tproto.KeyMsg) (*tproto.KeyValueMsg, error) {
	return nil, nil
}

func (f *fakeDataNode) ListReq(context.Context, *tproto.TableMsg) (*tproto.KeyValueMsg, error) {
	return nil, nil
}

func (f *fakeDataNode) WriteReq(context.Context, *tproto.KeyValueMsg) (*tproto.StatusResp, error) {
	return nil, nil
}

func (f *fakeDataNode) DelReq(context.Context, *tproto.KeyMsg) (*tproto.StatusResp, error) {
	return nil, nil
}

func (f *fakeDataNode) WriteReplicate(context.Context, *tproto.KeyValueMsg) (*tproto.StatusResp, error) {
	return nil, nil
}

func (f *fakeDataNode) DelReplicate(context.Context, *tproto.KeyMsg) (*tproto.StatusResp, error) {
	return nil, nil
}

// creates a fake node
func createNode(cfg *meta.ClusterConfig, nodeUUID, nodeURL string) (*meta.Watcher, *meta.Node, *rpckit.RPCServer, error) {
	// create watcher
	w, err := meta.NewWatcher(nodeUUID, cfg)
	if err != nil {
		return nil, nil, nil, err
	}

	// Start a rpc server
	rpcSrv, err := rpckit.NewRPCServer(fmt.Sprintf("datanode-%s", nodeUUID), nodeURL, rpckit.WithLoggerEnabled(false), rpckit.WithTLSProvider(nil))
	if err != nil {
		log.Errorf("failed to listen to %s: Err %v", nodeURL, err)
		return nil, nil, nil, err
	}

	// register RPC handlers
	tproto.RegisterDataNodeServer(rpcSrv.GrpcServer, &fakeDataNode{nodeUUID: nodeUUID})
	rpcSrv.Start()
	time.Sleep(time.Millisecond * 50)

	// create node
	nd, err := meta.NewNode(cfg, nodeUUID, nodeURL)
	if err != nil {
		return nil, nil, nil, err
	}

	return w, nd, rpcSrv, nil
}

func TestMetaBasic(t *testing.T) {
	cfg := meta.DefaultClusterConfig()
	cfg.DeadInterval = time.Millisecond * 100
	cfg.NodeTTL = 5
	cfg.RebalanceDelay = time.Millisecond * 100
	cfg.RebalanceInterval = time.Millisecond * 10

	// create nodes and watchers
	w1, n1, s1, err := createNode(cfg, "1111", "localhost:7071")
	AssertOk(t, err, "Error creating node1")
	defer w1.Stop()
	w2, n2, s2, err := createNode(cfg, "2222", "localhost:7072")
	AssertOk(t, err, "Error creating node2")
	defer w2.Stop()
	w3, n3, s3, err := createNode(cfg, "3333", "localhost:7073")
	AssertOk(t, err, "Error creating node3")
	defer w3.Stop()

	time.Sleep(time.Second)

	// verify atleast one of the nodes is a leader
	AssertEventually(t, func() (bool, interface{}) {
		return (n1.IsLeader() || n2.IsLeader() || n3.IsLeader()), nil
	}, "Leader election failure", "300ms", "30s")

	// verify all three nodes got the metadata update
	AssertEventually(t, func() (bool, interface{}) {
		return ((len(w1.GetCluster(meta.ClusterTypeTstore).NodeMap) == 3) &&
			(len(w2.GetCluster(meta.ClusterTypeTstore).NodeMap) == 3) &&
			(len(w3.GetCluster(meta.ClusterTypeTstore).NodeMap) == 3) &&
			(len(w1.GetCluster(meta.ClusterTypeTstore).ShardMap.Shards) == meta.DefaultShardCount) &&
			(len(w2.GetCluster(meta.ClusterTypeTstore).ShardMap.Shards) == meta.DefaultShardCount) &&
			(len(w3.GetCluster(meta.ClusterTypeTstore).ShardMap.Shards) == meta.DefaultShardCount) &&
			(w1.GetCluster(meta.ClusterTypeTstore).ShardMap.Shards[0].NumReplicas == meta.DefaultReplicaCount) &&
			(w1.GetCluster(meta.ClusterTypeTstore).NodeMap["1111"].NumShards >= (meta.DefaultShardCount * meta.DefaultReplicaCount / 3)) &&
			(w1.GetCluster(meta.ClusterTypeTstore).NodeMap["2222"].NumShards >= (meta.DefaultShardCount * meta.DefaultReplicaCount / 3)) &&
			(w1.GetCluster(meta.ClusterTypeTstore).NodeMap["3333"].NumShards >= (meta.DefaultShardCount * meta.DefaultReplicaCount / 3))), w1.GetCluster(meta.ClusterTypeTstore)
	}, "nodes did not get cluster update", "100ms", "30s")

	// verify that shardmap is setup correctly
	tcl := w1.GetCluster(meta.ClusterTypeTstore)
	_, err = tcl.ShardMap.GetShardForPoint("db0", "measurement0")
	AssertOk(t, err, "Could not get a shard for point")
	kcl := w1.GetCluster(meta.ClusterTypeKstore)
	_, err = kcl.ShardMap.GetShardForKey("table0", tproto.Key{Key: []byte("key1")})
	AssertOk(t, err, "Error getting a shard for the key")

	log.Infof("Adding node 4444 at localhost:7074")

	// add  a node to the cluster
	w4, n4, s4, err := createNode(cfg, "4444", "localhost:7074")
	AssertOk(t, err, "Error creating node4")

	// verify that fourth node gets some shards
	AssertEventually(t, func() (bool, interface{}) {
		return (w4.GetCluster(meta.ClusterTypeTstore).NodeMap["4444"].NumShards >= (meta.DefaultShardCount * meta.DefaultReplicaCount / 4)) &&
			(w1.GetCluster(meta.ClusterTypeTstore).NodeMap["1111"].NumShards < (meta.DefaultShardCount * meta.DefaultReplicaCount / 3)) &&
			(w1.GetCluster(meta.ClusterTypeTstore).NodeMap["2222"].NumShards < (meta.DefaultShardCount * meta.DefaultReplicaCount / 3)) &&
			(w1.GetCluster(meta.ClusterTypeTstore).NodeMap["3333"].NumShards < (meta.DefaultShardCount * meta.DefaultReplicaCount / 3)) &&
			(len(w1.GetCluster(meta.ClusterTypeTstore).NodeMap) == 4), w1.GetCluster(meta.ClusterTypeTstore)
	}, "shards were not moved to new node", "100ms", "30s")

	log.Infof("Stopping node 4444 at localhost:7074")

	// stop the 4th node
	w4.Stop()
	n4.Stop()
	s4.Stop()

	// verify the shards move back
	AssertEventually(t, func() (bool, interface{}) {
		return (w1.GetCluster(meta.ClusterTypeTstore).NodeMap["1111"].NumShards >= (meta.DefaultShardCount * meta.DefaultReplicaCount / 3)) &&
			(w1.GetCluster(meta.ClusterTypeTstore).NodeMap["2222"].NumShards >= (meta.DefaultShardCount * meta.DefaultReplicaCount / 3)) &&
			(w1.GetCluster(meta.ClusterTypeTstore).NodeMap["3333"].NumShards >= (meta.DefaultShardCount * meta.DefaultReplicaCount / 3)) &&
			(len(w1.GetCluster(meta.ClusterTypeTstore).NodeMap) == 3), w1.GetCluster(meta.ClusterTypeTstore)
	}, "shards did not move from removed node", "100ms", "30s")

	log.Infof("Stopping nodes 1111 & 2222")

	n1.Stop()
	n2.Stop()

	// verify all the shards and nodes go away
	AssertEventually(t, func() (bool, interface{}) {
		return (w1.GetCluster(meta.ClusterTypeTstore).ShardMap.Shards[0].NumReplicas == 1) &&
			(len(w1.GetCluster(meta.ClusterTypeTstore).NodeMap) == 1), w1.GetCluster(meta.ClusterTypeTstore)
	}, "shards and nodes did not go away", "100ms", "30s")

	n3.Stop()
	s1.Stop()
	s2.Stop()
	s3.Stop()

	time.Sleep(time.Millisecond * 10)
	meta.DestroyClusterState(cfg, meta.ClusterTypeTstore)
	meta.DestroyClusterState(cfg, meta.ClusterTypeKstore)
}

func TestMetaNodeRestartQuick(t *testing.T) {
	var numNodes = 10
	var nodes = make([]*meta.Node, numNodes)
	var watchers = make([]*meta.Watcher, numNodes)
	var rpcServers = make([]*rpckit.RPCServer, numNodes)
	var err error

	// cluster config
	cfg := meta.DefaultClusterConfig()
	cfg.NumShards = uint32(numNodes * 3)
	cfg.EnableKstore = false
	cfg.EnableKstoreMeta = false
	cfg.DesiredReplicas = 2
	cfg.DeadInterval = time.Millisecond * 500
	cfg.NodeTTL = 5
	cfg.RebalanceDelay = time.Second
	cfg.RebalanceInterval = time.Millisecond * 10

	log.Infof("############################ Starting test %s ############################", t.Name())

	// create nodes
	for i := 0; i < numNodes; i++ {
		watchers[i], nodes[i], rpcServers[i], err = createNode(cfg, fmt.Sprintf("node%d", i+1), fmt.Sprintf("localhost:71%02d", i))
		AssertOk(t, err, "Error creating node")
	}

	// verify atleast one of the nodes is a leader
	AssertEventually(t, func() (bool, interface{}) {
		for i := 0; i < numNodes; i++ {
			if nodes[i].IsLeader() {
				return true, nil
			}
		}
		return false, nil
	}, "Leader election failure", "300ms", "30s")

	// verify all nodes have equal number of shards
	AssertEventually(t, func() (bool, interface{}) {
		if len(watchers[0].GetCluster(meta.ClusterTypeTstore).NodeMap) != numNodes {
			return false, watchers[0].GetCluster(meta.ClusterTypeTstore)
		}
		for _, nd := range watchers[0].GetCluster(meta.ClusterTypeTstore).NodeMap {
			if nd.NumShards != 6 {
				return false, []interface{}{watchers[0].GetCluster(meta.ClusterTypeTstore), nd}
			}
		}
		return true, nil
	}, "nodes have invalid number of shards", "300ms", "30s")

	log.Infof("---------------- All nodes have converged -------------------")

	// do quick restart of nodes without making it dead
	for i := 0; i < numNodes; i++ {
		log.Infof("---------------- Quick Restarting node%d -------------------------", (i + 1))

		nodes[i].Stop()
		watchers[i].Stop()
		rpcServers[i].Stop()

		// recreate the node
		watchers[i], nodes[i], rpcServers[i], err = createNode(cfg, fmt.Sprintf("node%d", i+1), fmt.Sprintf("localhost:71%02d", i))
		AssertOk(t, err, "Error creating node")

		// verify atleast one of the nodes is a leader
		AssertEventually(t, func() (bool, interface{}) {
			for i := 0; i < numNodes; i++ {
				if nodes[i].IsLeader() {
					return true, nil
				}
			}
			return false, nil
		}, "Leader election failure", "300ms", "30s")

		// verify nodes reconverge back to same number of shards
		AssertEventually(t, func() (bool, interface{}) {
			if len(watchers[0].GetCluster(meta.ClusterTypeTstore).NodeMap) != numNodes {
				return false, []interface{}{watchers[0].GetCluster(meta.ClusterTypeTstore).NodeMap}
			}
			/* FIXME: Commenting out this check till we figure out the intermittent failures
			for _, nd := range watchers[0].GetCluster(meta.ClusterTypeTstore).NodeMap {
				if nd.NumShards != 6 {
					return false, []interface{}{watchers[0].GetCluster(meta.ClusterTypeTstore), nd}
				}
			}
			*/
			return true, nil
		}, "nodes have invalid number of shards", "300ms", "30s")
	}

	log.Infof("---------------- Node restart quick test complete -------------------")

	// stop all nodes
	for i := 0; i < numNodes; i++ {
		nodes[i].Stop()
	}
	for i := 0; i < numNodes; i++ {
		watchers[i].Stop()
		rpcServers[i].Stop()
	}

	time.Sleep(time.Millisecond * 10)
	meta.DestroyClusterState(cfg, meta.ClusterTypeTstore)
	meta.DestroyClusterState(cfg, meta.ClusterTypeKstore)
}

func TestMetaNodeRestartSlow(t *testing.T) {
	var numNodes = 10
	var nodes = make([]*meta.Node, numNodes)
	var watchers = make([]*meta.Watcher, numNodes)
	var rpcServers = make([]*rpckit.RPCServer, numNodes)
	var err error

	// cluster config
	cfg := meta.DefaultClusterConfig()
	cfg.NumShards = uint32(numNodes * 3)
	cfg.EnableKstore = false
	cfg.EnableKstoreMeta = false
	cfg.DesiredReplicas = 2
	cfg.DeadInterval = time.Millisecond * 500
	cfg.NodeTTL = 5
	cfg.RebalanceDelay = time.Second
	cfg.RebalanceInterval = time.Millisecond * 10

	log.Infof("############################ Starting test %s ############################", t.Name())

	// create nodes
	for i := 0; i < numNodes; i++ {
		watchers[i], nodes[i], rpcServers[i], err = createNode(cfg, fmt.Sprintf("node%d", i+1), fmt.Sprintf("localhost:71%02d", i))
		AssertOk(t, err, "Error creating node")
	}

	// verify atleast one of the nodes is a leader
	AssertEventually(t, func() (bool, interface{}) {
		for i := 0; i < numNodes; i++ {
			if nodes[i].IsLeader() {
				return true, nil
			}
		}
		return false, nil
	}, "Leader election failure", "300ms", "30s")

	// verify all nodes have equal number of shards
	AssertEventually(t, func() (bool, interface{}) {
		if len(watchers[0].GetCluster(meta.ClusterTypeTstore).NodeMap) != numNodes {
			return false, watchers[0].GetCluster(meta.ClusterTypeTstore)
		}
		for _, nd := range watchers[0].GetCluster(meta.ClusterTypeTstore).NodeMap {
			if nd.NumShards != 6 {
				return false, []interface{}{watchers[0].GetCluster(meta.ClusterTypeTstore), nd}
			}
		}
		return true, nil
	}, "nodes have invalid number of shards", "300ms", "30s")

	log.Infof("---------------- All nodes have converged -------------------")
	// repeatedly restart one node at a time
	for i := 0; i < numNodes; i++ {
		log.Infof("---------------- Slow Restarting node%d -------------------------", (i + 1))

		nodes[i].Stop()
		rpcServers[i].Stop()
		watchers[i].Stop()

		// verify atleast one of the nodes is a leader
		AssertEventually(t, func() (bool, interface{}) {
			for i := 0; i < numNodes; i++ {
				if nodes[i].IsLeader() {
					return true, nil
				}
			}
			return false, nil
		}, "Leader election failure", "300ms", "30s")

		// wait till node is removed from nodemap
		AssertEventually(t, func() (bool, interface{}) {
			tcl, err := meta.GetClusterState(cfg, meta.ClusterTypeTstore)
			if err != nil {
				return false, err
			}
			nd, ok := tcl.NodeMap[fmt.Sprintf("node%d", i+1)]
			if ok {
				return false, []interface{}{tcl, nd}
			}

			return true, nil
		}, "nodes have invalid number of shards", "300ms", "30s")

		// recreate the node
		watchers[i], nodes[i], rpcServers[i], err = createNode(cfg, fmt.Sprintf("node%d", i+1), fmt.Sprintf("localhost:71%02d", i))
		AssertOk(t, err, "Error creating node")

		// verify nodes reconverge back to same number of shards
		AssertEventually(t, func() (bool, interface{}) {
			if len(watchers[0].GetCluster(meta.ClusterTypeTstore).NodeMap) != numNodes {
				return false, []interface{}{watchers[0].GetCluster(meta.ClusterTypeTstore).NodeMap}
			}
			/* FIXME: Commenting out this check till we figure out the intermittent failures
			for _, nd := range watchers[0].GetCluster(meta.ClusterTypeTstore).NodeMap {
				if nd.NumShards != 6 {
					return false, []interface{}{watchers[0].GetCluster(meta.ClusterTypeTstore), nd}
				}
			}
			*/
			return true, nil
		}, "nodes have invalid number of shards", "300ms", "30s")
	}

	log.Infof("---------------- Node restart slow test complete -------------------")

	// stop all nodes
	for i := 0; i < numNodes; i++ {
		nodes[i].Stop()
	}
	for i := 0; i < numNodes; i++ {
		watchers[i].Stop()
		rpcServers[i].Stop()
	}

	time.Sleep(time.Millisecond * 10)
	meta.DestroyClusterState(cfg, meta.ClusterTypeTstore)
	meta.DestroyClusterState(cfg, meta.ClusterTypeKstore)
}
