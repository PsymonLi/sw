package equinix_test

import (
	"fmt"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
	"github.com/pensando/sw/venice/utils/log"
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

			swPorts := ts.model.SwitchPorts()
			err := ts.model.LLDPInfoGet(swPorts)
			Expect(err).Should(Succeed())
			for _, swPort := range swPorts.Ports {
				log.Infof("LLDP Infop Naples %v Port %v Info %#v", swPort.Node, swPort.Port, swPort.LLDP)
			}
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
