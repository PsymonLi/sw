package utils

import (
	"encoding/json"

	"github.com/pkg/errors"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/nic/agent/dscagent/types"
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

// ReadDSCStatusObj attempts to read DSCStatusObject and handle the error cases
func ReadDSCStatusObj(i types.InfraAPI) (obj types.DistributedServiceCardStatus, err error) {
	var dat []byte
	if dat, err = i.Read(types.VeniceConfigKind, types.VeniceConfigKey); err == nil {
		if err = json.Unmarshal(dat, &obj); err != nil {
			err = errors.Wrapf(types.ErrUnmarshal, "Err: %v", err)
		}
		return
	}
	err = errors.Wrapf(types.ErrObjNotFound, "Could not read venice config key err: %v", err)
	return
}
