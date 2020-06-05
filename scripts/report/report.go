package main

import (
	"bufio"
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"regexp"
	"strconv"
	"strings"
	"time"

	"github.com/pensando/test-infra-tracker/types"
	"github.com/pkg/errors"
)

// minCoverage is the minimum expected coverage for a package
const (
	minCoverage = 75.0
	// report should not enforce coverage when there are no test files.
	covIgnorePrefix = "[no test files]"
	// only handle pensando/sw repo
	swPkgPrefix = "github.com/pensando/sw/"
	swBaseRepo  = "pensando/sw"
	testbed     = "jobd-ci"

	// RedColor string in red
	RedColor = "\u001b[31m%s\u001b[0m\n"
	// GreenColor string in green
	GreenColor = "\u001b[32m%s\u001b[0m\n"
	// BlueColor string in blue
	BlueColor = "\u001b[34m%s\u001b[0m\n"
	// CyanColor string in cyan
	WhiteColor = "\u001b[37m%s\u001b[0m\n"
)

var (
	// ErrTestFailed errors out when there is a compilation failure or test assertions.
	ErrTestFailed = errors.New("test execution failed")

	// ErrTestCovFailed errors out when the coverage percent is < 75.0%
	ErrTestCovFailed = errors.New("coverage tests failed")

	// ErrParsingTestOutput is thrown when the script is unable to parse test output
	ErrParsingTestOutput = errors.New("unable to parse test output")

	errNotBaseRepo = errors.New("not pensando/sw repo")

	covIgnoreFilePath = fmt.Sprintf("%s/src/github.com/pensando/sw/scripts/report/.coverignore", os.Getenv("GOPATH"))
)

// TestReport summarizes all the targets. We capture only the failed tests.
type TestReport struct {
	RunFailed       bool
	Results         []*Target
	IgnoredPackages []string `json:"ignored-packages-for-coverage,omitempty"`
}

// Target holds test execution details.
type Target struct {
	Name        string
	Coverage    float64
	Duration    string
	FailedTests []string
	Error       string
	Reports     []*types.Report `json:"-"`
}

func main() {
	var filePath string
	flag.StringVar(&filePath, "output", "/testcase_result_export/testsuite_results.json", "JSON formatted Test report filepath")
	flag.Parse()
	if len(flag.Args()) < 1 {
		log.Fatalf("report needs at-least one target to run tests.")
	}

	var t TestReport

	// Read the .covignore file
	f, _ := os.Open(covIgnoreFilePath)
	defer f.Close()

	s := bufio.NewScanner(f)
	for s.Scan() {
		t.IgnoredPackages = append(t.IgnoredPackages, s.Text())
	}

	for _, tgt := range flag.Args() {
		p := Target{
			Name: tgt,
		}
		t.Results = append(t.Results, &p)
	}
	t.runCoverage()

	t.testCoveragePass()

	// dump testdata to a file only in jobd CI environment for jobd to pick it up for reporting.
	if isJobdCI() {
		err := t.dumpTestData(filePath)
		if err != nil {
			if err == errNotBaseRepo {
				log.Printf("skipped test dump for repo: %s as reporting is done only for base repo: %s", os.Getenv("JOB_FORK_REPOSITORY"), swBaseRepo)
			} else {
				log.Fatalf("Unable to dump test data to file: %s, error: %s", filePath, err.Error())
			}
		}
	}
	printTestSummary(t)
	if t.RunFailed {
		log.Fatalf("Test(s) failed, check the summary above to find failed tests.")
	}
}

func printTestSummary(t TestReport) {
	fmt.Printf(GreenColor, "Test summary:")
	for _, target := range t.Results {
		for _, report := range target.Reports {
			prettyPrint(report)
		}
	}
}

func prettyPrint(report *types.Report) {
	lineTemplate := "\t%s\t%s\t%d(ms)"
	var testStatus, lineColor string
	switch report.Result {
	case -1:
		testStatus = "FAIL"
		lineColor = RedColor
	case 0:
		testStatus = "SKIP"
		lineColor = BlueColor
	case 1:
		testStatus = "PASS"
		lineColor = GreenColor
	default:
		testStatus = "N/A"
		lineColor = WhiteColor
	}

	lineTxt := fmt.Sprintf(lineTemplate, testStatus, report.Name, report.Duration)
	fmt.Printf(lineColor, lineTxt)
}

func isJobdCI() bool {
	fmt.Printf("JOB_REPOSITORY: %s, JOB_FORK_REPOSITORY: %s, JOB_BASE_REPOSITORY: %s, JOB_ID: %s, TARGET_ID: %s\n",
		os.Getenv("JOB_REPOSITORY"), os.Getenv("JOB_FORK_REPOSITORY"), os.Getenv("JOB_BASE_REPOSITORY"),
		os.Getenv("JOB_ID"), os.Getenv("TARGET_ID"))
	return os.Getenv("JOB_ID") != "" &&
		os.Getenv("TARGET_ID") != "" &&
		os.Getenv("JOB_FORK_REPOSITORY") != ""
}

func (t *TestReport) filterFailedTests() TestReport {
	var retval TestReport
	retval.RunFailed = t.RunFailed
	for _, tgt := range t.Results {
		if tgt.Error != "" {
			retval.Results = append(retval.Results, tgt)
		}
	}
	return retval
}

func (t *TestReport) runCoverage() {
	for _, tgt := range t.Results {
		err := tgt.test(t.IgnoredPackages)
		if err != nil || tgt.Error != "" {
			t.RunFailed = true
		}
	}
}

func (t *TestReport) reportToJSON() ([]byte, error) {
	return json.MarshalIndent(t, "", "  ")
}

func (t *TestReport) testCoveragePass() {
	for _, tgt := range t.Results {
		if tgt.Error == ErrTestCovFailed.Error() {
			log.Println(fmt.Sprintf("\033[31m%s\033[39m", "Insufficient code coverage for the following packages:"))
			log.Println(fmt.Sprintf("\033[31m%s\033[39m", tgt.Name))
			t.RunFailed = true
		}
	}
}

func (tgt *Target) test(ignoredPackages []string) error {
	goTestCmd := "GOPATH=%s VENICE_DEV=1 CGO_LDFLAGS_ALLOW=-I/usr/local/share/libtool go test -json -timeout 20m %s -tags test -p 1 %s"
	cover := "-cover"
	isCovIgnored := tgt.checkCoverageIgnore(ignoredPackages)
	if isCovIgnored {
		cover = "" // no need for coverage for tests for which coverage is ignored
	}
	cmd := fmt.Sprintf(goTestCmd, os.Getenv("GOPATH"), cover, tgt.Name)

	start := time.Now()
	defer func(tgt *Target) {
		tgt.Duration = time.Since(start).String()
	}(tgt)
	out, err := exec.Command("sh", "-c", cmd).CombinedOutput()
	if err != nil {
		tgt.Error = ErrTestFailed.Error()
	}

	reports, consoleOut, coverage, err := ParseTestRunOutput(out)
	if err != nil {
		fmt.Printf("Error parsing test output. Error: %s", err.Error())
		fmt.Println("Printing the output as-is from the test run")
		fmt.Println(string(out))
		return ErrParsingTestOutput
	}
	for _, l := range consoleOut {
		fmt.Println(l)
	}
	tgt.Reports = reports
	tgt.Coverage = coverage
	for _, report := range reports {
		if report.Result == -1 {
			tgt.FailedTests = append(tgt.FailedTests, report.Name)
		}
	}
	if len(tgt.FailedTests) > 0 {
		log.Printf("Test Failure: %v\n", tgt.Name)
		return ErrTestFailed
	}
	if tgt.Coverage < minCoverage && !isCovIgnored {
		tgt.Error = ErrTestCovFailed.Error()
		return ErrTestCovFailed
	}
	return nil
}

//TestEvent is an unmarshalled form of the json formatted test event produced by 'go test'
type TestEvent struct {
	Time    time.Time
	Action  string
	Package string
	Test    string
	Output  string
	Elapsed float64
}

//ParseTestRunOutput parses go test output and returns individual test information along with console output
func ParseTestRunOutput(output []byte) ([]*types.Report, []string, float64, error) {
	scanner := bufio.NewScanner(bytes.NewReader(output))
	testCases := make([]*types.Report, 0)
	outputMsgs := make([]string, 0)
	var coverage float64
	targetID, err := strconv.Atoi(os.Getenv("TARGET_ID"))
	if err != nil {
		return testCases, outputMsgs, coverage, fmt.Errorf("while getting target ID: %v", err)
	}
	for scanner.Scan() {
		testEvent := &TestEvent{}
		err := json.Unmarshal(scanner.Bytes(), testEvent)
		if err != nil {
			fmt.Printf("Unable to parse test event: %v, err: %+v", testEvent, err)
			return testCases, outputMsgs, coverage, fmt.Errorf("unable to parse test events, err: %s", err.Error())
		}
		switch testEvent.Action {
		case "pass", "skip", "fail":
			if testEvent.Test == "" {
				// this event is at package level, skip it. No useful information in there.
				continue
			}
			testCase := convertToReport(testEvent)
			testCases = append(testCases, testCase)

		case "output":
			if strings.HasPrefix(testEvent.Output, "coverage:") {
				c, err := extractCoveragePercent(testEvent.Output)
				if err != nil {
					fmt.Printf("Unable to extract coverage percentage for package: %s", testEvent.Package)
				} else {
					coverage = c
				}
			} else if strings.Contains(testEvent.Output, covIgnorePrefix) {
				coverage = 100.0
			}
			outputMsgs = append(outputMsgs, testEvent.Output)

		case "run", "pause", "cont", "bench":
			// no-op, explicitly skipping these actions
		default:
			return testCases, outputMsgs, coverage, fmt.Errorf("unrecognized action: %s found", testEvent.Action)
		}
	}

	// set package level coverage, logURL for all test-cases
	for _, tc := range testCases {
		tc.Coverage = int32(coverage)
		tc.LogURL = fmt.Sprintf("http://jobd/logs/%d", targetID)
	}

	return testCases, outputMsgs, coverage, nil
}

func extractCoveragePercent(coverageStr string) (float64, error) {
	// The coverage percentage parsing should be > 0.0%
	// Cases include packages which has a *test.go file but doesn't test the main binary.
	// This will also ignore parsing coverage details for the integration tests themselves
	re := regexp.MustCompile("[1-9]+[0-9]*.[0-9]*%")
	v := re.FindString(coverageStr)
	if len(v) <= 0 {
		return 100, nil
	}
	v = strings.TrimSuffix(v, "%")
	f, err := strconv.ParseFloat(v, 64)
	if err != nil {
		return 0, fmt.Errorf("error: %s parsing float", err)
	}
	return f, nil
}

func convertToReport(event *TestEvent) *types.Report {
	report := &types.Report{
		Name:        event.Package + "." + event.Test,
		Description: event.Package + "." + event.Test,
		FinishTime:  event.Time,
		Duration:    int32(event.Elapsed * 1000),
	}
	switch event.Action {
	case "pass":
		report.Result = 1
	case "fail":
		report.Result = -1
	}
	parts := strings.SplitN(strings.TrimPrefix(event.Package, swPkgPrefix), "/", 2)
	report.Area = parts[0]
	if len(parts) > 1 {
		report.Subarea = parts[1]
	}
	return report
}

func (tgt *Target) checkCoverageIgnore(ignoredPackages []string) (mustIgnore bool) {
	for _, i := range ignoredPackages {
		if tgt.Name == i {
			mustIgnore = true
			return
		}
	}
	return
}

func (t *TestReport) dumpTestData(reportFilePath string) error {
	if os.Getenv("JOB_FORK_REPOSITORY") != swBaseRepo {
		return errNotBaseRepo
	}
	targetID, err := strconv.Atoi(os.Getenv("TARGET_ID"))
	if err != nil {
		return errors.Wrap(err, "unable to get target ID")
	}

	content, err := exec.Command("git", "rev-parse", "--short", "HEAD").Output()
	if err != nil {
		return errors.Wrap(err, "unable to extract commitId")
	}
	sha := strings.Trim(string(content), "\n")

	content, err = exec.Command("sh", "-c", `git log --format=%s -n 1 `+sha).CombinedOutput()
	if err != nil {
		return errors.Wrap(err, "unable to extract commit message")
	}
	title := strings.TrimSpace(string(content))

	reports := types.Reports{
		Repository: swBaseRepo,
		Testbed:    testbed,
		SHA:        sha,
		SHATitle:   title,
		TargetID:   int32(targetID),
		Testcases:  []*types.Report{},
	}

	for _, result := range t.Results {
		for _, report := range result.Reports {
			reports.Testcases = append(reports.Testcases, report)
		}
	}

	// dump reports to file
	rawContent, err := json.Marshal(reports)
	if err != nil {
		fmt.Printf("Error marshalling test report, err: %s", err.Error())
		return errors.Wrap(err, "unable to marshal test report")
	}
	err = ioutil.WriteFile(reportFilePath, rawContent, 0644)
	if err != nil {
		return errors.Wrap(err, fmt.Sprintf("failed writing test report to: %s", reportFilePath))
	}
	return nil
}
