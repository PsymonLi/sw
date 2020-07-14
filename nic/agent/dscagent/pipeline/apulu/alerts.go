// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

// +build apulu

package apulu

import (
	"context"
	"fmt"
	"io"
	"strings"
	"time"

	gogoproto "github.com/gogo/protobuf/types"
	"github.com/pkg/errors"
	uuid "github.com/satori/go.uuid"

	"github.com/pensando/sw/api"
	evtsapi "github.com/pensando/sw/api/generated/events"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/utils"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	halapi "github.com/pensando/sw/nic/apollo/agent/gen/pds"
	operdapi "github.com/pensando/sw/nic/operd/daemon/gen/operd"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/events"
	"github.com/pensando/sw/venice/utils/log"
)

var UnhealthyServices []string

// helper function to convert operd alert to venice alert
func convertToVeniceAlert(nEvt *operdapi.Alert) *evtsapi.Event {
	uuid := uuid.NewV4().String()
	// TODO: Timestamp is not populated by operd
	// until then use current time instead of nEvt.GetTimestamp()
	ts := gogoproto.TimestampNow()

	evtType, found := eventtypes.EventType_value[nEvt.GetName()]
	if !found {
		log.Errorf("Failed to find event type for alert %s", nEvt.GetName())
		return nil
	}
	eTypeAttrs := eventtypes.GetEventTypeAttrs(eventtypes.EventType(evtType))
	vAlert := &evtsapi.Event{
		TypeMeta: api.TypeMeta{Kind: "Event"},
		ObjectMeta: api.ObjectMeta{
			Name: uuid,
			UUID: uuid,
			CreationTime: api.Timestamp{
				Timestamp: *ts,
			},
			ModTime: api.Timestamp{
				Timestamp: *ts,
			},
			Tenant:    globals.DefaultTenant,
			Namespace: globals.DefaultNamespace,
			SelfLink:  fmt.Sprintf("/events/v1/events/%s", uuid),
			Labels:    map[string]string{"_category": globals.Kind2Category("Event")},
		},
		EventAttributes: evtsapi.EventAttributes{
			Type:     eTypeAttrs.EType,
			Severity: eTypeAttrs.Severity,
			Message:  nEvt.GetMessage(),
			Category: eTypeAttrs.Category,
			// nodename will be populated by Dispatcher if Source is unpopulated
			// Source:   &evtsapi.EventSource{Component: "DSC"},
			Count: 1,
		},
	}

	return vAlert
}

func queryAlerts(evtsDispatcher events.Dispatcher, client operdapi.OperSvcClient) {
	operInfoReqMsg := &operdapi.OperInfoRequest{
		Request: []*operdapi.OperInfoSpec{
			{
				InfoType: operdapi.OperInfoType_OPER_INFO_TYPE_ALERT,
				Action:   operdapi.OperInfoOp_OPER_INFO_OP_SUBSCRIBE,
			},
		},
	}

	// create a stream for alerts
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	stream, err := client.OperInfoSubscribe(ctx)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrAlertsGet, "Alerts subscription failure | Err %v", err))
		return
	}

	// send subscription request for alerts stream
	stream.Send(operInfoReqMsg)
	log.Info("Streaming alerts from pen-oper plugin")
	for {
		resp, err := stream.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			log.Error(errors.Wrapf(types.ErrAlertsRecv,
				"Error receiving alerts | Err %v", err))
			continue
		}
		if resp.GetStatus() != halapi.ApiStatus_API_STATUS_OK {
			log.Error(errors.Wrapf(types.ErrAlertsRecv,
				"Alerts response failure, | Err %v",
				resp.GetStatus().String()))
			continue
		}
		if resp.GetInfoType() != operdapi.OperInfoType_OPER_INFO_TYPE_ALERT {
			log.Error(errors.Wrapf(types.ErrAlertsRecv,
				"Invalid Alerts %v in response",
				resp.GetInfoType().String()))
			continue
		}

		// process the alerts
		alert := resp.GetAlertInfo()
		processAlert(alert)
		// convert to venice alert format
		vAlert := convertToVeniceAlert(alert)
		if vAlert == nil {
			log.Errorf("Failed to convert alert %+v", alert)
			continue
		}
		// send to dispatcher
		if err := evtsDispatcher.Action(*vAlert); err != nil {
			log.Error(errors.Wrapf(types.ErrAlertsFwd,
				"Error dispatching alert {%+v}, err: %v", alert, err))
		} else {
			log.Infof("Dispatched alert %+v", alert)
		}
	}
	return
}

// HandleAlerts handles collecting and reporting of alerts
func HandleAlerts(evtsDispatcher events.Dispatcher, client operdapi.OperSvcClient) {
	// check if pen-oper plugin is up and subscribe for alerts
	startAlertsExport := func() {
		ticker := time.NewTicker(time.Second * 5)
		for {
			select {
			case <-ticker.C:
				penOperURL := fmt.Sprintf("127.0.0.1:%s", types.PenOperGRPCDefaultPort)
				if utils.IsServerUp(penOperURL) == false {
					// wait until pen-oper plugin is up
					continue
				}
				queryAlerts(evtsDispatcher, client)
			}
		}
	}
	go startAlertsExport()
	return
}

func processAlert(nEvt *operdapi.Alert) {
	// TODO: May need further handling if processes are restartable
	if nEvt.GetName() == "DSC_SERVICE_STOPPED" {
		svcName := strings.Split(nEvt.GetMessage(), ":")
		UnhealthyServices = append(UnhealthyServices, svcName[1])
	}
}
