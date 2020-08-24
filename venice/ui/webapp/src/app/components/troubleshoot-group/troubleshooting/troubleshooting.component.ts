import { Component, ViewEncapsulation, OnInit, ViewChild } from '@angular/core';
import { Node, Link, NodeType, NodeStates, LinkStates } from '@app/models/frontend/shared/networkgraph.interface';
import { Utility } from '@app/common/Utility';
import { NodeConsts } from './networkgraph/networkgraph.component';
import { FormControl } from '@angular/forms';
import { NetworkgraphComponent } from './networkgraph/networkgraph.component';
import { ControllerService } from '@app/services/controller.service';
import { DSCWorkloadsTuple, ObjectsRelationsUtility } from '@app/common/ObjectsRelationsUtility';
import { WorkloadWorkload } from '@sdk/v1/models/generated/workload';
import { WorkloadService } from '@app/services/generated/workload.service';
import { ClusterService } from '@app/services/generated/cluster.service';
import { SecurityService } from '@app/services/generated/security.service';
import { ISecuritySGRule, SecuritySGRule, SecurityNetworkSecurityPolicy, SecuritySGRule_action_uihint, ISecurityNetworkSecurityPolicy, SecurityApp } from '@sdk/v1/models/generated/security';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';
import { TableCol } from '@app/components/shared/tableviewedit';
import { SelectItem } from 'primeng/api';
import { ITelemetry_queryMetricsQueryResponse } from '@sdk/v1/models/telemetry_query';
import { ITelemetry_queryMetricsQueryResult } from '@sdk/v1/models/telemetry_query';
import { Telemetry_queryMetricsQuerySpec, ITelemetry_queryMetricsQuerySpec, Telemetry_queryMetricsQuerySpec_function, Telemetry_queryMetricsQuerySpec_sort_order, Telemetry_queryMetricsQueryResult } from '@sdk/v1/models/generated/telemetry_query';
import { FwlogFwLogQuery, FwlogFwLogQuery_sort_order, FwlogFwLogList } from '@sdk/v1/models/generated/fwlog';
import { MetricsUtility } from '@app/common/MetricsUtility';
import { IPUtility } from '@app/common/IPUtility';
import { FormArray } from '@angular/forms';
import { SearchPolicySearchRequest, ISearchPolicyMatchEntry, ISearchPolicySearchResponse, SearchPolicySearchResponse_status } from '@sdk/v1/models/generated/search';
import { SearchService } from '@app/services/generated/search.service';
import { ClusterDistributedServiceCard, ClusterDSCProfile, ClusterDSCProfileSpec_feature_set, ClusterDSCProfileSpec_deployment_target } from '@sdk/v1/models/generated/cluster';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { ActivatedRoute, Router } from '@angular/router';
import { Observable } from 'rxjs';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { Location } from '@angular/common';
import { MetricsqueryService, TelemetryPollingMetricQueries, MetricsPollingQuery } from '@app/services/metricsquery.service';
import { IFieldsSelector } from '@sdk/v1/models/generated/search';
import { FwlogService } from '@app/services/generated/fwlog.service';
import { IMonitoringTechSupportRequest, MonitoringTechSupportRequest, IApiStatus } from '@sdk/v1/models/generated/monitoring';
import { TechSupport } from '../../../../../e2e/page-objects/techsupport.po';
import { formatDate } from '@angular/common';
import { Icon } from '@app/models/frontend/shared/icon.interface';

export enum TabShown {
  Topology,
  Policies
}
class SecuritySGRuleWrapper {
  order: number;
  rule: ISecuritySGRule;
  ruleHash: string;
}

interface RuleHitEntry {
  EspHits: number;
  IcmpHits: number;
  OtherHits: number;
  TcpHits: number;
  TotalHits: number;
  UdpHits: number;
}

@Component({
  selector: 'app-troubleshooting',
  templateUrl: './troubleshooting.component.html',
  styleUrls: ['./troubleshooting.component.scss'],
  encapsulation: ViewEncapsulation.None
})
export class TroubleshootingComponent extends DataComponent implements OnInit {
  @ViewChild('networkgraph') graph: NetworkgraphComponent;
  @ViewChild('rulesTable') rulesTable: PentableComponent;

  bodyicon: any = {
    margin: {
      top: '9px',
      left: '8px'
    },
    url: '/assets/images/icons/icon-troubleshooting.svg'
  };
  graphBufferSide = 120;
  graphBufferBottom = 40;
  graphBufferTop = 40;
  METRICS_POLLING_INTERVAL = 60000;
  runForceLayout = false;
  nodes: Node[] = [];
  links: Link[] = [];
  showTabs = false;
  showNetworkGraph = true;
  sourceFormControl: FormControl = new FormControl('', []);
  destFormControl: FormControl = new FormControl('', []);
  portFormControl: FormControl = new FormControl('', []);
  sourceSubmitted: boolean = false;
  destSubmitted: boolean = false;
  portSubmitted: boolean = false;
  protocolSubmitted: boolean = false;
  sourceNodeID: string = 's1';
  destNodeID: string = 'd1';
  sourceIP: string = '';
  destIP: string = '';
  destPort: string = null;
  sourceWorkload: WorkloadWorkload = null;
  destWorkload: WorkloadWorkload = null;
  sourceDSC: ClusterDistributedServiceCard = null;
  destDSC: ClusterDistributedServiceCard = null;
  tabsTriggered: boolean = false;
  subscriptions = [];
  sourceWorkloadFound: boolean = false;
  destWorkloadFound: boolean = false;

  // subscription for elastic search queries
  rulesSearchSubscription;
  fwlogsSearchSubscription;
  // Current filter applied to all the data in the table
  currentSearch;
  showRowExpand: boolean = false;

  workloads: ReadonlyArray<WorkloadWorkload> = [];
  ipMap = {};
  dscOptions: { [dscKey: string]: DSCWorkloadsTuple; };
  naplesList: ClusterDistributedServiceCard[] = [];
  policies: ISecurityNetworkSecurityPolicy[] = [];

  exportFilename: string = 'PSM-rules';
  maxSearchRecords: number = 8000;
  dataObjects: ReadonlyArray<any> = [];
  dataObjectsBackup: ReadonlyArray<any> = [];

  // map from rule index to rule hash
  ruleHashMap: { [index: number]: string } = {};
  // Map from rule has to aggregate hits
  ruleMetrics: { [hash: string]: RuleHitEntry } = {};
  // Map from rule hash to tooltip string
  // Helps avoid rebuidling the string unless there are changes
  ruleMetricsTooltip: { [hash: string]: string } = {};

  // display flags
  showTable = false;
  showTestResults = false;
  // Used for the table - when true there is a loading icon displayed
  tableLoading: boolean = false;
  tabShown = TabShown.Topology;
  // diagnostic test variables
  dscprofiles: ReadonlyArray<ClusterDSCProfile> = [];
  sourceDSCisAdmitted: boolean = false;
  sourceDSCPhase: String = '';
  sourceDSCCondition: String = '';
  sourceWorkloadisSystemGenerated: boolean = false;
  sourceDSCisVirtualized: boolean = false;
  sourceDSCisinFirewallProfile: boolean = false;
  sourceDSCHasPropagatedPolicy: boolean = false;
  sourcePolicyPropagatedPolicy: boolean = false;

  destDSCisAdmitted: boolean = false;
  destDSCPhase: String = '';
  destDSCCondition: String = '';
  destWorkloadisSystemGenerated: boolean = false;
  destDSCisVirtualized: boolean = false;
  destDSCisinFirewallProfile: boolean = false;
  destDSCHasPropagatedPolicy: boolean = false;
  destPolicyPropagatedPolicy: boolean = false;

  srcTest1Passed: boolean = false;
  srcTest2Passed: boolean = false;
  srcTest3Passed: boolean = false;
  destTest1Passed: boolean = false;
  destTest2Passed: boolean = false;
  destTest3Passed: boolean = false;

  // tech Support Variables
  showTechSupport = false;

  // Rule EDIT MODE Variables
  ruleEditableMap: { [index: number]: boolean } = {};
  editObject: SecurityNetworkSecurityPolicy = null;
  newRuleIndex: number = -1;
  showReorder: boolean = false;
  reorderToIndex: number = 0;
  display: boolean = false;
  securityApps: ReadonlyArray<SecurityApp> = [];
  securityAppOptions: SelectItem[] = [];

  // DSC Drop Metrics Variables
  srcDropData = [];
  destDropData = [];
  showSourceDSCDropData = false;
  showDestDSCDropData = false;

  // these stats are from backend
  lifInterfacePollingStats: ITelemetry_queryMetricsQueryResult = null;
  pifInterfacePollingStats: ITelemetry_queryMetricsQueryResult = null;

  // firewall log query Variables
  logAction = null;
  logCreationTime = null;
  logProtocol = null;
  logSourcePort = null;
  logDestPort = null;
  showFirewallLogSpecs = true;

  tableIcon: Icon = {
    margin: {
      top: '0px',
      left: '0px',
    },
    matIcon: 'grid_on'
  };

  cols: TableCol[] = [
    { field: 'order', header: 'Number', class: 'sgpolicy-rule-number', width: 4, sortable: true },
    { field: 'sourceIPs', header: 'Source IPs', class: 'sgpolicy-source-ip', width: 22 },
    { field: 'destIPs', header: 'Destination IPs', class: 'sgpolicy-dest-ip', width: 22 },
    { field: 'rule.action', header: 'Action', class: 'sgpolicy-action', width: 24, sortable: true },
    { field: 'protocolPort', header: 'Protocol / Application', class: 'sgpolicy-port', width: 20 },
    { field: 'TotalHits', header: 'Total Connection Hits', class: 'sgpolicy-rule-stat', width: 10 }
  ];

  ruleHitItems: SelectItem[] = [
    { label: 'TCP Hits', value: 'TcpHits' },
    { label: 'UDP Hits', value: 'UdpHits' },
    { label: 'ICMP Hits', value: 'IcmpHits' },
    { label: 'ESP Hits', value: 'EspHits' },
    { label: 'Other Hits', value: 'OtherHits' },
    { label: 'Total Hits', value: 'TotalHits' },
  ];

  constructor(protected controllerService: ControllerService,
    protected workloadService: WorkloadService,
    protected clusterService: ClusterService,
    protected securityService: SecurityService,
    protected uiconfigsService: UIConfigsService,
    protected metricsqueryService: MetricsqueryService,
    protected monitoringService: MonitoringService,
    private _route: ActivatedRoute,
    private _router: Router,
    private location: Location,
    protected searchService: SearchService,
    protected fwlogService: FwlogService
  ) { super(controllerService, uiconfigsService); }

  testGraphCode() {
    ObjectsRelationsUtility.testGraphCode();
  }

  ngOnInit() {
    this.tableLoading = true;
    this.showNetworkGraph = true;
    let buttons = [];
    buttons = [
      {
        cssClass: 'global-button-primary troubleshooting-techsupportrequests-toolbar-button troubleshooting-techsupportrequests-toolbar-button-ADD',
        text: 'ADD TECH-SUPPORT REQUEST',
        computeClass: () => this.showTechSupport ? '' : 'global-button-disabled',
        callback: () => { this.createNewTechSupportRequest(); }
      },
      {
        cssClass: 'global-button-primary troubleshooting-techsupportrequests-toolbar-button troubleshooting-techsupportrequests-toolbar-button-ADD',
        text: 'EXPORT REPORT',
        callback: () => { this.exportJSON(); },
      }
    ];
    if (this.uiconfigsService.isFeatureEnabled('troubleshooting')) {
      buttons.push(
        {
          cssClass: 'global-button-primary troubleshooting-techsupportrequests-toolbar-button troubleshooting-techsupportrequests-toolbar-button-ADD',
          text: 'Test Graph Paths',
          callback: () => { this.testGraphCode(); },
        }
      );
    }

    this.controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [{ label: 'Troubleshooting', url: Utility.getBaseUIUrl() + 'troubleshoot/troubleshooting' }]
    });

    this.defaultLayout();
    this.getWorkloads();
    this.getSecurityPolicies();

    this._route.queryParams.subscribe(params => {
      if (params.hasOwnProperty('sourceIP')) {
        if (IPUtility.isValidIP(params.sourceIP)) {
          this.sourceIP = params.sourceIP;
        }
      }
      if (params.hasOwnProperty('destIP')) {
        if (IPUtility.isValidIP(params.destIP)) {
          this.destIP = params.destIP;
        }
      }
      if (params.hasOwnProperty('port')) {
        this.destPort = params.port;
      }
    });
  }

  /**
 * Fetch workloads.
 */
  getWorkloads() {
    const workloadSubscription = this.workloadService.ListWorkloadCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.workloads = response.data as WorkloadWorkload[];
        this.buildIPMap();
        this.getDSCs();
      },
      this.controllerService.webSocketErrorHandler('Failed to get workloads')
    );
    this.subscriptions.push(workloadSubscription);
  }

  /**
* Builds Map {IP Address: Workload }
*/
  buildIPMap() {
    this.ipMap = {};
    // Taking IPs from spec, since status isn't always filled out currently
    this.workloads.forEach((w) => {
      w.spec.interfaces.forEach((intf) => {
        intf['ip-addresses'].forEach((ip) => {
          this.ipMap[ip] = w;
        });
      });
    });
  }

  /**
* Fetch DSCs
*/
  getDSCs() {
    const dscSubscription = this.clusterService.ListDistributedServiceCardCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.naplesList = response.data as ClusterDistributedServiceCard[];
        this.dscOptions = ObjectsRelationsUtility.buildDscWorkloadsMaps(this.workloads, this.naplesList);
        if (this.sourceIP) {
          this.submitSource();
        }
        if (this.destIP) {
          this.submitDest();
        }
        if (this.destPort) {
          this.submitPort();
        }
      }
    );
    this.subscriptions.push(dscSubscription);

    const subscription = this.clusterService.ListDSCProfileCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        this.dscprofiles = response.data;
      },
      this._controllerService.webSocketErrorHandler('Failed to get DSC Profile')
    );
    this.subscriptions.push(subscription); // add subscription to list, so that it will be cleaned up when component is destroyed.
  }

  /**
 * Fetches workload from ipOptions map
 */
  getWorkloadFromIP(IPAddress: string) {
    if (this.ipMap.hasOwnProperty(IPAddress)) {
      return this.ipMap[IPAddress];
    }
    return null;
  }

  /**
* Creates dscOptions map similar to ipOptions finds the DSC associated to source/destination Workload
*/
  getDSCfromWorkload(workload: WorkloadWorkload, isSource: boolean) {
    let dscName = '';
    Object.keys(this.dscOptions).forEach(dsc => {
      if (this.dscOptions[dsc].workloads.includes(workload)) {
        dscName = this.dscOptions[dsc].dsc.spec.id;
        if (isSource) {
          this.sourceDSC = this.dscOptions[dsc].dsc;
        } else {
          this.destDSC = this.dscOptions[dsc].dsc;
        }
      }
    });
    return dscName;
  }

  getInterfaceIndexFromWorkload(workload: WorkloadWorkload, IP) {
    return workload.spec.interfaces.findIndex((i) => {
      return i['ip-addresses'].includes(IP);
    });
  }

  /**
* Fetches Security Policies
*/
  getSecurityPolicies() {
    this.securityService.ListNetworkSecurityPolicyCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.policies = response.data as SecurityNetworkSecurityPolicy[];
        this.ruleHashMap = {};
        if (Array.isArray(this.policies) && this.policies.length > 0) {
          this.policies[0].status['rule-status'].forEach((rule, index) => {
            this.ruleHashMap[index] = rule['rule-hash'];
          });
        }
        this.getPolicyMetrics();
      },
      (error) => {
        this.controllerService.invokeRESTErrorToaster('Failed to get network security policies', error);
      }
    );
  }

  getSelectedDataObjects(): any[] {
    return this.rulesTable.selectedDataObjects;
  }

  clearSelectedDataObjects() {
    this.rulesTable.selectedDataObjects = [];
  }

  displayColumn(rowData: SecuritySGRuleWrapper, col: TableCol): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(rowData, fields);
    const column = col.field;
    switch (column) {
      case 'order':
        return rowData.order + 1;
      case 'protocolPort':
        return this.formatApp(rowData.rule);
      case 'sourceIPs':
        return rowData.rule['from-ip-addresses'].join(', ');
      case 'destIPs':
        return rowData.rule['to-ip-addresses'].join(', ');
      case 'rule.action':
        return SecuritySGRule_action_uihint[rowData.rule.action];
      case 'TotalHits':
        const entry = this.ruleMetrics[rowData.ruleHash];
        if (entry == null) {
          return '';
        }
        return entry.TotalHits;
    }
  }

  onColumnSelectChange(event) {
    this.rulesTable.onColumnSelectChange(event);
  }
  /**
*FUNCTIONS FOR SG POLICY RULE EDITOR
*/

  onAdd(rowData, isBefore: boolean = false) {
    this.newRuleIndex = isBefore ? rowData.order : (rowData.order + 1);
    this.onRuleAddCreateHelper();
  }

  private onRuleAddCreateHelper() {
    this.editObject = new SecurityNetworkSecurityPolicy(this.policies[0]);
    this.editObject.spec.rules = [];
    this.display = true;
  }

  onUpdate() {
    const editrules: Array<SecuritySGRule> = [];
    this.editObject = new SecurityNetworkSecurityPolicy(this.policies[0]);
    this.getSelectedDataObjects().forEach((ruleObj, index) => {
      editrules.push(ruleObj.rule);
    });
    this.editObject.spec.rules = editrules;
    this.display = true;
  }

  onSave(editedObject: SecurityNetworkSecurityPolicy) {
    if (editedObject) {
      const policy = (this.newRuleIndex > -1) ?
        this.updateFromNewRule(editedObject, this.newRuleIndex) :
        this.updateFromEditObject(editedObject);
      this.updatePolicy(policy);
    }
  }

  onDelete() {
    const selectedObjs = this.rulesTable.selectedDataObjects;
    if (selectedObjs && selectedObjs.length > 0) {
      const length = selectedObjs.length;
      const msg = `Are you sure you want to delete ${length === 1 ? 'this' : 'these'} \
        SG Policy ${length === 1 ? 'Rule' : 'Rules'}?`;
      this.controllerService.invokeConfirm({
        header: msg,
        message: 'This action cannot be reversed',
        acceptLabel: 'Delete',
        accept: () => {
          const selectedIdxMap = {};
          selectedObjs.forEach(item => {
            selectedIdxMap[item.order] = true;
          });
          const policy1 = this.policies[0];
          const rules = policy1.spec.rules.filter((rule, idx) => !selectedIdxMap[idx]);
          policy1.spec.rules = rules;
          this.updatePolicy(policy1);
        }
      });
    }
  }

  reorderKeyUp(event) {
    // hit enter to submit
    if (event.keyCode === 13) {
      this.onReorder();
      return;
    }
    // no '.' ,  '+' , '-'
    if (event.keyCode === 187 || event.keyCode === 189 ||
      event.keyCode === 190) {
      event.preventDefault();
    }
    this.reorderToIndex = event.target.value;
  }

  isReorderReady() {
    return this.reorderToIndex > 0;
  }

  onReorder() {
    const selectedObjs = this.getSelectedDataObjects();
    if (selectedObjs.length === 1 && this.reorderToIndex > 0) {
      const policy1 = this.policies[0];
      const curIdx: number = selectedObjs[0].order;
      // delte selected rule
      const arrDeletedItems = policy1.spec.rules.splice(curIdx, 1);
      let insertIndex = this.reorderToIndex - 1;
      if (insertIndex > policy1.spec.rules.length) {
        insertIndex = policy1.spec.rules.length;
      }
      policy1.spec.rules.splice(insertIndex, 0, arrDeletedItems[0]);
      this.updatePolicy(policy1);
    }
  }
  updateFromNewRule(editedObject, index: number) {
    const policy = editedObject;
    const policy1 = Utility.getLodash().cloneDeep(this.policies[0]);
    policy1.spec.rules.splice(index, 0, policy.spec.rules[0]);
    return policy1;
  }

  updateFromEditObject(editedObject) {
    const policy = editedObject;
    const policy1 = this.policies[0];
    let count = 0;
    for (let i = 0; i < policy1.spec.rules.length; i++) {
      if (this.ruleEditableMap[i] || this.reorderToIndex > 0) {
        policy1.spec.rules[i] = policy.spec.rules[count];
        count++;
      }
    }
    return policy1;
  }

  updatePolicy(policy) {
    if (policy) {
      let handler: Observable<{ body: ISecurityNetworkSecurityPolicy | IApiStatus | Error, statusCode: number }>;
      (<any>policy).meta.name = (<any>this.policies[0]).meta.name;
      handler = this.updateObject(policy, this.policies[0]);

      handler.subscribe(
        (response) => {
          this.controllerService.invokeSuccessToaster(Utility.UPDATE_SUCCESS_SUMMARY, 'Successfully updated policy.');
          // this.cancelObject();
          this.resetMapsAndSelection();
        },
        (error) => {
          this.controllerService.invokeRESTErrorToaster(Utility.CREATE_FAILED_SUMMARY, error);
          this.resetMapsAndSelection();
        }
      );
    }
  }

  updateObject(newObject: ISecurityNetworkSecurityPolicy, oldObject: ISecurityNetworkSecurityPolicy) {
    return this.securityService.UpdateNetworkSecurityPolicy(oldObject.meta.name, newObject, null, oldObject);
  }

  resetMapsAndSelection() {
    this.newRuleIndex = -1;
    this.ruleEditableMap = {};
    this.clearSelectedDataObjects();
    this.showReorder = false;
    this.reorderToIndex = 0;
  }

  /*
 * the below sections are for the action icons added to the rule tables
 */
  isAnythingSelected() {
    if (this.rulesTable) {
      if (this.rulesTable.selectedDataObjects.length) {
        return true;
      }
    }
    return false;
  }

  isOneSelected() {
    if (this.rulesTable) {
      if (this.rulesTable.selectedDataObjects.length === 1) {
        return true;
      }
    }
    return false;
  }

  checkboxClicked(event) {
    const index = event.data.order;
    this.ruleEditableMap[index] = !this.ruleEditableMap[index];
    // when the number box and reorder icon is hidden
    // need to reset the reorderToIndex otherwise once the
    // box and icon shows again, the reorder icon will be eneabled
    const selectedObjs = this.getSelectedDataObjects();
    if (selectedObjs.length !== 1) {
      this.reorderToIndex = 0;
    }
  }

  onClose() {
    this.display = false;
  }

  setPaginatorDisabled(disable) {
    const $ = Utility.getJQuery();
    const className = 'ui-paginator-element-disabled';
    if (disable) {
      $('.ui-paginator-element').addClass(className);
      $('.ui-paginator p-dropdown > .ui-dropdown').addClass(className);
    } else {
      $('.ui-paginator-element').removeClass(className);
      $('.ui-paginator p-dropdown > .ui-dropdown').removeClass(className);
    }
  }

  formatApp(rule: ISecuritySGRule) {
    let protoPorts = [];
    let apps = [];
    if (rule['apps'] && rule['apps'].length > 0) {
      apps = rule['apps'];
      return 'Applications: ' + apps.join('');
    }
    if (rule['proto-ports'] != null) {
      protoPorts = rule['proto-ports'].map((entry) => {
        if (entry.ports == null || entry.ports.length === 0) {
          return entry.protocol;
        }
        return entry.protocol + '/' + entry.ports;
      });
    }
    return 'Protocol / Ports: ' + protoPorts.join(', ');
  }

  /**
* Get Policy Metrics for src/dest Forwarding Drops
*/
  getPolicyMetrics() {
    const queryList: TelemetryPollingMetricQueries = {
      queries: [],
      tenant: Utility.getInstance().getTenant()
    };
    const query: ITelemetry_queryMetricsQuerySpec = {
      'kind': 'RuleMetrics',
      function: Telemetry_queryMetricsQuerySpec_function.none,
      'sort-order': Telemetry_queryMetricsQuerySpec_sort_order.descending,
      'group-by-field': 'name',
      'start-time': 'now() - 2m' as any,
      'end-time': 'now()' as any,
      fields: [],
    };
    queryList.queries.push({ query: query });
    // We create a new query for each rule
    // We then group each rule by reporter ID
    // We then sum them to generate the total for the rule
    const sub = this.metricsqueryService.pollMetrics('sgpolicyDetail', queryList, this.METRICS_POLLING_INTERVAL).subscribe(
      (data: ITelemetry_queryMetricsQueryResponse) => {
        if (data && data.results) {
          this.ruleMetrics = {};
          this.ruleMetricsTooltip = {};
          data.results.forEach((res) => {
            if (res.series == null) {
              return;
            }
            res.series.forEach((s) => {
              if (s.tags == null) {
                return;
              }
              const ruleHash = s.tags['name'];
              // We go through all the points, and keep the first point
              // for every different naples
              // Since we made the query in descending order, the first point is the most recent
              const uniquePoints = Utility.getLodash().uniqBy(s.values, (val) => {
                const i = MetricsUtility.findFieldIndex(s.columns, 'reporterID');
                return val[i];
              });
              // Sum up all the unique points
              const ruleHits: RuleHitEntry = {
                EspHits: 0,
                IcmpHits: 0,
                OtherHits: 0,
                TcpHits: 0,
                TotalHits: 0,
                UdpHits: 0
              };
              uniquePoints.forEach((point) => {
                this.ruleHitItems.map(item => item.value).forEach((col) => {
                  const index = s.columns.indexOf(col);
                  ruleHits[col] += point[index];
                });
              });
              this.ruleMetrics[ruleHash] = ruleHits;
              this.ruleMetricsTooltip[ruleHash] = this.createRuleTooltip(ruleHits);
            });
          });
        }
      },
    );
    this.subscriptions.push(sub);
  }

  createRuleTooltip(ruleMetric: RuleHitEntry): string {
    const defaultSpacing = 2;
    let retStrs = this.ruleHitItems
      .filter(item => {
        return ruleMetric[item.value] !== 0;
      }).map(item => {
        return item.label + ': ' + ruleMetric[item.value].toString();
      });
    // Right aligning numbers
    let maxLength = 0;
    retStrs.forEach((str) => {
      maxLength = Math.max(maxLength, str.length);
    });
    retStrs = retStrs.map((str) => {
      const items = str.split(' ');
      const count = items[items.length - 1];
      let title = '';
      for (let index = 0; index < items.length - 1; index++) {
        title += items[index];
      }
      return title + ' '.repeat(maxLength - str.length + defaultSpacing) + count;
    });
    return retStrs.join('\n');
  }

  tabChange(event) {
    this.showTable = false;
    const newTab = event.tab.textLabel;
    switch (newTab) {
      case 'TOPOLOGY':
        this.tabShown = TabShown.Topology;
        this.showNetworkGraph = true;
        this.positionNodesTopology();
        this.runTests();
        break;
      case 'POLICIES':
        this.tabShown = TabShown.Policies;
        this.showNetworkGraph = true;
        this.showTable = true;
        this.showTestResults = false;
        this.invokePolicySearch();
        break;
    }
  }

  /**
* Invoked when sourceIP is entered.
*/
  submitSource() {
    if (!IPUtility.isValidIP(this.sourceIP)) {
      this.sourceIP = '';
      this.sourceSubmitted = false;
      return;
    }
    this.sourceSubmitted = true;
    this.positionSourceSet();
    this.troubleshoot();
  }

  /**
   * Invoked when destIP is entered.
   */
  submitDest() {
    if (!IPUtility.isValidIP(this.destIP)) {
      this.destIP = '';
      this.destSubmitted = false;
      return;
    }
    this.showTestResults = false;
    this.destSubmitted = true;
    this.positionDestSet();
    this.troubleshoot();
  }

  /**
   * Invoked when Port is entered.
   */
  submitPort() {
    this.portSubmitted = false;
    if (this.destPort) {
      this.portSubmitted = true;
    }
    this.troubleshoot();
  }

  /**
 * Invoked when troubleshooot button is clicked
 */
  troubleshoot() {
    let url = null;
    this.showTestResults = false;
    this.showTechSupport = false;
    if (!this.tabsTriggered) {
      this.tabsTriggered = true;
      this.positionLoadingTabs();
    }

    if (this.sourceSubmitted && this.destSubmitted) {
      if (this.tabShown === TabShown.Topology ) {
        this.showTabs = true;
        this.positionNodesTopology();
        this.runTests();
    } else if (this.tabShown === TabShown.Policies) {
      this.invokePolicySearch();
    }
    if (this.portSubmitted) {
     // changes the route without moving from the current view or
        // triggering a navigation event,
        url = this._router.createUrlTree([], {
          relativeTo: this._route, queryParams: {
            sourceIP: this.sourceIP,
            destIP: this.destIP,
            port: this.destPort
          }
        }).toString();
    } else {
      // changes the route without moving from the current view or
        // triggering a navigation event,
        url = this._router.createUrlTree([], {
          relativeTo: this._route, queryParams: {
            sourceIP: this.sourceIP,
            destIP: this.destIP
          }
        }).toString();
    }
    this.location.go(url);
  }
  }

  /**
   * FUNCTIONS FOR RULES TABLE
   */

  addOrderRankingForQueries(rules: ISearchPolicyMatchEntry[]): ReadonlyArray<SecuritySGRuleWrapper[]> {
    const retRules = [];
    rules.forEach((rule, index) => {
      retRules.push(
        {
          order: rule.index,
          rule: rule.rule,
          ruleHash: this.ruleHashMap[rule.index],
        }
      );
    });
    return retRules;
  }

  invokePolicySearch() {
    // If there is text entered but it isn't a valid IP, we dont invoke search.
    if (this.sourceIP != null && this.sourceIP.length !== 0 && !IPUtility.isValidIP(this.sourceIP)) {
      return false;
    }
    // If there is text entered but it isn't a valid IP, we dont invoke search.
    if (this.destIP != null && this.destIP.length !== 0 && !IPUtility.isValidIP(this.destIP)) {
      return false;
    }

    // If all fields are blank we retun
    if ((this.destIP == null || this.destIP.length === 0) &&
      (this.sourceIP == null || this.sourceIP.length === 0) &&
      (this.destPort == null || this.destPort.length === 0)) {
      return;
    }

    const req = new SearchPolicySearchRequest();
    req.tenant = Utility.getInstance().getTenant();
    req.namespace = Utility.getInstance().getNamespace();
    if (this.destPort != null && this.destPort.trim().length !== 0) {
      const portInput = this.destPort.trim().split('/');
      if (portInput.length > 1) {
        req.protocol = portInput[0];
        req.port = portInput[1];
      } else {
        req.app = portInput[0];
      }
    }
    if (this.sourceIP != null && this.sourceIP.trim().length !== 0) {
      req['from-ip-address'] = this.sourceIP.trim();
    } else {
      req['from-ip-address'] = 'any';
    }
    if (this.destIP != null && this.destIP.trim().length !== 0) {
      req['to-ip-address'] = this.destIP.trim();
    } else {
      req['to-ip-address'] = 'any';
    }

    if (this.rulesSearchSubscription != null) {
      // There is another call in transit already, we want to ignore
      // the results of that call
      this.rulesSearchSubscription.unsubscribe();
    }
    this.currentSearch = {
      sourceIP: this.sourceIP,
      destIP: this.destIP,
      port: this.destPort
    };
    this.tableLoading = true;
    this.rulesSearchSubscription = this.searchService.PostPolicyQuery(req).subscribe(
      (data) => {
        const body = data.body as ISearchPolicySearchResponse;
        if (body.status === SearchPolicySearchResponse_status.match) {
          if (this.policies[0] == null || body.results[this.policies[0].meta.name] == null) {
            this.dataObjects = [];
          } else {
            const entries = body.results[this.policies[0].meta.name].entries;
            this.dataObjects = this.addOrderRankingForQueries(entries);
            this.dataObjectsBackup = Utility.getLodash().cloneDeepWith(this.dataObjects);
          }
        } else {
          this.dataObjects = [];
        }
        this.tableLoading = false;
        if (this.tabShown === TabShown.Policies) {
          this.positionNodesPolicies();
        }
      },
      (error) => {
        this.tableLoading = false;
        this._controllerService.invokeRESTErrorToaster('Policy search failed', error);
      },
    );

  }

  runTests() {
    this.runSourceTests();
    this.runDestinationTests();
    this.startDSCDropQuery();
    this.getFwlogs();
    if (this.tabShown === TabShown.Topology) {
      this.showTestResults = true;
    }
    if (this.sourceDSC || this.destDSC) {
      this.showTechSupport = true;
    }
  }

  runSourceTests() {
    this.srcTest1Passed = false;
    this.srcTest2Passed = false;
    this.srcTest3Passed = false;

    if (this.sourceWorkload && this.sourceDSC) {

      // TEST1 for Source DSC Health
      this.sourceDSCisAdmitted = this.sourceDSC.spec.admit;
      this.sourceDSCCondition =  Utility.getNaplesCondition(this.sourceDSC); // this.sourceDSC.status.conditions[0].type;
      this.sourceDSCPhase = this.sourceDSC.status['admission-phase'];
      if (this.sourceDSCisAdmitted && this.sourceDSCCondition === 'healthy' && this.sourceDSCPhase === 'admitted') {
        this.srcTest1Passed = true;
      }

      // TEST2 for vCenter Workload
      if (Utility.isWorkloadSystemGenerated(this.sourceWorkload)) {
        this.sourceWorkloadisSystemGenerated = true;
        this.sourceDSCisinFirewallProfile = ObjectsRelationsUtility.isDSCInDSCProfileFeatureSet(this.sourceDSC, this.dscprofiles as ClusterDSCProfile[], ClusterDSCProfileSpec_feature_set.flowaware_firewall);
        this.sourceDSCisVirtualized = ObjectsRelationsUtility.isDSCInDSCProfileDeploymentTarget(this.sourceDSC, this.dscprofiles as ClusterDSCProfile[], ClusterDSCProfileSpec_deployment_target.virtualized);

        if (this.sourceDSCisinFirewallProfile && this.sourceDSCisVirtualized) {
          this.srcTest2Passed = true;
        }
      }

      // TEST3 for DSC Propagation Status
      this.dscprofiles.forEach(profile => {
        if (this.sourceDSC.spec.dscprofile === profile.meta.name) {
          if (profile.status['propagation-status']['generation-id'] !== profile.meta['generation-id']) {
            this.sourceDSCHasPropagatedPolicy = false;
          } else if (!(profile.status['propagation-status'].updated === 0 && profile.status['propagation-status'].pending === 0) && profile.status['propagation-status']['generation-id'] === profile.meta['generation-id'] && profile.status['propagation-status'].pending === 0) {
            this.sourceDSCHasPropagatedPolicy = true;
          }
        }
      });

      if (this.policies[0] && this.policies[0].status['propagation-status']['generation-id'] === this.policies[0].meta['generation-id'] && this.policies[0].status['propagation-status'].pending === 0) {
        this.sourcePolicyPropagatedPolicy = true;
      } else {
        this.sourcePolicyPropagatedPolicy = false;
      }

      if (this.sourcePolicyPropagatedPolicy && this.sourceDSCHasPropagatedPolicy) {
        this.srcTest3Passed = true;
      }
    }
  }
  runDestinationTests() {
    this.destTest1Passed = false;
    this.destTest2Passed = false;
    this.destTest3Passed = false;

    if (this.destWorkload && this.destDSC) {
      // TEST 1 for Dest DSC Health
      this.destDSCisAdmitted = this.destDSC.spec.admit;
      this.destDSCCondition = this.destDSC.status.conditions[0].type;
      this.destDSCPhase = this.destDSC.status['admission-phase'];

      if (this.destDSCisAdmitted && this.destDSCCondition === 'healthy' && this.destDSCPhase === 'admitted') {
        this.destTest1Passed = true;
      }

      // TEST 2  for vCenter Workload
      if (Utility.isWorkloadSystemGenerated(this.destWorkload)) {
        this.destWorkloadisSystemGenerated = true;
        this.destDSCisinFirewallProfile = ObjectsRelationsUtility.isDSCInDSCProfileFeatureSet(this.destDSC, this.dscprofiles as ClusterDSCProfile[], ClusterDSCProfileSpec_feature_set.flowaware_firewall);
        this.destDSCisVirtualized = ObjectsRelationsUtility.isDSCInDSCProfileDeploymentTarget(this.destDSC, this.dscprofiles as ClusterDSCProfile[], ClusterDSCProfileSpec_deployment_target.virtualized);

        if (this.destDSCisinFirewallProfile && this.destDSCisVirtualized) {
          this.destTest2Passed = true;
        }
      }

      // TEST3 for DSC Propagation Status
      this.dscprofiles.forEach(profile => {
        if (this.destDSC.spec.dscprofile === profile.meta.name) {
          if (profile.status['propagation-status']['generation-id'] !== profile.meta['generation-id']) {
            this.destDSCHasPropagatedPolicy = false;
          } else if (!(profile.status['propagation-status'].updated === 0 && profile.status['propagation-status'].pending === 0) && profile.status['propagation-status']['generation-id'] === profile.meta['generation-id'] && profile.status['propagation-status'].pending === 0) {
            this.destDSCHasPropagatedPolicy = true;
          }
        }
      });

      if (this.policies[0] && this.policies[0].status['propagation-status']['generation-id'] === this.policies[0].meta['generation-id'] && this.policies[0].status['propagation-status'].pending === 0) {
        this.destPolicyPropagatedPolicy = true;
      } else {
        this.destPolicyPropagatedPolicy = false;
      }

      if (this.destPolicyPropagatedPolicy && this.destDSCHasPropagatedPolicy) {
        this.destTest3Passed = true;
      }
    }
  }

  /**
   * Functions for Graph Rendering
   */
  getGraphDimensions() {
    const $ = Utility.getJQuery();
    const height = $('#troubleshooting-graph').height();
    const width = $('#troubleshooting-graph').width();
    return { height: height, width: width };
  }

  defaultLayout() {
    const dim = this.getGraphDimensions();
    const height = dim.height;
    const width = dim.width;
    const buffer = this.graphBufferSide + NodeConsts[NodeType.workloadSource].radius;
    this.nodes = [
      { x: buffer, y: height / 3, label: '', secondaryLabel: '', type: NodeType.workloadSource, state: NodeStates.healthy, id: this.sourceNodeID },
      { x: width - (buffer * 3), y: height / 3, label: '', secondaryLabel: '', type: NodeType.workloadDestination, state: NodeStates.error, id: this.destNodeID },
    ];
    this.links = [];
    this.graph.drawGraph(this.nodes, this.links);
  }

  positionLoadingTabs() {
    const sourceNode = this.nodes.filter((n) => {
      return n.id === this.sourceNodeID;
    })[0];
    sourceNode.state = NodeStates.loading;
    const targetNode = this.nodes.filter((n) => {
      return n.id === this.destNodeID;
    })[0];
    targetNode.state = NodeStates.loading;
    this.graph.drawGraph(this.nodes, this.links);
  }

  /* Expects graph to be in defaultLayout setup
   * only updates the source label and the link.
   * This is so that it doesn't matter if the dest node is set first
   */
  positionSourceSet() {
    this.sourceWorkload = null;
    this.sourceDSC = null;
    this.sourceWorkloadFound = false;
    const sourceNode = this.nodes.filter((n) => {
      return n.id === this.sourceNodeID;
    })[0];
    this.sourceWorkload = this.getWorkloadFromIP(this.sourceIP);
    this.getDSCfromWorkload(this.sourceWorkload, true);
    if (this.sourceWorkload) {
      sourceNode.label = this.sourceWorkload.meta.name;
      sourceNode.secondaryLabel = this.sourceIP;
      sourceNode.state = NodeStates.pulsing;
      this.sourceWorkloadFound = true;
    } else {
      sourceNode.label = this.sourceIP;
      sourceNode.secondaryLabel = '';
      sourceNode.state = NodeStates.unknown;
    }

    const links = [
      { sourceID: this.sourceNodeID, targetID: this.destNodeID, directed: false, state: LinkStates.neutral },
    ];
    this.links = links;
    this.graph.drawGraph(this.nodes, this.links);
  }

  /* Expects graph to be in defaultLayout setup
   * only updates the destination label and the link.
   * This is so that it doesn't matter if the dest node is set first
   */
  positionDestSet() {
    this.destWorkload = null;
    this.destDSC = null;
    this.destWorkloadFound = false;
    const dim = this.getGraphDimensions();
    const height = dim.height;
    const width = dim.width;
    const destNode = this.nodes.filter((n) => {
      return n.id === this.destNodeID;
    })[0];
    this.destWorkload = this.getWorkloadFromIP(this.destIP);
    this.getDSCfromWorkload(this.destWorkload, false);
    if (this.destWorkload) {
      destNode.label = this.destWorkload.meta.name;
      destNode.secondaryLabel = this.destIP;
      destNode.state = NodeStates.pulsing;
      this.destWorkloadFound = true;
    } else {
      destNode.label = this.destIP;
      destNode.secondaryLabel = '';
      destNode.state = NodeStates.unknown;
    }
    // If there any links, then we must have set src node already.
    // don't redraw link
    if (this.links.length === 1) {
      this.links[0].animate = false;
    }
    this.graph.drawGraph(this.nodes, this.links);
  }

  /**
* Creates and Renders Graph for Topology Tab
*/
  positionNodesTopology() {
    if (this.tabShown === TabShown.Policies) {
      return;
    }
    const dim = this.getGraphDimensions();
    const width = dim.width;
    const buffer = this.graphBufferSide + NodeConsts[NodeType.workloadSource].radius;

    const sdscX = buffer + NodeConsts[NodeType.workloadSource].radius + 200 + NodeConsts[NodeType.naples].radius;
    const ddscX = width - buffer - NodeConsts[NodeType.workloadSource].radius - 200 - NodeConsts[NodeType.naples].radius;
    const netX = width / 2;
    let startY = this.graphBufferTop;
    startY += NodeConsts[NodeType.network].radius;
    const srcMicrosegVLANX = buffer + (sdscX - buffer) / 2;
    const srcExtVLANX = sdscX + (netX - sdscX) / 2;
    const destMicrosegVLANX = ddscX + (width - buffer - ddscX) / 2;
    const destExtVLANX = netX + (ddscX - netX) / 2;

    let dsctoNet = LinkStates.error, nettoDSC = LinkStates.error;
    let destDSC = '', sourceDSC = '';
    let destDSCState = NodeStates.unknown, sourceDSCState = NodeStates.unknown;
    let links = [];

    let srcMicrosegVLAN = null, srcExtVLAN = null, destMicrosegVLAN = null, destExtVLAN = null;

    const destNode = this.nodes.filter((n) => {
      return n.id === this.destNodeID;
    })[0];

    const sourceNode = this.nodes.filter((n) => {
      return n.id === this.sourceNodeID;
    })[0];

    const nodes = [
      { x: buffer, y: startY, label: sourceNode.label, secondaryLabel: sourceNode.secondaryLabel, type: NodeType.workloadSource, state: sourceNode.state, id: this.sourceNodeID },
      { x: width - buffer, y: startY, label: destNode.label, secondaryLabel: destNode.secondaryLabel, type: NodeType.workloadDestination, state: sourceNode.state, id: this.destNodeID },
      { x: netX, y: startY, label: '', secondaryLabel: '', type: NodeType.network, state: NodeStates.healthy, id: 'net1' },
    ];

    if (this.sourceWorkloadFound) {
      if (this.sourceWorkload !== null) {
        if (this.sourceWorkload.spec.interfaces.length > 0) {
          const interfaceIndex = this.getInterfaceIndexFromWorkload(this.sourceWorkload, this.sourceIP);
          srcMicrosegVLAN = this.sourceWorkload.spec.interfaces[interfaceIndex]['micro-seg-vlan'];
          srcExtVLAN = this.sourceWorkload.spec.interfaces[interfaceIndex]['external-vlan'];
          if (srcExtVLAN === null) {
            srcExtVLAN = this.sourceWorkload.spec.interfaces[interfaceIndex].network;
            nodes.push({ x: srcExtVLANX, y: startY, label: 'Network', secondaryLabel: srcExtVLAN, type: NodeType.empty, state: NodeStates.healthy, id: 'text1' });
          } else {
            nodes.push({ x: srcExtVLANX, y: startY, label: 'External VLAN', secondaryLabel: srcExtVLAN, type: NodeType.empty, state: NodeStates.healthy, id: 'text1' });
          }
        }
        sourceDSC = this.getDSCfromWorkload(this.sourceWorkload, true);
        if (sourceDSC) {
          sourceDSCState = NodeStates.healthy;
          dsctoNet = LinkStates.healthy;
        }
      }
      nodes.push({ x: sdscX, y: startY, label: sourceDSC, secondaryLabel: '', type: NodeType.naples, state: sourceDSCState, id: 'sdsc1' });
      nodes.push({ x: srcMicrosegVLANX, y: startY, label: 'Micro-seg VLAN', secondaryLabel: srcMicrosegVLAN, type: NodeType.empty, state: NodeStates.healthy, id: 'text1' });
    }
    if (this.destWorkloadFound) {
      if (this.destWorkload !== null) {
        if (this.destWorkload.spec.interfaces.length > 0) {
          const interfaceIndex = this.getInterfaceIndexFromWorkload(this.destWorkload, this.destIP);
          destMicrosegVLAN = this.destWorkload.spec.interfaces[interfaceIndex]['micro-seg-vlan'];
          destExtVLAN = this.destWorkload.spec.interfaces[interfaceIndex]['external-vlan'];
          if (destExtVLAN === null) {
            destExtVLAN = this.destWorkload.spec.interfaces[interfaceIndex].network;
            nodes.push({ x: destExtVLANX, y: startY, label: 'Network', secondaryLabel: destExtVLAN, type: NodeType.empty, state: NodeStates.healthy, id: 'text1' });
          } else {
            nodes.push({ x: destExtVLANX, y: startY, label: 'External VLAN', secondaryLabel: destExtVLAN, type: NodeType.empty, state: NodeStates.healthy, id: 'text1' });
          }
        }
        destDSC = this.getDSCfromWorkload(this.destWorkload, false);
        if (destDSC) {
          destDSCState = NodeStates.healthy;
          nettoDSC = LinkStates.healthy;
        }
      }
      nodes.push({ x: ddscX, y: startY, label: destDSC, secondaryLabel: '', type: NodeType.naples, state: destDSCState, id: 'ddsc1' });
      nodes.push({ x: destMicrosegVLANX, y: startY, label: 'Micro-seg VLAN', secondaryLabel: destMicrosegVLAN, type: NodeType.empty, state: NodeStates.healthy, id: 'text1' });
    }

    if (this.sourceWorkloadFound && this.destWorkloadFound) {
      links = [
        { sourceID: this.sourceNodeID, targetID: 'sdsc1', directed: false, state: LinkStates.healthy },
        { sourceID: 'sdsc1', targetID: 'net1', directed: false, state: dsctoNet },
        { sourceID: 'net1', targetID: 'ddsc1', directed: false, state: nettoDSC },
        { sourceID: 'ddsc1', targetID: this.destNodeID, directed: false, state: LinkStates.healthy },
      ];
    } else if (this.sourceWorkloadFound && !this.destWorkloadFound) {
      links = [
        { sourceID: this.sourceNodeID, targetID: 'sdsc1', directed: false, state: LinkStates.healthy },
        { sourceID: 'sdsc1', targetID: 'net1', directed: false, state: dsctoNet },
        { sourceID: 'net1', targetID: this.destNodeID, directed: false, state: LinkStates.healthy }
      ];
    } else if (!this.sourceWorkloadFound && this.destWorkloadFound) {
      links = [
        { sourceID: this.sourceNodeID, targetID: 'net1', directed: false, state: LinkStates.healthy },
        { sourceID: 'net1', targetID: 'ddsc1', directed: false, state: nettoDSC },
        { sourceID: 'ddsc1', targetID: this.destNodeID, directed: false, state: LinkStates.healthy },
      ];
    } else {
      links = [
        { sourceID: this.sourceNodeID, targetID: 'net1', directed: false, state: LinkStates.healthy },
        { sourceID: 'net1', targetID: this.destNodeID, directed: false, state: LinkStates.healthy },
      ];
    }
    this.nodes = Utility.getLodash().cloneDeepWith(nodes);
    this.links = links;
    this.graph.drawGraph(this.nodes, this.links);
  }

  /**
   * Creates and renders graph for Policies Tab
   */
  positionNodesPolicies() {
    if (this.portSubmitted && this.dataObjects.length >= 1) {
      this.positionNodesPolicieswithRules();
    } else {
      const dim = this.getGraphDimensions();
      const width = dim.width;
      const buffer = this.graphBufferSide + NodeConsts[NodeType.workloadSource].radius;
      let startY = this.graphBufferTop;
      startY += NodeConsts[NodeType.network].radius;
      const spX = width / 2;

      let securityPolicyName = '';
      if (this.policies.length > 0) {
        securityPolicyName = this.policies[0].meta.name;
      }

      const destNode = this.nodes.filter((n) => {
        return n.id === this.destNodeID;
      })[0];

      const sourceNode = this.nodes.filter((n) => {
        return n.id === this.sourceNodeID;
      })[0];

      const nodes = [
        { x: buffer, y: startY, label: sourceNode.label, secondaryLabel: sourceNode.secondaryLabel, type: NodeType.workloadSource, state: sourceNode.state, id: this.sourceNodeID },
        { x: width - buffer, y: startY, label: destNode.label, secondaryLabel: destNode.secondaryLabel, type: NodeType.workloadDestination, state: sourceNode.state, id: this.destNodeID },
        { x: spX, y: startY, label: 'Security Policy', secondaryLabel: securityPolicyName, type: NodeType.securityPolicy, state: NodeStates.healthy, id: 'sp1' },
      ];

      const links = [
        // Source to Policy
        { sourceID: this.sourceNodeID, targetID: 'sp1', directed: true, state: LinkStates.healthy },
        // Policy to SG
        { sourceID: 'sp1', targetID: this.destNodeID, directed: true, state: LinkStates.healthy },
      ];

      this.nodes = nodes;
      this.links = links;
      this.graph.drawGraph(this.nodes, this.links);
    }
  }

  positionNodesPolicieswithRules() {
    const dim = this.getGraphDimensions();
    const height = dim.height;
    const width = dim.width;
    const buffer = this.graphBufferSide + NodeConsts[NodeType.workloadSource].radius;
    let startY = this.graphBufferTop;
    startY += NodeConsts[NodeType.network].radius;
    const spX = buffer + NodeConsts[NodeType.workloadSource].radius + 200 + NodeConsts[NodeType.securityPolicy].radius;

    const destNode = this.nodes.filter((n) => {
      return n.id === this.destNodeID;
    })[0];

    const sourceNode = this.nodes.filter((n) => {
      return n.id === this.sourceNodeID;
    })[0];

    const nodes = [
      { x: buffer, y: startY, label: sourceNode.label, secondaryLabel: sourceNode.secondaryLabel, type: NodeType.workloadSource, state: sourceNode.state, id: this.sourceNodeID },
      { x: width - buffer, y: startY, label: destNode.label, secondaryLabel: destNode.secondaryLabel, type: NodeType.workloadDestination, state: sourceNode.state, id: this.destNodeID },
      { x: spX, y: startY, label: 'Security Policy', secondaryLabel: 'GS_Policy_1', type: NodeType.securityPolicy, state: NodeStates.healthy, id: 'sp1' },
    ];

    const links = [
      // Source to Policy
      { sourceID: this.sourceNodeID, targetID: 'sp1', directed: true, state: LinkStates.healthy },
    ];

    const spRadiusTotal = this.dataObjects.length * 2 * NodeConsts[NodeType.rule].radius;
    const nodeSpaceSp = (height - this.graphBufferBottom - this.graphBufferTop - spRadiusTotal) / (this.dataObjects.length);

    for (let i = 0; i < this.dataObjects.length; i++) {
      startY += NodeConsts[NodeType.rule].radius;
      const startX = spX + (width - buffer - spX) / 2;
      const ruleData = this.dataObjects[i];
      let ruleState = NodeStates.healthy;
      let linkState = LinkStates.healthy;
      const action = ruleData.rule.action;

      if (action === 'deny' || action === 'reject') {
        ruleState = NodeStates.error;
        linkState = LinkStates.error;
      }
      if (i === 0) {
        nodes.push({ x: startX, y: startY, label: ' APPLYING RULE: ' + (ruleData.order + 1), secondaryLabel: ruleData.rule.action, type: NodeType.rule, state: ruleState, id: 'rule' + i });
      } else {
        nodes.push({ x: startX, y: startY, label: 'RULE ' + (ruleData.order + 1), secondaryLabel: ruleData.rule.action, type: NodeType.rule, state: ruleState, id: 'rule' + i });
      }
      links.push({ sourceID: 'sp1', targetID: 'rule' + i, directed: false, state: linkState });
      links.push({ sourceID: 'rule' + i, targetID: this.destNodeID, directed: true, state: linkState });
      startY += nodeSpaceSp + NodeConsts[NodeType.securityPolicy].radius;
    }

    this.nodes = nodes;
    this.links = links;
    this.graph.drawGraph(this.nodes, this.links);
  }

  /**
   * Invoked to create and run query for DSC Forwarding Drops
   */
  startDSCDropQuery() {
    const columns = ['DropFlowHit', 'DropHardwareError', 'DropIpNormalization', 'DropParseLenError',
      'DropIpsg', 'DropIcmpNormalization', 'DropInputMapping', 'DropSrcLifMismatch', 'DropNacl',
      'DropIcmpFragPkt', 'DropInputPropertiesMiss', 'DropMultiDestNotPinnedUplink', 'DropIpFragPkt', 'DropFlowMiss'];

    const displayColumns = ['Drop Flow Hit', 'Hardware Errors', 'IP Normalization', 'Packet Length Errors',
      'Drop IPSG', 'ICMP Normalization', 'Input Mapping Table', 'Source LIF Mismatch', 'Drop NACL Hit',
      'ICMP/ICMPv6 Fragment', 'Input Properties Miss', 'Multi-Dest Not Pinned Uplink', 'IP Fragment', 'Flow Miss'];

    this.showSourceDSCDropData = false;
    this.showDestDSCDropData = false;
    this.srcDropData = [];
    this.destDropData = [];
    if (this.sourceWorkload && this.sourceDSC) {
      if (this.sourceDSC.spec.id) {
        const queryList: TelemetryPollingMetricQueries = {
          queries: [],
          tenant: Utility.getInstance().getTenant()
        };

        const query1: MetricsPollingQuery = this.topologyInterfaceQuery(
          'DropMetrics', columns, MetricsUtility.createReporterIDSelector(this.sourceDSC.meta.name));
        queryList.queries.push(query1);

        // refresh every 35 seconds
        const sub = this.metricsqueryService.pollMetrics('topologyInterfaces', queryList, MetricsUtility.THIRTYFIVE_SECONDS).subscribe(
          (data: ITelemetry_queryMetricsQueryResponse) => {
            this.srcDropData = [];
            const hasData = (data && data.results && data.results.length === queryList.queries.length && data.results[0].series[0]);
            if ( hasData) {
              data.results[0].series[0].values[0].forEach((dropvalue, index) => {
                if (dropvalue !== 0 && index !== 0) {
                  this.srcDropData.push({ column: displayColumns[index], value: dropvalue });
                }
              });
              this.showSourceDSCDropData = true;
            }
          },
          (err) => {
            this._controllerService.invokeErrorToaster('Error', 'Failed to load interface metrics.');
          }
        );
        this.subscriptions.push(sub);
      }
    }
    if (this.destWorkload && this.destDSC) {
      if (this.destDSC.spec.id) {
        const queryList: TelemetryPollingMetricQueries = {
          queries: [],
          tenant: Utility.getInstance().getTenant()
        };

        const query1: MetricsPollingQuery = this.topologyInterfaceQuery(
          'DropMetrics', columns,
          MetricsUtility.createReporterIDSelector(this.destDSC.meta.name));
        queryList.queries.push(query1);

        // refresh every 35 seconds
        const sub = this.metricsqueryService.pollMetrics('topologyInterfaces', queryList, MetricsUtility.THIRTYFIVE_SECONDS).subscribe(
          (data: ITelemetry_queryMetricsQueryResponse) => {
            this.destDropData = [];
            const hasData = (data && data.results && data.results.length === queryList.queries.length && data.results[0].series[0]);
            if ( hasData) {
              data.results[0].series[0].values[0].forEach((dropvalue, index) => {
                if (dropvalue !== 0 && index !== 0) {
                  this.destDropData.push({ column: displayColumns[index], value: dropvalue });
                }
              });
              this.showDestDSCDropData = true;
            }
          },
          (err) => {
            this._controllerService.invokeErrorToaster('Error', 'Failed to load interface metrics.');
          }
        );
        this.subscriptions.push(sub);
      }
    }
  }

  topologyInterfaceQuery(kind: string, fields: string[],
    selector: IFieldsSelector = null): MetricsPollingQuery {
    const query: ITelemetry_queryMetricsQuerySpec = {
      'kind': kind,
      'name': null,
      'selector': selector,
      'function': Telemetry_queryMetricsQuerySpec_function.last,
      'group-by-field': 'name',
      'group-by-time': null,
      'fields': fields != null ? fields : [],
      'sort-order': Telemetry_queryMetricsQuerySpec_sort_order.descending,
      'start-time': 'now() - 3m' as any,
      'end-time': 'now()' as any,
    };
    return { query: new Telemetry_queryMetricsQuerySpec(query), pollingOptions: {} };
  }

  /**
 * Invoked to create and send POST Request for latest Firewall Log between sourceIP and destination IP
 */
  getFwlogs() {
    this.showFirewallLogSpecs = false;
    const query = new FwlogFwLogQuery({
      'destination-ips': [this.destIP],
      'source-ips': [this.sourceIP],
      'source-ports': [],
      'destination-ports': [],
      'protocols': [],
      'actions': null,
      'sort-order': FwlogFwLogQuery_sort_order.descending,
      'max-results': 1,
      'tenants': ['default'],
      'reporter-ids': [],
    });
    if (this.fwlogsSearchSubscription) {
      this.fwlogsSearchSubscription.unsubscribe();
    }
    this.fwlogsSearchSubscription = this.fwlogService.PostGetLogs(query).subscribe(  //  use POST
      (resp) => {
        this.controllerService.removeToaster('Fwlog Search Failed');
        const body = resp.body as FwlogFwLogList;
        let logs = null;
        // Code to get data
        if (body.items !== undefined && body.items !== null) {
          logs = body.items[0];
        }
        if (logs != null) {
          this.logAction = logs.action;
          this.logProtocol = logs.protocol;
          this.logDestPort = logs['destination-port'];

          this.logSourcePort = logs['source-port'];
          this.logCreationTime = logs.meta['creation-time'];
          this.showFirewallLogSpecs = true;
        }
      },
      (error) => {
        this.showFirewallLogSpecs = false;
        this.controllerService.invokeRESTErrorToaster('Fwlog Search Failed', error);
      }
    );
    this.subscriptions.push(this.fwlogsSearchSubscription);
  }

  exportJSON() {
    const exportObj = {
      'Source Diagnostic Test Results': {
        'Source IP': this.sourceIP,
        'Source Workload': null,
        'Source DSC': null,
        'Source DSC Health Test Result': {},
        'Source DSC Profile Test Result': {},
        'Source DSC Propagated Policy Test Result': {}
      },
      'Network Connection Test Results': {
        'Last Network Connection Specs': {
          'Time': this.logCreationTime,
          'Protocol': this.logProtocol,
          'Source Port': this.logSourcePort,
          'Destination Port': this.logDestPort,
          'Session Action': this.logAction
        },
        'Source DSC Forwarding Drops': {
          'Time Period': 'Over Past Day',
        },
        'Destination DSC Forwarding Drops': {
          'Time Period': 'Over Past Day',
        }
      },
      'Destination Diagnostic Test Results': {
        'Destination IP': this.destIP,
        'Destination Workload': null,
        'Destination DSC': null,
        'Destination DSC Health Test Result': {},
        'Destination DSC Profile Test Result': {},
        'Destination DSC Propagated Policy Test Result': {}
      }
    };
    if (this.sourceWorkload != null) {
      exportObj['Source Diagnostic Test Results']['Source Workload'] = this.sourceWorkload.meta.name;
    }
    if (this.sourceDSC != null) {
      exportObj['Source Diagnostic Test Results']['Source DSC'] = this.sourceDSC.spec.id;
      exportObj['Source Diagnostic Test Results']['Source DSC Health Test Result']['Source DSC Admitted'] = this.sourceDSCisAdmitted;
      exportObj['Source Diagnostic Test Results']['Source DSC Health Test Result']['Source DSC Condition'] = this.sourceDSCCondition;
      exportObj['Source Diagnostic Test Results']['Source DSC Health Test Result']['Source DSC Phase'] = this.sourceDSCPhase;
      exportObj['Source Diagnostic Test Results']['Source DSC Profile Test Result']['Source DSC in Firewall Profile'] = this.sourceDSCisinFirewallProfile;
      exportObj['Source Diagnostic Test Results']['Source DSC Profile Test Result']['Source DSC is Virtualized'] = this.sourceDSCisVirtualized;
      exportObj['Source Diagnostic Test Results']['Source DSC Propagated Policy Test Result']['Source DSC has Propagated Policy'] = this.sourceDSCHasPropagatedPolicy;
      exportObj['Source Diagnostic Test Results']['Source DSC Propagated Policy Test Result']['Source Policy has been Propagated from Security Policy'] = this.sourcePolicyPropagatedPolicy;
    }
    if (this.destWorkload != null) {
      exportObj['Destination Diagnostic Test Results']['Destination Workload'] = this.destWorkload.meta.name;
    }
    if (this.destDSC != null) {
      exportObj['Destination Diagnostic Test Results']['Destination DSC'] = this.destDSC.spec.id;
      exportObj['Destination Diagnostic Test Results']['Destination DSC Health Test Result']['Destination DSC Admitted'] = this.destDSCisAdmitted;
      exportObj['Destination Diagnostic Test Results']['Destination DSC Health Test Result']['Destination DSC Condition'] = this.destDSCCondition;
      exportObj['Destination Diagnostic Test Results']['Destination DSC Health Test Result']['Destination DSC Phase'] = this.destDSCPhase;
      exportObj['Destination Diagnostic Test Results']['Destination DSC Profile Test Result']['Destination DSC in Firewall Profile'] = this.destDSCisinFirewallProfile;
      exportObj['Destination Diagnostic Test Results']['Destination DSC Profile Test Result']['Destination DSC is Virtualized'] = this.destDSCisVirtualized;
      exportObj['Destination Diagnostic Test Results']['Destination DSC Propagated Policy Test Result']['Destination DSC has Propagated Policy'] = this.destDSCHasPropagatedPolicy;
      exportObj['Destination Diagnostic Test Results']['Destination DSC Propagated Policy Test Result']['Destination Policy has been Propagated from Security Policy'] = this.destPolicyPropagatedPolicy;
    }
    this.srcDropData.forEach((dropData, index) => {
      exportObj['Network Connection Test Results']['Source DSC Forwarding Drops'][dropData.column] = dropData.value;
    });

    this.destDropData.forEach((dropData, index) => {
      exportObj['Network Connection Test Results']['Destination DSC Forwarding Drops'][dropData.column] = dropData.value;
    });
    const fieldName = 'troubleshooting-results.json';
    Utility.exportContent(JSON.stringify(exportObj, null, 2), 'text/json;charset=utf-8;', fieldName);
    Utility.getInstance().getControllerService().invokeInfoToaster('Data exported', 'Please find ' + fieldName + ' in your downloads folder');
  }

  createNewTechSupportRequest() {
    let handler: Observable<any>;
    const currentDate = formatDate(Date.now(), 'yyyy-MM-dd', 'en-US');
    const techSupportName = 'troubleshooting-' + currentDate + '_' + Utility.s4();
    const techsupport = new MonitoringTechSupportRequest({
      'meta': {
        name: techSupportName
      },
      'spec': {
        'node-selector': {
          names: []
        }
      },
      'status': {
        status: 'scheduled'
      }
    });
    if (this.sourceDSC) {
      techsupport.spec['node-selector'].names.push(this.sourceDSC.spec.id);
    }

    if (this.destDSC) {
      techsupport.spec['node-selector'].names.push(this.destDSC.spec.id);
    }

    handler = this.monitoringService.AddTechSupportRequest(techsupport as IMonitoringTechSupportRequest);
    handler.subscribe(
      (response) => {
        this.controllerService.invokeSuccessToaster(Utility.CREATE_SUCCESS_SUMMARY, this.generateCreateSuccessMsg(techsupport));
      },
      (error) => {
        this.controllerService.invokeRESTErrorToaster(Utility.CREATE_FAILED_SUMMARY, error);
      }
    );
  }

  generateCreateSuccessMsg(object: IMonitoringTechSupportRequest) {
    return 'Created tech support request ' + object.meta.name;
  }

}
