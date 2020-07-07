package main

//
import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"math/rand"
	"os"
	"strconv"
	"testing"

	"github.com/pensando/test-infra-tracker/types"

	. "github.com/pensando/sw/venice/utils/testutils"
)

// test vectors
const (
	test               = "github.com/pensando/sw/psmctl"
	testInvalidPkgName = "github.com/pensando/foo"
	testTargetID       = 12345
	testJobID          = 54321
)

// no tests to run
func TestCoverage_WithNoTestsToRun(t *testing.T) {
	targetID := os.Getenv("TARGET_ID")
	AssertOk(t, os.Setenv("TARGET_ID", strconv.Itoa(testTargetID)), "could not set TARGET_ID")
	tr := TestReport{
		Results: []*Target{
			{
				Name: test,
			},
		},
	}

	tr.runCoverage()
	_, err := tr.reportToJSON()
	AssertOk(t, err, "Could not convert report to json")

	tr.testCoveragePass()
	AssertEquals(t, false, tr.RunFailed, "Expected the run to pass and it failed")
	AssertOk(t, os.Setenv("TARGET_ID", targetID), "could not set TARGET_ID")
}

func TestCoverageFail(t *testing.T) {
	tr := TestReport{
		Results: []*Target{
			{
				Name:     test,
				Coverage: 45.0,
				Error:    ErrTestCovFailed.Error(),
			},
		},
	}
	tr.testCoveragePass()
	AssertEquals(t, true, tr.RunFailed, "expected the run to fail due to low coverage")
}

func TestFilterFailTests(t *testing.T) {
	tr := TestReport{
		RunFailed: true,
		Results: []*Target{
			{
				Name:     "test",
				Coverage: 45.0,
				Error:    ErrTestCovFailed.Error(),
			},
			{
				Name:     "test2",
				Coverage: 88.0,
				Error:    "",
			},
		},
	}
	tr2 := tr.filterFailedTests()
	AssertEquals(t, tr.RunFailed, tr2.RunFailed, "RunFailed should be same after filterFailedTests")
	AssertEquals(t, len(tr2.Results), 1, "Only failed test should be present")
	AssertEquals(t, tr.Results[0], tr2.Results[0], "Only failed test should be present")
}

func TestInvalidPackageName(t *testing.T) {
	tgt := Target{
		Name: testInvalidPkgName,
	}
	var ignoredPackages []string
	err := tgt.test(ignoredPackages)
	AssertEquals(t, ErrTestFailed.Error(), tgt.Error, "expected the test to fail, it passed instead")
	AssertEquals(t, ErrParsingTestOutput, err, "expected the test run to fail and the parser to fail as a result")
}

func TestTestDataDump(t *testing.T) {
	jobForkRepo := os.Getenv("JOB_FORK_REPOSITORY")
	targetID := os.Getenv("TARGET_ID")
	jobID := os.Getenv("JOB_ID")

	AssertOk(t, os.Setenv("JOB_FORK_REPOSITORY", swBaseRepo), "could not set JOB_FORK_REPOSITORY")
	AssertOk(t, os.Setenv("TARGET_ID", strconv.Itoa(testTargetID)), "could not set TARGET_ID")
	AssertOk(t, os.Setenv("JOB_ID", strconv.Itoa(testJobID)), "could not set JOB_ID")

	tr := TestReport{
		Results: []*Target{
			{
				Name:    "github.com/pensando/sw/venice/cmd/validation",
				Reports: []*types.Report{},
			},
		},
	}

	tr.runCoverage()
	testReportFilePath := fmt.Sprintf("/tmp/testing_test_data_dump_%d.json", rand.Int())
	err := tr.dumpTestData(testReportFilePath)
	AssertOk(t, err, "Unable to dump test data to file: %s", testReportFilePath)

	reportRaw, err := ioutil.ReadFile(testReportFilePath)
	AssertOk(t, err, "Unable to reach test data back from file: %s", testReportFilePath)

	unmarshalledReport := types.Reports{}
	err = json.Unmarshal(reportRaw, &unmarshalledReport)
	AssertOk(t, err, "Unable to unmarshal test report from file: %s", testReportFilePath)
	AssertEquals(t, tr.Results[0].Reports, unmarshalledReport.Testcases, "Unexpected reports found in unmarshalled report")

	AssertEquals(t, swBaseRepo, unmarshalledReport.Repository, "Found wrong repo in test data dump")
	Assert(t, unmarshalledReport.TargetID != int32(0), "Found wrong TargetID in test data dump")
	Assert(t, unmarshalledReport.SHA != "", "Found wrong SHA in test data dump")
	Assert(t, unmarshalledReport.SHATitle != "", "Found wrong SHA title in test data dump")
	AssertEquals(t, testbed, unmarshalledReport.Testbed, "Found wrong testbed in test data dump")

	// negative test cases
	AssertOk(t, os.Unsetenv("JOB_FORK_REPOSITORY"), "Could not unset environment variable")
	AssertOk(t, os.Unsetenv("TARGET_ID"), "Could not unset environment variable")
	// empty job repo environment should not dump report
	AssertEquals(t, errNotBaseRepo.Error(), tr.dumpTestData(testReportFilePath).Error(), "expected to fail when JOB_FORK_REPOSITORY env variable is not set")

	// non pensando/sw repo should report failure too
	AssertOk(t, os.Setenv("JOB_FORK_REPOSITORY", "pensando/swtest"), "Could not set environment variable")
	AssertEquals(t, errNotBaseRepo.Error(), tr.dumpTestData(testReportFilePath).Error(), "expected to fail when JOB_FORK_REPOSITORY env variable is not pensando/sw")

	AssertOk(t, os.Setenv("JOB_FORK_REPOSITORY", swBaseRepo), "Could not set environment variable")

	// empty TARGET_ID should fail too
	Assert(t, tr.dumpTestData(testReportFilePath) != nil, "expected to fail when TARGET_ID env variable is not set")

	AssertOk(t, os.Setenv("TARGET_ID", targetID), "Could not set environment variable")
	AssertOk(t, os.Setenv("JOB_FORK_REPOSITORY", jobForkRepo), "Could not set environment variable")
	AssertOk(t, os.Setenv("JOB_ID", jobID), "Could not set environment variable")
}
