// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"strconv"
	"time"

	"github.com/gogo/protobuf/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/api/generated/security"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/ref"
	"github.com/pensando/sw/venice/utils/runtime"
)

const (
	dNSDefaultPort = "53"
	dNSProto       = "udp"

	fTPDefaultPort = "21"
	fTPProto       = "tcp"

	tFTPDefaultPort = "69"
	tFTPProto       = "udp"

	sUNRPCDefaultPort = "111"
	sUNRPCProto       = "tcp"

	mSRPCDefaultPort = "135"
	mSRPCProto       = "tcp"
)

func getProtoPorts(app *security.App) []*netproto.ProtoPort {

	if app.Spec.ProtoPorts != nil && len(app.Spec.ProtoPorts) != 0 {
		var protoPorts []*netproto.ProtoPort
		for _, protoPort := range app.Spec.ProtoPorts {
			p := &netproto.ProtoPort{
				Protocol: protoPort.Protocol,
				Port:     protoPort.Ports,
			}
			protoPorts = append(protoPorts, p)

		}
		return protoPorts
	}
	if app.Spec.ALG != nil {
		switch app.Spec.ALG.Type {
		case security.ALG_DNS.String():
			return []*netproto.ProtoPort{
				&netproto.ProtoPort{
					Port:     dNSDefaultPort,
					Protocol: dNSProto,
				},
			}
		case security.ALG_MSRPC.String():
			return []*netproto.ProtoPort{
				&netproto.ProtoPort{
					Port:     mSRPCDefaultPort,
					Protocol: mSRPCProto,
				},
			}
		case security.ALG_SunRPC.String():
			return []*netproto.ProtoPort{
				&netproto.ProtoPort{
					Port:     sUNRPCDefaultPort,
					Protocol: sUNRPCProto,
				},
			}

		case security.ALG_FTP.String():
			return []*netproto.ProtoPort{
				&netproto.ProtoPort{
					Port:     fTPDefaultPort,
					Protocol: fTPProto,
				},
			}
		case security.ALG_TFTP.String():
			return []*netproto.ProtoPort{
				&netproto.ProtoPort{
					Port:     tFTPDefaultPort,
					Protocol: tFTPProto,
				},
			}
		}

	}

	return []*netproto.ProtoPort{}
}

// AppState is a wrapper for app object
type AppState struct {
	smObjectTracker
	App             *ctkit.App `json:"-"` // app object
	stateMgr        *Statemgr  // pointer to state manager
	markedForDelete bool
}

// AppStateFromObj converts from runtime object to app state
func AppStateFromObj(obj runtime.Object) (*AppState, error) {
	switch obj.(type) {
	case *ctkit.App:
		apobj := obj.(*ctkit.App)
		switch apobj.HandlerCtx.(type) {
		case *AppState:
			aps := apobj.HandlerCtx.(*AppState)
			return aps, nil
		default:
			return nil, ErrIncorrectObjectType
		}
	default:
		return nil, ErrIncorrectObjectType
	}
}

//GetDBObject get DB representation of the object
func (aps *AppState) GetDBObject() memdb.Object {
	return convertApp(aps)
}

// NewAppState creates new app state object
func NewAppState(app *ctkit.App, stateMgr *Statemgr) (*AppState, error) {
	aps := &AppState{
		App:      app,
		stateMgr: stateMgr,
	}
	app.HandlerCtx = aps

	aps.init(aps)
	//No need to send update notification as we are not updating status yet
	aps.smObjectTracker.skipUpdateNotification()
	err := stateMgr.AddObjectToMbus(app.MakeKey("security"), aps, references(app))
	if err != nil {
		log.Errorf("Error storing the sg policy in memdb. Err: %v", err)
		return nil, err
	}

	return aps, nil
}

// GetKey returns the key of App
func (aps *AppState) GetKey() string {
	return aps.App.GetKey()
}

// GetKind returns the kind of App
func (aps *AppState) GetKind() string {
	return aps.App.GetKind()
}

//GetGenerationID get genration ID
func (aps *AppState) GetGenerationID() string {
	return aps.App.GenerationID
}

//TrackedDSCs get DSCs that needs to be tracked
func (aps *AppState) TrackedDSCs() []string {

	dscs, _ := aps.stateMgr.ListDistributedServiceCards()

	trackedDSCs := []string{}
	for _, dsc := range dscs {
		if aps.stateMgr.isDscEnforcednMode(&dsc.DistributedServiceCard.DistributedServiceCard) {
			trackedDSCs = append(trackedDSCs, dsc.DistributedServiceCard.DistributedServiceCard.Name)
		}
	}

	return trackedDSCs
}

// Write writes the object to api server
func (aps *AppState) Write() error {
	return nil
}

// convertApp converts from npm state to netproto app
func convertApp(aps *AppState) *netproto.App {
	// build sg message
	creationTime, _ := types.TimestampProto(time.Now())
	app := netproto.App{
		TypeMeta:   aps.App.TypeMeta,
		ObjectMeta: agentObjectMeta(aps.App.ObjectMeta),
		Spec: netproto.AppSpec{
			AppIdleTimeout: aps.App.Spec.Timeout,
			ALG:            &netproto.ALG{},
		},
	}
	app.CreationTime = api.Timestamp{Timestamp: *creationTime}

	if aps.App.Spec.ALG != nil {
		switch aps.App.Spec.ALG.Type {
		case security.ALG_ICMP.String():
			if aps.App.Spec.ALG.Icmp != nil {
				app.Spec.ProtoPorts = []*netproto.ProtoPort{
					{
						Protocol: "icmp",
					},
				}
				ictype, _ := strconv.Atoi(aps.App.Spec.ALG.Icmp.Type)
				icode, _ := strconv.Atoi(aps.App.Spec.ALG.Icmp.Code)

				app.Spec.ALG.ICMP = &netproto.ICMP{
					Type: uint32(ictype),
					Code: uint32(icode),
				}
			}
		case security.ALG_DNS.String():
			if aps.App.Spec.ALG.Dns != nil {
				app.Spec.ALG.DNS = &netproto.DNS{
					DropMultiQuestionPackets: aps.App.Spec.ALG.Dns.DropMultiQuestionPackets,
					DropLargeDomainPackets:   aps.App.Spec.ALG.Dns.DropLargeDomainNamePackets,
					DropLongLabelPackets:     aps.App.Spec.ALG.Dns.DropLongLabelPackets,
					MaxMessageLength:         aps.App.Spec.ALG.Dns.MaxMessageLength,
					QueryResponseTimeout:     aps.App.Spec.ALG.Dns.QueryResponseTimeout,
				}
			} else {
				app.Spec.ALG.DNS = &netproto.DNS{}
			}
		case security.ALG_FTP.String():
			if aps.App.Spec.ALG.Ftp != nil {
				app.Spec.ALG.FTP = &netproto.FTP{
					AllowMismatchIPAddresses: aps.App.Spec.ALG.Ftp.AllowMismatchIPAddress,
				}
			} else {
				app.Spec.ALG.FTP = &netproto.FTP{}
			}
		case security.ALG_SunRPC.String():
			for _, sunrpc := range aps.App.Spec.ALG.Sunrpc {
				app.Spec.ALG.SUNRPC = append(app.Spec.ALG.SUNRPC,
					&netproto.RPC{
						ProgramID:        sunrpc.ProgramID,
						ProgramIDTimeout: sunrpc.Timeout,
					})
			}

			if len(app.Spec.ALG.SUNRPC) == 0 {
				app.Spec.ALG.SUNRPC = []*netproto.RPC{}
			}
		case security.ALG_MSRPC.String():
			for _, msrpc := range aps.App.Spec.ALG.Msrpc {
				app.Spec.ALG.MSRPC = append(app.Spec.ALG.MSRPC,
					&netproto.RPC{
						ProgramID:        msrpc.ProgramUUID,
						ProgramIDTimeout: msrpc.Timeout,
					})
			}
			if len(app.Spec.ALG.MSRPC) == 0 {
				app.Spec.ALG.MSRPC = []*netproto.RPC{}
			}
		case security.ALG_TFTP.String():
			app.Spec.ALG.TFTP = &netproto.TFTP{}
		case security.ALG_RTSP.String():
			app.Spec.ALG.RTSP = &netproto.RTSP{}
		}
	}

	app.Spec.ProtoPorts = getProtoPorts(&aps.App.App)
	return &app
}

// attachPolicy adds a policy to attached policy list
func (aps *AppState) attachPolicy(sgpName string) error {
	// see if the policy is already part of the list
	for _, pn := range aps.App.Status.AttachedPolicies {
		if pn == sgpName {
			return nil
		}
	}

	// add the policy to the list
	aps.App.Status.AttachedPolicies = append(aps.App.Status.AttachedPolicies, sgpName)

	// save the updated app
	aps.App.Write()

	return nil
}

// detachPolicy removes a policy from
func (aps *AppState) detachPolicy(sgpName string) error {
	// see if the policy is already part of the list
	for idx, pn := range aps.App.Status.AttachedPolicies {
		if pn == sgpName {
			aps.App.Status.AttachedPolicies = append(aps.App.Status.AttachedPolicies[:idx], aps.App.Status.AttachedPolicies[idx+1:]...)
		}
	}
	// save the updated app
	//Write is expensive, don't hold the policy delete
	go aps.App.Write()

	return nil
}

//GetAppWatchOptions gets options
func (sma *SmApp) GetAppWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	opts.FieldChangeSelector = []string{"Spec"}
	return &opts
}

//OnAppCreateReq create req from agent
func (sm *Statemgr) OnAppCreateReq(nodeID string, objinfo *netproto.App) error {
	return nil
}

//OnAppUpdateReq  update req from agent
func (sm *Statemgr) OnAppUpdateReq(nodeID string, objinfo *netproto.App) error {
	return nil

}

//OnAppDeleteReq  delete req from agent
func (sm *Statemgr) OnAppDeleteReq(nodeID string, objinfo *netproto.App) error {
	return nil

}

// OnAppOperUpdate gets called when agent sends oper update
func (sm *Statemgr) OnAppOperUpdate(nodeID string, objinfo *netproto.App) error {

	eps, err := sm.FindApp(objinfo.Tenant, objinfo.Name)
	if err != nil {
		return err
	}
	eps.updateNodeVersion(nodeID, objinfo.ObjectMeta.GenerationID)
	return nil
}

// OnAppOperDelete oper delete
func (sm *Statemgr) OnAppOperDelete(nodeID string, objinfo *netproto.App) error {
	return nil

}

// OnAppCreate handles app creation
func (sma *SmApp) OnAppCreate(app *ctkit.App) error {
	sm := sma.sm
	// see if we already have the app
	hs, err := sm.FindApp(app.Tenant, app.Name)
	if err == nil {
		hs.App = app
		return nil
	}

	log.Infof("Creating app: %+v", app)

	// create new app object
	hs, err = NewAppState(app, sm)
	if err != nil {
		log.Errorf("Error creating app %+v. Err: %v", app, err)
		return err
	}

	return nil
}

// OnAppUpdate handles update app event
func (sma *SmApp) OnAppUpdate(app *ctkit.App, napp *security.App) error {
	// see if anything changed
	sm := sma.sm
	_, ok := ref.ObjDiff(app.Spec, napp.Spec)
	if (napp.GenerationID == app.GenerationID) && !ok {
		app.ObjectMeta = napp.ObjectMeta
		return nil
	}
	app.ObjectMeta = napp.ObjectMeta
	app.Spec = napp.Spec

	aps, err := sm.FindApp(app.Tenant, app.Name)
	if err != nil {
		return err
	}

	// save the updated app
	sm.UpdateObjectToMbus(app.MakeKey("security"), aps, references(napp))

	return nil
}

// OnAppDelete handles app deletion
func (sma *SmApp) OnAppDelete(app *ctkit.App) error {
	// see if we have the app
	sm := sma.sm
	fapp, err := AppStateFromObj(app)
	if err != nil {
		log.Errorf("Could not find the app %v. Err: %v", app, err)
		return err
	}
	log.Infof("Deleting app: %+v", fapp)

	fapp.markedForDelete = true
	// delete the object
	return sm.DeleteObjectToMbus(app.MakeKey("security"), fapp,
		references(app))
}

// FindApp finds a app
func (sm *Statemgr) FindApp(tenant, name string) (*AppState, error) {
	// find the object
	obj, err := sm.FindObject("App", tenant, "default", name)
	if err != nil {
		return nil, err
	}

	return AppStateFromObj(obj)
}

// OnAppReconnect is called when ctkit reconnects to apiserver
func (sma *SmApp) OnAppReconnect() {
	return
}

// ListApps lists all apps
func (sm *Statemgr) ListApps() ([]*AppState, error) {
	objs := sm.ListObjects("App")

	var apps []*AppState
	for _, obj := range objs {
		app, err := AppStateFromObj(obj)
		if err != nil {
			log.Errorf("Error getting App %v", err)
			continue
		}

		apps = append(apps, app)
	}

	return apps, nil
}

var smgrApp *SmApp

// SmApp is statemanager struct for App
type SmApp struct {
	featureMgrBase
	sm *Statemgr
}

func initSmApp() {
	mgr := MustGetStatemgr()
	smgrApp = &SmApp{
		sm: mgr,
	}
	mgr.Register("statemgrapp", smgrApp)
}

// CompleteRegistration is the callback function statemgr calls after init is done
func (sma *SmApp) CompleteRegistration() {
	// if featureflags.IsOVerlayRoutingEnabled() == false {
	// 	return
	// }

	//initSmApp()
	log.Infof("Got CompleteRegistration for SmApp")
	sma.sm.SetAppReactor(smgrApp)
}

func init() {
	initSmApp()
}

//ProcessDSCCreate create
func (sma *SmApp) ProcessDSCCreate(dsc *cluster.DistributedServiceCard) {

	if sma.sm.isDscEnforcednMode(dsc) {
		sma.dscTracking(dsc, true)
	}
}

func (sma *SmApp) dscTracking(dsc *cluster.DistributedServiceCard, start bool) {

	apps, err := sma.sm.ListApps()
	if err != nil {
		log.Errorf("Error listing apps %v", err)
		return
	}

	for _, aps := range apps {
		if start {
			aps.smObjectTracker.startDSCTracking(dsc.Name)

		} else {
			aps.smObjectTracker.stopDSCTracking(dsc.Name)
		}
	}
}

//ProcessDSCUpdate update
func (sma *SmApp) ProcessDSCUpdate(dsc *cluster.DistributedServiceCard, ndsc *cluster.DistributedServiceCard) {

	//Process only if it is deleted or decomissioned
	if sma.sm.dscDecommissioned(ndsc) {
		sma.dscTracking(ndsc, false)
		return
	}

	//Run only if profile changes.
	if dsc.Spec.DSCProfile != ndsc.Spec.DSCProfile || sma.sm.dscRecommissioned(dsc, ndsc) {
		if sma.sm.isDscEnforcednMode(ndsc) {
			sma.dscTracking(ndsc, true)
		} else {
			sma.dscTracking(ndsc, false)
		}
	}
}

//ProcessDSCDelete delete
func (sma *SmApp) ProcessDSCDelete(dsc *cluster.DistributedServiceCard) {
	sma.dscTracking(dsc, false)
}
