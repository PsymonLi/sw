// {C} Copyright 2019 Pensando Systems Inc. All rights reserved.

package firewall_test

import (
	"fmt"
	"math/rand"
	"time"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)

var _ = Describe("firewall whitelist tests", func() {
	var startTime time.Time
	BeforeEach(func() {
		// verify cluster is in good health
		startTime = time.Now().UTC()
		Eventually(func() error {
			return ts.model.Action().VerifyClusterStatus()
		}).Should(Succeed())

		// delete the default allow policy
		Expect(ts.model.DefaultNetworkSecurityPolicy().Delete()).ShouldNot(HaveOccurred())
	})
	AfterEach(func() {
		ts.tb.AfterTestCommon()
		//Expect No Service is stopped
		Expect(ts.model.ServiceStoppedEvents(startTime, ts.model.Naples()).Len(0))

		// delete test policy if its left over. we can ignore the error here
		ts.model.NetworkSecurityPolicy("test-policy").Delete()
		ts.model.DefaultNetworkSecurityPolicy().Delete()

		// recreate default allow policy
		Expect(ts.model.DefaultNetworkSecurityPolicy().Restore()).ShouldNot(HaveOccurred())
	})
	Context("tags:type=basic;datapath=true;duration=short basic whitelist tests", func() {
		It("tags:sanity=true Should not ping between any workload without permit rules", func() {
			workloadPairs := ts.model.WorkloadPairs().WithinNetwork().Any(4)
			Eventually(func() error {
				return ts.model.Action().PingFails(workloadPairs)
			}).Should(Succeed())
		})

		It("tags:sanity=true Should allow TCP connections with specific permit rules", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling on naples sim till traffic issue is debugged")
			}

			// add permit rules for workload pairs
			workloadPairs := ts.model.WorkloadPairs().WithinNetwork().Any(4)
			spc := ts.model.NewNetworkSecurityPolicy("test-policy").AddRulesForWorkloadPairs(workloadPairs, "tcp/8000", "PERMIT")
			Expect(spc.Commit()).Should(Succeed())

			// verify policy was propagated correctly
			Eventually(func() error {
				return ts.model.Action().VerifyPolicyStatus(spc)
			}).Should(Succeed())

			// verify TCP connection works between workload pairs
			Eventually(func() error {
				return ts.model.Action().TCPSession(workloadPairs, 8000)
			}).Should(Succeed())

			// verify ping fails
			Eventually(func() error {
				return ts.model.Action().PingFails(workloadPairs)
			}).Should(Succeed())

			// verify connections in reverse direction fail
			Eventually(func() error {
				return ts.model.Action().TCPSessionFails(workloadPairs.ReversePairs(), 8000)
			}).Should(Succeed())
		})

		It("Should allow UDP connections with specific permit rules", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling UDP test on naples sim till traffic issue is debugged")
			}
			// add permit rules for workload pairs
			workloadPairs := ts.model.WorkloadPairs().WithinNetwork().Any(4)
			spc := ts.model.NewNetworkSecurityPolicy("test-policy").AddRulesForWorkloadPairs(workloadPairs, "udp/8000", "PERMIT")
			Expect(spc.Commit()).Should(Succeed())

			// verify policy was propagated correctly
			Eventually(func() error {
				return ts.model.Action().VerifyPolicyStatus(spc)
			}).Should(Succeed())

			// verify TCP connection works between workload pairs
			Eventually(func() error {
				return ts.model.Action().UDPSession(workloadPairs, 8000)
			}).Should(Succeed())

			// verify ping fails
			Eventually(func() error {
				return ts.model.Action().PingFails(workloadPairs)
			}).Should(Succeed())

			// verify connections in reverse direction fail
			Eventually(func() error {
				return ts.model.Action().UDPSessionFails(workloadPairs.ReversePairs(), 8000)
			}).Should(Succeed())
		})

		It("Ping should work with specific permit rules", func() {
			// add permit rules for workload pairs
			workloadPairs := ts.model.WorkloadPairs().WithinNetwork().Any(4)
			spc := ts.model.NewNetworkSecurityPolicy("test-policy").AddRulesForWorkloadPairs(workloadPairs, "icmp", "PERMIT")
			spc.AddRulesForWorkloadPairs(workloadPairs.ReversePairs(), "icmp", "PERMIT")
			Expect(spc.Commit()).Should(Succeed())

			// verify policy was propagated correctly
			Eventually(func() error {
				return ts.model.Action().VerifyPolicyStatus(spc)
			}).Should(Succeed())

			// verify ping is successful
			Eventually(func() error {
				return ts.model.Action().PingPairs(workloadPairs)
			}).Should(Succeed())

			// verify TCP connections fail
			Eventually(func() error {
				return ts.model.Action().TCPSessionFails(workloadPairs, 8000)
			}).Should(Succeed())
		})
	})
	Context("tags:type=basic;datapath=true;duration=long basic whitelist tests", func() {
		It("Should be able to update policy and verify it takes effect", func() {
			const maxRules = 5000
			const numIter = 10
			startPort := 2000
			boundaryPort := (startPort - 1) + maxRules

			// pick a workload pair
			workloadPairs := ts.model.WorkloadPairs().WithinNetwork().Any(4)

			// run multiple iterations, each time updating the policy with different number of rules
			for iter := 0; iter < numIter; iter++ {
				spc := ts.model.NewNetworkSecurityPolicy("test-policy")
				for i := startPort; i <= boundaryPort; i++ {
					spc = spc.AddRulesForWorkloadPairs(workloadPairs, fmt.Sprintf("tcp/%d", i), "PERMIT")
				}
				Expect(spc.Commit()).Should(Succeed())

				// verify policy was propagated correctly
				Eventually(func() error {
					return ts.model.Action().VerifyPolicyStatus(spc)
				}).Should(Succeed())

				// verify TCP connection works on boundary port
				Eventually(func() error {
					return ts.model.Action().TCPSession(workloadPairs, boundaryPort)
				}).Should(Succeed())

				// verify connections above boundary port does not work
				Eventually(func() error {
					return ts.model.Action().TCPSessionFails(workloadPairs, boundaryPort+1)
				}).Should(Succeed())

				// change the boundary port
				boundaryPort = rand.Intn(maxRules) + startPort

				// increment task count for each iteration
				ts.tb.AddTaskResult("", nil)
			}
		})
	})
})
