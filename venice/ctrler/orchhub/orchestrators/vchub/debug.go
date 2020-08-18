package vchub

import (
	"fmt"
	"strconv"

	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/useg"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils/usegvlanmgr"
)

/**
 * Debug commands
 * params:
 *    key: Name of the orch config
 *    action: Type of debug request
 *
 * action: useg - Returns the useg assignments
 * extra params:
 * 		dc: DC name for which to get the assignments from
 *
 * action: pcache - returns the entries in the pcache
 * extra params:
 *    kind: Optional, will only return entries of the given kind if supplied
 *
 * action: state - Returns vchub DC -> DVS -> PG map
 *
 * action: sync - Triggers a sync
 *
 * action: vlanSpace - Sets the size of the vlan space
 * params:
 *       count: size of vlan space per host
 *
 */

// DebugUseg is the action string for triggering useg debugging
const DebugUseg = "useg"

// DebugCache is the action string for triggering pcache debugging
const DebugCache = "pcache"

// DebugState is the action string for triggering state debugging
const DebugState = "state"

// DebugSync is the action string for triggering sync
const DebugSync = "sync"

// DebugVlanSpace is the action string for triggering sync
const DebugVlanSpace = "vlanSpace"

func (v *VCHub) debugHandler(action string, params map[string]string) (interface{}, error) {

	switch action {
	case DebugUseg:
		return v.debugUseg(params)
	case DebugCache:
		return v.debugCache(params)
	case DebugState:
		return v.debugState(params)
	case DebugSync:
		return v.debugSync(params)
	case DebugVlanSpace:
		return v.debugVlanSpace(params)
	default:
		return nil, fmt.Errorf("VCHub debug does not support action %s", action)
	}
}

func (v *VCHub) debugVlanSpace(params map[string]string) (interface{}, error) {
	countStr, ok := params["count"]
	if !ok {
		return nil, fmt.Errorf("count is a required param")
	}
	count, err := strconv.Atoi(countStr)
	if err != nil {
		return nil, err
	}
	newMax := useg.FirstUsegVlan + count
	if newMax > 4095 {
		return nil, fmt.Errorf("Vlan space is not allowed to be more than 4095")
	}
	usegvlanmgr.VlanMax = newMax
	return nil, nil
}

func (v *VCHub) debugUseg(params map[string]string) (interface{}, error) {
	dcName, ok := params["dc"]
	if !ok {
		return nil, fmt.Errorf("dc is a required param")
	}
	penDC := v.GetDC(dcName)
	penDVS := penDC.GetPenDVS(CreateDVSName(dcName))
	return penDVS.UsegMgr.Debug(params)
}

func (v *VCHub) debugCache(params map[string]string) (interface{}, error) {
	return v.cache.Debug(params)
}

type debugDC struct {
	ID  string
	DVS map[string]debugDVS
}

type debugDVS struct {
	ID  string
	PGs map[string]debugPG
}

type debugPG struct {
	ID      string
	Network string
}

func (v *VCHub) debugState(params map[string]string) (interface{}, error) {
	// DC -> DVS -> PG
	// Also contains launch time of vchub
	dcRet := map[string]debugDC{}
	v.DcMapLock.Lock()
	defer v.DcMapLock.Unlock()
	for dcName, dc := range v.DcMap {
		dcObj := debugDC{
			ID:  dc.DcRef.Value,
			DVS: map[string]debugDVS{},
		}
		dc.Lock()
		for dvsName, dvs := range dc.PenDvsMap {
			dvs.Lock()
			dvsObj := debugDVS{
				ID:  dvs.DvsRef.Value,
				PGs: map[string]debugPG{},
			}
			for pgName, pg := range dvs.Pgs {
				pgObj := debugPG{
					ID:      pg.PgRef.Value,
					Network: pg.NetworkMeta.GetName(),
				}
				dvsObj.PGs[pgName] = pgObj
			}
			dvs.Unlock()
			dcObj.DVS[dvsName] = dvsObj
		}
		dc.Unlock()
		dcRet[dcName] = dcObj
	}
	ret := struct {
		DcInfo     map[string]debugDC
		LaunchTime string
	}{
		DcInfo:     dcRet,
		LaunchTime: v.launchTime,
	}

	return ret, nil
}

func (v *VCHub) debugSync(params map[string]string) (interface{}, error) {
	v.Sync()
	return nil, nil
}
