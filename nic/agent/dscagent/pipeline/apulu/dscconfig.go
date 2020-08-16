// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

// +build apulu

package apulu

import (
	"github.com/pkg/errors"

	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
)

// HandleDSCConfig handles CRUD operations on DSCConfig
func HandleDSCConfig(infraAPI types.InfraAPI, oper types.Operation, DSCConf netproto.DSCConfig) error {
	switch oper {
	case types.Create:
		fallthrough
	case types.Update:
		return updateDSCConfigHandler(infraAPI, DSCConf)
	case types.Delete:
		return deleteDSCConfigHandler(infraAPI, DSCConf)
	default:
		return errors.Wrapf(types.ErrUnsupportedOp, "Op: %s", oper)
	}
}

func updateDSCConfigHandler(infraAPI types.InfraAPI, DSCConf netproto.DSCConfig) error {
	log.Infof("Received DSC Config: %#v", DSCConf)
	dat, _ := DSCConf.Marshal()
	log.Infof("Storing DSC  config (kind: %v) key: config Data(%v)", DSCConf.Kind, dat)

	if err := infraAPI.Store("DSCConfig", "config", dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "DSCConfig: %s | Error: %v", DSCConf.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreCreate, "DSCConfig: %s | Error: %v", DSCConf.GetKey(), err)
	}
	return nil
}

func deleteDSCConfigHandler(infraAPI types.InfraAPI, DSCConf netproto.DSCConfig) error {
	log.Infof("Deleting DSC  config (kind: %v) key: config ", DSCConf.Kind)

	if err := infraAPI.Delete("DSCConfig", "config"); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreDelete, "DSCConfig: %s | Error: %v", DSCConf.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreDelete, "DSCConfig: %s | Error: %v", DSCConf.GetKey(), err)
	}
	return nil
}
