package jobd

import (
	"fmt"
	"os"
	"testing"
	"time"

	TestUtils "github.com/pensando/sw/venice/utils/testutils"
)

func TestMain(m *testing.M) {

	runTests := m.Run()

	os.Exit(runTests)
}

func Test_jobd_get(t *testing.T) {

	data, err := jobdGet(tracePath)

	TestUtils.Assert(t, err == nil, "Error set")
	TestUtils.Assert(t, data != nil, "data not set")

	traces, err := getTraces()

	TestUtils.Assert(t, err == nil, "Error set")
	TestUtils.Assert(t, traces != nil, "data not set")

	TestUtils.Assert(t, len(traces.Traces) != 0, "data not set")

	tdata, err := getTraceFromID("trace-2628")

	TestUtils.Assert(t, err == nil, "Error set")
	TestUtils.Assert(t, tdata != nil, "data not set")

	status := isTraceExpired("trace-2628")

	TestUtils.Assert(t, status == false, "reservation expired")

	ctx, err := GetJobTestContext("trace-2618")
	TestUtils.Assert(t, err != nil, "Error not set")

	ctx, err = GetJobTestContext("test-2737")
	fmt.Printf("Error %v", err)
	TestUtils.Assert(t, err == nil, "Error set")

	TestUtils.Assert(t, !ctx.IsDone(), "already done")

	ctx.WaitForExpiry()

	TestUtils.Assert(t, ctx.IsDone(), "already done")

}

func Test_jobd_cancel(t *testing.T) {

	ctx, err := GetJobTestContext("test-2737")
	fmt.Printf("Error %v", err)
	TestUtils.Assert(t, err == nil, "Error set")

	TestUtils.Assert(t, !ctx.IsDone(), "already done")

	ctx.Cancel()

	time.Sleep(31 * time.Second)
	TestUtils.Assert(t, !ctx.IsDone(), "already done")

}
