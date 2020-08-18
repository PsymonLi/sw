package equinix_test

import (
	"encoding/binary"
	"encoding/json"
	"fmt"
	"net"
	"strconv"
	"strings"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/base"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/objects"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
	uuid "github.com/satori/go.uuid"
	yaml "gopkg.in/yaml.v2"
)

var (
	savedNetwork = make(map[string]string)
)

var _ = Describe("Network", func() {
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

	Context("Network tests", func() {
		It("Add a subnet & verify config", func() {
			nwIfs := make([]*objects.NetworkInterfaceCollection, len(defaultTenants))
			var (
				vpcs     []*objects.VpcObjCollection
				networks []*objects.NetworkCollection
			)

			nwNames := make([]string, len(defaultTenants))

			for i, tenantName := range defaultTenants {
				iStr := strconv.Itoa(i)

				//Create VPC config
				vpcName := "testVPC" + iStr
				vpcVni := uint32(700 + i)

				vpc := ts.model.NewVPC(tenantName, vpcName, getUniqueMac(i+1), vpcVni, "")
				Expect(vpc.Commit()).Should(Succeed())

				vpcs = append(vpcs, vpc)

				//Validate using Get command
				veniceVpc, err := ts.model.GetVPC(vpcName, tenantName)
				Expect(err).ShouldNot(HaveOccurred())
				Expect(veniceVpc.Obj.Name == vpcName).Should(BeTrue())

				//Create a subnet
				nwNames[i] = "testNetwork" + iStr
				nwVni := 0x80000 | vpcVni
				log.Infof("NW Vni %v", nwVni)
				nwp := &base.NetworkParams{
					nwNames[i],
					fmt.Sprintf("10.1.%v.0/24", i),
					fmt.Sprintf("10.1.%v.1", i),
					nwVni,
					vpcName,
					tenantName,
				}
				nwc := ts.model.NewNetwork(nwp)
				Expect(nwc.Commit()).Should(Succeed())

				networks = append(networks, nwc)

				//Verify network config in Venice
				nw, err := ts.model.GetNetwork(tenantName, nwNames[i])
				Expect(err).ShouldNot(HaveOccurred())
				Expect(nw.VeniceNetwork.Name == nwNames[i]).Should(BeTrue())

				// get all host nw interfaces
				nwIfs[i] = AttachNwInterfaceToSubnet(tenantName, nwNames[i])
			}

			for _, tenantName := range defaultTenants {
				//Verify propagation status
				Eventually(func() error {
					return ts.model.Networks(tenantName).VerifyPropagationStatus(naplesCount[tenantName])
				}).Should(Succeed())

				if *scaleFlag {
					continue
				}
				nwList := ts.model.Networks(tenantName)
				verifyNetAgentNwState(nwList)
				verifyPDSNwState(nwList)
			}

			for i, tenantName := range defaultTenants {
				// detach the interface to the subnet
				DetachNwInterfaceFromSubnet(tenantName, nwNames[i], nwIfs[i])
				Expect(networks[i].Delete()).Should(Succeed())
				Expect(vpcs[i].Delete()).Should(Succeed())
			}

			for _, tenantName := range defaultTenants {
				//Verify propagation status
				Eventually(func() error {
					return ts.model.Networks(tenantName).VerifyPropagationStatus(naplesCount[tenantName])
				}).Should(Succeed())

				if *scaleFlag {
					continue
				}
				nwList := ts.model.Networks(tenantName)
				verifyNetAgentNwState(nwList)
				verifyPDSNwState(nwList)
			}
		})

		It("Change subnet prefix len & verify config", func() {
			nwcc := make([]*objects.NetworkCollection, len(defaultTenants))

			oldPrefix := make([][]string, len(defaultTenants))

			for i, tenantName := range defaultTenants {
				//Get a network from default config and modify its prefix len
				nwcc[i] = ts.model.Networks(tenantName)

				oldPrefix[i] = make([]string, len(nwcc[i].Subnets()))

				for j, nw := range nwcc[i].Subnets() {
					oldPrefix[i][j] = nw.VeniceNetwork.Spec.IPv4Subnet

					ip := strings.Split(oldPrefix[i][j], "/")
					//Get IP addr without the prefix length
					ipPrefix := strings.Split(ip[0], ".")

					//default config has prefix len 16
					newPrefix := fmt.Sprintf("%v.%v.%v.0/24", ipPrefix[0], ipPrefix[1], ipPrefix[2])

					log.Infof("New prefix %v old prefix %v", newPrefix, oldPrefix[i][j])

					nw.UpdateIPv4Subnet(newPrefix)
				}

				Expect(nwcc[i].Commit()).Should(Succeed())
			}

			for _, tenantName := range defaultTenants {
				//Verify propagation status
				Eventually(func() error {
					return ts.model.Networks(tenantName).VerifyPropagationStatus(naplesCount[tenantName])
				}).Should(Succeed())

				if *scaleFlag {
					continue
				}
				nwList := ts.model.Networks(tenantName)
				verifyNetAgentNwState(nwList)
				verifyPDSNwState(nwList)
			}

			//Revert to original
			for i, nwc := range nwcc {
				for j, nw := range nwc.Subnets() {
					nw.UpdateIPv4Subnet(oldPrefix[i][j])
				}
				Expect(nwc.Commit()).Should(Succeed())
			}

			for _, tenantName := range defaultTenants {
				//Verify propagation status
				Eventually(func() error {
					return ts.model.Networks(tenantName).VerifyPropagationStatus(naplesCount[tenantName])
				}).Should(Succeed())

				if *scaleFlag {
					continue
				}
				nwList := ts.model.Networks(tenantName)
				verifyNetAgentNwState(nwList)
				verifyPDSNwState(nwList)
			}
		})

		It("Change Gateway IP & verify config", func() {

			nwcc := make([]*objects.NetworkCollection, len(defaultTenants))

			oldGw := make([][]string, len(defaultTenants))

			for i, tenantName := range defaultTenants {
				//Get a network from default config and modify its prefix len
				nwcc[i] = ts.model.Networks(tenantName)

				oldGw[i] = make([]string, len(nwcc[i].Subnets()))

				for j, nw := range nwcc[i].Subnets() {
					oldGw[i][j] = nw.VeniceNetwork.Spec.GetIPv4Gateway()

					ipBytes := strings.Split(oldGw[i][j], ".")
					newGw := fmt.Sprintf("%s.%s.%s.250", ipBytes[0], ipBytes[1], ipBytes[2])

					nw.UpdateIPv4Gateway(newGw)
					log.Infof("Gateway IP change from %s to %s", oldGw[i][j], newGw)
				}

				Expect(nwcc[i].Commit()).Should(Succeed())
			}

			for _, tenantName := range defaultTenants {
				//Verify propagation status
				Eventually(func() error {
					return ts.model.Networks(tenantName).VerifyPropagationStatus(naplesCount[tenantName])
				}).Should(Succeed())

				if *scaleFlag {
					continue
				}
				nwList := ts.model.Networks(tenantName)
				verifyNetAgentNwState(nwList)
				verifyPDSNwState(nwList)
			}

			//Revert to original
			for i, nwc := range nwcc {
				for j, nw := range nwc.Subnets() {
					nw.UpdateIPv4Gateway(oldGw[i][j])
				}
				Expect(nwc.Commit()).Should(Succeed())
			}

			for _, tenantName := range defaultTenants {
				//Verify propagation status
				Eventually(func() error {
					return ts.model.Networks(tenantName).VerifyPropagationStatus(naplesCount[tenantName])
				}).Should(Succeed())

				if *scaleFlag {
					continue
				}
				nwList := ts.model.Networks(tenantName)
				verifyNetAgentNwState(nwList)
				verifyPDSNwState(nwList)
			}
		})

		It("Change Vni & verify it's not allowed", func() {
			//Get existing tenant and network
			nwc, err := getNetworkCollection()
			Expect(err).ShouldNot(HaveOccurred())

			nwc = nwc.Any(1)
			nw := nwc.Subnets()[0]

			tenantName := nw.VeniceNetwork.Tenant
			nwName := nw.VeniceNetwork.Name

			//update Vni
			snc := nwc.Subnets()
			nwVni := snc[0].VeniceNetwork.Spec.VxlanVNI
			tempVni := nwVni + 10
			snc[0].VeniceNetwork.Spec.VxlanVNI = tempVni
			log.Infof("Trying Vni change from %v to %v", nwVni, tempVni)
			Expect(nwc.Commit()).ShouldNot(Succeed())

			nw, err = ts.model.GetNetwork(tenantName, nwName)
			Expect(err).ShouldNot(HaveOccurred())
			Expect(nw.VeniceNetwork.Spec.GetVxlanVNI() == nwVni).Should(BeTrue())

			nwList := ts.model.Networks(tenantName)
			verifyNetAgentNwState(nwList)
			verifyPDSNwState(nwList)
		})

		It("Change VPC for subnet & verify it's not allowed", func() {
			//Get existing tenant and network
			nwc, err := getNetworkCollection()
			Expect(err).ShouldNot(HaveOccurred())

			nwc = nwc.Any(1)
			nw := nwc.Subnets()[0]

			tenantName := nw.VeniceNetwork.Tenant
			nwName := nw.VeniceNetwork.Name

			tempVpcName := "testVPC"
			tempVpc := ts.model.NewVPC(tenantName, tempVpcName, "0001.0102.0202", uint32(700), "")
			Expect(tempVpc.Commit()).Should(Succeed())

			snc := nwc.Subnets()
			vpc := snc[0].VeniceNetwork.Spec.VirtualRouter
			snc[0].VeniceNetwork.Spec.VirtualRouter = tempVpcName
			log.Infof("Trying to change subnet VPC from %s to %s", vpc, tempVpcName)
			Expect(nwc.Commit()).ShouldNot(Succeed())

			nw, err = ts.model.GetNetwork(tenantName, nwName)
			Expect(err).ShouldNot(HaveOccurred())
			Expect(nw.VeniceNetwork.Spec.GetVirtualRouter() == vpc).Should(BeTrue())

			nwList := ts.model.Networks(tenantName)
			verifyNetAgentNwState(nwList)
			verifyPDSNwState(nwList)

			Expect(tempVpc.Delete()).Should(Succeed())
		})

		It("Change RT & verify config", func() {
			offset := uint32(10)

			for _, tenantName := range defaultTenants {
				nwc := ts.model.Networks(tenantName)

				type rtVal struct {
					exportRtval, importRtVal uint32
				}
				newRtVal := make(map[string]rtVal)
				for _, nw := range nwc.Subnets() {
					exportRTs := nw.VeniceNetwork.Spec.RouteImportExport.ExportRTs
					importRTs := nw.VeniceNetwork.Spec.RouteImportExport.ImportRTs
					//Update RT value
					exportRTs[0].AssignedValue += offset
					importRTs[0].AssignedValue += offset
					newRtVal[nw.VeniceNetwork.GetName()] = rtVal{exportRtval: exportRTs[0].AssignedValue,
						importRtVal: importRTs[0].AssignedValue}
				}

				Expect(nwc.Commit()).Should(Succeed())

				if *scaleFlag {
					continue
				}

				nwc = ts.model.Networks(tenantName)
				for _, nw := range nwc.Subnets() {
					exportAssignedVal := nw.VeniceNetwork.Spec.RouteImportExport.ExportRTs[0].GetAssignedValue()
					importAssignedVal := nw.VeniceNetwork.Spec.RouteImportExport.ImportRTs[0].GetAssignedValue()

					nwName := nw.VeniceNetwork.GetName()

					Expect(newRtVal).Should(HaveKeyWithValue(nwName, rtVal{exportRtval: exportAssignedVal,
						importRtVal: importAssignedVal}))
				}
			}

			//Verify propagation status
			for _, tenantName := range defaultTenants {
				//Verify propagation status
				Eventually(func() error {
					return ts.model.Networks(tenantName).VerifyPropagationStatus(naplesCount[tenantName])
				}).Should(Succeed())

				if *scaleFlag {
					continue
				}
				nwList := ts.model.Networks(tenantName)
				verifyNetAgentNwState(nwList)
				verifyPDSNwState(nwList)
			}

			for _, tenantName := range defaultTenants {
				nwc := ts.model.Networks(tenantName)
				for _, nw := range nwc.Subnets() {
					//Restore RT value
					exportRTs := nw.VeniceNetwork.Spec.RouteImportExport.ExportRTs
					importRTs := nw.VeniceNetwork.Spec.RouteImportExport.ImportRTs
					exportRTs[0].AssignedValue -= offset
					importRTs[0].AssignedValue -= offset
				}
				Expect(nwc.Commit()).Should(Succeed())
			}

			for _, tenantName := range defaultTenants {
				//Verify propagation status
				Eventually(func() error {
					return ts.model.Networks(tenantName).VerifyPropagationStatus(naplesCount[tenantName])
				}).Should(Succeed())

				if *scaleFlag {
					continue
				}
				nwList := ts.model.Networks(tenantName)
				verifyNetAgentNwState(nwList)
				verifyPDSNwState(nwList)
			}
		})

	})
})

func AttachNwInterfaceToSubnet(tenant, nw string) *objects.NetworkInterfaceCollection {
	// get all host nw interfaces
	//filter := fmt.Sprintf("spec.type=host-pf")
	//nwIfs, err := ts.model.ListNetworkInterfacesByFilter(filter)
	nwIfs, err := objects.GetAllPFNetworkInterfacesForTenant(tenant,
		ts.model.ConfigClient(), ts.model.Testbed())
	ExpectWithOffset(1, err).ShouldNot(HaveOccurred())
	ExpectWithOffset(1, nwIfs).ShouldNot(BeNil())
	ExpectWithOffset(1, len(nwIfs.Interfaces) != 0).Should(BeTrue())

	//First save existing network to revert to
	savedNetwork[tenant] = nwIfs.Interfaces[0].Spec.AttachNetwork
	// attach the first interface to the subnet
	nwIfs.Interfaces[0].Spec.AttachNetwork = nw
	log.Infof("Updating nwif: %s | spec: %v | status: %v",
		nwIfs.Interfaces[0].Name, nwIfs.Interfaces[0].Spec, nwIfs.Interfaces[0].Status)
	ExpectWithOffset(1, nwIfs.Commit()).Should(Succeed())
	return nwIfs
}

func DetachNwInterfaceFromSubnet(tenant, nw string, nwIfs *objects.NetworkInterfaceCollection) {

	for _, n := range nwIfs.Interfaces {
		if n.Spec.AttachNetwork == nw && n.Spec.AttachTenant == tenant {
			n.Spec.AttachNetwork = savedNetwork[tenant]
			break
		}
	}
	Expect(nwIfs.Commit()).Should(Succeed())
}

type pdsSubnet struct {
	spec struct {
		uuid string
		//vpc  string
		ip  string
		gw  string
		vni uint32
	}
}

func normalizeVeniceNetworkObj(obj *objects.NetworkCollection) map[string]pdsSubnet {
	cfg := make(map[string]pdsSubnet)

	for _, s := range obj.Subnets() {
		var sn pdsSubnet
		sn.spec.uuid = s.VeniceNetwork.GetUUID()
		sn.spec.ip = s.VeniceNetwork.Spec.GetIPv4Subnet()
		sn.spec.gw = s.VeniceNetwork.Spec.GetIPv4Gateway()
		sn.spec.vni = s.VeniceNetwork.Spec.GetVxlanVNI()
		cfg[sn.spec.uuid] = sn
	}
	return cfg
}

func ipNumtoStr(ipLong uint32) string {
	ipByte := make([]byte, 4)
	binary.BigEndian.PutUint32(ipByte, ipLong)
	ip := net.IP(ipByte)
	return ip.String()
}

func normalizePDSSubnetObj(subnets []*objects.Network, node *objects.Naples, isHWNode bool) map[string]pdsSubnet {

	m := make(map[string]pdsSubnet)

	for _, s := range subnets {
		snUuid := s.VeniceNetwork.GetUUID()
		naples := []*objects.Naples{node}
		var cmdOut []string
		if isHWNode {
			cmd := fmt.Sprintf("/nic/bin/pdsctl show subnet --id %s --yaml", snUuid)
			cmdOut, _ = ts.model.RunNaplesCommand(&objects.NaplesCollection{Nodes: naples}, cmd)
		} else {
			cmd := fmt.Sprintf("/naples/nic/bin/pdsctl show subnet --id %s --yaml", snUuid)
			cmdOut, _ = ts.model.RunFakeNaplesCommand(&objects.NaplesCollection{FakeNodes: naples}, cmd)
		}
		var data Subnet
		err := yaml.Unmarshal([]byte(cmdOut[0]), &data)
		if err != nil {
			return nil
		}
		id, _ := uuid.FromBytes(data.Spec.Id)

		var p pdsSubnet
		p.spec.uuid = id.String()

		gwStr := ipNumtoStr(data.Spec.IPV4prefix.Addr)
		prefixLen := data.Spec.IPV4prefix.Len
		ipv4Addr := net.ParseIP(gwStr)
		ipv4Mask := net.CIDRMask(int(prefixLen), 32)

		p.spec.ip = fmt.Sprintf("%s/%v", ipv4Addr.Mask(ipv4Mask), data.Spec.IPV4prefix.Len)
		//p.spec.ip = fmt.Sprintf("%s/%v", ipNumtoStr(data.Spec.IPV4prefix.Addr), data.Spec.IPV4prefix.Len)
		p.spec.gw = ipNumtoStr(data.Spec.IPv4Gateway)
		p.spec.vni = data.Spec.FabricEncap.Value.Val.Vnid

		m[p.spec.uuid] = p
	}

	return m
}

func verifyPDSNwState(obj *objects.NetworkCollection) {
	vSub := normalizeVeniceNetworkObj(obj)

	tenant, err := defaultTenantName()
	ExpectWithOffset(1, err).ShouldNot(HaveOccurred())

	npc, err := ts.model.Naples().SelectByTenant(tenant)
	ExpectWithOffset(1, err).ShouldNot(HaveOccurred())

	nodes := npc.FakeNodes
	for _, node := range nodes {
		EventuallyWithOffset(1, func() map[string]pdsSubnet {
			log.Infof("DSC %v : Trying to verify networks state in PDS agent...", node.Name())
			return normalizePDSSubnetObj(obj.Subnets(), node, false)
		}).Should(Equal(vSub))
	}

	nodes = npc.Nodes
	for _, node := range nodes {
		EventuallyWithOffset(1, func() map[string]pdsSubnet {
			log.Infof("DSC %v : Trying to verify networks state in PDS agent...", node.IP())
			return normalizePDSSubnetObj(obj.Subnets(), node, true)
		}).Should(Equal(vSub))
	}
}

//Netagent verification helpers
type netAgentSubnet struct {
	meta struct {
		name   string
		tenant string
		uuid   string
	}
	spec struct {
		vpc               string
		ip                string
		gw                string
		vni               uint32
		routeImportExport string
		ipamPolicy        string
	}
}

func normalizeVeniceForNA(obj *objects.NetworkCollection) map[string]netAgentSubnet {
	cfg := make(map[string]netAgentSubnet)

	for _, s := range obj.Subnets() {
		var sn netAgentSubnet
		sn.meta.name = s.VeniceNetwork.GetName()
		sn.meta.tenant = s.VeniceNetwork.GetTenant()
		sn.meta.uuid = s.VeniceNetwork.GetUUID()

		sn.spec.vpc = s.VeniceNetwork.Spec.GetVirtualRouter()
		sn.spec.ip = s.VeniceNetwork.Spec.GetIPv4Subnet()
		sn.spec.gw = s.VeniceNetwork.Spec.GetIPv4Gateway()
		sn.spec.vni = s.VeniceNetwork.Spec.GetVxlanVNI()
		sn.spec.routeImportExport = ConvertVeniceRdToNARd(s.VeniceNetwork.Spec.RouteImportExport).String()
		sn.spec.ipamPolicy = s.VeniceNetwork.Spec.GetIPAMPolicy()

		cfg[sn.meta.uuid] = sn
	}
	return cfg
}

func normalizeNASubnetObj(subnets []*objects.Network, node *objects.Naples, isHWNode bool) map[string]netAgentSubnet {

	var err error
	m := make(map[string]netAgentSubnet)

	naples := []*objects.Naples{node}
	var cmdOut []string
	cmd := "curl localhost:9007/api/networks/"
	if isHWNode {
		cmdOut, err = ts.model.RunNaplesCommand(&objects.NaplesCollection{Nodes: naples}, cmd)
	} else {
		cmdOut, err = ts.model.RunFakeNaplesCommand(&objects.NaplesCollection{FakeNodes: naples}, cmd)
	}

	if err != nil {
		log.Infof("NetAgent command returned err %s", err.Error())
		return nil
	}

	var nwData []netproto.Network
	err = json.Unmarshal([]byte(cmdOut[0]), &nwData)
	if err != nil {
		return nil
	}

	for _, s := range subnets {
		name := s.VeniceNetwork.GetName()
		for _, datum := range nwData {
			if name != datum.GetName() {
				continue
			}
			var n netAgentSubnet
			n.meta.name = datum.GetName()
			n.meta.tenant = datum.GetTenant()
			n.meta.uuid = datum.GetUUID()

			n.spec.vpc = datum.Spec.GetVrfName()

			//In netagent, o/p is gw_ip/prefix
			gwStr := ipNumtoStr(datum.Spec.GetV4Address()[0].Address.GetV4Address())
			prefixLen := datum.Spec.GetV4Address()[0].GetPrefixLen()
			ipv4Addr := net.ParseIP(gwStr)
			ipv4Mask := net.CIDRMask(int(prefixLen), 32)

			n.spec.ip = fmt.Sprintf("%s/%v", ipv4Addr.Mask(ipv4Mask), prefixLen)
			n.spec.gw = gwStr
			n.spec.vni = datum.Spec.GetVxLANVNI()
			n.spec.routeImportExport = datum.Spec.RouteImportExport.String()
			n.spec.ipamPolicy = datum.Spec.GetIPAMPolicy()

			m[n.meta.uuid] = n
		}
	}

	return m
}

func verifyNetAgentNwState(obj *objects.NetworkCollection) {
	vSub := normalizeVeniceForNA(obj)

	tenant, err := defaultTenantName()
	ExpectWithOffset(1, err).ShouldNot(HaveOccurred())

	npc, err := ts.model.Naples().SelectByTenant(tenant)
	ExpectWithOffset(1, err).ShouldNot(HaveOccurred())

	nodes := npc.FakeNodes
	for _, node := range nodes {
		EventuallyWithOffset(1, func() map[string]netAgentSubnet {
			log.Infof("DSC %v : Trying to verify networks state in Net agent...",
				node.Name())
			return normalizeNASubnetObj(obj.Subnets(), node, false)
		}).Should(Equal(vSub))
	}

	nodes = npc.Nodes
	for _, node := range nodes {
		EventuallyWithOffset(1, func() map[string]netAgentSubnet {
			log.Infof("DSC IP %v : Trying to verify networks state in Net agent...",
				node.IP())
			return normalizeNASubnetObj(obj.Subnets(), node, true)
		}).Should(Equal(vSub))
	}
}
