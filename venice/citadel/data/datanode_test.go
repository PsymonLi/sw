// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package data

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"math/rand"
	"os"
	"runtime"
	"testing"
	"time"

	"github.com/influxdata/influxdb/query"

	_ "github.com/influxdata/influxdb/tsdb/engine"
	_ "github.com/influxdata/influxdb/tsdb/index"

	"sync"

	influxmeta "github.com/influxdata/influxdb/services/meta"

	"github.com/pensando/sw/venice/citadel/meta"
	"github.com/pensando/sw/venice/citadel/tproto"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
	. "github.com/pensando/sw/venice/utils/testutils"
)

// createNode creates a node
func createNode(nodeUUID, nodeURL, dbPath string, qdbPath string, logger log.Logger) (*DNode, error) {
	// metadata config to disable metadata mgr
	cfg := meta.DefaultClusterConfig()
	cfg.EnableKstoreMeta = false
	cfg.EnableTstoreMeta = false

	// create the data node
	dn, err := NewDataNode(cfg, nodeUUID, nodeURL, dbPath, qdbPath, logger)
	if err != nil {
		return nil, err
	}

	// stop the metadata service for this test
	dn.metaNode.Stop()

	return dn, nil
}

func TestQueryStore(t *testing.T) {
	logger := log.GetNewLogger(log.GetDefaultConfig(t.Name()))

	// no default db dir in OSX
	if runtime.GOOS == "darwin" {
		return
	}

	// create a temp dir
	path, err := ioutil.TempDir("", "tstore-")
	AssertOk(t, err, "Error creating tmp dir")
	defer os.RemoveAll(path)

	// create nodes
	dnodes, err := createNode("node-110", "localhost:17308", path, "", logger)
	AssertOk(t, err, "Error creating nodes")
	defer dnodes.Stop()
}

func TestDataNodeBasic(t *testing.T) {
	const numNodes = 3
	dnodes := make([]*DNode, numNodes)
	clients := make([]*rpckit.RPCClient, numNodes)
	var err error
	logger := log.GetNewLogger(log.GetDefaultConfig(t.Name()))

	// create a temp dir
	path, err := ioutil.TempDir("", "kstore-")
	AssertOk(t, err, "Error creating tmp dir")
	defer os.RemoveAll(path)

	qpath, err := ioutil.TempDir("", "qstore-")
	AssertOk(t, err, "Error creating tmp dir")
	defer os.RemoveAll(path)

	// create nodes
	for idx := 0; idx < numNodes; idx++ {
		dnodes[idx], err = createNode(fmt.Sprintf("node-%d", idx), fmt.Sprintf("localhost:730%d", idx), fmt.Sprintf("%s/%d", path, idx), fmt.Sprintf("%s/%d", qpath, idx), logger)
		AssertOk(t, err, "Error creating nodes")
		defer dnodes[idx].Stop()
	}

	// create rpc client
	for idx := 0; idx < numNodes; idx++ {
		clients[idx], err = rpckit.NewRPCClient(fmt.Sprintf("datanode-%d", idx), fmt.Sprintf("localhost:730%d", idx), rpckit.WithLoggerEnabled(false))
		AssertOk(t, err, "Error connecting to grpc server")
		defer clients[idx].Close()
	}

	// make create shard api call
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		req := tproto.ShardReq{
			ClusterType: meta.ClusterTypeTstore,
			ReplicaID:   uint64(idx + 1),
			IsPrimary:   true,
		}

		// create tstore shard
		resp, err := dnclient.CreateShard(context.Background(), &req)
		AssertOk(t, err, "Error making the create shard call")
		Assert(t, resp.Status == "", "Invalid resp to create shard req", resp)

		// create kstore shard
		req.ClusterType = meta.ClusterTypeKstore
		_, err = dnclient.CreateShard(context.Background(), &req)
		AssertOk(t, err, "Error making the create shard call")
	}

	// make create database call
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		req := tproto.DatabaseReq{
			ClusterType: meta.ClusterTypeTstore,
			ReplicaID:   uint64(idx + 1),
			Database:    "db0",
		}

		_, err := dnclient.CreateDatabase(context.Background(), &req)
		AssertOk(t, err, "Error making the create database call")

		var dbInfo []*influxmeta.DatabaseInfo
		AssertEventually(t, func() (bool, interface{}) {
			resp, err := dnclient.ReadDatabases(context.Background(), &req)
			if err != nil {
				return false, err
			}
			if err = json.Unmarshal([]byte(resp.Status), &dbInfo); err != nil {
				return false, err
			}
			if len(dbInfo) != 1 {
				return false, dbInfo
			}

			for _, db := range dbInfo {
				if db.Name != "db0" {
					return false, db
				}
			}
			return true, nil
		}, "failed to read db")
	}

	// write some points
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		data := fmt.Sprintf("cpu,host=serverB,svc=nginx value1=11,value2=12 %v\n"+
			"cpu,host=serverC,svc=nginx value1=21,value2=22  %v\n", time.Now().UnixNano(), time.Now().UnixNano())

		req := tproto.PointsWriteReq{
			ClusterType: meta.ClusterTypeTstore,
			ReplicaID:   uint64(idx + 1),
			Database:    "db0",
			Points:      data,
		}

		_, err := dnclient.PointsWrite(context.Background(), &req)
		AssertOk(t, err, "Error writing points")
	}

	// execute some query
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		req := tproto.QueryReq{
			ClusterType: meta.ClusterTypeTstore,
			ReplicaID:   uint64(idx + 1),
			Database:    "db0",
			TxnID:       uint64(idx + 1),
			Query:       "SELECT * FROM cpu",
		}

		resp, err := dnclient.ExecuteQuery(context.Background(), &req)
		AssertOk(t, err, "Error execuitng query")
		log.Infof("Got query resp: %+v", resp)
	}

	// make delete database call
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		req := tproto.DatabaseReq{
			ClusterType: meta.ClusterTypeTstore,
			ReplicaID:   uint64(idx + 1),
			Database:    "db0",
		}

		_, err := dnclient.DeleteDatabase(context.Background(), &req)
		AssertOk(t, err, "Error making the delete database call")
	}

	// write some keys
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		kvl := []*tproto.KeyValue{
			{
				Key:   []byte("testKey1"),
				Value: []byte("testValue1"),
			},
			{
				Key:   []byte("testKey2"),
				Value: []byte("testValue2"),
			},
		}

		// build the request
		req := tproto.KeyValueMsg{
			ClusterType: meta.ClusterTypeKstore,
			ReplicaID:   uint64(idx + 1),
			ShardID:     uint64(idx + 1),
			Table:       "table0",
			Kvs:         kvl,
		}

		_, err := dnclient.WriteReq(context.Background(), &req)
		AssertOk(t, err, "Error writing kvs")
	}

	// read a key back
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		// keys to get
		kl := []*tproto.Key{
			{
				Key: []byte("testKey1"),
			},
			{
				Key: []byte("testKey2"),
			},
		}

		// build the request
		req := tproto.KeyMsg{
			ClusterType: meta.ClusterTypeKstore,
			ReplicaID:   uint64(idx + 1),
			ShardID:     uint64(idx + 1),
			Table:       "table0",
			Keys:        kl,
		}

		resp, err := dnclient.ReadReq(context.Background(), &req)
		AssertOk(t, err, "Error reading the keys back")
		log.Infof("Gor read response: %+v", resp)
	}

	// list all keys in a table
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		req := tproto.TableMsg{
			ClusterType: meta.ClusterTypeKstore,
			ReplicaID:   uint64(idx + 1),
			ShardID:     uint64(idx + 1),
			Table:       "table0",
		}

		resp, err := dnclient.ListReq(context.Background(), &req)
		AssertOk(t, err, "Error listing the keys")
		log.Infof("Gor read response: %+v", resp)
	}

	// delete the keys
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		// keys to delete
		kl := []*tproto.Key{
			{
				Key: []byte("testKey1"),
			},
			{
				Key: []byte("testKey2"),
			},
		}

		// build the request
		req := tproto.KeyMsg{
			ClusterType: meta.ClusterTypeKstore,
			ReplicaID:   uint64(idx + 1),
			ShardID:     uint64(idx + 1),
			Table:       "table0",
			Keys:        kl,
		}

		_, err := dnclient.DelReq(context.Background(), &req)
		AssertOk(t, err, "Error deleting the keys")
	}
}

func TestDataNodeErrors(t *testing.T) {
	const numNodes = 3
	dnodes := make([]*DNode, numNodes)
	clients := make([]*rpckit.RPCClient, numNodes)
	var err error
	logger := log.GetNewLogger(log.GetDefaultConfig(t.Name()))

	// create a temp dir
	path, err := ioutil.TempDir("", "kstore-")
	AssertOk(t, err, "Error creating tmp dir")
	defer os.RemoveAll(path)

	qpath, err := ioutil.TempDir("", "qstore-")
	AssertOk(t, err, "Error creating tmp dir")
	defer os.RemoveAll(path)

	// create nodes
	for idx := 0; idx < numNodes; idx++ {
		dnodes[idx], err = createNode(fmt.Sprintf("node-%d", idx), fmt.Sprintf("localhost:730%d", idx), fmt.Sprintf("%s/%d", path, idx), fmt.Sprintf("%s/%d", qpath, idx), logger)
		AssertOk(t, err, "Error creating nodes")
		defer dnodes[idx].Stop()
	}

	// create rpc client
	for idx := 0; idx < numNodes; idx++ {
		clients[idx], err = rpckit.NewRPCClient(fmt.Sprintf("datanode-%d", idx), fmt.Sprintf("localhost:730%d", idx), rpckit.WithLoggerEnabled(false))
		AssertOk(t, err, "Error connecting to grpc server")
		defer clients[idx].Close()
	}

	// make create shard api call
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		req := tproto.ShardReq{
			ClusterType: meta.ClusterTypeTstore,
			ReplicaID:   uint64(idx + 1),
			IsPrimary:   true,
		}

		// create tstore shard
		resp, err := dnclient.CreateShard(context.Background(), &req)
		AssertOk(t, err, "Error making the create shard call")
		Assert(t, resp.Status == "", "Invalid resp to create shard req", resp)

		// create kstore shard
		req.ClusterType = meta.ClusterTypeKstore
		_, err = dnclient.CreateShard(context.Background(), &req)
		AssertOk(t, err, "Error making the create shard call")
	}

	// update & delete shard call with invalid params
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		req := tproto.ShardReq{
			ClusterType: meta.ClusterTypeTstore,
			ReplicaID:   300,
			IsPrimary:   true,
		}

		// send update shard
		_, err := dnclient.UpdateShard(context.Background(), &req)
		Assert(t, (err != nil), "update shard with invalid params suceeded")
		_, err = dnclient.DeleteShard(context.Background(), &req)
		Assert(t, (err != nil), "delete shard with invalid params suceeded")
		req.ClusterType = meta.ClusterTypeKstore
		_, err = dnclient.UpdateShard(context.Background(), &req)
		Assert(t, (err != nil), "update shard with invalid params suceeded")
		_, err = dnclient.DeleteShard(context.Background(), &req)
		Assert(t, (err != nil), "delete shard with invalid params suceeded")
	}

	// sync shard messages with invalid params
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		req := tproto.SyncShardMsg{
			ClusterType:  meta.ClusterTypeTstore,
			SrcReplicaID: 300,
		}

		// send the message
		_, err := dnclient.SyncShardReq(context.Background(), &req)
		Assert(t, (err != nil), "SyncShardReq with invalid params suceeded")
		req.ClusterType = meta.ClusterTypeKstore
		_, err = dnclient.SyncShardReq(context.Background(), &req)
		Assert(t, (err != nil), "SyncShardReq with invalid params suceeded")
	}

	// sync shard info messages with invalid params
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		req := tproto.SyncShardInfoMsg{
			ClusterType:   meta.ClusterTypeTstore,
			SrcReplicaID:  300,
			DestReplicaID: 300,
		}

		// send the message
		_, err := dnclient.SyncShardInfo(context.Background(), &req)
		Assert(t, (err != nil), "SyncShardInfo with invalid params suceeded")
		req.ClusterType = meta.ClusterTypeKstore
		_, err = dnclient.SyncShardInfo(context.Background(), &req)
		Assert(t, (err != nil), "SyncShardInfo with invalid params suceeded")
	}

	// create database with invalid params
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		req := tproto.DatabaseReq{
			ClusterType: meta.ClusterTypeKstore,
			ReplicaID:   uint64(idx + 1),
			Database:    "db0",
		}

		_, err := dnclient.CreateDatabase(context.Background(), &req)
		Assert(t, (err != nil), "create database with invalid params suceeded")
		req.ClusterType = meta.ClusterTypeTstore
		req.ReplicaID = 100
		_, err = dnclient.CreateDatabase(context.Background(), &req)
		Assert(t, (err != nil), "create database with invalid params suceeded")
		_, err = dnclient.DeleteDatabase(context.Background(), &req)
		Assert(t, (err != nil), "delete database with invalid params suceeded")
	}

	// write points with invalid params
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		data := "cpu,host=serverB,svc=nginx value1=11,value2=12 10\n" +
			"cpu,host=serverC,svc=nginx value1=21,value2=22  20\n"

		req := tproto.PointsWriteReq{
			ClusterType: meta.ClusterTypeKstore,
			ReplicaID:   uint64(idx + 1),
			Database:    "db0",
			Points:      data,
		}

		_, err := dnclient.PointsWrite(context.Background(), &req)
		Assert(t, (err != nil), "points write with invalid params suceeded")
		req.ClusterType = meta.ClusterTypeTstore
		req.ReplicaID = 300
		_, err = dnclient.PointsWrite(context.Background(), &req)
		Assert(t, (err != nil), "points write with invalid params suceeded")
	}

	// execute some query
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		req := tproto.QueryReq{
			ClusterType: meta.ClusterTypeKstore,
			ReplicaID:   uint64(idx + 1),
			Database:    "db0",
			TxnID:       uint64(idx + 1),
			Query:       "SELECT * FROM cpu",
		}

		_, err := dnclient.ExecuteQuery(context.Background(), &req)
		Assert(t, (err != nil), "query with invalid params suceeded")
		req.ClusterType = meta.ClusterTypeTstore
		req.ReplicaID = 300
		_, err = dnclient.ExecuteQuery(context.Background(), &req)
		Assert(t, (err != nil), "query with invalid params suceeded")
	}

	// write keys with invalid params
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		kvl := []*tproto.KeyValue{
			{
				Key:   []byte("testKey1"),
				Value: []byte("testValue1"),
			},
			{
				Key:   []byte("testKey2"),
				Value: []byte("testValue2"),
			},
		}

		// build the request
		req := tproto.KeyValueMsg{
			ClusterType: meta.ClusterTypeTstore,
			ReplicaID:   uint64(idx + 1),
			ShardID:     uint64(idx + 1),
			Table:       "table0",
			Kvs:         kvl,
		}

		_, err := dnclient.WriteReq(context.Background(), &req)
		Assert(t, (err != nil), "kv write with invalid params suceeded")
		req.ClusterType = meta.ClusterTypeKstore
		req.ReplicaID = 300
		_, err = dnclient.WriteReq(context.Background(), &req)
		Assert(t, (err != nil), "kv write with invalid params suceeded")
	}

	// read a key with invalid params
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		// keys to get
		kl := []*tproto.Key{
			{
				Key: []byte("testKey1"),
			},
			{
				Key: []byte("testKey2"),
			},
		}

		// build the request
		req := tproto.KeyMsg{
			ClusterType: meta.ClusterTypeTstore,
			ReplicaID:   uint64(idx + 1),
			ShardID:     uint64(idx + 1),
			Table:       "table0",
			Keys:        kl,
		}

		_, err := dnclient.ReadReq(context.Background(), &req)
		Assert(t, (err != nil), "kv read with invalid params suceeded")
		req.ClusterType = meta.ClusterTypeKstore
		req.Table = "table1"
		_, err = dnclient.ReadReq(context.Background(), &req)
		Assert(t, (err != nil), "kv read with invalid params suceeded")
	}

	// list all keys in a table
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		req := tproto.TableMsg{
			ClusterType: meta.ClusterTypeTstore,
			ReplicaID:   uint64(idx + 1),
			ShardID:     uint64(idx + 1),
			Table:       "table0",
		}

		resp, err := dnclient.ListReq(context.Background(), &req)
		Assert(t, (err != nil), "kv list with invalid params suceeded")
		req.ClusterType = meta.ClusterTypeKstore
		resp, err = dnclient.ListReq(context.Background(), &req)
		AssertOk(t, err, "Error listing table")
		Assert(t, (len(resp.Kvs) == 0), "Got invalid respons for non-existing table", resp)
	}

	// delete the keys
	for idx := 0; idx < numNodes; idx++ {
		dnclient := tproto.NewDataNodeClient(clients[idx].ClientConn)

		// keys to delete
		kl := []*tproto.Key{
			{
				Key: []byte("testKey1"),
			},
			{
				Key: []byte("testKey2"),
			},
		}

		// build the request
		req := tproto.KeyMsg{
			ClusterType: meta.ClusterTypeTstore,
			ReplicaID:   uint64(idx + 1),
			ShardID:     uint64(idx + 1),
			Table:       "table0",
			Keys:        kl,
		}

		_, err := dnclient.DelReq(context.Background(), &req)
		Assert(t, (err != nil), "kv delete with invalid params suceeded")
		req.ClusterType = meta.ClusterTypeKstore
		_, err = dnclient.DelReq(context.Background(), &req)
		Assert(t, (err != nil), "kv delete with non-existing keys suceeded")
	}
}

func TestDataNodeTstoreClustering(t *testing.T) {
	const numNodes = 3
	dnodes := make([]*DNode, numNodes)
	clients := make([]*rpckit.RPCClient, numNodes)
	var err error
	logger := log.GetNewLogger(log.GetDefaultConfig(t.Name()))

	// metadata config
	cfg := meta.DefaultClusterConfig()
	cfg.EnableKstoreMeta = false
	cfg.DeadInterval = time.Millisecond * 100
	cfg.NodeTTL = 5
	cfg.RebalanceDelay = time.Millisecond * 100
	cfg.RebalanceInterval = time.Millisecond * 10

	// create a temp dir
	path, err := ioutil.TempDir("", "tstore-")
	AssertOk(t, err, "Error creating tmp dir")
	defer os.RemoveAll(path)

	qpath, err := ioutil.TempDir("", "qstore-")
	AssertOk(t, err, "Error creating tmp dir")
	defer os.RemoveAll(path)

	// create nodes
	for idx := 0; idx < numNodes; idx++ {
		// create the data node
		dnodes[idx], err = NewDataNode(cfg, fmt.Sprintf("node-%d", idx), fmt.Sprintf("localhost:730%d", idx), fmt.Sprintf("%s/%d", path, idx), fmt.Sprintf("%s/%d", qpath, idx), logger)
		AssertOk(t, err, "Error creating nodes")
	}

	// create rpc client
	for idx := 0; idx < numNodes; idx++ {
		clients[idx], err = rpckit.NewRPCClient(fmt.Sprintf("datanode-%d", idx), fmt.Sprintf("localhost:730%d", idx), rpckit.WithLoggerEnabled(false))
		AssertOk(t, err, "Error connecting to grpc server")
		defer clients[idx].Close()
	}

	watcher, err := meta.NewWatcher("watcher", cfg)
	AssertOk(t, err, "Error creating the watcher")
	defer watcher.Stop()

	// wait till cluster state has converged
	AssertEventually(t, func() (bool, interface{}) {
		if (len(watcher.GetCluster(meta.ClusterTypeTstore).NodeMap) != numNodes) ||
			(len(watcher.GetCluster(meta.ClusterTypeTstore).ShardMap.Shards) != meta.DefaultShardCount) {
			return false, nil
		}

		for idx := 0; idx < numNodes; idx++ {
			if watcher.GetCluster(meta.ClusterTypeTstore).NodeMap[fmt.Sprintf("node-%d", idx)].NumShards < (meta.DefaultShardCount * meta.DefaultReplicaCount / numNodes) {
				return false, nil
			}

			if dnodes[idx].HasPendingSync() {
				return false, nil
			}
		}

		return true, nil
	}, "nodes did not get cluster update", "100ms", "30s")

	// make create database call
	cl := dnodes[0].GetCluster(meta.ClusterTypeTstore)
	for _, shard := range cl.ShardMap.Shards {
		// walk all replicas in the shard
		for _, repl := range shard.Replicas {
			dnclient, rerr := dnodes[0].getDnclient(meta.ClusterTypeTstore, repl.NodeUUID)
			AssertOk(t, rerr, "Error getting datanode client")

			req := tproto.DatabaseReq{
				ClusterType: meta.ClusterTypeTstore,
				ReplicaID:   repl.ReplicaID,
				ShardID:     repl.ShardID,
				Database:    "db0",
			}

			_, err = dnclient.CreateDatabase(context.Background(), &req)
			AssertOk(t, err, "Error making the create database call")
		}
	}

	// write some points
	for _, shard := range cl.ShardMap.Shards {
		// walk all replicas in the shard
		for _, repl := range shard.Replicas {
			if repl.IsPrimary {

				dnclient, rerr := dnodes[0].getDnclient(meta.ClusterTypeTstore, repl.NodeUUID)
				AssertOk(t, rerr, "Error getting datanode client")

				data := fmt.Sprintf("cpu,host=serverB,svc=nginx value1=11,value2=12  %v\n"+
					"cpu,host=serverC,svc=nginx value1=21,value2=22  %v\n", time.Now().UnixNano(), time.Now().UnixNano())

				req := tproto.PointsWriteReq{
					ClusterType: meta.ClusterTypeTstore,
					ReplicaID:   repl.ReplicaID,
					ShardID:     repl.ShardID,
					Database:    "db0",
					Points:      data,
				}

				_, err = dnclient.PointsWrite(context.Background(), &req)
				AssertOk(t, err, "Error writing points")
			}
		}
	}

	// do a quick statefull restart
	for idx := 0; idx < numNodes; idx++ {
		log.Infof("############### Restarting(quick) node %s", fmt.Sprintf("node-%d", idx))
		err = dnodes[idx].Stop()
		AssertOk(t, err, "Error stopping data node")

		// create the data node
		dnodes[idx], err = NewDataNode(cfg, fmt.Sprintf("node-%d", idx), fmt.Sprintf("localhost:730%d", idx), fmt.Sprintf("%s/%d", path, idx), fmt.Sprintf("%s/%d", qpath, idx), logger)
		AssertOk(t, err, "Error creating nodes")

		// verify atleast one of the nodes is a leader
		AssertEventually(t, func() (bool, interface{}) {
			for i := 0; i < numNodes; i++ {
				if dnodes[i].metaNode.IsLeader() {
					return true, nil
				}
			}
			return false, nil
		}, "Leader election failure", "300ms", "30s")
	}

	// wait till cluster state has converged
	AssertEventually(t, func() (bool, interface{}) {
		if (len(watcher.GetCluster(meta.ClusterTypeTstore).NodeMap) != numNodes) ||
			(len(watcher.GetCluster(meta.ClusterTypeTstore).ShardMap.Shards) != meta.DefaultShardCount) {
			return false, nil
		}

		for idx := 0; idx < numNodes; idx++ {
			if watcher.GetCluster(meta.ClusterTypeTstore).NodeMap[fmt.Sprintf("node-%d", idx)].NumShards < (meta.DefaultShardCount * meta.DefaultReplicaCount / numNodes) {
				return false, nil
			}
		}

		return true, nil
	}, "nodes did not get cluster update", "100ms", "30s")

	// do a soft restart without expiring the lease
	for idx := 0; idx < numNodes; idx++ {
		log.Infof("############### Restarting(soft) node %s", fmt.Sprintf("node-%d", idx))
		err = dnodes[idx].SoftRestart()
		AssertOk(t, err, "Error softrestarting data node")

		// verify atleast one of the nodes is a leader
		AssertEventually(t, func() (bool, interface{}) {
			for i := 0; i < numNodes; i++ {
				if dnodes[i].metaNode.IsLeader() {
					return true, nil
				}
			}
			return false, nil
		}, "Leader election failure", "300ms", "30s")
	}

	// wait till cluster state has converged
	AssertEventually(t, func() (bool, interface{}) {
		if (len(watcher.GetCluster(meta.ClusterTypeTstore).NodeMap) != numNodes) ||
			(len(watcher.GetCluster(meta.ClusterTypeTstore).ShardMap.Shards) != meta.DefaultShardCount) {
			return false, nil
		}

		for idx := 0; idx < numNodes; idx++ {
			if watcher.GetCluster(meta.ClusterTypeTstore).NodeMap[fmt.Sprintf("node-%d", idx)].NumShards < (meta.DefaultShardCount * meta.DefaultReplicaCount / numNodes) {
				return false, nil
			}
		}

		return true, nil
	}, "nodes did not get cluster update", "100ms", "30s")

	// restart each of the nodes
	for idx := 0; idx < numNodes; idx++ {
		log.Infof("################## Restarting(slow) node %s", fmt.Sprintf("node-%d", idx))

		err = dnodes[idx].Stop()
		AssertOk(t, err, "Error stopping data node")
		time.Sleep(cfg.DeadInterval * 3)

		AssertEventually(t, func() (bool, interface{}) {
			if len(watcher.GetCluster(meta.ClusterTypeTstore).NodeMap) != (numNodes - 1) {
				return false, nil
			}

			return true, nil
		}, "node was not removed from cluster", "100ms", "30s")

		// create the data node
		dnodes[idx], err = NewDataNode(cfg, fmt.Sprintf("node-%d", idx), fmt.Sprintf("localhost:730%d", idx), fmt.Sprintf("%s/%d", path, idx), fmt.Sprintf("%s/%d", qpath, idx), logger)
		AssertOk(t, err, "Error creating nodes")

		// verify atleast one of the nodes is a leader
		AssertEventually(t, func() (bool, interface{}) {
			for i := 0; i < numNodes; i++ {
				if dnodes[i].metaNode.IsLeader() {
					return true, nil
				}
			}
			return false, nil
		}, "Leader election failure", "300ms", "30s")

		// wait till cluster state has converged
		AssertEventually(t, func() (bool, interface{}) {
			if (len(watcher.GetCluster(meta.ClusterTypeTstore).NodeMap) != numNodes) ||
				(len(watcher.GetCluster(meta.ClusterTypeTstore).ShardMap.Shards) != meta.DefaultShardCount) {
				return false, nil
			}

			for idx := 0; idx < numNodes; idx++ {
				if watcher.GetCluster(meta.ClusterTypeTstore).NodeMap[fmt.Sprintf("node-%d", idx)].NumShards < (meta.DefaultShardCount * meta.DefaultReplicaCount / numNodes) {
					return false, nil
				}
			}

			return true, nil
		}, "nodes did not get cluster update", "100ms", "30s")
	}

	for idx := 0; idx < numNodes; idx++ {
		// stop the data node
		err = dnodes[idx].Stop()
		AssertOk(t, err, "Error stopping nodes")
	}
	time.Sleep(time.Millisecond * 10)
	meta.DestroyClusterState(cfg, meta.ClusterTypeTstore)
	meta.DestroyClusterState(cfg, meta.ClusterTypeKstore)
}

func TestDataNodeKstoreClustering(t *testing.T) {
	const numNodes = 3
	dnodes := make([]*DNode, numNodes)
	clients := make([]*rpckit.RPCClient, numNodes)
	var err error
	logger := log.GetNewLogger(log.GetDefaultConfig(t.Name()))

	// metadata config
	cfg := meta.DefaultClusterConfig()
	cfg.EnableTstoreMeta = false
	cfg.DeadInterval = time.Millisecond * 100
	cfg.NodeTTL = 5
	cfg.RebalanceDelay = time.Millisecond * 100
	cfg.RebalanceInterval = time.Millisecond * 10

	// create a temp dir
	path, err := ioutil.TempDir("", "kstore-")
	AssertOk(t, err, "Error creating tmp dir")
	defer os.RemoveAll(path)

	qpath, err := ioutil.TempDir("", "qstore-")
	AssertOk(t, err, "Error creating tmp dir")
	defer os.RemoveAll(path)

	// create nodes
	for idx := 0; idx < numNodes; idx++ {
		// create the data node
		dnodes[idx], err = NewDataNode(cfg, fmt.Sprintf("node-%d", idx), fmt.Sprintf("localhost:730%d", idx), fmt.Sprintf("%s/%d", path, idx), fmt.Sprintf("%s/%d", qpath, idx), logger)
		AssertOk(t, err, "Error creating nodes")
	}

	// create rpc client
	for idx := 0; idx < numNodes; idx++ {
		clients[idx], err = rpckit.NewRPCClient(fmt.Sprintf("datanode-%d", idx), fmt.Sprintf("localhost:730%d", idx), rpckit.WithLoggerEnabled(false))
		AssertOk(t, err, "Error connecting to grpc server")
		defer clients[idx].Close()
	}

	watcher, err := meta.NewWatcher("watcher", cfg)
	AssertOk(t, err, "Error creating the watcher")
	defer watcher.Stop()

	// wait till cluster state has converged
	AssertEventually(t, func() (bool, interface{}) {
		if (len(watcher.GetCluster(meta.ClusterTypeKstore).NodeMap) != numNodes) ||
			(len(watcher.GetCluster(meta.ClusterTypeKstore).ShardMap.Shards) != meta.DefaultShardCount) {
			return false, nil
		}

		for idx := 0; idx < numNodes; idx++ {
			if watcher.GetCluster(meta.ClusterTypeKstore).NodeMap[fmt.Sprintf("node-%d", idx)].NumShards < (meta.DefaultShardCount * meta.DefaultReplicaCount / numNodes) {
				return false, nil
			}
		}

		return true, nil
	}, "nodes did not get cluster update", "100ms", "30s")

	// write some keys
	kcl := dnodes[0].GetCluster(meta.ClusterTypeKstore)
	for _, shard := range kcl.ShardMap.Shards {
		// walk all replicas in the shard
		for _, repl := range shard.Replicas {
			if repl.IsPrimary {
				dnclient, rerr := dnodes[0].getDnclient(meta.ClusterTypeKstore, repl.NodeUUID)
				AssertOk(t, rerr, "Error getting datanode client")

				kvl := []*tproto.KeyValue{
					{
						Key:   []byte("testKey1"),
						Value: []byte("testValue1"),
					},
					{
						Key:   []byte("testKey2"),
						Value: []byte("testValue2"),
					},
				}

				// build the request
				req := tproto.KeyValueMsg{
					ClusterType: meta.ClusterTypeKstore,
					ReplicaID:   repl.ReplicaID,
					ShardID:     repl.ShardID,
					Table:       "table0",
					Kvs:         kvl,
				}

				_, err = dnclient.WriteReq(context.Background(), &req)
				AssertOk(t, err, "Error writing kvs")
			}
		}
	}
	time.Sleep(time.Second)

	// do a quick statefull restart
	for idx := 0; idx < numNodes; idx++ {
		log.Infof("################## Restarting(quick) node %s", fmt.Sprintf("node-%d", idx))

		err = dnodes[idx].Stop()
		AssertOk(t, err, "Error stopping data node")

		// create the data node
		dnodes[idx], err = NewDataNode(cfg, fmt.Sprintf("node-%d", idx), fmt.Sprintf("localhost:730%d", idx), fmt.Sprintf("%s/%d", path, idx), fmt.Sprintf("%s/%d", qpath, idx), logger)
		AssertOk(t, err, "Error creating nodes")

		// verify atleast one of the nodes is a leader
		AssertEventually(t, func() (bool, interface{}) {
			for i := 0; i < numNodes; i++ {
				if dnodes[i].metaNode.IsLeader() {
					return true, nil
				}
			}
			return false, nil
		}, "Leader election failure", "300ms", "30s")
	}

	time.Sleep(time.Second)

	// wait till cluster state has converged
	AssertEventually(t, func() (bool, interface{}) {
		if (len(watcher.GetCluster(meta.ClusterTypeKstore).NodeMap) != numNodes) ||
			(len(watcher.GetCluster(meta.ClusterTypeKstore).ShardMap.Shards) != meta.DefaultShardCount) {
			return false, watcher.GetCluster(meta.ClusterTypeKstore)
		}

		for idx := 0; idx < numNodes; idx++ {
			if watcher.GetCluster(meta.ClusterTypeKstore).NodeMap[fmt.Sprintf("node-%d", idx)].NumShards < (meta.DefaultShardCount * meta.DefaultReplicaCount / numNodes) {
				return false, watcher.GetCluster(meta.ClusterTypeKstore)
			}
		}

		return true, nil
	}, "nodes did not get cluster update", "100ms", "30s")
	time.Sleep(time.Second)

	// do a soft restart without expiring the lease
	for idx := 0; idx < numNodes; idx++ {
		log.Infof("############### Restarting(soft) node %s", fmt.Sprintf("node-%d", idx))
		err = dnodes[idx].SoftRestart()
		AssertOk(t, err, "Error softrestarting data node")

		// verify atleast one of the nodes is a leader
		AssertEventually(t, func() (bool, interface{}) {
			for i := 0; i < numNodes; i++ {
				if dnodes[i].metaNode.IsLeader() {
					return true, nil
				}
			}
			return false, nil
		}, "Leader election failure", "300ms", "30s")
	}

	// wait till cluster state has converged
	AssertEventually(t, func() (bool, interface{}) {
		if (len(watcher.GetCluster(meta.ClusterTypeKstore).NodeMap) != numNodes) ||
			(len(watcher.GetCluster(meta.ClusterTypeKstore).ShardMap.Shards) != meta.DefaultShardCount) {
			return false, nil
		}

		for idx := 0; idx < numNodes; idx++ {
			if watcher.GetCluster(meta.ClusterTypeKstore).NodeMap[fmt.Sprintf("node-%d", idx)].NumShards < (meta.DefaultShardCount * meta.DefaultReplicaCount / numNodes) {
				return false, nil
			}
		}

		return true, nil
	}, "nodes did not get cluster update", "100ms", "30s")
	time.Sleep(time.Second)

	// restart each of the nodes
	for idx := 0; idx < numNodes; idx++ {
		log.Infof("################## Restarting(slow) node %s", fmt.Sprintf("node-%d", idx))

		err = dnodes[idx].Stop()
		AssertOk(t, err, "Error stopping data node")
		time.Sleep(cfg.DeadInterval * 3)

		AssertEventually(t, func() (bool, interface{}) {
			if len(watcher.GetCluster(meta.ClusterTypeKstore).NodeMap) != (numNodes - 1) {
				return false, watcher.GetCluster(meta.ClusterTypeKstore)
			}

			return true, nil
		}, "node was not removed from cluster", "100ms", "30s")

		// create the data node
		dnodes[idx], err = NewDataNode(cfg, fmt.Sprintf("node-%d", idx), fmt.Sprintf("localhost:730%d", idx), fmt.Sprintf("%s/%d", path, idx), fmt.Sprintf("%s/%d", qpath, idx), logger)
		AssertOk(t, err, "Error creating nodes")

		// verify atleast one of the nodes is a leader
		AssertEventually(t, func() (bool, interface{}) {
			for i := 0; i < numNodes; i++ {
				if dnodes[i].metaNode.IsLeader() {
					return true, nil
				}
			}
			return false, nil
		}, "Leader election failure", "300ms", "30s")

		// wait till cluster state has converged
		AssertEventually(t, func() (bool, interface{}) {
			if (len(watcher.GetCluster(meta.ClusterTypeKstore).NodeMap) != numNodes) ||
				(len(watcher.GetCluster(meta.ClusterTypeKstore).ShardMap.Shards) != meta.DefaultShardCount) {
				return false, watcher.GetCluster(meta.ClusterTypeKstore)
			}

			for idx := 0; idx < numNodes; idx++ {
				if watcher.GetCluster(meta.ClusterTypeKstore).NodeMap[fmt.Sprintf("node-%d", idx)].NumShards < (meta.DefaultShardCount * meta.DefaultReplicaCount / numNodes) {
					return false, watcher.GetCluster(meta.ClusterTypeKstore)
				}
			}

			return true, nil
		}, "nodes did not get cluster update", "100ms", "30s")

		time.Sleep(time.Second)
	}

	// delete some keys
	kcl = dnodes[0].GetCluster(meta.ClusterTypeKstore)
	for _, shard := range kcl.ShardMap.Shards {
		// walk all replicas in the shard
		for _, repl := range shard.Replicas {
			if repl.IsPrimary {
				dnclient, rerr := dnodes[0].getDnclient(meta.ClusterTypeKstore, repl.NodeUUID)
				AssertOk(t, rerr, "Error getting datanode client")

				// keys to delete
				kl := []*tproto.Key{
					{
						Key: []byte("testKey1"),
					},
					{
						Key: []byte("testKey2"),
					},
				}

				// build the request
				req := tproto.KeyMsg{
					ClusterType: meta.ClusterTypeKstore,
					ReplicaID:   repl.ReplicaID,
					ShardID:     repl.ShardID,
					Table:       "table0",
					Keys:        kl,
				}

				_, err = dnclient.DelReq(context.Background(), &req)
				AssertOk(t, err, "Error deleting the keys")
			}
		}
	}

	for idx := 0; idx < numNodes; idx++ {
		// stop the data node
		err = dnodes[idx].Stop()
		AssertOk(t, err, "Error stopping nodes")
	}

	time.Sleep(time.Millisecond * 10)
	meta.DestroyClusterState(cfg, meta.ClusterTypeTstore)
	meta.DestroyClusterState(cfg, meta.ClusterTypeKstore)
}

func TestSyncBuffer(t *testing.T) {
	const numNodes = 3
	dnodes := make([]*DNode, numNodes)
	clients := make([]*rpckit.RPCClient, numNodes)
	var err error
	logger := log.GetNewLogger(log.GetDefaultConfig(t.Name()))

	// metadata config
	cfg := meta.DefaultClusterConfig()
	cfg.EnableKstoreMeta = false
	cfg.DeadInterval = time.Millisecond * 100
	cfg.NodeTTL = 5
	cfg.RebalanceDelay = time.Millisecond * 100
	cfg.RebalanceInterval = time.Millisecond * 10

	// create a temp dir
	path, err := ioutil.TempDir("", "tstore-")
	AssertOk(t, err, "Error creating tmp dir")
	defer os.RemoveAll(path)

	qpath, err := ioutil.TempDir("", "qstore-")
	AssertOk(t, err, "Error creating tmp dir")
	defer os.RemoveAll(path)

	// create nodes
	for idx := 0; idx < numNodes; idx++ {
		// create the data node
		dnodes[idx], err = NewDataNode(cfg, fmt.Sprintf("node-%d", idx), fmt.Sprintf("localhost:730%d", idx), fmt.Sprintf("%s/%d", path, idx), fmt.Sprintf("%s/%d", qpath, idx), logger)
		AssertOk(t, err, "Error creating nodes")
	}

	// create rpc client
	for idx := 0; idx < numNodes; idx++ {
		clients[idx], err = rpckit.NewRPCClient(fmt.Sprintf("datanode-%d", idx), fmt.Sprintf("localhost:730%d", idx), rpckit.WithLoggerEnabled(false))
		AssertOk(t, err, "Error connecting to grpc server")
		defer clients[idx].Close()
	}

	watcher, err := meta.NewWatcher("watcher", cfg)
	AssertOk(t, err, "Error creating the watcher")
	defer watcher.Stop()

	// wait till cluster state has converged
	AssertEventually(t, func() (bool, interface{}) {
		if (len(watcher.GetCluster(meta.ClusterTypeTstore).NodeMap) != numNodes) ||
			(len(watcher.GetCluster(meta.ClusterTypeTstore).ShardMap.Shards) != meta.DefaultShardCount) {
			return false, nil
		}

		for idx := 0; idx < numNodes; idx++ {
			if watcher.GetCluster(meta.ClusterTypeTstore).NodeMap[fmt.Sprintf("node-%d", idx)].NumShards < (meta.DefaultShardCount * meta.DefaultReplicaCount / numNodes) {
				return false, nil
			}

			if dnodes[idx].HasPendingSync() {
				return false, nil
			}
		}

		return true, nil
	}, "nodes did not get cluster update", "100ms", "30s")

	// make create database call
	cl := dnodes[0].GetCluster(meta.ClusterTypeTstore)
	for _, shard := range cl.ShardMap.Shards {
		// walk all replicas in the shard
		for _, repl := range shard.Replicas {
			dnclient, rerr := dnodes[0].getDnclient(meta.ClusterTypeTstore, repl.NodeUUID)
			AssertOk(t, rerr, "Error getting datanode client")

			req := tproto.DatabaseReq{
				ClusterType: meta.ClusterTypeTstore,
				ReplicaID:   repl.ReplicaID,
				ShardID:     repl.ShardID,
				Database:    "db0",
			}

			_, err = dnclient.CreateDatabase(context.Background(), &req)
			AssertOk(t, err, "Error making the create database call")
		}
	}

	AssertEventually(t, func() (bool, interface{}) {
		cl := dnodes[0].GetCluster(meta.ClusterTypeTstore)
		for _, shard := range cl.ShardMap.Shards {
			if len(shard.Replicas) != meta.DefaultReplicaCount {
				return false, fmt.Sprintf("replica count didn't match for shard:%d, replicas:%+v", shard.ShardID, shard.Replicas)
			}
		}
		return true, nil
	}, "replica count didn't match")

	// get updated cluster info
	cl = dnodes[0].GetCluster(meta.ClusterTypeTstore)

	// sync buffer tests
	kvShardMap := make([]sync.Map, len(cl.ShardMap.Shards)+1)
	tsShardMap := make([]sync.Map, len(cl.ShardMap.Shards)+1)

	getTsMapKeys := func(i int) []uint64 {
		l := []uint64{}
		tsShardMap[i].Range(func(k interface{}, v interface{}) bool {
			l = append(l, k.(uint64))
			return true
		})
		return l
	}

	getKvMapKeys := func(i int) []uint64 {
		l := []uint64{}
		kvShardMap[i].Range(func(k interface{}, v interface{}) bool {
			l = append(l, k.(uint64))
			return true
		})
		return l
	}

	// write some points
	for _, shard := range cl.ShardMap.Shards {
		// walk all replicas in the shard
		for _, repl := range shard.Replicas {
			log.Infof("shard id %d replica id:%d (%v) node:%s replicas %+v len:%d", shard.ShardID, repl.ReplicaID, repl.IsPrimary, repl.NodeUUID, shard.Replicas, len(cl.ShardMap.Shards))
			if !repl.IsPrimary {
				data := "cpu,host=serverB,svc=nginx value1=11,value2=12 \n" +
					"cpu,host=serverC,svc=nginx value1=21,value2=22  \n"

				req := tproto.PointsWriteReq{
					ClusterType: meta.ClusterTypeTstore,
					ReplicaID:   repl.ReplicaID,
					ShardID:     repl.ShardID,
					Database:    "db0",
					Points:      data,
				}

				err = dnodes[0].addSyncBuffer(&tsShardMap[shard.ShardID], repl.NodeUUID, &req)
				AssertOk(t, err, "Error creating ts sync buffer")

				// try update
				if shard.ShardID == 1 {
					err = dnodes[0].addSyncBuffer(&tsShardMap[shard.ShardID], repl.NodeUUID, &req)
					AssertOk(t, err, "Error adding ts sync buffer")
				}

				// write some keys
				kvl := []*tproto.KeyValue{
					{
						Key:   []byte("testKey1"),
						Value: []byte("testValue1"),
					},
					{
						Key:   []byte("testKey2"),
						Value: []byte("testValue2"),
					},
				}

				// build the request
				kvReq := tproto.KeyValueMsg{
					ClusterType: meta.ClusterTypeKstore,
					ReplicaID:   repl.ReplicaID,
					ShardID:     repl.ShardID,
					Table:       "table0",
					Kvs:         kvl,
				}

				err = dnodes[0].addSyncBuffer(&kvShardMap[shard.ShardID], repl.NodeUUID, &kvReq)
				AssertOk(t, err, "Error writing kvs")
			}
		}
	}

	// check replica ids in sync buff map
	for i := 1; i <= len(cl.ShardMap.Shards); i++ {
		tsl := getTsMapKeys(i)
		Assert(t, len(tsl) == 1, fmt.Sprintf("invalid replica ids in ts sync buffer, expected 1, got %d", len(tsl)))
	}

	for i := 1; i <= len(cl.ShardMap.Shards); i++ {
		kvl := getKvMapKeys(i)
		Assert(t, len(kvl) == 1, fmt.Sprintf("invalid replica ids in kv sync buffer, expected 1, got %d", len(kvl)))
	}

	// update
	for _, shard := range cl.ShardMap.Shards {
		// walk all replicas in the shard
		for _, repl := range shard.Replicas {
			if !repl.IsPrimary {
				repInfo := []*tproto.ReplicaInfo{}
				for _, r := range shard.Replicas {
					repInfo = append(repInfo, &tproto.ReplicaInfo{
						ShardID:   r.ShardID,
						ReplicaID: r.ReplicaID,
						NodeUUID:  r.NodeUUID,
					})
				}

				req := tproto.ShardReq{
					ClusterType: meta.ClusterTypeTstore,
					ReplicaID:   repl.ReplicaID,
					ShardID:     repl.ShardID,
					IsPrimary:   true,
					Replicas:    repInfo,
				}

				err = dnodes[0].updateSyncBuffer(&tsShardMap[shard.ShardID], &req)
				AssertOk(t, err, "Error updating ts sync buffer")

				err = dnodes[0].updateSyncBuffer(&kvShardMap[shard.ShardID], &req)
				AssertOk(t, err, "Error updating kv sync buffer")
			}
		}
	}

	// check entry in map
	for i := 1; i <= len(cl.ShardMap.Shards); i++ {
		tsl := getTsMapKeys(i)
		Assert(t, len(tsl) == 1, fmt.Sprintf("invalid replica ids in ts sync buffer %d, expected 1, got %d", i, len(tsl)))
	}

	for i := 1; i <= len(cl.ShardMap.Shards); i++ {
		kvl := getKvMapKeys(i)
		Assert(t, len(kvl) == 1, fmt.Sprintf("invalid replica ids in kv sync buffer %d, expected 1, got %d", i, len(kvl)))
	}

	// delete
	for _, shard := range cl.ShardMap.Shards {
		// walk all replicas in the shard
		for _, repl := range shard.Replicas {
			if !repl.IsPrimary {
				err = dnodes[0].deleteShardSyncBuffer(&kvShardMap[shard.ShardID])
				AssertOk(t, err, "Error deleting kv sync buffer")

				err = dnodes[0].deleteShardSyncBuffer(&tsShardMap[shard.ShardID])
				AssertOk(t, err, "Error deleting ts sync buffer")
			}
		}
	}

	// make sure map is empty
	for i := 1; i <= len(cl.ShardMap.Shards); i++ {
		tsl := getTsMapKeys(i)
		Assert(t, len(tsl) == 0, fmt.Sprintf("invalid replica ids in ts sync buffer %d, expected 0,, got %d", i, len(tsl)))
	}

	for i := 1; i <= len(cl.ShardMap.Shards); i++ {
		kvl := getKvMapKeys(i)
		Assert(t, len(kvl) == 0, fmt.Sprintf("invalid replica ids in kv sync buffer %d, expected  0, got %d", i, len(kvl)))
	}

	addInvalidPoints := func() error {
		// add invalid entry
		ReplID := uint64(55)

		req := tproto.PointsWriteReq{
			ClusterType: meta.ClusterTypeTstore,
			ReplicaID:   ReplID,
			ShardID:     ReplID,
			Database:    "db0",
			Points:      "",
		}

		return dnodes[0].addSyncBuffer(&tsShardMap[0], "node-55", &req)
	}

	err = addInvalidPoints()
	AssertOk(t, err, "Error writing points")

	// check entry in map
	tsl := getTsMapKeys(0)
	Assert(t, len(tsl) == 1, fmt.Sprintf("invalid replica ids in ts sync buffer, expected 1, got %d", len(tsl)))

	// delete
	err = dnodes[0].deleteShardSyncBuffer(&tsShardMap[0])
	AssertOk(t, err, "Error deleting ts sync buffer")

	// make sure map is empty
	tsl = getTsMapKeys(0)
	Assert(t, len(tsl) == 0, fmt.Sprintf("invalid replica ids in ts sync buffer, expected 0, got %d", len(tsl)))

	// add invalid entry
	err = addInvalidPoints()
	AssertOk(t, err, "Error writing points")

	// check entry in map
	tsl = getTsMapKeys(0)
	Assert(t, len(tsl) == 1, fmt.Sprintf("invalid replica ids in ts sync buffer, expected 1, got %d", len(tsl)))

	// wait for go routine to process it
	time.Sleep(5 * time.Second)

	// delete
	err = dnodes[0].deleteShardSyncBuffer(&tsShardMap[0])
	AssertOk(t, err, "Error deleting ts sync buffer")

	// make sure map is empty
	tsl = getTsMapKeys(0)
	Assert(t, len(tsl) == 0, fmt.Sprintf("invalid replica ids in ts sync buffer, expected 0, got %d", len(tsl)))

	// add invalid entry and update the shard to remove it
	err = addInvalidPoints()
	AssertOk(t, err, "Error writing points")

	// check entry in map
	tsl = getTsMapKeys(0)
	Assert(t, len(tsl) == 1, fmt.Sprintf("invalid replica ids in ts sync buffer, expected 1, got %d", len(tsl)))

	upreq := tproto.ShardReq{
		ClusterType: meta.ClusterTypeTstore,
		ReplicaID:   uint64(55),
		ShardID:     uint64(55),
		IsPrimary:   true,
		Replicas:    nil, // empty replicas
	}

	err = dnodes[0].updateSyncBuffer(&tsShardMap[0], &upreq)
	AssertOk(t, err, "Error updating ts sync buffer")

	// check entry in map
	tsl = getTsMapKeys(0)
	Assert(t, len(tsl) == 0, fmt.Sprintf("invalid replica ids in ts sync buffer, expected 0, got %d", len(tsl)))

	err = addInvalidPoints()
	AssertOk(t, err, "Error writing points")

	// check entry in map
	tsl = getTsMapKeys(0)
	Assert(t, len(tsl) == 1, fmt.Sprintf("invalid replica ids in ts sync buffer, expected 1, got %d", len(tsl)))

	// check primary true->false
	upreq = tproto.ShardReq{
		ClusterType: meta.ClusterTypeTstore,
		ReplicaID:   uint64(55),
		ShardID:     uint64(55),
		IsPrimary:   false, // delete all
		Replicas:    nil,
	}

	err = dnodes[0].updateSyncBuffer(&tsShardMap[0], &upreq)
	AssertOk(t, err, "Error updating ts sync buffer")

	// check entry in map
	tsl = getTsMapKeys(0)
	Assert(t, len(tsl) == 0, fmt.Sprintf("invalid replica ids in ts sync buffer, expected 0, got %d", len(tsl)))

	for idx := 0; idx < numNodes; idx++ {
		// create the data node
		err := dnodes[idx].Stop()
		AssertOk(t, err, "failed to delete node")
	}

}

func TestAggQuery(t *testing.T) {
	const numNodes = 3
	dnodes := make([]*DNode, numNodes)
	clients := make([]*rpckit.RPCClient, numNodes)
	var err error
	logger := log.GetNewLogger(log.GetDefaultConfig(t.Name()))

	// metadata config
	cfg := meta.DefaultClusterConfig()
	cfg.EnableKstoreMeta = false
	cfg.DeadInterval = time.Millisecond * 100
	cfg.NodeTTL = 5
	cfg.RebalanceDelay = time.Millisecond * 100
	cfg.RebalanceInterval = time.Millisecond * 10

	// create a temp dir
	path, err := ioutil.TempDir("", "agg-tstore-")
	AssertOk(t, err, "Error creating tmp dir")
	defer os.RemoveAll(path)

	qpath, err := ioutil.TempDir("", "agg-qstore-")
	AssertOk(t, err, "Error creating tmp dir")
	defer os.RemoveAll(path)

	// create nodes
	for idx := 0; idx < numNodes; idx++ {
		// create the data node
		dnodes[idx], err = NewDataNode(cfg, fmt.Sprintf("node-%d", idx), fmt.Sprintf("localhost:730%d", idx), fmt.Sprintf("%s/%d", path, idx), fmt.Sprintf("%s/%d", qpath, idx), logger)
		AssertOk(t, err, "Error creating nodes")
	}

	// create rpc client
	for idx := 0; idx < numNodes; idx++ {
		clients[idx], err = rpckit.NewRPCClient(fmt.Sprintf("datanode-%d", idx), fmt.Sprintf("localhost:730%d", idx), rpckit.WithLoggerEnabled(false))
		AssertOk(t, err, "Error connecting to grpc server")
		defer clients[idx].Close()
	}

	watcher, err := meta.NewWatcher("watcher", cfg)
	AssertOk(t, err, "Error creating the watcher")
	defer watcher.Stop()

	// wait for metamgr to kick in
	time.Sleep(10 * time.Second)

	// wait till cluster state has converged
	AssertEventually(t, func() (bool, interface{}) {
		cl := dnodes[0].GetCluster(meta.ClusterTypeTstore)

		if (len(watcher.GetCluster(meta.ClusterTypeTstore).NodeMap) != numNodes) ||
			(len(watcher.GetCluster(meta.ClusterTypeTstore).ShardMap.Shards) != meta.DefaultShardCount) {
			return false, nil
		}

		for idx := 0; idx < numNodes; idx++ {
			if watcher.GetCluster(meta.ClusterTypeTstore).NodeMap[fmt.Sprintf("node-%d", idx)].NumShards < (meta.DefaultShardCount * meta.DefaultReplicaCount / numNodes) {
				return false, nil
			}

			if dnodes[idx].HasPendingSync() {
				return false, nil
			}
		}

		if len(cl.ShardMap.Shards) != meta.DefaultShardCount {
			return false, fmt.Errorf("didn't find all shards %+v", cl.ShardMap.Shards)
		}

		for _, shard := range cl.ShardMap.Shards {
			if len(shard.Replicas) != meta.DefaultReplicaCount {
				return false, fmt.Errorf("didn't find all replicas in shard %+v", shard)
			}
		}

		return true, nil
	}, "nodes did not get cluster update", "100ms", "30s")

	// make create database call
	cl := dnodes[0].GetCluster(meta.ClusterTypeTstore)
	for _, shard := range cl.ShardMap.Shards {
		// walk all replicas in the shard
		for _, repl := range shard.Replicas {
			dnclient, rerr := dnodes[0].getDnclient(meta.ClusterTypeTstore, repl.NodeUUID)
			AssertOk(t, rerr, "Error getting datanode client")

			req := tproto.DatabaseReq{
				ClusterType: meta.ClusterTypeTstore,
				ReplicaID:   repl.ReplicaID,
				ShardID:     repl.ShardID,
				Database:    "db0",
			}

			_, err = dnclient.CreateDatabase(context.Background(), &req)
			AssertOk(t, err, "Error making the create database call")
		}
	}

	// write some points
	cl = dnodes[0].GetCluster(meta.ClusterTypeTstore)
	for _, shard := range cl.ShardMap.Shards {
		// walk all replicas in the shard
		for _, repl := range shard.Replicas {
			if repl.IsPrimary {

				dnclient, rerr := dnodes[0].getDnclient(meta.ClusterTypeTstore, repl.NodeUUID)
				AssertOk(t, rerr, "Error getting datanode client")

				data := fmt.Sprintf("cpu,host=%s,svc=nginx value1=%d,value2=%d %v", repl.NodeUUID, repl.ShardID, repl.ReplicaID, time.Now().UnixNano())

				req := tproto.PointsWriteReq{
					ClusterType: meta.ClusterTypeTstore,
					ReplicaID:   repl.ReplicaID,
					ShardID:     repl.ShardID,
					Database:    "db0",
					Points:      data,
				}

				_, err = dnclient.PointsWrite(context.Background(), &req)
				AssertOk(t, err, "Error writing points")
				log.Infof("wrote points in shard: %d replica: %d node: %v", repl.ShardID, repl.ReplicaID, repl.NodeUUID)

				// query back
				qreq := tproto.QueryReq{
					ClusterType: meta.ClusterTypeTstore,
					ReplicaID:   repl.ReplicaID,
					ShardID:     repl.ShardID,
					Database:    "db0",
					TxnID:       uint64(repl.ReplicaID + 1),
					Query:       "SELECT * FROM cpu",
				}

				resp, err := dnclient.ExecuteQuery(context.Background(), &qreq)
				AssertOk(t, err, "query failed")

				Assert(t, len(resp.Result) == 1, "invalid number of results %+v", resp.Result)

				for _, rs := range resp.Result {
					rslt := query.Result{}
					err := rslt.UnmarshalJSON(rs.Data)
					AssertOk(t, err, "failed to unmarshal query response")

					Assert(t, len(rslt.Series) == 1, "invalid number of series %+v", rslt)

					for _, s := range rslt.Series {
						Assert(t, len(s.Columns) == 5, "invalid number of columns %+v", s.Columns)
						Assert(t, len(s.Values) == 1, "invalid number of values %+v", s.Values)
					}
				}
			}
		}
	}

	// execute some query
	AssertEventually(t, func() (bool, interface{}) {
		cl = watcher.GetCluster(meta.ClusterTypeTstore)
		for _, shard := range cl.ShardMap.Shards {
			// walk all replicas in the shard
			for _, repl := range shard.Replicas {

				if repl.State != meta.ReplicaStateReady {
					continue
				}

				log.Infof("==== query shard:%d replica: %d node %v ===", repl.ShardID, repl.ReplicaID, repl.NodeUUID)
				dnclient, rerr := dnodes[0].getDnclient(meta.ClusterTypeTstore, repl.NodeUUID)
				if rerr != nil {
					log.Errorf("error to get client %v", rerr)
					return false, rerr
				}

				req := tproto.QueryReq{
					ClusterType: meta.ClusterTypeTstore,
					ReplicaID:   repl.ReplicaID,
					ShardID:     repl.ShardID,
					Database:    "db0",
					TxnID:       uint64(repl.ReplicaID + 1),
					Query:       "SELECT * FROM cpu",
				}

				resp, err := dnclient.ExecuteQuery(context.Background(), &req)
				if err != nil {
					log.Errorf("query failed, %v", err)
					return false, fmt.Errorf("query failed, %s", err)
				}

				if len(resp.Result) != 1 {
					log.Errorf("invalid number of results %+v", resp.Result)
					return false, fmt.Errorf("invalid number of results %+v", resp.Result)
				}

				for _, rs := range resp.Result {
					rslt := query.Result{}
					if err := rslt.UnmarshalJSON(rs.Data); err != nil {
						log.Errorf("failed to unmarshal query response %+v", err)
						return false, fmt.Errorf("failed to unmarshal query response %+v", err)
					}

					if len(rslt.Series) != 1 {
						log.Errorf("invalid number of series %+v", rslt.Series)
						return false, fmt.Errorf("invalid number of series %+v", rslt.Series)
					}

					for _, s := range rslt.Series {
						if len(s.Columns) != 5 {
							log.Errorf("invalid number of columns %+v", s.Columns)
							return false, fmt.Errorf("invalid number of columns %+v", s.Columns)
						}

						if len(s.Values) != 1 {
							log.Errorf("invalid number of values %+v", s.Values)
							return false, fmt.Errorf("invalid number of values %+v", s.Values)
						}
					}
				}
			}
		}
		return true, nil
	}, "failed to query in replicas", "1s", "60s")

	// execute aggregated query
	cl = dnodes[0].GetCluster(meta.ClusterTypeTstore)
	for _, shard := range cl.ShardMap.Shards {
		// walk all replicas in the shard
		for _, repl := range shard.Replicas {
			log.Infof("==== agg-query shard:%d replica: %d node %v ===", repl.ShardID, repl.ReplicaID, repl.NodeUUID)
			dnclient, rerr := dnodes[0].getDnclient(meta.ClusterTypeTstore, repl.NodeUUID)
			AssertOk(t, rerr, "Error getting datanode client")

			req := tproto.QueryReq{
				ClusterType: meta.ClusterTypeTstore,
				ReplicaID:   repl.ReplicaID,
				ShardID:     repl.ShardID,
				Database:    "db0",
				TxnID:       uint64(repl.ReplicaID + 1),
				Query:       "SELECT * FROM cpu",
			}

			resp, err := dnclient.ExecuteAggQuery(context.Background(), &req)
			AssertOk(t, err, "Error executing aggregated query")

			Assert(t, len(resp.Result) == 1, "invalid number of results", resp.Result)
			for _, rs := range resp.Result {
				rslt := query.Result{}
				err := rslt.UnmarshalJSON(rs.Data)
				AssertOk(t, err, "failed to unmarshal query response")

				Assert(t, len(rslt.Series) == 1, "invalid number of series", rslt.Series)
				for _, s := range rslt.Series {
					Assert(t, len(s.Columns) == 5, "invalid number of columns", s.Columns)
					Assert(t, len(s.Values) == len(cl.ShardMap.Shards), fmt.Sprintf("invalid number of values %+v", s))
				}
			}
		}
	}

	// check query format
	for _, q1 := range []string{
		`SELECT time, value FROM cpu`,
		`SELECT value FROM cpu`,
		`SELECT value, host FROM cpu`,
		`SELECT * FROM cpu`,
		`SELECT time, * FROM cpu`,
		`SELECT value, * FROM cpu`,
		`SELECT max(value) FROM cpu`,
		`SELECT max(value), host FROM cpu`,
		`SELECT max(value), * FROM cpu`,
		`SELECT max(*) FROM cpu`,
		`SELECT max(/val/) FROM cpu`,
		`SELECT min(value) FROM cpu`,
		`SELECT min(value), host FROM cpu`,
		`SELECT min(value), * FROM cpu`,
		`SELECT min(*) FROM cpu`,
		`SELECT min(/val/) FROM cpu`,
		`SELECT first(value) FROM cpu`,
		`SELECT first(value), host FROM cpu`,
		`SELECT first(value), * FROM cpu`,
		`SELECT first(*) FROM cpu`,
		`SELECT first(/val/) FROM cpu`,
		`SELECT last(value) FROM cpu`,
		`SELECT last(value), host FROM cpu`,
		`SELECT last(value), * FROM cpu`,
		`SELECT last(*) FROM cpu`,
		`SELECT last(/val/) FROM cpu`,
		`SELECT count(value) FROM cpu`,
		`SELECT count(distinct(value)) FROM cpu`,
		`SELECT count(distinct value) FROM cpu`,
		`SELECT count(*) FROM cpu`,
		`SELECT count(/val/) FROM cpu`,
		`SELECT mean(value) FROM cpu`,
		`SELECT mean(*) FROM cpu`,
		`SELECT mean(/val/) FROM cpu`,
		`SELECT min(value), max(value) FROM cpu`,
		`SELECT min(*), max(*) FROM cpu`,
		`SELECT min(/val/), max(/val/) FROM cpu`,
		`SELECT first(value), last(value) FROM cpu`,
		`SELECT first(*), last(*) FROM cpu`,
		`SELECT first(/val/), last(/val/) FROM cpu`,
		`SELECT count(value) FROM cpu WHERE time >= now() - 1h GROUP BY time(10m)`,
		`SELECT distinct value FROM cpu`,
		`SELECT distinct(value) FROM cpu`,
		`SELECT value / total FROM cpu`,
		`SELECT min(value) / total FROM cpu`,
		`SELECT max(value) / total FROM cpu`,
		`SELECT top(value, 1) FROM cpu`,
		`SELECT top(value, host, 1) FROM cpu`,
		`SELECT top(value, 1), host FROM cpu`,
		`SELECT min(top) FROM (SELECT top(value, host, 1) FROM cpu) GROUP BY region`,
		`SELECT bottom(value, 1) FROM cpu`,
		`SELECT bottom(value, host, 1) FROM cpu`,
		`SELECT bottom(value, 1), host FROM cpu`,
		`SELECT max(bottom) FROM (SELECT bottom(value, host, 1) FROM cpu) GROUP BY region`,
		`SELECT percentile(value, 75) FROM cpu`,
		`SELECT percentile(value, 75.0) FROM cpu`,
		`SELECT sample(value, 2) FROM cpu`,
		`SELECT sample(*, 2) FROM cpu`,
		`SELECT sample(/val/, 2) FROM cpu`,
		`SELECT elapsed(value) FROM cpu`,
		`SELECT elapsed(value, 10s) FROM cpu`,
		`SELECT integral(value) FROM cpu`,
		`SELECT integral(value, 10s) FROM cpu`,
		`SELECT max(value) FROM cpu WHERE time >= now() - 1m GROUP BY time(10s, 5s)`,
		`SELECT max(value) FROM cpu WHERE time >= now() - 1m GROUP BY time(10s, '2000-01-01T00:00:05Z')`,
		`SELECT max(value) FROM cpu WHERE time >= now() - 1m GROUP BY time(10s, now())`,
		`SELECT max(mean) FROM (SELECT mean(value) FROM cpu GROUP BY host)`,
		`SELECT max(derivative) FROM (SELECT derivative(mean(value)) FROM cpu) WHERE time >= now() - 1m GROUP BY time(10s)`,
		`SELECT max(value) FROM (SELECT value + total FROM cpu) WHERE time >= now() - 1m GROUP BY time(10s)`,
		`SELECT value FROM cpu WHERE time >= '2000-01-01T00:00:00Z' AND time <= '2000-01-01T01:00:00Z'`,
		`SELECT value FROM (SELECT value FROM cpu) ORDER BY time DESC`,
	} {
		// execute aggregated query
		cl = dnodes[0].GetCluster(meta.ClusterTypeTstore)
		shard := cl.ShardMap.Shards[rand.Int63n(int64(len(cl.ShardMap.Shards)))]
		repl := shard.Replicas[shard.PrimaryReplica]

		req := tproto.QueryReq{
			ClusterType: meta.ClusterTypeTstore,
			ReplicaID:   repl.ReplicaID,
			ShardID:     repl.ShardID,
			Database:    "db0",
			TxnID:       uint64(repl.ReplicaID + 1),
			Query:       q1,
		}

		_, err := dnodes[0].ExecuteAggQuery(context.Background(), &req)
		AssertOk(t, err, "Error executing aggregated query")
	}

	for idx := 0; idx < numNodes; idx++ {
		// stop the data node
		err = dnodes[idx].Stop()
		AssertOk(t, err, "Error stopping nodes")
	}
	time.Sleep(time.Millisecond * 10)
	meta.DestroyClusterState(cfg, meta.ClusterTypeTstore)
	meta.DestroyClusterState(cfg, meta.ClusterTypeKstore)

}

func TestIsGrpcConnectErr(t *testing.T) {
	testData := []struct {
		err    error
		result bool
	}{
		{err: fmt.Errorf("connection error"), result: true},
		{err: fmt.Errorf("the connection is unavailable"), result: true},
		{err: fmt.Errorf("invalid node"), result: false},
		{err: nil, result: false},
	}

	// metadata config
	cfg := meta.DefaultClusterConfig()
	cfg.EnableKstoreMeta = false
	cfg.DeadInterval = time.Millisecond * 100
	cfg.NodeTTL = 5
	cfg.RebalanceDelay = time.Millisecond * 100
	cfg.RebalanceInterval = time.Millisecond * 10

	// create a temp dir
	path, err := ioutil.TempDir("", "tstore-")
	AssertOk(t, err, "Error creating tmp dir")
	defer os.RemoveAll(path)

	qpath, err := ioutil.TempDir("", "qstore-")
	AssertOk(t, err, "Error creating tmp dir")
	defer os.RemoveAll(path)

	logger := log.GetNewLogger(log.GetDefaultConfig(t.Name()))
	dn, err := NewDataNode(cfg, "node-"+t.Name(), "localhost:7300", path+t.Name(), qpath+t.Name(), logger)
	AssertOk(t, err, "Error creating nodes")

	for _, tc := range testData {
		s := dn.isGrpcConnectErr(tc.err)
		Assert(t, s == tc.result, fmt.Sprintf("expected %v, got %v (%+v)", tc.result, s, tc))
	}
}
