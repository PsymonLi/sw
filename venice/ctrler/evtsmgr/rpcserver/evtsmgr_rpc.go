// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

package rpcserver

import (
	"context"
	"sync"

	"github.com/pkg/errors"
	"github.com/satori/go.uuid"

	"github.com/pensando/sw/api"
	evtsapi "github.com/pensando/sw/api/generated/events"
	"github.com/pensando/sw/venice/ctrler/evtsmgr/alertengine"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/elastic"
	"github.com/pensando/sw/venice/utils/log"
)

var (
	once sync.Once
	last string

	indexType = elastic.GetDocType(globals.Events)
)

// EvtsMgrRPCHandler handles all event RPC calls
type EvtsMgrRPCHandler struct {
	esClient    elastic.ESClient
	alertEngine alertengine.Interface
	logger      log.Logger
}

// NewEvtsMgrRPCHandler returns a events RPC handler
func NewEvtsMgrRPCHandler(client elastic.ESClient, alertEngine alertengine.Interface, logger log.Logger) (*EvtsMgrRPCHandler, error) {
	evtsMgrRPCHandler := &EvtsMgrRPCHandler{
		esClient:    client,
		alertEngine: alertEngine,
		logger:      logger,
	}

	return evtsMgrRPCHandler, nil
}

// SendEvents sends the events to elastic by performing index or bulk operation.
// Once indexed, the same list of events is passed to the channel for further
// notification to all the watchers.
// Events library will gather events across tenants.
func (e *EvtsMgrRPCHandler) SendEvents(ctx context.Context, eventList *evtsapi.EventList) (*api.Empty, error) {
	// create a tracking request ID that will be helpful for debugging
	trackingID := uuid.NewV4().String()
	e.logger.Infof("{req: %s} processing req, len: %d", trackingID, len(eventList.Items))
	defer e.logger.Infof("{req: %s} finished processing req", trackingID)

	events := eventList.GetItems()
	if len(events) == 0 {
		return &api.Empty{}, nil
	}

	// index single event; it is costly to perform bulk operation for a single event (doc)
	if len(events) == 1 {
		event := events[0]
		if err := e.esClient.Index(ctx,
			elastic.GetIndex(globals.Events, event.GetTenant()),
			indexType, event.GetUUID(), event); err != nil {
			e.logger.Errorf("error sending event to elastic, err: %v", err)
			return nil, errors.Wrap(err, "error sending event to elastic")
		}
	} else {
		// index bulk events; construct bulk events request
		requests := make([]*elastic.BulkRequest, len(events))
		for i, evt := range events {
			requests[i] = &elastic.BulkRequest{
				RequestType: "index",
				IndexType:   indexType,
				ID:          evt.GetUUID(),
				Obj:         evt,
				Index:       elastic.GetIndex(globals.Events, evt.GetTenant()),
			}
		}

		if bulkResp, err := e.esClient.Bulk(ctx, requests); err != nil {
			e.logger.Errorf("error sending bulk events to elastic, err: %v", err)
			return nil, errors.Wrap(err, "error sending events to elastic")
		} else if len(bulkResp.Failed()) > 0 {
			e.logger.Errorf("bulk operation failed on elastic")
			return nil, errors.New("bulk operation failed on elastic")
		}
	}

	// send events to alert engine for processing
	e.alertEngine.ProcessEvents(trackingID, eventList)

	return &api.Empty{}, nil
}
