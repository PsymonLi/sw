package upggosdk

import (
	clientApi "github.com/pensando/sw/nic/delphi/gosdk/client_api"
	"github.com/pensando/sw/nic/upgrade_manager/export/upggosdk/nic/upgrade_manager/upgrade"
	"github.com/pensando/sw/venice/utils/log"
)

func canInvokeHandler(sdkClient clientApi.Client, name string, reqType upgrade.UpgReqStateType) bool {
	upgAppResp := upgrade.GetUpgAppResp(sdkClient, name)
	if upgAppResp == nil {
		log.Infof("UpgAppResp not found for %s", name)
		return true
	}
	if getUpgAppRespNextPass(reqType) == upgAppResp.GetUpgAppRespVal() ||
		getUpgAppRespNextFail(reqType) == upgAppResp.GetUpgAppRespVal() {
		log.Infof("Application %s already responded with %s", name, upgAppRespValToStr(upgAppResp.GetUpgAppRespVal()))
		return false
	}
	log.Infof("Can invoke appliation handler")
	return true
}

func createUpgAppResp(sdkClient clientApi.Client, name string) {
	log.Infof("Creating UpgAppResp for %s", name)
	upgAppResp := upgrade.GetUpgAppResp(sdkClient, name)
	if upgAppResp == nil {
		upgAppResp = &upgrade.UpgAppResp{
			Key: name,
		}
		sdkClient.SetObject(upgAppResp)
		if upgAppResp == nil {
			log.Infof("application unable to create response object")
		}
	}
}

func upgAppRespValToStr(respStateType upgrade.UpgStateRespType) string {
	return getUpgAppRespValToStr(respStateType)
}

func updateUpgAppResp(respStateType upgrade.UpgStateRespType, appHdlrResp *HdlrResp, name string, sdkClient clientApi.Client) {
	log.Infof("Updating UpgAppResp for %s", name)
	upgAppResp := upgrade.GetUpgAppResp(sdkClient, name)
	if upgAppResp == nil {
		log.Infof("UpgAppRespHdlr::UpdateUpgAppResp returning error for %s", name)
		return
	}
	if upgAppRespValToStr(respStateType) != "" {
		log.Infof("%s", upgAppRespValToStr(respStateType))
	}
	upgAppResp.UpgAppRespVal = respStateType
	if appHdlrResp.Resp == Fail {
		upgAppResp.UpgAppRespStr = appHdlrResp.ErrStr
	}
	sdkClient.SetObject(upgAppResp)
	return
}

func deleteUpgAppResp(name string, sdkClient clientApi.Client) {
	log.Infof("deleteUpgAppResp called")
	upgAppResp := upgrade.GetUpgAppResp(sdkClient, name)
	if upgAppResp != nil {
		sdkClient.DeleteObject(upgAppResp)
	}
}

func getUpgAppRespNextPass(reqType upgrade.UpgReqStateType) upgrade.UpgStateRespType {
	if reqType == upgrade.UpgReqStateType_UpgStateUpgPossible {
		return canUpgradeStateMachine[reqType].statePassResp
	}
	if upgCtx.upgType == upgrade.UpgType_UpgTypeNonDisruptive {
		return nonDisruptiveUpgradeStateMachine[reqType].statePassResp
	}
	return disruptiveUpgradeStateMachine[reqType].statePassResp
}

func getUpgAppRespNextFail(reqType upgrade.UpgReqStateType) upgrade.UpgStateRespType {
	if reqType == upgrade.UpgReqStateType_UpgStateUpgPossible {
		return canUpgradeStateMachine[reqType].stateFailResp
	}
	if upgCtx.upgType == upgrade.UpgType_UpgTypeNonDisruptive {
		return nonDisruptiveUpgradeStateMachine[reqType].stateFailResp
	}
	return disruptiveUpgradeStateMachine[reqType].stateFailResp
}

func getUpgAppRespNext(reqStateType upgrade.UpgReqStateType, isReqSuccess bool) upgrade.UpgStateRespType {
	if isReqSuccess {
		return getUpgAppRespNextPass(reqStateType)
	}
	return getUpgAppRespNextFail(reqStateType)
}
