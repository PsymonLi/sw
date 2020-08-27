// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

package infra

import (
	"os"
	"path"
	"sync"

	"github.com/pkg/errors"

	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/venice/utils/emstore"
	"github.com/pensando/sw/venice/utils/log"
)

// API implents types.InfraAPI. This is a collection of all infra specific APIs that are common across pipelines
type API struct {
	sync.Mutex
	config                      types.DistributedServiceCardStatus
	primaryDBPath, backupDBPath string
	primaryStore, backupStore   emstore.Emstore
	ifUpdCh                     chan error
	updMap                      map[string]types.UpdateIfEvent
}

// NewInfraAPI returns a new instance of InfraAPI. First db path is interpreted as primary and the second is secondary
func NewInfraAPI(primaryDBPath, backupDBPath string) (*API, error) {
	var i API

	i.primaryDBPath = primaryDBPath
	i.backupDBPath = backupDBPath

	if err := i.InitDB(false); err != nil {
		return nil, err
	}

	i.ifUpdCh = make(chan error, 1)
	i.updMap = make(map[string]types.UpdateIfEvent)

	return &i, nil
}

// List returns a list of objects by kind
func (i *API) List(kind string) ([][]byte, error) {
	return i.primaryStore.RawList(kind)
}

// Read returns a specific object based on key and kind
func (i *API) Read(kind, key string) ([]byte, error) {
	return i.primaryStore.RawRead(kind, key)
}

// Store stores a specific object based on key and kind
func (i *API) Store(kind, key string, data []byte) error {
	go func() {
		if i.backupStore != nil {
			if err := i.backupStore.RawWrite(kind, key, data); err != nil {
				log.Error(errors.Wrapf(types.ErrBackupStoreWrite, "Infra API: Err: %v", err))
			}
		}
	}()

	return i.primaryStore.RawWrite(kind, key, data)
}

// Delete deletes a specific object based on key and kind
func (i *API) Delete(kind, key string) error {
	go func() {
		if i.backupStore != nil {
			if err := i.backupStore.RawDelete(kind, key); err != nil {
				log.Error(errors.Wrapf(types.ErrBackupStoreDelete, "Infra API: Err: %v", err))
			}
		}
	}()

	return i.primaryStore.RawDelete(kind, key)
}

// AllocateID allocates a uint64 ID based on an offset.
func (i *API) AllocateID(kind emstore.ResourceType, offset int) uint64 {
	id, err := i.primaryStore.GetNextID(kind, offset)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrPrimaryStoreIDAlloc, "Infra API: Err: %v", err))
	}

	go func() {
		if i.backupStore != nil {
			_, err := i.backupStore.GetNextID(kind, offset)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrBackupStoreIDAlloc, "Infra API: Err: %v", err))
			}
		}
	}()
	return id
}

// GetDscName returns the DSC Name. Cluster-wide uniqueness is ensured by using the primary mac address from FRU
func (i *API) GetDscName() string {
	i.Lock()
	defer i.Unlock()
	return i.config.DSCName
}

// StoreConfig stores NetAgent config that it got from NMD
func (i *API) StoreConfig(config types.DistributedServiceCardStatus) {
	i.Lock()
	defer i.Unlock()
	i.config = config
}

// GetConfig stores NetAgent config that it got from NMD
func (i *API) GetConfig() types.DistributedServiceCardStatus {
	i.Lock()
	defer i.Unlock()
	return i.config
}

// openDB creates a new primary/backup DB
func (i *API) openDB() error {
	var err error
	if len(i.primaryDBPath) == 0 || len(i.backupDBPath) == 0 {
		log.Error(errors.Wrap(types.ErrMissingStorePaths, "Infra API: "))
		return errors.Wrap(types.ErrMissingStorePaths, "Infra API: ")
	}

	if _, err := os.Stat(i.primaryDBPath); os.IsNotExist(err) {
		if err := os.MkdirAll(path.Dir(i.primaryDBPath), 600); err != nil {
			log.Error(errors.Wrapf(types.ErrDBPathCreate, "Infra API: Path: %s | Err: %v", i.primaryDBPath, err))
			return errors.Wrapf(types.ErrDBPathCreate, "Infra API: Path: %s | Err: %v", i.primaryDBPath, err)
		}
	}

	if _, err := os.Stat(i.backupDBPath); os.IsNotExist(err) {
		if err := os.MkdirAll(path.Dir(i.backupDBPath), 600); err != nil {
			log.Error(errors.Wrapf(types.ErrDBPathCreate, "Infra API: Path: %s | Err: %v", i.backupDBPath, err))
			return errors.Wrapf(types.ErrDBPathCreate, "Infra API: Path: %s | Err: %v", i.backupDBPath, err)
		}
	}

	i.primaryStore, err = emstore.NewEmstore(emstore.BoltDBType, i.primaryDBPath)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrPrimaryStoreCreate, "Infra API: Err: %v", err))
		return errors.Wrapf(types.ErrPrimaryStoreCreate, "Infra API: Err: %v", err)
	}

	i.backupStore, err = emstore.NewEmstore(emstore.BoltDBType, i.backupDBPath)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBackupStoreCreate, "Infra API: Err: %v", err))
		return errors.Wrapf(types.ErrBackupStoreCreate, "Infra API: Err: %v", err)
	}
	return nil
}

// Close releases locks on primary and back up boltDB
func (i *API) Close() error {
	if i.primaryStore != nil {
		if err := i.primaryStore.Close(); err != nil {
			log.Error(errors.Wrapf(types.ErrPrimaryStoreClose, "Infra API: %s", err))
			return errors.Wrapf(types.ErrPrimaryStoreClose, "Infra API: %s", err)
		}
	}

	if i.backupStore != nil {
		if err := i.backupStore.Close(); err != nil {
			log.Error(errors.Wrapf(types.ErrBackupStoreClose, "Infra API: %s", err))
			return errors.Wrapf(types.ErrBackupStoreClose, "Infra API: %s", err)
		}
	}

	return nil
}

// purgeDB removes the DB files.
func (i *API) purgeDB() error {
	if err := i.Close(); err != nil {
		log.Error(errors.Wrapf(types.ErrStorePurgeClose, "Infra API: %s", err))
	}

	if err := os.RemoveAll(i.primaryDBPath); err != nil {
		log.Error(errors.Wrapf(types.ErrStorePurgeNuke, "Infra API: %s | Primary DB: %v", err, i.primaryDBPath))
	}

	if err := os.RemoveAll(i.backupDBPath); err != nil {
		log.Error(errors.Wrapf(types.ErrStorePurgeNuke, "Infra API: %s | Backup DB: %v", err, i.backupDBPath))
	}
	return nil
}

// InitDB purges the current DB and opens a new DB
func (i *API) InitDB(purge bool) error {
	if purge {
		i.purgeDB()
	}
	return i.openDB()
}

// NotifyVeniceConnection marks venice connection status to true.
func (i *API) NotifyVeniceConnection(up bool) {
	i.Lock()
	defer i.Unlock()
	i.config.IsConnectedToVenice = up
}

// UpdateIfChannel updates the intf update channel
func (i *API) UpdateIfChannel(evt types.UpdateIfEvent) {
	i.Lock()
	defer i.Unlock()

	i.updMap[evt.Intf.GetKey()] = evt
	select {
	case i.ifUpdCh <- nil:
	default:
	}
	log.Infof("Interface update enqueued for intf: %s", evt.Intf.GetKey())
}

// GetIntfUpdList returns the list of dirty interfaces (intfs with a status update)
func (i *API) GetIntfUpdList() (updList []types.UpdateIfEvent) {
	i.Lock()
	defer i.Unlock()

	for key, evt := range i.updMap {
		updList = append(updList, evt)
		delete(i.updMap, key)
	}
	return
}

// IfUpdateChannel returns the interface update channel
func (i *API) IfUpdateChannel() chan error {
	return i.ifUpdCh
}
