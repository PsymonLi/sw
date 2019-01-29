package cfggen

import (
	"fmt"
	"strconv"
	"strings"

	log "github.com/sirupsen/logrus"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/netagent/protos/netproto"
	"github.com/pensando/sw/nic/e2etests/go/agent/pkg"
)

func (c *CfgGen) GenerateALGs() error {
	var apps []*netproto.App
	var appManifest *pkg.Object
	for _, o := range c.Config.Objects {
		o := o
		if o.Kind == "App" {
			appManifest = &o
		}
	}
	if appManifest == nil {
		log.Debug("App Manifest missing.")
	}

	log.Infof("Generating %v ALGs.", appManifest.Count)

	for i := 0; i < appManifest.Count; i++ {
		var app netproto.App
		nsIdx := i % len(c.Namespaces)
		namespace := c.Namespaces[nsIdx]
		appType, proto, port, icmpCode, icmpType, idleTimeout, err := convertAppInfo(c.Template.ALGInfo[i%len(c.Template.ALGInfo)])
		if err != nil {
			return err
		}

		appName := fmt.Sprintf("%s-%s-%d", appManifest.Name, appType, i)
		switch appType {
		case "ftp":
			app = netproto.App{
				TypeMeta: api.TypeMeta{
					Kind: "App",
				},
				ObjectMeta: api.ObjectMeta{
					Tenant:    "default",
					Namespace: namespace.Name,
					Name:      appName,
				},
				Spec: netproto.AppSpec{
					ALGType:        appType,
					AppIdleTimeout: idleTimeout,
					ProtoPorts:     []string{fmt.Sprintf("%s/%s", proto, port)},
					ALG: &netproto.ALG{
						FTP: &netproto.FTP{
							AllowMismatchIPAddresses: true,
						},
					},
				},
			}
		case "tftp":
			app = netproto.App{
				TypeMeta: api.TypeMeta{
					Kind: "App",
				},
				ObjectMeta: api.ObjectMeta{
					Tenant:    "default",
					Namespace: namespace.Name,
					Name:      appName,
				},
				Spec: netproto.AppSpec{
					ALGType:        appType,
					AppIdleTimeout: idleTimeout,
					ProtoPorts:     []string{fmt.Sprintf("%s/%s", proto, port)},
					ALG: &netproto.ALG{
						TFTP: &netproto.TFTP{},
					},
				},
			}
		case "dns":
			app = netproto.App{
				TypeMeta: api.TypeMeta{
					Kind: "App",
				},
				ObjectMeta: api.ObjectMeta{
					Tenant:    "default",
					Namespace: namespace.Name,
					Name:      appName,
				},
				Spec: netproto.AppSpec{
					ALGType:        appType,
					AppIdleTimeout: idleTimeout,
					ProtoPorts:     []string{fmt.Sprintf("%s/%s", proto, port)},
					ALG: &netproto.ALG{
						DNS: &netproto.DNS{
							DropMultiQuestionPackets: true,
							DropLargeDomainPackets:   true,
							DropMultiZonePackets:     true,
							MaxMessageLength:         512,
							QueryResponseTimeout:     idleTimeout},
					},
				},
			}
		case "icmp":
			c, _ := strconv.Atoi(icmpCode)
			t, _ := strconv.Atoi(icmpType)

			app = netproto.App{
				TypeMeta: api.TypeMeta{
					Kind: "App",
				},
				ObjectMeta: api.ObjectMeta{
					Tenant:    "default",
					Namespace: namespace.Name,
					Name:      appName,
				},
				Spec: netproto.AppSpec{
					ALGType:        appType,
					AppIdleTimeout: idleTimeout,
					ProtoPorts:     []string{fmt.Sprintf("%s/%s", proto, port)},
					ALG: &netproto.ALG{
						ICMP: &netproto.ICMP{
							Type: uint32(t),
							Code: uint32(c),
						},
					},
				},
			}
		}
		apps = append(apps, &app)
	}
	c.Apps = apps
	return nil
}

// convertAppProtoPort converts appProtoPort template to an app type, protocol and port icmp code and icmp type.
func convertAppInfo(appProtoPort string) (appType, protocol, port, icmpCode, icmpType, idleTimeout string, err error) {
	components := strings.Split(appProtoPort, "/")
	if len(components) != 4 {
		log.Errorf("Invalid value %s in the template file.", appProtoPort)
		err = fmt.Errorf("invalid value in the template file. Expecting <AlgType>/<Protocol>/<Port>/<AppIdleTimeout> format. Found: %s", appProtoPort)
		return
	}
	appType = components[0]
	idleTimeout = components[3]
	switch {
	case components[1] == "tcp" || components[1] == "udp":
		protocol = components[1]
		port = components[2]
		return
	case components[0] == "icmp":
		protocol = components[0]
		icmpCode = components[1]
		icmpType = components[2]
		return
	default:
		err = fmt.Errorf("invalid protocol %s", protocol)
		return
	}
}
