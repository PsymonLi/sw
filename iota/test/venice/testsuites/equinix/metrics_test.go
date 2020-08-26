package equinix_test

import (
	"fmt"
	"time"

	"github.com/pensando/sw/iota/test/venice/iotakit/model/objects"
	"github.com/pensando/sw/metrics/apulu/genfields"
	"github.com/pensando/sw/metrics/types"
	cq "github.com/pensando/sw/venice/citadel/broker/continuous_query"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)

var timeDiffTolerance = 1 * time.Minute
var firstTimestampMapBeforeReloading = map[string]string{}
var firstTimestampMapAfterReloading = map[string]string{}
var hasRunNodeReloadTest = false

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

	Context("Verify basic metrics ", func() {
		It("Check metrics fields", checkMetricsFields)

		It("Check CQ metrics fields", checkCQMetricsFields)

		It("Reloading venice nodes", func() {
			hasRunNodeReloadTest = true

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

		It("Check data loss for all metrics", func() {
			vnc := ts.model.VeniceNodes()
			Eventually(func() error {
				for m, timeStampAfterReloading := range firstTimestampMapAfterReloading {
					if timeStampBeforeReloading, ok := firstTimestampMapBeforeReloading[m]; !ok {
						By(fmt.Sprintf("Cannot find saved timestamp for %v before reloading nodes", m))
						return fmt.Errorf("Cannot find saved timestamp for %v before reloading nodes", m)
					} else {
						diffDuration, err := vnc.CalcTimeDiffDuration(timeStampBeforeReloading, timeStampAfterReloading)
						if err != nil {
							By(fmt.Sprintf("Error parse time string %v and %v. Err: %v", timeStampBeforeReloading, timeStampAfterReloading, err))
							return fmt.Errorf("Error parse time string %v and %v. Err: %v", timeStampBeforeReloading, timeStampAfterReloading, err)
						}
						if diffDuration > timeDiffTolerance {
							By(fmt.Sprintf("Data loss detected for %v based on first timestamp. Before: %v, After: %v", m, timeStampBeforeReloading, timeStampAfterReloading))
							return fmt.Errorf("Data loss detected for %v based on first timestamp. Before: %v, After: %v", m, timeStampBeforeReloading, timeStampAfterReloading)
						}
					}
				}
				return nil
			}).Should(Succeed())
		})
	})
})

func checkMetricsFields() {
	// get node collection and init telemetry client
	vnc := ts.model.VeniceNodes()
	err := vnc.InitTelemetryClient()
	Expect(err).Should(BeNil())

	for _, k := range types.CloudDscMetricsList {
		if k == "IPv4FlowDropMetrics" {
			continue
		}

		Eventually(func() error {
			naplesMetricsErr := ts.model.ForEachNaples(func(n *objects.NaplesCollection) error {
				fields := genfields.GetFieldNamesFromKind(k)
				for _, name := range n.Names() {
					By(fmt.Sprintf("checking %v in naples %v", k, name))

					resp, err := vnc.QueryMetricsByReporter(k, name, "")
					if err != nil {
						fmt.Printf("query failed %v \n", err)
						return err
					}

					err = vnc.ValidateMetricsQueryResponse(resp, fields, "")
					if err != nil {
						return err
					}

					if timeString, err := vnc.GetMetricsQueryResponseFirstTimeString(resp); err != nil {
						if len(resp.Results) > 0 && len(resp.Results[0].Series) > 0 {
							By(fmt.Sprintf("Error extracting timestamp. Resp: %+v, Err: %+v", resp.Results[0].Series[0], err))
						}
						return err
					} else if timeString != "" {
						if !hasRunNodeReloadTest {
							firstTimestampMapBeforeReloading[k] = timeString
						} else {
							firstTimestampMapAfterReloading[k] = timeString
						}
					}
				}
				return nil
			})
			if naplesMetricsErr != nil {
				return naplesMetricsErr
			}

			fakeNaplesMetricsErr := ts.model.ForEachFakeNaples(func(n *objects.NaplesCollection) error {
				fields := genfields.GetFieldNamesFromKind(k)
				for _, name := range n.Names() {
					By(fmt.Sprintf("checking %v in naples %v", k, name))

					resp, err := vnc.QueryMetricsByReporter(k, name, "")
					if err != nil {
						fmt.Printf("query failed %v \n", err)
						return err
					}

					err = vnc.ValidateMetricsQueryResponse(resp, fields, "")
					if err != nil {
						return err
					}

					if timeString, err := vnc.GetMetricsQueryResponseFirstTimeString(resp); err != nil {
						if len(resp.Results) > 0 && len(resp.Results[0].Series) > 0 {
							By(fmt.Sprintf("Error extracting timestamp. Resp: %+v, Err: %+v", resp.Results[0].Series[0], err))
						}
						return err
					} else if timeString != "" {
						if !hasRunNodeReloadTest {
							firstTimestampMapBeforeReloading[k] = timeString
						} else {
							firstTimestampMapAfterReloading[k] = timeString
						}
					}
				}
				return nil
			})
			if fakeNaplesMetricsErr != nil {
				return fakeNaplesMetricsErr
			}

			return nil
		}, time.Duration(15)*time.Minute, time.Duration(30)*time.Second).Should(Succeed())
	}
}

func checkCQMetricsFields() {
	// get node collection and init telemetry client
	vnc := ts.model.VeniceNodes()
	err := vnc.InitTelemetryClient()
	Expect(err).Should(BeNil())

	for s := range cq.RetentionPolicyMap {
		if s != "5minutes" {
			continue
		}
		for _, m := range types.CloudDscMetricsList {
			// drop metrics are reported only on drop
			if m == "IPv4FlowDropMetrics" || m == "DropMetrics" {
				continue
			}
			cq := m + "_" + s
			Eventually(func() error {
				naplesMetricsErr := ts.model.ForEachNaples(func(n *objects.NaplesCollection) error {
					fields := genfields.GetFieldNamesFromKind(m)
					for _, name := range n.Names() {
						By(fmt.Sprintf("checking %v in naples %v", cq, name))

						resp, err := vnc.QueryMetricsByReporter(cq, name, "")
						if err != nil {
							fmt.Printf("query failed %v \n", err)
							return err
						}

						err = vnc.ValidateMetricsQueryResponse(resp, fields, "")
						if err != nil {
							return err
						}

						if timeString, err := vnc.GetMetricsQueryResponseFirstTimeString(resp); err != nil {
							if len(resp.Results) > 0 && len(resp.Results[0].Series) > 0 {
								By(fmt.Sprintf("Error extracting timestamp. Resp: %+v, Err: %+v", resp.Results[0].Series[0], err))
							}
							return err
						} else if timeString != "" {
							if !hasRunNodeReloadTest {
								firstTimestampMapBeforeReloading[cq] = timeString
							} else {
								firstTimestampMapAfterReloading[cq] = timeString
							}
						}
					}
					return nil
				})
				if naplesMetricsErr != nil {
					return naplesMetricsErr
				}

				fakeNaplesMetricsErr := ts.model.ForEachFakeNaples(func(n *objects.NaplesCollection) error {
					fields := genfields.GetFieldNamesFromKind(m)
					for _, name := range n.Names() {
						By(fmt.Sprintf("checking %v in naples %v", cq, name))

						resp, err := vnc.QueryMetricsByReporter(cq, name, "")
						if err != nil {
							fmt.Printf("query failed %v \n", err)
							return err
						}

						err = vnc.ValidateMetricsQueryResponse(resp, fields, "")
						if err != nil {
							return err
						}

						if timeString, err := vnc.GetMetricsQueryResponseFirstTimeString(resp); err != nil {
							if len(resp.Results) > 0 && len(resp.Results[0].Series) > 0 {
								By(fmt.Sprintf("Error extracting timestamp. Resp: %+v, Err: %+v", resp.Results[0].Series[0], err))
							}
							return err
						} else if timeString != "" {
							if !hasRunNodeReloadTest {
								firstTimestampMapBeforeReloading[cq] = timeString
							} else {
								firstTimestampMapAfterReloading[cq] = timeString
							}
						}
					}
					return nil
				})
				if fakeNaplesMetricsErr != nil {
					return fakeNaplesMetricsErr
				}

				return nil
			}, time.Duration(15)*time.Minute, time.Duration(30)*time.Second).Should(Succeed())
		}
	}
}
