package utils

import (
	"fmt"
	"os"
	"strconv"
	"strings"
	"testing"
	"time"

	"github.com/pensando/test-infra-tracker/types"

	. "github.com/pensando/sw/venice/utils/testutils"
)

const (
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

	testOutputWithUnrecognizedEvent = `{"Time":"2020-05-04T10:45:42.341018-07:00","Action":"init","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize"}
{"Time":"2020-05-04T10:45:42.341018-07:00","Action":"run","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize"}
{"Time":"2020-05-04T10:45:42.341322-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize","Output":"=== RUN   TestProtectedCommandsAuthorizer_Authorize\n"}
{"Time":"2020-05-04T10:45:42.371546-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize","Output":"--- PASS: TestProtectedCommandsAuthorizer_Authorize (0.03s)\n"}
{"Time":"2020-05-04T10:45:42.371594-07:00","Action":"pass","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize","Elapsed":0.03}
{"Time":"2020-05-04T10:45:42.371616-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"PASS\n"}
{"Time":"2020-05-04T10:45:42.371621-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"coverage: 1.1% of statements\n"}
{"Time":"2020-05-04T10:45:42.372432-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"ok  \tgithub.com/pensando/sw/nic/agent/nmd/state\t0.306s\n"}
{"Time":"2020-05-04T10:45:42.372469-07:00","Action":"pass","Package":"github.com/pensando/sw/nic/agent/nmd/state","Elapsed":0.306}`

	passedTestOutput = `{"Time":"2020-05-04T10:45:42.341018-07:00","Action":"run","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestProtectedCommandsAuthorizer_Authorize"}
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

	skippedTestStdout = `{"Time":"2020-05-04T10:45:57.1088-07:00","Action":"run","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize"}
{"Time":"2020-05-04T10:45:57.109092-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize","Output":"=== RUN   TestMacBasedAuthorizer_Authorize\n"}
{"Time":"2020-05-04T10:45:57.109103-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize","Output":"    TestMacBasedAuthorizer_Authorize: authorizers_test.go:57: Skip because not in job-ci environment\n"}
{"Time":"2020-05-04T10:45:57.109111-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize","Output":"--- SKIP: TestMacBasedAuthorizer_Authorize (0.00s)\n"}
{"Time":"2020-05-04T10:45:57.109114-07:00","Action":"skip","Package":"github.com/pensando/sw/nic/agent/nmd/state","Test":"TestMacBasedAuthorizer_Authorize","Elapsed":0}
{"Time":"2020-05-04T10:45:57.109124-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"PASS\n"}
{"Time":"2020-05-04T10:45:57.109127-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"coverage: 0.0% of statements\n"}
{"Time":"2020-05-04T10:45:57.110001-07:00","Action":"output","Package":"github.com/pensando/sw/nic/agent/nmd/state","Output":"ok  \tgithub.com/pensando/sw/nic/agent/nmd/state\t0.201s\n"}
{"Time":"2020-05-04T10:45:57.11005-07:00","Action":"pass","Package":"github.com/pensando/sw/nic/agent/nmd/state","Elapsed":0.201}`

	testTargetID          = 12345
	testLowCoverageStdout = `coverage: 41.7% of statements`
	testCovIgnoreZeroCov  = "coverage: 0.0% of statements"
)

var (
	jsParse = JSONTestOutputParser{}
)

func TestStdOutParsing_failedTestCase(t *testing.T) {
	targetID := os.Getenv("TARGET_ID")
	AssertOk(t, os.Setenv("TARGET_ID", strconv.Itoa(testTargetID)), "could not set TARGET_ID")
	expectedTestReport := getExpectedFailedTestReport(t)
	parsed, err := jsParse.Parse([]byte(failedTestStdOut))
	AssertOk(t, err, "parsing stdout failed")
	reports := parsed.TestCases
	Assert(t, len(reports) == 1, "expected to find single element in reports slice")
	AssertEquals(t, expectedTestReport, reports[0], "unexpected test report found")
	AssertEquals(t, 41.7, parsed.Coverage, "did not get expected coverage results")
	AssertOk(t, os.Setenv("TARGET_ID", targetID), "could not set TARGET_ID")
}

func TestStdOutParsing_SkippedTestCase(t *testing.T) {
	targetID := os.Getenv("TARGET_ID")
	AssertOk(t, os.Setenv("TARGET_ID", strconv.Itoa(testTargetID)), "could not set TARGET_ID")
	expectedTestReport := getExpectedSkippedTestReport(t)
	parsed, err := jsParse.Parse([]byte(skippedTestStdout))
	AssertOk(t, err, "parsing stdout failed")
	reports := parsed.TestCases
	Assert(t, len(reports) == 1, "expected to find single element in reports slice")
	AssertEquals(t, expectedTestReport, reports[0], "unexpected test report found")
	AssertEquals(t, 100.0, parsed.Coverage, "did not get expected coverage results")
	AssertOk(t, os.Setenv("TARGET_ID", targetID), "could not set TARGET_ID")
}

func TestStdOutParsing_passedTestCase(t *testing.T) {
	targetID := os.Getenv("TARGET_ID")
	AssertOk(t, os.Setenv("TARGET_ID", strconv.Itoa(testTargetID)), "could not set TARGET_ID")
	expectedTestReport := getExpectedPassedTestReport(t)
	parsed, err := jsParse.Parse([]byte(passedTestOutput))
	AssertOk(t, err, "parsing stdout failed")
	reports := parsed.TestCases
	Assert(t, len(reports) == 1, "expected to find single element in reports slice")
	AssertEquals(t, expectedTestReport, reports[0], "unexpected test report found")
	AssertEquals(t, 1.1, parsed.Coverage, "did not get expected coverage results")
	AssertOk(t, os.Setenv("TARGET_ID", targetID), "could not set TARGET_ID")
}

func TestStdOutParsing_unrecognizedEvent(t *testing.T) {
	AssertOk(t, os.Setenv("TARGET_ID", strconv.Itoa(testTargetID)), "could not set TARGET_ID")
	_, err := jsParse.Parse([]byte(testOutputWithUnrecognizedEvent))
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
	parsed, err := jsParse.Parse([]byte(mixedTestResultOutput))
	AssertOk(t, err, "parsing stdout failed")
	reports := parsed.TestCases
	Assert(t, len(reports) == 3, "expected to find single element in reports slice")
	assertReportIsPresent(t, reports, expectedPassedTestReport)
	assertReportIsPresent(t, reports, expectedFailedTestReport)
	assertReportIsPresent(t, reports, expectedSkippedTestReport)
	assertTestIsNotReported(t, reports, pausedTestCaseName)
	AssertEquals(t, 41.7, parsed.Coverage, "did not get expected coverage results")
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

func getExpectedSkippedTestReport(t *testing.T) *types.Report {
	finishTime, err := time.Parse(time.RFC3339, "2020-05-04T10:45:57.109114-07:00")
	AssertOk(t, err, "time string expected to be parsed successfully")
	targetID, err := strconv.Atoi(os.Getenv("TARGET_ID"))
	AssertOk(t, err, "targetID expected to be set")
	expectedTestReport := &types.Report{
		Name:        "nic/agent/nmd/state.TestMacBasedAuthorizer_Authorize",
		Description: "nic/agent/nmd/state.TestMacBasedAuthorizer_Authorize",
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

func getExpectedFailedTestReport(t *testing.T) *types.Report {
	finishTime, err := time.Parse(time.RFC3339, "2020-05-04T10:46:12.942892-07:00")
	AssertOk(t, err, "time string expected to be parsed successfully")
	targetID, err := strconv.Atoi(os.Getenv("TARGET_ID"))
	AssertOk(t, err, "targetID expected to be set")
	expectedTestReport := &types.Report{
		Name:        "nic/agent/nmd/state.TestAudienceExtractor_ForHostManagedDSCCerts",
		Description: "nic/agent/nmd/state.TestAudienceExtractor_ForHostManagedDSCCerts",
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

func getExpectedPassedTestReport(t *testing.T) *types.Report {
	finishTime, err := time.Parse(time.RFC3339, "2020-05-04T10:45:42.371594-07:00")
	AssertOk(t, err, "time string expected to be parsed successfully")
	targetID, err := strconv.Atoi(os.Getenv("TARGET_ID"))
	AssertOk(t, err, "targetID expected to be set")
	expectedTestReport := &types.Report{
		Name:        "nic/agent/nmd/state.TestProtectedCommandsAuthorizer_Authorize",
		Description: "nic/agent/nmd/state.TestProtectedCommandsAuthorizer_Authorize",
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

func TestCoverageExtractor(t *testing.T) {
	coverage, err := extractCoveragePercent(testLowCoverageStdout)
	AssertOk(t, err, "coverage parsing expected to pass")
	AssertEquals(t, 41.7, coverage, "Expected coverage 41.7%% for missing test files")

	coverage, err = extractCoveragePercent(testCovIgnoreZeroCov)
	AssertOk(t, err, "coverage parsing expected to pass")
	AssertEquals(t, 100.0, coverage, "Expected coverage 100%% for missing test files")
}
