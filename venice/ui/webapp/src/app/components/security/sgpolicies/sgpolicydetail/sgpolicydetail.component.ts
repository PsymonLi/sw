import { Component, OnInit, ViewChild, ViewEncapsulation, OnDestroy, AfterViewInit } from '@angular/core';
import { FormControl } from '@angular/forms';
import { ActivatedRoute } from '@angular/router';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { IPUtility } from '@app/common/IPUtility';
import { Utility } from '@app/common/Utility';
import { BaseComponent } from '@app/components/base/base.component';
import { SearchUtil } from '@app/components/search/SearchUtil';
import { LazyrenderComponent } from '@app/components/shared/lazyrender/lazyrender.component';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { SearchService } from '@app/services/generated/search.service';
import { SecurityService } from '@app/services/generated/security.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { SearchPolicySearchRequest, ISearchPolicyMatchEntry } from '@sdk/v1/models/generated/search';
import { ISecuritySGRule, SecuritySGPolicy, SecuritySGRule_action_uihint } from '@sdk/v1/models/generated/security';
import { Table } from 'primeng/table';
import { TableCol } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { CustomFormControl } from '@sdk/v1/utils/validators';

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
}

@Component({
  selector: 'app-sgpolicydetail',
  templateUrl: './sgpolicydetail.component.html',
  styleUrls: ['./sgpolicydetail.component.scss'],
  encapsulation: ViewEncapsulation.None
})
export class SgpolicydetailComponent extends BaseComponent implements OnInit, OnDestroy, AfterViewInit {
  @ViewChild('sgpolicyTable') sgpolicyTurboTable: Table;
  @ViewChild(LazyrenderComponent) lazyRenderWrapper: LazyrenderComponent;
  viewInitComplete: boolean = false;
  searchPolicyInvoked: boolean = false;  // avoid loop caused by invokeSearchPolicy
  subscriptions = [];

  cols: TableCol[] = [
    { field: 'ruleNum', header: '', class: 'sgpolicy-rule-number', width: 4 },
    { field: 'sourceIPs', header: 'Source IPs', class: 'sgpolicy-source-ip', width: 24 },
    { field: 'destIPs', header: 'Destination IPs', class: 'sgpolicy-dest-ip', width: 24 },
    { field: 'action', header: 'Action', class: 'sgpolicy-action', width: 24 },
    { field: 'protocolPort', header: 'Protocol/Ports', class: 'sgpolicy-port', width: 24 },
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
      top: '0px',
      left: '0px',
    },
    svgIcon: 'policy'
  };

  // Id of the policy the user has navigated to
  selectedPolicyId: string;

  // Current policy that is being displayed
  selectedPolicy: SecuritySGPolicy;

  // TODO: Update with actual creator
  creator = 'pensando';


  sourceIpFormControl: FormControl = CustomFormControl(new FormControl('', [IPUtility.isValidIPValidator
  ]), { description: 'Value must be in dot notation' });

  destIpFormControl: FormControl = CustomFormControl(new FormControl('', [IPUtility.isValidIPValidator
  ]), { description: 'Value must be in dot notation' });

  portFormControl: FormControl = CustomFormControl(new FormControl('', [
  ]), { description: 'Value should be either <protocol>/<port> or app name' });

  // Holds all policy objects
  sgPolicies: ReadonlyArray<SecuritySGPolicy>;
  sgPoliciesEventUtility: HttpEventUtility<SecuritySGPolicy>;

  // Holds all the policy rules of the currently selected policy
  sgPolicyRules: ReadonlyArray<SecuritySGRuleWrapper[]>;

  // Current filter applied to all the data in the table
  currentSearch;

  // Index of the rule to highlight
  selectedRuleIndex: number;

  // Error message to display next to IP search
  searchErrorMessage: string;

  // subscription for elastic search queries
  searchSubscription;

  // Whether we show a deletion overlay
  showDeletionScreen: boolean;

  // Whether we show a missing overlay
  showMissingScreen: boolean;

  ruleCount: number = 0;

  constructor(protected _controllerService: ControllerService,
    protected securityService: SecurityService,
    protected searchService: SearchService,
    private _route: ActivatedRoute,
    protected uiconfigsService: UIConfigsService,
  ) {
    super(_controllerService, uiconfigsService);
  }


  ngOnInit() {
    this.initializeData();

    this._controllerService.publish(Eventtypes.COMPONENT_INIT, { 'component': 'SgpolicydetailComponent', 'state': Eventtypes.COMPONENT_INIT });
    this._route.paramMap.subscribe(params => {
      const id = params.get('id');
      this.selectedPolicyId = id;
      this.initializeData();
      this.getSGPoliciesDetail();
      this._controllerService.setToolbarData({
        buttons: [],
        breadcrumb: [
          { label: 'Security Group Policies', url: Utility.getBaseUIUrl() + 'security/sgpolicies' },
          { label: id, url: Utility.getBaseUIUrl() + 'security/sgpolicies/' + id }]
      });
    });

  }

  initializeData() {
    // Initializing variables so that state is cleared between routing of different
    // sgpolicies
    // Ex. /sgpolicies/policy1 -> /sgpolicies/policy2
    this.subscriptions.forEach(sub => {
      sub.unsubscribe();
    });
    this.subscriptions = [];
    this.sgPolicies = [];
    this.sgPolicyRules = [];
    this.ruleCount = 0;
    this.showDeletionScreen = false;
    this.showMissingScreen = false;
    this.searchErrorMessage = '';
    this.selectedRuleIndex = null;
    this.currentSearch = null;
    this.selectedPolicy = null;
    this.sourceIpFormControl.setValue('');
    this.destIpFormControl.setValue('');
    this.portFormControl.setValue('');
    this.enableFormControls();
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
  }

  enableFormControls() {
    this.sourceIpFormControl.enable();
    this.destIpFormControl.enable();
    this.portFormControl.enable();
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
      return;
    }

    if (event.keyCode === SearchUtil.EVENT_KEY_ENTER) {
      this.invokePolicySearch();
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
    this.ruleCount = this.sgPolicyRules.length;
    if (this.searchPolicyInvoked) {
      this.searchPolicyInvoked = false;
      return;
    }
    if (this.currentSearch != null) {
      this.invokePolicySearch(this.currentSearch.sourceIP, this.currentSearch.destIP, this.currentSearch.port);
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
    this.errorMessage = '';
    if (this.selectedRuleIndex != null) {
      this.selectedRuleIndex = null;
      // scroll back to top
      this.lazyRenderWrapper.resetTableView();
    }
    this.updateRulesByPolicy();
    this.currentSearch = null;
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
    // If we are displaying old data, we force update to new data
    if (this.lazyRenderWrapper.hasUpdate) {
      // Current search is set to be the new data
      // When on data update fires from resetting the table view,
      // We will call this function again with the provided search.
      this.lazyRenderWrapper.resetTableView();
      return;
    }
    this.searchSubscription = this.searchService.PostPolicyQuery(req).subscribe(
      (data) => {
        // const body = data.body as ISearchPolicySearchResponse;
        const body = data.body as any;
        if (body.status === 'MATCH') {
          if (this.selectedPolicy == null || body.results[this.selectedPolicy.meta.name] == null) {
            this.searchErrorMessage = 'No Matching Rule';
            this.searchPolicyInvoked = true;
            this.sgPolicyRules = [];
          } else {
            this.searchErrorMessage = '';
            const entries = body.results[this.selectedPolicy.meta.name].entries;
            this.searchPolicyInvoked = true;
            this.sgPolicyRules = this.addOrderRankingForQueries(entries);
          }
        } else {
          this.searchErrorMessage = 'No Matching Rule';
          this.searchPolicyInvoked = true;
          this.sgPolicyRules = [];
        }
        this.loading = false;
      },
      this._controllerService.restErrorHandler('Policy search failed')
    );

  }

  ngAfterViewInit() {
    this.viewInitComplete = true;
  }


  /**
   * Component is about to exit
   */
  ngOnDestroy() {
    // publish event that AppComponent is about to be destroyed
    this._controllerService.publish(Eventtypes.COMPONENT_DESTROY, { 'component': 'sgpolicydetailComponent', 'state': Eventtypes.COMPONENT_DESTROY });
    this.subscriptions.forEach(subscription => {
      subscription.unsubscribe();
    });
    if (this.searchSubscription != null) {
      this.searchSubscription.unsubscribe();
    }
  }

  /**
   * Overide super's API
   * It will return this Component name
   */
  getClassName(): string {
    return this.constructor.name;
  }

  updateRulesByPolicy() {
    this.sgPolicyRules = this.addOrderRanking(this.selectedPolicy.spec.rules);
  }

  getSGPoliciesDetail() {
    // We perform a get as well as a watch so that we can know if the object the user is
    // looking for exists or not.
    const getSubscription = this.securityService.GetSGPolicy(this.selectedPolicyId).subscribe(
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
      }
    );
    this.subscriptions.push(getSubscription);
    this.sgPoliciesEventUtility = new HttpEventUtility<SecuritySGPolicy>(SecuritySGPolicy);
    this.sgPolicies = this.sgPoliciesEventUtility.array;
    // const subscription = this.securityService.WatchSGPolicy({ 'field-selector': 'meta.name=' + this.selectedPolicyId}).subscribe(
    const subscription = this.securityService.WatchSGPolicy({ 'field-selector': 'meta.name=' + this.selectedPolicyId }).subscribe(
      response => {
        if (this.searchSubscription != null) { this.searchSubscription.unsubscribe(); }  // avoid racing condition for searchSubscription and WatchSGPolicy
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
          this.updateRulesByPolicy();
        } else {
          // Must have received a delete event.
          this.showDeletionScreen = true;
          // Putting focus onto the overlay to prevent user
          // being able to interact with the page underneath
          this.disableFormControls();

          this.selectedPolicy = null;
          this.sgPolicyRules = [];
        }
      },
    );
    this.subscriptions.push(subscription);
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
          rule: rule
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
          rule: rule.rule
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
    }
  }

  formatApp(rule: ISecuritySGRule) {
    let protoPorts = [];
    let apps = [];
    if (rule['apps'] != null) {
      apps = rule['apps'];
    }
    if (rule['proto-ports'] != null) {
      protoPorts = rule['proto-ports'].map((entry) => {
        return entry.protocol + '/' + entry.ports;
      });
    }
    return protoPorts.concat(apps).join(', ');
  }


}
