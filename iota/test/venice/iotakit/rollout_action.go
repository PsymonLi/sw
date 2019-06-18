// {C} Copyright 2019 Pensando Systems Inc. All rights reserved.

package iotakit

import (
	"bytes"
	"context"
	"crypto/tls"
	"fmt"
	"io/ioutil"
	"mime/multipart"
	"net/http"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/rollout"
	loginctx "github.com/pensando/sw/api/login/context"

	//iota "github.com/pensando/sw/iota/protos/gogen"
	"github.com/pensando/sw/venice/utils/log"
)

// PerformImageUpload triggers image upgrade
func (act *ActionCtx) PerformImageUpload() error {

	fileBuf, err := ioutil.ReadFile("/go/src/github.com/pensando/sw/bin/upgrade-bundle/bundle.tar")
	if err != nil {
		log.Infof("Error (%+v) reading file /go/src/github.com/pensando/sw/bin/upgrade-bundle/bundle.tar", err)
		return fmt.Errorf("Error (%+v) reading file /go/src/github.com/pensando/sw/bin/upgrade-bundle/bundle.tar", err)
	}
	bkCtx, cancelFunc := context.WithTimeout(context.Background(), 10*time.Minute)
	defer cancelFunc()
	ctx, err := act.model.tb.VeniceLoggedInCtx(bkCtx)
	if err != nil {
		return err
	}
	_, err = act.UploadBundle(ctx, "bundle.tar", fileBuf)
	if err != nil {
		return err
	}

	return nil
}

// UploadBundle performs a rollout in the cluster
func (act *ActionCtx) UploadBundle(ctx context.Context, filename string, content []byte) (int, error) {

	body := &bytes.Buffer{}
	writer := multipart.NewWriter(body)
	part, err := writer.CreateFormFile("file", filename)
	if err != nil {
		return 0, fmt.Errorf("CreateFormFile failed %v", err)
	}
	written, err := part.Write(content)
	if err != nil {
		return 0, fmt.Errorf("writing form %v", err)
	}

	err = writer.Close()
	if err != nil {
		return 0, fmt.Errorf("closing writer %v", err)
	}
	uri := fmt.Sprintf("https://%s/objstore/v1/uploads/images/", act.model.tb.GetVeniceURL()[0])
	req, err := http.NewRequest("POST", uri, body)
	if err != nil {
		return 0, fmt.Errorf("http.newRequest failed %v", err)
	}
	authzHeader, ok := loginctx.AuthzHeaderFromContext(ctx)
	if !ok {
		return 0, fmt.Errorf("no authorization header in context")
	}
	req.Header.Set("Authorization", authzHeader)
	req.Header.Set("Content-Type", writer.FormDataContentType())
	transport := &http.Transport{TLSClientConfig: &tls.Config{InsecureSkipVerify: true}}
	client := &http.Client{Transport: transport}
	resp, err := client.Do(req)
	if err != nil {
		return 0, fmt.Errorf("Sending req %v", err)
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		body, _ := ioutil.ReadAll(resp.Body)
		log.Errorf("Did not get a success on upload [%v]", string(body))
		return 0, fmt.Errorf("failed to get upload [%v][%v]", resp.Status, string(body))
	}
	return written, nil

}

// VerifyRolloutStatus verifies status of rollout in the iota cluster
func (act *ActionCtx) VerifyRolloutStatus(rolloutName string) error {
	var numRetries int
	bkCtx, cancelFunc := context.WithTimeout(context.Background(), 60*time.Minute)
	defer cancelFunc()
	ctx, err := act.model.tb.VeniceLoggedInCtx(bkCtx)

	if err != nil {
		return err
	}

	restcls, err := act.model.tb.VeniceRestClient()
	if err != nil {
		return err
	}

	for numRetries = 0; numRetries < 24; numRetries++ {
		obj := api.ObjectMeta{Name: rolloutName, Tenant: "default"}
		r1, err := restcls[0].RolloutV1().Rollout().Get(ctx, &obj)
		if err != nil || r1.Name != rolloutName {
			log.Errorf("ts:%s Rollout GET failed for [%s] err: %+v r1: %+v", time.Now().String(), rolloutName, err, r1)
			time.Sleep(time.Second)
			continue
		}
		log.Infof("ts:%s Rollout GET validated for [%s]", time.Now().String(), rolloutName)
		numRetries = 0
		break
	}

	if numRetries != 0 {
		return fmt.Errorf("Failed to create rollout object")
	}

	// Verify pre-install rollout Node status
	for numRetries = 0; numRetries < 60; numRetries++ {
		obj := api.ObjectMeta{Name: rolloutName, Tenant: "default"}
		rollout, err := restcls[0].RolloutV1().Rollout().Get(ctx, &obj)
		if err != nil {
			log.Errorf("ts:%s Rollout GET failed for status check, err: %+v rollouts: %+v", time.Now().String(), err, rollout)
			time.Sleep(time.Second * 5)
			continue
		}
		status := rollout.Status.GetControllerNodesStatus()
		if len(status) == 0 {
			log.Errorf("ts:%s Pre-install in progress", time.Now().String())
			time.Sleep(time.Second * 5)
			continue
		}
		log.Infof("ts:%s Pre-install completed for : %d nodes", time.Now().String(), len(status))
		numRetries = 0
		break
	}
	if numRetries != 0 {
		return fmt.Errorf("rollout pre-check failed")
	}

	// Verify rollout Node status
	for numRetries = 0; numRetries < 60; numRetries++ {
		obj := api.ObjectMeta{Name: rolloutName, Tenant: "default"}
		rollout, err := restcls[0].RolloutV1().Rollout().Get(ctx, &obj)
		if err != nil {
			log.Errorf("ts:%s Rollout GET failed for status check, err: %+v rollouts: %+v", time.Now().String(), err, rollout)
			time.Sleep(time.Second * 5)
			continue
		}
		var numNodes int
		status := rollout.Status.GetControllerNodesStatus()
		if len(status) == 0 {
			log.Errorf("ts:%s Pre-install in progress", time.Now().String())
			time.Sleep(time.Second * 5)
			continue
		}

		for i := 0; i < len(status); i++ {
			if status[i].Phase != "WAITING_FOR_TURN" {
				log.Errorf("ts:%s Controller node Pre-install Failed", time.Now().String())
				time.Sleep(time.Second * 5)
				continue
			}
			numNodes++
		}

		if numNodes != len(act.model.VeniceNodes().nodes) {
			log.Errorf("ts:%s Pre-install completed for : %d nodes", time.Now().String(), numNodes)
			time.Sleep(time.Second * 5)
			continue
		}
		log.Infof("ts:%s Pre-install Status: Complete", time.Now().String())
		numRetries = 0
		break
	}
	if numRetries != 0 {
		return fmt.Errorf("rollout pre-check not completed for all nodes")
	}

	// Verify Start Time of rollout object
	for numRetries = 0; numRetries < 20; numRetries++ {
		obj := api.ObjectMeta{Name: rolloutName, Tenant: "default"}
		r1, err := restcls[0].RolloutV1().Rollout().Get(ctx, &obj)
		if err != nil || r1.Name != rolloutName {
			log.Infof("ts:%s Rollout GET failed for [%s] err: %+v r1: %+v", time.Now().String(), rolloutName, err, r1)
			time.Sleep(time.Second * 5)
			continue
		}
		if r1.Status.StartTime == nil || r1.Spec.ScheduledStartTime.Seconds > r1.Status.StartTime.Seconds {
			log.Infof("ts:%s Waiting for pre-install to complete and to schedule rollout : %s", time.Now().String(), rolloutName)
			time.Sleep(time.Second * 5)
			continue
		}
		log.Infof("ts:%s Rollout successfully started at [%+v] for [%s]", time.Now().String(), time.Unix(r1.Status.StartTime.Seconds, 0), rolloutName)
		numRetries = 0
		break
	}
	if numRetries != 0 {
		return fmt.Errorf("rollout failed to trigger")
	}

	// Verify rollout Node status
	for numRetries = 0; numRetries < 100; numRetries++ {
		obj := api.ObjectMeta{Name: rolloutName, Tenant: "default"}
		rollout, err := restcls[0].RolloutV1().Rollout().Get(ctx, &obj)
		if err != nil {
			log.Infof("ts:%s Rollout GET failed for status check, err: %+v rollouts: %+v", time.Now().String(), err, rollout)
			time.Sleep(time.Second * 5)
			continue
		}

		status := rollout.Status.GetControllerNodesStatus()
		var numNodes int
		if len(status) == 0 {
			log.Infof("ts:%s Rollout controller node status: not found", time.Now().String())
			time.Sleep(time.Second * 5)
			continue
		}

		for i := 0; i < len(status); i++ {
			if status[i].Phase == "COMPLETE" {
				numNodes++
			}
		}
		if numNodes != len(status) {
			log.Infof("ts:%s Rollout completed for : %d nodes", time.Now().String(), numNodes)
			time.Sleep(time.Second * 5)
			continue
		}
		log.Infof("ts:%s Rollout Node Status: Complete", time.Now().String())
		numRetries = 0
		break
	}
	if numRetries != 0 {
		return fmt.Errorf("rollout controller node failed")
	}

	// Verify rollout service status
	for numRetries = 0; numRetries < 180; numRetries++ {
		obj := api.ObjectMeta{Name: rolloutName, Tenant: "default"}
		rollout, err := restcls[0].RolloutV1().Rollout().Get(ctx, &obj)
		if err != nil {
			log.Infof("ts:%s Rollout LIST failed for status check, err: %+v rollouts: %+v", time.Now().String(), err, rollout)
			time.Sleep(time.Second * 5)
			continue
		}
		status := rollout.Status.GetControllerServicesStatus()
		if len(status) == 0 {
			log.Infof("ts:%s Rollout controller services status: Not Found", time.Now().String())
			time.Sleep(time.Second * 5)
			continue
		}
		if status[0].Phase != "COMPLETE" {
			log.Infof("ts:%s Rollout controller services status: : %+v", time.Now().String(), status[0].Phase)
			time.Sleep(time.Second * 5)
			continue
		}
		numRetries = 0
		log.Infof("ts:%s Rollout Controller Services Status: Complete", time.Now().String())
		break
	}
	if numRetries != 0 {
		return fmt.Errorf("rollout services failed on some nodes")
	}

	// Verify rollout smartNIC status
	for numRetries = 0; numRetries < 60; numRetries++ {
		restcls, err := act.model.tb.VeniceRestClient()
		if err != nil {
			log.Infof("ts:%s Failed to get restclient err %+v", time.Now().String(), err)
			time.Sleep(time.Second * 5)
			continue
		}
		obj := api.ObjectMeta{Name: rolloutName, Tenant: "default"}
		rollout, err := restcls[0].RolloutV1().Rollout().Get(ctx, &obj)
		if err != nil {
			log.Infof("ts:%s Rollout LIST failed for status check, err: %+v rollouts: %+v", time.Now().String(), err, rollout)
			time.Sleep(time.Second * 5)
			continue
		}
		status := rollout.Status.GetSmartNICsStatus()
		log.Infof("ts:%s Rollout smartNIC status len %d: status:  %+v", time.Now().String(), len(status), status)
		var numNodes int
		if len(status) == 0 {
			log.Infof("ts:%s Rollout smartNIC status: not found", time.Now().String())
			time.Sleep(time.Second * 5)
			continue
		}

		for i := 0; i < len(status); i++ {
			log.Infof("ts:%s SmartNIC Rollout status %s", time.Now().String(), status[i].Phase)
			if status[i].Phase == "COMPLETE" {
				numNodes++
			}
		}
		if numNodes != len(status) {
			log.Infof("ts:%s SmartNIC Rollout completed for : %d nodes", time.Now().String(), numNodes)
			time.Sleep(time.Second * 5)
			continue
		}
		log.Infof("ts:%s Rollout Smart NIC Status: Complete", time.Now().String())
		numRetries = 0
		break
	}
	if numRetries != 0 {
		return fmt.Errorf("rollout smartNIC node failed")
	}

	// Verify rollout overall status
	for numRetries = 0; numRetries < 60; numRetries++ {
		obj := api.ObjectMeta{Name: rolloutName, Tenant: "default"}
		rollout, err := restcls[0].RolloutV1().Rollout().Get(ctx, &obj)
		if err != nil {
			log.Infof("ts:%s Rollout LIST failed for status check, err: %+v rollouts: %+v", time.Now().String(), err, rollout)
			time.Sleep(time.Second * 5)
			continue
		}
		opState := rollout.Status.OperationalState

		if opState == "PROGRESSING" || opState == "SUSPEND_IN_PROGRESS" {
			log.Infof("ts:%s Overall Rollout status: %s", time.Now().String(), rollout.Status.OperationalState)
			time.Sleep(time.Second * 5)
			continue
		}

		if opState == "FAILURE" || opState == "DEADLINE_EXCEEDED" {
			log.Infof("ts:%s Overall Rollout status: %s", time.Now().String(), rollout.Status.OperationalState)
			return fmt.Errorf("rollout smartNIC node failed")
		}

		if opState == "SUCCESS" || opState == "SUSPENDED" {
			log.Infof("ts:%s Overall Rollout status: %s", time.Now().String(), rollout.Status.OperationalState)
			break
		}

		log.Infof("ts:%s Overall Rollout Status: %s", time.Now().String(), opState)
		numRetries = 0
		break
	}

	// Verify delete on rollout object
	for numRetries = 0; numRetries < 25; numRetries++ {
		obj := api.ObjectMeta{Name: rolloutName, Tenant: "default"}
		r1, err := restcls[0].RolloutV1().Rollout().Delete(ctx, &obj)
		if err != nil || r1.Name != rolloutName {
			time.Sleep(time.Second)
			continue
		}
		log.Infof("ts:%s Rollout DELETE validated for [%s]", time.Now().String(), rolloutName)
		numRetries = 0
		break
	}
	if numRetries != 0 {
		return fmt.Errorf("rollout delete object failed")
	}

	// Verify GET for all rollout objects (LIST) returns empty
	for numRetries = 0; numRetries < 25; numRetries++ {
		ometa := api.ObjectMeta{Tenant: "default"}
		rollouts, err := restcls[0].RolloutV1().Rollout().List(ctx, &api.ListWatchOptions{ObjectMeta: ometa})
		if err != nil || len(rollouts) != 0 {
			log.Infof("ts:%s Rollout LIST has unexpected objects, err: %+v rollouts: %+v", time.Now().String(), err, rollouts)
			time.Sleep(time.Second)
			continue
		}
		numRetries = 0
		break
	}
	if numRetries != 0 {
		return fmt.Errorf("Error: rollout list returned objects")
	}
	return nil
}

// PerformRollout performs a rollout in the cluster
func (act *ActionCtx) PerformRollout(rollout *rollout.Rollout) error {
	bkCtx, cancelFunc := context.WithTimeout(context.Background(), 15*time.Minute)
	defer cancelFunc()
	ctx, err := act.model.tb.VeniceLoggedInCtx(bkCtx)
	if err != nil {
		return err
	}

	restcls, err := act.model.tb.VeniceRestClient()
	if err != nil {
		return err
	}
	//cleanup the existing rollout object with the same name
	//fetch the image and upload
	err = act.PerformImageUpload()
	if err != nil {
		log.Infof("Errored PerformImageUpload")
		return err
	}
	//create the rollout object
	r1, err := restcls[0].RolloutV1().Rollout().CreateRollout(ctx, rollout)
	if err != nil || r1.Name != rollout.Name {
		return err
	}

	return nil
}
