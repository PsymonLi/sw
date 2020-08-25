package equinix_test

import (
	"fmt"
	"reflect"
	"strings"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/objects"
	"github.com/pensando/sw/venice/utils/log"
	yaml "gopkg.in/yaml.v2"
)

var _ = Describe("LLDP", func() {

	BeforeEach(func() {
		// verify cluster is in good health
		Eventually(func() error {
			return ts.model.VerifyClusterStatus()
		}).Should(Succeed())

	})
	AfterEach(func() {

	})

	Context("LLDP Tests", func() {

		It("LLDP Neighbor", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("LLDP test cases are enabled only for HW naples")
			}
			Eventually(func() error {
				return verifyUplinkNeighbor()
			}).Should(Succeed())
		})

		It("Peer LLDP Neighbor", func() {
			if !ts.tb.HasNaplesHW() {
				Skip("LLDP test cases are enabled only for HW naples")
			}

			// Peer neighbor verificaation.
			// not using eventually as the above test pass ensures uplink interface is up and has neighbor info
			verifyPeerNeighbor()
		})
	})
})

func verifyUplinkNeighbor() error {
	uplinks, err := ts.model.ConfigClient().ListNetworkUplinkInterfaces()
	if err != nil {
		log.Infof("No uplink interfaces to test LLDP")
		return nil
	}

	for _, u := range uplinks {
		neighbor := u.Status.IFUplinkStatus.GetLLDPNeighbor()
		log.Infof("LLDP: Uplink %v neighbor: [%+v]", u.GetName(), neighbor)
		if neighbor.GetChassisID() == "" &&
			neighbor.GetSysName() == "" &&
			neighbor.GetSysDescription() == "" &&
			neighbor.GetPortID() == "" &&
			neighbor.GetPortDescription() == "" {
			return fmt.Errorf("Uplink doesn't have LLDP neighbor (%+v)", u)
		}

	}
	return nil
}

type uplinkStatus struct {
	LldpStatus struct {
		LldpIfStatus struct {
			Name        string `yaml:"ifname"`
			ChassisInfo struct {
				SysName string `yaml:"sysname"`
			} `yaml:"lldpifchassisstatus"`
		} `yaml:"lldpifstatus"`
	} `yaml:"lldpstatus"`
}

type intf struct {
	TypeMeta string `yaml:"typemeta"`
	ObjMeta  string `yaml:"objmeta"`
	Spec     struct {
	} `yaml:"spec"`
	Status struct {
		IfIndex    uint32 `yaml:ifindex`
		OperStatus uint32 `yaml:operstatus`
		IfStatus   struct {
			UplinkStatus uplinkStatus `yaml:"uplinkifstatus"`
		} `yaml:"ifstatus"`
	} `yaml:"status"`
	Stats struct {
	} `yaml:"stats"`
}

func verifyPeerNeighbor() {

	swPorts := ts.model.SwitchPorts()
	err := ts.model.LLDPInfoGet(swPorts)
	Expect(err).Should(Succeed())
	for _, swPort := range swPorts.Ports {
		log.Infof("LLDP Infop Naples %v Port %v Info %#v", swPort.Node, swPort.Port, swPort.LLDP)
	}

	var data intf
	var matchLldpNeighbor = func(output string) bool {
		s := strings.Split(output, "\r\n\r\n---\r\n")
		for _, line := range s {
			data = intf{}
			err = yaml.Unmarshal([]byte(line), &data)
			Expect(err).ShouldNot(HaveOccurred())

			// skip non-uplink interfaces. pdsctl is returning all interfaces for --yaml even type is specified
			if reflect.DeepEqual(&data.Status.IfStatus.UplinkStatus, &uplinkStatus{}) {
				continue
			}

			// skip oob interfaces
			if strings.Contains(data.Status.IfStatus.UplinkStatus.LldpStatus.LldpIfStatus.Name, "oob") {
				continue
			}

			match := false
			for _, p := range swPorts.Ports {
				if data.Status.IfStatus.UplinkStatus.LldpStatus.LldpIfStatus.Name == p.LLDP.PortDesc &&
					data.Status.IfStatus.UplinkStatus.LldpStatus.LldpIfStatus.ChassisInfo.SysName == p.LLDP.SysName {
					//match found
					log.Infof("Uplink %v/%v is found in Peer's neighbor table", data.Status.IfStatus.UplinkStatus.LldpStatus.LldpIfStatus.ChassisInfo.SysName, data.Status.IfStatus.UplinkStatus.LldpStatus.LldpIfStatus.Name)
					match = true
					break
				}
			}
			if match == false {
				// uplink interface is not found in peer's neighbor table
				log.Infof("Didn't find Uplink %v/%v in Peer's neighbor table", data.Status.IfStatus.UplinkStatus.LldpStatus.LldpIfStatus.ChassisInfo.SysName, data.Status.IfStatus.UplinkStatus.LldpStatus.LldpIfStatus.Name)
				return false
			}
		}
		return true
	}

	ts.model.ForEachNaples(func(nc *objects.NaplesCollection) error {
		cmd := "/nic/bin/pdsctl show interface --type uplink --yaml"
		cmdOut, err := ts.model.RunNaplesCommand(nc, cmd)
		Expect(err).ShouldNot(HaveOccurred())
		for _, cmdLine := range cmdOut {
			Expect(matchLldpNeighbor(cmdLine)).Should(BeTrue())
		}
		return nil
	})
}
