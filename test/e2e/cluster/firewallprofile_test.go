// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

package cluster

import (
	"context"
	"fmt"
	"time"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"

	"github.com/pensando/sw/api"
	apierrors "github.com/pensando/sw/api/errors"
	cmd "github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/security"
	"github.com/pensando/sw/test/utils"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/netutils"
)

// firewallTestGroup firewall tests
type firewallTestGroup struct {
	suite           *TestSuite
	authAgentClient *netutils.HTTPClient
}

// instantiate test suite
var firewallTg = &firewallTestGroup{}

// setupTest setup test suite
func (fwp *firewallTestGroup) setupTest() {
	fwp.suite = ts
	Eventually(func() error {
		ctx, cancel := context.WithTimeout(ts.tu.MustGetLoggedInContext(context.Background()), 5*time.Second)
		defer cancel()
		var err error
		fwp.authAgentClient, err = utils.GetNodeAuthTokenHTTPClient(ctx, ts.tu.APIGwAddr, []string{"*"})
		return err
	}, 30, 5).Should(BeNil(), "Failed to get node auth token")
	snIf := ts.tu.APIClient.ClusterV1().DistributedServiceCard()
	validateCluster()
	ctx := context.Background()
	validateNICHealth(ctx, snIf, ts.tu.NumNaplesHosts, cmd.ConditionStatus_TRUE)
}

// teardownTest cleans up test suite
func (fwp *firewallTestGroup) teardownTest() {
	for _, nicContainer := range ts.tu.NaplesNodes {
		By(fmt.Sprintf("teardownTest UnPausing NIC container %s", nicContainer))
		ts.tu.LocalCommandOutput(fmt.Sprintf("docker unpause %s", nicContainer))
		time.Sleep(time.Second)
	}
	ctx := context.Background()
	snIf := ts.tu.APIClient.ClusterV1().DistributedServiceCard()
	validateCluster()
	validateNICHealth(ctx, snIf, ts.tu.NumNaplesHosts, cmd.ConditionStatus_TRUE)
	defaultFwp := security.FirewallProfile{
		ObjectMeta: api.ObjectMeta{
			Name:   "default",
			Tenant: "default",
		},
	}
	defaultFwp.Defaults("")
	_, err := fwp.suite.restSvc.SecurityV1().FirewallProfile().Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).ShouldNot(HaveOccurred())

}

// test to check for proper propagation status for unreachable dscs
func (fwp *firewallTestGroup) testFirewallPendingPropagation() {
	ctx := context.Background()
	snIf := ts.tu.APIClient.ClusterV1().DistributedServiceCard()
	updateFwp := security.FirewallProfile{
		ObjectMeta: api.ObjectMeta{
			Name:   "default",
			Tenant: "default",
		},
	}
	updateFwp.Defaults("")
	numNaples := int32(len(ts.tu.NaplesNodes))
	if numNaples == 0 {
		Skip("No DSC's found, skipping propagation test")
	}
	for _, nicContainer := range ts.tu.NaplesNodes {
		By(fmt.Sprintf("Pausing NIC container %s", nicContainer))
		ts.tu.LocalCommandOutput(fmt.Sprintf("docker pause %s", nicContainer))
	}
	validateNICHealth(ctx, snIf, ts.tu.NumNaplesHosts, cmd.ConditionStatus_UNKNOWN)
	By(fmt.Sprintf("Updating the firewallprofile with new values : %v", updateFwp))

	time.Sleep(5 * time.Second)

	_, err := fwp.suite.restSvc.SecurityV1().FirewallProfile().Update(fwp.suite.loggedInCtx, &updateFwp)
	Expect(err).ShouldNot(HaveOccurred())

	By(fmt.Sprintf("Verify the propagation is pending to unreachable DSCs total: %d", numNaples))
	Eventually(func() bool {
		fwpStat, err := fwp.suite.restSvc.SecurityV1().FirewallProfile().Get(fwp.suite.loggedInCtx, &updateFwp.ObjectMeta)
		By(fmt.Sprintf("%s: FirewallProfile paused Status : %v", time.Now().String(), fwpStat.Status))
		Expect(err).ShouldNot(HaveOccurred())
		if fwpStat.Status.PropagationStatus.Pending == numNaples {
			return true
		}
		return false
	}, 120, 1).Should(BeTrue(), "Unreachable DSC Propagation of FirewallProfile failed")
	By(fmt.Sprintf("Verify the propagation is successful to the pending DSCs"))
}

// test to check for default firewallprofile in the system
func (fwp *firewallTestGroup) testFirewallDefaultCheck() {
	defaultFwp := security.FirewallProfile{
		ObjectMeta: api.ObjectMeta{
			Name:   "default",
			Tenant: "default",
		},
	}
	defaultFwp.Defaults("")
	fwps, err := fwp.suite.restSvc.SecurityV1().FirewallProfile().List(fwp.suite.loggedInCtx, &api.ListWatchOptions{})
	Expect(err).ShouldNot(HaveOccurred())
	Expect(len(fwps) != 0).Should(BeTrue(), "No firewall profiles found")
	for index, i := range fwps {
		By(fmt.Sprintf("found fwp[%d]: %+v", index, i))
	}

	// get default fwp
	fwpSystem, err := fwp.suite.restSvc.SecurityV1().FirewallProfile().Get(fwp.suite.loggedInCtx, &defaultFwp.ObjectMeta)
	if err != nil {
		apiErr := apierrors.FromError(err)
		log.Errorf("%v", apiErr)
	}
	Expect(err).ShouldNot(HaveOccurred())
	Expect(fwpSystem.ObjectMeta.Tenant == defaultFwp.ObjectMeta.Tenant).Should(BeTrue(), "failed to get default FirewallProfile")

}

//
// test to update firewallprofile
func (fwp *firewallTestGroup) testFirewallUpdate() {
	numNaples := int32(len(ts.tu.NaplesNodes))
	if numNaples == 0 {
		Skip("No DSC's found, skipping propagation test")
	}
	updateFwp := security.FirewallProfile{
		TypeMeta: api.TypeMeta{Kind: "FirewallProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      "default",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: security.FirewallProfileSpec{
			SessionIdleTimeout:        "3m",
			TCPConnectionSetupTimeout: "1m",
			TCPCloseTimeout:           "3m",
			TCPHalfClosedTimeout:      "3m",
			TCPDropTimeout:            "3m",
			UDPDropTimeout:            "3m",
			ICMPDropTimeout:           "3m",
			DropTimeout:               "3m",
			TcpTimeout:                "3m",
			UdpTimeout:                "3m",
			IcmpTimeout:               "3m",
			TcpHalfOpenSessionLimit:   20000,
			UdpActiveSessionLimit:     20000,
			IcmpActiveSessionLimit:    20000,
			DetectApp:                 true,
		},
	}
	By(fmt.Sprintf("Updating the firewallprofile with new values : %v", updateFwp))
	_, err := fwp.suite.restSvc.SecurityV1().FirewallProfile().Update(fwp.suite.loggedInCtx, &updateFwp)
	Expect(err).ShouldNot(HaveOccurred())

	time.Sleep(time.Second)
	Eventually(func() bool {
		fwpStat, err := fwp.suite.restSvc.SecurityV1().FirewallProfile().Get(fwp.suite.loggedInCtx, &updateFwp.ObjectMeta)
		Expect(err).ShouldNot(HaveOccurred())
		if fwpStat.Status.PropagationStatus.Pending != 0 {
			log.Errorf("FirewallProfile Status : %v", fwpStat.Status)
			return false
		}
		return true
	}, 120, 1).Should(BeTrue(), "Failed to Propagate the FirewallProfile update")

}

// test range validations for firewallprofile
func (fwp *firewallTestGroup) testFirewallSpecValidation() {
	By(fmt.Sprintf("Set invalid values for FirewallProfile spec and check failures"))
	fwpInt := fwp.suite.restSvc.SecurityV1().FirewallProfile()
	defaultFwp := security.FirewallProfile{
		ObjectMeta: api.ObjectMeta{
			Name:   "default",
			Tenant: "default",
		},
	}
	defaultFwp.Defaults("")
	defaultFwp.Spec.SessionIdleTimeout = "29s"
	_, err := fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))
	defaultFwp.Spec.SessionIdleTimeout = "172801s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))

	defaultFwp.Defaults("")
	defaultFwp.Spec.TCPConnectionSetupTimeout = "0s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))
	defaultFwp.Spec.TCPConnectionSetupTimeout = "61s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))

	defaultFwp.Defaults("")
	defaultFwp.Spec.TCPCloseTimeout = "0s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))
	defaultFwp.Spec.TCPCloseTimeout = "301s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))

	defaultFwp.Defaults("")
	defaultFwp.Spec.TCPHalfClosedTimeout = "0s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))
	defaultFwp.Spec.TCPHalfClosedTimeout = "172801s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))

	defaultFwp.Defaults("")
	defaultFwp.Spec.TCPDropTimeout = "0s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))
	defaultFwp.Spec.TCPDropTimeout = "301s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))

	defaultFwp.Defaults("")
	defaultFwp.Spec.UDPDropTimeout = "0s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))
	defaultFwp.Spec.UDPDropTimeout = "172801s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))

	defaultFwp.Defaults("")
	defaultFwp.Spec.ICMPDropTimeout = "0s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))
	defaultFwp.Spec.ICMPDropTimeout = "301s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))

	defaultFwp.Defaults("")
	defaultFwp.Spec.DropTimeout = "0s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))
	defaultFwp.Spec.DropTimeout = "301s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))

	defaultFwp.Defaults("")
	defaultFwp.Spec.TcpTimeout = "0s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))
	defaultFwp.Spec.TcpTimeout = "172801s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))

	defaultFwp.Defaults("")
	defaultFwp.Spec.UdpTimeout = "0s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))
	defaultFwp.Spec.UdpTimeout = "172801s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))

	defaultFwp.Defaults("")
	defaultFwp.Spec.IcmpTimeout = "0s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))
	defaultFwp.Spec.IcmpTimeout = "172801s"
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))

	defaultFwp.Defaults("")
	defaultFwp.Spec.TcpHalfOpenSessionLimit = 128001
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))

	defaultFwp.Defaults("")
	defaultFwp.Spec.UdpActiveSessionLimit = 128001
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))

	defaultFwp.Defaults("")
	defaultFwp.Spec.IcmpActiveSessionLimit = 128001
	_, err = fwpInt.Update(fwp.suite.loggedInCtx, &defaultFwp)
	Expect(err).Should(HaveOccurred())
	By(fmt.Sprintf("Expected error success: %v", apierrors.FromError(err)))
}

// Firewall test suite
var _ = Describe("firewall", func() {
	Context("Firewall Profile tests", func() {
		// setup
		BeforeEach(firewallTg.setupTest)

		// test cases
		It("FirewallProfile get default should succeed", firewallTg.testFirewallDefaultCheck)

		It("FirewallProfile invalid spec validations should succeed", firewallTg.testFirewallSpecValidation)

		It("FirewallProfile update should succeed", firewallTg.testFirewallUpdate)

		It("FirewallProfile pending dsc should report proper propagation", firewallTg.testFirewallPendingPropagation)

		// test cleanup
		AfterEach(firewallTg.teardownTest)
	})
})
