package cfggen

import (
	"fmt"

	log "github.com/sirupsen/logrus"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/nic/e2etests/go/agent/pkg"
)

func (c *CfgGen) GenerateNamespaces() error {
	var cfg pkg.IOTAConfig
	var vrfs []*netproto.Namespace
	var vrfManifest *pkg.Object
	for _, o := range c.Config.Objects {
		if o.Kind == "Namespace" {
			vrfManifest = &o
			break
		}
	}
	if vrfManifest == nil {
		log.Debug("Namespace Manifest missing.")
		log.Info("Skipping Namespace Generation")
		return nil
	}

	log.Infof("Generating %v VRFs.", vrfManifest.Count)

	for i := 0; i < vrfManifest.Count; i++ {
		vrfName := fmt.Sprintf("%s-%d", vrfManifest.Name, i)
		n := netproto.Namespace{
			TypeMeta: api.TypeMeta{
				Kind: "Namespace",
			},
			ObjectMeta: api.ObjectMeta{
				Tenant: "default",
				Name:   vrfName,
			},
			Spec: netproto.NamespaceSpec{
				NamespaceType: "CUSTOMER",
			},
		}
		vrfs = append(vrfs, &n)
	}
	cfg.Type = "netagent"
	cfg.ObjectKey = "meta.tenant/meta.name"
	cfg.RestEndpoint = "api/namespaces/"
	cfg.Objects = vrfs
	c.Namespaces = cfg

	return nil
}
