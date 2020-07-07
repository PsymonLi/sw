package utils

import (
	"bufio"
	"bytes"
	"encoding/json"
	"fmt"
	"os"
	"regexp"
	"strconv"
	"strings"
	"time"

	"github.com/pensando/test-infra-tracker/types"
)

const (
	// report should not enforce coverage when there are no test files.
	covIgnorePrefix = "[no test files]"
	// only handle pensando/sw repo
	swPkgPrefix = "github.com/pensando/sw/"
)

//TestEvent is the structure for every test event
type TestEvent struct {
	Time    time.Time
	Action  string
	Package string
	Test    string
	Output  string
	Elapsed float64
}

//ParseOutput structures the output of Parse
type ParseOutput struct {
	TestCases  []*types.Report
	OutputMsgs []string
	Coverage   float64
}

//TestOutputParser is an interface which simplifies the process of adding parsers
type TestOutputParser interface {
	Parse(testOutput []byte) (*ParseOutput, error)
}

//JSONTestOutputParser is a parser for JSON format
type JSONTestOutputParser struct {
}

//Parse go test output and returns individual test information along with console output
func (js *JSONTestOutputParser) Parse(testOutput []byte) (*ParseOutput, error) {

	ret := new(ParseOutput)
	scanner := bufio.NewScanner(bytes.NewReader(testOutput))

	ret.TestCases = []*types.Report{}
	ret.OutputMsgs = []string{}

	targetID, err := strconv.Atoi(os.Getenv("TARGET_ID"))
	if err != nil {
		return ret, fmt.Errorf("while getting target ID: %v", err)
	}
	for scanner.Scan() {
		testEvent := &TestEvent{}
		err := json.Unmarshal(scanner.Bytes(), testEvent)
		if err != nil {
			fmt.Printf("Unable to parse test event: %v, err: %+v", testEvent, err)
			return ret, fmt.Errorf("unable to parse test events, err: %s", err.Error())
		}
		switch testEvent.Action {
		case "pass", "skip", "fail":
			if testEvent.Test == "" {
				// this event is at package level, skip it. No useful information in there.
				continue
			}
			testCase := convertToReport(testEvent)
			ret.TestCases = append(ret.TestCases, testCase)

		case "output":
			if strings.HasPrefix(testEvent.Output, "coverage:") {
				c, err := extractCoveragePercent(testEvent.Output)
				if err != nil {
					fmt.Printf("Unable to extract coverage percentage for package: %s", testEvent.Package)
				} else {
					ret.Coverage = c
				}
			} else if strings.Contains(testEvent.Output, covIgnorePrefix) {
				ret.Coverage = 100.0
			}
			ret.OutputMsgs = append(ret.OutputMsgs, testEvent.Output)

		case "run", "pause", "cont", "bench":
			// no-op, explicitly skipping these actions
		default:
			return ret, fmt.Errorf("unrecognized action: %s found", testEvent.Action)
		}
	}

	// set package level coverage, logURL for all test-cases
	for _, tc := range ret.TestCases {
		tc.Coverage = int32(ret.Coverage)
		tc.LogURL = fmt.Sprintf("http://jobd/logs/%d", targetID)
	}

	return ret, nil
}

//extractCoveragePercent returns coverage percentage
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
	trimmedPkgName := strings.TrimPrefix(event.Package, swPkgPrefix)
	report := &types.Report{
		Name:        trimmedPkgName + "." + event.Test,
		Description: trimmedPkgName + "." + event.Test,
		FinishTime:  event.Time,
		Duration:    int32(event.Elapsed * 1000),
	}
	switch event.Action {
	case "pass":
		report.Result = 1
	case "fail":
		report.Result = -1
	}
	parts := strings.SplitN(trimmedPkgName, "/", 2)
	report.Area = parts[0]
	if len(parts) > 1 {
		report.Subarea = parts[1]
	}
	return report
}
