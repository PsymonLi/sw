package rollout_test

import (
	"errors"
	"fmt"
	"os"
	"time"

	"github.com/pensando/sw/iota/test/venice/iotakit/model/common"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/objects"
	"github.com/pensando/sw/metrics/iris/genfields"
	"github.com/pensando/sw/metrics/types"
	cq "github.com/pensando/sw/venice/citadel/broker/continuous_query"
	cmdtypes "github.com/pensando/sw/venice/cmd/types"
	"github.com/pensando/sw/venice/utils/log"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)

var timeDiffTolerance = 1 * time.Minute
var firstTimestampMapBeforeReloading = map[string]string{}
var firstTimestampMapAfterReloading = map[string]string{}
var rolloutTestPerformed = false

var _ = Describe("rollout tests", func() {
	BeforeEach(func() {
		// verify cluster is in good health
		Eventually(func() error {
			return ts.model.VerifyClusterStatus()
		}).Should(Succeed())

		ts.model.ForEachNaples(func(nc *objects.NaplesCollection) error {
			_, err := ts.model.RunNaplesCommand(nc, "touch /data/upgrade_to_same_firmware_allowed")
			Expect(err).ShouldNot(HaveOccurred())
			return nil
		})
	})
	AfterEach(func() {
		ts.model.ForEachNaples(func(nc *objects.NaplesCollection) error {
			ts.model.RunNaplesCommand(nc, "rm /data/upgrade_to_same_firmware_allowed")
			return nil
		})
	})

	Context("Iota Rollout tests", func() {

		It("Check metrics fields before rollout", checkMetricsFields)

		It("Check CQ metrics fields before rollout", checkCQMetricsFields)

		It("Check cluster metrics before rollout", checkClusterMetrics)

		It("Perform Rollout", func() {
			rolloutTestPerformed = true
			var workloadPairs *objects.WorkloadPairCollection
			targetBranch := "master"
			if branch := os.Getenv("TARGET_BRANCH"); branch != "" {
				targetBranch = branch
			}
			rollout, err := ts.model.GetRolloutObject(common.RolloutSpec{
				BundleType:   "upgrade-bundle",
				TargetBranch: targetBranch}, ts.scaleData)
			Expect(err).ShouldNot(HaveOccurred())

			if *simOnlyFlag == false {
				workloadPairs = ts.model.WorkloadPairs().WithinNetwork().Any(40)
				log.Infof(" Length workloadPairs %v", len(workloadPairs.ListIPAddr()))
				Expect(len(workloadPairs.ListIPAddr()) != 0).Should(BeTrue())
			}

			err = ts.model.PerformRollout(rollout, ts.scaleData, "upgrade-bundle")
			Expect(err).ShouldNot(HaveOccurred())
			rerr := make(chan bool)
			if *simOnlyFlag == false {
				go func() {
					options := &objects.ConnectionOptions{
						Duration:          "180s",
						Port:              "8000",
						Proto:             "tcp",
						ReconnectAttempts: 100,
					}
					_ = ts.model.ConnectionWithOptions(workloadPairs, options)
					log.Infof("TCP SESSION TEST COMPLETE")
					rerr <- true
					return
				}()
			}

			// verify rollout is successful
			Eventually(func() error {
				return ts.model.VerifyRolloutStatus(rollout.Name)
			}).Should(Succeed())
			log.Infof("Rollout Completed. Waiting for Fuz tests to complete..")

			if *simOnlyFlag == false {
				errWaitingForFuz := func() error {
					select {
					case <-rerr:
						log.Infof("Verified DataPlane using fuz connections")
						return nil
					case <-time.After(time.Duration(900) * time.Second):
						log.Infof("Timeout while waiting for fuz api to return")
						return errors.New("Timeout while waiting for fuz api to return")
					}
				}
				Expect(errWaitingForFuz()).ShouldNot(HaveOccurred())
			}

			return
		})

		It("Check metrics fields after rollout", checkMetricsFields)

		It("Check CQ metrics fields after rollout", checkCQMetricsFields)

		It("Check cluster metrics after rollout", checkClusterMetrics)

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
	if !ts.tb.HasNaplesHW() {
		Skip("No naples hw detected, skip checking metrics")
	}

	// get node collection and init telemetry client
	vnc := ts.model.VeniceNodes()
	err := vnc.InitTelemetryClient()
	Expect(err).Should(BeNil())

	for _, k := range types.DscMetricsList {
		if k == "IPv4FlowDropMetrics" || k == "RuleMetrics" || k == "MirrorMetrics" {
			continue
		}

		Eventually(func() error {
			return ts.model.ForEachNaples(func(n *objects.NaplesCollection) error {
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
						if !rolloutTestPerformed {
							firstTimestampMapBeforeReloading[k] = timeString
						} else {
							firstTimestampMapAfterReloading[k] = timeString
						}
					}
				}
				return nil
			})
		}).Should(Succeed())
	}
}

func checkClusterMetrics() {
	if !ts.tb.HasNaplesHW() {
		Skip("No naples hw detected, skip checking metrics")
	}

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
		resp, err := vnc.QueryMetricsFields(kind, "")
		if err != nil {
			fmt.Printf("query failed %v \n", err)
			return err
		}

		// make sure cluster metrics exists
		err = vnc.ValidateMetricsQueryResponse(resp, fields, "")
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
			resp, err := vnc.QueryMetricsFields(cq, "")
			if err != nil {
				fmt.Printf("query failed %v \n", err)
				return err
			}

			// make sure cluster metrics exists
			err = vnc.ValidateMetricsQueryResponse(resp, fields, "")
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
			if m == "IPv4FlowDropMetrics" || m == "RuleMetrics" || m == "MirrorMetrics" {
				continue
			}
			cq := m + "_" + s
			Eventually(func() error {
				return ts.model.ForEachNaples(func(n *objects.NaplesCollection) error {
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
							if !rolloutTestPerformed {
								firstTimestampMapBeforeReloading[cq] = timeString
							} else {
								firstTimestampMapAfterReloading[cq] = timeString
							}
						}
					}
					return nil
				})
			}, time.Duration(10)*time.Minute, time.Duration(30)*time.Second).Should(Succeed())
		}
	}
}
