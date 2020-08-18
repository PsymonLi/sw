package cache

import (
	"context"
	"testing"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	smmock "github.com/pensando/sw/venice/ctrler/orchhub/statemgr"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils/pcache"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/ref"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func setupLogger(logName string) log.Logger {
	config := log.GetDefaultConfig(logName)
	config.LogToStdout = true
	config.Filter = log.AllowAllFilter
	logger := log.SetConfig(config)
	return logger
}

func copyHost(host *defs.VCHubHost) *defs.VCHubHost {
	temp := ref.DeepCopy(*host).(defs.VCHubHost)
	return &temp
}

func TestValidateHost(t *testing.T) {
	sm, _, err := smmock.NewMockStateManager()
	AssertOk(t, err, "failed to create statemgr")
	logger := setupLogger("host_test")
	cache := NewCache(sm, logger)

	{
		// Bad object type
		exp1, exp2 := cache.ValidateHost(defs.VCHubHost{})
		Assert(t, !exp1 && !exp2, "expected false, false, got %v %v", exp1, exp2)
	}
	{
		// No DSCs
		exp1, exp2 := cache.ValidateHost(&defs.VCHubHost{
			Host: &cluster.Host{},
		})
		Assert(t, !exp1 && exp2, "expected false, true, got %v %v", exp1, exp2)
	}
	{
		// Valid object
		exp1, exp2 := cache.ValidateHost(&defs.VCHubHost{
			Host: &cluster.Host{
				Spec: cluster.HostSpec{
					DSCs: []cluster.DistributedServiceCardID{
						{
							MACAddress: "aaaa.bbbb.cccc",
						},
					},
				},
			},
		})
		Assert(t, exp1 && exp2, "expected true, true, got %v %v", exp1, exp2)
	}

}

func TestHost(t *testing.T) {
	sm, _, err := smmock.NewMockStateManager()
	AssertOk(t, err, "failed to create statemgr")
	logger := setupLogger("host_test")
	pCache := pcache.NewPCache(sm, logger)
	cache := &Cache{
		Log:      logger,
		stateMgr: sm,
		pCache:   pCache,
	}
	pCache.RegisterKind(hostKind, &pcache.KindOpts{
		WriteToApiserver: true,
		WriteObj:         cache.writeHost,
		DeleteObj:        cache.deleteHost,
		Validator: func(in interface{}) (bool, bool) {
			obj, ok := in.(*defs.VCHubHost)
			if !ok {
				return false, false
			}
			if len(obj.Spec.DSCs) == 0 {
				return false, true
			}
			return true, true
		},
	})
	// Set invalid object. Should be in pcache but not statemgr
	host1 := &defs.VCHubHost{
		Host: &cluster.Host{
			TypeMeta: api.TypeMeta{
				Kind:       "Host",
				APIVersion: "v1",
			},
			ObjectMeta: api.ObjectMeta{
				Name: "host1",
			},
		},
	}
	exp := copyHost(host1)
	cache.SetHost(host1, true)
	AssertEquals(t, exp, cache.GetHostByName("host1"), "Cache entry wasn't equal")
	_, err = sm.Controller().Host().Find(host1.GetObjectMeta())
	Assert(t, err != nil, "Obj shouldn't be in statemgr")

	// Set with correct info. Should be in pcache and statemgr
	host1 = copyHost(host1)
	host1.Spec.DSCs = []cluster.DistributedServiceCardID{
		{
			MACAddress: "aaaa.bbbb.cccc",
		},
	}
	exp = copyHost(host1)
	cache.SetHost(host1, true)
	AssertEquals(t, exp, cache.GetHostByName("host1"), "Cache entry wasn't equal")
	smHost, err := sm.Controller().Host().Find(host1.GetObjectMeta())
	AssertEquals(t, exp.Host, &smHost.Host, "host in statemgr did not match expected")

	// Set with invalid object, should delete as true. should be in pcache but not statemgr
	host1 = copyHost(host1)
	host1.Spec.DSCs = []cluster.DistributedServiceCardID{}
	cache.SetHost(host1, true)
	exp = copyHost(host1)
	AssertEquals(t, exp, cache.GetHostByName("host1"), "Cache entry wasn't equal")
	_, err = sm.Controller().Host().Find(host1.GetObjectMeta())
	Assert(t, err != nil, "Obj shouldn't be in statemgr")

	// Set without push update to valid object
	host1 = copyHost(host1)
	host1.Spec.DSCs = []cluster.DistributedServiceCardID{
		{
			MACAddress: "aaaa.bbbb.cccc",
		},
	}
	exp = copyHost(host1)
	cache.SetHost(host1, false)
	AssertEquals(t, exp, cache.GetHostByName("host1"), "Cache entry wasn't equal")
	_, err = sm.Controller().Host().Find(host1.GetObjectMeta())
	Assert(t, err != nil, "Obj shouldn't be in statemgr")

	// Set host info shouldn't push object
	exp = copyHost(host1)
	cache.SetHostInfo(host1)
	AssertEquals(t, exp, cache.GetHostByName("host1"), "Cache entry wasn't equal")
	_, err = sm.Controller().Host().Find(host1.GetObjectMeta())
	Assert(t, err != nil, "Obj shouldn't be in statemgr")

	// Add copy into statemgr with user labels
	smHost1 := copyHost(host1)
	smHost1.Labels = map[string]string{
		"user": "label",
	}
	smHost1.Spec.DSCs = []cluster.DistributedServiceCardID{
		{
			MACAddress: "eeee.eeee.eeee",
		},
	}

	err = sm.Controller().Host().Create(smHost1.Host)
	AssertOk(t, err, "failed to create host")

	// Revalidate host should lead to object showing up in statemgr
	cache.RevalidateHosts()
	exp.Labels = map[string]string{
		"user": "label",
	}
	AssertEquals(t, exp, cache.GetHostByName("host1"), "Cache entry wasn't equal")
	smHost, err = sm.Controller().Host().Find(host1.GetObjectMeta())
	AssertEquals(t, exp.Host, &smHost.Host, "host in statemgr did not match expected")

	// Delete call should remove object from pcache and statemgr
	cache.DeleteHost(host1)
	Assert(t, cache.GetHostByName("host1") == nil, "Cache entry wasn't equal")
	_, err = sm.Controller().Host().Find(host1.GetObjectMeta())
	Assert(t, err != nil, "Obj shouldn't be in statemgr")

	// Test host listing
	// Create host1 in pcache only
	// Create host2 in both pcache and statemgr with different spec and status
	// Create host3 in statemgr only
	cache.SetHost(host1, false)
	host2 := &defs.VCHubHost{
		Host: &cluster.Host{
			TypeMeta: api.TypeMeta{
				Kind:       "Host",
				APIVersion: "v1",
			},
			ObjectMeta: api.ObjectMeta{
				Name: "host2",
			},
		},
	}
	host2.Spec.DSCs = []cluster.DistributedServiceCardID{
		{
			MACAddress: "aaaa.bbbb.cccc",
		},
	}
	cache.SetHost(host2, false)
	smHost2 := copyHost(host2)
	smHost2.Spec.DSCs = []cluster.DistributedServiceCardID{
		{
			MACAddress: "eeee.eeee.eeee",
		},
	}
	smHost2.Status.AdmittedDSCs = []string{
		"eeee.eeee.eeee",
	}
	err = sm.Controller().Host().Create(smHost2.Host)

	host3 := &defs.VCHubHost{
		Host: &cluster.Host{
			TypeMeta: api.TypeMeta{
				Kind:       "Host",
				APIVersion: "v1",
			},
			ObjectMeta: api.ObjectMeta{
				Name: "host3",
			},
		},
	}
	host3.Spec.DSCs = []cluster.DistributedServiceCardID{
		{
			MACAddress: "aaaa.bbbb.cccc",
		},
	}
	err = sm.Controller().Host().Create(host3.Host)

	// Host should be merged copy
	h2 := cache.GetHostByName("host2")
	Assert(t, h2 != nil, "host2 returned nil")
	AssertEquals(t, "aaaa.bbbb.cccc", h2.Spec.DSCs[0].MACAddress, "spec mac did not match")
	AssertEquals(t, "eeee.eeee.eeee", h2.Status.AdmittedDSCs[0], "status mac did not match")

	hosts := cache.ListHosts(context.Background(), true)
	AssertEquals(t, 2, len(hosts), "should have only returned hosts in cache")
	h2 = hosts[h2.GetKey()]
	Assert(t, h2 != nil, "host2 returned nil")
	AssertEquals(t, "aaaa.bbbb.cccc", h2.Spec.DSCs[0].MACAddress, "spec mac did not match")
	AssertEquals(t, "eeee.eeee.eeee", h2.Status.AdmittedDSCs[0], "status mac did not match")

	hosts = cache.ListHosts(context.Background(), false)
	AssertEquals(t, 3, len(hosts), "should have only returned hosts in cache")

}
