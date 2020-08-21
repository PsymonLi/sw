import { ChangeDetectorRef, ChangeDetectionStrategy, Component, OnInit, ViewChild, ViewEncapsulation } from '@angular/core';
import { ControllerService } from '@app/services/controller.service';
import { Utility } from '@app/common/Utility';
import { Animations } from '@app/animations';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { IPUtility } from '@app/common/IPUtility';
import { OverlayPanel } from 'primeng/primeng';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { ClusterDistributedServiceCard } from '@sdk/v1/models/generated/cluster';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { ClusterService } from '@app/services/generated/cluster.service';
import { PrettyDatePipe } from '@app/components/shared/Pipes/PrettyDate.pipe';
import { TableCol, CustomExportMap } from '@app/components/shared/tableviewedit';
import { SearchUtil } from '@app/components/search/SearchUtil';
import { SecurityService } from '@app/services/generated/security.service';
import { SecurityNetworkSecurityPolicy } from '@sdk/v1/models/generated/security';
import { PolicyRuleTuple } from './';
import { SelectItem } from 'primeng/primeng';
import { TimeRange } from '@app/components/shared/timerange/utility';
import { Subscription, Subject } from 'rxjs';
import { debounceTime } from 'rxjs/operators';
import { ValidatorFn } from '@angular/forms';
import { TimeRangeOption, citadelTimeOptions, citadelMaxTimePeriod } from '@app/components/shared/timerange/timerange.component';
import { FwlogService } from '@app/services/generated/fwlog.service';
import { IFwlogFwLogQuery, FwlogFwLogQuery, FwlogFwLogQuery_sort_order, FwlogFwLogQuery_actions, FwlogFwLogList, FwlogFwLog, IFwlogFwLog } from '@sdk/v1/models/generated/fwlog';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';

@Component({
  selector: 'app-fwlogs',
  templateUrl: './fwlogs.component.html',
  styleUrls: ['./fwlogs.component.scss'],
  animations: [Animations],
  changeDetection: ChangeDetectionStrategy.OnPush,
  encapsulation: ViewEncapsulation.None
})
export class FwlogsComponent extends DataComponent implements OnInit {
  public static ALLOPTION = 'All';
  @ViewChild('fwlogsTable') fwlogsTable: PentableComponent;
  @ViewChild('ruleDetailsOverlay') overlay: OverlayPanel;

  dataObjects: ReadonlyArray<FwlogFwLog> = [];
  loading: boolean = false;

  isTabComponent = false;
  disableTableWhenRowExpanded = false;
  maxRecords: number = 8000;

  getVeniceRecords: boolean = false;
  veniceRecords: number = 0;

  query: FwlogFwLogQuery = new FwlogFwLogQuery({ 'sort-order': FwlogFwLogQuery_sort_order.descending, 'max-results': this.maxRecords }, false);

  actionOptions = Utility.convertEnumToSelectItem(FwlogFwLogQuery_actions);

  exportFilename: string = 'PSM-fwlogs';
  startingSortField: string = 'time';
  startingSortOrder: number = -1;

  naples: ClusterDistributedServiceCard[] = [];
  // Used for processing the stream events
  naplesEventUtility: HttpEventUtility<ClusterDistributedServiceCard>;
  macAddrToName: { [key: string]: string; } = {};

  // Used for rule details shows on hover
  currentRule = '';
  policyName = '';
  currentRuleObject = {};
  ruleMap = new Map<string, PolicyRuleTuple>();

  // Holds all policy objects
  sgPolicies: ReadonlyArray<SecurityNetworkSecurityPolicy> = [];
  sgPoliciesEventUtility: HttpEventUtility<SecurityNetworkSecurityPolicy>;

  lastUpdateTime: string = '';

  bodyIcon: any = {
    margin: {
      top: '9px',
      left: '8px'
    },
    url: '/assets/images/icons/monitoring/icon-firewall-policy-black.svg'
  };

  headerIcon: Icon = {
    margin: {
      top: '0px',
      left: '10px',
    },
    matIcon: 'grid_on'
  };

  inOverlay: boolean = false;

  selectedTimeRange: TimeRange;

  // Only time is supported as sortable by the backend
  cols: TableCol[] = [
    { field: 'meta.mod-time', header: 'Time', class: 'fwlogs-column', sortable: true, width: 16 },
    { field: 'source-ip', header: 'Source', class: 'fwlogs-column-ip', sortable: true, width: 10 },
    { field: 'destination-ip', header: 'Destination', class: 'fwlogs-column-ip', sortable: true, width: 10 },
    { field: 'protocol', header: 'Protocol', class: 'fwlogs-column-port', sortable: true, width: 7 },
    { field: 'source-port', header: 'Src Port', class: 'fwlogs-column-port', sortable: true, width: 5 },
    { field: 'destination-port', header: 'Dest Port', class: 'fwlogs-column-port', sortable: true, width: 6 },
    { field: 'action', header: 'Action', class: 'fwlogs-column', sortable: true, width: 5 },
    { field: 'reporter-id', header: 'Reporter', class: 'fwlogs-column', sortable: true, width: 7 },
    { field: 'direction', header: 'Direction', class: 'fwlogs-column', sortable: true, width: 7 },
    { field: 'session-id', header: 'Session ID', class: 'fwlogs-column', sortable: true},
    { field: 'flow-action', header: 'Session Action', class: 'fwlogs-column', sortable: true},
    { field: 'rule-id', header: 'Policy Name', class: 'fwlogs-column', sortable: true, width: 8, roleGuard: 'securitynetworksecuritypolicy_read' },
  ];

  searchSubscription: Subscription;

  fwlogsQueryObserver: Subject<IFwlogFwLogQuery> = new Subject();

  timeRangeOptions: TimeRangeOption[] = citadelTimeOptions;
  maxTimePeriod = citadelMaxTimePeriod;

  exportMap: CustomExportMap = {
    'reporter-id': (opts) => {
      return this.getNaplesNameFromReporterID(opts.data);
    },
    'meta.mod-time': (opts) => {
      const dataObj = opts.data as FwlogFwLog;
      const time = dataObj.meta['mod-time'] as any;
      return new PrettyDatePipe('en-US').transform(time, 'ns');
    },
    'rule-id': (opts) => {
      const dataObj = opts.data as FwlogFwLog;
      return this.displayPolicyName(dataObj);
    },
  };

  constructor(
    protected controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    private clusterService: ClusterService,
    protected cdr: ChangeDetectorRef,
    protected securityService: SecurityService,
    protected fwlogService: FwlogService
  ) {
    super(controllerService, uiconfigsService);
  }

  // Fetches count of logs inside venice
  fetchVeniceRecords() {
    const query = new FwlogFwLogQuery({
       'sort-order': FwlogFwLogQuery_sort_order.descending,
       'max-results': 2, // For fetching the count of venice records we don't need to fetch all records
       'actions': null,
       'end-time': new Date().toISOString(),
       'tenant': [Utility.getInstance().getTenant()],
      }, false);
      this.loading = true;
      this.getVeniceRecords = true;
      this.fwlogsQueryObserver.next(query);
  }

  addAllOption(options: SelectItem[]) {
    let newSelectItems: SelectItem[] = [];
    newSelectItems.push({ label: 'All', value: 'All'});
    newSelectItems = newSelectItems.concat(options);
    return newSelectItems;
  }

  setDefaultToolbar() {
      const buttons = [];
      // TODO: 2020-02-28, we enable fwLog page. But backend has issue in /telemetry/v1/fwlogs API.
      if (this.uiconfigsService.isAuthorized(UIRolePermissions.monitoringfwlogpolicy_read)) {
        buttons.push({
          cssClass: 'global-button-primary fwlogs-button',
          text: 'FIREWALL LOG EXPORT POLICIES',
          callback: () => { this.controllerService.navigate(['/monitoring', 'fwlogs', 'fwlogpolicies']); }
        });
      }
    this.controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [{ label: 'Firewall Logs', url: Utility.getBaseUIUrl() + 'monitoring/fwlogs' }]
    });
  }

  ngOnInit() {
    super.ngOnInit();
    this.penTable = this.fwlogsTable;
    this.fwlogQueryListener();
    this.fetchVeniceRecords();
    this.getNaples();
    this.setSearchFormValidators();
    this.getSGPolicies();
    this.setDefaultToolbar();
    this.actionOptions = this.addAllOption(this.actionOptions);
    this.query.$formGroup.value.actions.push(FwlogsComponent.ALLOPTION);
  }


  private setSearchFormValidators() {
    this.query.$formGroup.get('source-ips').setValidators(IPUtility.isValidIPValidator);
    this.query.$formGroup.get('destination-ips').setValidators(IPUtility.isValidIPValidator);
    const msg = 'port should be number in range [0, 65536]'; // VS-783 set validators for Ports
    this.query.$formGroup.get('source-ports').setValidators(this.isPortInputsValid('source-ports', msg));
    this.query.$formGroup.get('destination-ports').setValidators(this.isPortInputsValid('destination-ports', msg));
  }

  setTimeRange(timeRange: TimeRange) {
    // Pushing into next event loop
    setTimeout(() => {
      if (timeRange.isSame(this.selectedTimeRange)) {
        return;
      }
      this.selectedTimeRange = timeRange;
      if (!this.getVeniceRecords) {
        this.getFwlogs();
      }
    }, 0);
  }


  displayColumn(data, col): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(data, fields);
    const column = col.field;
    switch (column) {
      default:
        return Array.isArray(value) ? JSON.stringify(value, null, 2) : value;
    }
  }

  displayPolicyName(data): string {
    return this.ruleMap.get(data['rule-id']) ? this.ruleMap.get(data['rule-id']).policy.meta.name : null;
  }

  getNaplesNameFromReporterID(data: FwlogFwLog): string {
    if (data == null || data['reporter-id'] == null) {
      return '';
    }
    let name = this.macAddrToName[data['reporter-id']];
    if (name == null || name === '') {
      name = data['reporter-id'];
    }
    return name;
  }

  clearSearch() {
    this.query = new FwlogFwLogQuery({ 'sort-order': FwlogFwLogQuery_sort_order.descending, 'max-results': this.maxRecords }, false);
    this.getFwlogs();  // after clear search criteria, we want to restore table records.
  }

  onTableSort(event) {
    // FwlogFwLogQuery only does time ordering sort, so only fetch if sorting by time column.
    // otherwise, just let table do local sort.
    if (event.field === 'meta.mod-time') {
      this.getFwlogs(event.order);
    }
  }

  getNaples() {
    const dscSubscription = this.clusterService.ListDistributedServiceCardCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.naples  = response.data as ClusterDistributedServiceCard [];
        // mac-address to name map
        this.macAddrToName = {};
        for (const smartnic of this.naples) {
          this.macAddrToName[smartnic.meta.name] = smartnic.spec.id;
        }
        this.refreshGui(this.cdr);
      }
    );
    this.subscriptions.push(dscSubscription); // add subscription to list, so that it will be cleaned up when component is destroyed.
  }


  handleRuleClick() {
    this.controllerService.navigate(['/security', 'sgpolicies', this.policyName]);
  }

  handleOverlayEnter(event) {
    this.inOverlay = true;
  }

  // The icon and overlay generated dont perfectly overlap.
  // This delay allows the user to enter the overlay if they need to copy or select the text
  rowLeave() {
    setTimeout(() => {
      if (!this.inOverlay) {
        this.overlay.hide();
      }
    }, 100);
  }

  handleOverlayLeave(event) {
    this.overlay.hide();
    this.inOverlay = false;
  }

  handleHover(data) {
    this.currentRule = data['rule-id'];
    const mapValue: PolicyRuleTuple = this.ruleMap.get(this.currentRule);
    if (mapValue != null) {
      this.policyName = mapValue.policy.meta.name;
      this.currentRuleObject = mapValue.rule;
    } else {
      this.policyName = null;
      this.currentRuleObject = null;
    }
  }

  getSGPolicies() {
    this.sgPoliciesEventUtility = new HttpEventUtility<SecurityNetworkSecurityPolicy>(SecurityNetworkSecurityPolicy);
    this.sgPolicies = this.sgPoliciesEventUtility.array;
    const subscription = this.securityService.WatchNetworkSecurityPolicy().subscribe(
      response => {
        this.sgPoliciesEventUtility.processEvents(response);
        for (const policy of this.sgPolicies) {
          if (policy.spec != null && policy.spec.rules != null) {
            const rulesLength = policy.spec.rules.length;
            for (let i = 0 ; i < rulesLength ; i++) {
              if (policy.spec.rules[i] != null) {
                const newTuple: PolicyRuleTuple = {
                  policy : policy,
                  rule: policy.spec.rules[i]
                };
                if (policy.status['rule-status'] && policy.status['rule-status'][i]) {  // policy.status['rule-status'] may not be available.
                     this.ruleMap.set(policy.status['rule-status'][i]['rule-hash'], newTuple);
                }
              }
            }
          }
        }
        this.refreshGui(this.cdr);
      },
      this.controllerService.webSocketErrorHandler('Failed to get security policies')
    );
    this.subscriptions.push(subscription);
  }

  fwlogQueryListener() {
    // In case multiple components invoke a request for logs
    // We buffer them and take the last request in a 500ms window
    const sub = this.fwlogsQueryObserver.pipe(debounceTime(500)).subscribe(
      (query) => {
         this.searchSubscription = this.fwlogService.PostGetLogs(query).subscribe(  //  use POST
          (resp) => {
            this.controllerService.removeToaster('Fwlog Search Failed');
            this.lastUpdateTime = new Date().toISOString();
            const body = resp.body as FwlogFwLogList;
            if (!this.isSearching()) {
              const listMeta = body['list-meta'] || {};
              this.veniceRecords = listMeta['total-count'] || 0;
            }
            if (!this.getVeniceRecords) {
              // Code to get data
              const logs = body.items || [];
              this.dataObjects = logs.map((l) => {
                return new FwlogFwLog(l);
              });
              this.loading = false;
            } else {
              this.getVeniceRecords = false;
              if (this.selectedTimeRange !== undefined) {
                this.getFwlogs();
              } else {
                this.loading = false;
              }
            }
            this.refreshGui(this.cdr);
          },
          (error) => {
            this.dataObjects = [];
            this.controllerService.invokeRESTErrorToaster('Fwlog Search Failed', error);
            this.loading = false;
            this.refreshGui(this.cdr);
          }
        );
        this.subscriptions.push(this.searchSubscription);
      }
    );
    this.subscriptions.push(sub);
  }

  getFwlogs(order = this.fwlogsTable.sortOrder) {
    if (this.query.$formGroup.invalid) {
      this.controllerService.invokeErrorToaster('Fwlog Search', 'Invalid query');
      return;
    }

    this.loading = true;

    if (this.searchSubscription != null) {
      this.searchSubscription.unsubscribe();
    }

    // Remove any invalid query toasters if there are any.
    this.controllerService.removeToaster('Fwlog Search');

    let sortOrder = FwlogFwLogQuery_sort_order.ascending;
    if (order === -1) {
      sortOrder = FwlogFwLogQuery_sort_order.descending;
    }

    const query = new FwlogFwLogQuery(null, false);
    const queryVal: any = this.query.getFormGroupValues();
    if (queryVal.actions != null && queryVal.actions.includes(FwlogsComponent.ALLOPTION)) {
      queryVal.actions = null;
    }
    const fields = [
      'source-ips',
      'destination-ips',
      'protocols'
    ];
    fields.forEach(
      (field) => {
        if (typeof queryVal[field] === 'string') {
          query[field] = queryVal[field].split(',').map((e: string) => {
            return e.trim();
          }).filter((e: string) => {
            if (e.length === 0) {
              return false;
            }
            return true;
          });
        }
      }
    );
    const fieldsInt = [
      'source-ports',
      'destination-ports'
    ];

    if (queryVal.actions !== null) {
      query.actions = [queryVal.actions];
    } else {
      query.actions = queryVal.actions;
    }


    fieldsInt.forEach(
      (field) => {
        if (typeof queryVal[field] === 'string') {
          query[field] = queryVal[field].split(',')
            .filter((e: string) => {
              if (e.length === 0) {
                return false;
              }
              return true;
            })
            .map((e: string) => {
              return parseInt(e, 10);
            })
            .filter((e) => {
              return !isNaN(e);
            });
        }
      }
    );

    query['sort-order'] = sortOrder;
    query['max-results'] = this.maxRecords;

    if (this.selectedTimeRange != null) {
      query['start-time'] = this.selectedTimeRange.getTime().startTime.toISOString() as any;
      query['end-time'] = this.selectedTimeRange.getTime().endTime.toISOString() as any;
    }

    query.tenants = [Utility.getInstance().getTenant()];

    // Get request
    this.fwlogsQueryObserver.next(query);
  }

  keyUpInput(event) {
    if (event.keyCode === SearchUtil.EVENT_KEY_ENTER) {
      this.getFwlogs();
    }
  }

  isPortInputsValid(objectname: string, msg: string): ValidatorFn {
    return Utility.isValueInRangeValdator(0, 65536, objectname, msg);
  }

  isSearching() {
    const searchFields = ['destination-ips', 'destination-ports', 'source-ips', 'source-ports', 'protocols', 'actions'];
    const formGroupValues = this.query.getFormGroupValues() || {};
    return Object.keys(formGroupValues).some(formGroupKey => {
      return searchFields.some(searchField => {
        return searchField === formGroupKey && formGroupValues[formGroupKey] && formGroupValues[formGroupKey].length > 0;
      });
    });
  }
}
