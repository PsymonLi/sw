package impl

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/monitoring"
	apiintf "github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/kvstore/store"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/runtime"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestValidateFlowExportPolicy(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), time.Minute*5)
	defer cancel()

	logConfig := &log.Config{
		Module:      "Mirror-hooks",
		Format:      log.LogFmt,
		Filter:      log.AllowAllFilter,
		Debug:       false,
		CtxSelector: log.ContextAll,
		LogToStdout: true,
		LogToFile:   false,
	}

	// Initialize logger config
	l := log.SetConfig(logConfig)
	s := &flowExpHooks{
		logger: l,
	}
	storecfg := store.Config{
		Type:    store.KVStoreTypeMemkv,
		Codec:   runtime.NewJSONCodec(runtime.NewScheme()),
		Servers: []string{t.Name()},
	}

	kvs, err := store.New(storecfg)
	if err != nil {
		t.Fatalf("unable to create kvstore %s", err)
	}
	txn := kvs.NewTxn()

	testFlowExpSpecList := []struct {
		name   string
		policy monitoring.FlowExportPolicy
		fail   bool
	}{
		{
			name: "invalid interval (empty)",
			fail: true,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "empty-interval",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval: "",
					Format:   "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/1234",
						},
					},
				},
			},
		},

		{
			name: "invalid interval",
			fail: true,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "invalid-interval-155",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval:         "155",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/1234",
						},
					},
				},
			},
		},

		{
			name: "invalid DNS",
			fail: true,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "invalid-DNS",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "test.pensando.iox",
							Transport:   "UDP/1234",
						},
					},
				},
			},
		},

		{
			name: "duplicate targets",
			fail: true,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "duplicate-targets",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/1234",
						},
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/1234",
						},
					},
				},
			},
		},

		{
			name: "duplicate targets with diff proto-ports",
			fail: false,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "duplicate-targets-diff-protoport",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/1234",
						},
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/1235",
						},
					},
				},
			},
		},

		{
			name: "invalid destination (empty)",
			fail: true,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "empty-destination",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "",
							Transport:   "UDP/1234",
						},
					},
				},
			},
		},

		{
			name: "invalid transport (empty)",
			fail: true,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "empty-transport",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "",
						},
					},
				},
			},
		},

		{
			name: "invalid targets (empty)",
			fail: true,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "empty-targets",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
				},
			},
		},

		{
			name: "invalid Transport, missing port",
			fail: true,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "invalid-Transport-missing-port",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "UDP",
						},
					},
				},
			},
		},

		{
			name: "invalid Transport, missing protocol",
			fail: true,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "invalid-Transport-missing-protocol",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "1234",
						},
					},
				},
			},
		},

		{
			name: "invalid protocol, should be UDP",
			fail: true,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "invalid-protocol-should-be-UDP",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "IP/1234",
						},
					},
				},
			},
		},

		{
			name: "invalid port",
			fail: true,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "invalid-port-aaaa",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/aaaa",
						},
					},
				},
			},
		},

		{
			name: "invalid port",
			fail: true,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "invalid-port-65536",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/65536",
						},
					},
				},
			},
		},

		{
			name: "valid policy",
			fail: false,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "valid-policy",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1/8"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2/24"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/1234",
						},
					},
				},
			},
		},

		{
			name: "invalid policy with invalid gateway",
			fail: true,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "invalid-gateway-policy",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/1234",
							Gateway:     "invalid",
						},
					},
				},
			},
		},
		{
			name: "valid policy with gateway",
			fail: false,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "valid-gateway-policy",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/1234",
							Gateway:     "10.1.1.254",
						},
					},
				},
			},
		},
		{
			name: "valid policy with duplicate exports",
			fail: false,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "dup-valid-gateway-policy",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},

						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.1"},
							},
							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1010"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/1234",
							Gateway:     "10.1.1.254",
						},
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/1234",
						},
					},
				},
			},
		},
		{
			name: "src dst any any",
			fail: false,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "valid-gateway-policy",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"any"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"any"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/1234",
							Gateway:     "10.1.1.254",
						},
					},
				},
			},
		},
		{
			name: "invalid multiple src any any",
			fail: true,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "valid-gateway-policy",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1", "any"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"any"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/1234",
							Gateway:     "10.1.1.254",
						},
					},
				},
			},
		},
		{
			name: "invalid multiple dst any any",
			fail: true,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "valid-gateway-policy",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"any"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1", "any"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/1234",
							Gateway:     "10.1.1.254",
						},
					},
				},
			},
		},
		{
			name: "invalid ip string",
			fail: true,
			policy: monitoring.FlowExportPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "valid-gateway-policy",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"any"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1, 2.2.2.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/1234",
							Gateway:     "10.1.1.254",
						},
					},
				},
			},
		},
	}

	for i := range testFlowExpSpecList {
		_, ok, err := s.validateFlowExportPolicy(ctx, kvs, txn, "", apiintf.CreateOper, false,
			testFlowExpSpecList[i].policy)

		if testFlowExpSpecList[i].fail == true {
			t.Logf(fmt.Sprintf("test [%v] returned %v", testFlowExpSpecList[i].name, err))
			Assert(t, ok == false, "test [%v] returned %v", testFlowExpSpecList[i].name, err)
		} else {
			t.Log(fmt.Sprintf("test [%v] returned %v", testFlowExpSpecList[i].name, err))
			Assert(t, ok == true, "test [%v] returned %v", testFlowExpSpecList[i].name, err)
		}
	}
}

func TestInvalidType(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), time.Minute*5)
	defer cancel()

	logConfig := &log.Config{
		Module:      "Mirror-hooks",
		Format:      log.LogFmt,
		Filter:      log.AllowAllFilter,
		Debug:       false,
		CtxSelector: log.ContextAll,
		LogToStdout: true,
		LogToFile:   false,
	}

	// Initialize logger config
	l := log.SetConfig(logConfig)
	s := &flowExpHooks{
		logger: l,
	}
	storecfg := store.Config{
		Type:    store.KVStoreTypeMemkv,
		Codec:   runtime.NewJSONCodec(runtime.NewScheme()),
		Servers: []string{t.Name()},
	}

	kvs, err := store.New(storecfg)
	if err != nil {
		t.Fatalf("unable to create kvstore %s", err)
	}
	txn := kvs.NewTxn()

	_, ok, err := s.validateFlowExportPolicy(ctx, kvs, txn, "", apiintf.CreateOper, false,
		"test")
	Assert(t, ok == false, "didnt fail for invalid type")
	Assert(t, err != nil, "didnt fail for invalid type")

}

func TestFlowExportMaxCollectorValidation(t *testing.T) {
	testExportMax := monitoring.FlowExportPolicy{
		TypeMeta: api.TypeMeta{
			Kind: "FlowExportPolicy",
		},
		ObjectMeta: api.ObjectMeta{
			Namespace: globals.DefaultNamespace,
			Name:      globals.DefaultTenant,
			Tenant:    globals.DefaultTenant,
		},
		Spec: monitoring.FlowExportPolicySpec{
			Format: monitoring.MonitoringExportFormat_SYSLOG_BSD.String(),
		},
	}
	var maxCollectors []monitoring.ExportConfig
	for i := 1; i <= veniceMaxNetflowCollectors+1; i++ {
		newCol := monitoring.ExportConfig{
			Destination: "10.1.1.1",
		}
		maxCollectors = append(maxCollectors, newCol)
	}
	testExportMax.Spec.Exports = maxCollectors
	testFlowExportList := &monitoring.FlowExportPolicyList{
		TypeMeta: api.TypeMeta{
			Kind:       "FlowExportPolicyList",
			APIVersion: "v1",
		},
		Items: []*monitoring.FlowExportPolicy{
			{
				TypeMeta: api.TypeMeta{
					Kind: "flowExportPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "valid-policy",
					Tenant:    globals.DefaultTenant,
				},

				Spec: monitoring.FlowExportPolicySpec{
					MatchRules: []*monitoring.MatchRule{
						{
							Src: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.1"},
							},

							Dst: &monitoring.MatchSelector{
								IPAddresses: []string{"1.1.1.2"},
							},
							AppProtoSel: &monitoring.AppProtoSelector{
								ProtoPorts: []string{"TCP/1000"},
							},
						},
					},

					Interval:         "15s",
					TemplateInterval: "5m",
					Format:           "IPFIX",
					Exports: []monitoring.ExportConfig{
						{
							Destination: "10.1.1.100",
							Transport:   "UDP/1234",
						},
					},
				},
			},
		},
	}
	err := globalFlowExportValidator(&testExportMax, testFlowExportList)
	Assert(t, err != nil, "validation passed, expecting to fail for %v", testExportMax.Name)

}
