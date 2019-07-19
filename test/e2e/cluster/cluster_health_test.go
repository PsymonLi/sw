package cluster

import (
	"context"
	"fmt"
	"strings"
	"time"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	cmd "github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/venice/globals"
)

// Contains all the cluster health related tests

var _ = Describe("cluster health tests", func() {
	var (
		obj       api.ObjectMeta
		clusterIf cmd.ClusterV1ClusterInterface
	)
	BeforeEach(func() {
		validateCluster()

		apiGwAddr := ts.tu.ClusterVIP + ":" + globals.APIGwRESTPort
		apiClient, err := apiclient.NewRestAPIClient(apiGwAddr)
		Expect(err).ShouldNot(HaveOccurred())
		clusterIf = apiClient.ClusterV1().Cluster()
		ctx := ts.tu.MustGetLoggedInContext(context.Background())
		obj = api.ObjectMeta{Name: "testCluster"}
		_, err = clusterIf.Get(ctx, &obj)
		Expect(err).ShouldNot(HaveOccurred())
	})

	It(fmt.Sprintf("Test cluster health by killing %s (venice service)", globals.EvtsMgr), func() {
		// kill evtsmgr on one of the nodes and check cluster health
		nodeIP := ts.tu.VeniceNodeIPs[0]
		out := ts.tu.CommandOutput(nodeIP, fmt.Sprint("ps aux | grep [e]vtsmgr | awk '{print $2}'"))
		By(fmt.Sprintf("ts: %s Killing %s (pid %s) on %v", time.Now(), globals.EvtsMgr, out, nodeIP))
		killCmd := fmt.Sprintf("kill -9 %s", out)
		killCmdOut := ts.tu.CommandOutput(nodeIP, killCmd)
		By(fmt.Sprintf("ts: %s (%s) output: %s", time.Now(), killCmd, killCmdOut))
		Eventually(func() bool {
			return checkClusterHealth(clusterIf, &obj, cmd.ConditionStatus_FALSE.String(),
				fmt.Sprintf("Service %s failed to run desired number of instances", globals.EvtsMgr))
		}, 150, 2).Should(BeTrue(),
			fmt.Sprintf("ts: %s Cluster status is expected to be un-healthy as %s is not running on %s",
				time.Now(), globals.EvtsMgr, nodeIP))

		// pod should run the container again and things should be back to normal
		Eventually(func() bool {
			return checkClusterHealth(clusterIf, &obj, cmd.ConditionStatus_TRUE.String(), "")
		}, 150, 2).Should(BeTrue(),
			fmt.Sprintf("ts: %s Cluster status is expected to be healthy as %s should be started back on node %s",
				time.Now(), globals.EvtsMgr, nodeIP))
	})

	It("Test cluster health by pausing/resuming CMD", func() {
		if ts.tu.NumQuorumNodes < 2 {
			Skip(fmt.Sprintf("Skipping: %d quorum nodes found, need >= 2", ts.tu.NumQuorumNodes))
		}

		// pause cmd on one of the nodes and check cluster health
		nodeIP := ts.tu.VeniceNodeIPs[0]
		By(fmt.Sprintf("ts: %s Pausing %s on %v", time.Now(), globals.Cmd, nodeIP))
		ts.tu.CommandOutput(nodeIP, fmt.Sprintf("docker pause %s", globals.Cmd))
		Eventually(func() bool {
			return checkClusterHealth(clusterIf, &obj, cmd.ConditionStatus_FALSE.String(),
				fmt.Sprintf("node %s is not healthy", ts.tu.IPToNameMap[nodeIP]))
		}, 200, 2).Should(BeTrue(),
			fmt.Sprintf("ts: %s Cluster status is expected to be un-healthy as %s is not running on %s",
				time.Now(), globals.Cmd, nodeIP))

		// resume cmd and check cluster status
		By(fmt.Sprintf("ts: %s Unpausing %s on %v", time.Now(), globals.Cmd, nodeIP))
		ts.tu.CommandOutput(nodeIP, fmt.Sprintf("docker unpause %s", globals.Cmd))
		Eventually(func() bool {
			return checkClusterHealth(clusterIf, &obj, cmd.ConditionStatus_TRUE.String(), "")
		}, 200, 2).Should(BeTrue(),
			fmt.Sprintf("ts: %s Cluster status is expected to be healthy as %s is resumed back on node %s",
				time.Now(), globals.Cmd, nodeIP))
	})
})

// helper to check if the cluster health meets the given expected condition
func checkClusterHealth(clusterIf cmd.ClusterV1ClusterInterface, clusterObjMeta *api.ObjectMeta,
	expectedHealthCondition, expectedReason string) bool {
	cl, err := clusterIf.Get(ts.tu.MustGetLoggedInContext(context.Background()), clusterObjMeta)
	if err != nil {
		return false
	}
	for _, cond := range cl.Status.Conditions {
		if cond.Type == cmd.ClusterCondition_HEALTHY.String() {
			return cond.Status == expectedHealthCondition && strings.Contains(cond.Reason, expectedReason)
		}
	}
	return false
}
