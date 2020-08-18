package cache

import (
	"github.com/pensando/sw/venice/ctrler/orchhub/statemgr"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils/pcache"
	"github.com/pensando/sw/venice/utils/log"
)

// Cache adds methods to pcache for VCHub
type Cache struct {
	Log      log.Logger
	stateMgr *statemgr.Statemgr
	pCache   *pcache.PCache
}

// NewCache creates a new cache
func NewCache(stateMgr *statemgr.Statemgr, logger log.Logger) *Cache {
	pCache := pcache.NewPCache(stateMgr, logger)
	cache := &Cache{
		Log:      logger,
		stateMgr: stateMgr,
		pCache:   pCache,
	}
	pCache.RegisterKind(workloadKind, &pcache.KindOpts{
		WriteToApiserver: true,
		WriteObj:         cache.writeWorkload,
		DeleteObj:        cache.deleteWorkload,
		Validator:        cache.ValidateWorkload,
	})
	pCache.RegisterKind(hostKind, &pcache.KindOpts{
		WriteToApiserver: true,
		WriteObj:         cache.writeHost,
		DeleteObj:        cache.deleteHost,
		Validator:        cache.ValidateHost,
	})
	return cache
}

// Debug returns the pcache entries
func (c *Cache) Debug(params map[string]string) (interface{}, error) {
	return c.pCache.Debug(params)
}

// SetValidator is used to set a special validator (used in testing)
func (c *Cache) SetValidator(kind string, validator pcache.ValidatorFn) {
	c.pCache.SetValidator(kind, validator)
}
