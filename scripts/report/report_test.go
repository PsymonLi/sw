package main

//
import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"math/rand"
	"os"
	"strconv"
	"strings"
	"testing"
	"time"

	"github.com/pensando/test-infra-tracker/types"

	. "github.com/pensando/sw/venice/utils/testutils"
)

// test vectors
const (
	test             = "github.com/pensando/sw/psmctl"
	failedTestStdOut = `{"Time":"2020-05-04T10:46:12.908788-07:00","Action":"run","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestAudienceExtractor_ForHostManagedDSCCerts"}
{"Time":"2020-05-04T10:46:12.909013-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestAudienceExtractor_ForHostManagedDSCCerts","Output":"=== RUN   TestAudienceExtractor_ForHostManagedDSCCerts\n"}
{"Time":"2020-05-04T10:46:12.942813-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestAudienceExtractor_ForHostManagedDSCCerts","Output":"    TestAudienceExtractor_ForHostManagedDSCCerts: testutils.go:48: \u001b[31mauthorizers_test.go:53: Expected to find an empty Audience slice\u001b[39m\n"}
{"Time":"2020-05-04T10:46:12.942841-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestAudienceExtractor_ForHostManagedDSCCerts","Output":"        \n"}
{"Time":"2020-05-04T10:46:12.94288-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestAudienceExtractor_ForHostManagedDSCCerts","Output":"--- FAIL: TestAudienceExtractor_ForHostManagedDSCCerts (0.03s)\n"}
{"Time":"2020-05-04T10:46:12.942892-07:00","Action":"fail","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestAudienceExtractor_ForHostManagedDSCCerts","Elapsed":0.03}
{"Time":"2020-05-04T10:46:12.942903-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"FAIL\n"}
{"Time":"2020-05-04T10:46:12.942922-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"coverage: 41.7% of statements\n"}
{"Time":"2020-05-04T10:46:12.943797-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"exit status 1\n"}
{"Time":"2020-05-04T10:46:12.943828-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"FAIL\tgithub.com/pensando/sw/nic/agent/nmd/state\t0.235s\n"}
{"Time":"2020-05-04T10:46:12.943834-07:00","Action":"fail","Package":"github.com/pensando/sw/nic/agent/nmd/state","Elapsed":0.472}`

	skippedTestStdout = `{"Time":"2020-05-04T10:45:57.1088-07:00","Action":"run","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize"}
{"Time":"2020-05-04T10:45:57.109092-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize","Output":"=== RUN   TestMacBasedAuthorizer_Authorize\n"}
{"Time":"2020-05-04T10:45:57.109103-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize","Output":"    TestMacBasedAuthorizer_Authorize: authorizers_test.go:57: Skip because not in job-ci environment\n"}
{"Time":"2020-05-04T10:45:57.109111-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize","Output":"--- SKIP: TestMacBasedAuthorizer_Authorize (0.00s)\n"}
{"Time":"2020-05-04T10:45:57.109114-07:00","Action":"skip","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize","Elapsed":0}
{"Time":"2020-05-04T10:45:57.109124-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"PASS\n"}
{"Time":"2020-05-04T10:45:57.109127-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"coverage: 0.0% of statements\n"}
{"Time":"2020-05-04T10:45:57.110001-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"ok  \tgithub.com/pensando/sw/nic/agent/nmd/state\t0.201s\n"}
{"Time":"2020-05-04T10:45:57.11005-07:00","Action":"pass","Package":"github.com/pensando/sw/nic/agent/nmd/state","Elapsed":0.201}`

	passedTestOutput = `{"Time":"2020-05-04T10:45:42.341018-07:00","Action":"run","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize"}
{"Time":"2020-05-04T10:45:42.341322-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize","Output":"=== RUN   TestProtectedCommandsAuthorizer_Authorize\n"}
{"Time":"2020-05-04T10:45:42.371546-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize","Output":"--- PASS: TestProtectedCommandsAuthorizer_Authorize (0.03s)\n"}
{"Time":"2020-05-04T10:45:42.371594-07:00","Action":"pass","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize","Elapsed":0.03}
{"Time":"2020-05-04T10:45:42.371616-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"PASS\n"}
{"Time":"2020-05-04T10:45:42.371621-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"coverage: 1.1% of statements\n"}
{"Time":"2020-05-04T10:45:42.372432-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"ok  \tgithub.com/pensando/sw/nic/agent/nmd/state\t0.306s\n"}
{"Time":"2020-05-04T10:45:42.372469-07:00","Action":"pass","Package":"github.com/pensando/sw/nic/agent/nmd/state","Elapsed":0.306}`

	testOutputWithUnrecognizedEvent = `{"Time":"2020-05-04T10:45:42.341018-07:00","Action":"init","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize"}
{"Time":"2020-05-04T10:45:42.341018-07:00","Action":"run","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize"}
{"Time":"2020-05-04T10:45:42.341322-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize","Output":"=== RUN   TestProtectedCommandsAuthorizer_Authorize\n"}
{"Time":"2020-05-04T10:45:42.371546-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize","Output":"--- PASS: TestProtectedCommandsAuthorizer_Authorize (0.03s)\n"}
{"Time":"2020-05-04T10:45:42.371594-07:00","Action":"pass","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize","Elapsed":0.03}
{"Time":"2020-05-04T10:45:42.371616-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"PASS\n"}
{"Time":"2020-05-04T10:45:42.371621-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"coverage: 1.1% of statements\n"}
{"Time":"2020-05-04T10:45:42.372432-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"ok  \tgithub.com/pensando/sw/nic/agent/nmd/state\t0.306s\n"}
{"Time":"2020-05-04T10:45:42.372469-07:00","Action":"pass","Package":"github.com/pensando/sw/nic/agent/nmd/state","Elapsed":0.306}`

	mixedTestResultOutput = `{"Time":"2020-05-04T10:45:42.341018-07:00","Action":"run","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize"}
{"Time":"2020-05-04T10:45:42.341322-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize","Output":"=== RUN   TestProtectedCommandsAuthorizer_Authorize\n"}
{"Time":"2020-05-04T10:45:42.371546-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize","Output":"--- PASS: TestProtectedCommandsAuthorizer_Authorize (0.03s)\n"}
{"Time":"2020-05-04T10:45:42.371594-07:00","Action":"pass","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize","Elapsed":0.03}
{"Time":"2020-05-04T10:45:57.1088-07:00","Action":"run","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize"}
{"Time":"2020-05-04T10:45:57.1088-07:00","Action":"run","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize_Paused"}
{"Time":"2020-05-04T10:45:57.109092-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize","Output":"=== RUN   TestMacBasedAuthorizer_Authorize\n"}
{"Time":"2020-05-04T10:45:57.109103-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize","Output":"    TestMacBasedAuthorizer_Authorize: authorizers_test.go:57: Skip because not in job-ci environment\n"}
{"Time":"2020-05-04T10:45:57.109111-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize","Output":"--- SKIP: TestMacBasedAuthorizer_Authorize (0.00s)\n"}
{"Time":"2020-05-04T10:45:57.109113-07:00","Action":"pause","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize_Paused"}
{"Time":"2020-05-04T10:45:57.109114-07:00","Action":"skip","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize","Elapsed":0}
{"Time":"2020-05-04T10:46:12.908788-07:00","Action":"run","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestAudienceExtractor_ForHostManagedDSCCerts"}
{"Time":"2020-05-04T10:46:12.909013-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestAudienceExtractor_ForHostManagedDSCCerts","Output":"=== RUN   TestAudienceExtractor_ForHostManagedDSCCerts\n"}
{"Time":"2020-05-04T10:46:12.942813-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestAudienceExtractor_ForHostManagedDSCCerts","Output":"    TestAudienceExtractor_ForHostManagedDSCCerts: testutils.go:48: \u001b[31mauthorizers_test.go:53: Expected to find an empty Audience slice\u001b[39m\n"}
{"Time":"2020-05-04T10:46:12.942841-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestAudienceExtractor_ForHostManagedDSCCerts","Output":"        \n"}
{"Time":"2020-05-04T10:46:12.94288-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestAudienceExtractor_ForHostManagedDSCCerts","Output":"--- FAIL: TestAudienceExtractor_ForHostManagedDSCCerts (0.03s)\n"}
{"Time":"2020-05-04T10:46:12.942892-07:00","Action":"fail","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestAudienceExtractor_ForHostManagedDSCCerts","Elapsed":0.03}
{"Time":"2020-05-04T10:46:12.942903-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"FAIL\n"}
{"Time":"2020-05-04T10:46:12.942922-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"coverage: 41.7% of statements\n"}
{"Time":"2020-05-04T10:46:12.943797-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"exit status 1\n"}
{"Time":"2020-05-04T10:46:12.943828-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"FAIL\tgithub.com/pensando/sw/nic/agent/nmd/state\t0.235s\n"}
{"Time":"2020-05-04T10:46:12.943834-07:00","Action":"fail","Package":"github.com/pensando/sw/nic/agent/nmd/state","Elapsed":0.472}`

	testLowCoverageStdout = `coverage: 41.7% of statements`
	testInvalidPkgName    = "github.com/pensando/foo"
	testCovIgnoreZeroCov  = "coverage: 0.0% of statements"
	testTargetID          = 12345
	testJobID             = 54321
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

func TestStdOutParsing_failedTestCase(t *testing.T) {
	targetID := os.Getenv("TARGET_ID")
	AssertOk(t, os.Setenv("TARGET_ID", strconv.Itoa(testTargetID)), "could not set TARGET_ID")
	expectedTestReport := getExpectedFailedTestReport(t)
	reports, _, coverage, err := ParseTestRunOutput([]byte(failedTestStdOut))
	AssertOk(t, err, "parsing stdout failed")
	Assert(t, len(reports) == 1, "expected to find single element in reports slice")
	AssertEquals(t, expectedTestReport, reports[0], "unexpected test report found")
	AssertEquals(t, 41.7, coverage, "did not get expected coverage results")
	AssertOk(t, os.Setenv("TARGET_ID", targetID), "could not set TARGET_ID")
}

func getExpectedFailedTestReport(t *testing.T) *types.Report {
	finishTime, err := time.Parse(time.RFC3339, "2020-05-04T10:46:12.942892-07:00")
	AssertOk(t, err, "time string expected to be parsed successfully")
	targetID, err := strconv.Atoi(os.Getenv("TARGET_ID"))
	AssertOk(t, err, "targetID expected to be set")
	expectedTestReport := &types.Report{
		Name:        "github.com/pensando/sw/nic/agent/nmd/state.TestAudienceExtractor_ForHostManagedDSCCerts",
		Description: "github.com/pensando/sw/nic/agent/nmd/state.TestAudienceExtractor_ForHostManagedDSCCerts",
		Area:        "nic",
		Subarea:     "agent/nmd/state",
		FinishTime:  finishTime,
		Duration:    30,
		Coverage:    41,
		Result:      -1,
		LogURL:      fmt.Sprintf("http://jobd/logs/%d", targetID),
	}
	return expectedTestReport
}

func TestStdOutParsing_SkippedTestCase(t *testing.T) {
	targetID := os.Getenv("TARGET_ID")
	AssertOk(t, os.Setenv("TARGET_ID", strconv.Itoa(testTargetID)), "could not set TARGET_ID")
	expectedTestReport := getExpectedSkippedTestReport(t)
	reports, _, coverage, err := ParseTestRunOutput([]byte(skippedTestStdout))
	AssertOk(t, err, "parsing stdout failed")
	Assert(t, len(reports) == 1, "expected to find single element in reports slice")
	AssertEquals(t, expectedTestReport, reports[0], "unexpected test report found")
	AssertEquals(t, 100.0, coverage, "did not get expected coverage results")
	AssertOk(t, os.Setenv("TARGET_ID", targetID), "could not set TARGET_ID")
}

func getExpectedSkippedTestReport(t *testing.T) *types.Report {
	finishTime, err := time.Parse(time.RFC3339, "2020-05-04T10:45:57.109114-07:00")
	AssertOk(t, err, "time string expected to be parsed successfully")
	targetID, err := strconv.Atoi(os.Getenv("TARGET_ID"))
	AssertOk(t, err, "targetID expected to be set")
	expectedTestReport := &types.Report{
		Name:        "github.com/pensando/sw/nic/agent/nmd/state.TestMacBasedAuthorizer_Authorize",
		Description: "github.com/pensando/sw/nic/agent/nmd/state.TestMacBasedAuthorizer_Authorize",
		Area:        "nic",
		Subarea:     "agent/nmd/state",
		FinishTime:  finishTime,
		Duration:    0,
		Coverage:    100,
		Result:      0,
		LogURL:      fmt.Sprintf("http://jobd/logs/%d", targetID),
	}
	return expectedTestReport
}

func TestStdOutParsing_passedTestCase(t *testing.T) {
	targetID := os.Getenv("TARGET_ID")
	AssertOk(t, os.Setenv("TARGET_ID", strconv.Itoa(testTargetID)), "could not set TARGET_ID")
	expectedTestReport := getExpectedPassedTestReport(t)
	reports, _, coverage, err := ParseTestRunOutput([]byte(passedTestOutput))
	AssertOk(t, err, "parsing stdout failed")
	Assert(t, len(reports) == 1, "expected to find single element in reports slice")
	AssertEquals(t, expectedTestReport, reports[0], "unexpected test report found")
	AssertEquals(t, 1.1, coverage, "did not get expected coverage results")
	AssertOk(t, os.Setenv("TARGET_ID", targetID), "could not set TARGET_ID")
}

func getExpectedPassedTestReport(t *testing.T) *types.Report {
	finishTime, err := time.Parse(time.RFC3339, "2020-05-04T10:45:42.371594-07:00")
	AssertOk(t, err, "time string expected to be parsed successfully")
	targetID, err := strconv.Atoi(os.Getenv("TARGET_ID"))
	AssertOk(t, err, "targetID expected to be set")
	expectedTestReport := &types.Report{
		Name:        "github.com/pensando/sw/nic/agent/nmd/state.TestProtectedCommandsAuthorizer_Authorize",
		Description: "github.com/pensando/sw/nic/agent/nmd/state.TestProtectedCommandsAuthorizer_Authorize",
		Area:        "nic",
		Subarea:     "agent/nmd/state",
		FinishTime:  finishTime,
		Duration:    30,
		Coverage:    1,
		Result:      1,
		LogURL:      fmt.Sprintf("http://jobd/logs/%d", targetID),
	}
	return expectedTestReport
}

func TestStdOutParsing_unrecognizedEvent(t *testing.T) {
	AssertOk(t, os.Setenv("TARGET_ID", strconv.Itoa(testTargetID)), "could not set TARGET_ID")
	_, _, _, err := ParseTestRunOutput([]byte(testOutputWithUnrecognizedEvent))
	AssertError(t, err, "parsing stdout should have failed")
}

func TestStdOutParsing_mixedResultTestCases(t *testing.T) {
	targetID := os.Getenv("TARGET_ID")
	AssertOk(t, os.Setenv("TARGET_ID", strconv.Itoa(testTargetID)), "could not set TARGET_ID")

	expectedSkippedTestReport := getExpectedSkippedTestReport(t)
	expectedSkippedTestReport.Coverage = 41
	expectedFailedTestReport := getExpectedFailedTestReport(t)
	expectedFailedTestReport.Coverage = 41
	expectedPassedTestReport := getExpectedPassedTestReport(t)
	expectedPassedTestReport.Coverage = 41

	pausedTestCaseName := "TestMacBasedAuthorizer_Authorize_Paused"
	Assert(t, strings.Contains(mixedTestResultOutput, pausedTestCaseName), "test case: %s is not present in the test output", pausedTestCaseName)
	reports, _, coverage, err := ParseTestRunOutput([]byte(mixedTestResultOutput))
	AssertOk(t, err, "parsing stdout failed")
	Assert(t, len(reports) == 3, "expected to find single element in reports slice")
	assertReportIsPresent(t, reports, expectedPassedTestReport)
	assertReportIsPresent(t, reports, expectedFailedTestReport)
	assertReportIsPresent(t, reports, expectedSkippedTestReport)
	assertTestIsNotReported(t, reports, pausedTestCaseName)
	AssertEquals(t, 41.7, coverage, "did not get expected coverage results")
	AssertOk(t, os.Setenv("TARGET_ID", targetID), "could not set TARGET_ID")
}

func assertReportIsPresent(t *testing.T, reports []*types.Report, expectedPassedTestReport *types.Report) {
	for _, actualReport := range reports {
		if expectedPassedTestReport.Name == actualReport.Name {
			AssertEquals(t, expectedPassedTestReport, actualReport, "unexpected test report found for test: %s", expectedPassedTestReport.Name)
			return
		}
	}
	FailTest(t, "report not found for test: %s", expectedPassedTestReport.Name)
}

func assertTestIsNotReported(t *testing.T, reports []*types.Report, testName string) {
	for _, report := range reports {
		if testName == report.Name {
			FailTest(t, "test: %s is not expected to be in the report", testName)
		}
	}
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

func TestCoverageExtractor(t *testing.T) {
	coverage, err := extractCoveragePercent(testLowCoverageStdout)
	AssertOk(t, err, "coverage parsing expected to pass")
	AssertEquals(t, 41.7, coverage, "Expected coverage 41.7%% for missing test files")

	coverage, err = extractCoveragePercent(testCovIgnoreZeroCov)
	AssertOk(t, err, "coverage parsing expected to pass")
	AssertEquals(t, 100.0, coverage, "Expected coverage 100%% for missing test files")
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
				Name: "github.com/pensando/sw/venice/cmd/validation",
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
