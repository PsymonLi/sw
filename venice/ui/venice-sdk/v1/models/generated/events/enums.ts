/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */

// generate enum based on strings instead of numbers
// (see https://blog.rsuter.com/how-to-implement-an-enum-with-string-values-in-typescript/)
export enum ApiListWatchOptions_sort_order {
    'none' = "none",
    'by-name' = "by-name",
    'by-name-reverse' = "by-name-reverse",
    'by-version' = "by-version",
    'by-version-reverse' = "by-version-reverse",
    'by-creation-time' = "by-creation-time",
    'by-creation-time-reverse' = "by-creation-time-reverse",
    'by-mod-time' = "by-mod-time",
    'by-mod-time-reverse' = "by-mod-time-reverse",
}

export enum EventsEvent_severity {
    'info' = "info",
    'warn' = "warn",
    'critical' = "critical",
    'debug' = "debug",
}

export enum EventsEvent_type {
    'DISK_THRESHOLD_EXCEEDED' = "DISK_THRESHOLD_EXCEEDED",
    'MIGRATION_FAILED' = "MIGRATION_FAILED",
    'MIGRATION_TIMED_OUT' = "MIGRATION_TIMED_OUT",
    'ORCH_ALREADY_MANAGED' = "ORCH_ALREADY_MANAGED",
    'ORCH_CONFIG_PUSH_FAILURE' = "ORCH_CONFIG_PUSH_FAILURE",
    'ORCH_CONNECTION_ERROR' = "ORCH_CONNECTION_ERROR",
    'ORCH_DSC_MODE_INCOMPATIBLE' = "ORCH_DSC_MODE_INCOMPATIBLE",
    'ORCH_DSC_NOT_ADMITTED' = "ORCH_DSC_NOT_ADMITTED",
    'ORCH_HOST_CONFLICT' = "ORCH_HOST_CONFLICT",
    'ORCH_INVALID_ACTION' = "ORCH_INVALID_ACTION",
    'ORCH_LOGIN_FAILURE' = "ORCH_LOGIN_FAILURE",
    'ORCH_UNSUPPORTED_VERSION' = "ORCH_UNSUPPORTED_VERSION",
    'VNIC_SESSION_LIMIT_EXCEEDED' = "VNIC_SESSION_LIMIT_EXCEEDED",
    'VNIC_SESSION_THRESHOLD_EXCEEDED' = "VNIC_SESSION_THRESHOLD_EXCEEDED",
    'VNIC_SESSION_WITHIN_THRESHOLD' = "VNIC_SESSION_WITHIN_THRESHOLD",
    'AUDITING_FAILED' = "AUDITING_FAILED",
    'AUTO_GENERATED_TLS_CERT' = "AUTO_GENERATED_TLS_CERT",
    'CLOCK_SYNC_FAILED' = "CLOCK_SYNC_FAILED",
    'CONFIG_RESTORED' = "CONFIG_RESTORED",
    'CONFIG_RESTORE_ABORTED' = "CONFIG_RESTORE_ABORTED",
    'CONFIG_RESTORE_FAILED' = "CONFIG_RESTORE_FAILED",
    'DSC_ADMITTED' = "DSC_ADMITTED",
    'DSC_DEADMITTED' = "DSC_DEADMITTED",
    'DSC_DECOMMISSIONED' = "DSC_DECOMMISSIONED",
    'DSC_HEALTHY' = "DSC_HEALTHY",
    'DSC_MAX_SESSION_LIMIT_APPROACH' = "DSC_MAX_SESSION_LIMIT_APPROACH",
    'DSC_MAX_SESSION_LIMIT_REACHED' = "DSC_MAX_SESSION_LIMIT_REACHED",
    'DSC_REJECTED' = "DSC_REJECTED",
    'DSC_UNHEALTHY' = "DSC_UNHEALTHY",
    'DSC_UNREACHABLE' = "DSC_UNREACHABLE",
    'ELECTION_CANCELLED' = "ELECTION_CANCELLED",
    'ELECTION_NOTIFICATION_FAILED' = "ELECTION_NOTIFICATION_FAILED",
    'ELECTION_STARTED' = "ELECTION_STARTED",
    'ELECTION_STOPPED' = "ELECTION_STOPPED",
    'HOST_DSC_SPEC_CONFLICT' = "HOST_DSC_SPEC_CONFLICT",
    'ICMP_ACTIVE_SESSION_LIMIT_APPROACH' = "ICMP_ACTIVE_SESSION_LIMIT_APPROACH",
    'ICMP_ACTIVE_SESSION_LIMIT_REACHED' = "ICMP_ACTIVE_SESSION_LIMIT_REACHED",
    'LEADER_CHANGED' = "LEADER_CHANGED",
    'LEADER_ELECTED' = "LEADER_ELECTED",
    'LEADER_LOST' = "LEADER_LOST",
    'LOGIN_FAILED' = "LOGIN_FAILED",
    'MODULE_CREATION_FAILED' = "MODULE_CREATION_FAILED",
    'NODE_DISJOINED' = "NODE_DISJOINED",
    'NODE_HEALTHY' = "NODE_HEALTHY",
    'NODE_JOINED' = "NODE_JOINED",
    'NODE_UNREACHABLE' = "NODE_UNREACHABLE",
    'OTHER_ACTIVE_SESSION_LIMIT_APPROACH' = "OTHER_ACTIVE_SESSION_LIMIT_APPROACH",
    'OTHER_ACTIVE_SESSION_LIMIT_REACHED' = "OTHER_ACTIVE_SESSION_LIMIT_REACHED",
    'PASSWORD_CHANGED' = "PASSWORD_CHANGED",
    'PASSWORD_RESET' = "PASSWORD_RESET",
    'QUORUM_MEMBER_ADD' = "QUORUM_MEMBER_ADD",
    'QUORUM_MEMBER_HEALTHY' = "QUORUM_MEMBER_HEALTHY",
    'QUORUM_MEMBER_REMOVE' = "QUORUM_MEMBER_REMOVE",
    'QUORUM_MEMBER_UNHEALTHY' = "QUORUM_MEMBER_UNHEALTHY",
    'QUORUM_UNHEALTHY' = "QUORUM_UNHEALTHY",
    'TCP_HALF_OPEN_SESSION_LIMIT_APPROACH' = "TCP_HALF_OPEN_SESSION_LIMIT_APPROACH",
    'TCP_HALF_OPEN_SESSION_LIMIT_REACHED' = "TCP_HALF_OPEN_SESSION_LIMIT_REACHED",
    'UDP_ACTIVE_SESSION_LIMIT_APPROACH' = "UDP_ACTIVE_SESSION_LIMIT_APPROACH",
    'UDP_ACTIVE_SESSION_LIMIT_REACHED' = "UDP_ACTIVE_SESSION_LIMIT_REACHED",
    'UNSUPPORTED_QUORUM_SIZE' = "UNSUPPORTED_QUORUM_SIZE",
    'CONFIG_FAIL' = "CONFIG_FAIL",
    'BGP_SESSION_DOWN' = "BGP_SESSION_DOWN",
    'BGP_SESSION_ESTABLISHED' = "BGP_SESSION_ESTABLISHED",
    'BOND0_NO_IP' = "BOND0_NO_IP",
    'COLLECTOR_REACHABLE' = "COLLECTOR_REACHABLE",
    'COLLECTOR_UNREACHABLE' = "COLLECTOR_UNREACHABLE",
    'LINK_DOWN' = "LINK_DOWN",
    'LINK_UP' = "LINK_UP",
    'ROLLOUT_FAILED' = "ROLLOUT_FAILED",
    'ROLLOUT_STARTED' = "ROLLOUT_STARTED",
    'ROLLOUT_SUCCESS' = "ROLLOUT_SUCCESS",
    'ROLLOUT_SUSPENDED' = "ROLLOUT_SUSPENDED",
    'DSC_MEM_PARTITION_USAGE_ABOVE_THRESHOLD' = "DSC_MEM_PARTITION_USAGE_ABOVE_THRESHOLD",
    'DSC_MEM_PARTITION_USAGE_BELOW_THRESHOLD' = "DSC_MEM_PARTITION_USAGE_BELOW_THRESHOLD",
    'FLOWLOGS_DROPPED' = "FLOWLOGS_DROPPED",
    'FLOWLOGS_RATE_LIMITED' = "FLOWLOGS_RATE_LIMITED",
    'FLOWLOGS_REPORTING_ERROR' = "FLOWLOGS_REPORTING_ERROR",
    'MEM_TEMP_ABOVE_THRESHOLD' = "MEM_TEMP_ABOVE_THRESHOLD",
    'MEM_TEMP_BELOW_THRESHOLD' = "MEM_TEMP_BELOW_THRESHOLD",
    'NAPLES_CATTRIP_INTERRUPT' = "NAPLES_CATTRIP_INTERRUPT",
    'NAPLES_ERR_PCIEHEALTH_EVENT' = "NAPLES_ERR_PCIEHEALTH_EVENT",
    'NAPLES_FATAL_INTERRUPT' = "NAPLES_FATAL_INTERRUPT",
    'NAPLES_INFO_PCIEHEALTH_EVENT' = "NAPLES_INFO_PCIEHEALTH_EVENT",
    'NAPLES_OVER_TEMP' = "NAPLES_OVER_TEMP",
    'NAPLES_OVER_TEMP_EXIT' = "NAPLES_OVER_TEMP_EXIT",
    'NAPLES_PANIC_EVENT' = "NAPLES_PANIC_EVENT",
    'NAPLES_POST_DIAG_FAILURE_EVENT' = "NAPLES_POST_DIAG_FAILURE_EVENT",
    'NAPLES_SERVICE_STOPPED' = "NAPLES_SERVICE_STOPPED",
    'NAPLES_WARN_PCIEHEALTH_EVENT' = "NAPLES_WARN_PCIEHEALTH_EVENT",
    'SERVICE_PENDING' = "SERVICE_PENDING",
    'SERVICE_RUNNING' = "SERVICE_RUNNING",
    'SERVICE_STARTED' = "SERVICE_STARTED",
    'SERVICE_STOPPED' = "SERVICE_STOPPED",
    'SERVICE_UNRESPONSIVE' = "SERVICE_UNRESPONSIVE",
    'SYSTEM_COLDBOOT' = "SYSTEM_COLDBOOT",
    'SYSTEM_RESOURCE_USAGE' = "SYSTEM_RESOURCE_USAGE",
}

export enum EventsEvent_category {
    'cluster' = "cluster",
    'network' = "network",
    'system' = "system",
    'rollout' = "rollout",
    'config' = "config",
    'resource' = "resource",
    'orchestration' = "orchestration",
}

export enum EventsEventAttributes_severity {
    'info' = "info",
    'warn' = "warn",
    'critical' = "critical",
    'debug' = "debug",
}

export enum EventsEventAttributes_category {
    'cluster' = "cluster",
    'network' = "network",
    'system' = "system",
    'rollout' = "rollout",
    'config' = "config",
    'resource' = "resource",
    'orchestration' = "orchestration",
}


export enum ApiListWatchOptions_sort_order_uihint {
    'by-creation-time' = "By Creation Time",
    'by-creation-time-reverse' = "By Creation Time Reverse",
    'by-mod-time' = "By Modification Time",
    'by-mod-time-reverse' = "By Modification Time Reverse",
    'by-name' = "By Name",
    'by-name-reverse' = "By Name Reverse",
    'by-version' = "By Version",
    'by-version-reverse' = "By Version Reverse",
    'none' = "None",
}




/**
 * bundle of all enums for databinding to options, radio-buttons etc.
 * usage in component:
 *   import { AllEnums, minValueValidator, maxValueValidator } from '../../models/webapi';
 *
 *   @Component({
 *       ...
 *   })
 *   export class xxxComponent implements OnInit {
 *       allEnums = AllEnums;
 *       ...
 *       ngOnInit() {
 *           this.allEnums = AllEnums.instance;
 *       }
 *   }
*/
export class AllEnums {
    private static _instance: AllEnums = new AllEnums();
    constructor() {
        if (AllEnums._instance) {
            throw new Error("Error: Instantiation failed: Use AllEnums.instance instead of new");
        }
        AllEnums._instance = this;
    }
    static get instance(): AllEnums {
        return AllEnums._instance;
    }

    ApiListWatchOptions_sort_order = ApiListWatchOptions_sort_order;
    EventsEvent_severity = EventsEvent_severity;
    EventsEvent_type = EventsEvent_type;
    EventsEvent_category = EventsEvent_category;
    EventsEventAttributes_severity = EventsEventAttributes_severity;
    EventsEventAttributes_category = EventsEventAttributes_category;

    ApiListWatchOptions_sort_order_uihint = ApiListWatchOptions_sort_order_uihint;
}
