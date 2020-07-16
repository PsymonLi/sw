package jobd

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"strings"
	"time"

	"github.com/pensando/sw/venice/utils/log"
)

const (
	jobdServer = "http://10.8.50.60:15000"

	tracePath = jobdServer + "/trace/"
)

type jobdTrace struct {
	Trace      string    `json:"trace"`
	User       string    `json:"user"`
	Runtime    time.Time `json:"runtime"`
	Expiretime time.Time `json:"expiretime"`
	Resources  struct {
		CPUs        int         `json:"CPUs"`
		Memory      int         `json:"Memory"`
		PvtNws      []string    `json:"PvtNws"`
		DataNws     interface{} `json:"DataNws"`
		Disk        int         `json:"Disk"`
		Networks    interface{} `json:"Networks"`
		Servers     interface{} `json:"Servers"`
		ServerType  string      `json:"ServerType"`
		NicType     string      `json:"NicType"`
		Constraints struct {
			SameSwitch bool `json:"SameSwitch"`
		} `json:"Constraints"`
	} `json:"resources"`
	Vmware struct {
		Host      string `json:"Host"`
		Datastore string `json:"Datastore"`
	} `json:"vmware"`
	Baremetals []interface{} `json:"baremetals"`
}

type jobdTraces struct {
	Traces []jobdTrace `json:"Traces"`
}

func jobdGet(path string) ([]byte, error) {

	response, err := http.Get(path)
	if err != nil {
		fmt.Printf("The HTTP request failed with error %s\n", err)
		return nil, err
	}
	data, _ := ioutil.ReadAll(response.Body)
	fmt.Println(string(data))

	return data, nil

}

func getTraces() (*jobdTraces, error) {

	resp, err := jobdGet(tracePath)
	if err != nil {
		return nil, err
	}
	traceData := jobdTraces{}
	err = json.Unmarshal(resp, &traceData)
	if err != nil {
		return nil, err
	}

	return &traceData, nil
}

func getTraceFromID(id string) (*jobdTrace, error) {

	resp, err := jobdGet(tracePath)
	if err != nil {
		return nil, err
	}

	traceData := jobdTraces{}
	err = json.Unmarshal(resp, &traceData)
	if err != nil {
		return nil, err
	}

	for _, trace := range traceData.Traces {
		if trace.Trace == id {
			return &trace, nil
		}
	}
	return nil, fmt.Errorf("trace %v not found", id)
}

func isTraceExpired(id string) bool {

	trace, err := getTraceFromID(id)
	if err != nil {
		if strings.Contains(err.Error(), "not found") {
			log.Infof("Trace %v not found, assuming it as expired", id)
			return true
		}
		log.Infof("Error finding trace %v", err.Error())
	}

	log.Infof("trace %v not expired", trace.User)

	return false
}

//GetJobTestContext gets context to be monitored by caller
func GetJobTestContext(testID string) (*TestbedCtx, error) {

	ctx, cancel := context.WithCancel(context.Background())
	tbCtx := &TestbedCtx{id: testID, ctx: ctx, cancel: cancel}

	testID = strings.Replace(testID, "test", "trace", -1)
	_, err := getTraceFromID(testID)
	if err != nil {
		log.Infof("Trace %v not found, assuming it as expired", testID)
		cancel()
		return tbCtx, err
	}

	go func() {
		log.Infof("Started monitoring for reservation %v", testID)
		for true {
			time.Sleep(30 * time.Second)
			if isTraceExpired(testID) {
				log.Infof("Reservation %v expired", testID)
				cancel()
				return
			}
			if tbCtx.IsDone() {
				log.Infof("Testbed %v cancelled from caller", testID)
				//If cancelled by outside
				return
			}
		}
	}()
	return tbCtx, nil
}

//TestbedCtx jobd testbed context
type TestbedCtx struct {
	id     string
	ctx    context.Context
	cancel context.CancelFunc
}

//IsDone returns true if done
func (jctx *TestbedCtx) IsDone() bool {
	select {
	case <-jctx.ctx.Done():
		return true
	default:
		return false
	}
}

//Cancel cancels context
func (jctx *TestbedCtx) Cancel() {
	jctx.cancel()
}

//WaitForExpiry waits for expiry
func (jctx *TestbedCtx) WaitForExpiry() {
	<-jctx.ctx.Done()
	log.Infof("Reservation %v expired", jctx.id)
}
