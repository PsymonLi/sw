import { ChangeDetectorRef, Component, ViewChild, ViewEncapsulation, IterableDiffer, IterableDiffers, DoCheck, OnInit } from '@angular/core';
import { FormArray } from '@angular/forms';
import { MatExpansionPanel } from '@angular/material/expansion';
import { Animations } from '@app/animations';
import { Utility } from '@app/common/Utility';
import { GuidedSearchCriteria, SearchGrammarItem, SearchsuggestionTypes } from '@app/components/search/';
import { SearchUtil } from '@app/components/search/SearchUtil';
import { ControllerService } from '@app/services/controller.service';
import { SearchService } from '@app/services/generated/search.service';
import { AuditService } from '@app/services/generated/audit.service';
import { LRUMap } from 'lru_map';
import { AuditAuditEvent, IAuditAuditEvent, AuditAuditEvent_outcome } from '@sdk/v1/models/generated/audit';
import { SearchSearchRequest, SearchSearchResponse, SearchSearchRequest_sort_order } from '@sdk/v1/models/generated/search';
import { LazyLoadEvent } from 'primeng/primeng';
import { Subscription } from 'rxjs';
import { RowClickEvent, TableCol, CustomExportMap } from '@app/components/shared/tableviewedit';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { IApiStatus, IMonitoringArchiveQuery, MonitoringArchiveRequest, MonitoringArchiveRequestStatus_status, IMonitoringCancelArchiveRequest } from '@sdk/v1/models/generated/monitoring';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { PenServerTableComponent } from '@app/components/shared/pentable/penservertable.component';


@Component({
  selector: 'app-auditevents',
  templateUrl: './auditevents.component.html',
  styleUrls: ['./auditevents.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None
})

export class AuditeventsComponent extends DataComponent implements OnInit, DoCheck {

  public static AUDITEVENT: string = 'AuditEvent';
  @ViewChild('auditTable') auditTable: PenServerTableComponent;
  @ViewChild('auditeventSearchPanel') auditeventSearchExpansionPanel: MatExpansionPanel;

  dataObjects: ReadonlyArray<AuditAuditEvent> = [];
  isTabComponent = false;
  disableTableWhenRowExpanded = false;

  veniceRecords: number = 0;
  auditEventFieldSelectorOutput: any;

  fieldFormArray = new FormArray([]);
  currentSearchCriteria: string = '';

  exportFilename: string = 'PSM-auditevents';

  maxRecords: number = 4000; // AuditEvent query size may cause problem in backend, reduce it to 4000.
  startingSortField: string = 'meta.mod-time';
  startingSortOrder: number = -1;

  loading: boolean = false;
  auditEventDetail: AuditAuditEvent;

  // This will be a map from meta.name to AuditEvent
  cache = new LRUMap<String, AuditAuditEvent>(Utility.getAuditEventCacheSize());  // cache with limit 10

  auditArchiveQuery: IMonitoringArchiveQuery = {}; // TODO: for cancel you can add this line or may be chcek what the advanced search is returning

  lastUpdateTime: string = '';

  bodyicon: any = {
    margin: {
      top: '9px',
      left: '8px',
    },
    url: '/assets/images/icons/monitoring/ic_audit-black.svg',
  };

  headerIcon: Icon = {
    margin: {
      top: '0px',
      left: '0px',
    },
    matIcon: 'grid_on'
  };
  // Backend currently only supports time being sorted
  cols: TableCol[] = [
    { field: 'user.name', header: 'Who', class: 'auditevents-column-common auditevents-column-who', sortable: true, width: 100 },
    { field: 'meta.mod-time', header: 'Time', class: 'auditevents-column-common auditevents-column-date', sortable: true, width: '180px' },
    { field: 'action', header: 'Action', class: 'auditevents-column-common auditevents-column-action', sortable: true, width: 100 },
    { field: 'resource.tenant', header: 'Tenant', class: 'auditevents-column-common auditevents-column-tenant', sortable: true, width: 100 },
    { field: 'resource.kind', header: 'Act On (kind)', class: 'auditevents-column-common auditevents-column-act_on', sortable: true, width: 100 },
    { field: 'resource.name', header: 'Act On (name)', class: 'auditevents-column-common auditevents-column-act_on', sortable: true, width: 100 },
    { field: 'outcome', header: 'Outcome', class: 'auditevents-column-common auditevents-column-outcome', sortable: true, width: 100 },
    { field: 'client-ips', header: 'Client', class: 'auditevents-column-common auditevents-column-client_ips', sortable: false, width: 150 },
    { field: 'gateway-node', header: 'Service Node', class: 'auditevents-column-common auditevents-column-gateway_node', sortable: true, width: 100 },
    { field: 'service-name', header: 'Service Name', class: 'auditevents-column-common auditevents-column-service_name', sortable: true, width: 100 },
  ];

  exportMap: CustomExportMap = {
    'client-ips': (opts) => {
      const value = Utility.getObjectValueByPropertyPath(opts.data, opts.field);
      return Array.isArray(value) ? value.join(',') : value;
    },
  };

  archiveRequestsAuditUtility: HttpEventUtility<MonitoringArchiveRequest>;
  exportedArchiveRequests: ReadonlyArray<MonitoringArchiveRequest> = [];
  currentArchReqLength: number = 0;
  enableExport: boolean = true;
  displayArchPanel: boolean = false;
  archiveStatusMsg: string = '';
  anyQueryAfterRefresh: boolean = false;
  arrayDiffers: IterableDiffer<any>;

  requestStatus: MonitoringArchiveRequestStatus_status;
  requestName: string = '';
  firstElem: MonitoringArchiveRequest = null;
  archiveQuery: MonitoringArchiveRequest;
  watchArchiveSubscription: Subscription;

  constructor(
    protected controllerService: ControllerService,
    protected cdr: ChangeDetectorRef,
    protected searchService: SearchService,
    protected monitoringService: MonitoringService,
    protected auditService: AuditService,
    protected uiconfigsService: UIConfigsService,
    protected iterableDiffers: IterableDiffers
  ) {
    super(controllerService, uiconfigsService);
    this.arrayDiffers = iterableDiffers.find([]).create(HttpEventUtility.trackBy);
  }

  ngOnInit() {
    super.ngOnInit();
    this.penTable = this.auditTable;
    this.watchArchiveObject();
  }

  fetchLazyDataObjects(event: LazyLoadEvent) {
    this.lazyLoadEvent = event;
    setTimeout(() => {
      this.getAuditEvents();
    }, 0);
  }

  setDefaultToolbar() {
    const buttons = [];
    if (this.checkPermissions()) {
      const exportButton = {
        cssClass: 'global-button-primary global-button-padding',
        genTooltip: () => this.getTooltip(),
        text: 'EXPORT AUDIT EVENTS',
        computeClass: () => !this.auditTable.showRowExpand && this.enableExport ? '' : 'global-button-disabled',
        callback: () => {
          this.auditTable.createNewObject();
          this.displayArchPanel = false;
        }
      };
      buttons.push(exportButton);
    }
    this.controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [{ label: 'Audit Events', url: Utility.getBaseUIUrl() + 'monitoring/auditevents' }]
    });
  }

  getTooltip() {
    return this.auditTable.showRowExpand ? 'Row is Expanded' : (this.enableExport ? 'Ready to submit archive request' : 'Only ONE archive request can be running at a time');
  }

  /**
   * User will only be allowed to fire only one archive Audit Event request at a time
   * Show the status of the archive request in display panel
   * Allow user to download files on success or cancel when the request is running
   */
  ngDoCheck() {
    const changes = this.arrayDiffers.diff(this.exportedArchiveRequests);
    if (changes) {
      this.handleArchiveLogChange();
      this.refreshGui(this.cdr);
    }
  }

  getEmittedArchiveQuery(valueEmitted) {
    this.archiveQuery = valueEmitted;
    this.watchArchiveObject();
  }

  private handleArchiveLogChange() {
    if (this.exportedArchiveRequests.length >= this.currentArchReqLength && this.archiveQuery) {
      let index = -1;
      for (const archiveRequest of this.exportedArchiveRequests) {
        if (archiveRequest.meta.name === this.archiveQuery.meta.name) {
          index = this.exportedArchiveRequests.indexOf(archiveRequest);
        }
      }
      if (index !== -1) {
        this.currentArchReqLength += (this.exportedArchiveRequests.length - this.currentArchReqLength);
        this.firstElem = this.exportedArchiveRequests[index];
        if (this.firstElem.status !== null) {
          if (this.anyQueryAfterRefresh || this.firstElem.status.status === MonitoringArchiveRequestStatus_status.running || this.firstElem.status.status === null) {
            this.displayArchPanel = true;
          }
          this.requestName = this.firstElem.meta.name;
          if (this.firstElem.status.status === MonitoringArchiveRequestStatus_status.running || this.firstElem.status.status === null) {
            // disable the archive request button
            this.enableExport = false;
            this.requestStatus = MonitoringArchiveRequestStatus_status.running;
          } else if (this.firstElem.status.status === MonitoringArchiveRequestStatus_status.completed) {
            // show download link
            this.enableExport = true;
            this.requestStatus = MonitoringArchiveRequestStatus_status.completed;
            // enable the archive request button
          } else {
            this.enableExport = true;
            this.requestStatus = this.firstElem.status.status;
          }
        }
      }
    }
  }

  checkPermissions(): boolean {
    const boolArchiveRequestActions = this.uiconfigsService.isAuthorized(UIRolePermissions['monitoringarchiverequest_all-actions']);
    const boolObjStoreCreate = this.uiconfigsService.isAuthorized(UIRolePermissions.objstoreobject_create);
    return (boolObjStoreCreate && boolArchiveRequestActions);
  }

  /**
   * Cancel any Running Archive Request
   */
  onCancelRecord(event) {
    const object = this.firstElem;
    if (event) {
      event.stopPropagation();
    }
    // Should not be able to cancel any record while we are editing
    if (this.auditTable.showRowExpand) {
      return;
    }
    this.controllerService.invokeConfirm({
      header: this.generateCancelConfirmMsg(object),
      message: 'This action cannot be reversed',
      acceptLabel: 'Cancel Request',
      accept: () => {
        const cancelRequest: IMonitoringCancelArchiveRequest = {
          kind: object.kind,
          'api-version': object['api-version'],
          meta: object.meta
        };
        const sub = this.monitoringService.Cancel(object.meta.name, cancelRequest).subscribe(
          (response) => {
            // TODO: BETTER SOL: From backend if we have some status value saying cancellation in process!
            this.controllerService.invokeSuccessToaster(Utility.CANCEL_SUCCESS_SUMMARY, this.generateCancelSuccessMsg(object));
          },
          (err) => {
            if (err.body instanceof Error) {
              console.error('Service returned code: ' + err.statusCode + ' data: ' + <Error>err.body);
            } else {
              console.error('Service returned code: ' + err.statusCode + ' data: ' + <IApiStatus>err.body);
            }
            this.controllerService.invokeRESTErrorToaster(Utility.CANCEL_FAILED_SUMMARY, err);
          }
        );
        this.subscriptions.push(sub);
      }
    });
  }

  generateCancelSuccessMsg(object: MonitoringArchiveRequest): string {
    return 'Canceled archive request ' + object.meta.name;
  }

  generateCancelConfirmMsg(object: any): string {
    return 'Are you sure to cancel archive request: ' + object.meta.name;
  }

  /**
   * Watch on Archive Audit Event Requests
   * Show any running archive Audit Event request when the user comes on this page
   */
  watchArchiveObject() {
    this.archiveRequestsAuditUtility = new HttpEventUtility<MonitoringArchiveRequest>(MonitoringArchiveRequest);
    this.exportedArchiveRequests = this.archiveRequestsAuditUtility.array;
    if (this.watchArchiveSubscription) {
      this.watchArchiveSubscription.unsubscribe();
    }
    this.watchArchiveSubscription = this.monitoringService.WatchArchiveRequest({ 'field-selector': 'spec.type=auditevent' }).subscribe(
      (response) => {
        this.currentArchReqLength = this.exportedArchiveRequests.length;
        this.archiveRequestsAuditUtility.processEvents(response);
        if (this.exportedArchiveRequests.length > 0 && this.exportedArchiveRequests[0].status.status === MonitoringArchiveRequestStatus_status.running) {
          this.enableExport = false;
          this.displayArchPanel = true;
        }
        this.refreshGui(this.cdr);
      },
      this.controllerService.webSocketErrorHandler('Failed to get Audit Archive Requests')
    );
    this.subscriptions.push(this.watchArchiveSubscription);
  }

  getArchiveQuery(archQuer: IMonitoringArchiveQuery) {
    this.auditArchiveQuery = archQuer;
  }

  getAuditEvents(field = this.auditTable.sortField, order = this.auditTable.sortOrder) {
    this.loading = true;
    try {
      if (this.lazyLoadEvent && Object.keys(this.lazyLoadEvent).length === 0) {
        this.auditTable.resetLazyLoadEvent();
        this.loading = false;
        this.refreshGui(this.cdr);
        return;
      }
      const searchSearchRequest = this.auditTable.advancedSearchComponent.getSearchRequest(field, order, 'AuditEvent', false, this.maxRecords);
      searchSearchRequest.from = this.lazyLoadEvent.first;
      searchSearchRequest['max-results'] = this.lazyLoadEvent.rows;
      searchSearchRequest['sort-by'] = this.lazyLoadEvent.sortField;
      searchSearchRequest['sort-order'] = this.lazyLoadEvent.sortOrder > 0 ? SearchSearchRequest_sort_order.ascending : SearchSearchRequest_sort_order.descending;
      this.refreshGui(this.cdr);
      this._callSearchRESTAPI(searchSearchRequest);
    } catch (error) {
      this.controllerService.invokeErrorToaster('Input Error', error.toString());
      this.loading = false;
      this.refreshGui(this.cdr);
    }
  }

  private _callSearchRESTAPI(searchSearchRequest: SearchSearchRequest) {
    const subscription = this.searchService.PostQuery(searchSearchRequest).subscribe(
      response => {
        this.clearLazyLoadEvent();
        const data: SearchSearchResponse = response.body as SearchSearchResponse;
        let objects = data.entries;
        if (!objects || objects.length === 0) {
          this.controllerService.invokeInfoToaster('Information', 'No audit-events found. Please change search criteria.');
          objects = [];
        }
        const entries = [];
        for (let k = 0; k < objects.length; k++) {
          entries.push(objects[k].object); // objects[k] is a SearchEntry object
        }
        this.lastUpdateTime = new Date().toISOString();
        this.dataObjects = this.mergeEntriesByName(entries);
        // response.body  is like  {total-hits: 430, actual-hits: 430, time-taken-msecs: 100, entries: Array(430)}.
        if (searchSearchRequest !== undefined && searchSearchRequest != null && searchSearchRequest.query != null && searchSearchRequest.query.fields != null && Array.isArray(searchSearchRequest.query.fields.requirements) && searchSearchRequest.query.fields.requirements.length === 0) {
          this.veniceRecords = response.body['total-hits'];
        }
        this.totalRecords = response.body['total-hits'];
        this.loading = false;
        this.refreshGui(this.cdr);
      },
      (error) => {
        this.loading = false;
        this.controllerService.invokeRESTErrorToaster('Failed to get audit-events', error);
        this.refreshGui(this.cdr);
      }
    );
    this.subscriptions.push(subscription);
  }

  /**
   * Merge Audit Event by meta.name to remove redundant info
   * It will also preserve uuids in a list for getting request and response
   * @param entries
   */
  mergeEntriesByName(entries: IAuditAuditEvent[]): AuditAuditEvent[] {
    const tmpMap = {};
    entries.forEach(ele => {
      const eleCopy = Utility.getLodash().cloneDeep(ele);
      const key = ele.meta.name;
      if (tmpMap.hasOwnProperty(key)) {
        if (tmpMap[key].outcome === AuditAuditEvent_outcome.failure) {
          eleCopy.outcome = AuditAuditEvent_outcome.failure;
        }
        tmpMap[key] = Utility.getLodash().merge(tmpMap[key], eleCopy);
        tmpMap[key]['uuids'].push(eleCopy.meta.uuid);
      } else {
        tmpMap[key] = eleCopy;
        tmpMap[key]['uuids'] = [eleCopy.meta.uuid];
      }
    });
    return Object.values(tmpMap);
  }

  /**
   * This API serves html template
   */
  displayAuditEvent(rowData): string {
    return JSON.stringify(rowData, null, 4);
  }

  /**
   * Handle logics when user click the row
   * @param event
  */
  showAuditEventDetail(event: RowClickEvent) {
    this.expandRowRequest(event.event, event.rowData);
  }

  /**
   * Override super's API.
   * This api serves html template
   * @param auditevent
   * @param col
   */
  displayColumn(auditevent, col): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(auditevent, fields);
    const column = col.field;
    switch (column) {
      case 'client-ips':
        return Array.isArray(value) ? value.join(',') : value;
      default:
        return Array.isArray(value) ? JSON.stringify(value, null, 2) : value;
    }
  }

  /**
   * This API is a helper function. It generates GuidedSearchCriteria from field-selector.
   * @param includeKind
   */
  genGuidedSearchCriteria(includeKind: boolean = true): GuidedSearchCriteria {
    const obj = {
      is: [AuditeventsComponent.AUDITEVENT],
      has: (this.advancedSearchComponent) ? this.advancedSearchComponent.getValues() : []
    };
    if (!includeKind) {
      delete obj.is;
    }
    return obj;
  }

  /**
   * This function builds request json for invoking Search API
   * It should examine the current search context to decide the type of search. (by category, kind, label, fields,etc)
   * @param searched
   */
  protected buildComplexSearchPayload(list: any[], searched: string): any {
    const payload = {
      'max-results': 50,
      'query': {
      }
    };
    // We evaluate the has operations last so that we
    // know if the object kind is an event or not.
    const fieldRequirementIndexes = [];
    for (let i = 0; i < list.length; i++) {
      const obj: SearchGrammarItem = list[i];
      switch (obj.type) {
        case SearchsuggestionTypes.OP_IS:
          payload.query['kinds'] = [AuditeventsComponent.AUDITEVENT];
          break;
        case SearchsuggestionTypes.OP_HAS:
          fieldRequirementIndexes.push(i);
          break;
        default:
          console.error(this.getClassName() + 'buildComplexSearchPayload() does not recognize ' + searched);
      }
    }
    fieldRequirementIndexes.forEach((index) => {
      const obj = list[index];
      const isEvent = payload.query['kinds'] != null && payload.query['kinds'].length === 1 && SearchUtil.isKindInSpecialEventList(payload.query['kinds'][0]);
      payload.query['fields'] = {
        'requirements': SearchUtil.buildSearchFieldsLabelsPayloadHelper(obj, true, isEvent)
      };
    });
    return payload;
  }

  /**
   * This serves HTML API. It clear audit-event search and refresh data.
   * @param $event
   */
  onCancelAuditSearch($event) {
    this.controllerService.invokeInfoToaster('Information', 'Cleared search criteria, audit-events refreshed.');
    this.getAuditEvents();
    this.refreshGui(this.cdr);
  }

  /**
   * This is a helper API.  It generates a string representing search criteria.
   */
  buildSearchCriteriaString(): string {
    const guidedSearchCriteria: GuidedSearchCriteria = this.genGuidedSearchCriteria(false);
    return SearchUtil.getSearchInputStringFromGuidedSearchCriteria(guidedSearchCriteria);
  }

}
