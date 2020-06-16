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
	"github.com/satori/go.uuid"

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

// helper function to convert HAL alert to venice alert
func convertToVeniceAlert(nEvt *operdapi.Alert) *evtsapi.Event {
	uuid := uuid.NewV4().String()
	// TODO: Timestamp is not populated by pdsagent
	// until then use current time instead of nEvt.GetTimestamp()
	ts := gogoproto.TimestampNow()

	evtType := eventtypes.EventType_value[nEvt.GetName()]
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

func queryAlerts(ctx context.Context, evtsDispatcher events.Dispatcher, stream operdapi.AlertsSvc_AlertsGetClient) {
	for {
		resp, err := stream.Recv()
		if err == io.EOF {
			break
		}
		select {
		case <-ctx.Done():
			log.Info("Closing alerts query with pen-oper")
			return
		default:
		}
		if err != nil {
			log.Error(errors.Wrapf(types.ErrAlertsRecv,
				"Error receiving alerts | Err %v", err))
			continue
		}
		if resp.GetApiStatus() != halapi.ApiStatus_API_STATUS_OK {
			log.Error(errors.Wrapf(types.ErrAlertsRecv,
				"Alerts response failure, | Err %v",
				resp.GetApiStatus().String()))
			continue
		}
		// process the alerts
		alert := resp.GetResponse()
		processAlert(alert)
		// convert to venice alert format
		vAlert := convertToVeniceAlert(alert)
		// send to dispatcher
		if err := evtsDispatcher.Action(*vAlert); err != nil {
			log.Error(errors.Wrapf(types.ErrAlertsFwd,
				"Error forwarding alert {%+v} to venice, err: %v", alert, err))
			return
		} else {
			log.Infof("Forwarded alert %+v to venice", alert)
		}
	}
	return
}

// HandleAlerts handles collecting and reporting of alerts
func HandleAlerts(ctx context.Context, evtsDispatcher events.Dispatcher, client operdapi.AlertsSvcClient) {
	emptyStruct := &halapi.Empty{}
	// create a stream for alerts
	alertsStream, err := client.AlertsGet(ctx, emptyStruct)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrAlertsGet,
			"AlertsGet failure | Err %v", err))
		return
	}

	// periodically query for alerts from pen_oper plugin
	go func(stream operdapi.AlertsSvc_AlertsGetClient) {
		ticker := time.NewTicker(time.Second * 5)

		for {
			select {
			case <-ctx.Done():
				log.Info("Closing periodic alerts query with pen-oper")
				return
			case <-ticker.C:
				log.Info("Querying alerts")
				penOperURL := fmt.Sprintf("127.0.0.1:%s", types.PenOperGRPCDefaultPort)
				if utils.IsServerUp(penOperURL) == false {
					// pen_oper is not up yet, skip querying
					continue
				}
				queryAlerts(ctx, evtsDispatcher, stream)
			}
		}
	}(alertsStream)
	return
}

func processAlert(nEvt *operdapi.Alert) {
	// TODO: May need further handling if processes are restartable
	if nEvt.GetName() == "NAPLES_SERVICE_STOPPED" {
		svcName := strings.Split(nEvt.GetMessage(), ":")
		UnhealthyServices = append(UnhealthyServices, svcName[1])
	}
}
