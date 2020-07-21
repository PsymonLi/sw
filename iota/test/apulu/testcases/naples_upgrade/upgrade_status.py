#! /usr/bin/python3
from enum import Enum

class UpgStage(Enum):
    '''
    Upgrade stage
    '''
    UPG_STAGE_NONE            = 0
    UPG_STAGE_COMPAT_CHECK    = 1
    UPG_STAGE_START           = 2
    UPG_STAGE_BACKUP          = 3
    UPG_STAGE_PREPARE         = 4
    UPG_STAGE_SYNC            = 5
    UPG_STAGE_CONFIG_REPLAY   = 6
    UPG_STAGE_PRE_SWITCHOVER  = 7
    UPG_STAGE_SWITCHOVER      = 8
    UPG_STAGE_READY           = 9
    UPG_STAGE_PRE_RESPAWN     = 10
    UPG_STAGE_RESPAWN         = 11
    UPG_STAGE_ROLLBACK        = 12
    UPG_STAGE_REPEAL          = 13
    UPG_STAGE_FINISH          = 14
    UPG_STAGE_EXIT            = 15
    UPG_STAGE_MAX             = 16 

upg_stage_dict = {stage.name : stage for stage in UpgStage}

def UpgStageFromStr(stage):
    return  upg_stage_dict[stage] if stage in upg_stage_dict else UpgStage.UPG_STAGE_MAX

class UpgStatus(Enum):
    UPG_STATUS_SUCCESS     = 0
    UPG_STATUS_FAILED      = 1
    UPG_STATUS_IN_PROGRESS = 2
    UPG_STATUS_CRITICAL    = 3
    UPG_STATUS_MAX         = 4

upg_status_dict = {
    "success"      : UpgStatus.UPG_STATUS_SUCCESS,
    "failed"       : UpgStatus.UPG_STATUS_FAILED,
    "in-progress" : UpgStatus.UPG_STATUS_IN_PROGRESS,
    "critical"     : UpgStatus.UPG_STATUS_CRITICAL
 }

def UpgStatusFromStr(status):
    return upg_status_dict[status] if status in upg_status_dict else UpgStatus.UPG_STATUS_MAX

class PdsUpgContext:
    def __init__(self, ts, status, stage):
        self.ts = ts
        self.status = UpgStatusFromStr(status)
        self.stage = UpgStageFromStr(stage)
    
    def __str__(self):
        return f"timestamp {self.ts}, status {self.status.name}, stage {self.stage.name}"