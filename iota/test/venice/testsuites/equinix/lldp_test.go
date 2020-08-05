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
			verifyUplinkNeighbor()
		})
	})
})

func verifyUplinkNeighbor() {
	uplinks, err := ts.model.ConfigClient().ListNetworkUplinkInterfaces()
	if err != nil {
		log.Infof("No uplink interfaces to test LLDP")
		return
	}

	for _, u := range uplinks {
		neighbor := u.Status.IFUplinkStatus.GetLLDPNeighbor()
		log.Infof("LLDP: Uplink %v neighbor: [%+v]", u.GetName(), neighbor)
		if neighbor.GetChassisID() == "" &&
			neighbor.GetSysName() == "" &&
			neighbor.GetSysDescription() == "" &&
			neighbor.GetPortID() == "" &&
			neighbor.GetPortDescription() == "" {
			err = fmt.Errorf("Uplink doesn't have LLDP neighbor (%+v)", u)
		}
		Expect(err).Should(Succeed())
	}
}
