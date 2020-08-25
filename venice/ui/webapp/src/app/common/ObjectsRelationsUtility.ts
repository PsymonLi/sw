import { Utility } from '@app/common/Utility';
import { ClusterDistributedServiceCard, ClusterDSCProfile, ClusterDSCProfileSpec_deployment_target, ClusterDSCProfileSpec_feature_set, ClusterHost, IClusterAutoMsgHostWatchHelper, IClusterDistributedServiceCardID } from '@sdk/v1/models/generated/cluster';
import { NetworkNetwork, NetworkNetworkInterface } from '@sdk/v1/models/generated/network';
import { OrchestrationOrchestrator } from '@sdk/v1/models/generated/orchestration';
import { SecurityNetworkSecurityPolicy, SecuritySecurityGroup } from '@sdk/v1/models/generated/security';
import { IWorkloadAutoMsgWorkloadWatchHelper, WorkloadWorkload, WorkloadWorkloadIntfSpec } from '@sdk/v1/models/generated/workload';
import { EventTypes } from '@sdk/v1/services/generated/abstract.service';

export interface ConnectionNode {
    source: any;
    destination: any;
    frequency?: number;
    payloadsum?: number;
}

export interface SimpleGraph {
    [kind: string]: any[];
}

export interface IPrange {
    start: string;
    end: string;
}

export interface SecuritygroupWorkloadPolicyTuple {
    securitygroup: SecuritySecurityGroup;
    workloads: WorkloadWorkload[];
    policies?: SecurityNetworkSecurityPolicy[];
}
export interface WorkloadDSCHostSecurityTuple {
    dscs: ClusterDistributedServiceCard[];
    workload: WorkloadWorkload;
    host: ClusterHost;
    securitygroups?: SecuritySecurityGroup[];
}

export interface DSCWorkloadHostTuple {
    dsc: ClusterDistributedServiceCard;
    workloads: WorkloadWorkload[];
    host: ClusterHost;
}

export interface HostWorkloadTuple {
    workloads: WorkloadWorkload[];
    host: ClusterHost;
}

export interface DSCWorkloadsTuple {
    workloads: WorkloadWorkload[];
    dsc: ClusterDistributedServiceCard;
}

export interface VcenterWorkloadsTuple {
    workloads: WorkloadWorkload[];
    vcenter: OrchestrationOrchestrator;
}

export interface NetworkWorkloadsTuple {
    workloads: WorkloadWorkload[];
    network: NetworkNetwork;
}

export interface DSCnameToMacMap {
    [key: string]: string; // dsc.spec.id - dsc.meta.name
}

export interface DSCmacToNameMap {
    [key: string]: string; // dsc.meta.name- dsc.spec.id
}

export interface DSCsNameMacMap {
    nameToMacMap: DSCnameToMacMap;
    macToNameMap: DSCmacToNameMap;
}

export interface HandleWatchItemResult {
    hasChange: boolean;
    list: any[];
}

/**
 * This utility class includes helper funcitons to link objects
 *
 * A
 * nic.spec.ip-config.ip-address = nic.status.ip-config.ip-address //"172.16.0.3/24"  where spec and status match
 * nic.status.host = host.meta.name //ae-s6
 *
 * B
 * host.spec.smart-nics[i].mac-address = nic.status.primary-mac // "00ae.cd00.17d0"
 * host.spec.smart-nics[i].mac-address = host.status.admitted-smart-nics[i] //"00ae.cd00.17d0"  where spec and status match
 *
 * C.
 * app.status.attached-policies[i] = SGPolicy.meta.name // policy1  app.meta.name = iperfUdpApp // link to (D)
 *
 * D.
 * SecurityNetworkSecurityPolicy (SGPolicy)
   SGPolicy.spec.rules[i].apps[j]= app.meta.name  //iperfUdpApp   SGPolicy.meta.name = policy1
   SGPolicy.spec.rules[i].from-ip-addresses[i] // virtual IP  -  not real DSC's IP  //"10.100.0.101", "10.100.0.102"
   SGPolicy.spec.rules[i].to-ip-addresses[i]  //  virtual IP  -  not real DSC's IP  //"10.100.0.102", "10.100.0.103"  link to (E)
   SGPolicy.spec.attach-groups =  security-group // To F

   SGPolicy.status.rule-status[i] is
    (
      either
        security-group-X to security-group-Y
      or
        from-IP-range to to-IP-range
    )
    &&
    (
       protocal-port or app
    )

  SGPolicy.spec.rules[i] maps to SGPolicy.status.rule-status[i] // the status i-th "rule-hash" --> the i-rule
  rule-hash is log in firewall log
   fwlog record
   {
      "source": "50.141.173.181", //
      "destination": "34.14.21.65",
      "source-port": 2242,
      "destination-port": 3876,
      "protocol": "GRE",
      "action": "allow",
      "direction": "from-host",
      "rule-id": "4918",                // policy's rule
      "session-id": "92",
      "session-state": "flow_delete",0
      "reporter-id": "00ae.cd00.1146",  // link back to DSC  ID:naples-2, mac:00ae.cd00.1146
      "time": "2019-10-03T17:49:06.353077274Z"
    },

 *  E
 *  Workload (Workloads are like docker processes running in one VM (physical box). One Workload has m interfaces/DSCs  )
 *  Workload.spec.host-name = host.meta.name   // link to host (B), host links to physical DSC (A)
 *  Workload.spec.interfaces[i] are Virtual NIC.
 *  Workload.spec.interfaces[i].mac-address !=   NIC.meta.name and NIC.status.primary-mac // "00ae.cd00.50a0"
 *  Workload.spec.interfaces[i].ip-addresses[i] = policy.spec.rules[i].to-ip-addresses or from-ip-addresses // "10.100.0.103"
 *
 *  F. SecurityGroup (2019-11-06 understanding)
 *    SecurityGroup.spec.WorkloadSelector[i] = Workload.meta.labels?
 *    SecurityGroup.spec.MatchPrefixes[i] = other security groups (name)
 *    SecurityGroup.status.Workloads[i] = workloads names  // To E
 *    SecurityGroup.status.Policies = Policies names  // To D
 */
export class ObjectsRelationsUtility {

    public static WORKLOAD_APP_LABEL_KEY_PREFIX: string = 'config.app.';

    public static buildDSCsNameMacMap(naples: ReadonlyArray<ClusterDistributedServiceCard> | ClusterDistributedServiceCard[]): DSCsNameMacMap {
        const _myDSCnameToMacMap: DSCsNameMacMap = {
            nameToMacMap: {},
            macToNameMap: {}
        };

        for (const smartnic of naples) {
            if (smartnic.spec.id != null && smartnic.spec.id !== '') {
                _myDSCnameToMacMap.nameToMacMap[smartnic.spec.id] = smartnic.meta.name;
                _myDSCnameToMacMap.macToNameMap[smartnic.meta.name] = smartnic.spec.id;
            }
        }
        return _myDSCnameToMacMap;
    }
    /**
     * Build a map key=workload.meta.name, value = 'WorkloadDSCHostTuple' object
     *
     * workload -- 1:1 --> host
     * host -- 1:m --> DSCs
     * workload and security-groups are linked
     */
    public static buildWorkloadDscHostSecuritygroupMap(workloads: ReadonlyArray<WorkloadWorkload> | WorkloadWorkload[],
        naples: ReadonlyArray<ClusterDistributedServiceCard> | ClusterDistributedServiceCard[],
        hosts: ReadonlyArray<ClusterHost> | ClusterHost[],
        securitygroups: ReadonlyArray<SecuritySecurityGroup> | SecuritySecurityGroup[],
    ): { [workloadkey: string]: WorkloadDSCHostSecurityTuple; } {
        const workloadDSCHostTupleMap: { [workloadkey: string]: WorkloadDSCHostSecurityTuple; } = {};
        for (const workload of workloads) {
            const workloadSpecHostName = workload.spec['host-name'];
            const host: ClusterHost = this.getHostByMetaName(hosts, workloadSpecHostName);
            let nics: ClusterDistributedServiceCard[] = [];
            if (host) {
                nics = this.getDSCsByHost(naples, host);
            }
            let mysecurtiygrous: SecuritySecurityGroup[] = [];
            if (securitygroups) {
                mysecurtiygrous = this.getSecurityGroupsByWorkload(securitygroups, workload);
            }

            const newTuple: WorkloadDSCHostSecurityTuple = {
                workload: workload,
                host: host,
                dscs: nics,
                securitygroups: mysecurtiygrous
            };
            workloadDSCHostTupleMap[workload.meta.name] = newTuple;
        }
        return workloadDSCHostTupleMap;
    }

    /**
     * Build a map key=dsc.meta.name, value = 'DSCWorkloadHostTuple' object
     *
     * dsc -- 1:1 --> host
     * host -- 1:m --> workload
     *
     * e.g DSC page can use this api to find workloads
     * const dscWorkloadHostMap  = ObjectsRelationsUtility.buildDscWorkloadHostMap(this.dataObjects, this.naples, this.hostObjects );

     */
    public static buildDscWorkloadHostMap(workloads: ReadonlyArray<WorkloadWorkload> | WorkloadWorkload[],
        naples: ReadonlyArray<ClusterDistributedServiceCard> | ClusterDistributedServiceCard[],
        hosts: ReadonlyArray<ClusterHost> | ClusterHost[],
    ): { [dsckey: string]: DSCWorkloadHostTuple; } {
        const dscWorkloadHostMap: { [dsckey: string]: DSCWorkloadHostTuple; } = {};
        for (const dsc of naples) {
            const hostNameFromDsc = dsc.status.host;
            const host: ClusterHost = this.getHostByMetaName(hosts, hostNameFromDsc);
            const linkworkloads = this.findAllWorkloadsInHosts(workloads, hosts);
            const newTuple: DSCWorkloadHostTuple = {
                workloads: linkworkloads,
                host: host,
                dsc: dsc
            };
            dscWorkloadHostMap[dsc.meta.name] = newTuple;
        }
        return dscWorkloadHostMap;
    }

    /**
     * Build a map key=host.meta.name, value = 'HostWorkloadTuple' object
     *
     * dsc -- 1:1 --> host
     * host -- 1:m --> workloads
     *
     * e.g Host page can use this api to find workloads
     * const hostWorkloadsTuple  = ObjectsRelationsUtility.buildHostWorkloadsMap(this.hosts this.workloads );
     *
     */
    public static buildHostWorkloadsMap(workloads: ReadonlyArray<WorkloadWorkload> | WorkloadWorkload[],
        hosts: ReadonlyArray<ClusterHost> | ClusterHost[],
    ): { [hostKey: string]: HostWorkloadTuple; } {
        const hostWorkloadsTuple: { [hostKey: string]: HostWorkloadTuple; } = {};
        for (const host of hosts) {
            const linkworkloads = this.findAssociatedWorkloadsByHost(workloads, host);
            const newTuple: HostWorkloadTuple = {
                workloads: linkworkloads,
                host: host
            };
            hostWorkloadsTuple[host.meta.name] = newTuple;
        }
        return hostWorkloadsTuple;
    }

    /**
     * Build a map key=dsc.meta.name, value = 'DSCWorkloadsTuple' object
     *
     * dsc -- 1:1 --> host
     * host -- 1:m --> workloads
     * @param workloads
     * @param naples
     */
    public static buildDscWorkloadsMaps(workloads: ReadonlyArray<WorkloadWorkload> | WorkloadWorkload[], naples: ReadonlyArray<ClusterDistributedServiceCard> | ClusterDistributedServiceCard[])
        : { [dscKey: string]: DSCWorkloadsTuple; } {
        const dscWorkloadsTuple: { [dscKey: string]: DSCWorkloadsTuple; } = {};
        for (const naple of naples) {
            const linkworkloads = this.findAssociatedWorkloadsByDSC(workloads, naple);
            const newTuple: DSCWorkloadsTuple = {
                workloads: linkworkloads,
                dsc: naple
            };
            dscWorkloadsTuple[naple.meta.name] = newTuple;
        }
        return dscWorkloadsTuple;
    }

    /**
 * Build a map key=dsc.meta.name, value = 'DSCWorkloadsTuple' object
 *
 * dsc -- 1:1 --> host
 * host -- 1:m --> workloads
 * @param workloads
 * @param naples
 */
    public static buildDscWorkloadMaps(workloads: ReadonlyArray<WorkloadWorkload> | WorkloadWorkload[], naples: ReadonlyArray<ClusterDistributedServiceCard> | ClusterDistributedServiceCard[])
        : any[] {
        const dscWorkloadMap = [];
        const dscWorkloads = {};

        for (const naple of naples) {
            const linkworkloads = this.findAssociatedWorkloadsByDSC(workloads, naple);
            dscWorkloads[naple.meta.name] = linkworkloads;
        }

        Object.keys(dscWorkloads).forEach(dsc => {
            dscWorkloadMap.push({ dsc: dsc, workload: dscWorkloads[dsc] });
        });
        return dscWorkloadMap;
    }

    /**
     * Build a map key=vcenter.meta.name, value = 'Workload' object
     *
     * vcenter -- 1:m --> workloads
     *
     * e.g Vcenter page can use this api to find workloads
     * const vcenterWorkloadsTuple  = ObjectsRelationsUtility.buildVcenterWorkloadsMap(this.vcenters this.workloads );
     *
     */
    public static buildVcenterWorkloadsMap(workloads: ReadonlyArray<WorkloadWorkload> | WorkloadWorkload[],
        vcenters: ReadonlyArray<OrchestrationOrchestrator>): VcenterWorkloadsTuple {
        const vcenterWorkloadsTuple: VcenterWorkloadsTuple = {} as VcenterWorkloadsTuple;
        vcenters.forEach((vcenter: OrchestrationOrchestrator) => {
            const linkworkloads: WorkloadWorkload[] = this.findAssociatedWorkloadsByVcenter(workloads as any[], vcenter);
            vcenterWorkloadsTuple[vcenter.meta.name] = linkworkloads;
        });
        return vcenterWorkloadsTuple;
    }

    /**
     * Build a map key=network.meta.name, value = 'Workload' object
     *
     * network -- 1:m --> workloads
     *
     * e.g network page can use this api to find workloads
     * const networkWorkloadsTuple  = ObjectsRelationsUtility.buildNetworkWorkloadsMap(this.networks this.workloads );
     *
     */
    public static buildNetworkWorkloadsMap(workloads: ReadonlyArray<WorkloadWorkload> | WorkloadWorkload[],
        networks: ReadonlyArray<NetworkNetwork>): NetworkWorkloadsTuple {
        const networkWorkloadsTuple: NetworkWorkloadsTuple = {} as NetworkWorkloadsTuple;
        networks.forEach((network: NetworkNetwork) => {
            const linkworkloads: WorkloadWorkload[] = this.findAssociatedWorkloadsByNetwork(workloads as any[], network);
            networkWorkloadsTuple[network.meta.name] = linkworkloads;
        });
        return networkWorkloadsTuple;
    }

    public static buildSecuitygroupWorkloadPolicyMap(securityGroups: ReadonlyArray<SecuritySecurityGroup> | SecuritySecurityGroup[], workloads: ReadonlyArray<WorkloadWorkload> | WorkloadWorkload[], securitypolicies?: ReadonlyArray<SecurityNetworkSecurityPolicy> | SecurityNetworkSecurityPolicy[])
        : { [securitygroupKey: string]: SecuritygroupWorkloadPolicyTuple; } {
        const securitygroupWorkloadPolicyTuple: { [securitygroupKey: string]: SecuritygroupWorkloadPolicyTuple; } = {};
        for (const securitygroup of securityGroups) {
            const inclduedworkloads = this.findAssociatedWorkloadsFromSecuritygroup(workloads, securitygroup);
            const policies = this.findIncludedSecuritypolicesFromSecuritygroup(securitypolicies, securitygroup);
            const newTuple: SecuritygroupWorkloadPolicyTuple = {
                workloads: inclduedworkloads,
                securitygroup: securitygroup,
                policies: policies
            };
            securitygroupWorkloadPolicyTuple[securitygroup.meta.name] = newTuple;
        }
        return securitygroupWorkloadPolicyTuple;
    }


    /**
     * Given a DSC/Naple, find all associated workloads
     *  dsc -- 1:1 --> host  -- 1..m -> workloads
     * @param workloads
     * @param naple
     */
    public static findAssociatedWorkloadsByDSC(workloads: ReadonlyArray<WorkloadWorkload> | WorkloadWorkload[], naple: ClusterDistributedServiceCard): WorkloadWorkload[] {
        const workloadWorkloads: WorkloadWorkload[] = [];
        for (const workload of workloads) {
            const hostname = naple.status.host;
            if (workload.spec['host-name'] === hostname) {
                workloadWorkloads.push(workload);
            }
        }
        return workloadWorkloads;
    }

    /**
     * Give a ClusterHost, find all the workloads that associated with this host
     * @param workloads
     * @param host
     */
    public static findAssociatedWorkloadsByHost(workloads: ReadonlyArray<WorkloadWorkload> | WorkloadWorkload[], host: ClusterHost): WorkloadWorkload[] {
        const workloadWorkloads: WorkloadWorkload[] = [];
        for (const workload of workloads) {
            const workloadSpecHostName = workload.spec['host-name'];
            if (host.meta.name === workloadSpecHostName) {
                workloadWorkloads.push(Utility.trimUIFields(workload));
            }
        }
        return workloadWorkloads;
    }

    public static findAssociatedWorkloadsByVcenter(workloadList: WorkloadWorkload[], vcenter: OrchestrationOrchestrator): WorkloadWorkload[] {
        return workloadList.filter((workload: WorkloadWorkload) => {
            return workload && workload.meta && workload.meta.labels && workload.meta.labels['io.pensando.orch-name'] === vcenter.meta.name;
        });
    }

    public static findAssociatedWorkloadsByNetwork(workloadList: WorkloadWorkload[], network: NetworkNetwork): WorkloadWorkload[] {
        return workloadList.filter((workload: WorkloadWorkload) => {
            if (workload.spec && workload.spec.interfaces && workload.spec.interfaces.length > 0) {
                for (let i = 0; i < workload.spec.interfaces.length; i++) {
                    const itf = workload.spec.interfaces[i];
                    if (itf && itf.network === network.meta.name) {
                        return true;
                    }
                }
            }
            return false;
        });
    }

    public static findAllWorkloadsInHosts(workloads: ReadonlyArray<WorkloadWorkload> | WorkloadWorkload[], hosts: ReadonlyArray<ClusterHost> | ClusterHost[]): WorkloadWorkload[] {
        const workloadWorkloads: WorkloadWorkload[] = [];
        for (const workload of workloads) {
            const workloadSpecHostName = workload.spec['host-name'];
            const host: ClusterHost = this.getHostByMetaName(hosts, workloadSpecHostName);
            if (host) {
                workloadWorkloads.push(workload);
            }
        }
        return workloadWorkloads;
    }



    public static getDSCsByHost(naples: ReadonlyArray<ClusterDistributedServiceCard> | ClusterDistributedServiceCard[], host: ClusterHost): ClusterDistributedServiceCard[] {
        if (!host) {
            return [];
        }
        const hostSpecDSClen = host.spec.dscs.length;
        const nics: ClusterDistributedServiceCard[] = [];
        for (let i = 0; i < hostSpecDSClen; i++) {
            const hostSpecDsc: IClusterDistributedServiceCardID = host.spec.dscs[i];
            if (hostSpecDsc['mac-address'] != null) {
                const macAddress = hostSpecDsc['mac-address'];
                const dsc = this.getDSCByMACaddress(naples, macAddress);
                if (dsc) {
                    nics.push(dsc);
                }
            } else if (hostSpecDsc.id != null) {
                const hostId = hostSpecDsc.id;
                const dsc = this.getDSCById(naples, hostId);
                if (dsc) {
                    nics.push(dsc);
                }
            }
        }
        return nics;
    }

    public static getHostByMetaName(hosts: ReadonlyArray<ClusterHost> | ClusterHost[], hostname: string): ClusterHost {
        if (!hosts) {
            return null;
        }
        const interfacesLength = hosts.length;
        for (let i = 0; i < interfacesLength; i++) {
            const host: ClusterHost = hosts[i];
            if (host.meta.name === hostname) {
                return host;
            }
        }
        return null;
    }

    public static getDSCById(naples: ReadonlyArray<ClusterDistributedServiceCard> | ClusterDistributedServiceCard[], hostId: string): ClusterDistributedServiceCard {
        const interfacesLength = naples.length;
        for (let i = 0; i < interfacesLength; i++) {
            const nicSpecId = naples[i].spec.id;
            if (nicSpecId === hostId) {
                return naples[i];
            }
        }
        return null;
    }

    public static getDSCByMACaddress(naples: ReadonlyArray<ClusterDistributedServiceCard> | ClusterDistributedServiceCard[], macAddress: string): ClusterDistributedServiceCard {
        const interfacesLength = naples.length;
        for (let i = 0; i < interfacesLength; i++) {
            const napleMac = naples[i].status['primary-mac'];
            if (napleMac === macAddress) {
                return naples[i];
            }
        }
        return null;
    }

    /**
     * As a user, I want find trace from ip-1 to ip-2
     *  1. ip1's workload,  workload to DSC
     *     ip2's workload,  workload to DSC
     *  2.
     */

    /**
     * Given an ipAddress, find all the workloads
     * @param workloads
     * @param ipAddress
     */
    public static getWorkloadFromIPAddress(workloads: ReadonlyArray<WorkloadWorkload> | WorkloadWorkload[], ipAddress: string): WorkloadWorkload[] {
        const matchingWorkloads: WorkloadWorkload[] = [];
        for (const workload of workloads) {
            const interfaces: WorkloadWorkloadIntfSpec[] = workload.spec.interfaces;
            for (const wlInterface of interfaces) {
                const matchedIps = wlInterface['ip-addresses'].find((ip) => {
                    return (ip === ipAddress);
                });
                if (matchedIps && matchedIps.length > 0) {
                    matchingWorkloads.push(workload);
                }
            }
        }
        return matchingWorkloads;
    }

    /**
     * Give a workload, this API finds all connected workloads
     * @param workloads
     * @param givenWorkload
     */
    public static findPossibleConnectedWorkloadsFromWorkload(workloads: ReadonlyArray<WorkloadWorkload> | WorkloadWorkload[], givenWorkload: WorkloadWorkload): WorkloadWorkload[] {
        const workloadWorkloads: WorkloadWorkload[] = [];
        const interfaces: WorkloadWorkloadIntfSpec[] = givenWorkload.spec.interfaces;
        for (const wlInterface of interfaces) {
            const ips = wlInterface['ip-addresses'];
            for (const ip of ips) {
                const wls: WorkloadWorkload[] = this.getWorkloadFromIPAddress(workloads, ip);
                workloadWorkloads.concat(...wls);
            }
        }
        return workloadWorkloads;
    }

    /**
     * Given securitygroup, find associated workloads
     * Logic:  all securitygroup.spec['workload-selector'].requirements[i] .key = workload.meta.labels.key
     *                                                                     .value = workload.meta.labels.value
     * "workload-selector": {
            "requirements": [
              {
                "key": "env",
                "operator": "in",
                "values": [
                  "21"
                ]
              },
              {
                "key": "tag",
                "operator": "in",
                "values": [
                  "1"
                ]
              }
            ]
          }
     * worloads
       {
        "events": [
            {
            "type": "Created",
            "object": {
                "kind": "Workload",
                "api-version": "v1",
                "meta": {
                    "name": "wl1-a",
                    "labels": {
                        "env": "21",  // match
                        "tag": "1"
                    }
                }
            }
            },
            {
            "type": "Created",
            "object": {
                "kind": "Workload",
                "api-version": "v1",
                "meta": {
                    "name": "wl1",
                    "labels": {   // match
                        "env": "21",
                        "tag": "1"
                    }
                }
            }
            }
        ]
        }
     * @param workloads
     * @param securityGroup
     */
    public static findAssociatedWorkloadsFromSecuritygroup(workloads: ReadonlyArray<WorkloadWorkload> | WorkloadWorkload[], securityGroup: SecuritySecurityGroup): WorkloadWorkload[] {
        const workloadWorkloads: WorkloadWorkload[] = [];
        for (const workload of workloads) {
            /* let allRequirementMatch: boolean = true;
            for (const sgroupWorkloadSelectorRequirement of securityGroup.spec['workload-selector'].requirements) {
                const key = sgroupWorkloadSelectorRequirement.key;
                const values = sgroupWorkloadSelectorRequirement.values;
                const labelValue = workload.meta.labels[key];
                if (labelValue) {
                    if (!values.includes(labelValue)) {
                        allRequirementMatch = false;
                        break;
                    }
                } else {
                    allRequirementMatch = false;
                    break;
                }
            } */
            const allRequirementMatch = this.isSecurityGroupWorkloadLabelMatchWorkloadLabels(securityGroup, workload);
            if (allRequirementMatch) {
                workloadWorkloads.push(workload);
            }
        }
        return workloadWorkloads;
    }

    public static isSecurityGroupWorkloadLabelMatchWorkloadLabels(securityGroup: SecuritySecurityGroup, workload: WorkloadWorkload): boolean {
        let allRequirementMatch: boolean = true;
        for (const sgroupWorkloadSelectorRequirement of securityGroup.spec['workload-selector'].requirements) {
            const key = sgroupWorkloadSelectorRequirement.key;
            const values = sgroupWorkloadSelectorRequirement.values;
            const operator = sgroupWorkloadSelectorRequirement.operator;
            const labelValue = workload.meta.labels[key];
            if (labelValue) {
                if (!values.includes(labelValue)) { // TODO: add more check here. Here, it only handles operator is "equal"
                    allRequirementMatch = false;
                    break;
                }
            } else {
                allRequirementMatch = false;
                break;
            }
        }
        return allRequirementMatch;
    }

    /**
     * Give a securityGroup, find all assoicated security-policies
     * Logic:  securitypolicy.spec['attach-groups'][i] === securityGroup.meta.name
     *
     * @param securitypolicies
     * @param securitygroup
     */
    public static findIncludedSecuritypolicesFromSecuritygroup(securitypolicies: ReadonlyArray<SecurityNetworkSecurityPolicy> | SecurityNetworkSecurityPolicy[], securitygroup: SecuritySecurityGroup): SecurityNetworkSecurityPolicy[] {
        const securityNetworkSecurityPolicies: SecurityNetworkSecurityPolicy[] = [];
        for (const securitypolicy of securitypolicies) {
            const attachedGroups = securitypolicy.spec['attach-groups'];
            for (const attachedGroup of attachedGroups) {
                if (attachedGroup === securitygroup.meta.name) {
                    securityNetworkSecurityPolicies.push(securitypolicy);
                }
            }
        }
        return securityNetworkSecurityPolicies;
    }

    static getSecurityGroupsByWorkload(securitygroups: ReadonlyArray<SecuritySecurityGroup> | SecuritySecurityGroup[], workload: WorkloadWorkload): SecuritySecurityGroup[] {
        const linkedSecurityGroups: SecuritySecurityGroup[] = [];
        for (const securityGroup of securitygroups) {
            const allRequirementMatch = this.isSecurityGroupWorkloadLabelMatchWorkloadLabels(securityGroup, workload);
            if (allRequirementMatch) {
                linkedSecurityGroups.push(securityGroup);
            }
        }
        return linkedSecurityGroups;
    }


    /**
    *
    * @param veniceObjectWatchHelper
    * @param type
    */
    public static findTypedItemsFromWSResponse(veniceObjectWatchHelper: IWorkloadAutoMsgWorkloadWatchHelper | IClusterAutoMsgHostWatchHelper, type: EventTypes) {
        const typedItemList = [];
        if (!veniceObjectWatchHelper) {
            return [];
        }
        const events = veniceObjectWatchHelper.events;
        for (let i = 0; events && i < events.length; i++) {
            if (events[i].type === type && events[i].object) {
                typedItemList.push(events[i].object);
            }
        }
        return typedItemList;
    }

    public static handleAddedItemsFromWatch(addedItems: any[], dataList: any[], genFunc: (item: any) => any): HandleWatchItemResult {
        let needUpdate: boolean = false;
        for (let i = 0; i < addedItems.length; i++) {
            const index = dataList.findIndex((w) => w.meta.name === addedItems[i].meta.name);
            if (index < 0) {
                const workload = genFunc(addedItems[i]);  // genFunc is like: new WorkloadWorkload(addedItems[i]);
                dataList = dataList.concat(workload); // insert into workloadList
                needUpdate = true;
            }
        }
        const handleWatchItemResult: HandleWatchItemResult = {
            hasChange: needUpdate,
            list: dataList
        };
        return handleWatchItemResult;
    }

    public static handleUpdatedItemsFromWatch(updatedItems: any[], dataList: any[], genFunc: (item: any) => any): HandleWatchItemResult {
        let needUpdate: boolean = false;
        for (let i = 0; i < updatedItems.length; i++) {
            const index = dataList.findIndex((w) => w.meta.name === updatedItems[i].meta.name);
            if (index >= 0) {
                const workload = genFunc(updatedItems[i]);
                dataList[index] = workload;  // update to new value
                needUpdate = true;
            }
        }
        const handleWatchItemResult: HandleWatchItemResult = {
            hasChange: needUpdate,
            list: dataList
        };
        return handleWatchItemResult;
    }

    public static handleDeletedItemsFromWatch(deletedIems: any[], dataList: any[]): HandleWatchItemResult {
        let needUpdate: boolean = false;
        for (let i = 0; i < deletedIems.length; i++) {
            const index = dataList.findIndex((w) => w.meta.name === deletedIems[i].meta.name);
            if (index >= 0) {
                dataList.splice(index, 1);
                needUpdate = true;
            }
        }
        const handleWatchItemResult: HandleWatchItemResult = {
            hasChange: needUpdate,
            list: dataList
        };
        return handleWatchItemResult;
    }

    public static getNetworkinterfaceUIName(selectedNetworkInterface: NetworkNetworkInterface, naplesList: ClusterDistributedServiceCard[], delimiter: string = '-'): string {
        const myDSCnameToMacMap = ObjectsRelationsUtility.buildDSCsNameMacMap(naplesList);
        const niName = selectedNetworkInterface.meta.name;
        const idx = niName.indexOf(delimiter);
        const macPart = niName.substring(0, idx);
        const typePart = niName.substring(idx + 1);
        let dscname = myDSCnameToMacMap.macToNameMap[selectedNetworkInterface.status.dsc];
        if (niName.indexOf(dscname) === 0) {
            return niName; // VS-1374 as niName='systest-naples-6-uplink-1-2' and dscname = 'systest-naples-6'
        }
        if (!dscname) {
            // NI-name is 00ae.cd01.0ed8-uplink129 where NI.status.dsc is missing.
            dscname = myDSCnameToMacMap.macToNameMap[macPart];
            if (!dscname) {
                // NI-name is 00aecd0115e0-pf-70
                dscname = myDSCnameToMacMap.macToNameMap[Utility.chunk(macPart, 4).join('.')];
            }
        }
        return dscname ? dscname + '-' + typePart : niName;
    }

    public static getDSCFromNetworkinterface(selectedNetworkInterface: NetworkNetworkInterface, naplesList: ClusterDistributedServiceCard[]): ClusterDistributedServiceCard {
        const dscMac = selectedNetworkInterface.status.dsc;
        return naplesList.find((dsc: ClusterDistributedServiceCard) => dsc.meta.name === dscMac);
    }

    public static getDSCProfileFromDSC(dsc: ClusterDistributedServiceCard, dscprofiles: ClusterDSCProfile[]): ClusterDSCProfile {
        const dscProfilenameFromDSC = dsc.spec.dscprofile;
        const dscProfile = dscprofiles.find((profile: ClusterDSCProfile) => profile.meta.name === dscProfilenameFromDSC);
        return dscProfile;
    }

    public static isDSCInDSCProfileFeatureSet(dsc: ClusterDistributedServiceCard, dscprofiles: ClusterDSCProfile[], featureSet: ClusterDSCProfileSpec_feature_set): boolean {
        const profile: ClusterDSCProfile = this.getDSCProfileFromDSC(dsc, dscprofiles);
        if (!profile) {
            return false;
        }
        return (profile.spec['feature-set'] === featureSet);
    }

    public static isDSCInDSCProfileDeploymentTarget(dsc: ClusterDistributedServiceCard, dscprofiles: ClusterDSCProfile[], deploymentTarget: ClusterDSCProfileSpec_deployment_target): boolean {
        const profile: ClusterDSCProfile = this.getDSCProfileFromDSC(dsc, dscprofiles);
        if (!profile) {
            return false;
        }
        return (profile.spec['deployment-target'] === deploymentTarget);
    }


    /**
     * This API is needed in DSC metrics charts.
     * When creating a graph with a stats category like session stats (available with the flow-aware or flow-aware+firewall Feature Sets), ensure that only DSC’s in that mode are selectable
     *
     * @param dsc
     * @param dscprofiles
     * @param featureSet
     */
    public static isDSCInFlowawareOrFallwallMode(dsc: ClusterDistributedServiceCard, dscprofiles: ClusterDSCProfile[]): boolean {
        return (this.isDSCInDSCProfileFeatureSet(dsc, dscprofiles, ClusterDSCProfileSpec_feature_set.flowaware) ||
            this.isDSCInDSCProfileFeatureSet(dsc, dscprofiles, ClusterDSCProfileSpec_feature_set.flowaware_firewall));
    }

    /**
     * This API is needed in Network Interface metrics charts
     * We first find the DSC which owns the given selectedNetworkInterface object
     * Then, we compute wheter the DSC is the right mode
     * @param selectedNetworkInterface
     * @param dscs
     * @param dscprofiles
     */
    public static isNetworkInterfaceInFlowawareOrFallwallMode(selectedNetworkInterface: NetworkNetworkInterface, dscs: ClusterDistributedServiceCard[], dscprofiles: ClusterDSCProfile[]): boolean {
        const dsc: ClusterDistributedServiceCard = this.getDSCFromNetworkinterface(selectedNetworkInterface, dscs);
        return this.isDSCInFlowawareOrFallwallMode(dsc, dscprofiles);
    }

    /**
     * This API is needed in Workload Component
     * @param WorkloadWorkload
     * Given a workload object, loop through workload.spec.interface[] to make workload.meta.labels[]
     *      workload.spec.interfaces:[{
     *          "mac-address": "0050.56b8.15b4",
     *          "micro-seg-vlan": 1034,
     *      },...]}
     * workload.meta.labels key name as "config.app.X" and intetface's micro-seg-vlan value ('1034') as value
     *      workload.meta.labels:[
     *          { "config.app.0": "App-1034" },
     *          { "config.app.1": "App-xxx" }]
     * if  label key "config.app.x"  already exists, don't over write the key and value.
     */

    public static updateWorkloadAppLabelWithInterface(workload: WorkloadWorkload): WorkloadWorkload {
        workload.spec.interfaces.map((interfaceObj, index) => {
            if (interfaceObj['micro-seg-vlan']) {
                // We try to make "config.app.0": "App-1034";
                const labelValue = 'App-' + interfaceObj['micro-seg-vlan'];
                const labelKey = ObjectsRelationsUtility.WORKLOAD_APP_LABEL_KEY_PREFIX + index;
                // If key config.app.# does not exist, we add to workload.meta.labels
                if (!workload.meta.labels) {
                    workload.meta.labels = {};
                }
                if (!workload.meta.labels[labelKey]) {
                    workload.meta.labels[labelKey] = labelValue;
                }
            }
        });
        return workload;
    }

    /**
     * Remove all 'config.app.#' key in workload.meta.labels {..}
     * @param workload
     */
    public static removeAllWorkloadAppLabels(workload: WorkloadWorkload): WorkloadWorkload {
        if (!workload.meta.labels) {
            return workload;
        }
        const configApps = Object.keys(workload.meta.labels).filter(label => label.includes(ObjectsRelationsUtility.WORKLOAD_APP_LABEL_KEY_PREFIX));
        configApps.map((appName) => {
            delete workload.meta.labels[appName];
        });
        return workload;
    }

    /**
     * This API is convienence function to check if node-start can reach node-end
     * @param graph
     * @param start
     * @param end
     */
    public static isNodeReachable(graph: SimpleGraph, start, end): boolean {
        const pathOne = this.findReachablePath(graph, start, end);
        return (!!pathOne); // pathOne can be undefind.  use "!!" trick to return boolean
    }

    /**
     * This API returns the first rearch-able path from "start" to "end" node
     * It uses BSF . O(n) is linear
     * @param graph
     * @param start
     * @param end
     */
    public static findReachablePath(graph: SimpleGraph, start, end): any {
        const deque = [];
        const dist = {};
        dist[start] = [start];
        deque.push(start);
        while (deque.length > 0) {
            const at = Utility.getLodash().cloneDeep(deque[0]); // have to clone "at" as next line is going to remove it.
            deque.shift();
            if (graph[at]) {
                for (const next of graph[at]) {
                    if (!dist[next]) {
                        dist[next] = dist[at].concat(next);
                        deque.push(next);
                    }
                    if (next === end) {
                        return dist[end];
                    }
                }
            }
        }
        return dist[end];
    }

    /**
     * This API builds a matrix reachable path bewteen two nodes
     * @param graph
     * @param nodes
     *
     * output (notice: A-B-D)
     * "{"A":{"B":["A","B"],"C":["A","C"],"D":["A","B","D"]},"B":{"C":["B","C"],"D":["B","D"]},"C":{"D":["C","D"]},"E":{"F":["E","F"]}}"
     */
    public static findReachablePathMatrix(graph: SimpleGraph, nodes: any[]) {
        const allPaths = [];
        const matrix = {};
        for (let i = 0; i < nodes.length; i++) {
            let iObj = null;
            for (let j = i + 1; j < nodes.length; j++) {
                console.log ('findRearchablePathMatrix()', i, j, nodes[i], nodes[j]);
                if (nodes[i] !== nodes[j]) {
                    const ijPath = this.findReachablePath(graph, nodes[i], nodes[j]);
                    if (!!ijPath) {
                        iObj = (iObj) ? iObj : {};
                        allPaths.push(ijPath);
                        iObj[nodes[j]] = ijPath;
                    }
                }
            }
            if (iObj) {
                matrix[nodes[i]] = iObj;
            }
        }
        return matrix;
    }

    /**
     * A helper function to compute paths for two nodes in a graph
     * @param inputgraph
     * @param startNode
     * @param end
     * @param currentPath
     * @param allpaths
     * graph  as
     * {"A":["B","C"],"B":["C","D"],"C":["D"],"D":["C"],"E":["F"],"F":["C"]}
     * findAllPathsFor2Nodes(graph, 'A', 'D', [], validPaths)
     * [['A', 'B', 'C', 'D'], ['A', 'B', 'D'], ['A', 'C', 'D']]
     *
     * reference: https://www.python.org/doc/essays/graphs/
     * Time complexity is O(n!).  Be careful !!!!
     */
    public static findAllPathsFor2NodesInGraphHelper(graph, startNode, endNode, currentPath, validPaths: any[]) {
        if (!currentPath) {
            currentPath = [];
        }
        currentPath.push(startNode);
        if (startNode === endNode) {
            validPaths.push([...currentPath]);
            return;
        }
        const adjacencyList = graph[startNode];
        for (let i = 0; adjacencyList && i < adjacencyList.length; i++) {
            const node = adjacencyList[i];
            if (!currentPath.includes(node)) {
                const newPath = Utility.getLodash().cloneDeepWith(currentPath); // must clone currentPath to avoid value being overwritten
                this.findAllPathsFor2NodesInGraphHelper(graph, node, endNode, newPath, validPaths);
            }
        }
    }

    /**
     * Compute paths for two nodes in a graph.
     * const allpathsAD =  ObjectsRelationsUtility.findAllPathsFor2NodesInGraph(graph, 'A', 'D');
     * @param graph
     * @param startNode
     * @param endNode
     */
    public static findAllPathsFor2NodesInGraph(graph, startNode, endNode): any[] {
        const allpaths = [];
        ObjectsRelationsUtility.findAllPathsFor2NodesInGraphHelper(graph, startNode, endNode, [], allpaths);
        return allpaths;
    }

    /**
     * Find all paths of a given graph
     * @param graph
     * @param nodes
     * graph  as
     * {"A":["B","C"],"B":["C","D"],"C":["D"],"D":["C"],"E":["F"],"F":["C"]}
     * nodes is Objects.key(graph)
     *
     */
    public static findAllGraphPaths(graph: SimpleGraph | any, nodes: any[]): any[] {
        const allPaths = [];
        for (let i = 0; i < nodes.length; i++) {
            for (let j = i + 1; j < nodes.length; j++) {
                const ijAllpaths = [];
                if (nodes[i] !== nodes[j]) {
                    this.findAllPathsFor2NodesInGraphHelper(graph, nodes[i], nodes[j], [], ijAllpaths);
                    if (ijAllpaths.length) {
                        allPaths.push(ijAllpaths);
                    }
                }
            }
        }
        return allPaths;
    }

    /**
     * Given a node, this API finds all the nodes that this reference node can reerch to
     * @param refNode
     * @param allPaths
     */
    public static findAllNodeDestinationsFromAllpaths(refNode, allPaths: any[]) {
        const endWithmes = [];
        for (let i = 0; i < allPaths.length; i++) {
            const iThPath = allPaths[i][0];
            const startElem = iThPath[0];
            if (startElem === refNode) {
                const lastElem = iThPath[iThPath.length - 1];
                if (!endWithmes.includes(lastElem)) {
                    endWithmes.push(lastElem);
                }
            }
        }
        return endWithmes;
    }

    /**
     * Given a node, the API finds all other nodes that can reach to this reference node.
     * @param refNode
     * @param allPaths
     */
    public static findAllNodesCanReachMeFromAllpaths(refNode, allPaths: any[]) {
        const canReachmeNodes = [];
        for (let i = 0; i < allPaths.length; i++) {
            const iThPath = allPaths[i][0];
            const lastElem = iThPath[iThPath.length - 1];
            if (lastElem === refNode) {
                const startElem = iThPath[0];
                if (!canReachmeNodes.includes(startElem)) {
                    canReachmeNodes.push(startElem);
                }
            }
        }
        return canReachmeNodes;
    }

    public static findAllSourcesByConnections(inputNode, connections: ConnectionNode[], nodes: any[]): any[] {
        const allPaths = this.findAllGraphPathsWithConnections(connections, nodes);
        return this.findAllNodesCanReachMeFromAllpaths(inputNode, allPaths);
    }


    public static findAllDestinationsByConnections(inputNode, connections: ConnectionNode[], nodes: any[]): any[] {
        const allPaths = this.findAllGraphPathsWithConnections(connections, nodes);
        return this.findAllNodeDestinationsFromAllpaths(inputNode, allPaths);
    }

    /**
     * Connvience API to build paths from connections.
     * Internally, convert connections to graph
     * @param connections
     * @param nodes
     */
    public static findAllGraphPathsWithConnections(connections: ConnectionNode[], nodes: any[]): any[] {
        const graph: SimpleGraph = this.buildGraphFromConnections(connections);
        return this.findAllGraphPaths(graph, nodes);
    }

    /**
    * Connvience API to build paths from connections, given startNode and endNode
    * Internally, convert connections to graph
    * @param connections
    * @param nodes
    */
    public static findAllPathsFor2NodesFromConnections(connections: ConnectionNode[], startNode, endNode): any[] {
        const graph: SimpleGraph = this.buildGraphFromConnections(connections);
        return ObjectsRelationsUtility.findAllPathsFor2NodesInGraph(graph, startNode, endNode);
    }

    /**
     *
     * @param connections
     */
    public static buildGraphFromConnections(connections: ConnectionNode[]): SimpleGraph {
        const graph: SimpleGraph = {};
        for (let i = 0; i < connections.length; i++) {
            const node = connections[i];
            if (!graph[node.source]) {
                graph[node.source] = [];
            }
            const list = graph[node.source];
            if (!list.includes(node.destination)) {
                list.push(node.destination);
            }
        }
        return graph;
    }

    /**
     * Compress connections list which has duplicated entries to histogram.
     * From
     * [{"source":"C","target":"D"},{"source":"D","target":"C"},{"source":"E","target":"F"},{"source":"F","target":"C"},{"source":"C","target":"D"},{"source":"D","target":"C"}]
     * to (add frequecy and payloadsum)
     * [{"source":"C","target":"D","frequency":2,"payloadsum":0},{"source":"D","target":"C","frequency":2,"payloadsum":0},{"source":"E","target":"F","frequency":1,"payloadsum":0},{"source":"F","target":"C","frequency":1,"payloadsum":0}]
     * @param connections
     */
    public static compressConnections(connections: ConnectionNode[],
        makeKay: (node: ConnectionNode) => string = (node: ConnectionNode) => {
            return node.source + '__' + node.destination;
        }): ConnectionNode[] {
        const newconnections: ConnectionNode[] = [];
        const graph: { [key: string]: ConnectionNode } = {};
        for (let i = 0; i < connections.length; i++) {
            const node = connections[i];
            const key = makeKay(node);
            if (!graph[key]) {
                const newNode = Utility.getLodash().cloneDeep(node);
                if (!newNode.frequency) {
                    newNode.frequency = 1;
                }
                if (!newNode.payloadsum) {
                    newNode.payloadsum = 0;
                }
                graph[key] = newNode;
                newconnections.push(newNode);
            } else {
                graph[key].frequency += 1;
                if (node.payloadsum) {
                    graph[key].payloadsum += node.payloadsum;
                }
            }
        }
        return newconnections;
    }

    /**
     * Given a ip string and a workload, find the application that use this IP
     * @param ip
     * @param workload
     */
    public static findWorkloadApplicationFromIp(ip: string, workload: WorkloadWorkload): string {
        let index = -1;
        const interfaces: WorkloadWorkloadIntfSpec[] = workload.spec.interfaces;
        for (let i = 0; i < interfaces.length; i++) {
            const wlInterface = interfaces[i];
            const matchedIps = wlInterface['ip-addresses'].find((ipAddress) => {
                return (ip === ipAddress);
            });
            if (matchedIps) {
                index = i;
                break;
            }
        }
        if (index >= 0) {
            const keys = Object.keys(workload.meta.labels).filter(label => label.includes(ObjectsRelationsUtility.WORKLOAD_APP_LABEL_KEY_PREFIX + index));
            const key = keys[0];
            const appName = workload.meta.labels[key];
            // the current format is config.app.# = appname. In the future will have
            // config.app.0 = {name}__{type}__{ip}__{protocol/port}
            // for example:
            // config.app.0 = nginx__webserver__10.1.10.7__tcp/80
            // Other option?
            // Other o{name}__{ip}__{protocol/port}
            return appName;
        }
        return null;
    }

    /**
     * Given a ip, try to find application associated with thi IP
     * @param ip
     * @param workloads
     */
    public static findAppNameFromWorkloadsByIp(ip: string, workloads: WorkloadWorkload[]): string {
        const workload = this.getWorkloadFromIPAddress(workloads, ip);
        if (workload && workload.length > 0) {
            return this.findWorkloadApplicationFromIp(ip, workload[0]);
        }
        return null;
    }

    /**
     * Utility function to compute the ip range
     * @param ipAddress
     * For example: input '152.2.136.0/26' ==> '152.2.136.0 - '152.2.136.63
     */
    public static getIpRangeFromAddressAndNetmask(ipAddress: string): IPrange {
        const part = ipAddress.split('/'); // part[0] = base address, part[1] = netmask
        const ipaddress = part[0].split('.');
        let netmaskblocks: any = ['0', '0', '0', '0'];
        if (!/\d+\.\d+\.\d+\.\d+/.test(part[1])) {
            // part[1] has to be between 0 and 32
            netmaskblocks = ('1'.repeat(parseInt(part[1], 10)) + '0'.repeat(32 - parseInt(part[1], 10))).match(/.{1,8}/g);
            netmaskblocks = netmaskblocks.map((el) => parseInt(el, 2));
        } else {
            // xxx.xxx.xxx.xxx
            netmaskblocks = part[1].split('.').map((el) => parseInt(el, 10));
        }
        // invert for creating broadcast address (highest address)
        // tslint:disable-next-line: no-bitwise
        const invertedNetmaskblocks = netmaskblocks.map((el: any) => el ^ 255);
        // tslint:disable-next-line: no-bitwise
        const baseAddress = ipaddress.map((block: any, idx) => block & netmaskblocks[idx]);
        // tslint:disable-next-line: no-bitwise
        const broadcastaddress = baseAddress.map(function (block, idx) { return block | invertedNetmaskblocks[idx]; });
        const ipRange: IPrange = {
            start: baseAddress.join('.'),
            end: broadcastaddress.join('.')
        };
        return ipRange;
        // return [baseAddress.join('.'), broadcastaddress.join('.')];
    }

    /**
     * Check if input ip address fall into a range of [start -  end]
     * For example: '152.2.136.24'  is in range of  '152.2.136.0 - '152.2.136.63
     * @param ipaddr
     * @param start
     * @param end
     */
    public static checkIpaddrInRange(ipaddr: string, start: string, end: string) {
        const IPtoNum: (ip: string) => Number = (ip) => {
            return Number(
                ip.split('.')
                    .map(d => ('000' + d).substr(-3))
                    .join('')
            );
        };
        return (IPtoNum(start) < IPtoNum(ipaddr) && IPtoNum(end) > IPtoNum(ipaddr));
    }

    /**
     * Check if '152.2.136.24'  is in range of '152.2.136.0/26'
     * @param ipaddress
     * @param ipWithMask
     */
    public static checkIpAddressInRangeWithIPMask(ipaddress: string, ipWithMask: string): boolean {
        const range = this.getIpRangeFromAddressAndNetmask(ipWithMask);
        return this.checkIpaddrInRange(ipaddress, range.start, range.end);
    }

    public static testGraphCode() {
        const myIp = '152.2.136.0/26';
        const range = this.getIpRangeFromAddressAndNetmask(myIp);

        const testInRangeIP = '152.2.136.24';
        const isInRange = this.checkIpaddrInRange(testInRangeIP, range.start, range.end);
        console.log(myIp + ' is in range: [' + range.start + ' , ' + range.end + '] ' + testInRangeIP + ' is in range? ' + isInRange);

        const graph: SimpleGraph = {
            'A': ['B', 'C'],
            'B': ['C', 'D'],
            'C': ['D'],
            'D': ['C'],
            'E': ['F'],
            'F': ['C']
        };

        const connections: ConnectionNode[] = [
            {
                source: 'A',
                destination: 'B'
            },
            {
                source: 'A',
                destination: 'C'
            },
            {
                source: 'B',
                destination: 'C'
            },
            {
                source: 'B',
                destination: 'D'
            },
            {
                source: 'C',
                destination: 'D'
            },
            {
                source: 'D',
                destination: 'C'
            },
            {
                source: 'E',
                destination: 'F'
            },
            {
                source: 'F',
                destination: 'C'
            }
            ,
            // purposely duplicate entries
            {
                source: 'C',
                destination: 'D'
            },
            {
                source: 'D',
                destination: 'C'
            },
        ];

        const pathAD = ObjectsRelationsUtility.findReachablePath(graph, 'A', 'D');
        const pathCD = ObjectsRelationsUtility.findReachablePath(graph, 'C', 'D');
        const isRearchableAE = ObjectsRelationsUtility.isNodeReachable(graph, 'A', 'E');
        console.log('A-D is reachable', pathAD, pathCD, isRearchableAE);

        const matrix = ObjectsRelationsUtility.findReachablePathMatrix(graph, Object.keys(graph));
        console.log('Graph matrix', matrix);

        const allpathsAD = ObjectsRelationsUtility.findAllPathsFor2NodesInGraph(graph, 'A', 'D');
        console.log('A-D All Paths', allpathsAD);
        const allpathsBF = ObjectsRelationsUtility.findAllPathsFor2NodesInGraph(graph, 'B', 'F');
        console.log('B-F All Paths', allpathsBF);
        const allpathsEC = ObjectsRelationsUtility.findAllPathsFor2NodesInGraph(graph, 'E', 'C');
        console.log('E-C All Paths', allpathsEC);

        const allGraphPaths = ObjectsRelationsUtility.findAllGraphPaths(graph, Object.keys(graph));
        console.log('All Graph Paths', allGraphPaths);

        const myGraph = ObjectsRelationsUtility.buildGraphFromConnections(connections);
        const connAllGraphPaths = ObjectsRelationsUtility.findAllGraphPaths(myGraph, Object.keys(myGraph));
        console.log('All Connection Graph Paths', connAllGraphPaths);

        const testfindAllGraphPathsWithConnections = this.findAllGraphPathsWithConnections(connections, Object.keys(myGraph));
        console.log('testfindAllGraphPathsWithConnections', testfindAllGraphPathsWithConnections);

        const testfindAllPathsFor2NodesFromConnections = this.findAllPathsFor2NodesFromConnections(connections, 'E', 'C');
        console.log('testfindAllPathsFor2NodesFromConnections', testfindAllPathsFor2NodesFromConnections);

        const testcompressConnections = this.compressConnections(connections);
        console.log('testcompressConnections', testcompressConnections);

        const testFindAllSourcesByConnections = this.findAllSourcesByConnections('D', connections, Object.keys(myGraph));
        console.log('testFindAllSourcesByConnections. Nodes can go to D ', testFindAllSourcesByConnections);

        const testFindAllDestionationsByConnections = this.findAllDestinationsByConnections('A', connections, Object.keys(myGraph));
        console.log('testFindAllDestionationsByConnections. Nodes A can rearch out to', testFindAllDestionationsByConnections);

    }
}
