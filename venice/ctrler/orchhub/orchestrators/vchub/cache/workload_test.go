package cache

import (
	"context"
	"testing"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	smmock "github.com/pensando/sw/venice/ctrler/orchhub/statemgr"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils/pcache"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/ref"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func copyWorkload(workload *defs.VCHubWorkload) *defs.VCHubWorkload {
	temp := ref.DeepCopy(*workload).(defs.VCHubWorkload)
	return &temp
}

func TestValidateWorkload(t *testing.T) {
	sm, _, err := smmock.NewMockStateManager()
	AssertOk(t, err, "failed to create statemgr")
	logger := setupLogger("workload_test")
	cache := NewCache(sm, logger)

	{
		// Bad object type
		exp1, exp2 := cache.ValidateWorkload(defs.VCHubWorkload{})
		Assert(t, !exp1 && !exp2, "expected false, false, got %v %v", exp1, exp2)
	}
	{
		// No host name
		exp1, exp2 := cache.ValidateWorkload(&defs.VCHubWorkload{
			Workload: &workload.Workload{},
		})
		Assert(t, !exp1 && exp2, "expected false, true, got %v %v", exp1, exp2)
	}
	{
		// host does not exist yet
		exp1, exp2 := cache.ValidateWorkload(&defs.VCHubWorkload{
			Workload: &workload.Workload{
				Spec: workload.WorkloadSpec{
					HostName: "host1",
				},
			},
		})
		Assert(t, !exp1 && exp2, "expected false, true, got %v %v", exp1, exp2)
	}
	// Create host
	host1 := &cluster.Host{
		TypeMeta: api.TypeMeta{
			Kind:       "Host",
			APIVersion: "v1",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "host1",
		},
	}
	err = sm.Controller().Host().Create(host1)
	AssertOk(t, err, "failed to create host")

	{
		// No interfaces
		exp1, exp2 := cache.ValidateWorkload(&defs.VCHubWorkload{
			Workload: &workload.Workload{
				Spec: workload.WorkloadSpec{
					HostName: "host1",
				},
			},
		})
		Assert(t, !exp1 && exp2, "expected false, true, got %v %v", exp1, exp2)
	}
	{
		// Interface doesn't have a useg
		exp1, exp2 := cache.ValidateWorkload(&defs.VCHubWorkload{
			Workload: &workload.Workload{
				Spec: workload.WorkloadSpec{
					HostName: "host1",
					Interfaces: []workload.WorkloadIntfSpec{
						{
							MACAddress:  "aaaa.bbbb.cccc",
							IpAddresses: []string{},
						},
					},
				},
			},
		})
		Assert(t, !exp1 && !exp2, "expected false, false, got %v %v", exp1, exp2)
	}
	{
		// Valid workload
		exp1, exp2 := cache.ValidateWorkload(&defs.VCHubWorkload{
			Workload: &workload.Workload{
				Spec: workload.WorkloadSpec{
					HostName: "host1",
					Interfaces: []workload.WorkloadIntfSpec{
						{
							MACAddress:   "aaaa.bbbb.cccc",
							MicroSegVlan: 10,
							IpAddresses:  []string{},
						},
					},
				},
			},
		})
		Assert(t, exp1 && exp2, "expected true, true, got %v %v", exp1, exp2)
	}
}

func TestWorkload(t *testing.T) {
	sm, _, err := smmock.NewMockStateManager()
	AssertOk(t, err, "failed to create statemgr")
	logger := setupLogger("workload_test")
	pCache := pcache.NewPCache(sm, logger)
	cache := &Cache{
		Log:      logger,
		stateMgr: sm,
		pCache:   pCache,
	}
	pCache.RegisterKind(workloadKind, &pcache.KindOpts{
		WriteToApiserver: true,
		WriteObj:         cache.writeWorkload,
		DeleteObj:        cache.deleteWorkload,
		Validator: func(in interface{}) (bool, bool) {
			obj, ok := in.(*defs.VCHubWorkload)
			if !ok {
				return false, false
			}
			if len(obj.Spec.Interfaces) == 0 {
				return false, true
			}
			return true, true
		},
	})

	// Set invalid object. Should be in pcache but not statemgr
	w1 := &defs.VCHubWorkload{
		Workload: &workload.Workload{
			TypeMeta: api.TypeMeta{
				Kind:       "Workload",
				APIVersion: "v1",
			},
			ObjectMeta: api.ObjectMeta{
				Name:      "w1",
				Tenant:    globals.DefaultTenant,
				Namespace: globals.DefaultNamespace,
			},
			Spec: workload.WorkloadSpec{
				HostName: "host1",
			},
		},
	}
	exp := copyWorkload(w1)
	cache.SetWorkload(w1, true)
	AssertEquals(t, exp, cache.GetWorkloadByName("w1"), "Cache entry wasn't equal")
	_, err = sm.Controller().Workload().Find(w1.GetObjectMeta())
	Assert(t, err != nil, "Obj shouldn't be in statemgr")

	// Set with correct info. Should be in pcache and statemgr
	w1 = copyWorkload(w1)
	w1.Spec.Interfaces = []workload.WorkloadIntfSpec{
		{
			MACAddress:   "aaaa.bbbb.cccc",
			MicroSegVlan: 10,
			IpAddresses:  []string{},
		},
	}
	exp = copyWorkload(w1)
	cache.SetWorkload(w1, true)
	AssertEquals(t, exp, cache.GetWorkloadByName("w1"), "Cache entry wasn't equal")
	smWorkload, err := sm.Controller().Workload().Find(w1.GetObjectMeta())
	AssertEquals(t, exp.Workload, &smWorkload.Workload, "host in statemgr did not match expected")

	// Set with invalid object, should delete as true. should be in pcache but not statemgr
	w1 = copyWorkload(w1)
	w1.Spec.Interfaces = []workload.WorkloadIntfSpec{}
	cache.SetWorkload(w1, true)
	exp = copyWorkload(w1)
	AssertEquals(t, exp, cache.GetWorkloadByName("w1"), "Cache entry wasn't equal")
	_, err = sm.Controller().Workload().Find(w1.GetObjectMeta())
	Assert(t, err != nil, "Obj shouldn't be in statemgr")

	// Set without push update to valid object
	w1 = copyWorkload(w1)
	w1.Spec.Interfaces = []workload.WorkloadIntfSpec{
		{
			MACAddress:   "aaaa.bbbb.cccc",
			MicroSegVlan: 10,
			IpAddresses:  []string{},
		},
	}
	exp = copyWorkload(w1)
	cache.SetWorkload(w1, false)
	AssertEquals(t, exp, cache.GetWorkloadByName("w1"), "Cache entry wasn't equal")
	_, err = sm.Controller().Workload().Find(w1.GetObjectMeta())
	Assert(t, err != nil, "Obj shouldn't be in statemgr")

	// Set host info shouldn't push object
	exp = copyWorkload(w1)
	cache.SetVMInfo(w1)
	AssertEquals(t, exp, cache.GetWorkloadByName("w1"), "Cache entry wasn't equal")
	_, err = sm.Controller().Workload().Find(w1.GetObjectMeta())
	Assert(t, err != nil, "Obj shouldn't be in statemgr")

	// Add copy into statemgr with user labels
	smWorkload1 := copyWorkload(w1)
	smWorkload1.Labels = map[string]string{
		"user": "label",
	}
	smWorkload1.Spec.Interfaces = []workload.WorkloadIntfSpec{
		{
			MACAddress:   "eeee.eeee.eeee",
			MicroSegVlan: 400,
		},
	}

	err = sm.Controller().Workload().Create(smWorkload1.Workload)
	AssertOk(t, err, "failed to create workload")

	// Revalidate workload should lead to object showing up in statemgr
	cache.RevalidateWorkloads()
	exp.Labels = map[string]string{
		"user": "label",
	}
	AssertEquals(t, exp.Workload, cache.GetWorkloadByName("w1").Workload, "Cache entry wasn't equal")
	smWorkload, err = sm.Controller().Workload().Find(w1.GetObjectMeta())
	AssertEquals(t, exp.Workload, &smWorkload.Workload, "host in statemgr did not match expected")

	// Delete call should remove object from pcache and statemgr
	cache.DeleteWorkload(w1)
	Assert(t, cache.GetWorkloadByName("w1") == nil, "Cache entry wasn't equal")
	_, err = sm.Controller().Workload().Find(w1.GetObjectMeta())
	Assert(t, err != nil, "Obj shouldn't be in statemgr")

	// Test host listing
	// Create w1 in pcache only
	// Create w2 in both pcache and statemgr with different spec and status
	// Create w3 in statemgr only
	cache.SetWorkload(w1, false)
	w2 := &defs.VCHubWorkload{
		Workload: &workload.Workload{
			TypeMeta: api.TypeMeta{
				Kind:       "Workload",
				APIVersion: "v1",
			},
			ObjectMeta: api.ObjectMeta{
				Name:      "w2",
				Tenant:    globals.DefaultTenant,
				Namespace: globals.DefaultNamespace,
			},
			Spec: workload.WorkloadSpec{
				HostName: "host1",
			},
		},
	}
	w2.Spec.Interfaces = []workload.WorkloadIntfSpec{
		{
			MACAddress:   "aaaa.bbbb.cccc",
			MicroSegVlan: 10,
			IpAddresses:  []string{},
		},
	}
	cache.SetWorkload(w2, false)
	smWorkload2 := copyWorkload(w2)
	smWorkload2.Spec.Interfaces = []workload.WorkloadIntfSpec{
		{
			MACAddress: "eeee.eeee.eeee",
		},
	}
	smWorkload2.Status.Interfaces = []workload.WorkloadIntfStatus{
		{
			MACAddress: "eeee.eeee.eeee",
		},
	}
	err = sm.Controller().Workload().Create(smWorkload2.Workload)

	w3 := &defs.VCHubWorkload{
		Workload: &workload.Workload{
			TypeMeta: api.TypeMeta{
				Kind:       "Workload",
				APIVersion: "v1",
			},
			ObjectMeta: api.ObjectMeta{
				Name:      "w3",
				Tenant:    globals.DefaultTenant,
				Namespace: globals.DefaultNamespace,
			},
			Spec: workload.WorkloadSpec{
				HostName: "host1",
			},
		},
	}
	w3.Spec.Interfaces = []workload.WorkloadIntfSpec{
		{
			MACAddress: "aaaa.bbbb.cccc",
		},
	}
	err = sm.Controller().Workload().Create(w3.Workload)

	// workload should be merged copy
	h2 := cache.GetWorkloadByName("w2")
	Assert(t, h2 != nil, "w2 returned nil")
	AssertEquals(t, "aaaa.bbbb.cccc", h2.Spec.Interfaces[0].MACAddress, "spec mac did not match")
	AssertEquals(t, "eeee.eeee.eeee", h2.Status.Interfaces[0].MACAddress, "status mac did not match")

	workloads := cache.ListWorkloads(context.Background(), true)
	AssertEquals(t, 2, len(workloads), "should have only returned workloads in cache")
	h2 = workloads[h2.GetKey()]
	Assert(t, h2 != nil, "w2 returned nil")
	AssertEquals(t, "aaaa.bbbb.cccc", h2.Spec.Interfaces[0].MACAddress, "spec mac did not match")
	AssertEquals(t, "eeee.eeee.eeee", h2.Status.Interfaces[0].MACAddress, "status mac did not match")

	workloads = cache.ListWorkloads(context.Background(), false)
	AssertEquals(t, 3, len(workloads), "should have only returned workloads in cache")

}
