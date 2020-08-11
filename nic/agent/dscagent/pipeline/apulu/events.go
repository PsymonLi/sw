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

// UnhealthyServices var
var UnhealthyServices []string

// helper function to convert operd event to venice event
func convertToVeniceEvent(nEvt *operdapi.Event) *evtsapi.Event {
	uuid := uuid.NewV4().String()
	// TODO: Timestamp is not populated by operd
	// until then use current time instead of nEvt.GetTimestamp()
	ts := gogoproto.TimestampNow()
	operdEvtType := nEvt.GetType()

	evtType, found := eventtypes.EventType_value[operdapi.EventType_name[int32(operdEvtType)]]
	if !found {
		log.Errorf("Failed to find event type for event %s", operdEvtType.String())
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

func queryEvents(evtsDispatcher events.Dispatcher, client operdapi.OperSvcClient) {
	operInfoReqMsg := &operdapi.OperInfoRequest{
		Request: []*operdapi.OperInfoSpec{
			{
				InfoType: operdapi.OperInfoType_OPER_INFO_TYPE_EVENT,
				Action:   operdapi.OperInfoOp_OPER_INFO_OP_SUBSCRIBE,
			},
		},
	}

	// create a stream for events
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	stream, err := client.OperInfoSubscribe(ctx)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrEventsGet, "Events subscription failure | Err %v", err))
		return
	}

	// send subscription request for events stream
	stream.Send(operInfoReqMsg)
	log.Info("Streaming events from pen-oper plugin")
	for {
		resp, err := stream.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			log.Error(errors.Wrapf(types.ErrEventsRecv,
				"Error receiving events | Err %v", err))
			continue
		}
		if resp.GetStatus() != halapi.ApiStatus_API_STATUS_OK {
			log.Error(errors.Wrapf(types.ErrEventsRecv,
				"Events response failure, | Err %v",
				resp.GetStatus().String()))
			continue
		}
		if resp.GetInfoType() != operdapi.OperInfoType_OPER_INFO_TYPE_EVENT {
			log.Error(errors.Wrapf(types.ErrEventsRecv,
				"Invalid events %v in response",
				resp.GetInfoType().String()))
			continue
		}

		// process the events
		event := resp.GetEventInfo()
		processEvent(event)
		// convert to venice event format
		vAlert := convertToVeniceEvent(event)
		if vAlert == nil {
			log.Errorf("Failed to convert event %+v", event)
			continue
		}
		// send to dispatcher
		if err := evtsDispatcher.Action(*vAlert); err != nil {
			log.Error(errors.Wrapf(types.ErrEventsFwd,
				"Error dispatching event {%+v}, err: %v", event, err))
		} else {
			log.Infof("Dispatched event %+v", event)
		}
	}
	return
}

// HandleEvents handles collecting and reporting of events
func HandleEvents(evtsDispatcher events.Dispatcher, client operdapi.OperSvcClient) {
	// check if pen-oper plugin is up and subscribe for events
	startEventsExport := func() {
		ticker := time.NewTicker(time.Second * 5)
		for {
			select {
			case <-ticker.C:
				penOperURL := fmt.Sprintf("127.0.0.1:%s", types.PenOperGRPCDefaultPort)
				if utils.IsServerUp(penOperURL) == false {
					// wait until pen-oper plugin is up
					continue
				}
				queryEvents(evtsDispatcher, client)
			}
		}
	}
	go startEventsExport()
	return
}

func processEvent(nEvt *operdapi.Event) {
	// TODO: May need further handling if processes are restartable
	if nEvt.GetType() == operdapi.EventType_DSC_SERVICE_STOPPED {
		svcName := strings.Split(nEvt.GetMessage(), ":")
		UnhealthyServices = append(UnhealthyServices, svcName[1])
	}
}
