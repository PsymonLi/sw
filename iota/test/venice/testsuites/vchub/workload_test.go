// {C} Copyright 2019 Pensando Systems Inc. All rights reserved.

package vchub_test

import (
	"time"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/common"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/objects"
	"github.com/pensando/sw/venice/utils/log"
)

var _ = Describe("Vc hub workload tests", func() {
	startTime := time.Now().UTC()
	BeforeEach(func() {
		// verify cluster is in good health
		startTime = time.Now().UTC()
		Eventually(func() error {
			return ts.model.VerifyClusterStatus()
		}).Should(Succeed())

		// delete the default allow policy
		Expect(ts.model.DefaultNetworkSecurityPolicy().Delete()).ShouldNot(HaveOccurred())
	})
	AfterEach(func() {
		//Expect No Service is stopped
		Expect(ts.model.ServiceStoppedEvents(startTime, ts.model.Naples()).Len(0))

		// delete test policy if its left over. we can ignore the error here
		ts.model.NetworkSecurityPolicy("test-policy").Delete()
		ts.model.DefaultNetworkSecurityPolicy().Delete()

		// recreate default allow policy
		Expect(ts.model.DefaultNetworkSecurityPolicy().Restore()).ShouldNot(HaveOccurred())
	})

	Context("Bring up teardown workloads", func() {

		It("Bring up workload and test traffic", func() {

			workloads := ts.model.Workloads()
			err := ts.model.TeardownWorkloads(workloads)
			Expect(err == nil)

			workloads = ts.model.BringUpNewWorkloads(ts.model.Hosts(), ts.model.Networks("").Any(1), 1)
			Expect(workloads.Error()).ShouldNot(HaveOccurred())

			// verify workload status is good, Put all check w.r.t to venice here.
			Eventually(func() error {
				return ts.model.VerifyWorkloadStatus(workloads)
			}).Should(Succeed())

			spc := ts.model.NewNetworkSecurityPolicy("test-policy").AddRulesForWorkloadPairs(workloads.MeshPairs(), "tcp/8000", "PERMIT")
			Expect(spc.Commit()).Should(Succeed())

			// verify policy was propagated correctly
			Eventually(func() error {
				return ts.model.VerifyPolicyStatus(spc)
			}).Should(Succeed())
			// check ping between new workloads
			Eventually(func() error {
				return ts.model.TCPSession(workloads.MeshPairs(), 8000)
			}).ShouldNot(HaveOccurred())

			/*
				// check ping between new workloads Once Datapath is ready
				Eventually(func() error {
						return ts.model.TCPSession(workloads.RemotePairs().Any(4), 8000)
				}).ShouldNot(HaveOccurred())
			*/

			//Teardown the new workloads
			//err := workloads.Teardown()
			//Expect(err == nil)

			// verify workload status is good, Put all check w.r.t to venice here.
			Eventually(func() error {
				return ts.model.VerifyWorkloadStatus(workloads)
			}).Should(Succeed())
		})

		runTraffic := func(workloadPairs *objects.WorkloadPairCollection, rerr chan error) {
			options := &objects.ConnectionOptions{
				Duration:          "180s",
				Port:              "8000",
				Proto:             "tcp",
				ReconnectAttempts: 100,
			}
			err := ts.model.ConnectionWithOptions(workloadPairs, options)
			log.Infof("TCP SESSION TEST COMPLETE")
			rerr <- err
			return
		}

		It("Vmotion basic test", func() {

			Skip("Skipping basic vmotion test..")
			workloads := ts.model.Workloads()
			err := ts.model.TeardownWorkloads(workloads)
			Expect(err == nil)

			// Skip("Skipping move tests..")
			workloads = ts.model.BringUpNewWorkloads(ts.model.Hosts(), ts.model.Networks("").Any(1), 1)
			Expect(workloads.Error()).ShouldNot(HaveOccurred())

			// verify workload status is good, Put all check w.r.t to venice here.
			Eventually(func() error {
				return ts.model.VerifyWorkloadStatus(workloads)
			}).Should(Succeed())

			spc := ts.model.NewNetworkSecurityPolicy("test-policy").AddRulesForWorkloadPairs(workloads.MeshPairs(), "tcp/8000", "PERMIT")
			Expect(spc.Commit()).Should(Succeed())

			// Select one workload on each host in the same network
			for _, wl := range workloads.Workloads {
				log.Infof("Found Workload %v", wl)
			}
			//Get All possible Host & Corresponding workload combination
			hostWorkloads := ts.model.HostWorkloads()
			log.Infof("Found WL/host = %d", len(hostWorkloads))

			if len(hostWorkloads) < 2 {
				Skip("Skipping vmotion tests as there are not enough workloads")
			}

			log.Infof("Found sufficient worklaods")
			srcHost := hostWorkloads[0].Host()
			srcWorkloads := hostWorkloads[0].Workloads().Any(1)
			dstHost := hostWorkloads[1].Host()
			dstWorkloads := hostWorkloads[1].Workloads().Any(1)

			wPairs := srcWorkloads.MeshPairsWithOther(dstWorkloads)

			terr := make(chan error)

			go runTraffic(wPairs, terr)

			err = ts.model.MoveWorkloads(srcWorkloads, dstHost)
			Expect(err == nil)
			err = ts.model.MoveWorkloads(dstWorkloads, srcHost)
			Expect(err == nil)

			Eventually(func() error {
				return ts.model.VerifyWorkloadMigrationStatus(srcWorkloads)
			}).Should(Succeed())

			Eventually(func() error {
				return ts.model.VerifyWorkloadMigrationStatus(dstWorkloads)
			}).Should(Succeed())

			Expect(terr == nil)
		})

		It("Bring up 8 local and remote workloads and test traffic", func() {

			workloads := ts.model.Workloads()
			err := ts.model.TeardownWorkloads(workloads)
			Expect(err == nil)

			// verify workload status is good, Put all check w.r.t to venice here.
			Eventually(func() error {
				return ts.model.VerifyWorkloadStatus(workloads)
			}).Should(Succeed())

			// Use 4 WLs on host 1 and 2 (two in each network)
			networks := ts.model.Networks("").Any(2)

			// Run traffic between 8 remote pairs

			workloads = ts.model.BringUpNewWorkloads(ts.model.Hosts(), networks, 4)
			Expect(workloads.Error()).ShouldNot(HaveOccurred())

			remotePairs := workloads.RemotePairsWithinNetwork()
			log.Infof("Found %d remote pairs", len(remotePairs.Pairs))
			localPairs := workloads.LocalPairsWithinNetwork()
			log.Infof("Found %d local pairs", len(localPairs.Pairs))

			wlp := objects.NewWorkloadPairCollection(workloads.Client, workloads.Testbed)
			for _, p := range remotePairs.Pairs {
				wlp.Pairs = append(wlp.Pairs, p)
			}
			for _, p := range localPairs.Pairs {
				wlp.Pairs = append(wlp.Pairs, p)
			}

			spc := ts.model.NewNetworkSecurityPolicy("test-policy").AddRulesForWorkloadPairs(wlp, "tcp/8000", "PERMIT")
			vmotionSubnet := common.VmotionSubnet + ".0/24"
			spc.AddRule(vmotionSubnet, vmotionSubnet, "any", "PERMIT")
			Expect(spc.Commit()).Should(Succeed())

			// verify policy was propagated correctly
			Eventually(func() error {
				return ts.model.VerifyPolicyStatus(spc)
			}).Should(Succeed())

			wlp2 := objects.NewWorkloadPairCollection(workloads.Client, workloads.Testbed)
			lp := localPairs.Pairs[0]
			wlp2.Pairs = append(wlp2.Pairs, lp)

			for _, p := range remotePairs.Pairs {
				if lp.First == p.First || lp.First == p.Second ||
					lp.Second == p.First || lp.Second == p.Second {
					// found a remote pair using one of the workloads of local-pair
					wlp2.Pairs = append(wlp2.Pairs, p)
					break
				}
			}
			Expect(len(wlp2.Pairs) == 2)

			moveWl := lp.First
			log.Infof("Move WL %+v", moveWl)
			rp := wlp2.Pairs[1]
			var dstHost, srcHost *objects.Host
			if rp.First.NodeName() != moveWl.NodeName() {
				dstHost = rp.First.Host()
			} else {
				dstHost = rp.Second.Host()
			}
			srcHost = moveWl.Host()

			log.Infof("Start Traffic between a local and remote pair")
			mvWlc := objects.NewWorkloadCollection(workloads.Client, workloads.Testbed)
			mvWlc.Workloads = append(mvWlc.Workloads, moveWl)
			dstHosts := objects.NewHostCollection(workloads.Client, workloads.Testbed)
			dstHosts.Hosts = append(dstHosts.Hosts, dstHost)

			tErr := make(chan error)
			go runTraffic(wlp2, tErr)
			// wait for sessions to estabblish before starting migration
			time.Sleep(10 * time.Second)

			log.Infof("Migrate workload %s -> %s", moveWl.Name(), dstHost.Name())
			err = ts.model.MoveWorkloads(mvWlc, dstHosts)
			Expect(err).ShouldNot(HaveOccurred())
			err = <-tErr
			Expect(err).ShouldNot(HaveOccurred())

			Eventually(func() error {
				return ts.model.VerifyWorkloadMigrationStatus(mvWlc)
			}).Should(Succeed())
			time.Sleep(30 * time.Second)

			// log.Infof("Migrate workload (AGAIN) %s", moveWl.Name())
			// Repeat migration - self host - remove it later
			// err = ts.model.MoveWorkloads(mvWlc, dstHosts)
			// Expect(err == nil)
			// Eventually(func() error {
			// 		return ts.model.VerifyWorkloadMigrationStatus(mvWlc)
			// 	}).Should(Succeed())
			log.Infof("Start Traffic between all local and remote pair")
			// check traffic between all local and remote workloads
			go runTraffic(wlp, tErr)
			time.Sleep(5 * time.Second)

			// move the workload back.. simultenously Find a diffrent WL on srchost and move to other host
			dstHosts = objects.NewHostCollection(workloads.Client, workloads.Testbed)
			dstHosts.Hosts = append(dstHosts.Hosts, srcHost)
			for _, rp := range remotePairs.Pairs {
				if rp.First.Name() != moveWl.Name() && rp.First.NodeName() == srcHost.Name() {
					mvWlc.Workloads = append(mvWlc.Workloads, rp.First)
					dstHosts.Hosts = append(dstHosts.Hosts, rp.Second.Host())
					log.Infof("Migrate workloads %s to %s", rp.First.Name(), rp.Second.Host().Name())
				} else if rp.Second.Name() != moveWl.Name() && rp.Second.NodeName() == srcHost.Name() {
					mvWlc.Workloads = append(mvWlc.Workloads, rp.Second)
					dstHosts.Hosts = append(dstHosts.Hosts, rp.First.Host())
					log.Infof("Migrate workloads %s to %s", rp.Second.Name(), rp.First.Host().Name())
				} else {
					continue
				}
				break
			}
			Expect(len(mvWlc.Workloads) == len(dstHosts.Hosts))
			for i, wl := range mvWlc.Workloads {
				log.Infof("Migrate workloads %s from %s to %s", wl.Name(), wl.NodeName(), dstHosts.Hosts[i].Name())
			}

			err = ts.model.MoveWorkloads(mvWlc, dstHosts)
			Expect(err).ShouldNot(HaveOccurred())

			err = <-tErr
			Expect(err).ShouldNot(HaveOccurred())

			// verify workload status is good, Put all check w.r.t to venice here.
			Eventually(func() error {
				return ts.model.VerifyWorkloadStatus(workloads)
			}).Should(Succeed())

			Eventually(func() error {
				return ts.model.VerifyWorkloadMigrationStatus(mvWlc)
			}).Should(Succeed())
		})
	})
})
