// {C} Copyright 2019 Pensando Systems Inc. All rights reserved.

package firewall_test

import (
	"errors"
	"time"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)

var _ = Describe("firewall tests", func() {
	var startTime time.Time
	BeforeEach(func() {

		startTime = time.Now().UTC()
		// verify cluster is in good health
		Eventually(func() error {
			return ts.model.Action().VerifyClusterStatus()
		}).Should(Succeed())
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

	Context("tags:type=basic;datapath=true;duration=long Basic firewall tests", func() {
		It("tags:sanity=true Should establish TCP session between all workload with default policy", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling on naples sim till traffic issue is debugged")
			}

			// ping all workload pairs in same subnet
			workloadPairs := ts.model.WorkloadPairs().Permit(ts.model.DefaultNetworkSecurityPolicy(), "tcp")
			Eventually(func() error {
				return ts.model.Action().TCPSession(workloadPairs, 0)
			}).Should(Succeed())
		})

		It("Should not establish TCP session between any workload with deny policy", func() {
			// change the default policy to deny all
			workloadPairs := ts.model.WorkloadPairs().Deny(ts.model.DefaultNetworkSecurityPolicy(), "tcp")
			// randomly pick one workload and verify ping fails between them
			Eventually(func() error {
				//Pass 0 to derice ports from workload pairs
				return ts.model.Action().TCPSessionFails(workloadPairs, 0)
			}).Should(Succeed())

		})

		It("Should establish TCP session between all workload with default policy for longer duration", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling on naples sim till traffic issue is debugged")
			}

			workloadPairs := ts.model.WorkloadPairs().Permit(ts.model.DefaultNetworkSecurityPolicy(), "tcp")
			err := make(chan error)

			go func() {
				err <- ts.model.Action().TCPSessionWithOptions(workloadPairs, 0, "30s", 5)
			}()

			select {
			case <-err:
			case <-time.After(time.Duration(120) * time.Second):
				err <- errors.New("Test timed out")
			}

			Expect(err != nil).To(Equal(true))
		})
	})
})
