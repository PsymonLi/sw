package equinix_test

import (
	"encoding/json"
	"fmt"
	"strconv"
	"strings"

	mapset "github.com/deckarep/golang-set"
	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
	iota "github.com/pensando/sw/iota/protos/gogen"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/base"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/objects"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/log"
	uuid "github.com/satori/go.uuid"
	yaml "gopkg.in/yaml.v2"
)

var _ = Describe("Interface tests", func() {
	BeforeEach(func() {
		// verify cluster is in good health
		Eventually(func() error {
			return ts.model.VerifyClusterStatus()
		}).Should(Succeed())
	})
	AfterEach(func() {
	})

	Context("Interface tests", func() {

		It("Update Loopback IP", func() {
			fakeNaples := !ts.tb.HasNaplesHW()

			lbc := objects.GetLoopbacks(ts.model.ConfigClient(), ts.model.Testbed())
			Expect(lbc.Error()).ShouldNot(HaveOccurred())

			old := objects.GetLoopbacks(ts.model.ConfigClient(), ts.model.Testbed()) //For restoring
			Expect(old.Error()).ShouldNot(HaveOccurred())

			for i, intf := range lbc.Interfaces {
				oldIP := strings.Split(intf.Spec.IPConfig.IPAddress, "/")[0]
				ipBytes := strings.Split(oldIP, ".")
				lastByte, _ := strconv.Atoi(ipBytes[3])
				newLoopbackIP := fmt.Sprintf("%s.%s.%s.%v/32", ipBytes[0], ipBytes[1], ipBytes[2],
					(lastByte+1)%256)
				if fakeNaples {
					newLoopbackIP = fmt.Sprintf("10.8.250.%v/32", i+1)
				}
				intf.Spec.IPAllocType = "static"
				log.Infof("New Loopback ip : %s", newLoopbackIP)
				intf.Spec.IPConfig.IPAddress = newLoopbackIP
			}

			Expect(lbc.Commit()).Should(Succeed())

			//Verify in Venice
			Expect(verifyVeniceIntf(lbc)).Should(Succeed())

			Eventually(func() error {
				//verify api/interfaces/ in netagent
				return verifyNetAgentIntf(lbc)
			}).Should(Succeed())

			Eventually(func() error {
				//verify routerID has changed in pdsctl
				return verifyPDSAgentIntf(lbc)
			}).Should(Succeed())

			//reset back to original loopbackIP
			Expect(old.Commit()).Should(Succeed())
		})

		It("Remove Loopback IP", func() {
			lbc := objects.GetLoopbacks(ts.model.ConfigClient(), ts.model.Testbed())
			Expect(lbc.Error()).ShouldNot(HaveOccurred())

			old := objects.GetLoopbacks(ts.model.ConfigClient(), ts.model.Testbed()) //For restoring
			Expect(old.Error()).ShouldNot(HaveOccurred())

			for _, intf := range lbc.Interfaces {
				newLoopbackIP := ""
				intf.Spec.IPAllocType = "static"
				log.Infof("New Loopback ip : %s", newLoopbackIP)
				intf.Spec.IPConfig.IPAddress = newLoopbackIP
			}

			Expect(lbc.Commit()).Should(Succeed())

			//Verify in Venice
			Expect(verifyVeniceIntf(lbc)).Should(Succeed())

			Eventually(func() error {
				//verify api/interfaces/ in netagent
				return verifyNetAgentIntf(lbc)
			}).Should(Succeed())

			//reset back to original loopbackIP
			Expect(old.Commit()).Should(Succeed())

			//Verify in Venice
			Expect(verifyVeniceIntf(old)).Should(Succeed())

			Eventually(func() error {
				//verify api/interfaces/ in netagent
				return verifyNetAgentIntf(old)
			}).Should(Succeed())

			Eventually(func() error {
				//verify routerID has changed in pdsctl
				return verifyPDSAgentIntf(lbc)
			}).Should(Succeed())
		})

	})

	Context("Host interface tests", func() {
		It("Remove host intf attachments", func() {
			tenantName, err := defaultTenantName()
			Expect(err).ShouldNot(HaveOccurred())

			var nodes *objects.NaplesCollection
			var nodeName string
			if ts.tb.HasNaplesSim() {
				nodes = ts.model.Naples().AnyFakeNodes(1)
				Expect(nodes.Refresh()).Should(Succeed())
				nodeName = nodes.FakeNodes[0].Instances[0].Dsc.GetName()
			} else {
				nodes = ts.model.Naples().Any(1)
				Expect(nodes.Refresh()).Should(Succeed())
				nodeName = nodes.Nodes[0].Instances[0].Dsc.GetName()
			}

			//list host-pf intfs for a node
			pfc, err := objects.GetPFNetworkInterfacesForTenant(tenantName, nodeName,
				ts.model.ConfigClient(), ts.tb)
			Expect(err).ShouldNot(HaveOccurred())

			//save original tenant/nw attachments
			savedPfc, err := objects.GetPFNetworkInterfacesForTenant(tenantName,
				nodeName, ts.model.ConfigClient(), ts.tb)
			Expect(err).ShouldNot(HaveOccurred())

			//Remove all but one pf attachments and check vpc/subnets of tenant are still there in naples
			for i := 0; i < len(pfc.Interfaces)-1; i++ {
				pf := pfc.Interfaces[i]
				pf.Spec.AttachNetwork = ""
				pf.Spec.AttachTenant = ""
			}
			Expect(pfc.Commit()).Should(Succeed())

			verifyNaplesStateForTenant(nodes, tenantName, true)

			//Remove last subnet attachment
			pfc.Interfaces[len(pfc.Interfaces)-1].Spec.AttachNetwork = ""
			pfc.Interfaces[len(pfc.Interfaces)-1].Spec.AttachTenant = ""
			Expect(pfc.Commit()).Should(Succeed())

			//Verify no vpc and subnet for that tenant exist in netagent and pdsagent
			verifyNaplesStateForTenant(nodes, tenantName, false)

			//Verify vpc and subnet for that tenant exist in other nodes
			otherNodes := ts.model.Naples().Difference(nodes)

			verifyNaplesStateForTenant(otherNodes, tenantName, true)

			//re-attach subnets to intfs
			Expect(savedPfc.Commit()).Should(Succeed())
			//verify venice and naples state match for vpc & subnets on all nodes
			verifyNaplesStateForTenant(ts.model.Naples(), tenantName, true)
		})

		It("Change host intf attachments", func() {
			tenantName, err := defaultTenantName()
			Expect(err).ShouldNot(HaveOccurred())

			var nodes *objects.NaplesCollection
			var nodeName string
			if ts.tb.HasNaplesSim() {
				nodes = ts.model.Naples().AnyFakeNodes(1)
				nodeName = nodes.FakeNodes[0].Instances[0].Dsc.GetName()
			} else {
				nodes = ts.model.Naples().Any(1)
				nodeName = nodes.Nodes[0].Instances[0].Dsc.GetName()
			}

			//list host-pf intfs for a node
			pfc, err := objects.GetPFNetworkInterfacesForTenant(tenantName, nodeName,
				ts.model.ConfigClient(), ts.tb)
			Expect(err).ShouldNot(HaveOccurred())

			//save original tenant/nw attachments
			savedPfc, err := objects.GetPFNetworkInterfacesForTenant(tenantName,
				nodeName, ts.model.ConfigClient(), ts.tb)
			Expect(err).ShouldNot(HaveOccurred())

			//Remove all pf attachments
			for _, pf := range pfc.Interfaces {
				pf.Spec.AttachNetwork = ""
				pf.Spec.AttachTenant = ""
			}
			Expect(pfc.Commit()).Should(Succeed())
			verifyNaplesStateForTenant(nodes, tenantName, false)

			//Create new tenant, VPC, subnet
			newTenantName := "tenant0"
			tenant := ts.model.NewTenant(newTenantName)
			Expect(tenant.Commit()).Should(Succeed())

			//Create VPC config
			vpcName := "testVPC"
			vpcVni := uint32(700)
			vpc := ts.model.NewVPC(newTenantName, vpcName, "0001.0102.0202", vpcVni, "")
			Expect(vpc.Commit()).Should(Succeed())

			//Create a subnet
			nwName := "testNetwork"
			nwVni := 0x80000 | vpcVni
			log.Infof("NW Vni %v", nwVni)
			nwp := &base.NetworkParams{
				nwName,
				"10.1.2.0/24",
				"10.1.2.1",
				nwVni,
				vpcName,
				newTenantName,
			}
			nwc := ts.model.NewNetwork(nwp)
			Expect(nwc.Commit()).Should(Succeed())

			pfc.Interfaces[0].Spec.AttachTenant = newTenantName
			pfc.Interfaces[0].Spec.AttachNetwork = nwName
			Expect(pfc.Commit()).Should(Succeed())

			verifyNaplesStateForTenant(nodes, newTenantName, true)

			//verify n/w & vpc don't exist in other nodes
			otherNodes := ts.model.Naples().Difference(nodes)

			verifyNaplesStateForTenant(otherNodes, newTenantName, false)

			//re-attach pre-existing subnets to intfs
			Expect(savedPfc.Commit()).Should(Succeed())
			//verify venice and naples state match for vpc & subnets on all nodes
			verifyNaplesStateForTenant(ts.model.Naples(), tenantName, true)

			//Revert all config from venice
			Expect(nwc.Delete()).Should(Succeed())
			Expect(vpc.Delete()).Should(Succeed())

			verifyNaplesStateForTenant(nodes, newTenantName, false)

			Expect(tenant.Delete()).Should(Succeed())
		})
	})
})

func GetVpcNamesFromNetAgent(node *objects.Naples, tenant string, isHWNode bool) (mapset.Set, error) {
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
		return nil, nil
	}

	gotVpcSet := mapset.NewSet()
	for _, datum := range vrfData {
		if datum.GetTenant() == tenant {
			gotVpcSet.Add(datum.GetName())
		}
	}

	return gotVpcSet, nil
}

func GetNwNamesFromNetAgent(node *objects.Naples, tenant string, isHWNode bool) (mapset.Set, error) {
	var err error
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
		return nil, err
	}

	var nwData []netproto.Network
	err = json.Unmarshal([]byte(cmdOut[0]), &nwData)
	if err != nil {
		return nil, err
	}

	gotNwSet := mapset.NewSet()
	for _, datum := range nwData {
		if datum.GetTenant() == tenant {
			gotNwSet.Add(datum.GetName())
		}
	}
	return gotNwSet, nil
}

func getVpcUUIDFromPDS(vpcuuid string, node *objects.Naples, isHWNode bool) (string, error) {
	naples := []*objects.Naples{node}
	var cmdOut []string
	var err error
	if isHWNode {
		cmd := fmt.Sprintf("/nic/bin/pdsctl show vpc --id %v --yaml", vpcuuid)
		cmdOut, err = ts.model.RunNaplesCommand(&objects.NaplesCollection{Nodes: naples}, cmd)
	} else {
		cmd := fmt.Sprintf("/naples/nic/bin/pdsctl show vpc --id %v --yaml", vpcuuid)
		cmdOut, err = ts.model.RunFakeNaplesCommand(&objects.NaplesCollection{FakeNodes: naples}, cmd)
	}
	if err != nil {
		return "", err
	}

	var data VpcDef
	err = yaml.Unmarshal([]byte(cmdOut[0]), &data)
	if err != nil {
		return "", err
	}

	if data.Spec.VPCType == 2 {
		id, err := uuid.FromBytes(data.Spec.Id)
		if err != nil {
			return "", err
		}
		return id.String(), nil
	}

	return "", nil
}

func getSubnetUUIDFromPDS(snuuid string, node *objects.Naples, isHWNode bool) (string, error) {
	naples := []*objects.Naples{node}
	var cmdOut []string
	var err error
	if isHWNode {
		cmd := fmt.Sprintf("/nic/bin/pdsctl show subnet --id %v --yaml", snuuid)
		cmdOut, err = ts.model.RunNaplesCommand(&objects.NaplesCollection{Nodes: naples}, cmd)
	} else {
		cmd := fmt.Sprintf("/naples/nic/bin/pdsctl show subnet --id %v --yaml", snuuid)
		cmdOut, err = ts.model.RunFakeNaplesCommand(&objects.NaplesCollection{FakeNodes: naples}, cmd)
	}
	if err != nil {
		return "", err
	}

	var data Subnet
	err = yaml.Unmarshal([]byte(cmdOut[0]), &data)
	if err != nil {
		return "", err
	}

	id, err := uuid.FromBytes(data.Spec.Id)
	if err != nil {
		return "", err
	}
	return id.String(), err
}

func verifyNaplesStateForTenant(naples *objects.NaplesCollection, tenant string, expected bool) {
	var nodes []*objects.Naples
	var isHWNode bool

	if ts.tb.HasNaplesSim() {
		nodes = naples.FakeNodes
		isHWNode = false
	} else {
		nodes = naples.Nodes
		isHWNode = true
	}

	cfgClient := ts.model.ConfigClient()

	for _, node := range nodes {
		//list VPCs for tenant
		vpcs, err := cfgClient.ListVPC(tenant)
		ExpectWithOffset(1, err).ShouldNot(HaveOccurred())

		expVpcSet := mapset.NewSet()
		if expected {
			for _, v := range vpcs {
				expVpcSet.Add(v.GetName())
			}
		}

		EventuallyWithOffset(1, func() (mapset.Set, error) {
			log.Infof("DSC %v , tenant %v: Trying to match VPCs from NetAgent...",
				node.IP(), tenant)
			return GetVpcNamesFromNetAgent(node, tenant, isHWNode)
		}).Should(Equal(expVpcSet))

		for _, v := range vpcs {
			if v.GetName() == globals.DefaultVrf {
				continue
			}
			vpcuuid, _ := getVpcUUIDFromPDS(v.GetUUID(), node, isHWNode)

			if expected {
				ExpectWithOffset(1, vpcuuid).Should(Equal(v.GetUUID()))
			} else {
				ExpectWithOffset(1, vpcuuid).Should(BeEmpty())
			}
		}
	}

	for _, node := range nodes {
		//list subnets for tenant
		nws, err := cfgClient.ListNetwork(tenant)
		ExpectWithOffset(1, err).ShouldNot(HaveOccurred())

		expNwSet := mapset.NewSet()
		if expected {
			for _, nw := range nws {
				expNwSet.Add(nw.GetName())
			}
		}

		EventuallyWithOffset(1, func() (mapset.Set, error) {
			log.Infof("DSC %v, tenant %v: Trying to match networks from NetAgent...",
				node.IP(), tenant)
			return GetNwNamesFromNetAgent(node, tenant, isHWNode)
		}).Should(Equal(expNwSet))

		for _, nw := range nws {
			snuuid, _ := getSubnetUUIDFromPDS(nw.GetUUID(), node, isHWNode)

			if expected {
				ExpectWithOffset(1, snuuid).Should(Equal(nw.GetUUID()))
			} else {
				ExpectWithOffset(1, snuuid).Should(BeEmpty())
			}
		}
	}
}

func verifyVeniceIntf(intf *objects.NetworkInterfaceCollection) error {
	//Get updated venice info
	updc := objects.GetLoopbacks(ts.model.ConfigClient(), ts.model.Testbed())
	if updc.HasError() {
		return updc.Error()
	}

	for _, i := range intf.Interfaces {
		match := false
		for _, d := range updc.Interfaces {
			if i.Spec.IPConfig.GetIPAddress() == d.Spec.IPConfig.GetIPAddress() {
				match = true
				break
			}
		}
		if !match {
			return fmt.Errorf("loopback ip %s not found", i.Spec.IPConfig.GetIPAddress())
		}
	}

	return nil
}

func verifyNetAgentIntf(intf *objects.NetworkInterfaceCollection) error {

	if !ts.tb.HasNaplesHW() {
		return ts.model.ForEachFakeNaples(func(nc *objects.NaplesCollection) error {
			cmdOut, err := ts.model.RunFakeNaplesBackgroundCommand(nc,
				"curl localhost:9007/api/interfaces/")
			if err != nil {
				return err
			}

			cmdResp, _ := cmdOut.([]*iota.Command)
			for _, cmdLine := range cmdResp {
				intfData := []netproto.Interface{}
				err := json.Unmarshal([]byte(cmdLine.Stdout), &intfData)
				if err != nil {
					return err
				}

				for _, i := range intf.Interfaces {
					for _, datum := range intfData {
						if datum.Spec.Type == "LOOPBACK" &&
							datum.Spec.IPAddress == i.Spec.IPConfig.IPAddress {
							return nil
						}
					}
				}
			}
			return fmt.Errorf("Node IP not found")
		})
	}

	var lb_netagent_fn = func(cmdout *string) error {
		intfData := []netproto.Interface{}
		err := json.Unmarshal([]byte(*cmdout), &intfData)
		if err != nil {
			return err
		}

		for _, i := range intf.Interfaces {
			for _, datum := range intfData {
				if datum.Spec.Type == "LOOPBACK" &&
					datum.Spec.IPAddress == i.Spec.IPConfig.IPAddress {
					return nil
				}
			}
		}
		return fmt.Errorf("NetAgent : No loopback IP found")
	}

	return ts.model.ForEachNaples(func(nc *objects.NaplesCollection) error {
		cmdOut, err := ts.model.RunNaplesCommand(nc, "curl localhost:9007/api/interfaces/")
		if err != nil {
			return err
		}

		return lb_netagent_fn(&cmdOut[0])
	})
}

func verifyPDSAgentIntf(intf *objects.NetworkInterfaceCollection) error {

	if !ts.tb.HasNaplesHW() {
		return ts.model.ForEachFakeNaples(func(nc *objects.NaplesCollection) error {
			cmdOut, err := ts.model.RunFakeNaplesBackgroundCommand(nc,
				"/naples/nic/bin/pdsctl show bgp --detail")
			if err != nil {
				return err
			}

			cmdResp, _ := cmdOut.([]*iota.Command)

			for _, cmdLine := range cmdResp {
				for _, i := range intf.Interfaces {
					ip := strings.Split(i.Spec.IPConfig.IPAddress, "/")[0]
					if strings.Contains(cmdLine.Stdout, ip) {
						return nil
					}
				}
			}
			return fmt.Errorf("Loopback IP not found\n")
		})
	}

	return ts.model.ForEachNaples(func(nc *objects.NaplesCollection) error {
		cmdout, err := ts.model.RunNaplesCommand(nc, "/nic/bin/pdsctl show bgp --detail")
		if err != nil {
			return err
		}

		for _, i := range intf.Interfaces {
			ip := strings.Split(i.Spec.IPConfig.GetIPAddress(), "/")[0]
			if strings.Contains(cmdout[0], ip) {
				return nil
			}
		}
		return fmt.Errorf("Loopback IP not found\n")
	})
}
