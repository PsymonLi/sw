package cluster

import (
	"context"
	"encoding/json"
	"fmt"
	"reflect"
	"strings"
	"time"

	"github.com/gogo/protobuf/types"
	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/monitoring"
	testutils "github.com/pensando/sw/test/utils"
	"github.com/pensando/sw/venice/ctrler/tsm/statemgr"
	"github.com/pensando/sw/venice/globals"
)

var _ = Describe("mirror session tests", func() {
	Context("MirrorSession CRUD Tests", func() {
		var testMirrorSessions = []monitoring.MirrorSession{
			{
				ObjectMeta: api.ObjectMeta{
					Name:   "TestMirrorSession1",
					Tenant: "default",
				},
				TypeMeta: api.TypeMeta{
					Kind:       "MirrorSession",
					APIVersion: "v1",
				},
				Spec: monitoring.MirrorSessionSpec{
					PacketSize:    128,
					SpanID:        1023,
					PacketFilters: []string{monitoring.MirrorSessionSpec_ALL_PKTS.String()},
					Collectors: []monitoring.MirrorCollector{
						{
							Type: monitoring.PacketCollectorType_ERSPAN_TYPE_3.String(),
							ExportCfg: &monitoring.MirrorExportConfig{
								Destination: "192.168.1.10",
							},
						},
					},
					MatchRules: []monitoring.MatchRule{
						{
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"10.1.1.1"},
							},
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"10.1.1.10"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/5555"},
							},
						},
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"10.2.2.20"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"UDP/5555"},
							},
						},
					},
				},
			},
			{
				ObjectMeta: api.ObjectMeta{
					Name:   "TestMirrorSession999",
					Tenant: "default",
				},
				TypeMeta: api.TypeMeta{
					Kind:       "MirrorSession",
					APIVersion: "v1",
				},
				Spec: monitoring.MirrorSessionSpec{
					PacketSize:    128,
					PacketFilters: []string{monitoring.MirrorSessionSpec_ALL_PKTS.String()},
					Collectors: []monitoring.MirrorCollector{
						{
							Type: monitoring.PacketCollectorType_ERSPAN_TYPE_3.String(),
							ExportCfg: &monitoring.MirrorExportConfig{
								Destination: "192.168.1.10",
							},
						},
					},
					MatchRules: []monitoring.MatchRule{
						{
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"10.1.1.1"},
							},
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"10.1.1.10"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"UDP/5555"},
							},
						},
					},
				},
			},
		}

		var testValidateMirrorSessions = map[*monitoring.MirrorSession]bool{
			{
				ObjectMeta: api.ObjectMeta{
					Name:   "ValidateMirrorSession1",
					Tenant: "default",
				},
				TypeMeta: api.TypeMeta{
					Kind:       "MirrorSession",
					APIVersion: "v1",
				},
				Spec: monitoring.MirrorSessionSpec{
					PacketSize:    128,
					SpanID:        1023,
					PacketFilters: []string{monitoring.MirrorSessionSpec_ALL_PKTS.String()},
					Collectors: []monitoring.MirrorCollector{
						{
							Type: monitoring.PacketCollectorType_ERSPAN_TYPE_3.String(),
							ExportCfg: &monitoring.MirrorExportConfig{
								Destination: "192.168.1.101",
							},
						},
					},
					MatchRules: []monitoring.MatchRule{
						{
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"10.1.1.1"},
							},
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"10.1.1.101"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/5555"},
							},
						},
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"10.2.2.202"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"UDP/5555"},
							},
						},
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"10.2.2.201"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"icmp/5555"},
							},
						},
					},
				},
			}: true,
			{
				ObjectMeta: api.ObjectMeta{
					Name:   "ValidateMirrorSession2",
					Tenant: "default",
				},
				TypeMeta: api.TypeMeta{
					Kind:       "MirrorSession",
					APIVersion: "v1",
				},
				Spec: monitoring.MirrorSessionSpec{
					PacketSize:    128,
					PacketFilters: []string{monitoring.MirrorSessionSpec_ALL_PKTS.String()},
					Collectors: []monitoring.MirrorCollector{
						{
							Type: monitoring.PacketCollectorType_ERSPAN_TYPE_3.String(),
							ExportCfg: &monitoring.MirrorExportConfig{
								Destination: "192.168.1.102",
							},
						},
					},
					MatchRules: []monitoring.MatchRule{
						{
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"10.1.1.2"},
							},
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"10.1.1.102"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"UDP/1234-1236"},
							},
						},
					},
				},
			}: true,
			{
				ObjectMeta: api.ObjectMeta{
					Name:   "ValidateMirrorSession3",
					Tenant: "default",
				},
				TypeMeta: api.TypeMeta{
					Kind:       "MirrorSession",
					APIVersion: "v1",
				},
				Spec: monitoring.MirrorSessionSpec{
					PacketSize:    128,
					PacketFilters: []string{monitoring.MirrorSessionSpec_ALL_PKTS.String()},
					Collectors: []monitoring.MirrorCollector{
						{
							Type: monitoring.PacketCollectorType_ERSPAN_TYPE_3.String(),
							ExportCfg: &monitoring.MirrorExportConfig{
								Destination: "192.168.1.103",
							},
						},
					},
					MatchRules: []monitoring.MatchRule{
						{
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"10.1.1.3"},
							},
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"10.1.1.103"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"any"},
							},
						},
					},
				},
			}: true,
			{
				ObjectMeta: api.ObjectMeta{
					Name:   "ValidateMirrorSessionInvalidProtocol",
					Tenant: "default",
				},
				TypeMeta: api.TypeMeta{
					Kind:       "MirrorSession",
					APIVersion: "v1",
				},
				Spec: monitoring.MirrorSessionSpec{
					PacketSize:    128,
					PacketFilters: []string{monitoring.MirrorSessionSpec_ALL_PKTS.String()},
					Collectors: []monitoring.MirrorCollector{
						{
							Type: monitoring.PacketCollectorType_ERSPAN_TYPE_3.String(),
							ExportCfg: &monitoring.MirrorExportConfig{
								Destination: "192.168.1.104",
							},
						},
					},
					MatchRules: []monitoring.MatchRule{
						{
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"10.1.1.4"},
							},
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"10.1.1.104"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"arp"},
							},
						},
					},
				},
			}: false, // only tcp, udp, icmp, any are supported
			{
				ObjectMeta: api.ObjectMeta{
					Name:   "ValidateMirrorSessionAnyProtoWithPort",
					Tenant: "default",
				},
				TypeMeta: api.TypeMeta{
					Kind:       "MirrorSession",
					APIVersion: "v1",
				},
				Spec: monitoring.MirrorSessionSpec{
					PacketSize:    128,
					PacketFilters: []string{monitoring.MirrorSessionSpec_ALL_PKTS.String()},
					Collectors: []monitoring.MirrorCollector{
						{
							Type: monitoring.PacketCollectorType_ERSPAN_TYPE_3.String(),
							ExportCfg: &monitoring.MirrorExportConfig{
								Destination: "192.168.1.105",
							},
						},
					},
					MatchRules: []monitoring.MatchRule{
						{
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"10.1.1.5"},
							},
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"10.1.1.105"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"any/1234"},
							},
						},
					},
				},
			}: false, // cannot specify port when protocol is any
		}

		// XXX:Need to create other objects - VRF/EPs/routes etc for testing with packets

		var (
			mirrorRestIf monitoring.MonitoringV1MirrorSessionInterface
		)

		BeforeEach(func() {
			apiGwAddr := ts.tu.ClusterVIP + ":" + globals.APIGwRESTPort
			restSvc, err := apiclient.NewRestAPIClient(apiGwAddr)
			if err == nil {
				By("Creating MirrorSession Client ------")
				mirrorRestIf = restSvc.MonitoringV1().MirrorSession()
			}
			Expect(err).ShouldNot(HaveOccurred())
			Expect(mirrorRestIf).ShouldNot(Equal(nil))

			By("Cleaning up MirrorSession before each test case ------")
			ctx := ts.tu.MustGetLoggedInContext(context.Background())
			s, err := mirrorRestIf.List(ctx, &api.ListWatchOptions{})
			Expect(err).ShouldNot(HaveOccurred())
			Eventually(func() bool {
				By("Deleting MirrorSession ------")
				for _, i := range s {
					_, err := mirrorRestIf.Delete(ctx, i.GetObjectMeta())
					if err != nil {
						return false
					}
				}
				return true
			}, 30, 5).Should(BeTrue(), fmt.Sprintf("Failed to delete mirror session after testing process"))
			time.Sleep(5 * time.Second)
		})

		AfterEach(func() {
			ctx := ts.tu.MustGetLoggedInContext(context.Background())
			s, err := mirrorRestIf.List(ctx, &api.ListWatchOptions{})
			Expect(err).ShouldNot(HaveOccurred())
			Eventually(func() bool {
				By("Deleting MirrorSession ------")
				for _, i := range s {
					_, err := mirrorRestIf.Delete(ctx, i.GetObjectMeta())
					if err != nil {
						return false
					}
				}
				return true
			}, 30, 5).Should(BeTrue(), fmt.Sprintf("Failed to delete mirror session after testing process"))
			time.Sleep(5 * time.Second)
		})

		It("Validate mirror session", func() {
			ctx := ts.tu.MustGetLoggedInContext(context.Background())
			for ms, isValid := range testValidateMirrorSessions {
				By(fmt.Sprintf("Validating mirror session %v", ms.Name))
				_, err := mirrorRestIf.Create(ctx, ms)
				By(fmt.Sprintf("Get err: %v", err))
				if isValid {
					Expect(err).Should(BeNil())
				} else {
					Expect(err).ShouldNot(BeNil())
				}
			}
		})

		It("Check max mirror sessions", func() {
			ctx := ts.tu.MustGetLoggedInContext(context.Background())
			ms := testMirrorSessions[0]
			initMirrorMap := make(map[string]monitoring.MirrorSessionSpec)
			listMirrorMap := make(map[string]monitoring.MirrorSessionSpec)

			for i := 0; i < statemgr.MaxMirrorSessions+1; i++ {
				ms.Name = fmt.Sprintf("max-mirror-%d", i+1)
				ms.Spec.SpanID = uint32(i + 1)
				ms.Spec.Collectors = []monitoring.MirrorCollector{
					{
						Type: monitoring.PacketCollectorType_ERSPAN_TYPE_2.String(),
						ExportCfg: &monitoring.MirrorExportConfig{
							Destination: fmt.Sprintf("192.168.%d.1", i),
						},
						StripVlanHdr: true,
					},
				}

				By(fmt.Sprintf("Creating MirrorSession %v", ms.Name))
				_, err := mirrorRestIf.Create(ctx, &ms)
				if i < statemgr.MaxMirrorSessions {
					Expect(err).ShouldNot(HaveOccurred())
					initMirrorMap[ms.GetName()] = ms.GetSpec()
				} else {
					Expect(err).Should(HaveOccurred())
				}
			}
			time.Sleep(10 * time.Second)
			s, err := mirrorRestIf.List(ctx, &api.ListWatchOptions{})
			Expect(err).ShouldNot(HaveOccurred())

			By("Deep check obtained mirror session content------")
			for _, sess := range s {
				listMirrorMap[sess.GetName()] = sess.GetSpec()
			}
			Expect(reflect.DeepEqual(initMirrorMap, listMirrorMap)).To(Equal(true))
		})

		It("Should run mirror sessions Test 1", func() {
			// create mirror session with exp duration of 5s
			// check that it starts active and then stops after 5s
			// delete mirror session

			ctx := ts.tu.MustGetLoggedInContext(context.Background())
			ms := testMirrorSessions[0]
			initMirrorMap := make(map[string]*monitoring.MirrorSessionSpec)
			listMirrorMap := make(map[string]*monitoring.MirrorSessionSpec)

			By("Creating MirrorSession ------")
			_, err := mirrorRestIf.Create(ctx, &ms)
			Expect(err).ShouldNot(HaveOccurred())
			spec := ms.GetSpec()
			initMirrorMap[ms.GetName()] = &spec
			time.Sleep(10 * time.Second)

			By("Checking that MirrorSession has started------")
			Eventually(func() bool {
				tms, err := mirrorRestIf.Get(ctx, &ms.ObjectMeta)
				if err != nil {
					By(fmt.Sprintf("GET err:%s", err))
					return false
				}
				spec := tms.GetSpec()
				listMirrorMap[tms.GetName()] = &spec
				if tms.Status.ScheduleState != monitoring.MirrorSessionState_ACTIVE.String() {
					By(fmt.Sprintf("mirror: %v state: %v", tms.Name, tms.Status.ScheduleState))
					return false
				}
				By("Deep check obtained mirror session content------")
				if len(initMirrorMap) != len(listMirrorMap) {
					By(fmt.Sprintf("get inconsistent number of mirror session after initialization. Prev: %v, Now: %v", len(initMirrorMap), len(listMirrorMap)))
					return false
				}
				// need to update spanID
				for name, spec := range initMirrorMap {
					if _, ok := listMirrorMap[name]; !ok {
						By(fmt.Sprintf("cannot find mirror session %v after initialization", name))
						return false
					}
					spec.SpanID = listMirrorMap[name].SpanID
					if !reflect.DeepEqual(spec, listMirrorMap[name]) {
						By(fmt.Sprintf("get inconsistent mirror session %v after initialization. Prev: %+v, After: %+v", ms.Name, *spec, *listMirrorMap[name]))
						return false
					}
				}

				return true
			}, 5, 1).Should(BeTrue(), fmt.Sprintf("Failed to start %s", ms.Name))

			/* XXX Reenable after session expiry is added in NAPLES
			By("Checking that MirrorSession has stopped using expriy time------")
			Eventually(func() bool {
				tms, err := mirrorRestIf.Get(ctx, &ms.ObjectMeta)
				if err != nil {
					By(fmt.Sprintf("GET err:%s", err))
					return false
				}
				if tms.Status.ScheduleState != monitoring.MirrorSessionState_STOPPED.String() {
					By(fmt.Sprintf("mirror state: %v", tms.Status.ScheduleState))
					return false
				}
				return true
			}, 10, 1).Should(BeTrue(), fmt.Sprintf("Failed to stop %s", ms.Name)) */
		})

		It("Should run mirror sessions Test 2", func() {
			// create mirror session with exp duration of 5s
			// check that it starts active and then stops after 5s
			// delete mirror session

			ctx := ts.tu.MustGetLoggedInContext(context.Background())
			apiGwAddr := ts.tu.ClusterVIP + ":" + globals.APIGwRESTPort
			nodeAuthFile, err := testutils.GetNodeAuthTokenTempFile(ctx, apiGwAddr, []string{"*"})

			ms := testMirrorSessions[1]
			initMirrorMap := make(map[string]*monitoring.MirrorSessionSpec)
			listMirrorMap := make(map[string]*monitoring.MirrorSessionSpec)

			By("Creating MirrorSession ------")
			_, err = mirrorRestIf.Create(ctx, &ms)
			Expect(err).ShouldNot(HaveOccurred())
			spec := ms.GetSpec()
			initMirrorMap[ms.GetName()] = &spec

			//_, err = mirrorRestIf.Update(ctx, &ms)
			//Expect(err).ShouldNot(HaveOccurred())

			By("Checking that MirrorSession has started------")
			var tms *monitoring.MirrorSession
			Eventually(func() bool {
				tmp, err := mirrorRestIf.Get(ctx, &ms.ObjectMeta)
				if err != nil {
					By(fmt.Sprintf("GET Mirror: %s err:%s", tmp.GetName(), err))
					return false
				}
				tms = tmp
				spec := tms.GetSpec()
				listMirrorMap[tms.GetName()] = &spec
				if tms.Status.ScheduleState != monitoring.MirrorSessionState_ACTIVE.String() {
					By(fmt.Sprintf("mirror: %v state: %v", tms.Name, tms.Status.ScheduleState))
					return false
				}
				By("Deep check obtained mirror session content------")
				if len(initMirrorMap) != len(listMirrorMap) {
					By(fmt.Sprintf("get inconsistent number of mirror session after initialization. Prev: %v, Now: %v", len(initMirrorMap), len(listMirrorMap)))
					return false
				}
				// need to update spanID
				for name, spec := range initMirrorMap {
					if _, ok := listMirrorMap[name]; !ok {
						By(fmt.Sprintf("cannot find mirror session %v after initialization", name))
						return false
					}
					spec.SpanID = listMirrorMap[name].SpanID
					if !reflect.DeepEqual(spec, listMirrorMap[name]) {
						By(fmt.Sprintf("get inconsistent mirror session %v after initialization. Prev: %+v, After: %+v", ms.Name, *spec, *listMirrorMap[name]))
						return false
					}
				}
				return true
			}, 30, 5).Should(BeTrue(), fmt.Sprintf("Failed to start %s", ms.Name))

			By("Checking mirror sessions on naples card")
			Eventually(func() bool {
				for _, naples := range ts.tu.NaplesNodes {
					By(fmt.Sprintf("curl -s -k --key %s --cert %s https://%s:8888/api/mirror/sessions/", nodeAuthFile, nodeAuthFile, ts.tu.NameToIPMap[naples]))
					st := ts.tu.LocalCommandOutput(fmt.Sprintf("curl -s -k --key %s --cert %s https://%s:8888/api/mirror/sessions/", nodeAuthFile, nodeAuthFile, ts.tu.NameToIPMap[naples]))
					fmt.Printf("received mirror session from naples: %s", st)
					var naplesMirror []monitoring.MirrorSession
					if err := json.Unmarshal([]byte(st), &naplesMirror); err != nil {
						By(fmt.Sprintf("received mirror session from naples: %v, %+v", naples, st))
						return false
					}

					if !strings.Contains(st, "TestMirrorSession999") {
						By(fmt.Sprintf("received mirror session from naples: %v, %v, %v", naples, string([]byte(st)), naplesMirror))
						By(fmt.Sprintf("invalid number of mirror session in %v, got %d, expected %d", naples, len(naplesMirror), 1))
						return false
					}
				}
				return true
			}, 120, 30).Should(BeTrue(), fmt.Sprintf("Failed to start %s", ms.Name))

			// Update mirror session match rules
			tms.Spec.MatchRules = []monitoring.MatchRule{
				{
					Dst: &monitoring.MatchSelector{
						IPAddresses: []string{"10.1.1.1"},
					},
					Src: &monitoring.MatchSelector{
						IPAddresses: []string{"10.1.1.10"},
					},
					AppProtoSel: &monitoring.AppProtoSelector{
						ProtoPorts: []string{"icmp"},
					},
				},
			}

			By("Update mirror session match rules")
			_, err = mirrorRestIf.Update(ctx, tms)
			Expect(err).ShouldNot(HaveOccurred())

			By("Checking mirror sessions on naples card")
			Eventually(func() bool {
				for _, naples := range ts.tu.NaplesNodes {
					By(fmt.Sprintf("curl -s -k --key %s --cert %s https://%s:8888/api/mirror/sessions/", nodeAuthFile, nodeAuthFile, ts.tu.NameToIPMap[naples]))
					st := ts.tu.LocalCommandOutput(fmt.Sprintf("curl -s -k --key %s --cert %s https://%s:8888/api/mirror/sessions/", nodeAuthFile, nodeAuthFile, ts.tu.NameToIPMap[naples]))
					fmt.Printf("received mirror session from naples: %s", st)
					var naplesMirror []monitoring.MirrorSession
					if err := json.Unmarshal([]byte(st), &naplesMirror); err != nil {
						By(fmt.Sprintf("received mirror session from naples: %v, %+v", naples, st))
						return false
					}

					if !strings.Contains(st, "TestMirrorSession999") {
						By(fmt.Sprintf("received mirror session from naples: %v, %v, %v", naples, string([]byte(st)), naplesMirror))
						By(fmt.Sprintf("invalid number of mirror session in %v, got %d, expected %d", naples, len(naplesMirror), 1))
						return false
					}
				}
				return true
			}, 120, 30).Should(BeTrue(), fmt.Sprintf("Failed to start %s", ms.Name))
		})

		It("Should run mirror sessions after shcedule time", func() {
			// create mirror session with exp duration of 5s
			// check that it starts active and then stops after 5s
			ctx := ts.tu.MustGetLoggedInContext(context.Background())
			apiGwAddr := ts.tu.ClusterVIP + ":" + globals.APIGwRESTPort
			nodeAuthFile, err := testutils.GetNodeAuthTokenTempFile(ctx, apiGwAddr, []string{"*"})
			if err != nil {
				By(fmt.Sprintf("Error GetNodeAuthTokenTempFile: %s", err))
			}
			scheduledTestCase := monitoring.MirrorSession{
				ObjectMeta: api.ObjectMeta{
					Name:   "TestMirrorSession2",
					Tenant: "default",
				},
				TypeMeta: api.TypeMeta{
					Kind:       "MirrorSession",
					APIVersion: "v1",
				},
				Spec: monitoring.MirrorSessionSpec{
					PacketSize:    128,
					PacketFilters: []string{monitoring.MirrorSessionSpec_ALL_PKTS.String()},
					Collectors: []monitoring.MirrorCollector{
						{
							Type: monitoring.PacketCollectorType_ERSPAN_TYPE_3.String(),
							ExportCfg: &monitoring.MirrorExportConfig{
								Destination: "192.168.1.10",
							},
						},
					},
					MatchRules: []monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"10.1.1.10"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/5555"},
							},
						},
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"10.2.2.20"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"UDP/5555"},
							},
						},
					},
					// for this test case
					// add schedule time after 60 seconds
					// test mirror session state
					StartConditions: monitoring.MirrorStartConditions{
						ScheduleTime: &api.Timestamp{
							Timestamp: types.Timestamp{
								Seconds: time.Now().Unix() + 60,
							},
						},
					},
				},
			}

			ms := scheduledTestCase
			initMirrorMap := make(map[string]*monitoring.MirrorSessionSpec)
			listMirrorMap := make(map[string]*monitoring.MirrorSessionSpec)

			By("Creating MirrorSession ------")
			spec := ms.GetSpec()
			initMirrorMap[ms.GetName()] = &spec
			_, err = mirrorRestIf.Create(ctx, &ms)
			Expect(err).ShouldNot(HaveOccurred())

			By("Checking whether MirrorSession has started after scheduled time or not------")
			Eventually(func() bool {
				tms, err := mirrorRestIf.Get(ctx, &ms.ObjectMeta)
				spec := tms.GetSpec()
				listMirrorMap[tms.GetName()] = &spec
				if err != nil {
					By(fmt.Sprintf("GET err:%s", err))
					return false
				}

				// raise error if session start before scheduled time
				// the expected state should be SCHEDULED
				if tms.Status.ScheduleState != monitoring.MirrorSessionState_SCHEDULED.String() {
					By(fmt.Sprintf("unexpected mirror session state: %v before scheduled time", tms.Status.ScheduleState))
					return false
				}

				return true
			}, 60, 3).Should(BeTrue(), fmt.Sprintf("Failed to get SCHEDULED state for scheduled mirror session %s", ms.Name))

			Eventually(func() bool {
				By("Make sure mirror session is not on naples before scheduled time------")
				for _, naples := range ts.tu.NaplesNodes {
					By(fmt.Sprintf("curl -s -k --key %s --cert %s https://%s:8888/api/mirror/sessions/", nodeAuthFile, nodeAuthFile, ts.tu.NameToIPMap[naples]))
					st := ts.tu.LocalCommandOutput(fmt.Sprintf("curl -s -k --key %s --cert %s https://%s:8888/api/mirror/sessions/", nodeAuthFile, nodeAuthFile, ts.tu.NameToIPMap[naples]))
					By(fmt.Sprintf("received mirror session from naples: %s", st))
					var naplesMirror []monitoring.MirrorSession
					if err := json.Unmarshal([]byte(st), &naplesMirror); err != nil {
						By(fmt.Sprintf("received mirror session from naples: %v, %+v", naples, st))
						return false
					}

					if strings.Contains(st, "TestMirrorSession2") {
						By("Scheduled mirror session should not be on naples!!!")
						return false
					}
				}

				By("Deep check obtained mirror session content on venice------")
				// Deep check mirror session on venice
				if len(initMirrorMap) != len(listMirrorMap) {
					By(fmt.Sprintf("get inconsistent number of mirror session after initialization. Prev: %v, Now: %v", len(initMirrorMap), len(listMirrorMap)))
					return false
				}
				// need to update spanID
				for name, spec := range initMirrorMap {
					if _, ok := listMirrorMap[name]; !ok {
						By(fmt.Sprintf("cannot find mirror session %v after initialization", name))
						return false
					}
					spec.SpanID = listMirrorMap[name].SpanID
					if !reflect.DeepEqual(spec, listMirrorMap[name]) {
						By(fmt.Sprintf("get inconsistent mirror session %v after initialization. Prev: %+v, After: %+v", ms.Name, *spec, *listMirrorMap[name]))
						return false
					}
				}
				return true
			}, 60, 3).Should(BeTrue(), fmt.Sprintf("Failed to get SCHEDULED state for scheduled mirror session %s", ms.Name))

			Eventually(func() bool {
				// get the new state
				tms, err := mirrorRestIf.Get(ctx, &ms.ObjectMeta)
				if err != nil {
					By(fmt.Sprintf("GET err:%s", err))
					return false
				}

				// raise error if session doesn't start after scheduled time
				// the expected state should be ACTIVE
				if tms.Status.ScheduleState != monitoring.MirrorSessionState_ACTIVE.String() {
					By(fmt.Sprintf("unexpected mirror session state: %v after scheduled time", tms.Status.ScheduleState))
					return false
				}

				By("Deep check obtained mirror session content on venice------")
				ms := scheduledTestCase
				initMirrorMap := make(map[string]*monitoring.MirrorSessionSpec)
				listMirrorMap := make(map[string]*monitoring.MirrorSessionSpec)
				initSpec := ms.GetSpec()
				listSpec := ms.GetSpec()
				initMirrorMap[ms.GetName()] = &initSpec
				listMirrorMap[tms.GetName()] = &listSpec
				if len(initMirrorMap) != len(listMirrorMap) {
					By(fmt.Sprintf("get inconsistent number of mirror session after initialization. Prev: %v, Now: %v", len(initMirrorMap), len(listMirrorMap)))
					return false
				}
				// need to update spanID
				for name, spec := range initMirrorMap {
					if _, ok := listMirrorMap[name]; !ok {
						By(fmt.Sprintf("cannot find mirror session %v after initialization", name))
						return false
					}
					spec.SpanID = listMirrorMap[name].SpanID
					if !reflect.DeepEqual(spec, listMirrorMap[name]) {
						By(fmt.Sprintf("get inconsistent mirror session %v after initialization. Prev: %+v, After: %+v", ms.Name, *spec, *listMirrorMap[name]))
						return false
					}
				}

				By("Make sure mirror session is on naples after scheduled time------")
				for _, naples := range ts.tu.NaplesNodes {
					By(fmt.Sprintf("curl -s -k --key %s --cert %s https://%s:8888/api/mirror/sessions/", nodeAuthFile, nodeAuthFile, ts.tu.NameToIPMap[naples]))
					st := ts.tu.LocalCommandOutput(fmt.Sprintf("curl -s -k --key %s --cert %s https://%s:8888/api/mirror/sessions/", nodeAuthFile, nodeAuthFile, ts.tu.NameToIPMap[naples]))
					By(fmt.Sprintf("received mirror session from naples: %s", st))
					var naplesMirror []monitoring.MirrorSession
					if err := json.Unmarshal([]byte(st), &naplesMirror); err != nil {
						By(fmt.Sprintf("received mirror session from naples: %v, %+v", naples, st))
						return false
					}

					By(fmt.Sprintf("received mirror session from naples: %v, %v, %v", naples, string([]byte(st)), naplesMirror))
					if !strings.Contains(st, "TestMirrorSession2") {
						By("Active mirror session should be on naples!!!")
						return false
					}

					By("Deep check obtained mirror session content on naples------")
					if !reflect.DeepEqual(naplesMirror[0].GetName(), ms.GetName()) {
						By(fmt.Sprintf("get inconsistent mirror session %v after initialization", ms.Name))
						return false
					}
				}

				return true
			}, 180, 30).Should(BeTrue(), fmt.Sprintf("Failed to get mirror session active state after scheduled time %s", ms.Name))
		})

		It("Validate propagation status for mirror session", func() {
			ctx := ts.tu.MustGetLoggedInContext(context.Background())
			ms := testMirrorSessions[1]
			By("Creating MirrorSession ------")
			_, err := mirrorRestIf.Create(ctx, &ms)
			Expect(err).ShouldNot(HaveOccurred())
			By("Checking that MirrorSession has been propagated to agents------")
			Eventually(func() bool {
				mirrorStat, err := mirrorRestIf.Get(ctx, &ms.ObjectMeta)
				Expect(err).ShouldNot(HaveOccurred())
				if int(mirrorStat.Status.PropagationStatus.Updated) != len(ts.tu.NaplesNodes) {
					By(fmt.Sprintf("MirrorSession Status : %+v", mirrorStat.Status))
					return false
				}
				By(fmt.Sprintf("MirrorSession Status : %+v", mirrorStat.Status))
				return true
			}, 120, 1).Should(BeTrue(), "Failed to Propagate the MirrorSession")
		})
	})
})
