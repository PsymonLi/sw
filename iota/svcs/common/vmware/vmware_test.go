package vmware

import (
	"context"
	"fmt"
	"os"
	"strconv"
	"strings"
	"testing"
	"time"

	TestUtils "github.com/pensando/sw/venice/utils/testutils"

	"github.com/pensando/sw/venice/utils/netutils"
	conv "github.com/pensando/sw/venice/utils/strconv"

	"github.com/vmware/govmomi/event"
	"github.com/vmware/govmomi/object"
	"github.com/vmware/govmomi/vim25/mo"
	"github.com/vmware/govmomi/vim25/types"
)

func TestMain(m *testing.M) {

	runTests := m.Run()

	os.Exit(runTests)
}

func Test_datastore(t *testing.T) {

	vc1, err := NewVcenter(context.Background(), "192.168.69.120", "administrator@vsphere.local", "N0isystem$", "")

	TestUtils.Assert(t, err == nil, "Connected to venter")
	TestUtils.Assert(t, vc1 != nil, "Vencter context set")

	dc, err := vc1.SetupDataCenter("iota-dc")
	TestUtils.Assert(t, err == nil, "successfuly setup dc")
	dc, ok := vc1.datacenters["iota-dc"]
	TestUtils.Assert(t, ok, "successfuly setup dc")

	TestUtils.Assert(t, len(dc.clusters) == 1, "successfuly setup dc")

	c, ok := dc.clusters["iota-cluster"]
	TestUtils.Assert(t, ok, "successfuly setup cluster")
	TestUtils.Assert(t, len(c.hosts) == 1, "successfuly setup cluster")

	_, err = dc.findHost("iota-cluster", "tb36-host1.pensando.io")
	TestUtils.Assert(t, err == nil, "successfuly setup cluster")
	_, err = dc.Datastore("", "datastore1")
	TestUtils.Assert(t, err == nil, "successfuly setup cluster")

	pgName, err := dc.FindDvsPortGroup("pen-dvs", DvsPGMatchCriteria{Type: DvsVlanID, VlanID: 100})
	fmt.Printf("Error %v %v\n", pgName, err)
	TestUtils.Assert(t, err == nil, "Found matchin PG")

	vm, err := dc.vc.NewVM("iota-control-vm")
	TestUtils.Assert(t, err == nil, "Deploy VM done")
	TestUtils.Assert(t, vm != nil, "Deploy VM done")

	err = vm.ReconfigureNetwork("iota-def-network", pgName, 0)
	fmt.Printf("Error %v %v\n", pgName, err)
	TestUtils.Assert(t, err == nil, "dvs updated")
	/*host, err := NewHost(context.Background(), "10.10.2.30", "root", "N0isystem$")

	TestUtils.Assert(t, err == nil, "Connected to host")
	TestUtils.Assert(t, host != nil, "Host context set")

	ds, err := host.Datastore("datastore1")

	TestUtils.Assert(t, err == nil, "successfuly found")
	TestUtils.Assert(t, ds != nil, "Ds found")

	buf := bytes.NewBufferString("THIS IS TEST UPLOAD FILE")
	err = ds.Upload("testUpload.txt", buf)

	TestUtils.Assert(t, err == nil, "Upload done")

	err = ds.Remove("testUpload.txt")

	TestUtils.Assert(t, err == nil, "Remove done") */
}

func Test_dvs_create_delete(t *testing.T) {
	vc, err := NewVcenter(context.Background(), "barun-vc.pensando.io", "administrator@pensando.io", "N0isystem$", "")
	TestUtils.Assert(t, err == nil, "Connected to venter")
	TestUtils.Assert(t, vc != nil, "Vencter context set")

	vc.DestroyDataCenter("iota-dc")
	vc.DestroyDataCenter("iota-dc1")

	dc, err := vc.CreateDataCenter("iota-dc")

	TestUtils.Assert(t, err == nil, "successfuly created")
	TestUtils.Assert(t, dc != nil, "Ds not created")

	c, err := dc.CreateCluster("cluster1")
	TestUtils.Assert(t, err == nil, "successfuly created")
	TestUtils.Assert(t, c != nil, "Cluster not created")

	err = c.AddHost("tb36-host1.pensando.io", "root", "pen123!", "")
	TestUtils.Assert(t, err == nil, "host added")

	hostSpecs := []DVSwitchHostSpec{
		DVSwitchHostSpec{
			Name:  "tb36-host1.pensando.io",
			Pnics: []string{"vmnic1", "vmnic2"},
		},
	}
	dvsSpec := DVSwitchSpec{Hosts: hostSpecs,
		Name: "iota-dvs", Cluster: "cluster1", MaxPorts: 10,
		Pvlans: []DvsPvlanPair{DvsPvlanPair{Primary: 500,
			Secondary: 500, Type: "promiscuous"}},
		PortGroups: []DvsPortGroup{DvsPortGroup{Name: "pg1", Ports: 10, Type: "earlyBinding"}}}
	err = dc.AddDvs(dvsSpec)
	TestUtils.Assert(t, err == nil, "dvs added")

	dvs, err := dc.findDvs("iota-dvs")
	TestUtils.Assert(t, err == nil && dvs != nil, "dvs found")
	err = dc.AddPvlanPairsToDvs("iota-dvs", []DvsPvlanPair{DvsPvlanPair{Primary: 500,
		Secondary: 501, Type: "isolated"}})
	TestUtils.Assert(t, err == nil, "pvlan added")

	err = dc.AddPortGroupToDvs("iota-dvs",
		[]DvsPortGroup{DvsPortGroup{Name: "pg2", Ports: 10, Type: "earlyBinding"}})
	TestUtils.Assert(t, err == nil, "pvlan added")

	vmInfo, err := dc.DeployVM("cluster1", "tb36-host1.pensando.io",
		"build-111", 4, 4, []string{"VM Network"}, "/Users/sudhiaithal/build-114")
	fmt.Printf("Deploy VM error %v", err)
	TestUtils.Assert(t, err == nil, "Deploy VM done")

	fmt.Println("Vm info : ", vmInfo.IP)

	vm, err := dc.vc.NewVM("build-111")
	TestUtils.Assert(t, err == nil, "Deploy VM done")
	TestUtils.Assert(t, vm != nil, "Deploy VM done")

	err = vm.ReconfigureNetwork("VM Network", "pg1", 0)
	TestUtils.Assert(t, err == nil, "dvs updated")
	//	err = host.DestoryVM("build-111")
	//err = dc.RemoveHostsFromDvs(dvsSpec)
	//TestUtils.Assert(t, err == nil, "dvs updated")
}

/*
func Test_vcenter_dissocaitate(t *testing.T) {
	host, err := NewHost(context.Background(), "tb60-host1.pensando.io", "root", "pen123!")
	TestUtils.Assert(t, err == nil, "Connected to host")
	TestUtils.Assert(t, host != nil, "Host context set")

	vcenter, err := host.GetVcenterForHost()

	fmt.Printf("Vcenter IP %v", vcenter)
	TestUtils.Assert(t, err == nil, "Did not find vcenter")

	vc, err := NewVcenter(context.Background(), vcenter, "administrator@pensando.io", "N0isystem$",
		"YN69K-6YK5J-78X8T-0M3RH-0T12H")

	TestUtils.Assert(t, err == nil, "Connected to venter")
	TestUtils.Assert(t, vc != nil, "Vencter context set")

	err = vc.DisconnectHost("tb60-host1.pensando.io")
	TestUtils.Assert(t, err == nil, "Disconect succesful")

}
*/

func Test_vcenter_find_host(t *testing.T) {

	//TestUtils.Assert(t, false, "Ds not created")

	ctx, _ := context.WithCancel(context.Background())
	vc, err := NewVcenter(ctx, "barun-vc.pensando.io", "administrator@pensando.io", "N0isystem$",
		"YN69K-6YK5J-78X8T-0M3RH-0T12H")

	TestUtils.Assert(t, err == nil, "Connected to venter")
	TestUtils.Assert(t, vc != nil, "Vencter context set")

	dc, err := vc.SetupDataCenter("sudhiaithal-iota-dc")
	TestUtils.Assert(t, err == nil, "successfuly setup dc")
	dc, ok := vc.datacenters["sudhiaithal-iota-dc"]
	TestUtils.Assert(t, ok, "sudhiaithal setup dc")

	c, ok := dc.clusters["sudhiaithal-iota-cluster"]
	TestUtils.Assert(t, ok, "successfuly setup cluster")
	TestUtils.Assert(t, len(c.hosts) == 2, "successfuly setup cluster")

	vm, err2 := dc.NewVM("node1-ep1")
	TestUtils.Assert(t, err2 == nil, "VM FOUND")
	TestUtils.Assert(t, vm != nil, "VM FOND")

	//dvs, err := dc.findDvs("#Pen-DVS-ptprabu-iota-dc")
	//TestUtils.Assert(t, err == nil, "successfuly found dvs")
	//TestUtils.Assert(t, dvs != nil, "dvs nil")

	vm, err2 = dc.NewVM("workload-host-1-w1")
	TestUtils.Assert(t, err2 == nil, "VM FOUND")
	//TestUtils.Assert(t, vm != nil, "VM FOND")

	/*
		pgName := "iota-vmotion-pg2"
		vNWs := []NWSpec{
			{Name: pgName},
		}
		vspec := VswitchSpec{Name: "vswitch0"}

		err = dc.AddNetworks("ptprabu-iota-cluster", "tb68-host1.pensando.io", vNWs, vspec)
		TestUtils.Assert(t, err == nil, "Added")

		err = dc.AddKernelNic("ptprabu-iota-cluster", "tb68-host1.pensando.io", KernelNetworkSpec{
			Portgroup:     pgName,
			EnableVmotion: true})
		fmt.Printf("ERror %v", err)
		TestUtils.Assert(t, err == nil, "Added") */

	nvm, err := dc.CloneVM("ptprabu-iota-cluster", "192.168.68.172", "node1-ep3", "temp-vm", 4, 4)
	//fmt.Printf("Error %v", err.Error())
	TestUtils.Assert(t, err == nil, "VM Clone failed")
	TestUtils.Assert(t, nvm != nil, "VM FOND")

	TestUtils.Assert(t, false, "Ds not created")

	err2 = dc.ReconfigureVMNetwork(vm, "iota-def-network", "#Pen-DVS-sudhiaithal-iota-dc", "#Pen-PG-Network-Vlan-1", 1, true)
	fmt.Printf("Error %v", err2)
	TestUtils.Assert(t, err2 == nil, "Reconfig faild")

	ip, err := vm.vm.WaitForIP(ctx, true)
	fmt.Printf("VM IP %v", ip)

	err2 = dc.ReconfigureVMNetwork(vm, "iota-def-network", "#Pen-DVS-sudhiaithal-iota-dc", "#Pen-PG-Network-Vlan-2", 1, true)
	fmt.Printf("Error %v", err2)
	TestUtils.Assert(t, err2 == nil, "Reconfig faild")

	ip, err = vm.vm.WaitForIP(ctx, true)
	fmt.Printf("VM IP %v", ip)

	TestUtils.Assert(t, false, "Ds not created")

	/*dvsSpec := DVSwitchSpec{
	Name: "iota-dvs", Cluster: "sudhiaithal-iota-cluster",
	MaxPorts: 10,
	Pvlans: []DvsPvlanPair{DvsPvlanPair{Primary: 1024,
		Secondary: 1024, Type: "promiscuous"}}} */

	//err = dc.AddDvs(dvsSpec)
	//TestUtils.Assert(t, err == nil, "dvs added")

	/*
		vm, err2 = dc.NewVM("node2-ep1")
		TestUtils.Assert(t, err2 == nil, "VM FOUND")
		TestUtils.Assert(t, vm != nil, "VM FOND")

		macs, err3 := vm.ReadMacs()
		for nw, mac := range macs {
			fmt.Printf("NW : %v. mac %v\n", nw, mac)
		}
		TestUtils.Assert(t, err3 != nil, "VM FOUND") */

	//dvs, err := dc.findDvs("#Pen-DVS-sudhiaithal-iota-dc")
	//TestUtils.Assert(t, err == nil && dvs != nil, "dvs found")

	//pgName, err := dc.FindDvsPortGroup("Pen-DVS-sudhiaithal-iota-dc", DvsPGMatchCriteria{Type: DvsPvlan, VlanID: int32(199)})
	//fmt.Printf("Dvs port name %v %v", pgName, err)
	//TestUtils.Assert(t, err == nil, "Connected to venter")

	//err = dc.RelaxSecurityOnPg("#Pen-DVS-sudhiaithal-iota-dc", "#Pen-PG-Network-Vlan-2")
	//fmt.Printf("Error %v\n", err)
	//TestUtils.Assert(t, err == nil, "pvlan added")

	/*
		for i := int32(0); i < 100; i += 2 {
			err = dc.AddPvlanPairsToDvs("iota-dvs", []DvsPvlanPair{DvsPvlanPair{Primary: 3003 + i,
				Secondary: 3003 + i, Type: "promiscuous"}})
			fmt.Printf("Error %v\n", err)
			TestUtils.Assert(t, err == nil, "pvlan added")

			err = dc.AddPvlanPairsToDvs("iota-dvs", []DvsPvlanPair{DvsPvlanPair{Primary: 3003 + i,
				Secondary: 3003 + i + 1, Type: "isolated"}})
			fmt.Printf("Error %v\n", err)
			TestUtils.Assert(t, err == nil, "pvlan added")
		} */

	//pgName = constants.EsxDataNWPrefix + strconv.Itoa(spec.PrimaryVlan)
	//Create the port group
	/*err = dc.AddPortGroupToDvs("Pen-DVS-sudhiaithal-iota-dc",
		[]DvsPortGroup{DvsPortGroup{Name: "my-pg",
			VlanOverride: true,
			Private:      true,
			Ports:        32, Type: "earlyBinding",
			Vlan: int32(800)}})
	fmt.Printf("Error %v\n", err)
	TestUtils.Assert(t, err == nil, "pvlan added")*/

	/*
		ctx, cancel := context.WithCancel(context.Background())
		vc, err := NewVcenter(ctx, "barun-vc.pensando.io", "administrator@pensando.io", "N0isystem$",
			"YN69K-6YK5J-78X8T-0M3RH-0T12H")

		TestUtils.Assert(t, err == nil, "Connected to venter")
		TestUtils.Assert(t, vc != nil, "Vencter context set")

		dc, err := vc.SetupDataCenter("iota-dc")
		TestUtils.Assert(t, err == nil, "successfuly setup dc")
		dc, ok := vc.datacenters["iota-dc"]
		TestUtils.Assert(t, ok, "successfuly setup dc")

		TestUtils.Assert(t, len(dc.clusters) == 1, "successfuly setup dc")

		c, ok := dc.clusters["iota-cluster"]
		TestUtils.Assert(t, ok, "successfuly setup cluster")
		TestUtils.Assert(t, len(c.hosts) == 1, "successfuly setup cluster")

		cancel()

		active, err := vc.Client().SessionManager.SessionIsActive(ctx)
		TestUtils.Assert(t, active == false, "Connected to venter active")

		ctx, cancel = context.WithCancel(context.Background())
		err = vc.Reinit(ctx)
		TestUtils.Assert(t, err == nil, "Reinit failed")
		active, err = vc.Client().SessionManager.SessionIsActive(vc.Ctx())
		TestUtils.Assert(t, active == true, "Connected to venter active")

		err = dc.setUpFinder()
		//TestUtils.Assert(t, err == nil, "Setup finder succes")

		vhost, err := dc.findHost("iota-cluster", "tb60-host1.pensando.io")
		fmt.Printf("Err %v", err)
		TestUtils.Assert(t, err == nil, "Connected to venter")
		TestUtils.Assert(t, vhost != nil, "Vencter context set")

		//Create vmkitni

		vNWs := []NWSpec{
			{Name: "my-vmk2"},
		}
		vspec := VswitchSpec{Name: "vSwitch0"}

		err = dc.AddNetworks("iota-cluster", "tb60-host1.pensando.io", vNWs, vspec)
		TestUtils.Assert(t, err == nil, "Add network failed")

		err = dc.AddKernelNic("iota-cluster", "tb60-host1.pensando.io", "my-vmk2", true)
		fmt.Printf("Err %v", err)
		TestUtils.Assert(t, err == nil, "Creted VMK")

		err = dc.RemoveKernelNic("iota-cluster", "tb60-host1.pensando.io", "my-vmk2")
		fmt.Printf("Err %v", err)
		TestUtils.Assert(t, err == nil, "Removed VMK")

		err = dc.RemoveNetworks("iota-cluster", "tb60-host1.pensando.io", vNWs)
		TestUtils.Assert(t, err == nil, "Add network failed")

	*/
}
func Test_vcenter_vmotion_vmk(t *testing.T) {

	//	TestUtils.Assert(t, false, "Ds not created")

	ctx, _ := context.WithCancel(context.Background())
	vc, err := NewVcenter(ctx, "barun-vc-7.pensando.io", "administrator@pensando.io", "N0isystem$",
		"YN69K-6YK5J-78X8T-0M3RH-0T12H")

	TestUtils.Assert(t, err == nil, "Connected to venter")
	TestUtils.Assert(t, vc != nil, "Vencter context set")

	dc, err := vc.SetupDataCenter("pbhide-iota-dc")
	TestUtils.Assert(t, err == nil, "successfuly setup dc")
	dc, ok := vc.datacenters["pbhide-iota-dc"]
	TestUtils.Assert(t, ok, "pbhide setup dc")

	c, ok := dc.clusters["pbhide-iota-cluster"]
	TestUtils.Assert(t, ok, "successfuly setup cluster")
	TestUtils.Assert(t, len(c.hosts) == 2, "successfuly setup cluster")

	dvs, err := dc.findDvs("#Pen-DVS-pbhide-iota-dc")
	TestUtils.Assert(t, err == nil, "successfuly found dvs")
	TestUtils.Assert(t, dvs != nil, "dvs nil")
	fmt.Printf("====Create VMKNIC\n")
	vHost, err := dc.findHost("pbhide-iota-cluster", "10.8.101.31")
	TestUtils.Assert(t, err == nil, "host not found")
	vmkSpec := KernelNetworkSpec{
		IPAddress:     "169.254.0.31",
		Subnet:        "255.255.255.0",
		Portgroup:     "#Pen-PG-Network-Vlan-1",
		EnableVmotion: true,
	}
	err = vHost.AddKernelNic(vmkSpec)
	TestUtils.Assert(t, err == nil, "Failed to setup vmknic %v", err)

	// time.Sleep(5 * time.Second)
	// fmt.Printf("Remove vmks on host %s\n", vHost.Name)
	// err = vHost.RemoveKernelNic("#Pen-PG-Network-Vlan-1")
	// TestUtils.Assert(t, err == nil, "Failed to remove vmknic %v", err)

	vHost, err = dc.findHost("pbhide-iota-cluster", "10.8.104.65")
	TestUtils.Assert(t, err == nil, "host not found")
	vmkSpec = KernelNetworkSpec{
		IPAddress:     "169.254.0.65",
		Subnet:        "255.255.255.0",
		Portgroup:     "#Pen-PG-Network-Vlan-1",
		EnableVmotion: true,
	}
	err = vHost.AddKernelNic(vmkSpec)
	TestUtils.Assert(t, err == nil, "Failed to setup vmknic %v", err)
	time.Sleep(5 * time.Second)
	// fmt.Printf("Remove vmks on host %s\n", vHost.Name)
	// err = vHost.RemoveKernelNic("#Pen-PG-Network-Vlan-1")
	// TestUtils.Assert(t, err == nil, "Failed to remove vmknic %v", err)
}

func Test_vcenter_hosts(t *testing.T) {

	// Parag:
	//    TestUtils.Assert(t, false, "Ds not created")

	ctx, _ := context.WithCancel(context.Background())
	vc, err := NewVcenter(ctx, "barun-vc-7.pensando.io", "administrator@pensando.io", "N0isystem$",
		"YN69K-6YK5J-78X8T-0M3RH-0T12H")

	TestUtils.Assert(t, err == nil, "Connected to venter")
	TestUtils.Assert(t, vc != nil, "Vencter context set")

	dcName := "HPE-DevSetup-1"
	dc, err := vc.SetupDataCenter(dcName)
	TestUtils.Assert(t, err == nil, "successfuly setup dc")
	dc, ok := vc.datacenters[dcName]
	TestUtils.Assert(t, ok, dcName)

	// dvsName := "#Pen-DVS-" + dcName
	dvsName := "DualDSC-DVS-139"
	dvs, err := dc.findDvs(dvsName)
	TestUtils.Assert(t, err == nil, fmt.Sprintf("%s", err))
	TestUtils.Assert(t, dvs != nil, "dvs nil")

	var dvsInfo mo.DistributedVirtualSwitch
	err = dvs.Properties(vc.Ctx(), dvs.Reference(), []string{"name", "config"}, &dvsInfo)
	TestUtils.Assert(t, err == nil, "Failed to get dvs props")

	fmt.Printf("DVS Name %s, ConfigVersion %s\n", dvsInfo.Name, dvsInfo.Config.GetDVSConfigInfo().ConfigVersion)

	fmt.Printf("====Find Hosts\n")
	finder := vc.Finder()
	finder.SetDatacenter(dc.ref)
	hosts, err := finder.HostSystemList(vc.Ctx(), "*")
	TestUtils.AssertOk(t, err, "Failed to get hosts from DC")
	var moHosts []mo.HostSystem
	for _, host := range hosts {
		fmt.Printf("Host found %s\n", host.Name())
		var moHost mo.HostSystem
		err = host.Properties(vc.Ctx(), host.Reference(), []string{"name", "config"}, &moHost)
		moHosts = append(moHosts, moHost)
	}
	// check if host has pensando nic connected
	pnicsUsed := map[string](map[string]bool){}
	for _, mHost := range moHosts {
		for _, ps := range mHost.Config.Network.ProxySwitch {
			if ps.Spec.Backing == nil {
				fmt.Errorf("Pnic Backing Spec is missing")
				continue	
			}
			pnicBacking, ok := ps.Spec.Backing.(*types.DistributedVirtualSwitchHostMemberPnicBacking)
			if !ok {
				continue
			}
			fmt.Printf("DVS Proxy %s Backing PnicSpec %v\n", ps.DvsName, pnicBacking)
			for _, pnic := range ps.Pnic {
				fmt.Printf("Host %s, Pnic - %s\n", mHost.Name, pnic)
				if _, ok := pnicsUsed[mHost.Name]; ok {
					pnicsUsed[mHost.Name][pnic] = true
				} else {
					pnicsUsed[mHost.Name] = map[string]bool{}
					pnicsUsed[mHost.Name][pnic] = true
				}
			}
		}
	}
	fmt.Printf("pnics used : %v\n", pnicsUsed)

	hostName1 := "192.168.71.139"
	addRemoveHosts := map[string]bool{}
	addRemoveHosts[hostName1] = true
	reconfigureHostSpec := []types.DistributedVirtualSwitchHostMemberConfigSpec{}
	for i := 0; i < 1; i++ {
		for _, mHost := range moHosts {
			if _, ok := addRemoveHosts[mHost.Name]; !ok {
				continue
			}
			backing := new(types.DistributedVirtualSwitchHostMemberPnicBacking)
			hostSpec := types.DistributedVirtualSwitchHostMemberConfigSpec{
				Host:      mHost.Reference(),
				Operation: string(types.ConfigSpecOperationEdit),
				Backing:   backing,
			}
			uplink := 0
			for _, pnic := range mHost.Config.Network.Pnic {
				macStr, _ := conv.ParseMacAddr(pnic.Mac)
				if !netutils.IsPensandoMACAddress(macStr) {
					continue
				}
				used := false
				if _, ok := pnicsUsed[mHost.Name]; ok {
					_, used = pnicsUsed[mHost.Name][pnic.Key]
				}
				fmt.Printf("Pnic - %s : %s : %v\n", pnic.Device, macStr, used)
				// don't add vmnic10
				if !used && pnic.Device != "vmnic10" {
					// add it
					uplink++
					uplinkStr := strconv.FormatInt(int64(uplink), 10)
					backing.PnicSpec = append(backing.PnicSpec,
						types.DistributedVirtualSwitchHostMemberPnicSpec{
							PnicDevice:         pnic.Device,
							UplinkPortgroupKey: "dvportgroup-1104",
							UplinkPortKey:      uplinkStr})
				}
			}
			if len(backing.PnicSpec) == 0 {
				fmt.Printf("Removing all pnics\n")
			} else {
				fmt.Printf("Adding all pnics\n")
			}
			reconfigureHostSpec = append(reconfigureHostSpec, hostSpec)
		}
		// just set the values that we change??
		dvsCfg := &types.DVSConfigSpec{
			ConfigVersion: dvsInfo.Config.GetDVSConfigInfo().ConfigVersion,
			Name:          dvsInfo.Name,
			Host:          reconfigureHostSpec,
		}
		task, err := dvs.Reconfigure(vc.Ctx(), dvsCfg)
		TestUtils.AssertOk(t, err, "Reconfigure DVS failed %s", err)
		_, err = task.WaitForResult(vc.Ctx())
		TestUtils.AssertOk(t, err, "Reconfigure DVS Task failed %s", err)

		// time.Sleep(10*time.Second)
	}
}

func Test_PGs(t *testing.T) {

	// Parag:
	//    TestUtils.Assert(t, false, "Ds not created")

	ctx, _ := context.WithCancel(context.Background())
	vc, err := NewVcenter(ctx, "barun-vc-7.pensando.io", "administrator@pensando.io", "N0isystem$",
		"YN69K-6YK5J-78X8T-0M3RH-0T12H")

	TestUtils.Assert(t, err == nil, "Connected to venter")
	TestUtils.Assert(t, vc != nil, "Vencter context set")

	dcName := "HPE-DevSetup-1"
	dc, err := vc.SetupDataCenter(dcName)
	TestUtils.Assert(t, err == nil, "successfuly setup dc")
	dc, ok := vc.datacenters[dcName]
	TestUtils.Assert(t, ok, dcName)

	// dvsName := "#Pen-DVS-" + dcName
	dvsName := "DualDSC-DVS-139"
	dvs, err := dc.findDvs(dvsName)
	TestUtils.Assert(t, err == nil, fmt.Sprintf("%s", err))
	TestUtils.Assert(t, dvs != nil, "dvs nil")
	pgName := "HostPG-201"
	netRef, err := dc.Finder().Network(dc.vc.Ctx(), pgName)
	TestUtils.Assert(t, err == nil, fmt.Sprintf("PG not found - %s", err))
	objPg, ok := netRef.(*object.DistributedVirtualPortgroup)
	var dvsPg mo.DistributedVirtualPortgroup
	err = objPg.Properties(dc.vc.Ctx(), objPg.Reference(), []string{"name", "config"}, &dvsPg)
	TestUtils.Assert(t, err == nil, fmt.Sprintf("PG config not found - %s", err))
	portSetting := dvsPg.Config.DefaultPortConfig.(*types.VMwareDVSPortSetting)
	fmt.Printf("Net Teaming config = %v\n", portSetting.UplinkTeamingPolicy.UplinkPortOrder)
	err = dc.AddPortGroupToDvs(dvsName,
		[]DvsPortGroup{DvsPortGroup{Name: "pg2", Ports: 10, Type: "earlyBinding"}})
	TestUtils.Assert(t, err == nil, "pvlan added")

	netRef, err = dc.Finder().Network(dc.vc.Ctx(), "pg2")
	TestUtils.Assert(t, err == nil, fmt.Sprintf("PG not found - %s", err))
	objPg, ok = netRef.(*object.DistributedVirtualPortgroup)
	err = objPg.Properties(dc.vc.Ctx(), objPg.Reference(), []string{"name", "config"}, &dvsPg)
	TestUtils.Assert(t, err == nil, fmt.Sprintf("PG config not found - %s", err))
	portSetting = dvsPg.Config.DefaultPortConfig.(*types.VMwareDVSPortSetting)
	fmt.Printf("Net Teaming config = %v\n", portSetting.UplinkTeamingPolicy.UplinkPortOrder)
}

type vmRelocationSpec struct {
	vmName                 string
	srcDcName, srcHostName string
	dstDcName, dstHostName string
}

func Test_vmotions(t *testing.T) {

	// Parag:
	//    TestUtils.Assert(t, false, "Ds not created")

	ctx, _ := context.WithCancel(context.Background())
	vc, err := NewVcenter(ctx, "barun-vc-7.pensando.io", "administrator@pensando.io", "N0isystem$",
		"YN69K-6YK5J-78X8T-0M3RH-0T12H")

	TestUtils.Assert(t, err == nil, "Connected to venter")
	TestUtils.Assert(t, vc != nil, "Vencter context set")

	// specify all vms to be moved
	//    vmRelocations := []vmRelocationSpec {
	//        {   vmName: "testnode8",
	//            srcDcName: "HPE-ESX1",
	//            srcHostName: "192.168.71.139",
	//            dstDcName: "HPE-ESX1",
	//            dstHostName: "192.168.70.205",
	//        },
	//        {   vmName: "testnode3",
	//            srcDcName: "HPE-ESX1",
	//            srcHostName: "192.168.70.205",
	//            dstDcName: "HPE-ESX1",
	//            dstHostName: "192.168.71.139",
	//        },
	//    }
	vmRelocations := []vmRelocationSpec{
		// host-1 : 10.8.100.105
		{vmName: "workload-host-1-w0", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.105",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.103.6"},
		{vmName: "workload-host-1-w4", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.105",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.103.6"},
		{vmName: "workload-host-1-w8", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.105",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.103.6"},
		{vmName: "workload-host-1-w12", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.105",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.103.6"},
		{vmName: "workload-host-1-w16", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.105",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.103.6"},
		{vmName: "workload-host-1-w20", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.105",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.103.6"},
		{vmName: "workload-host-1-w24", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.105",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.103.6"},
		{vmName: "workload-host-1-w28", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.105",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.103.6"},

		// host-2 : 10.8.103.6
		{vmName: "workload-host-2-w0", srcDcName: "3784-iota-dc", srcHostName: "10.8.103.6",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.102.7"},
		{vmName: "workload-host-2-w4", srcDcName: "3784-iota-dc", srcHostName: "10.8.103.6",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.102.7"},
		{vmName: "workload-host-2-w8", srcDcName: "3784-iota-dc", srcHostName: "10.8.103.6",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.102.7"},
		{vmName: "workload-host-2-w12", srcDcName: "3784-iota-dc", srcHostName: "10.8.103.6",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.102.7"},
		{vmName: "workload-host-2-w16", srcDcName: "3784-iota-dc", srcHostName: "10.8.103.6",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.102.7"},
		{vmName: "workload-host-2-w20", srcDcName: "3784-iota-dc", srcHostName: "10.8.103.6",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.102.7"},
		{vmName: "workload-host-2-w24", srcDcName: "3784-iota-dc", srcHostName: "10.8.103.6",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.102.7"},
		{vmName: "workload-host-2-w28", srcDcName: "3784-iota-dc", srcHostName: "10.8.103.6",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.102.7"},

		{vmName: "workload-host-3-w0", srcDcName: "3784-iota-dc", srcHostName: "10.8.102.7",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.164"},
		{vmName: "workload-host-3-w4", srcDcName: "3784-iota-dc", srcHostName: "10.8.102.7",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.164"},
		{vmName: "workload-host-3-w8", srcDcName: "3784-iota-dc", srcHostName: "10.8.102.7",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.164"},
		{vmName: "workload-host-3-w12", srcDcName: "3784-iota-dc", srcHostName: "10.8.102.7",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.164"},
		{vmName: "workload-host-3-w16", srcDcName: "3784-iota-dc", srcHostName: "10.8.102.7",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.164"},
		{vmName: "workload-host-3-w20", srcDcName: "3784-iota-dc", srcHostName: "10.8.102.7",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.164"},
		{vmName: "workload-host-3-w24", srcDcName: "3784-iota-dc", srcHostName: "10.8.102.7",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.164"},
		{vmName: "workload-host-3-w28", srcDcName: "3784-iota-dc", srcHostName: "10.8.102.7",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.164"},

		{vmName: "workload-host-4-w0", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.164",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.107"},
		{vmName: "workload-host-4-w4", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.164",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.107"},
		{vmName: "workload-host-4-w8", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.164",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.107"},
		{vmName: "workload-host-4-w12", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.164",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.107"},
		{vmName: "workload-host-4-w16", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.164",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.107"},
		{vmName: "workload-host-4-w20", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.164",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.107"},
		{vmName: "workload-host-4-w24", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.164",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.107"},
		{vmName: "workload-host-4-w28", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.164",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.107"},

		{vmName: "workload-host-5-w0", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.107",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.2"},
		{vmName: "workload-host-5-w4", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.107",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.2"},
		{vmName: "workload-host-5-w8", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.107",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.2"},
		{vmName: "workload-host-5-w12", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.107",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.2"},
		{vmName: "workload-host-5-w16", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.107",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.2"},
		{vmName: "workload-host-5-w20", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.107",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.2"},
		{vmName: "workload-host-5-w24", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.107",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.2"},
		{vmName: "workload-host-5-w28", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.107",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.2"},

		{vmName: "workload-host-6-w0", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.2",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.106"},
		{vmName: "workload-host-6-w4", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.2",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.106"},
		{vmName: "workload-host-6-w8", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.2",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.106"},
		{vmName: "workload-host-6-w12", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.2",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.106"},
		{vmName: "workload-host-6-w16", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.2",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.106"},
		{vmName: "workload-host-6-w20", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.2",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.106"},
		{vmName: "workload-host-6-w24", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.2",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.106"},
		{vmName: "workload-host-6-w28", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.2",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.106"},

		{vmName: "workload-host-7-w0", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.106",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.102.56"},
		{vmName: "workload-host-7-w4", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.106",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.102.56"},
		{vmName: "workload-host-7-w8", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.106",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.102.56"},
		{vmName: "workload-host-7-w12", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.106",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.102.56"},
		{vmName: "workload-host-7-w16", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.106",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.102.56"},
		{vmName: "workload-host-7-w20", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.106",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.102.56"},
		{vmName: "workload-host-7-w24", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.106",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.102.56"},
		{vmName: "workload-host-7-w28", srcDcName: "3784-iota-dc", srcHostName: "10.8.100.106",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.102.56"},

		{vmName: "workload-host-8-w0", srcDcName: "3784-iota-dc", srcHostName: "10.8.102.56",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.105"},
		{vmName: "workload-host-8-w4", srcDcName: "3784-iota-dc", srcHostName: "10.8.102.56",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.105"},
		{vmName: "workload-host-8-w8", srcDcName: "3784-iota-dc", srcHostName: "10.8.102.56",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.105"},
		{vmName: "workload-host-8-w12", srcDcName: "3784-iota-dc", srcHostName: "10.8.102.56",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.105"},
		{vmName: "workload-host-8-w16", srcDcName: "3784-iota-dc", srcHostName: "10.8.102.56",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.105"},
		{vmName: "workload-host-8-w20", srcDcName: "3784-iota-dc", srcHostName: "10.8.102.56",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.105"},
		{vmName: "workload-host-8-w24", srcDcName: "3784-iota-dc", srcHostName: "10.8.102.56",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.105"},
		{vmName: "workload-host-8-w28", srcDcName: "3784-iota-dc", srcHostName: "10.8.102.56",
			dstDcName: "3784-iota-dc", dstHostName: "10.8.100.105"},
	}

	// optional event listener
	runEventReceiver := func(vc *Vcenter, ref types.ManagedObjectReference) {
		// return
		ctx := vc.Ctx()
		// chQ := channelqueue.NewChQueue()
		// chQ.Start(ctx)
		chQ := make(chan types.BaseEvent, 1000)

		receiveEvents := func(ref types.ManagedObjectReference, events []types.BaseEvent) error {
			l := len(events)
			for i := l - 1; i >= 0; i-- {
				// fmt.Printf("Post Event\n")
				chQ <- events[i]
				// chQ.Send(events[i])
			}
			return nil
		}

		go func() {
			// readCh := chQ.ReadCh()
			fmt.Printf("Event Processor start\n")
			for {
				select {
				case <-ctx.Done():
					return
				case ev := <-chQ:
					// Ignore all events that do not point to Pen-DVS, e.Dvs.Dvs is not populated
					switch e := ev.(type) {
					case *types.VmEmigratingEvent:
						fmt.Printf("Event %d - %v - %T for VM %s\n", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value)
					case *types.VmBeingHotMigratedEvent:
						fmt.Printf("Event %d - %v - %T for VM %s\n", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value)
					case *types.VmBeingMigratedEvent:
						fmt.Printf("Event %d - %v - %T for VM %s\n", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value)
					case *types.VmBeingRelocatedEvent:
						fmt.Printf("Event %d - %v - %T for VM %s\n", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value)
					case *types.VmMigratedEvent:
						fmt.Printf("Event %d - %v - %T for VM %s\n", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value)
					case *types.VmRelocatedEvent:
						fmt.Printf("Event %d - %v - %T for VM %s\n", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value)
					case *types.VmRelocateFailedEvent:
						fmt.Printf("Event %d - %v - %T for VM %s\n", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value)
					case *types.VmFailedMigrateEvent:
						fmt.Printf("Event %d - %v - %T for VM %s\n", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value)
					case *types.MigrationEvent:
						// This may be a generic vMotion failure.. but it does not have DestHost Info
						fmt.Printf("Event %d - %v - %T for VM %s reason %s\n", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value, e.Fault.LocalizedMessage)
					case *types.EventEx:
						s := strings.Split(e.EventTypeId, ".")
						evType := s[len(s)-1]
						if evType == "VmHotMigratingWithEncryptionEvent" {
							fmt.Printf("EventEx %d - %v - TypeId %s Vm %s\n", e.GetEvent().Key, ref.Value, e.EventTypeId, e.Vm.Vm.Value)
						} else {
							// v.Log.Debugf("Ignored EventEx %d - %s - TypeId %s...", e.GetEvent().Key, ref.Value, e.EventTypeId)
						}
					case *types.ExtendedEvent:
						// v.Log.Debugf("Ignored ExtendedEvent %d - TypeId %s ...", e.GetEvent().Key, ref.Value, e.EventTypeId)
					default:
						// fmt.Printf("Ignored Event %d\n", e.GetEvent().Key)
						// v.Log.Debugf("Ignored Event %d - %s - %T received ... (+%v)", ev.GetEvent().Key, ref.Value, ev, ev)
						// v.Log.Debugf("Ignored Event %d - %s - %T received ...", ev.GetEvent().Key, ref.Value, ev)
					}
				}
			}
		}()

		eventMgr := event.NewManager(vc.Client().Client)
		fmt.Printf("Event Receiver Starting on %v\n", ref)
		eventMgr.Events(ctx, []types.ManagedObjectReference{ref}, 10, true, false, receiveEvents)
		// Done
		fmt.Printf("Event Receiver Exiting\n")
	}
	{
		dcName := "3784-iota-dc"
		finder := vc.Finder()
		if finder == nil {
			fmt.Printf("Error finder\n")
		} else {
			dc, _ := finder.Datacenter(vc.Ctx(), dcName)
			dcFolders, _ := dc.Folders(vc.Ctx())
			go runEventReceiver(vc, dcFolders.VmFolder.Reference())
		}
	}
	// Get DC reference, get conext and start receiver
	for i := 0; i < 1; i++ {
		relocationTasks := map[string]*object.Task{}
		for _, relo := range vmRelocations {
			fmt.Printf("Start Vmotion\n")
			srcHostName := relo.srcHostName
			dstHostName := relo.dstHostName
			srcDcName := relo.srcDcName
			dstDcName := relo.dstDcName
			if (i % 2) != 0 {
				// reverse
				srcHostName = relo.dstHostName
				dstHostName = relo.srcHostName
				srcDcName = relo.dstDcName
				dstDcName = relo.srcDcName
			}
			// src
			dc, err := vc.SetupDataCenter(srcDcName)
			srcHost := dc.findVcHost("", srcHostName)
			TestUtils.Assert(t, srcHost != nil, "Src host %s not found", srcHostName)
			vm := dc.findVcVM(relo.vmName)
			TestUtils.Assert(t, vm != nil, "VM %s not found", relo.vmName)
			// dst
			dc, err = vc.SetupDataCenter(dstDcName)
			dstHost := dc.findVcHost("", dstHostName)
			var moHost mo.HostSystem
			err = dstHost.Properties(vc.Ctx(), dstHost.Reference(), []string{"datastore", "config", "parent"}, &moHost)

			TestUtils.Assert(t, dstHost != nil, "Dst host %s not found", dstHostName)
			dsRef := moHost.Datastore[0].Reference()
			hostRef := moHost.Reference()
			rsPool, err := dstHost.ResourcePool(vc.Ctx())
			TestUtils.AssertOk(t, err, "Resource pool")
			rsPoolRef := rsPool.Reference()
			relocateSpec := types.VirtualMachineRelocateSpec{
				Datastore: &dsRef,
				Host:      &hostRef,
				Pool:      &rsPoolRef,
			}
			task, err := vm.Relocate(vc.Ctx(), relocateSpec, types.VirtualMachineMovePriorityDefaultPriority)
			TestUtils.AssertOk(t, err, "Failed to Start relocation")
			relocationTasks[relo.vmName] = task
		}
		for vmName, task := range relocationTasks {
			fmt.Printf("Wait for vmotion %s\n", vmName)
			_, err = task.WaitForResult(vc.Ctx())
			TestUtils.AssertOk(t, err, "Relocation of VM %s failed %s\n", vmName, err)
		}
		time.Sleep(5 * time.Second)
	}
}

func Test_HostConnection(t *testing.T) {

	// Parag:
	//    TestUtils.Assert(t, false, "Ds not created")

	ctx, _ := context.WithCancel(context.Background())
	vc, err := NewVcenter(ctx, "barun-vc-7.pensando.io", "administrator@pensando.io", "N0isystem$",
		"YN69K-6YK5J-78X8T-0M3RH-0T12H")

	TestUtils.Assert(t, err == nil, "Connected to venter")
	TestUtils.Assert(t, vc != nil, "Vencter context set")

	dcName := "HPE-DevSetup-1"
	dc, err := vc.SetupDataCenter(dcName)
	hostName := "192.168.70.205"
	vcHost := dc.findVcHost("Cluster1", hostName)
	dvsName := "#Pen-DVS-" + dcName
	dvs, err := dc.findDvs(dvsName)
	TestUtils.Assert(t, dvs != nil, "dvs nil")

	pollDvsAndHost := func(ctx context.Context, host *object.HostSystem, dvs *object.DistributedVirtualSwitch) {
		fmt.Printf("Checking host and DVS Runtime at %s\n", time.Now().Format("Jan 2 15:04:05 2006 MST"))
		var HostConnected types.HostSystemConnectionState
		isDvsMember := false
		for {
			select {
			case <-ctx.Done():
				return
			case <-time.After(time.Second * 1):
				var moDVS mo.DistributedVirtualSwitch
				err = dvs.Properties(vc.Ctx(), dvs.Reference(), []string{"name", "config", "runtime"}, &moDVS)
				TestUtils.Assert(t, err == nil, "Failed to get dvs props")
				dvsMemberHost := false
				for _, hostRT := range moDVS.Runtime.HostMemberRuntime {
					if hostRT.Host == host.Reference() {
						dvsMemberHost = true
					}
				}
				if isDvsMember != dvsMemberHost {
					isDvsMember = dvsMemberHost
					fmt.Printf("Host Member status %v at %s\n", isDvsMember, time.Now().Format("Jan 2 15:04:05 2006 MST"))
				}

				var moHost mo.HostSystem
				err = host.Properties(vc.Ctx(), host.Reference(), []string{"name", "config", "runtime"}, &moHost)
				TestUtils.Assert(t, err == nil, "Failed to get Host properties")
				if HostConnected != moHost.Runtime.ConnectionState {
					fmt.Printf("Host connection status %s at %s\n", moHost.Runtime.ConnectionState, time.Now().Format("Jan 2 15:04:05 2006 MST"))
					HostConnected = moHost.Runtime.ConnectionState
				}
			}
		}
	}

	ctx, cancel := context.WithCancel(context.Background())
	go pollDvsAndHost(ctx, vcHost, dvs)
	time.Sleep(15 * time.Minute)
	cancel()
}

func (dc *DataCenter) findVcVM(vmName string) *object.VirtualMachine {
	finder := dc.Finder()
	vms, err := finder.VirtualMachineList(dc.vc.Ctx(), "*")
	if err != nil {
		fmt.Printf("VM list error %s\n", err)
		return nil
	}
	for _, vm := range vms {
		fmt.Printf("VM found %s\n", vm.Name())
		if vm.Name() == vmName {
			return vm
		}
	}
	return nil
}

func (dc *DataCenter) findVcHost(clusterName, hostName string) *object.HostSystem {
	// Cluster TBD
	fmt.Printf("====Find Host %s\n", hostName)
	finder := dc.Finder()
	hosts, err := finder.HostSystemList(dc.vc.Ctx(), "*")
	if err != nil {
		fmt.Printf("Host list error %s\n", err)
		return nil
	}
	for _, host := range hosts {
		fmt.Printf("Host found %s\n", host.Name())
		if host.Name() == hostName {
			return host
		}
	}
	fmt.Printf("Host not found\n")
	return nil
}

func Test_vcenter_ovf_deploy(t *testing.T) {

	host, err := NewHost(context.Background(), "tb36-host2.pensando.io", "root", "pen123!")
	TestUtils.Assert(t, err == nil, "Connected to host")

	var intf interface{}

	intf = host

	pgs, err := host.ListNetworks()
	TestUtils.Assert(t, err == nil, "PG list failed")
	for _, pg := range pgs {
		fmt.Printf("PG %v %v %v\n", pg.Name, pg.Vlan, pg.Vswitch)
	}
	intf.(EntityIntf).DestoryVM("asds")

	TestUtils.Assert(t, !host.IsVcenter(), "NOt vcenter")

	TestUtils.Assert(t, false, "Ds not created")

	ctx, _ := context.WithCancel(context.Background())
	vc, err := NewVcenter(ctx, "barun-vc.pensando.io", "administrator@pensando.io", "N0isystem$",
		"YN69K-6YK5J-78X8T-0M3RH-0T12H")

	TestUtils.Assert(t, err == nil, "Connected to venter")
	TestUtils.Assert(t, vc != nil, "Vencter context set")

	dc, err := vc.SetupDataCenter("iota-dc")
	TestUtils.Assert(t, err == nil, "successfuly setup dc")
	dc, ok := vc.datacenters["iota-dc"]
	TestUtils.Assert(t, ok, "successfuly setup dc")

	c, ok := dc.clusters["iota-cluster"]
	TestUtils.Assert(t, ok, "successfuly setup cluster")
	TestUtils.Assert(t, len(c.hosts) == 2, "successfuly setup cluster")

	vhost, err := dc.findHost("iota-cluster", "tb60-host2.pensando.io")
	TestUtils.Assert(t, err == nil, "Connected to venter")
	TestUtils.Assert(t, vhost != nil, "Vencter context set")

	pgName, err := dc.FindDvsPortGroup("pen-dvs", DvsPGMatchCriteria{Type: DvsVlanID, VlanID: int32(3)})
	fmt.Printf("Dvs port name %v %v", pgName, err)
	TestUtils.Assert(t, err == nil, "Connected to venter")

	//TestUtils.Assert(t, false, "Ds not created")

	//vmInfo, err := host.DeployVM("build-111", 4, 4, []string{"VM Network"}, "/home/sudhiaithal/build-111/")
	/*vmInfo, err := dc.DeployVM("iota-cluster", "tb60-host2.pensando.io",
		"build-112", 4, 4, []string{"VM Network", "VM Network", "VM Network", "iota-def-network"}, "/Users/sudhiaithal/build-114")
	fmt.Printf("Deploy VM error %v", err)
	TestUtils.Assert(t, err == nil, "Deploy VM done")

	fmt.Println("Vm info : ", vmInfo.IP)*/

	/*
		ctx, cancel := context.WithCancel(context.Background())
		vc, err := NewVcenter(ctx, "barun-vc.pensando.io", "administrator@pensando.io", "N0isystem$",
			"YN69K-6YK5J-78X8T-0M3RH-0T12H")

		TestUtils.Assert(t, err == nil, "Connected to venter")
		TestUtils.Assert(t, vc != nil, "Vencter context set")

		dc, err := vc.SetupDataCenter("iota-dc")
		TestUtils.Assert(t, err == nil, "successfuly setup dc")
		dc, ok := vc.datacenters["iota-dc"]
		TestUtils.Assert(t, ok, "successfuly setup dc")

		TestUtils.Assert(t, len(dc.clusters) == 1, "successfuly setup dc")

		c, ok := dc.clusters["iota-cluster"]
		TestUtils.Assert(t, ok, "successfuly setup cluster")
		TestUtils.Assert(t, len(c.hosts) == 1, "successfuly setup cluster")

		cancel()

		active, err := vc.Client().SessionManager.SessionIsActive(ctx)
		TestUtils.Assert(t, active == false, "Connected to venter active")

		ctx, cancel = context.WithCancel(context.Background())
		err = vc.Reinit(ctx)
		TestUtils.Assert(t, err == nil, "Reinit failed")
		active, err = vc.Client().SessionManager.SessionIsActive(vc.Ctx())
		TestUtils.Assert(t, active == true, "Connected to venter active")

		err = dc.setUpFinder()
		//TestUtils.Assert(t, err == nil, "Setup finder succes")

		vhost, err := dc.findHost("iota-cluster", "tb60-host1.pensando.io")
		fmt.Printf("Err %v", err)
		TestUtils.Assert(t, err == nil, "Connected to venter")
		TestUtils.Assert(t, vhost != nil, "Vencter context set")

		//Create vmkitni

		vNWs := []NWSpec{
			{Name: "my-vmk2"},
		}
		vspec := VswitchSpec{Name: "vSwitch0"}

		err = dc.AddNetworks("iota-cluster", "tb60-host1.pensando.io", vNWs, vspec)
		TestUtils.Assert(t, err == nil, "Add network failed")

		err = dc.AddKernelNic("iota-cluster", "tb60-host1.pensando.io", "my-vmk2", true)
		fmt.Printf("Err %v", err)
		TestUtils.Assert(t, err == nil, "Creted VMK")

		err = dc.RemoveKernelNic("iota-cluster", "tb60-host1.pensando.io", "my-vmk2")
		fmt.Printf("Err %v", err)
		TestUtils.Assert(t, err == nil, "Removed VMK")

		err = dc.RemoveNetworks("iota-cluster", "tb60-host1.pensando.io", vNWs)
		TestUtils.Assert(t, err == nil, "Add network failed")

	*/
}

/*
func Test_vcenter_migration(t *testing.T) {

	ctx, _ := context.WithCancel(context.Background())
	vc, err := NewVcenter(ctx, "barun-vc.pensando.io", "administrator@pensando.io", "N0isystem$",
		"YN69K-6YK5J-78X8T-0M3RH-0T12H")

	TestUtils.Assert(t, err == nil, "Connected to venter")
	TestUtils.Assert(t, vc != nil, "Vencter context set")

	dc, err := vc.SetupDataCenter("iota-dc")
	TestUtils.Assert(t, err == nil, "successfuly setup dc")
	dc, ok := vc.datacenters["iota-dc"]
	TestUtils.Assert(t, ok, "successfuly setup dc")

	TestUtils.Assert(t, len(dc.clusters) == 1, "successfuly setup dc")

	c, ok := dc.clusters["iota-cluster"]
	TestUtils.Assert(t, ok, "successfuly setup cluster")
	TestUtils.Assert(t, len(c.hosts) == 2, "successfuly setup cluster")

	//vhost, _ := dc.findHost("iota-cluster", "tb60-host2.pensando.io")

	viewMgr := view.NewManager(dc.client.Client.Client)

	//VCObject("HostSystem")
	var hosts []mo.HostSystem
	//v.ListObj(defs.HostSystem, []string{"config", "name"}, &hosts, dc.ref)

	cView, err := viewMgr.CreateContainerView(dc.vc.Ctx(), dc.ref.Reference(), []string{}, true)
	TestUtils.Assert(t, err == nil, "View create failed")
	err = cView.Retrieve(dc.vc.Ctx(), []string{string("HostSystem")}, []string{"config", "name", "datastore"}, &hosts)
	TestUtils.Assert(t, err == nil, "View retriee ")

	for _, host := range hosts {
		fmt.Printf("HOst name %v : \n", host.Name)
		for _, ds := range host.Datastore {
			ds.Reference()
			fmt.Printf("Data store name %v %v\n", ds.String(), ds.Value)
		}
	}

	ds, _ := dc.finder.DatastoreList(dc.vc.Ctx(), "*")
	for _, d := range ds {
		fmt.Printf("Data Store %v %v ", d.Reference(), d.Name())
	}

	err = dc.LiveMigrate("my-vm", "tb60-host2.pensando.io", "tb60-host1.pensando.io", "iota-cluster")

	fmt.Printf("Err %v", err)
	TestUtils.Assert(t, err == nil, "Vmotion failed")
	//TestUtils.Assert(t, vhost != nil, "Vencter context set")
}
*/
func Test_ovf_deploy(t *testing.T) {

	host, err := NewHost(context.Background(), "10.8.100.2", "root", "Pen0trl!")

	TestUtils.Assert(t, err == nil, "Connected to host")
	TestUtils.Assert(t, host != nil, "Host context set")

	hostVM, err3 := host.NewVM("iota-control-vm-10.8.100.2")
	TestUtils.Assert(t, hostVM != nil && err3 == nil, "VM FOND")

	nws, _ := host.ListNetworks()
	for _, nw := range nws {
		if strings.Contains(nw.Name, "iota-naples-mgmt") {
			hostVM.ReconfigureNetwork(nw.Name, "iota-def-network", 0)
		}
	}

	news := []NWSpec{{Name: "iota-naples-mgmt-0", Vlan: 0}}
	host.RemoveNetworks(news)

	vsname := "iota-naples-mgmt-switch"
	vsspec := VswitchSpec{Name: vsname}

	host.RemoveVswitch(vsspec)

	vsspec = VswitchSpec{Name: vsname, Pnics: []string{"vmnic4"}}

	err = host.AddVswitch(vsspec)
	TestUtils.Assert(t, err == nil, "Connected to host")

	news = []NWSpec{{Name: "iota-naples-mgmt-network-0", Vlan: 0}}

	err = host.AddNetworks(news, vsspec)

	TestUtils.Assert(t, err == nil, "Connected to host")

	err = hostVM.ReconfigureNetwork("iota-def-network", "iota-naples-mgmt-network-0", 0)
	TestUtils.Assert(t, err == nil, "Connected to host")

	/*
		vc, err := NewVcenter(context.Background(), "192.168.69.120", "administrator@vsphere.local", "N0isystem$")

		TestUtils.Assert(t, err == nil, "Connected to venter")
		TestUtils.Assert(t, vc != nil, "Vencter context set")

		vc.DestroyDataCenter("iota-dc")
		vc.DestroyDataCenter("iota-dc1")

		dc, err := vc.CreateDataCenter("iota-dc")

		TestUtils.Assert(t, err == nil, "successfuly created")
		TestUtils.Assert(t, dc != nil, "Ds not created")

		c, err := dc.CreateCluster("cluster1")
		TestUtils.Assert(t, err == nil, "successfuly created")
		TestUtils.Assert(t, c != nil, "Cluster not created")

		err = c.AddHost("tb36-host1.pensando.io", "root", "pen123!")
		TestUtils.Assert(t, err == nil, "host added") */

	/*
		dc, err = vc1.SetupDataCenter("iota-dc")
		TestUtils.Assert(t, err == nil, "successfuly setup dc")
		dc, ok := vc1.datacenters["iota-dc"]
		TestUtils.Assert(t, ok, "successfuly setup dc")

		//vmInfo, err := host.DeployVM("build-111", 4, 4, []string{"VM Network"}, "/home/sudhiaithal/build-111/")
		vmInfo, err := dc.DeployVM("cluster1", "tb36-host1.pensando.io",
			"build-111", 4, 4, []string{"VM Network"}, "/Users/sudhiaithal/build-114")
		fmt.Printf("Deploy VM error %v", err)
		TestUtils.Assert(t, err == nil, "Deploy VM done")

		fmt.Println("Vm info : ", vmInfo.IP)

		vsname := "test-vs"
		vsspec := VswitchSpec{Name: vsname}
		err = dc.AddVswitch("cluster1", "tb36-host1.pensando.io", vsspec)
		TestUtils.Assert(t, err == nil, "successfuly setup vswitch")

		vswitchs, err := dc.ListVswitches("cluster1", "tb36-host1.pensando.io")
		TestUtils.Assert(t, err == nil, "successfuly setup vswitch")
		fmt.Printf("Vswitches %v\n", vswitchs)

		err = dc.DestoryVM("build-111")
		TestUtils.Assert(t, err == nil, "destroy VM successful")

		cl, ok := dc.clusters["cluster1"]
		err = cl.DeleteHost("tb36-host1.pensando.io")
		TestUtils.Assert(t, err == nil, "delete host successull")

		host, err := NewHost(context.Background(), "tb36-host1.pensando.io", "root", "pen123!")

		TestUtils.Assert(t, err == nil, "Connected to host")
		TestUtils.Assert(t, host != nil, "Host context set")

		vsname = "test-vs"
		vsspec = VswitchSpec{Name: vsname}
		err = host.AddVswitch(vsspec)
		TestUtils.Assert(t, err == nil, "successfuly setup vswitch")

		vswitchs, err = host.ListVswitchs()
		TestUtils.Assert(t, err == nil, "successfuly setup vswitch")
		fmt.Printf("Vswitches %v\n", vswitchs)

		vmInfo, err = host.DeployVM("build-111", 4, 4, []string{"VM Network"}, "/home/sudhiaithal/build-114/")
		fmt.Printf("Deploy VM error %v", err)
		TestUtils.Assert(t, err == nil, "Deploy VM done")

		fmt.Println("Vm info : ", vmInfo.IP)

		err = host.DestoryVM("build-111")

		TestUtils.Assert(t, err == nil, "destroy VM successful")
	*/
	/*
		//vmInfo, err := host.DeployVM("build-111", 4, 4, []string{"VM Network"}, "/home/sudhiaithal/build-111/")
		vmInfo, err := dc.DeployVM("cluster1", "tb36-host1.pensando.io",
			"build-111", 4, 4, []string{}, "/Users/sudhiaithal/build-114")
		fmt.Printf("Deploy VM error %v", err)
		TestUtils.Assert(t, err == nil, "Deploy VM done")

		fmt.Println("Vm info : ", vmInfo.IP)
	*/
	/*
		TestUtils.Assert(t, len(vc1.datacenters) == 1, "successfuly setup dc")
		cluster, ok := dc.clusters["cluster1"]
		TestUtils.Assert(t, ok, "successfuly setup cluster")
		TestUtils.Assert(t, len(cluster.hosts) == 1 && cluster.hosts[0] == "tb36-host1.pensando.io",
			"successfuly setup host cluster")

	*/

	/*

		err = dc.DestroyCluster("cluster1")
		TestUtils.Assert(t, err == nil, "Cluster Destroyed")

		err = vc.DestroyDataCenter("iota-dc")
		TestUtils.Assert(t, err == nil, "Deleted succesfully")
	*/
	//vsname := "test-vs-1"
	//vsspec := VswitchSpec{Name: vsname, Pnics: []string{"vmnic2", "vmnic3"}}
	//err = host.AddVswitch(vsspec)
	//TestUtils.Assert(t, err == nil, "Vss added")

	/*
		nws := []NWSpec{{Name: "pg_test1", Vlan: 1}, {Name: "pg_test2", Vlan: 2}}

		nets, err1 := host.AddNetworks(nws, vsspec)
		TestUtils.Assert(t, err1 == nil, "Pg added")
		TestUtils.Assert(t, len(nets) == len(nws), "Pg added")

		nets = append(nets, "VM Network")
		//vmInfo, err := host.DeployVM("build-111", 4, 4, []string{"VM Network"}, "/home/sudhiaithal/build-111/")
		vmInfo, err := host.DeployVM("build-111", 4, 4, []string{"VM Network", "pg_test1"}, "//Users/sudhiaithal/Downloads/build-111/")
		TestUtils.Assert(t, err == nil, "Deploy VM done")

		fmt.Println("Vm info : ", vmInfo.IP)

		finder, _, err1 := host.client.finder()
		TestUtils.Assert(t, err1 == nil, "Client fined found")
		TestUtils.Assert(t, finder != nil, "Client fined found")

		vm, err2 := finder.VirtualMachine(host.context.context, "build-111")
		TestUtils.Assert(t, err2 == nil, "VM FOUND")
		TestUtils.Assert(t, vm != nil, "VM FOND")

		ip, _ := vm.WaitForIP(host.context.context)
		fmt.Println("IP ADDRESS OF GUEST ", ip)

		hostVM, err3 := host.NewVM("build-111")
		TestUtils.Assert(t, hostVM != nil && err3 == nil, "VM FOND")

		err = hostVM.ReconfigureNetwork("pg_test1", "pg_test2")
		TestUtils.Assert(t, err == nil, "Reconfigured")

		err = host.DestoryVM("build-111")
		TestUtils.Assert(t, err == nil, "Destroy VM done")

		err = host.RemoveNetworks(nws)
		TestUtils.Assert(t, err == nil, "Pg removed")

		TestUtils.Assert(t, err == nil, "Host context set")

	*/
}
