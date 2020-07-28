import { Component, OnInit, ViewChild, ViewEncapsulation, OnDestroy, AfterViewInit, ChangeDetectorRef, ChangeDetectionStrategy } from '@angular/core';
import { FormControl, FormGroup } from '@angular/forms';
import { ActivatedRoute } from '@angular/router';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { IPUtility } from '@app/common/IPUtility';
import { Utility } from '@app/common/Utility';
import { SearchUtil } from '@app/components/search/SearchUtil';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { SearchService } from '@app/services/generated/search.service';
import { SecurityService } from '@app/services/generated/security.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { SearchPolicySearchRequest, ISearchPolicyMatchEntry, ISearchPolicySearchResponse, SearchPolicySearchResponse_status } from '@sdk/v1/models/generated/search';
import { ISecuritySGRule, SecuritySGRule, SecurityNetworkSecurityPolicy, SecuritySGRule_action_uihint, ISecurityNetworkSecurityPolicy, SecurityApp } from '@sdk/v1/models/generated/security';
import { CustomFormControl } from '@sdk/v1/utils/validators';
import { TableCol, CustomExportMap, CustomExportFunctionOpts } from '@app/components/shared/tableviewedit';
import { MetricsqueryService, TelemetryPollingMetricQueries } from '@app/services/metricsquery.service';
import { ITelemetry_queryMetricsQuerySpec, Telemetry_queryMetricsQuerySpec_function, Telemetry_queryMetricsQuerySpec_sort_order, FieldsRequirement_operator } from '@sdk/v1/models/generated/telemetry_query';
import { ITelemetry_queryMetricsQueryResponse } from '@sdk/v1/models/telemetry_query';
import { SelectItem } from 'primeng/api';
import { MetricsUtility } from '@app/common/MetricsUtility';
import { ClusterDistributedServiceCard } from '@sdk/v1/models/generated/cluster';
import { ClusterService } from '@app/services/generated/cluster.service';
import { Animations } from '@app/animations';
import { Observable, Subscription } from 'rxjs';
import { IApiStatus } from '@sdk/v1/models/generated/monitoring';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { PenPushTableComponent } from '@app/components/shared/pentable/penpushtable.component';

/**
 * Component for displaying a security policy and providing IP searching
 * on the rules.
 *
 * When there is text entered in any of the search boxes, the
 * search button and the clear search button appear. User can hit enter in
 * any of the boxes to trigger a search or can hit the search button. If they
 * are searching an invalid IP, an error message will show up to the right
 * of the buttons. If the search executes and returns no matches, we display
 * an error message of No Matching Results. Otherwise, the table will scroll
 * to the matching rule and it will be highlighted.
 *
 * If the user starts typing in the search box and changes the query,
 * then the highlighting wil disappear but the user remains at current
 * scroll position. If the user clicks the clear search button,
 * the search will clear and they will be brought back to the top of the table.
 *
 * If new data comes in while we have a search displayed, there are two scenarios
 * 1. User clicks the load new data, and we make a new elastic request and take
 * them to the matching rule in the new data.
 * 2. User makes a new query but haven't clicked load data. We force switch to
 * the new data and then make the query they requested.
 *
 *
 * If a user navigates to a policy that doesn't exist
 * Ex. /security/sgpolicies/fakePolicy
 * they will be shown a Policy does not exist overlay
 * If the policy becomes created while they are on the page
 * it will immediately go away and show the policy
 * If a policy is deleted while a user is looking at it,
 * it will immediately show a policy is deleted overlay.
 * Again, if the policy becomes recreated the overlay will disappear
 * and will show the data.
 *
 *  VS-126 logic
 *  if (sgPolicy.meta.generation-id == sgPolicy.status.propagation-status.generation-id  && sgPolicy.status.propagation-status.pending==0) it is good.
 *   sgPolicy.status.propagation-status.pending + sgPolicy.status.propagation-status.updated === total # of naples
 *
 */
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
  selector: 'app-sgpolicydetail',
  templateUrl: './sgpolicydetail.component.html',
  styleUrls: ['./sgpolicydetail.component.scss'],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush,
  animations: [Animations]
})
export class SgpolicydetailComponent extends DataComponent implements OnInit, OnDestroy {
  @ViewChild('sgPolicyDetailTable') sgPolicyDetailTable: PenPushTableComponent;
  searchPolicyInvoked: boolean = false;  // avoid loop caused by invokeSearchPolicy
  subscriptions = [];
  macToNameMap: { [key: string]: string } = {};
  naples: ReadonlyArray<ClusterDistributedServiceCard> = [];
  naplesEventUtility: HttpEventUtility<ClusterDistributedServiceCard>;
  viewPendingNaples: boolean = false;
  started_get_policy_metrics: boolean = false;

  METRICS_POLLING_INTERVAL = 60000;

  cols: TableCol[] = [
    { field: 'ruleNum', header: 'Number', class: 'sgpolicy-rule-number', width: 4 },
    { field: 'sourceIPs', header: 'Source IPs', class: 'sgpolicy-source-ip', width: 22 },
    { field: 'destIPs', header: 'Destination IPs', class: 'sgpolicy-dest-ip', width: 22 },
    { field: 'action', header: 'Action', class: 'sgpolicy-action', width: 8 },
    { field: 'protocolPort', header: 'Protocol / Application', class: 'sgpolicy-port', width: 20 },
    { field: 'TotalHits', header: 'Total Connection Hits', class: 'sgpolicy-rule-stat', width: 10 },
  ];

  ruleHitItems: SelectItem[] = [
    { label: 'TCP Hits', value: 'TcpHits' },
    { label: 'UDP Hits', value: 'UdpHits' },
    { label: 'ICMP Hits', value: 'IcmpHits' },
    { label: 'ESP Hits', value: 'EspHits' },
    { label: 'Other Hits', value: 'OtherHits' },
    { label: 'Total Hits', value: 'TotalHits' },
  ];

  // Used for the table - when true there is a loading icon displayed
  loading: boolean = false;

  bodyicon: any = {
    margin: {
      top: '9px',
      left: '8px'
    },
    url: '/assets/images/icons/security/icon-security-policy-black.svg'
  };

  policyIcon: Icon = {
    margin: {
      top: '-18px',
      left: '0px',
    },
    svgIcon: 'policy'
  };

  // Id of the policy the user has navigated to
  selectedPolicyId: string;

  // Current policy that is being displayed
  selectedPolicy: SecurityNetworkSecurityPolicy;

  // TODO: Update with actual creator
  creator = 'pensando';


  sourceIpFormControl: FormControl = CustomFormControl(new FormControl('', [IPUtility.isValidIPValidator
  ]), { description: 'Value must be in dot notation' });

  destIpFormControl: FormControl = CustomFormControl(new FormControl('', [IPUtility.isValidIPValidator
  ]), { description: 'Value must be in dot notation' });

  portFormControl: FormControl = CustomFormControl(new FormControl('', [
  ]), { description: 'Value should be either <protocol>/<port> or app name' });

  securityApps: ReadonlyArray<SecurityApp> = [];
  securityAppOptions: SelectItem[] = [];

  // Holds all policy objects
  sgPolicies: ReadonlyArray<SecurityNetworkSecurityPolicy>;
  sgPoliciesEventUtility: HttpEventUtility<SecurityNetworkSecurityPolicy>;

  // Holds all the policy rules of the currently selected policy
  dataObjects: ReadonlyArray<any>;

  // Current filter applied to all the data in the table
  currentSearch;

  // Index of the rule to highlight
  selectedRuleIndex: number;

  // Error message to display next to IP search
  searchErrorMessage: string;

  // subscription for elastic search queries
  searchSubscription;

  routerSubscription: Subscription;

  // Whether we show a deletion overlay
  showDeletionScreen: boolean;

  // Whether we show a missing overlay
  showMissingScreen: boolean;

  ruleCount: number = 0;

  // map from rule index to rule hash
  ruleHashMap: { [index: number]: string } = {};
  // Map from rule has to aggregate hits
  ruleMetrics: { [hash: string]: RuleHitEntry } = {};
  // Map from rule hash to tooltip string
  // Helps avoid rebuidling the string unless there are changes
  ruleMetricsTooltip: { [hash: string]: string } = {};

  // EDIT MODE
  ruleEditableMap: { [index: number]: boolean } = {};
  editObject: SecurityNetworkSecurityPolicy = null;
  newRuleIndex: number = -1;

  // properties from tableview component
  disableTableWhenRowExpanded: boolean = true;
  isTabComponent: boolean = false;

  exportFilename: string = 'PSM-sgpolicy';

  /** Define exportMap for table export.  Basicially, it is following how displayColumn() **/
  exportMap: CustomExportMap = {
    'ruleNum': (opts: CustomExportFunctionOpts) => {
      return opts.data.order + 1;
    },
    'sourceIPs': (opts: CustomExportFunctionOpts) => {
      return opts.data.rule['from-ip-addresses'].join(', ');
    },
    'destIPs': (opts: CustomExportFunctionOpts) => {
      return opts.data.rule['to-ip-addresses'].join(', ');
    },
    'action': (opts: CustomExportFunctionOpts) => {
      return SecuritySGRule_action_uihint[opts.data.rule.action];
    },
    'protocolPort': (opts: CustomExportFunctionOpts) => {
      return this.formatApp(opts.data.rule);
    },
    'TotalHits': (opts: CustomExportFunctionOpts) => {
      const entry = this.ruleMetrics[opts.data.ruleHash];
      if (entry == null) {
        return '';
      }
      return String(entry.TotalHits); // to return string
    }
  };

  display: boolean = false;
  showReorder: boolean = false;
  reorderToIndex: number = 0;
  shouldEnableButtons: boolean;


  constructor(protected _controllerService: ControllerService,
    protected securityService: SecurityService,
    protected searchService: SearchService,
    private _route: ActivatedRoute,
    private clusterService: ClusterService,
    protected uiconfigsService: UIConfigsService,
    protected cdr: ChangeDetectorRef,
    protected metricsqueryService: MetricsqueryService,
  ) {
    super(_controllerService, uiconfigsService);
  }

  ngOnInit() {
    super.ngOnInit();
    this.routerSubscription = this._route.paramMap.subscribe(params => {
      const id = params.get('id');
      this.selectedPolicyId = id;
      this.initializeData();
      this.getSGPoliciesDetail();
    });
  }

  /**
   * Set toolbar
   * @param id
   */
  private setToolbar(id: string) {
    let buttons = [];
    if (this.uiconfigsService.isAuthorized(UIRolePermissions.networklbpolicy_update) && this.dataObjects.length === 0) {
      // Where there is no rule in this policy, we add a toolbar button to let user create a rule.
      buttons = [{
        cssClass: 'global-button-primary sgpolicy-toolbar-button sgpolicy-toolbar-button-ADD',
        text: 'ADD RULE',
        callback: () => { this.onCreate(); }
      }];
    }
    this._controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [
        { label: 'Network Security Policies', url: Utility.getBaseUIUrl() + 'security/sgpolicies' },
        { label: id, url: Utility.getBaseUIUrl() + 'security/sgpolicies/' + id }
      ]
    });
  }

  // Blank hook to satisfy inheritance.
  // Any toolbar buttons should be added to the toolbar setting in postNgInit route subscription
  setDefaultToolbar() { }

  initializeData() {
    // Initializing variables so that state is cleared between routing of different
    // sgpolicies
    // Ex. /sgpolicies/policy1 -> /sgpolicies/policy2
    this.subscriptions.forEach(sub => {
      sub.unsubscribe();
    });
    this.subscriptions = [];
    this.sgPolicies = [];
    this.dataObjects = [];
    this.ruleCount = 0;
    this.showDeletionScreen = false;
    this.showMissingScreen = false;
    this.viewPendingNaples = false;
    this.searchErrorMessage = '';
    this.selectedRuleIndex = null;
    this.currentSearch = null;
    this.selectedPolicy = null;
    this.sourceIpFormControl.setValue('');
    this.destIpFormControl.setValue('');
    this.portFormControl.setValue('');
    this.enableFormControls();
    this.getNaples();
    this.getSecurityApps();
    this.penTable = this.sgPolicyDetailTable;
  }

  getSecurityApps() {
    const sub = this.securityService.ListAppCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        this.securityApps = response.data;
        this.securityAppOptions = this.securityApps.map(item => {
          return {
            label: item.meta.name,
            value: item.meta.name
          };
        });
        this.cdr.detectChanges();
      },
      this.controllerService.webSocketErrorHandler('Failed to get apps')
    );
    this.subscriptions.push(sub);
  }


  getNaples() {
    const dscSubscription = this.clusterService.ListDistributedServiceCardCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.naples = response.data as ClusterDistributedServiceCard[]; // mac-address to Name map
        this.macToNameMap = {};
        for (const smartnic of this.naples) {
          if (smartnic.spec.id != null && smartnic.spec.id !== '') {
            this.macToNameMap[smartnic.meta.name] = smartnic.spec.id;
          }
        }
        this.cdr.detectChanges();
      },
      (error) => {
        this._controllerService.invokeRESTErrorToaster('Error', 'Failed to get DSCs info');
      }
    );
    this.subscriptions.push(dscSubscription);
  }


  getNaplesName(mac: string): string {
    return this.macToNameMap[mac];
  }

  viewPendingNaplesList() {
    this.viewPendingNaples = !this.viewPendingNaples;
  }

  disableFormControls() {
    // Setting values to blank so that the hint text shows up instead of
    // just a blank field
    this.sourceIpFormControl.setValue('');
    this.destIpFormControl.setValue('');
    this.portFormControl.setValue('');
    this.sourceIpFormControl.disable();
    this.destIpFormControl.disable();
    this.portFormControl.disable();
    this.cdr.detectChanges();
  }

  enableFormControls() {
    this.sourceIpFormControl.enable();
    this.destIpFormControl.enable();
    this.portFormControl.enable();
    this.cdr.detectChanges();
  }

  keyUpInput(event) {
    // if all search input is empty, we reset the table
    // If a user hits the enter key on an empty search, we shouldn't try to execute a policy search.
    let sourceIP = this.sourceIpFormControl.value;
    let destIP = this.destIpFormControl.value;
    let port = this.portFormControl.value;
    if (sourceIP === '' && destIP === '' && port === '') {
      this.searchErrorMessage = '';
      this.updateRulesByPolicy();
      this.currentSearch = null;
      this.searchPolicyInvoked = false;
      return;
    }

    if (event.keyCode === SearchUtil.EVENT_KEY_ENTER) {
      this.invokePolicySearch();
      this.cdr.detectChanges();
    } else if (this.currentSearch != null) {
      // If the keystroke changed the search fields
      // to be different than the current search
      // we clear the current search and selected index,
      // but we don't scroll to the top like we do on clearSearch
      sourceIP = this.sourceIpFormControl.value;
      destIP = this.destIpFormControl.value;
      port = this.portFormControl.value;
      if (sourceIP !== this.currentSearch.sourceIP ||
        destIP !== this.currentSearch.destIP ||
        port !== this.currentSearch.port) {
        this.searchErrorMessage = '';
        this.selectedRuleIndex = null;
        this.currentSearch.sourceIP = null;
        this.currentSearch.destIP = null;
        this.currentSearch.port = null;
      }
    } else {
      this.searchErrorMessage = '';
    }
  }

  /**
   * Hooked onto the output emitter of lazyRender component
   * for when the data changes
   */
  dataUpdated() {
    this.ruleCount = this.dataObjects.length;
    if (this.searchPolicyInvoked) {
      this.searchPolicyInvoked = false;
      this.cdr.detectChanges();
      return;
    }
    if (this.currentSearch != null) {
      this.invokePolicySearch(this.currentSearch.sourceIP, this.currentSearch.destIP, this.currentSearch.port);
      this.cdr.detectChanges();
    }
  }

  /**
   * Called by HTML to decide whether to show the search button
   */
  showSearchButton(): boolean {
    const sourceIP = this.sourceIpFormControl.value;
    const destIP = this.destIpFormControl.value;
    const port = this.portFormControl.value;
    return (sourceIP != null && sourceIP.length > 0) ||
      (destIP != null && destIP.length > 0) ||
      (port != null && port.length > 0);
  }

  clearSearch() {
    this.sourceIpFormControl.setValue('');
    this.destIpFormControl.setValue('');
    this.portFormControl.setValue('');
    this.searchErrorMessage = '';
    if (this.selectedRuleIndex != null) {
      this.selectedRuleIndex = null;
    }
    this.updateRulesByPolicy();
    this.currentSearch = null;
    this.searchPolicyInvoked = false;
    this.cdr.detectChanges();
  }

  invokePolicySearch(sourceIP = null, destIP = null, port: string = null) {
    // Read values from form control if not provided
    if (sourceIP == null && destIP == null && port == null) {
      sourceIP = this.sourceIpFormControl.value;
      destIP = this.destIpFormControl.value;
      port = this.portFormControl.value;
    }
    // If there is text entered but it isn't a valid IP, we dont invoke search.
    if (sourceIP != null && sourceIP.length !== 0 && !IPUtility.isValidIP(sourceIP)) {
      this.searchErrorMessage = 'Invalid IP';
      return false;
    }
    // If there is text entered but it isn't a valid IP, we dont invoke search.
    if (destIP != null && destIP.length !== 0 && !IPUtility.isValidIP(destIP)) {
      this.searchErrorMessage = 'Invalid IP';
      return false;
    }

    // If all fields are blank we retun
    if ((destIP == null || destIP.length === 0) &&
      (sourceIP == null || sourceIP.length === 0) &&
      (port == null || port.length === 0)) {
      return;
    }

    this.searchErrorMessage = '';

    const req = new SearchPolicySearchRequest();
    req.tenant = Utility.getInstance().getTenant();
    req.namespace = Utility.getInstance().getNamespace();
    req['sg-policy'] = this.selectedPolicyId;
    if (port != null && port.trim().length !== 0) {
      const portInput = port.trim().split('/');
      if (portInput.length > 1) {
        req.protocol = portInput[0];
        req.port = portInput[1];
      } else {
        req.app = portInput[0];
      }
    }
    if (sourceIP != null && sourceIP.trim().length !== 0) {
      req['from-ip-address'] = sourceIP.trim();
    } else {
      req['from-ip-address'] = 'any';
    }
    if (destIP != null && destIP.trim().length !== 0) {
      req['to-ip-address'] = destIP.trim();
    } else {
      req['to-ip-address'] = 'any';
    }


    if (this.searchSubscription != null) {
      // There is another call in transit already, we want to ignore
      // the results of that call
      this.searchSubscription.unsubscribe();
    }
    this.currentSearch = {
      sourceIP: sourceIP,
      destIP: destIP,
      port: port
    };
    this.loading = true;
    this.cdr.detectChanges();

    this.searchSubscription = this.searchService.PostPolicyQuery(req).subscribe(
      (data) => {
        const body = data.body as ISearchPolicySearchResponse;
        if (body.status === SearchPolicySearchResponse_status.match) {
          if (this.selectedPolicy == null || body.results[this.selectedPolicy.meta.name] == null) {
            this.searchErrorMessage = 'No Matching Rule';
            this.searchPolicyInvoked = true;
            this.dataObjects = [];
          } else {
            this.searchErrorMessage = '';
            const entries = body.results[this.selectedPolicy.meta.name].entries;
            this.searchPolicyInvoked = true;
            this.dataObjects = this.addOrderRankingForQueries(entries);
          }
        } else {
          this.searchErrorMessage = 'No Matching Rule';
          this.searchPolicyInvoked = true;
          this.dataObjects = [];
        }
        this.loading = false;
        this.cdr.detectChanges();
      },
      (error) => {
        this.loading = false;
        this._controllerService.invokeRESTErrorToaster('Policy search failed', error);
        this.cdr.detectChanges();
      },
    );

  }


  /**
   * Component is about to exit
   */
  ngOnDestroy() {
    if (this.searchSubscription != null) {
      this.searchSubscription.unsubscribe();
    }
    if (this.routerSubscription != null) {
      this.routerSubscription.unsubscribe();
    }
    super.ngOnDestroy();
  }


  updateRulesByPolicy() {
    this.dataObjects = this.addOrderRanking(this.selectedPolicy.spec.rules);
  }

  getSGPoliciesDetail() {
    this.loading = true;
    this.cdr.detectChanges();
    // We perform a get as well as a watch so that we can know if the object the user is
    // looking for exists or not.
    const getSubscription = this.securityService.GetNetworkSecurityPolicy(this.selectedPolicyId).subscribe(
      response => {
        // We do nothing, and wait for the callback of the watch to populate the view
      },
      error => {
        // If we receive any error code we display object is missing
        // TODO: Update to be more descriptive based on error message
        this.showMissingScreen = true;
        // Putting focus onto the overlay to prevent user
        // being able to interact with the page underneath
        this.disableFormControls();
      },
      () => {
        this.loading = false;
        this.cdr.detectChanges();
      }
    );
    this.subscriptions.push(getSubscription);
    this.sgPoliciesEventUtility = new HttpEventUtility<SecurityNetworkSecurityPolicy>(SecurityNetworkSecurityPolicy);
    this.sgPolicies = this.sgPoliciesEventUtility.array;
    const subscription = this.securityService.WatchNetworkSecurityPolicy({ 'field-selector': 'meta.name=' + this.selectedPolicyId }).subscribe(
      response => {
        if (this.searchSubscription != null) { this.searchSubscription.unsubscribe(); }  // avoid racing condition for searchSubscription and WatchNetworkSecurityPolicy
        this.sgPoliciesEventUtility.processEvents(response);
        if (this.sgPolicies.length > 1) {
          // because of the name selector, we should
          // have only got one object
          console.error(
            'Received more than one security policy. Expected '
            + this.selectedPolicyId + ', received ' +
            this.sgPolicies.map((policy) => policy.meta.name).join(', '));
        }
        if (this.sgPolicies.length > 0) {
          // In case object was deleted and then readded while we are on the same screen
          this.showDeletionScreen = false;
          // In case object wasn't created yet and then was added while we are on the same screen
          this.showMissingScreen = false;
          this.enableFormControls();
          // Set sgpolicyrules
          this.selectedPolicy = this.sgPolicies[0];
          this.searchPolicyInvoked = false;
          this.generateRuleMap();
          if (!this.started_get_policy_metrics) { // only start polling metrics once
            this.started_get_policy_metrics = true;
            this.getPolicyMetrics();
          }
          this.updateRulesByPolicy();
          this.exportFilename = this.selectedPolicy.meta.name;
          this.resetMapsAndSelection();
        } else {
          // Must have received a delete event.
          this.showDeletionScreen = true;
          // Putting focus onto the overlay to prevent user
          // being able to interact with the page underneath
          this.disableFormControls();

          this.selectedPolicy = null;
          this.dataObjects = [];
        }
        // Once receive server update, we update the toolbar.
        this.setToolbar(this.selectedPolicyId);
      },
      this._controllerService.webSocketErrorHandler('Failed to get SG Policy'),
      () => {
        this.loading = false;
        this.cdr.detectChanges();
      }
    );
    this.subscriptions.push(subscription);
  }

  generateRuleMap() {
    this.ruleHashMap = {};
    if (this.selectedPolicy == null) {
      return;
    }
    this.selectedPolicy.status['rule-status'].forEach((rule, index) => {
      this.ruleHashMap[index] = rule['rule-hash'];
    });
  }

  getPolicyMetrics() {
    if (this.selectedPolicy == null || this.selectedPolicy.status['rule-status'].length === 0) {
      return;
    }
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
          this.cdr.detectChanges();
        }
      },
    );
    this.subscriptions.push(sub);
  }

  /**
   * Adds a wrapper object around the rules to store ordering
   * so that they can easily be uniquely identified
   * @param rules
   */
  addOrderRanking(rules: ISecuritySGRule[]) {
    const retRules = [];
    rules.forEach((rule, index) => {
      retRules.push(
        {
          order: index,
          rule: rule,
          ruleHash: this.ruleHashMap[index],
        }
      );
    });
    return retRules;
  }

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

  displayColumn(rowData: SecuritySGRuleWrapper, col: TableCol): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(rowData, fields);
    const column = col.field;
    switch (column) {
      case 'ruleNum':
        return rowData.order + 1;
      case 'protocolPort':
        return this.formatApp(rowData.rule);
      case 'sourceIPs':
        return rowData.rule['from-ip-addresses'].join(', ');
      case 'destIPs':
        return rowData.rule['to-ip-addresses'].join(', ');
      case 'action':
        return SecuritySGRule_action_uihint[rowData.rule.action];
      case 'TotalHits':
        const entry = this.ruleMetrics[rowData.ruleHash];
        if (entry == null) {
          return '';
        }
        return entry.TotalHits;
    }
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

  /*
   * the below sections are for the action icons added to the rule tables
   */
  isAnythingSelected() {
    if (this.sgPolicyDetailTable) {
      if (this.getSelectedDataObjects().length) {
        return true;
      }
    }
    return false;
  }

  isOneSelected() {
    if (this.sgPolicyDetailTable) {
      if (this.getSelectedDataObjects().length === 1) {
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
    this.cdr.detectChanges();
  }

  resetMapsAndSelection() {
    this.newRuleIndex = -1;
    this.ruleEditableMap = {};
    this.clearSelectedDataObjects();
    this.showReorder = false;
    this.reorderToIndex = 0;
    this.cdr.detectChanges();
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
    this.cdr.detectChanges();
  }

  isReorderReady() {
    return this.reorderToIndex > 0;
  }

  onClose() {
    this.display = false;
    this.cdr.detectChanges();
  }

  onUpdate() {
    const editrules: Array<SecuritySGRule> = [];
    this.editObject = new SecurityNetworkSecurityPolicy(this.selectedPolicy);
    this.getSelectedDataObjects().forEach((ruleObj, index) => {
      editrules.push(ruleObj.rule);
    });
    this.editObject.spec.rules = editrules;
    this.display = true;
    this.cdr.detectChanges();
  }

  onAdd(rowData, isBefore: boolean = false) {
    this.newRuleIndex = isBefore ? rowData.order : (rowData.order + 1);
    this.onRuleAddCreateHelper();
  }

  private onRuleAddCreateHelper() {
    this.editObject = new SecurityNetworkSecurityPolicy(this.selectedPolicy);
    this.editObject.spec.rules = [];
    this.display = true;
    this.cdr.detectChanges();
  }

  onCreate() {
    this.newRuleIndex = 0;
    this.onRuleAddCreateHelper();
  }

  onReorder() {
    const selectedObjs = this.getSelectedDataObjects();
    if (selectedObjs.length === 1 && this.reorderToIndex > 0) {
      const policy1 = this.selectedPolicy.getFormGroupValues();
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
    this.cdr.detectChanges();
  }

  onDelete() {
    const selectedObjs = this.getSelectedDataObjects();
    if (selectedObjs && selectedObjs.length > 0) {
      const length = selectedObjs.length;
      const msg = `Are you sure you want to delete ${length > 1 ? 'these' : 'this'} \
        SG Policy ${length > 1 ? 'Rules' : 'Rule'}?`;
      this.controllerService.invokeConfirm({
        header: msg,
        message: 'This action cannot be reversed',
        acceptLabel: 'Delete',
        accept: () => {
          const selectedIdxMap = {};
          selectedObjs.forEach(item => {
            selectedIdxMap[item.order] = true;
          });
          const policy1 = this.selectedPolicy.getFormGroupValues();
          const rules = policy1.spec.rules.filter((rule, idx) => !selectedIdxMap[idx]);
          policy1.spec.rules = rules;
          this.updatePolicy(policy1);
        }
      });
    }
  }

  onSave(editedObject: SecurityNetworkSecurityPolicy) {
    if (editedObject) {
      const policy = (this.newRuleIndex > -1) ?
        this.updateFromNewRule(editedObject, this.newRuleIndex) :
        this.updateFromEditObject(editedObject);
      this.updatePolicy(policy);
    }
  }

  updateFromNewRule(editedObject, index: number) {
    const policy = editedObject;
    const policy1 = Utility.getLodash().cloneDeep(this.selectedPolicy.getFormGroupValues());
    policy1.spec.rules.splice(index, 0, policy.spec.rules[0]);
    return policy1;
  }

  updateFromEditObject(editedObject) {
    const policy = editedObject;
    const policy1 = this.selectedPolicy.getFormGroupValues();
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
      (<any>policy).meta.name = (<any>this.selectedPolicy).meta.name;
      handler = this.updateObject(policy, this.selectedPolicy);

      handler.subscribe(
        (response) => {
          this.controllerService.invokeSuccessToaster(Utility.UPDATE_SUCCESS_SUMMARY, 'Successfully updated policy.');
          // this.cancelObject();
          this.resetMapsAndSelection();
          this.cdr.detectChanges();
        },
        (error) => {
          this.controllerService.invokeRESTErrorToaster(Utility.CREATE_FAILED_SUMMARY, error);
          this.resetMapsAndSelection();
          this.cdr.detectChanges();
        }
      );
    }
  }

  updateObject(newObject: ISecurityNetworkSecurityPolicy, oldObject: ISecurityNetworkSecurityPolicy) {
    return this.securityService.UpdateNetworkSecurityPolicy(oldObject.meta.name, newObject, null, oldObject);
  }
}
