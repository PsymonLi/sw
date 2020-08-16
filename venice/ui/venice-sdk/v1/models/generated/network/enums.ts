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

export enum NetworkBGPAuthStatus_status {
    'disabled' = "disabled",
    'enabled' = "enabled",
}

export enum NetworkBGPNeighbor_enable_address_families {
    'ipv4-unicast' = "ipv4-unicast",
    'l2vpn-evpn' = "l2vpn-evpn",
}

export enum NetworkIPAMPolicySpec_type {
    'dhcp-relay' = "dhcp-relay",
}

export enum NetworkNetworkInterfaceSpec_admin_status {
    'up' = "up",
    'down' = "down",
}

export enum NetworkNetworkInterfaceSpec_type {
    'none' = "none",
    'host-pf' = "host-pf",
    'uplink-eth' = "uplink-eth",
    'uplink-mgmt' = "uplink-mgmt",
    'loopback-tep' = "loopback-tep",
}

export enum NetworkNetworkInterfaceSpec_ip_alloc_type {
    'none' = "none",
    'static' = "static",
    'dhcp' = "dhcp",
}

export enum NetworkNetworkInterfaceStatus_type {
    'none' = "none",
    'host-pf' = "host-pf",
    'uplink-eth' = "uplink-eth",
    'uplink-mgmt' = "uplink-mgmt",
    'loopback-tep' = "loopback-tep",
}

export enum NetworkNetworkInterfaceStatus_oper_status {
    'up' = "up",
    'down' = "down",
}

export enum NetworkNetworkSpec_type {
    'bridged' = "bridged",
    'routed' = "routed",
}

export enum NetworkNetworkStatus_oper_state {
    'active' = "active",
    'rejected' = "rejected",
}

export enum NetworkPauseSpec_type {
    'disable' = "disable",
    'link' = "link",
    'priority' = "priority",
}

export enum NetworkPolicerAction_policer_action {
    'drop' = "drop",
}

export enum NetworkRDSpec_address_family {
    'ipv4-unicast' = "ipv4-unicast",
    'l2vpn-evpn' = "l2vpn-evpn",
}

export enum NetworkRouteDistinguisher_type {
    'type0' = "type0",
    'type1' = "type1",
    'type2' = "type2",
}

export enum NetworkTLSServerPolicySpec_client_authentication {
    'mandatory' = "mandatory",
    'optional' = "optional",
    'none' = "none",
}

export enum NetworkTransceiverStatus_state {
    'state_na' = "state_na",
    'state_removed' = "state_removed",
    'state_inserted' = "state_inserted",
    'state_pending' = "state_pending",
    'state_sprom_read' = "state_sprom_read",
    'state_sprom_read_err' = "state_sprom_read_err",
}

export enum NetworkTransceiverStatus_cable_type {
    'none' = "none",
    'copper' = "copper",
    'fiber' = "fiber",
}

export enum NetworkTransceiverStatus_pid {
    'unknown' = "unknown",
    'qsfp_100g_cr4' = "qsfp_100g_cr4",
    'qsfp_40gbase_cr4' = "qsfp_40gbase_cr4",
    'sfp_25gbase_cr_s' = "sfp_25gbase_cr_s",
    'sfp_25gbase_cr_l' = "sfp_25gbase_cr_l",
    'sfp_25gbase_cr_n' = "sfp_25gbase_cr_n",
    'qsfp_100g_aoc' = "qsfp_100g_aoc",
    'qsfp_100g_acc' = "qsfp_100g_acc",
    'qsfp_100g_sr4' = "qsfp_100g_sr4",
    'qsfp_100g_lr4' = "qsfp_100g_lr4",
    'qsfp_100g_er4' = "qsfp_100g_er4",
    'qsfp_40gbase_er4' = "qsfp_40gbase_er4",
    'qsfp_40gbase_sr4' = "qsfp_40gbase_sr4",
    'qsfp_40gbase_lr4' = "qsfp_40gbase_lr4",
    'qsfp_40gbase_aoc' = "qsfp_40gbase_aoc",
    'sfp_25gbase_sr' = "sfp_25gbase_sr",
    'sfp_25gbase_lr' = "sfp_25gbase_lr",
    'sfp_25gbase_er' = "sfp_25gbase_er",
    'sfp_25gbase_aoc' = "sfp_25gbase_aoc",
    'sfp_10gbase_sr' = "sfp_10gbase_sr",
    'sfp_10gbase_lr' = "sfp_10gbase_lr",
    'sfp_10gbase_lrm' = "sfp_10gbase_lrm",
    'sfp_10gbase_er' = "sfp_10gbase_er",
    'sfp_10gbase_aoc' = "sfp_10gbase_aoc",
    'sfp_10gbase_cu' = "sfp_10gbase_cu",
    'qsfp_100g_cwdm4' = "qsfp_100g_cwdm4",
    'qsfp_100g_psm4' = "qsfp_100g_psm4",
    'pid_na' = "pid_na",
}

export enum NetworkVirtualRouterSpec_type {
    'unknown' = "unknown",
    'tenant' = "tenant",
    'infra' = "infra",
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
    NetworkBGPAuthStatus_status = NetworkBGPAuthStatus_status;
    NetworkBGPNeighbor_enable_address_families = NetworkBGPNeighbor_enable_address_families;
    NetworkIPAMPolicySpec_type = NetworkIPAMPolicySpec_type;
    NetworkNetworkInterfaceSpec_admin_status = NetworkNetworkInterfaceSpec_admin_status;
    NetworkNetworkInterfaceSpec_type = NetworkNetworkInterfaceSpec_type;
    NetworkNetworkInterfaceSpec_ip_alloc_type = NetworkNetworkInterfaceSpec_ip_alloc_type;
    NetworkNetworkInterfaceStatus_type = NetworkNetworkInterfaceStatus_type;
    NetworkNetworkInterfaceStatus_oper_status = NetworkNetworkInterfaceStatus_oper_status;
    NetworkNetworkSpec_type = NetworkNetworkSpec_type;
    NetworkNetworkStatus_oper_state = NetworkNetworkStatus_oper_state;
    NetworkPauseSpec_type = NetworkPauseSpec_type;
    NetworkPolicerAction_policer_action = NetworkPolicerAction_policer_action;
    NetworkRDSpec_address_family = NetworkRDSpec_address_family;
    NetworkRouteDistinguisher_type = NetworkRouteDistinguisher_type;
    NetworkTLSServerPolicySpec_client_authentication = NetworkTLSServerPolicySpec_client_authentication;
    NetworkTransceiverStatus_state = NetworkTransceiverStatus_state;
    NetworkTransceiverStatus_cable_type = NetworkTransceiverStatus_cable_type;
    NetworkTransceiverStatus_pid = NetworkTransceiverStatus_pid;
    NetworkVirtualRouterSpec_type = NetworkVirtualRouterSpec_type;

    ApiListWatchOptions_sort_order_uihint = ApiListWatchOptions_sort_order_uihint;
}
