// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/featureflags"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/memdb/objReceiver"
	"github.com/pensando/sw/venice/utils/ref"
)

var dscMgrRc *DSCMgrRc

// DSCMgrRc is object manager for distriibuted service card with routing config
type DSCMgrRc struct {
	featureMgrBase
	sm *Statemgr
}

// CompleteRegistration is the callback function statemgr calls after init is done
func (dscMgr *DSCMgrRc) CompleteRegistration() {
	if featureflags.IsOVerlayRoutingEnabled() == false {
		return
	}

	dscMgr.sm.SetDistributedServiceCardReactor(dscMgr)
	dscMgr.sm.EnableSelectivePushForKind("DSCConfig")
}

func init() {
	mgr := MustGetStatemgr()
	dscMgrRc = &DSCMgrRc{
		sm: mgr,
	}

	mgr.Register("statemgrsmartnicrc", dscMgrRc)
}

//GetDistributedServiceCardWatchOptions gets options
func (dscMgr *DSCMgrRc) GetDistributedServiceCardWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	opts.FieldChangeSelector = []string{}
	return &opts
}

func convertDSCConfig(smartNic *ctkit.DistributedServiceCard) *netproto.DSCConfig {
	DSCConfig := &netproto.DSCConfig{
		TypeMeta: api.TypeMeta{
			Kind: "DSCConfig",
		},
		ObjectMeta: smartNic.DistributedServiceCard.ObjectMeta,
	}
	DSCConfig.Spec = netproto.DSCConfigSpec{}
	DSCConfig.Spec.TxPolicer = smartNic.Spec.TxPolicer
	DSCConfig.Spec.Tenant = smartNic.Spec.PolicerAttachTenant
	DSCConfig.Status.DSC = smartNic.Name

	return DSCConfig
}

//GetDBObject get db object
func (smartNic *DistributedServiceCardState) GetDBObject() memdb.Object {
	return convertDSCConfig(smartNic.DistributedServiceCard)
}

//PushObject get push object
func (smartNic *DistributedServiceCardState) PushObject() memdb.PushObjectHandle {
	return smartNic.pushObject
}

//TrackedDSCs tracked dscs
func (smartNic *DistributedServiceCardState) TrackedDSCs() []string {
	mgr := MustGetStatemgr()
	trackedDSCs := []string{}

	dsc, err := mgr.FindDistributedServiceCardByMacAddr(smartNic.DistributedServiceCard.Status.PrimaryMAC)
	if err != nil {
		log.Errorf("Error looking up DSC %v", smartNic.DistributedServiceCard.Status.PrimaryMAC)
	} else {
		trackedDSCs = append(trackedDSCs, dsc.DistributedServiceCard.DistributedServiceCard.Name)
		log.Infof("Adding %v as tracked DSC for %v", trackedDSCs, smartNic.DistributedServiceCard.Status.PrimaryMAC)
	}
	return trackedDSCs
}

// OnDistributedServiceCardCreate handles smartNic creation
func (dscMgr *DSCMgrRc) OnDistributedServiceCardCreate(smartNic *ctkit.DistributedServiceCard) error {
	defer dscMgr.sm.sendDscUpdateNotification(CreateEvent, &smartNic.DistributedServiceCard, nil)
	sns, err := dscMgr.sm.dscCreate(smartNic)
	if err != nil {
		return err
	}
	dscMgr.sm.addDSCRelatedobjects(smartNic, sns, false)
	dscMgr.sm.PeriodicUpdaterPush(sns)
	dscState, err := DistributedServiceCardStateFromObj(smartNic)
	if err != nil {
		log.Errorf("error creating network interface state")
	}

	if smartNic.Spec.RoutingConfig != "" {
		log.Infof("Sending routing config dsc: %s | rtcfg: %s", smartNic.Name, smartNic.Spec.RoutingConfig)
		dscMgr.sm.mbus.SendRoutingConfig(smartNic.Name, "", smartNic.Spec.RoutingConfig)
	}
	receiver, err := dscMgr.sm.mbus.FindReceiver(dscState.DistributedServiceCard.Status.PrimaryMAC)
	if err != nil {
		log.Errorf("error finding receiver for %v %v", dscState.DistributedServiceCard.Status.PrimaryMAC, err)
		return kvstore.NewTxnFailedError()
	}
	pushObj, err := dscMgr.sm.AddPushObjectToMbus(smartNic.MakeKey(string(apiclient.GroupCluster)), dscState, references(smartNic), []objReceiver.Receiver{receiver})
	if err != nil {
		log.Errorf("error updating  push object %v", err)
		return err
	}
	dscState.pushObject = pushObj
	return nil
}

// OnDistributedServiceCardUpdate handles update event on smartnic
func (dscMgr *DSCMgrRc) OnDistributedServiceCardUpdate(smartNic *ctkit.DistributedServiceCard, nsnic *cluster.DistributedServiceCard) error {
	copyDSC := ref.DeepCopy(&smartNic.DistributedServiceCard).(*cluster.DistributedServiceCard)
	defer dscMgr.sm.sendDscUpdateNotification(UpdateEvent, copyDSC, nsnic)
	log.Infof("DSC spec old: %+v | new: %+v", smartNic.Spec, nsnic.Spec)
	oldRt := smartNic.Spec.RoutingConfig
	newRt := nsnic.Spec.RoutingConfig
	oldTp := smartNic.Spec.TxPolicer
	newTp := nsnic.Spec.TxPolicer
	oldTen := smartNic.Spec.PolicerAttachTenant
	newTen := nsnic.Spec.PolicerAttachTenant

	sns, err := dscMgr.sm.updateDSC(smartNic, nsnic)
	if err != nil {
		log.Errorf("updateDsc returned error: %s", err)
		return nil
	}

	dscMgr.sm.PeriodicUpdaterPush(sns)
	dscState, err := DistributedServiceCardStateFromObj(smartNic)
	if err != nil {
		log.Errorf("error creating network interface state")
		return nil
	}
	if oldRt != newRt {
		log.Infof("Sending routing config dsc: %s | rtcfg old: %s, new: %s", smartNic.Name, oldRt, newRt)
		dscMgr.sm.mbus.SendRoutingConfig(smartNic.Name, oldRt, newRt)
	}
	if oldTp != newTp || oldTen != newTen {
		log.Infof("Sending DSC config dsc: %s | tpcfg old: %s, new: %s tenant old:%s new:%s", smartNic.Name, oldTp, newTp, oldTen, newTen)
		err = dscMgr.sm.UpdatePushObjectToMbus(smartNic.MakeKey(string(apiclient.GroupCluster)), dscState, references(smartNic))
		if err != nil {
			log.Errorf("error updating DSC config to DSC err: %v", err)
			return err
		}
	}
	return nil
}

// OnDistributedServiceCardDelete handles smartNic deletion
func (dscMgr *DSCMgrRc) OnDistributedServiceCardDelete(smartNic *ctkit.DistributedServiceCard) error {
	defer dscMgr.sm.sendDscUpdateNotification(DeleteEvent, &smartNic.DistributedServiceCard, nil)
	hs, err := dscMgr.sm.deleteDsc(smartNic)
	if err != nil {
		return err
	}
	dscState, err := DistributedServiceCardStateFromObj(smartNic)
	if err != nil {
		log.Errorf("error creating network interface state")
		return nil
	}
	log.Infof("Sending routing config dsc: %s | rtcfg: %s", smartNic.Name, smartNic.Spec.RoutingConfig)
	dscMgr.sm.mbus.SendRoutingConfig(smartNic.Name, smartNic.Spec.RoutingConfig, "")
	err = dscMgr.sm.DeletePushObjectToMbus(smartNic.MakeKey(string(apiclient.GroupCluster)), dscState, references(smartNic))
	if err != nil {
		log.Errorf("error Deleting DSC Configuration from DSC err: %v", err)
	}
	return dscMgr.sm.deleteDscRelatedObjects(smartNic, hs, false)

}

// OnDistributedServiceCardReconnect is called when ctkit reconnects to apiserver
func (dscMgr *DSCMgrRc) OnDistributedServiceCardReconnect() {
	return
}
