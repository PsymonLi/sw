package monitor_test

import (
	"encoding/json"
	"fmt"
	"time"

	"github.com/pensando/sw/api/fields"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/objects"
	"github.com/pensando/sw/metrics/iris/genfields"
	"github.com/pensando/sw/metrics/types"
	cq "github.com/pensando/sw/venice/citadel/broker/continuous_query"
	cmdtypes "github.com/pensando/sw/venice/cmd/types"
	"github.com/pensando/sw/venice/utils/telemetryclient"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"

	"github.com/pensando/sw/venice/utils/log"
)

var _ = Describe("metrics test", func() {
	var startTime time.Time
	BeforeEach(func() {
		// verify cluster is in good health
		Eventually(func() error {
			return ts.model.VerifyClusterStatus()
		}).Should(Succeed())
		startTime = time.Now().UTC()
	})
	AfterEach(func() {
		Expect(ts.model.ServiceStoppedEvents(startTime, ts.model.Naples()).Len(0))
	})

	Context("Verify flowdropmetrics ", func() {
		It("Flow drops should increament", func() {
			Skip("IPv4FlowDropMetrics is disabled")
			if !ts.tb.HasNaplesHW() {
				Skip("Disabling flow drop stats on naples sim")
			}
			// spc := ts.model.NewNetworkSecurityPolicy("test-policy").AddRule("any", "any", "any", "PERMIT")
			// Expect(spc.Commit()).ShouldNot(HaveOccurred())

			//Verif policy was propagted correctly

			startTime := time.Now().UTC().Format(time.RFC3339)

			// get node collection and init telemetry client
			vnc := ts.model.VeniceNodes()
			err := vnc.InitTelemetryClient()
			Expect(err).Should(BeNil())

			// establish TCP session between workload pairs in same subnet
			workloadPairs := ts.model.WorkloadPairs().WithinNetwork().Any(1)
			log.Infof("wait to get to the console")
			time.Sleep(time.Second * 30)
			Eventually(func() error {
				return ts.model.DropIcmpFlowTTLSession(workloadPairs, "--icmp --icmptype 4 --count 1")
			}).Should(Succeed())
			time.Sleep(time.Second * 30)

			// check fwlog, enable when fwlogs are reported to Venice
			// Eventually(func() error {
			// 	return ts.model.FindFwlogForWorkloadPairsFromObjStore("default", "ICMP", 0, "allow", workloadPairs)
			// }).Should(Succeed())

			Eventually(func() error {
				return vnc.QueryDropMetricsForWorkloadPairs(workloadPairs, startTime)
			}).Should(Succeed())

		})
	})

	Context("Verify basic metrics ", func() {
		It("Check metrics fields", checkMetricsFields)

		It("Check CQ metrics fields", checkCQMetricsFields)

		It("Check cluster metrics", checkClusterMetrics)

		It("Reloading venice nodes", func() {
			// get node collection and init telemetry client
			vnc := ts.model.VeniceNodes()
			err := vnc.InitTelemetryClient()
			Expect(err).Should(BeNil())

			// reload each host
			ts.model.ForEachVeniceNode(func(vnc *objects.VeniceNodeCollection) error {
				Expect(ts.model.ReloadVeniceNodes(vnc)).Should(Succeed())

				// wait for cluster to be back in good state
				Eventually(func() error {
					return ts.model.VerifyClusterStatus()
				}).Should(Succeed())

				return nil
			})
		})

		It("Check metrics fields after reloading nodes", checkMetricsFields)

		It("Check CQ metrics fields after reloading nodes", checkCQMetricsFields)

		It("Check cluster metrics after reloading nodes", checkClusterMetrics)
	})
})

func checkMetricsFields() {
	if !ts.tb.HasNaplesHW() {
		Skip("No naples hw detected, skip checking metrics")
	}
	tms := time.Now().UTC().Add(time.Second * -60).Format(time.RFC3339)

	// get node collection and init telemetry client
	vnc := ts.model.VeniceNodes()
	err := vnc.InitTelemetryClient()
	Expect(err).Should(BeNil())

	for _, k := range types.DscMetricsList {
		if k == "IPv4FlowDropMetrics" {
			continue
		}

		Eventually(func() error {
			return ts.model.ForEachNaples(func(n *objects.NaplesCollection) error {
				flds := genfields.GetFieldNamesFromKind(k)
				for _, name := range n.Names() {
					By(fmt.Sprintf("checking %v in naples %v", k, name))

					resp, err := vnc.QueryMetricsByReporter(k, name, tms)
					if err != nil {
						fmt.Printf("query failed %v \n", err)
						return err
					}

					if err := validateResp(resp, flds, tms); err != nil {
						return err
					}

					if err := checkIntfMetrics(vnc, k, name, tms); err != nil {
						return err
					}

				}
				return nil
			})
		}, time.Duration(10)*time.Minute, time.Duration(30)*time.Second).Should(Succeed())
	}
}

func checkIntfMetrics(vnc *objects.VeniceNodeCollection, kind string, reporterID string, tms string) error {
	// check interface stats
	ifNames := map[string][]string{
		"LifMetrics":     {"pf-69", "pf-70"},
		"MacMetrics":     {"uplink-1-1", "uplink-1-2"},
		"MgmtMacMetrics": {"mgmt-1-3"},
	}

	flds := genfields.GetFieldNamesFromKind(kind)

	if ns, ok := ifNames[kind]; ok {
		for _, n := range ns {
			fmt.Printf("checking [%v] in %v with name: %v \n", kind, reporterID, n)
			sel := fields.Selector{
				Requirements: []*fields.Requirement{
					{
						Key:    "name",
						Values: []string{reporterID + "-" + n},
					},
					{
						Key:    "reporterID",
						Values: []string{reporterID},
					},
				},
			}

			resp, err := vnc.QueryMetricsSelector(kind, tms, sel)
			if err != nil {
				fmt.Printf("query failed %v \n", err)
				return err
			}

			err = validateResp(resp, flds, tms)
			if err != nil {
				return err
			}
		}
	}

	return nil
}

func checkClusterMetrics() {
	if !ts.tb.HasNaplesHW() {
		Skip("No naples hw detected, skip checking metrics")
	}
	tms := time.Now().UTC().Add(time.Second * -60).Format(time.RFC3339)
	cqtms := time.Now().UTC().Add(time.Minute * -10).Format(time.RFC3339)

	// get node collection and init telemetry client
	vnc := ts.model.VeniceNodes()
	err := vnc.InitTelemetryClient()
	Expect(err).Should(BeNil())

	kind := "Cluster"
	clusterZeroMap := cmdtypes.GetSmartNICMetricsZeroMap()
	fields := []string{}
	for f := range clusterZeroMap {
		fields = append(fields, f)
	}
	Eventually(func() error {
		By(fmt.Sprintf("checking %v\n", kind))

		// query cluster metrics
		resp, err := vnc.QueryMetricsFields(kind, tms)
		if err != nil {
			fmt.Printf("query failed %v \n", err)
			return err
		}

		// make sure cluster metrics exists
		err = validateResp(resp, fields, tms)
		if err != nil {
			return err
		}

		// query cluster CQ metrics
		for s := range cq.RetentionPolicyMap {
			if s != "5minutes" {
				continue
			}
			cq := kind + "_" + s
			By(fmt.Sprintf("checking %v\n", cq))
			resp, err := vnc.QueryMetricsFields(cq, cqtms)
			if err != nil {
				fmt.Printf("query failed %v \n", err)
				return err
			}

			// make sure cluster metrics exists
			err = validateResp(resp, fields, cqtms)
			if err != nil {
				return err
			}
		}

		return nil
	}, time.Duration(10)*time.Minute, time.Duration(30)*time.Second).Should(Succeed())
}

func checkCQMetricsFields() {
	if !ts.tb.HasNaplesHW() {
		Skip("No naples hw detected, skip checking metrics")
	}
	tms := time.Now().Add(time.Minute * -10).Format(time.RFC3339)

	// get node collection and init telemetry client
	vnc := ts.model.VeniceNodes()
	err := vnc.InitTelemetryClient()
	Expect(err).Should(BeNil())

	for s := range cq.RetentionPolicyMap {
		if s != "5minutes" {
			continue
		}
		for _, m := range types.DscMetricsList {
			// drop metrics are reported only on drop
			if m == "IPv4FlowDropMetrics" || m == "RuleMetrics" {
				continue
			}
			cq := m + "_" + s
			Eventually(func() error {
				return ts.model.ForEachNaples(func(n *objects.NaplesCollection) error {
					fields := genfields.GetFieldNamesFromKind(m)
					for _, name := range n.Names() {
						By(fmt.Sprintf("checking %v in naples %v", cq, name))

						resp, err := vnc.QueryMetricsByReporter(cq, name, tms)
						if err != nil {
							fmt.Printf("query failed %v \n", err)
							return err
						}
						err = validateResp(resp, fields, tms)
						if err != nil {
							return err
						}

						if err := checkIntfMetrics(vnc, cq, name, tms); err != nil {
							return err
						}
					}
					return nil
				})
			}, time.Duration(10)*time.Minute, time.Duration(30)*time.Second).Should(Succeed())
		}
	}
}

func validateResp(resp *telemetryclient.MetricsQueryResponse, fields []string, tms string) error {
	if len(resp.Results) == 0 || len(resp.Results[0].Series) == 0 {
		res, err := json.Marshal(resp)
		fmt.Printf("query ts %v returned(%v) %+v \n", tms, err, string(res))
		return fmt.Errorf("no results")
	}

	for _, r := range resp.Results[0].Series {

		// get index
		cIndex := map[string]int{}
		for i, c := range r.Columns {
			cIndex[c] = i
		}

		for _, f := range fields {
			if _, ok := cIndex[f]; !ok {
				fmt.Printf("failed to find %v \n", f)
				return fmt.Errorf("failed to find %v", f)
			}
			fmt.Printf("\tcheck %v \u2714 \n", f)
		}
	}

	return nil
}
