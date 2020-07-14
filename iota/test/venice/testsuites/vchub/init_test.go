// {C} Copyright 2019 Pensando Systems Inc. All rights reserved.

package vchub_test

import (
	"flag"
	"os"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
	. "github.com/pensando/sw/venice/utils/testutils"

	"testing"

	"github.com/pensando/sw/iota/test/venice/iotakit/model"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/objects"
	"github.com/pensando/sw/iota/test/venice/iotakit/testbed"
	"github.com/pensando/sw/venice/utils/log"
)

var testbedParams = flag.String("testbed", "/warmd.json", "testbed params file (i.e warmd.json)")
var topoName = flag.String("topo", "3Venice_Nodes", "topology name")
var debugFlag = flag.Bool("debug", false, "set log level to debug")
var scaleFlag = flag.Bool("scale", false, "enable scale configuration")
var scaleDataFlag = flag.Bool("scale-data", false, "enable datapath scale")

// TestSuite : cluster test suite - all venice only test cases go here
type TestSuite struct {
	tb        *testbed.TestBed        // testbed
	model     model.SysModelInterface // system model
	scaleData bool                    // configuration if connections would need to scale
}

var ts *TestSuite

func TestIotaVcHub(t *testing.T) {
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
	tb, model, err := model.InitSuite(*topoName, *testbedParams, *scaleFlag, *scaleDataFlag)

	if err != nil {
		log.Infof("Error setting up suite %v", err)
	}
	Assert(t, err == nil, "Init suite failed")

	// test suite
	ts = &TestSuite{
		tb:        tb,
		model:     model,
		scaleData: *scaleDataFlag,
		//stress:    *stressFlag,
	}

	RegisterFailHandler(Fail)
	RunSpecsWithCustomReporters(t, "Iota VcHub E2E Suite", []Reporter{model})

}

// BeforeSuite runs before the test suite and sets up the testbed
var _ = BeforeSuite(func() {

	naplesMemPath := os.Getenv("GOPATH") + "/src/github.com/pensando/sw/iota/scripts/naples/ps_mem.py"

	copyNaplesMemUtil := func() {
		ts.model.ForEachNaples(func(nc *objects.NaplesCollection) error {
			_, err := ts.model.RunNaplesCommand(nc,
				"cd /data;rm -f mtrack_out.txt ps_out.txt vmotion_minfo.txt slab_out.txt; touch mtrack_out.txt ; touch ps_out.txt; touch vmotion_minfo.txt; touch slab_out.txt;cd /")
			Expect(err).ShouldNot(HaveOccurred())
			return nil
		})
	}

	resetMemTrack := func() {
		ts.model.ForEachNaples(func(nc *objects.NaplesCollection) error {
			err := ts.model.CopyToNaples(nc, []string{naplesMemPath}, "/data")
			Expect(err).ShouldNot(HaveOccurred())
			return nil
		})
	}

	copyNaplesMemUtil()
	resetMemTrack()
	// verify cluster, workload are in good health
	Eventually(func() error {
		return ts.model.VerifySystemHealth(true)
	}).Should(Succeed())

})

// AfterSuite handles cleanup after test suite completes
var _ = AfterSuite(func() {
	if ts != nil && ts.tb != nil {
		ts.model.Cleanup()
		ts.model.PrintResult()
	}
})
