package firewall_test

import (
	"fmt"
	"time"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/objects"
)

const (
	timeFormat = "2006-01-02T15:04:05"
	bucketName = "fwlogs"
	tenantName = "default"
)

var _ = Describe("tests for storing firewall logs in object store and elastic", func() {
	var startTime time.Time
	BeforeEach(func() {
		// verify cluster is in good health
		startTime = time.Now().UTC()
		Eventually(func() error {
			return ts.model.VerifyClusterStatus()
		}).Should(Succeed())
	})

	AfterEach(func() {
		//Expect No Service is stopped
		Expect(ts.model.ServiceStoppedEvents(startTime, ts.model.Naples()).Len(0))

		// delete test policy if its left over. we can ignore thes error here
		ts.model.NetworkSecurityPolicy("test-policy").Delete()
		ts.model.DefaultNetworkSecurityPolicy().Delete()

		Expect(ts.model.DefaultNetworkSecurityPolicy().Restore()).ShouldNot(HaveOccurred())

		// verify policy was propagated correctly
		Eventually(func() error {
			return ts.model.VerifyPolicyStatus(ts.model.DefaultNetworkSecurityPolicy())
		}).Should(Succeed())
	})

	// TODO: Checking exact losg needs much more backend work before tests can be written for it.
	// MinIO's SQL queries would have to get exposed through API Gateway for querying exact logs from a bucket.
	// ObjectStore's client interface would have to get extended to included SQL queries.
	// For now, just checking that number of objects in the bucket are increasing when logs are getting uploaded
	// to the bucket.
	Context("verify fwlog on traffic in objectstore and elastic", func() {
		It("tags:regression=true venice isolate APIServer node, should not affect reporting of fwlogs", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("runs only on hardware naples")
			}
			ts.model.WorkloadPairs().WithinNetwork().Permit(ts.model.DefaultNetworkSecurityPolicy(), "icmp")

			naples := ts.model.Naples()
			apiServerNode, err := ts.model.VeniceNodes().GetVeniceNodeWithService("pen-apiserver")
			Expect(err).ShouldNot(HaveOccurred())

			// Get the Venice node ip
			isolatedNodeIP := apiServerNode.Nodes[0].IP()
			fmt.Println("Isolating node", isolatedNodeIP)
			err = ts.model.DisconnectVeniceNodesFromCluster(apiServerNode, naples)
			Expect(err).ShouldNot(HaveOccurred())

			// Sleep for 60 seconds to make sure we detect partitioning.
			time.Sleep(800 * time.Second)
			Eventually(func() error {
				return ts.model.VerifyClusterStatus()
			}).Should(Succeed())

			// Push logs and verify
			pushLogsAndVerify(time.Second*800, isolatedNodeIP)

			//Connect Back and make sure cluster is good
			fmt.Println("Bringing back node", isolatedNodeIP)
			ts.model.ConnectVeniceNodesToCluster(apiServerNode, naples)
			time.Sleep(60 * time.Second)
			Eventually(func() error {
				return ts.model.VerifyClusterStatus()
			}).Should(Succeed())

			// Again verify
			pushLogsAndVerify(time.Second * 400)
		})

		// Skpping this test case due to issues in Minio when a node is isolated using a firewall rule.
		// In such a case, Minio still tries to contact to the isolated disk and timesout after 1min. As
		// a result all the get/upload request time increases by 1 min. If there are many fwlogs, then listing
		// of objects will take huge amount of time. The correct solution is to move listing of objects
		// from minio to Elastic. In Elastic we maintain an index of all minio objects.
		It("tags:regression=true venice isolate nodes in a loop, should not affect reporting of fwlogs", func() {
			Skip("reenable this test after moving the backed implementation to Elastic")
			if !ts.tb.HasNaplesHW() {
				Skip("runs only on hardware naples")
			}
			ts.model.WorkloadPairs().WithinNetwork().Permit(ts.model.DefaultNetworkSecurityPolicy(), "icmp")

			ts.model.ForEachVeniceNode(func(vnc *objects.VeniceNodeCollection) error {
				naples := ts.model.Naples()
				err := ts.model.DisconnectVeniceNodesFromCluster(vnc, naples)
				Expect(err).ShouldNot(HaveOccurred())

				// Sleep for 60 seconds to make sure we detect partitioning.
				time.Sleep(800 * time.Second)
				Eventually(func() error {
					return ts.model.VerifyClusterStatus()
				}).Should(Succeed())

				// Push logs and verify
				pushLogsAndVerify(time.Second * 800)

				//Connect Back and make sure cluster is good
				ts.model.ConnectVeniceNodesToCluster(vnc, naples)
				time.Sleep(60 * time.Second)
				Eventually(func() error {
					return ts.model.VerifyClusterStatus()
				}).Should(Succeed())

				// Again verify
				pushLogsAndVerify(time.Second * 400)

				return nil
			})
		})

		It("tags:regression=true should push fwlog to objectstore", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("runs only on hardware naples")
			}
			ts.model.WorkloadPairs().WithinNetwork().Permit(ts.model.DefaultNetworkSecurityPolicy(), "icmp")
			pushLogsAndVerify(time.Second * 800)
		})

		It("tags:regression=true reloading venice nodes should not affect fwlogs", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("runs only on hardware naples")
			}
			ts.model.WorkloadPairs().WithinNetwork().Permit(ts.model.DefaultNetworkSecurityPolicy(), "icmp")

			// reload each host
			ts.model.ForEachVeniceNode(func(vnc *objects.VeniceNodeCollection) error {
				reloadingNodeIP := vnc.Nodes[0].IP()
				fmt.Println("Reloading node", reloadingNodeIP)
				Expect(ts.model.ReloadVeniceNodes(vnc)).Should(Succeed())

				// Again verify
				pushLogsAndVerify(time.Second*800, reloadingNodeIP)

				// wait for cluster to be back in good state
				Eventually(func() error {
					return ts.model.VerifyClusterStatus()
				}).Should(Succeed())

				return nil
			})
		})
	})
})

func pushLogsAndVerify(timeout time.Duration, nodeIpsToSkipFromQuery ...string) {
	fmt.Println("IPstoSkip", nodeIpsToSkipFromQuery)

	workloadPairs := ts.model.WorkloadPairs().WithinNetwork()

	// Get the naples id from the workload
	workloadA := workloadPairs.Pairs[0].First
	workloadB := workloadPairs.Pairs[0].Second
	naplesAMac := workloadA.NaplesMAC()
	naplesBMac := workloadB.NaplesMAC()
	currentObjectCountNaplesA, currentObjectCountNaplesB := 0, 0
	var err error
	jitter := 30 * time.Second
	Eventually(func() error {
		currentObjectCountNaplesA, err =
			ts.model.GetFwLogObjectCount(tenantName, bucketName, naplesAMac, jitter, nodeIpsToSkipFromQuery...)
		jitter = jitter + (30 * time.Second)
		return err
	}).Should(Succeed())

	jitter = 30 * time.Second
	if naplesAMac != naplesBMac {
		Eventually(func() error {
			currentObjectCountNaplesB, err =
				ts.model.GetFwLogObjectCount(tenantName, bucketName, naplesBMac, jitter, nodeIpsToSkipFromQuery...)
			jitter = jitter + (30 * time.Second)
			return err
		}).Should(Succeed())
	}

	By(fmt.Sprintf("currentObjectCountNaplesA %d, currentObjectCountNaplesB %d",
		currentObjectCountNaplesA, currentObjectCountNaplesB))

	// Do it in a loop, and verify that the object count keeps going up in object store
	for i := 0; i < 3; i++ {
		Eventually(func() error {
			return ts.model.PingPairs(workloadPairs)
		}).Should(Succeed())

		By(fmt.Sprintf("workload ip address %+v", workloadPairs.ListIPAddr()))
		By(fmt.Sprintf("naplesAMac %+v, naplesBMac %+v", naplesAMac, naplesBMac))

		// check object count
		Eventually(func() bool {
			newObjectCountNaplesA, newObjectCountNaplesB := 0, 0
			var err error

			jitter = 30 * time.Second
			Eventually(func() error {
				newObjectCountNaplesA, err =
					ts.model.GetFwLogObjectCount(tenantName, bucketName, naplesAMac, jitter, nodeIpsToSkipFromQuery...)
				jitter = jitter + (30 * time.Second)
				return err
			}).Should(Succeed())

			jitter = 30 * time.Second
			if naplesAMac != naplesBMac {
				Eventually(func() error {
					newObjectCountNaplesB, err =
						ts.model.GetFwLogObjectCount(tenantName, bucketName, naplesBMac, jitter, nodeIpsToSkipFromQuery...)
					jitter = jitter + (30 * time.Second)
					return err
				}).Should(Succeed())
			}

			if naplesAMac != naplesBMac {
				By(fmt.Sprintf("newObjectCountNaplesA %d, newObjectCountNaplesB %d",
					newObjectCountNaplesA, newObjectCountNaplesB))
				if newObjectCountNaplesA > currentObjectCountNaplesA && newObjectCountNaplesB > currentObjectCountNaplesB {
					currentObjectCountNaplesA = newObjectCountNaplesA
					currentObjectCountNaplesB = newObjectCountNaplesB
					return true
				}
				return false
			} else {
				By(fmt.Sprintf("newObjectCountNaplesA %d", newObjectCountNaplesA))
				if newObjectCountNaplesA > currentObjectCountNaplesA {
					currentObjectCountNaplesA = newObjectCountNaplesA
					currentObjectCountNaplesB = newObjectCountNaplesA
					return true
				}
				return false
			}

		}, timeout).Should(BeTrue())
	}
}
