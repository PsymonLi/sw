// {C} Copyright 2019 Pensando Systems Inc. All rights reserved.

package smartnic_test

import (
	"flag"
	"os"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"

	"testing"

	"github.com/pensando/sw/iota/test/venice/iotakit"
	"github.com/pensando/sw/venice/utils/log"
)

var testbedParams = flag.String("testbed", "/warmd.json", "testbed params file (i.e warmd.json)")
var topoName = flag.String("topo", "3Venice_3NaplesSim", "topology name")
var debugFlag = flag.Bool("debug", false, "set log level to debug")

// TestSuite : smartnic test suite
type TestSuite struct {
	tb    *iotakit.TestBed  // testbed
	model *iotakit.SysModel // system model
}

var ts *TestSuite

func TestIotaVeniceSmartNic(t *testing.T) {
	// set log level
	cfg := log.GetDefaultConfig("venice-e2e")
	if *debugFlag {
		cfg.Debug = true
		cfg.Filter = log.AllowDebugFilter
	}
	log.SetConfig(cfg)

	if os.Getenv("JOB_ID") == "" {
		log.Warnf("Skipping Iota tests outside warmd environment")
		return
	}
	RegisterFailHandler(Fail)
	RunSpecs(t, "Iota SmartNIC E2E Suite")
}

// BeforeSuite runs before the test suite and sets up the testbed
var _ = BeforeSuite(func() {
	tb, model, err := iotakit.InitSuite(*topoName, *testbedParams)
	Expect(err).ShouldNot(HaveOccurred())

	// verify cluster, workload are in good health
	Eventually(func() error {
		return model.Action().VerifySystemHealth()
	}).Should(Succeed())

	// test suite
	ts = &TestSuite{
		tb:    tb,
		model: model,
	}
})

// AfterSuite handles cleanup after test suite completes
var _ = AfterSuite(func() {
	if ts != nil && ts.tb != nil {
		ts.tb.Cleanup()
		ts.tb.PrintResult()
	}
})
