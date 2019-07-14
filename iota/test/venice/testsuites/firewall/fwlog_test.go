// {C} Copyright 2019 Pensando Systems Inc. All rights reserved.

package firewall_test

import (
	"fmt"
	"time"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)

var _ = Describe("fwlog tests", func() {
	BeforeEach(func() {
		// verify cluster is in good health
		Eventually(func() error {
			return ts.model.Action().VerifyClusterStatus()
		}).Should(Succeed())

	})

	AfterEach(func() {
		ts.tb.AfterTestCommon()

		// delete test policy if its left over. we can ignore thes error here
		ts.model.SGPolicy("test-policy").Delete()
		ts.model.DefaultSGPolicy().Delete()

		// recreate default allow policy
		Expect(ts.model.DefaultSGPolicy().Commit()).ShouldNot(HaveOccurred())

		// verify policy was propagated correctly
		Eventually(func() error {
			return ts.model.Action().VerifyPolicyStatus(ts.model.DefaultSGPolicy())
		}).Should(Succeed())
	})

	Context("verify fwlog on traffic ", func() {
		It("should log ICMP allow in fwlog", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling on naples sim till shm flag is enabled")
			}

			workloadPairs := ts.model.WorkloadPairs().WithinNetwork()
			//Naples time is set in UTC
			startTime := time.Now().UTC()

			Eventually(func() error {
				return ts.model.Action().PingPairs(workloadPairs)
			}).Should(Succeed())

			By(fmt.Sprintf("workload ip address %+v", workloadPairs.ListIPAddr()))

			// check fwlog
			Eventually(func() error {
				return ts.model.Action().FindFwlogForWorkloadPairs("ICMP", "allow", startTime.String(), 0, workloadPairs.ReversePairs())
			}).Should(Succeed())
		})

		It("should log TCP/8000 allow in fwlog", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling on naples sim till shm flag is enabled")
			}

			workloadPairs := ts.model.WorkloadPairs().Permit(ts.model.DefaultSGPolicy(), "tcp")
			//Naples time is set in UTC
			startTime := time.Now().UTC()

			Eventually(func() error {
				return ts.model.Action().TCPSession(workloadPairs, 8000)
			}).Should(Succeed())

			By(fmt.Sprintf("workload ip address %+v", workloadPairs.ListIPAddr()))

			// check fwlog
			Eventually(func() error {
				return ts.model.Action().FindFwlogForWorkloadPairs("TCP", "allow", startTime.String(), 8000, workloadPairs)
			}).Should(Succeed())
		})

		It("should log UDP/9000 allow in fwlog", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling on naples sim till shm flag is enabled")
			}

			Expect(ts.model.DefaultSGPolicy().Delete()).Should(Succeed())

			workloadPairs := ts.model.WorkloadPairs().WithinNetwork()
			//Naples time is set in UTC
			startTime := time.Now().UTC()
			// allow policy
			policy := ts.model.NewSGPolicy("test-policy").AddRule("any", "any", "udp/0-65535", "PERMIT")
			Expect(policy.Commit()).ShouldNot(HaveOccurred())

			By(fmt.Sprintf("workload ip address %+v", workloadPairs.ListIPAddr()))

			Eventually(func() error {
				return ts.model.Action().UDPSession(workloadPairs, 9000)
			}).Should(Succeed())

			// check fwlog
			Eventually(func() error {
				return ts.model.Action().FindFwlogForWorkloadPairs("UDP", "allow", startTime.String(), 9000, workloadPairs)
			}).Should(Succeed())
		})

		It("should log ICMP deny in fwlog", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling on naples sim till shm flag is enabled")
			}

			Expect(ts.model.DefaultSGPolicy().Delete()).Should(Succeed())
			//Naples time is set in UTC
			startTime := time.Now().UTC()

			// deny policy
			denyPolicy := ts.model.NewSGPolicy("test-policy").AddRule("any", "any", "icmp", "DENY")
			denyPolicy.AddRule("any", "any", "", "PERMIT")
			Expect(denyPolicy.Commit()).ShouldNot(HaveOccurred())

			// verify policy was propagated correctly
			Eventually(func() error {
				return ts.model.Action().VerifyPolicyStatus(ts.model.SGPolicy("test-policy"))
			}).Should(Succeed())

			workloadPairs := ts.model.WorkloadPairs().WithinNetwork()
			By(fmt.Sprintf("workload ip address %+v", workloadPairs.ListIPAddr()))

			Eventually(func() error {
				return ts.model.Action().PingFails(workloadPairs)
			}).Should(Succeed())

			// check fwlog
			Eventually(func() error {
				return ts.model.Action().FindFwlogForWorkloadPairs("ICMP", "deny", startTime.String(), 0, workloadPairs.ReversePairs())
			}).Should(Succeed())
		})

		It("should log TCP/8100 deny in fwlog", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling on naples sim till shm flag is enabled")
			}

			Expect(ts.model.DefaultSGPolicy().Delete()).Should(Succeed())
			//Naples time is set in UTC
			startTime := time.Now().UTC()

			// deny policy
			debyPolicy := ts.model.NewSGPolicy("test-policy").AddRule("any", "any", "tcp/0-65535", "DENY")
			debyPolicy.AddRule("any", "any", "", "PERMIT")
			Expect(debyPolicy.Commit()).ShouldNot(HaveOccurred())

			// verify policy was propagated correctly
			Eventually(func() error {
				return ts.model.Action().VerifyPolicyStatus(ts.model.SGPolicy("test-policy"))
			}).Should(Succeed())

			workloadPairs := ts.model.WorkloadPairs().WithinNetwork()
			By(fmt.Sprintf("workload ip address %+v", workloadPairs.ListIPAddr()))

			Eventually(func() error {
				return ts.model.Action().TCPSessionFails(workloadPairs, 8100)
			}).Should(Succeed())

			// check fwlog
			Eventually(func() error {
				return ts.model.Action().FindFwlogForWorkloadPairs("TCP", "deny", startTime.String(), 8100, workloadPairs)
			}).Should(Succeed())
		})

		It("should log UDP/9100 deny in fwlog", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling on naples sim till shm flag is enabled")
			}

			Expect(ts.model.DefaultSGPolicy().Delete()).Should(Succeed())

			// deny policy
			debyPolicy := ts.model.NewSGPolicy("test-policy").AddRule("any", "any", "udp/0-65535", "DENY")
			debyPolicy.AddRule("any", "any", "", "PERMIT")
			Expect(debyPolicy.Commit()).ShouldNot(HaveOccurred())

			// verify policy was propagated correctly
			Eventually(func() error {
				return ts.model.Action().VerifyPolicyStatus(ts.model.SGPolicy("test-policy"))
			}).Should(Succeed())

			workloadPairs := ts.model.WorkloadPairs().WithinNetwork()
			//Naples time is set in UTC
			startTime := time.Now().UTC()
			By(fmt.Sprintf("workload ip address %+v", workloadPairs.ListIPAddr()))

			Eventually(func() error {
				return ts.model.Action().UDPSessionFails(workloadPairs, 9100)
			}).Should(Succeed())

			// check fwlog
			Eventually(func() error {
				return ts.model.Action().FindFwlogForWorkloadPairs("UDP", "deny", startTime.String(), 9100, workloadPairs)
			}).Should(Succeed())
		})

		It("should log ICMP reject in fwlog", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling on naples sim till shm flag is enabled")
			}

			Expect(ts.model.DefaultSGPolicy().Delete()).Should(Succeed())
			//Naples time is set in UTC
			startTime := time.Now().UTC()

			// reject policy
			denyPolicy := ts.model.NewSGPolicy("test-policy").AddRule("any", "any", "icmp", "REJECT")
			denyPolicy.AddRule("any", "any", "", "PERMIT")
			Expect(denyPolicy.Commit()).ShouldNot(HaveOccurred())

			// verify policy was propagated correctly
			Eventually(func() error {
				return ts.model.Action().VerifyPolicyStatus(ts.model.SGPolicy("test-policy"))
			}).Should(Succeed())

			workloadPairs := ts.model.WorkloadPairs().WithinNetwork()
			Eventually(func() error {
				return ts.model.Action().PingFails(workloadPairs)
			}).Should(Succeed())

			// check fwlog
			Eventually(func() error {
				return ts.model.Action().FindFwlogForWorkloadPairs("ICMP", "reject", startTime.String(), 0, workloadPairs.ReversePairs())
			}).Should(Succeed())
		})

		It("should log TCP/8200 reject in fwlog", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling on naples sim till shm flag is enabled")
			}

			Expect(ts.model.DefaultSGPolicy().Delete()).Should(Succeed())
			//Naples time is set in UTC
			startTime := time.Now().UTC()

			// deny policy
			denyPolicy := ts.model.NewSGPolicy("test-policy").AddRule("any", "any", "tcp/0-65535", "REJECT")
			denyPolicy.AddRule("any", "any", "", "PERMIT")
			Expect(denyPolicy.Commit()).ShouldNot(HaveOccurred())

			// verify policy was propagated correctly
			Eventually(func() error {
				return ts.model.Action().VerifyPolicyStatus(ts.model.SGPolicy("test-policy"))
			}).Should(Succeed())

			workloadPairs := ts.model.WorkloadPairs().WithinNetwork()
			By(fmt.Sprintf("workload ip address %+v", workloadPairs.ListIPAddr()))

			Eventually(func() error {
				return ts.model.Action().TCPSessionFails(workloadPairs, 8200)
			}).Should(Succeed())

			// check fwlog
			Eventually(func() error {
				return ts.model.Action().FindFwlogForWorkloadPairs("TCP", "reject", startTime.String(), 8200, workloadPairs)
			}).Should(Succeed())
		})

		It("should log UDP/9200 reject in fwlog", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling on naples sim till shm flag is enabled")
			}

			Expect(ts.model.DefaultSGPolicy().Delete()).Should(Succeed())

			// reject policy
			rejectPolicy := ts.model.NewSGPolicy("test-policy").AddRule("any", "any", "udp/0-65535", "REJECT")
			rejectPolicy.AddRule("any", "any", "", "PERMIT")
			Expect(rejectPolicy.Commit()).ShouldNot(HaveOccurred())

			// verify policy was propagated correctly
			Eventually(func() error {
				return ts.model.Action().VerifyPolicyStatus(ts.model.SGPolicy("test-policy"))
			}).Should(Succeed())

			workloadPairs := ts.model.WorkloadPairs().WithinNetwork()
			//Naples time is set in UTC
			startTime := time.Now().UTC()
			By(fmt.Sprintf("workload ip address %+v", workloadPairs.ListIPAddr()))

			Eventually(func() error {
				return ts.model.Action().UDPSessionFails(workloadPairs, 9200)
			}).Should(Succeed())

			// check fwlog
			Eventually(func() error {
				return ts.model.Action().FindFwlogForWorkloadPairs("UDP", "reject", startTime.String(), 9200, workloadPairs)
			}).Should(Succeed())
		})
	})
})
