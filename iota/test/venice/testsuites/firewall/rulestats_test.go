// {C} Copyright 2019 Pensando Systems Inc. All rights reserved.

package firewall_test

import (
	"time"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)

var _ = Describe("rulestats tests", func() {
	BeforeEach(func() {
		// verify cluster is in good health
		Eventually(func() error {
			return ts.model.Action().VerifyClusterStatus()
		}).Should(Succeed())

		// delete the default allow policy
		Expect(ts.model.DefaultNetworkSecurityPolicy().Delete()).ShouldNot(HaveOccurred())
	})

	AfterEach(func() {
		ts.tb.AfterTestCommon()

		// delete test policy if its left over. we can ignore the error here
		ts.model.NetworkSecurityPolicy("test-policy").Delete()
		ts.model.DefaultNetworkSecurityPolicy().Delete()

		// recreate default allow policy
		Expect(ts.model.DefaultNetworkSecurityPolicy().Restore()).ShouldNot(HaveOccurred())
	})

	Context("tags:type=basic;datapath=true;duration=short  Verify rulestats on traffic ", func() {
		It("tags:sanity=true TCP Rule stats should increment for default allow all policy", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling rule stats test on naples sim")
			}
			spc := ts.model.NewNetworkSecurityPolicy("test-policy").AddRule("any", "any", "any", "PERMIT")
			Expect(spc.Commit()).ShouldNot(HaveOccurred())

			// verify policy was propagated correctly
			Eventually(func() error {
				return ts.model.Action().VerifyPolicyStatus(spc)
			}).Should(Succeed())

			//Naples time is set in UTC
			startTime := time.Now().UTC()
			// establish TCP session between workload pairs in same subnet
			workloadPairs := ts.model.WorkloadPairs().WithinNetwork().Any(1)
			Eventually(func() error {
				return ts.model.Action().TCPSession(workloadPairs, 8000)
			}).Should(Succeed())
			time.Sleep(time.Second * 30)

			// check fwlog
			Eventually(func() error {
				return ts.model.Action().FindFwlogForWorkloadPairs("TCP", "allow", startTime.String(), 8000, workloadPairs)
			}).Should(Succeed())

			// verify TCP hits and total hits got incremented
			expCount := []map[string]float64{{"TotalHits": 1, "TcpHits": 1}}
			Eventually(func() error {
				return ts.model.Action().VerifyRuleStats(startTime.String(), spc, expCount)
			}).Should(Succeed())

		})
		It("UDP Rule stats should increment for default allow all policy", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling rule stats test on naples sim")
			}
			spc := ts.model.NewNetworkSecurityPolicy("test-policy").AddRule("any", "any", "any", "PERMIT")
			Expect(spc.Commit()).ShouldNot(HaveOccurred())

			// verify policy was propagated correctly
			Eventually(func() error {
				return ts.model.Action().VerifyPolicyStatus(spc)
			}).Should(Succeed())

			//Naples time is set in UTC
			startTime := time.Now().UTC()
			// establish UDP session between workload pairs
			workloadPairs := ts.model.WorkloadPairs().WithinNetwork().Any(1)
			Eventually(func() error {
				return ts.model.Action().UDPSession(workloadPairs, 9000)
			}).Should(Succeed())
			time.Sleep(time.Second * 30)

			// check fwlog
			Eventually(func() error {
				return ts.model.Action().FindFwlogForWorkloadPairs("UDP", "allow", startTime.String(), 9000, workloadPairs)
			}).Should(Succeed())

			// verify UDP hits and total hits got incremented
			expCount := []map[string]float64{{"TotalHits": 1, "UdpHits": 1}}
			Eventually(func() error {
				return ts.model.Action().VerifyRuleStats(startTime.String(), spc, expCount)
			}).Should(Succeed())

		})
		It("Rule stats should increment for default allow all policy", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling rule stats test on naples sim")
			}
			spc := ts.model.NewNetworkSecurityPolicy("test-policy").AddRule("any", "any", "any", "PERMIT")
			Expect(spc.Commit()).ShouldNot(HaveOccurred())

			// verify policy was propagated correctly
			Eventually(func() error {
				return ts.model.Action().VerifyPolicyStatus(spc)
			}).Should(Succeed())

			//Naples time is set in UTC
			startTime := time.Now().UTC()

			// establish ICMP session between workload pairs
			workloadPairs := ts.model.WorkloadPairs().WithinNetwork().Any(1)
			Eventually(func() error {
				return ts.model.Action().PingPairs(workloadPairs)
			}).Should(Succeed())
			time.Sleep(time.Second * 30)

			// check fwlog
			Eventually(func() error {
				return ts.model.Action().FindFwlogForWorkloadPairs("ICMP", "allow", startTime.String(), 0, workloadPairs.ReversePairs())
			}).Should(Succeed())

			// verify ICMP hits and total hits got incremented
			expCount := []map[string]float64{{"TotalHits": 1, "IcmpHits": 1}}
			Eventually(func() error {
				return ts.model.Action().VerifyRuleStats(startTime.String(), spc, expCount)
			}).Should(Succeed())
		})
	})
})
