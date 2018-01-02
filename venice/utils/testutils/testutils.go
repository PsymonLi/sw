// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package testutils

import (
	"fmt"
	"path/filepath"
	"reflect"
	"runtime"
	"time"

	"github.com/Sirupsen/logrus"

	"github.com/pensando/sw/venice/utils/testenv"
)

// Evaluator prototype for eveluator function
type Evaluator func() (bool, []interface{})

// TBApi is common T,B interface
type TBApi interface {
	Error(args ...interface{})
	Errorf(format string, args ...interface{})
	Fail()
	FailNow()
	Failed() bool
	Fatal(args ...interface{})
	Fatalf(format string, args ...interface{})
	Log(args ...interface{})
	Logf(format string, args ...interface{})
}

// Assert fails the test if the condition is false.
func Assert(tb TBApi, condition bool, msg string, v ...interface{}) {
	if !condition {
		_, file, line, _ := runtime.Caller(1)
		tb.Fatalf("\033[31m%s:%d: "+msg+"\033[39m\n\n", append([]interface{}{filepath.Base(file), line}, v...)...)
	}
}

// AssertOk fails the test if an err is not nil.
func AssertOk(tb TBApi, err error, msg string) {
	if err != nil {
		_, file, line, _ := runtime.Caller(1)
		tb.Fatalf("\033[31m%s:%d: %s. unexpected error: %s\033[39m\n\n", filepath.Base(file), line, msg, err.Error())
		tb.FailNow()
	}
}

// AssertEquals fails the test if exp is not equal to act.
func AssertEquals(tb TBApi, exp, act interface{}, msg string) {
	if !reflect.DeepEqual(exp, act) {
		_, file, line, _ := runtime.Caller(1)
		tb.Fatalf("\033[31m%s:%d:\n\n\texp: %#v\n\n\tgot: %#v\n\n\tmsg: %#v\n\n\033[39m", filepath.Base(file), line, exp, act, msg)
	}
}

// AssertOneOf fails the test if actual is not one of expected.
func AssertOneOf(tb TBApi, act string, exp []string) {
	for ii := range exp {
		if act == exp[ii] {
			return
		}
	}
	_, file, line, _ := runtime.Caller(1)
	tb.Fatalf("\033[31m%s:%d:\n\n\texp: one of %#v\n\n\tgot: %#v\033[39m\n\n", filepath.Base(file), line, exp, act)
}

// AssertEventually polls evaluator periodically and checks if it reached desired condition
// intervals are pollInterval followed by timeoutInterval in time.ParseDuration() format
func AssertEventually(tb TBApi, eval Evaluator, msg string, intervals ...string) {
	var err error
	pollInterval := time.Millisecond * 10
	timeoutInterval := time.Second * 10

	// parse intervals
	if len(intervals) > 0 {
		pollInterval, err = time.ParseDuration(intervals[0])
		if err != nil {
			tb.Fatalf("%#v is not a valid parsable duration string.", intervals[0])
		}
	}
	if len(intervals) > 1 {
		timeoutInterval, err = time.ParseDuration(intervals[1])
		if err != nil {
			tb.Fatalf("%#v is not a valid parsable duration string.", intervals[1])
		}
	}

	timer := time.Now()
	timeout := time.After(timeoutInterval)

	// loop till we reach timeout interval
	for {
		select {
		case <-timeout:
			// evaluate one last time
			c, strs := eval()
			if c {
				return
			}
			logrus.Errorf("Evaluator timed out after %v", time.Since(timer))
			_, file, line, _ := runtime.Caller(1)
			msg2 := ""
			if len(strs) > 0 {
				msg2 = ": " + fmt.Sprint(strs...)
			}
			tb.Fatalf("\033[31m%s:%d: "+msg+msg2+"\033[39m\n\n",
				append([]interface{}{filepath.Base(file), line})...)
		case <-time.After(pollInterval):
			c, _ := eval()
			if c {
				return
			}
		}
	}
}

// AssertConsistently polls evaluator periodically and checks that the condition
// specified continuously matches until the timeout. intervals are pollInterval
// followed by timeoutInterval in time.ParseDuration() format.
func AssertConsistently(tb TBApi, eval Evaluator, msg string, intervals ...string) {
	var err error
	pollInterval := time.Millisecond * 10
	timeoutInterval := time.Second * 10

	// parse intervals
	if len(intervals) > 0 {
		pollInterval, err = time.ParseDuration(intervals[0])
		if err != nil {
			tb.Fatalf("%#v is not a valid parsable duration string.", intervals[0])
		}
	}
	if len(intervals) > 1 {
		timeoutInterval, err = time.ParseDuration(intervals[1])
		if err != nil {
			tb.Fatalf("%#v is not a valid parsable duration string.", intervals[1])
		}
	}

	_, file, line, _ := runtime.Caller(1)
	timer := time.Now()
	timeout := time.After(timeoutInterval)

	// loop till we reach timeout interval
	for {
		select {
		case <-timeout:
			// evaluate one last time
			c, strs := eval()
			if c {
				return
			}
			logrus.Errorf("Evaluator failed after %v", time.Since(timer))
			msg2 := ""
			if len(strs) > 0 {
				msg2 = ": " + fmt.Sprint(strs...)
			}
			tb.Fatalf("\033[31m%s:%d: "+msg+msg2+"\033[39m\n\n",
				append([]interface{}{filepath.Base(file), line})...)
		case <-time.After(pollInterval):
			c, strs := eval()
			if !c {
				logrus.Errorf("Evaluator failed after %v", time.Since(timer))
				msg2 := ""
				if len(strs) > 0 {
					msg2 = ": " + fmt.Sprint(strs...)
				}
				tb.Fatalf("\033[31m%s:%d: "+msg+msg2+"\033[39m\n\n",
					append([]interface{}{filepath.Base(file), line})...)
			}
			return
		}
	}
}

// CheckEventually polls eveluator periodically and checks if it reached desired condition
// intervals are pollInterval followed by timeoutInterval in time.ParseDuration() format
// returns true if desired state is reached
func CheckEventually(eval Evaluator, intervals ...string) bool {
	var err error
	pollInterval := time.Millisecond * 10
	timeoutInterval := time.Second * 10

	// parse intervals
	if len(intervals) > 0 {
		pollInterval, err = time.ParseDuration(intervals[0])
		if err != nil {
			logrus.Fatalf("%#v is not a valid parsable duration string.", intervals[0])
		}
	}
	if len(intervals) > 1 {
		timeoutInterval, err = time.ParseDuration(intervals[1])
		if err != nil {
			logrus.Fatalf("%#v is not a valid parsable duration string.", intervals[1])
		}
	}

	timer := time.Now()
	timeout := time.After(timeoutInterval)

	// loop till we reach timeout interval
	for {
		select {
		case <-time.After(pollInterval):
			c, _ := eval()
			if c {
				return true
			}
		case <-timeout:
			// eveluate one last time
			c, strs := eval()
			if !c {
				_, file, line, _ := runtime.Caller(1)

				msg2 := ""
				if len(strs) > 0 {
					msg2 = ": " + fmt.Sprint(strs...)
				}

				logrus.Errorf("%s:%d: Evaluator timed out after %v", filepath.Base(file), line, time.Since(timer))
				logrus.Errorf("%s:%d: "+msg2+"\n\n", append([]interface{}{filepath.Base(file), line})...)
				return false
			}

			return true
		}
	}
}

func init() {
	// Switch rpckit to test mode. This disables TLS by default.
	// Tests can instantiate a custom TLS provider and set it using
	// testenv.SetTestModeTLSProvider()
	testenv.EnableRpckitTestMode()
}
