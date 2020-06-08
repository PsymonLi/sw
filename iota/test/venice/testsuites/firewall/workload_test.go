// {C} Copyright 2019 Pensando Systems Inc. All rights reserved.

package firewall_test

import (
	"fmt"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)

var _ = Describe("workload tests", func() {
	BeforeEach(func() {
		// verify cluster is in good health
		Eventually(func() error {
			return ts.model.VerifyClusterStatus()
		}).Should(Succeed())
	})
	AfterEach(func() {
	})

	Context("Basic workload tests", func() {
		It("Should be able to bringup new workloads and connect between them", func() {
			return
			workloads := ts.model.BringUpNewWorkloads(ts.model.Hosts(), ts.model.Networks("").Any(1), 1)
			Expect(workloads.Error()).ShouldNot(HaveOccurred())
			// verify workload status is good
			Eventually(func() error {
				return ts.model.VerifyWorkloadStatus(workloads)
			}).Should(Succeed())

			// check ping between new workloads
			Eventually(func() error {
				return ts.model.TCPSession(workloads.MeshPairs().Any(4), 8000)
			}).ShouldNot(HaveOccurred())

			//Teardown the new workloads
			err := ts.model.TeardownWorkloads(workloads)
			Expect(err == nil)
		})

		It("Delete Workloads when npm is down and make sure endpoints are cleaned up", func() {

			vnc := ts.model.VeniceNodes()
			Expect(len(vnc.Nodes) != 0).Should(BeTrue())

			npmNodes, err := vnc.GetVeniceContainersWithService("pen-npm", false)
			Expect(err).ShouldNot(HaveOccurred())
			Expect(len(npmNodes.Containers) != 0).Should(BeTrue())

			Expect(npmNodes.Pause()).ShouldNot(HaveOccurred())

			wlds, err := ts.model.ConfigClient().ListWorkload()
			Expect(err).ShouldNot(HaveOccurred())
			Expect(len(wlds) != 0).Should(BeTrue())

			Expect(ts.model.ConfigClient().DeleteWorkloads(wlds)).ShouldNot(HaveOccurred())

			eps, err := ts.model.ConfigClient().ListEndpoints("")
			Expect(err).ShouldNot(HaveOccurred())
			Expect(len(eps) != 0).Should(BeTrue())

			Expect(npmNodes.UnPause()).ShouldNot(HaveOccurred())

			Eventually(func() error {
				eps, err := ts.model.ConfigClient().ListEndpoints("")
				if err != nil {
					return err
				}
				if len(eps) != 0 {
					return fmt.Errorf("Endpoints still not cleaned up")
				}
				return nil
			}).Should(Succeed())

			//Ping should not succeed
			workloadPairs := ts.model.WorkloadPairs().WithinNetwork().Any(4)
			Eventually(func() error {
				return ts.model.PingPairs(workloadPairs)
			}).ShouldNot(Succeed())

			//Restore worklaods
			Expect(ts.model.ConfigClient().CreateWorkloads(wlds)).ShouldNot(HaveOccurred())

			Eventually(func() error {
				eps, err := ts.model.ConfigClient().ListEndpoints("")
				if err != nil {
					return err
				}
				if len(eps) == 0 {
					return fmt.Errorf("Endpoints not restored")
				}
				return nil
			}).Should(Succeed())

		})
	})
})
