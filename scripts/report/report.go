package main

import (
	"bufio"
	"encoding/json"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"strconv"
	"strings"
	"time"

	"github.com/pensando/test-infra-tracker/types"
	"github.com/pkg/errors"

	"github.com/pensando/sw/scripts/utils"
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

	//RedColor for Test Summary
	RedSumm = "\033[1;31m%s\033[0m"
	//GreenColor for Test Summary
	GreenSumm = "\033[0;32m%s\033[0m"
	//BlueColor for Test Summary
	BlueSumm = "\033[0;34m%s\033[0m"
	//CyanColor for Test Summary
	WhiteSumm = "\033[0;36m%s\033[0m"
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

type failTest struct {
	report     *types.Report
	testStatus string
	lineColor  string
}

var failTests = []failTest{}

func printTestSummary(t TestReport) {
	fmt.Printf(GreenColor, "Test summary:")
	var testStatus, lineColor string
	var failNum, passNum, skipNum int
	for _, target := range t.Results {
		for _, report := range target.Reports {
			switch report.Result {
			case -1:
				testStatus = "FAIL"
				lineColor = RedColor
				failNum++
			case 0:
				testStatus = "SKIP"
				lineColor = BlueColor
				skipNum++
			case 1:
				testStatus = "PASS"
				lineColor = GreenColor
				passNum++
			default:
				testStatus = "N/A"
				lineColor = WhiteColor
			}
			if testStatus == "FAIL" {
				var msg failTest
				msg.report = report
				msg.testStatus = testStatus
				msg.lineColor = lineColor
				failTests = append(failTests, msg)
			} else {
				prettyPrint(report, testStatus, lineColor)
			}

		}
	}
	for _, reports := range failTests {
		prettyPrint(reports.report, reports.testStatus, reports.lineColor)
	}
	summaryPrint(passNum, failNum, skipNum)
}

func summaryPrint(passNum int, failNum int, skipNum int) {

	var result, resultColor string

	if failNum == 0 {
		result = "SUCCESS!"
		resultColor = GreenSumm
	} else {
		result = "FAIL!"
		resultColor = RedSumm
	}

	total := failNum + passNum + skipNum
	lineTemplate := "\nRan %d of %d Specs"
	lineTxt := fmt.Sprintf(lineTemplate, total-skipNum, total)

	fmt.Printf(resultColor, lineTxt)
	fmt.Printf(resultColor, "\n"+result)
	fmt.Printf(" -- ")
	fmt.Printf(GreenSumm, strconv.Itoa(passNum)+" Passed ")
	fmt.Printf("| ")
	fmt.Printf(RedSumm, strconv.Itoa(failNum)+" Failed ")
	fmt.Printf("| ")
	fmt.Printf(WhiteSumm, strconv.Itoa(skipNum)+" Skipped ")

}

func prettyPrint(report *types.Report, testStatus string, lineColor string) {
	lineTemplate := "\t%s\t%s\t%d(ms)"
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
	jsParse := utils.JSONTestOutputParser{}
	parsed, err := jsParse.Parse(out)
	if err != nil {
		fmt.Printf("Error parsing test output. Error: %s", err.Error())
		fmt.Println("Printing the output as-is from the test run")
		fmt.Println(string(out))
		return ErrParsingTestOutput
	}

	for _, l := range parsed.OutputMsgs {
		fmt.Println(l)
	}
	tgt.Reports = parsed.TestCases
	tgt.Coverage = parsed.Coverage
	for _, report := range parsed.TestCases {
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
