package equinix_test

import (
	"context"
	"fmt"
	"regexp"
	"time"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"

	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/objects"
)

func runTest(policy *objects.FwlogPolicyCollection, collectors *objects.VeniceNodeCollection, wpc *objects.WorkloadPairCollection) {
	By(fmt.Sprintf("fwlog policy: %v", policy.Policies[0]))

	// Commit the policy.
	Expect(policy.Commit()).Should(Succeed())

	// Run UDP servers to collect syslog messages.
	ctx, cancel := context.WithCancel(context.Background())
	outChs := ts.model.RuncCommandUntilCancel(ctx, collectors, "nc -u -l 9999")
	time.Sleep(1 * time.Second)

	// Trigger workload pings.
	Eventually(func() error {
		return ts.model.PingPairs(wpc)
	}).Should(Succeed())
	time.Sleep(1 * time.Second)

	// Stop UDP servers.
	cancel()

	// Check the syslogs messages captured.
	By(fmt.Sprintf("number of outChs: %v", len(outChs)))
	for i, ch := range outChs {
		By(fmt.Sprintf("Fwlogs collected on node %v", collectors.Nodes[i].GetTestNode().NodeMgmtIP))
		for s := range ch {
			By(s)
			Expect(validateFwlogs(s, wpc)).ShouldNot(Succeed())
		}
	}
}

func validateFwlogs(logs string, wpc *objects.WorkloadPairCollection) error {
	exp := "localhost[ a-zA-Z]*: Allow[ a-zA-Z0-9:,-.]*"

	for _, wp := range wpc.Pairs {
		sip, dip := wp.First.GetIP(), wp.Second.GetIP()
		rexp := fmt.Sprintf("%s src %s:[0-9]*, dst %s", exp, sip, dip)
		re, _ := regexp.Compile(rexp)
		if str := re.FindString(logs); str == "" {
			By(fmt.Sprintf("cannot find flow log for sip %v -> dip %v", sip, dip))
			return fmt.Errorf("cannot find flow log for sip %v -> dip %v", sip, dip)
		}
	}

	By("flog logs check: PASS")
	return nil
}

var _ = Describe("fwlog UDP syslog tests: ICMP", func() {
	var startTime time.Time
	var workloadPairs *objects.WorkloadPairCollection
	var numOfVeniceNodes int
	var veniceIPAddrs []string
	var fwlogPolicy *objects.FwlogPolicyCollection

	BeforeEach(func() {
		// Verify cluster is in good health.
		startTime = time.Now().UTC()
		Eventually(func() error {
			return ts.model.VerifyClusterStatus()
		}).Should(Succeed())

		// Get workload pairs.
		workloadPairs = ts.model.WorkloadPairs().WithinNetwork()
		By(fmt.Sprintf("workload ip address %+v", workloadPairs.ListIPAddr()))

		// Get count of Venice nodes.
		numOfVeniceNodes = len(ts.model.VeniceNodes().Nodes)

		// Get IP addresses of all Venice nodes.
		for _, node := range ts.model.VeniceNodes().Nodes {
			veniceIPAddrs = append(veniceIPAddrs, node.GetTestNode().NodeMgmtIP)
		}
		By(fmt.Sprintf("venice ip address %+v", veniceIPAddrs))
	})

	AfterEach(func() {
		// Expect No Service is stopped
		Expect(ts.model.ServiceStoppedEvents(startTime, ts.model.Naples()).Len(0))
		veniceIPAddrs = veniceIPAddrs[:0]
	})

	Context("fwlog UDP syslog tests: ICMP", func() {
		It("Create operation", func() {
			Skip("Skipping till operd supports syslog export of flow logs")

			if !ts.tb.HasNaplesHW() {
				Skip("Not supported on sim")
			}

			// Create fwlog policy with 1 collector.
			collectors := objects.VeniceNodeCollection{}
			fwlogPolicy = ts.model.NewFwlogPolicy("test-fwlog-policy")
			fwlogPolicy.Policies[0].VenicePolicy.Spec.Targets[0].Destination = veniceIPAddrs[0]
			fwlogPolicy.Policies[0].VenicePolicy.Spec.Targets[0].Transport = "udp/9999"
			collectors.Nodes = append(collectors.Nodes, ts.model.VeniceNodes().Nodes[0])

			runTest(fwlogPolicy, &collectors, workloadPairs)
		})

		It("Update operation 1", func() {
			// Update operation by changing an existing collector
			Skip("Update operation not supported yet")

			if !ts.tb.HasNaplesHW() {
				Skip("Not supported on sim")
			}

			if numOfVeniceNodes < 2 {
				Skip("Need at least 2 Venice nodes")
			}

			collectors := objects.VeniceNodeCollection{}
			fwlogPolicy.Policies[0].VenicePolicy.Spec.Targets[0].Destination = veniceIPAddrs[1]
			fwlogPolicy.Policies[0].VenicePolicy.Spec.Targets[0].Transport = "udp/9999"
			collectors.Nodes = append(collectors.Nodes, ts.model.VeniceNodes().Nodes[1])

			runTest(fwlogPolicy, &collectors, workloadPairs)
		})

		It("Update operation 2", func() {
			// Update operation by adding a new collector
			Skip("Update operation not supported yet")

			if !ts.tb.HasNaplesHW() {
				Skip("Not supported on sim")
			}

			if numOfVeniceNodes < 2 {
				Skip("Need at least 2 Venice nodes")
			}

			collectors := objects.VeniceNodeCollection{}
			collectors.Nodes = append(collectors.Nodes, ts.model.VeniceNodes().Nodes[1])

			target := monitoring.ExportConfig{}
			target.Destination = veniceIPAddrs[0]
			target.Transport = "udp/9999"
			fwlogPolicy.Policies[0].VenicePolicy.Spec.Targets = append(fwlogPolicy.Policies[0].VenicePolicy.Spec.Targets, target)
			collectors.Nodes = append(collectors.Nodes, ts.model.VeniceNodes().Nodes[0])

			runTest(fwlogPolicy, &collectors, workloadPairs)
		})

		It("Update operation 3", func() {
			// Update operation by deleting an existing collector
			Skip("Update operation not supported yet")

			if !ts.tb.HasNaplesHW() {
				Skip("Not supported on sim")
			}

			if numOfVeniceNodes < 2 {
				Skip("Need at least 2 Venice nodes")
			}

			collectors := objects.VeniceNodeCollection{}
			fwlogPolicy.Policies[0].VenicePolicy.Spec.Targets = fwlogPolicy.Policies[0].VenicePolicy.Spec.Targets[1:]
			collectors.Nodes = append(collectors.Nodes, ts.model.VeniceNodes().Nodes[0])

			runTest(fwlogPolicy, &collectors, workloadPairs)
		})

		It("Delete operation", func() {
			Skip("Delete operation not supported yet")

			if !ts.tb.HasNaplesHW() {
				Skip("Not supported on sim")
			}

			Expect(fwlogPolicy.Delete()).Should(Succeed())
		})
	})
})
