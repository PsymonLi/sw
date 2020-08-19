package utils

import (
	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/log"
)

// RaiseEvent raises an event to venice
func RaiseEvent(eventType eventtypes.EventType, message, dscName string) {
	nic := &cluster.DistributedServiceCard{
		TypeMeta: api.TypeMeta{
			Kind: "DistributedServiceCard",
		},
		ObjectMeta: api.ObjectMeta{
			Name: dscName,
		},
		Spec: cluster.DistributedServiceCardSpec{
			ID: dscName,
		},
	}
	recorder.Event(eventType, message, nic)
	log.Infof("Event raised %s", message)
}
