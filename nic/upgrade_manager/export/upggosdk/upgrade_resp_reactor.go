package upggosdk

import (
	"github.com/pensando/sw/nic/delphi/gosdk/client_api"
	"github.com/pensando/sw/nic/delphi/proto/delphi"
	"github.com/pensando/sw/nic/upgrade_manager/export/upggosdk/nic/upgrade_manager/upgrade"
	"github.com/pensando/sw/venice/utils/log"
)

type upgrespctx struct {
	agentHdlrs AgentHandlers
	sdkClient  clientApi.Client
}

var upgRespCtx upgrespctx

func (ctx *upgrespctx) invokeAgentHandler(obj *upgrade.UpgResp) {
	var errStrList []string
	switch obj.GetUpgRespVal() {
	case upgrade.UpgRespType_UpgRespPass:
		log.Infof("upgrade success!!")
		ctx.agentHdlrs.UpgSuccessful()
	case upgrade.UpgRespType_UpgRespFail:
		log.Infof("upgrade failed!!")
		for _, str := range obj.GetUpgRespFailStr() {
			errStrList = append(errStrList, str)
		}
		ctx.agentHdlrs.UpgFailed(&errStrList)
	case upgrade.UpgRespType_UpgRespAbort:
		log.Infof("upgrade aborted!!")
		for _, str := range obj.GetUpgRespFailStr() {
			errStrList = append(errStrList, str)
		}
		ctx.agentHdlrs.UpgAborted(&errStrList)
	case upgrade.UpgRespType_UpgRespUpgPossible:
		log.Infof("%d length", len(obj.GetUpgRespFailStr()))
		if len(obj.GetUpgRespFailStr()) == 0 {
			log.Infof("upgrade possible")
			ctx.agentHdlrs.UpgPossible(&upgCtx)
		} else {
			for _, str := range obj.GetUpgRespFailStr() {
				errStrList = append(errStrList, str)
			}
			log.Infof("upgrade not possible")
			ctx.agentHdlrs.UpgNotPossible(&upgCtx, &errStrList)
		}
	}
}

func (ctx *upgrespctx) deleteUpgReqSpec() {
	upgReq := upgrade.GetUpgReq(ctx.sdkClient)
	if upgReq == nil {
		log.Infof("upgReq not found")
		return
	}
	ctx.sdkClient.DeleteObject(upgReq)
	log.Infof("Upgrade Req Object deleted for next request")
}

func (ctx *upgrespctx) OnUpgRespCreate(obj *upgrade.UpgResp) {
	log.Infof("OnUpgRespCreate called %d", obj.GetUpgRespVal())
	ctx.deleteUpgReqSpec()
	ctx.invokeAgentHandler(obj)
}

func (ctx *upgrespctx) OnUpgRespUpdate(old, obj *upgrade.UpgResp) {
	log.Infof("OnUpgRespUpdate called %d", obj.GetUpgRespVal())
	ctx.deleteUpgReqSpec()
	ctx.invokeAgentHandler(obj)
}

func (ctx *upgrespctx) OnUpgRespDelete(obj *upgrade.UpgResp) {
	log.Infof("OnUpgRespDelete called %d", obj.GetUpgRespVal())
}

//upgRespInit init resp subtree coming from upgrade manager
func upgRespInit(client clientApi.Client, hdlrs AgentHandlers) {
	log.Infof("upgRespInit called")
	ctx := &upgrespctx{
		agentHdlrs: hdlrs,
		sdkClient:  client,
	}
	upgrade.UpgRespMount(client, delphi.MountMode_ReadMode)
	upgrade.UpgRespWatch(client, ctx)
	client.WatchMount(ctx)
	upgRespCtx = *ctx
}

func (ctx *upgrespctx) OnMountComplete() {
	log.Infof("OnMountComplete got called to restore UpgResp")
	upgresp := upgrade.GetUpgResp(ctx.sdkClient)
	if upgresp == nil {
		log.Infof("OnMountComplete could not find UpgResp")
		return
	}
	ctx.OnUpgRespCreate(upgresp)
}
