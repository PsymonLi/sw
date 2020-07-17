// {C} Copyright 2019 Pensando Systems Inc. All rights reserved.

package vchub_test

import (
	"fmt"
	"time"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/common"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/objects"
	"github.com/pensando/sw/venice/utils/log"
)

var _ = Describe("Vc hub workload tests", func() {
	startTime := time.Now().UTC()

	captureNaplesMem := func() {
		ts.model.ForEachNaples(func(nc *objects.NaplesCollection) error {
			_, err := ts.model.RunNaplesCommand(nc,
				"cd /data ; ./ps_mem.py | grep hal >> ps_out.txt && halctl show system memory summary >> ps_out.txt && halctl show system memory mtrack >> mtrack_out.txt && halctl show system memory slab >> slab_out.txt; cd /")
			Expect(err).ShouldNot(HaveOccurred())
			return nil
		})
	}

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
		captureNaplesMem()
	})

	var defaultDuration = "180s"
	var longDuration = "360s"

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

		runTraffic := func(workloadPairs *objects.WorkloadPairCollection, duration string, rerr chan error) {
			options := &objects.ConnectionOptions{
				Duration:          duration,
				Port:              "8000",
				Proto:             "tcp",
				ReconnectAttempts: 100,
				NumConns:          10,
			}
			err := ts.model.ConnectionWithOptions(workloadPairs, options)
			log.Infof("TCP SESSION TEST COMPLETE")
			rerr <- err
			return
		}

		It("Vmotion basic test", func() {
			workloads := ts.model.Workloads()
			err := ts.model.TeardownWorkloads(workloads)
			Expect(err == nil)
			numWorkloads := 1
			if ts.scaleData {
				numWorkloads = 4
			}

			// Skip("Skipping move tests..")
			workloads = ts.model.BringUpNewWorkloads(ts.model.Hosts(), ts.model.Networks("").Any(1), numWorkloads)
			Expect(workloads.Error()).ShouldNot(HaveOccurred())

			// verify workload status is good, Put all check w.r.t to venice here.
			Eventually(func() error {
				return ts.model.VerifyWorkloadStatus(workloads)
			}).Should(Succeed())

			spc := ts.model.NewNetworkSecurityPolicy("test-policy").AddRulesForWorkloadPairs(workloads.MeshPairs(), "tcp/8000", "PERMIT")
			vmotionSubnet := common.VmotionSubnet + ".0/24"
			spc.AddRule(vmotionSubnet, vmotionSubnet, "any", "PERMIT")
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

			var srcHostCollections []*objects.HostWorkloadCollection
			var dscHostCollections []*objects.HostWorkloadCollection
			var wPairsCollections []*objects.WorkloadPairCollection
			for i := 0; i < len(hostWorkloads); i += 2 {
				srcHostCollections = append(srcHostCollections, hostWorkloads[i])
				dscHostCollections = append(dscHostCollections, hostWorkloads[i+1])
				srcWorkloads := hostWorkloads[i].Workloads()
				dstWorkloads := hostWorkloads[i+1].Workloads().Any(numWorkloads)
				wPairsCollections = append(wPairsCollections, srcWorkloads.MeshPairsWithOther(dstWorkloads))
			}
			log.Infof("Found sufficient worklaods")

			terr := make(chan error, len(wPairsCollections))
			for _, wPairs := range wPairsCollections {
				wPairs := wPairs
				go runTraffic(wPairs, defaultDuration, terr)
			}

			moverErr := make(chan error, len(hostWorkloads)/2)
			moveWorkloads := func(src *objects.HostWorkloadCollection, dst *objects.HostWorkloadCollection) {
				srcWorkloads := src.Workloads()
				dstWorkloads := dst.Workloads()
				log.Infof("Moving %v -> %v", srcWorkloads.String(), dst.Host().Hosts[0].Name())

				err = ts.model.MoveWorkloads(common.MoveWorkloadsSpec{
					DstHostCollection:  dst.Host(),
					WorkloadCollection: srcWorkloads,
				})
				Expect(err == nil)
				log.Infof("Moving %v -> %v", dstWorkloads.String(), src.Host().Hosts[0].Name())
				err = ts.model.MoveWorkloads(common.MoveWorkloadsSpec{
					DstHostCollection:  src.Host(),
					WorkloadCollection: dstWorkloads,
				})
				Expect(err == nil)
				Eventually(func() error {
					return ts.model.VerifyWorkloadMigrationStatus(srcWorkloads)
				}).Should(Succeed())

				Eventually(func() error {
					return ts.model.VerifyWorkloadMigrationStatus(dstWorkloads)
				}).Should(Succeed())

				moverErr <- nil

			}

			for i := 0; i < len(hostWorkloads)/2; i++ {
				go moveWorkloads(srcHostCollections[i], dscHostCollections[i])
			}

			for i := 0; i < len(hostWorkloads)/2; i++ {
				err := <-moverErr
				Expect(err == nil)
			}

			for i := 0; i < len(wPairsCollections); i++ {
				err := <-terr
				Expect(err == nil)
			}

		})

		It("Bring up 8 local and remote workloads, move back and forth while traffic still progressing", func() {

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

			//Mesh pairs to generate traffic from all
			wpairs := workloads.MeshPairs()

			spc := ts.model.NewNetworkSecurityPolicy("test-policy").AddRulesForWorkloadPairs(wpairs, "tcp/8000", "PERMIT")
			vmotionSubnet := common.VmotionSubnet + ".0/24"
			spc.AddRule(vmotionSubnet, vmotionSubnet, "any", "PERMIT")
			Expect(spc.Commit()).Should(Succeed())

			// verify policy was propagated correctly
			Eventually(func() error {
				return ts.model.VerifyPolicyStatus(spc)
			}).Should(Succeed())

			//Map For which host worklaods map to which other
			hostMap := make(map[string]*objects.Host)
			hostWorkloadCollection := make(map[string]*objects.WorkloadCollection)

			hosts := ts.model.Hosts().Hosts
			for index, host := range hosts {
				hostMap[host.Name()] = hosts[len(hosts)-index-1]
				hostWorkloadCollection[host.Name()] = objects.NewWorkloadCollection(workloads.Client, workloads.Testbed)
			}

			for _, wl := range workloads.Workloads {
				wc := hostWorkloadCollection[wl.Host().Name()]
				wc.Workloads = append(wc.Workloads, wl)
			}

			terr := make(chan error)
			go runTraffic(wpairs, longDuration, terr)

			// wait for sessions to estabblish before starting migration
			time.Sleep(10 * time.Second)

			log.Infof("Starting forward move for workloads.")
			moverErr := make(chan error, len(hostWorkloadCollection))
			moveWorkloads := func(wc *objects.WorkloadCollection, dst *objects.HostCollection) {
				log.Infof("Moving %v -> %v from %v", wc.String(), dst.Hosts[0].Name(), wc.Workloads[0].Host().Name())
				err = ts.model.MoveWorkloads(common.MoveWorkloadsSpec{
					DstHostCollection:  dst,
					WorkloadCollection: wc,
				})

				Expect(err == nil)
				Eventually(func() error {
					return ts.model.VerifyWorkloadMigrationStatus(wc)
				}).Should(Succeed())
				moverErr <- nil
			}

			for host, wc := range hostWorkloadCollection {
				dstHost := hostMap[host]
				dstHc := objects.NewHostCollection(workloads.Client, workloads.Testbed)
				dstHc.Hosts = append(dstHc.Hosts, dstHost)
				wc := wc
				go moveWorkloads(wc, dstHc)
			}

			for i := 0; i < len(hostWorkloadCollection); i++ {
				err := <-moverErr
				Expect(err == nil)
			}

			log.Infof("Starting backward move for workloads.")
			//Now move back the workloads
			for _, host := range hosts {
				hostWorkloadCollection[host.Name()] = objects.NewWorkloadCollection(workloads.Client, workloads.Testbed)
			}

			for _, wl := range workloads.Workloads {
				wc := hostWorkloadCollection[wl.Host().Name()]
				wc.Workloads = append(wc.Workloads, wl)
			}

			moverErr = make(chan error, len(hostWorkloadCollection))
			for host, wc := range hostWorkloadCollection {
				dstHost := hostMap[host]
				dstHc := objects.NewHostCollection(workloads.Client, workloads.Testbed)
				dstHc.Hosts = append(dstHc.Hosts, dstHost)
				wc := wc
				go moveWorkloads(wc, dstHc)
			}

			for i := 0; i < len(hostWorkloadCollection); i++ {
				err := <-moverErr
				Expect(err == nil)
			}

			Expect(terr == nil)

		})

		It("Abort and restart vmotion of workloads", func() {
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
			workloads = ts.model.BringUpNewWorkloads(ts.model.Hosts(), networks, 8)
			Expect(workloads.Error()).ShouldNot(HaveOccurred())

			//Mesh pairs to generate traffic from all
			wpairs := workloads.MeshPairs()

			spc := ts.model.NewNetworkSecurityPolicy("test-policy").AddRulesForWorkloadPairs(wpairs, "tcp/8000", "PERMIT")
			vmotionSubnet := common.VmotionSubnet + ".0/24"
			spc = spc.AddRule(vmotionSubnet, vmotionSubnet, "any", "PERMIT")
			Expect(spc.Commit()).Should(Succeed())
			Eventually(func() error {
				return ts.model.VerifyPolicyStatus(spc)
			}).Should(Succeed())

			// verify policy was propagated correctly

			//Map For which host worklaods map to which other
			hostMap := make(map[string]*objects.Host)
			hostWorkloadCollection := make(map[string]*objects.WorkloadCollection)

			hosts := ts.model.Hosts().Hosts
			for index, host := range hosts {
				hostMap[host.Name()] = hosts[len(hosts)-index-1]
				hostWorkloadCollection[host.Name()] = objects.NewWorkloadCollection(workloads.Client, workloads.Testbed)
			}

			for _, wl := range workloads.Workloads {
				wc := hostWorkloadCollection[wl.Host().Name()]
				wc.Workloads = append(wc.Workloads, wl)
			}

			terr := make(chan error)
			go runTraffic(wpairs, longDuration, terr)

			// wait for sessions to estabblish before starting migration
			time.Sleep(10 * time.Second)

			log.Infof("Starting forward move with abort")
			moverErr := make(chan error, len(hostWorkloadCollection))
			moveAbortWorkloads := func(wc *objects.WorkloadCollection, dst *objects.HostCollection) {
				log.Infof("Abort Moving %v -> %v from %v", wc.String(), dst.Hosts[0].Name(), wc.Workloads[0].Host().Name())

				err = ts.model.MoveWorkloads(common.MoveWorkloadsSpec{
					DstHostCollection:     dst,
					WorkloadCollection:    wc,
					Timeout:               30,
					NumberOfParallelMoves: 2,
				})

				Expect(err != nil)
				Eventually(func() error {
					return ts.model.VerifyWorkloadMigrationAbortStatus(wc)
				}).Should(Succeed())
				moverErr <- nil
			}

			moveWorkloads := func(wc *objects.WorkloadCollection, dst *objects.HostCollection) {
				log.Infof("Moving %v -> %v from %v", wc.String(), dst.Hosts[0].Name(), wc.Workloads[0].Host().Name())
				err = ts.model.MoveWorkloads(common.MoveWorkloadsSpec{
					DstHostCollection:  dst,
					WorkloadCollection: wc,
				})
				Expect(err != nil)
				Eventually(func() error {
					return ts.model.VerifyWorkloadMigrationStatus(wc)
				}).Should(Succeed())
				moverErr <- nil
			}

			for host, wc := range hostWorkloadCollection {
				dstHost := hostMap[host]
				dstHc := objects.NewHostCollection(workloads.Client, workloads.Testbed)
				dstHc.Hosts = append(dstHc.Hosts, dstHost)
				wc := wc
				go moveAbortWorkloads(wc, dstHc)
			}

			for i := 0; i < len(hostWorkloadCollection); i++ {
				err := <-moverErr
				Expect(err == nil)
			}

			moverErr = make(chan error, len(hostWorkloadCollection))
			for host, wc := range hostWorkloadCollection {
				dstHost := hostMap[host]
				dstHc := objects.NewHostCollection(workloads.Client, workloads.Testbed)
				dstHc.Hosts = append(dstHc.Hosts, dstHost)
				wc := wc
				go moveWorkloads(wc, dstHc)
			}

			for i := 0; i < len(hostWorkloadCollection); i++ {
				err := <-moverErr
				Expect(err == nil)
			}

			log.Infof("Starting abort backward move for workloads.")
			//Now move back the workloads
			for _, host := range hosts {
				hostWorkloadCollection[host.Name()] = objects.NewWorkloadCollection(workloads.Client, workloads.Testbed)
			}

			for _, wl := range workloads.Workloads {
				wc := hostWorkloadCollection[wl.Host().Name()]
				wc.Workloads = append(wc.Workloads, wl)
			}

			moverErr = make(chan error, len(hostWorkloadCollection))
			for host, wc := range hostWorkloadCollection {
				dstHost := hostMap[host]
				dstHc := objects.NewHostCollection(workloads.Client, workloads.Testbed)
				dstHc.Hosts = append(dstHc.Hosts, dstHost)
				wc := wc
				moveAbortWorkloads(wc, dstHc)
			}

			for i := 0; i < len(hostWorkloadCollection); i++ {
				err := <-moverErr
				Expect(err == nil)
			}

			moverErr = make(chan error, len(hostWorkloadCollection))
			log.Infof("Starting backward move for workloads.")
			for host, wc := range hostWorkloadCollection {
				dstHost := hostMap[host]
				dstHc := objects.NewHostCollection(workloads.Client, workloads.Testbed)
				dstHc.Hosts = append(dstHc.Hosts, dstHost)
				wc := wc
				moveWorkloads(wc, dstHc)
			}

			for i := 0; i < len(hostWorkloadCollection); i++ {
				err := <-moverErr
				Expect(err == nil)
			}

			Expect(terr == nil)

		})

		It("Disconnect/reconnect vcenter", func() {

			workloads := ts.model.Workloads()
			err := ts.model.TeardownWorkloads(workloads)
			Expect(err == nil)
			err = ts.model.DisconnectVeniceAndOrchestrator()
			Expect(err == nil)

			// verify workload status is good, Put all check w.r.t to venice here.
			Eventually(func() error {
				orch, _ := ts.model.GetOrchestrator()
				if connected, err := orch.Connected(); err == nil && connected {
					return fmt.Errorf("Orchestration is still connected")
				}
				return nil
			}).Should(Succeed())

			workloads = ts.model.Workloads()
			err = ts.model.TeardownWorkloads(workloads)
			Expect(err == nil)

			workloads = ts.model.BringUpNewWorkloads(ts.model.Hosts(), ts.model.Networks("").Any(1), 1)
			Expect(workloads.Error()).ShouldNot(HaveOccurred())

			spc := ts.model.NewNetworkSecurityPolicy("test-policy").AddRulesForWorkloadPairs(workloads.MeshPairs(), "tcp/8000", "PERMIT")
			Expect(spc.Commit()).Should(Succeed())

			// verify policy was propagated correctly
			Eventually(func() error {
				return ts.model.VerifyPolicyStatus(spc)
			}).Should(Succeed())

			err = ts.model.AllowVeniceAndOrchestrator()
			Expect(err == nil)

			// verify workload status is good, Put all check w.r.t to venice here.
			Eventually(func() error {
				orch, _ := ts.model.GetOrchestrator()
				if connected, err := orch.Connected(); err == nil && connected {
					return nil
				}
				return fmt.Errorf("Orchestration is not connected")
			}).Should(Succeed())

			// verify workload status is good after connected to orchestrator
			Eventually(func() error {
				return ts.model.VerifyWorkloadStatus(workloads)
			}).Should(Succeed())

			// check ping between new workloads
			Eventually(func() error {
				return ts.model.TCPSession(workloads.MeshPairs(), 8000)
			}).ShouldNot(HaveOccurred())

		})

	})
})
