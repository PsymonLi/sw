package equinix_test

import (
	"encoding/binary"
	"encoding/json"
	"fmt"
	"net"
	"strings"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/objects"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
	uuid "github.com/satori/go.uuid"
	yaml "gopkg.in/yaml.v2"
)

func getUniqueMac(i int) string {
	bytes := make([]byte, 3)
	for j := 2; j >= 0; j-- {
		bytes[j] = (byte)(i % 255)
		i = i / 255
	}
	return fmt.Sprintf("0001.01%02x.%02x%02x", bytes[0], bytes[1], bytes[2])
}

var _ = Describe("VPC", func() {
	var (
		defaultTenants []string
		naplesCount    = make(map[string]int32)
	)

	BeforeEach(func() {
		// verify cluster is in good health
		Eventually(func() error {
			return ts.model.VerifyClusterStatus()
		}).Should(Succeed())

		if len(defaultTenants) == 0 {
			tenantList, err := ts.model.ConfigClient().ListTenant()
			Expect(err).ShouldNot(HaveOccurred())
			for _, t := range tenantList {
				if t.GetName() != "default" {
					defaultTenants = append(defaultTenants, t.GetName())
					naplesCount[t.GetName()] = getNaplesCount(t.GetName())
				}
			}
		}
	})

	AfterEach(func() {
	})

	Context("VPC tests", func() {
		It("Add & delete VPC two times", func() {
			vpcAddDel(defaultTenants, naplesCount)
			vpcAddDel(defaultTenants, naplesCount)
		})

		It("Multiple VPCs per Tenant", func() {
			var newVpcList []*objects.VpcObjCollection
			rmacCtr := 1
			for i, tenantName := range defaultTenants {
				log.Infof("%v Tenant : %v", i, tenantName)
				//Create VPC
				vpcName1 := "testVPC1"
				vpc1 := ts.model.NewVPC(tenantName, vpcName1, getUniqueMac(rmacCtr), uint32(700+i), "")
				Expect(vpc1.Commit()).Should(Succeed())
				newVpcList = append(newVpcList, vpc1)
				rmacCtr += 1
				//Create 2nd VPC
				vpcName2 := "testVPC2"
				vpc2 := ts.model.NewVPC(tenantName, vpcName2, getUniqueMac(rmacCtr), uint32(1400+i), "")
				Expect(vpc2.Commit()).Should(Succeed())

				newVpcList = append(newVpcList, vpc2)
				rmacCtr += 1

				//Validate using Get command
				veniceVpc, err := ts.model.GetVPC(vpcName1, tenantName)
				Expect(err).ShouldNot(HaveOccurred())
				Expect(veniceVpc.Obj.Name == vpcName1).Should(BeTrue())

				//Verify for 2nd vpc
				veniceVpc, err = ts.model.GetVPC(vpcName2, tenantName)
				Expect(err).ShouldNot(HaveOccurred())
				Expect(veniceVpc.Obj.Name == vpcName2).Should(BeTrue())
			}

			for _, tenantName := range defaultTenants {
				//check propagation status
				Eventually(func() error {
					vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
						ts.model.Testbed())
					Expect(err).ShouldNot(HaveOccurred())
					return vpcc.VerifyPropagationStatus(naplesCount[tenantName])
				}, "5m", "10s").Should(Succeed())

				if *scaleFlag {
					continue
				}
				vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
					ts.model.Testbed())
				Expect(err).ShouldNot(HaveOccurred())
				verifyNetAgentVpcState(vpcc)
				verifyPDSVpcState(vpcc)
			}

			//Delete VPC
			for _, vpc := range newVpcList {
				//Delete previously added VPC
				Expect(vpc.Delete()).Should(Succeed())
			}

			for _, tenantName := range defaultTenants {
				//check propagation status
				Eventually(func() error {
					vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
						ts.model.Testbed())
					Expect(err).ShouldNot(HaveOccurred())
					return vpcc.VerifyPropagationStatus(naplesCount[tenantName])
				}, "5m", "10s").Should(Succeed())

				if *scaleFlag {
					continue
				}
				vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
					ts.model.Testbed())
				Expect(err).ShouldNot(HaveOccurred())
				verifyNetAgentVpcState(vpcc)
				verifyPDSVpcState(vpcc)
			}
		})

		It("Update router mac for VPC", func() {

			voc := objects.NewVPCCollection(ts.model.ConfigClient(), ts.model.Testbed())

			var savedRmac []string

			for i, tenantName := range defaultTenants {
				vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
					ts.model.Testbed())

				Expect(err).ShouldNot(HaveOccurred())
				Expect(len(vpcc.Objs)).ShouldNot(BeZero())

				//pick first non-default vpc
				vpcName := vpcc.Objs[0].Obj.GetName()
				log.Infof("VPC %s Tenant %s", vpcName, tenantName)
				vpc, err := ts.model.GetVPC(vpcName, tenantName)
				Expect(err).ShouldNot(HaveOccurred())
				voc.AddVPC(vpc)

				oldRmac := vpc.Obj.Spec.GetRouterMACAddress()
				savedRmac = append(savedRmac, oldRmac)

				newRmac := getUniqueMac(i + 1)

				vpc.UpdateRMAC(newRmac)

				log.Infof("Old rmac %s new rmac %s", oldRmac, newRmac)
			}

			Expect(voc.Update()).Should(Succeed())

			//veniceVpc, err := ts.model.GetVPC(vpcName, tenantName)
			//Expect(err).ShouldNot(HaveOccurred())
			//Expect(veniceVpc.Obj.Spec.RouterMACAddress == newRmac).Should(BeTrue())

			for _, tenantName := range defaultTenants {
				//check propagation status
				Eventually(func() error {
					vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
						ts.model.Testbed())
					Expect(err).ShouldNot(HaveOccurred())
					return vpcc.VerifyPropagationStatus(naplesCount[tenantName])
				}).Should(Succeed())

				if *scaleFlag {
					continue
				}

				vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
					ts.model.Testbed())
				Expect(err).ShouldNot(HaveOccurred())
				verifyNetAgentVpcState(vpcc)
				verifyPDSVpcState(vpcc)
			}

			for i, v := range voc.Objs {
				//Revert to old mac
				v.UpdateRMAC(savedRmac[i])
			}
			Expect(voc.Update()).Should(Succeed())

			for _, tenantName := range defaultTenants {
				//check propagation status
				Eventually(func() error {
					vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
						ts.model.Testbed())
					Expect(err).ShouldNot(HaveOccurred())
					return vpcc.VerifyPropagationStatus(naplesCount[tenantName])
				}).Should(Succeed())

				if *scaleFlag {
					continue
				}

				vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
					ts.model.Testbed())
				Expect(err).ShouldNot(HaveOccurred())
				verifyNetAgentVpcState(vpcc)
				verifyPDSVpcState(vpcc)
			}
		})

		It("Change VPC RT & verify config", func() {

			voc := objects.NewVPCCollection(ts.model.ConfigClient(), ts.model.Testbed())
			offset := uint32(10)

			for _, tenantName := range defaultTenants {
				vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
					ts.model.Testbed())

				Expect(err).ShouldNot(HaveOccurred())
				Expect(len(vpcc.Objs)).ShouldNot(BeZero())
				//pick first VPC
				vpcName := vpcc.Objs[0].Obj.GetName()
				log.Infof("VPC %s Tenant %s", vpcName, tenantName)
				vpc, err := ts.model.GetVPC(vpcName, tenantName)
				Expect(err).ShouldNot(HaveOccurred())
				voc.AddVPC(vpc)

				//Change its RT and update to api server
				exportRTs := vpc.Obj.Spec.RouteImportExport.ExportRTs
				importRTs := vpc.Obj.Spec.RouteImportExport.ImportRTs
				//Update RT value
				exportRTs[0].AssignedValue += offset
				importRTs[0].AssignedValue += offset
				log.Infof("Export RT new assigned value %v", exportRTs[0].AssignedValue)
				log.Infof("Import RT new assigned value %v", importRTs[0].AssignedValue)
			}

			Expect(voc.Update()).Should(Succeed())

			for _, tenantName := range defaultTenants {
				//check propagation status
				Eventually(func() error {
					vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
						ts.model.Testbed())
					Expect(err).ShouldNot(HaveOccurred())
					return vpcc.VerifyPropagationStatus(naplesCount[tenantName])
				}).Should(Succeed())

				if *scaleFlag {
					continue
				}

				vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
					ts.model.Testbed())
				Expect(err).ShouldNot(HaveOccurred())
				verifyNetAgentVpcState(vpcc)
				verifyPDSVpcState(vpcc)
			}

			for _, v := range voc.Objs {
				//Restore RT value
				exportRTs := v.Obj.Spec.RouteImportExport.ExportRTs
				importRTs := v.Obj.Spec.RouteImportExport.ImportRTs

				exportRTs[0].AssignedValue -= offset
				importRTs[0].AssignedValue -= offset
			}

			Expect(voc.Update()).Should(Succeed())

			for _, tenantName := range defaultTenants {
				//check propagation status
				Eventually(func() error {
					vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
						ts.model.Testbed())
					Expect(err).ShouldNot(HaveOccurred())
					return vpcc.VerifyPropagationStatus(naplesCount[tenantName])
				}).Should(Succeed())

				if *scaleFlag {
					continue
				}

				vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
					ts.model.Testbed())
				Expect(err).ShouldNot(HaveOccurred())
				verifyNetAgentVpcState(vpcc)
				verifyPDSVpcState(vpcc)
			}

		})
	})
})

func getNaplesCount(tenant string) int32 {
	npc, err := ts.model.Naples().SelectByTenant(tenant)
	ExpectWithOffset(1, err).ShouldNot(HaveOccurred())
	return int32(len(npc.FakeNodes) + len(npc.Nodes))

}

func vpcAddDel(tenants []string, naplesCount map[string]int32) {

	vpcName := "testVPC"

	var newVpcList []*objects.VpcObjCollection

	for i, tenantName := range tenants {
		vpc := ts.model.NewVPC(tenantName, vpcName, getUniqueMac(i+1), uint32(700+i), "")
		Expect(vpc.Commit()).Should(Succeed())

		newVpcList = append(newVpcList, vpc)

		//Validate using Get command
		veniceVpc, err := ts.model.GetVPC(vpcName, tenantName)
		Expect(err).ShouldNot(HaveOccurred())
		Expect(veniceVpc.Obj.Name == vpcName).Should(BeTrue())
	}

	for _, tenantName := range tenants {
		//check propagation status
		Eventually(func() error {
			vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
				ts.model.Testbed())
			Expect(err).ShouldNot(HaveOccurred())
			return vpcc.VerifyPropagationStatus(naplesCount[tenantName])
		}, "5m", "10s").Should(Succeed())

		if *scaleFlag {
			continue
		}
		vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
			ts.model.Testbed())
		Expect(err).ShouldNot(HaveOccurred())
		verifyNetAgentVpcState(vpcc)
		verifyPDSVpcState(vpcc)
	}

	for _, vpc := range newVpcList {
		//Delete previously added VPC
		Expect(vpc.Delete()).Should(Succeed())
	}

	for _, tenantName := range tenants {
		//check propagation status
		Eventually(func() error {
			vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
				ts.model.Testbed())
			Expect(err).ShouldNot(HaveOccurred())
			return vpcc.VerifyPropagationStatus(naplesCount[tenantName])
		}, "5m", "10s").Should(Succeed())

		if *scaleFlag {
			continue
		}

		vpcc, err := objects.TenantVPCCollection(tenantName, ts.model.ConfigClient(),
			ts.model.Testbed())
		Expect(err).ShouldNot(HaveOccurred())
		verifyNetAgentVpcState(vpcc)
		verifyPDSVpcState(vpcc)
	}
}

func defaultTenantName() (string, error) {
	ten, err := ts.model.ConfigClient().ListTenant()
	if err != nil {
		return "", err
	}

	if len(ten) == 0 {
		return "", fmt.Errorf("not enough tenants to list")
	}

	//select first tenant ; there is only one tenant right now in iota
	return ten[0].Name, nil

}
func getVpcCollection(tenantName string) (*objects.VpcObjCollection, error) {
	if tenantName == "" {
		ten, err := ts.model.ConfigClient().ListTenant()
		if err != nil {
			return nil, err
		}

		if len(ten) == 0 {
			return nil, fmt.Errorf("not enough tenants to list networks")
		}

		//select first tenant
		tenantName = ten[0].Name
	}

	vpcList, err := ts.model.ConfigClient().ListVPC(tenantName)
	if err != nil {
		return nil, err
	}
	if len(vpcList) == 0 {
		return nil, fmt.Errorf("no VPCs in tenant %s", tenantName)
	}
	voc := objects.NewVPCCollection(ts.model.ConfigClient(), ts.model.Testbed())
	for _, v := range vpcList {
		if v.Spec.Type == "tenant" {
			voc.AddVPC(&objects.Vpc{v})
		}
	}
	return voc, nil
}

//////////////////////////////////////////////////////////
type VpcDef struct {
	TypeMeta string `yaml:"typemeta"`
	ObjMeta  string `yaml:"objmeta"`
	Spec     struct {
		Id          []byte `yaml:"id"`
		VPCType     int32  `yaml:"type"`
		RouterMac   uint64 `yaml:"virtualroutermac"`
		FabricEncap struct {
			Type  int32 `yaml:"type"`
			Value struct {
				Val struct {
					Vnid uint32 `yaml:"vnid"`
				} `yaml:"val"`
			} `yaml:"value"`
		} `yaml:"fabricencap"`
	} `yaml:"spec"`
	Status struct {
		HwId uint32 `yaml:"hwid"`
	} `yaml:"status"`
	Stats struct {
	} `yaml:"stats"`
}

type pdsVpc struct {
	spec struct {
		uuid    string
		vpcType string
		rmac    string
		vni     uint32
	}
}

func normalizeVeniceVpcObj(vpcs *objects.VpcObjCollection) map[string]pdsVpc {
	cfg := make(map[string]pdsVpc)

	for _, v := range vpcs.Objs {
		var p pdsVpc
		p.spec.uuid = v.Obj.GetUUID()
		p.spec.vpcType = strings.ToLower(v.Obj.Spec.GetType())
		p.spec.rmac = v.Obj.Spec.GetRouterMACAddress()
		p.spec.vni = v.Obj.Spec.GetVxLanVNI()
		cfg[p.spec.uuid] = p
	}
	return cfg
}

func macNumtoStr(macNum uint64) string {
	macByte := make([]byte, 8)
	binary.BigEndian.PutUint64(macByte, macNum)
	mac := net.HardwareAddr(macByte)
	macStr := strings.Split(mac.String()[6:], ":")
	return fmt.Sprintf("%s.%s.%s", macStr[0]+macStr[1], macStr[2]+macStr[3], macStr[4]+macStr[5])
}

func normalizePDSVpcObj(vpcs []*objects.Vpc, node *objects.Naples, isHWNode bool) map[string]pdsVpc {

	m := make(map[string]pdsVpc)

	for _, s := range vpcs {
		vpcUuid := s.Obj.GetUUID()

		naples := []*objects.Naples{node}
		var cmdOut []string
		if isHWNode {
			cmd := fmt.Sprintf("/nic/bin/pdsctl show vpc --id %s --yaml", vpcUuid)
			cmdOut, _ = ts.model.RunNaplesCommand(&objects.NaplesCollection{Nodes: naples}, cmd)
		} else {
			cmd := fmt.Sprintf("/naples/nic/bin/pdsctl show vpc --id %s --yaml", vpcUuid)
			cmdOut, _ = ts.model.RunFakeNaplesCommand(&objects.NaplesCollection{FakeNodes: naples}, cmd)
		}
		var data VpcDef
		err := yaml.Unmarshal([]byte(cmdOut[0]), &data)
		if err != nil {
			return nil
		}
		id, _ := uuid.FromBytes(data.Spec.Id)

		var p pdsVpc
		p.spec.uuid = id.String()

		switch data.Spec.VPCType {
		case 1:
			p.spec.vpcType = "infra"
		case 2:
			p.spec.vpcType = "tenant"
		}

		p.spec.rmac = macNumtoStr(data.Spec.RouterMac)
		p.spec.vni = data.Spec.FabricEncap.Value.Val.Vnid

		m[p.spec.uuid] = p
	}
	return m
}

func verifyPDSVpcState(vc *objects.VpcObjCollection) {
	veniceVpc := normalizeVeniceVpcObj(vc)

	tenant, err := defaultTenantName()
	ExpectWithOffset(1, err).ShouldNot(HaveOccurred())

	npc, err := ts.model.Naples().SelectByTenant(tenant)
	ExpectWithOffset(1, err).ShouldNot(HaveOccurred())

	nodes := npc.FakeNodes
	for _, node := range nodes {
		EventuallyWithOffset(1, func() map[string]pdsVpc {
			log.Infof("DSC %v : Trying to verify VPC state in PDS agent...", node.Name())
			return normalizePDSVpcObj(vc.Objs, node, false)
		}).Should(Equal(veniceVpc))
	}

	nodes = npc.Nodes
	for _, node := range nodes {
		EventuallyWithOffset(1, func() map[string]pdsVpc {
			log.Infof("DSC %v : Trying to verify VPC statein PDS agent...", node.IP())
			return normalizePDSVpcObj(vc.Objs, node, true)
		}).Should(Equal(veniceVpc))
	}
}

//////////////////////////////////////////////////////////
//Netagent verification helpers
type netAgentVpc struct {
	meta struct {
		name   string
		tenant string
		uuid   string
	}
	spec struct {
		rmac              string
		vni               uint32
		routeImportExport string
		ipamPolicy        string
	}
}

func ConvertVeniceRdToNARd(rdSpec *network.RDSpec) *netproto.RDSpec {
	// Note that this is needed because the venice proto for RDSpec
	// and netagent proto for RDSpec are slightly different.
	netAgentRDSpec := netproto.RDSpec{
		AddressFamily: rdSpec.AddressFamily,
		RDAuto:        rdSpec.RDAuto,
	}

	if rdSpec.RD != nil {
		netAgentRDSpec.RD = &netproto.RouteDistinguisher{
			Type:          rdSpec.RD.Type,
			AdminValue:    rdSpec.RD.AdminValue.Value,
			AssignedValue: rdSpec.RD.AssignedValue,
		}
	}
	for _, exportRT := range rdSpec.ExportRTs {
		netagentExportRT := netproto.RouteDistinguisher{
			Type:          exportRT.Type,
			AdminValue:    exportRT.AdminValue.Value,
			AssignedValue: exportRT.AssignedValue,
		}
		netAgentRDSpec.ExportRTs = append(netAgentRDSpec.ExportRTs, &netagentExportRT)
	}
	for _, importRT := range rdSpec.ImportRTs {
		netagentImportRT := netproto.RouteDistinguisher{
			Type:          importRT.Type,
			AdminValue:    importRT.AdminValue.Value,
			AssignedValue: importRT.AssignedValue,
		}
		netAgentRDSpec.ImportRTs = append(netAgentRDSpec.ImportRTs, &netagentImportRT)
	}

	return &netAgentRDSpec
}

func normalizeVeniceVpcForNA(vpcs *objects.VpcObjCollection) map[string]netAgentVpc {
	cfg := make(map[string]netAgentVpc)

	for _, v := range vpcs.Objs {
		var data netAgentVpc
		data.meta.name = v.Obj.GetName()
		data.meta.tenant = v.Obj.GetTenant()
		data.meta.uuid = v.Obj.GetUUID()

		data.spec.rmac = v.Obj.Spec.GetRouterMACAddress()
		data.spec.vni = v.Obj.Spec.GetVxLanVNI()
		data.spec.routeImportExport = ConvertVeniceRdToNARd(v.Obj.Spec.RouteImportExport).String()
		data.spec.ipamPolicy = v.Obj.Spec.GetDefaultIPAMPolicy()
		cfg[data.meta.uuid] = data
	}
	return cfg
}

func normalizeNAVpcObj(vpcs []*objects.Vpc, node *objects.Naples, isHWNode bool) map[string]netAgentVpc {

	m := make(map[string]netAgentVpc)

	naples := []*objects.Naples{node}
	var cmdOut []string
	cmd := "curl localhost:9007/api/vrfs/"
	if isHWNode {
		cmdOut, _ = ts.model.RunNaplesCommand(&objects.NaplesCollection{Nodes: naples}, cmd)
	} else {
		cmdOut, _ = ts.model.RunFakeNaplesCommand(&objects.NaplesCollection{FakeNodes: naples}, cmd)
	}

	var vrfData []netproto.Vrf
	err := json.Unmarshal([]byte(cmdOut[0]), &vrfData)
	if err != nil {
		return nil
	}

	for _, vpc := range vpcs {
		name := vpc.Obj.GetName()
		for _, datum := range vrfData {
			if name != datum.GetName() {
				continue
			}
			var n netAgentVpc
			n.meta.name = datum.GetName()
			n.meta.tenant = datum.GetTenant()
			n.meta.uuid = datum.GetUUID()

			n.spec.rmac = datum.Spec.GetRouterMAC()
			n.spec.vni = datum.Spec.GetVxLANVNI()
			n.spec.routeImportExport = datum.Spec.RouteImportExport.String()
			n.spec.ipamPolicy = datum.Spec.GetIPAMPolicy()

			m[n.meta.uuid] = n
		}
	}

	return m
}

func verifyNetAgentVpcState(vpcs *objects.VpcObjCollection) {
	veniceVpc := normalizeVeniceVpcForNA(vpcs)

	tenant, err := defaultTenantName()
	ExpectWithOffset(1, err).ShouldNot(HaveOccurred())

	npc, err := ts.model.Naples().SelectByTenant(tenant)
	ExpectWithOffset(1, err).ShouldNot(HaveOccurred())

	nodes := npc.FakeNodes
	for _, node := range nodes {
		EventuallyWithOffset(1, func() map[string]netAgentVpc {
			log.Infof("DSC %v : Trying to verify VPC state in Net agent...", node.Name())
			return normalizeNAVpcObj(vpcs.Objs, node, false)
		}).Should(Equal(veniceVpc))
	}

	nodes = npc.Nodes
	for _, node := range nodes {
		EventuallyWithOffset(1, func() map[string]netAgentVpc {
			log.Infof("DSC %v : Trying to verify VPC state in Net agent...", node.IP())
			return normalizeNAVpcObj(vpcs.Objs, node, true)
		}).Should(Equal(veniceVpc))
	}
}
