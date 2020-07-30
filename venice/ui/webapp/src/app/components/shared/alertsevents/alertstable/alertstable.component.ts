import { Component, OnInit, Input, ViewChild, SimpleChanges, OnChanges, OnDestroy, ViewEncapsulation, ChangeDetectionStrategy, ChangeDetectorRef } from '@angular/core';
import { MonitoringAlert, MonitoringAlertSpec_state, MonitoringAlertStatus_severity, MonitoringAlertSpec_state_uihint } from '@sdk/v1/models/generated/monitoring';
import { EventsEventAttributes_severity } from '@sdk/v1/models/generated/events';
import { TimeRange } from '@app/components/shared/timerange/utility';
import { TableCol, CustomExportMap } from '@app/components/shared/tableviewedit';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { Utility } from '@app/common/Utility';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { AlerttableService } from '@app/services/alerttable.service';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { SearchService } from '@app/services/generated/search.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { Observable, forkJoin, throwError, Subscription } from 'rxjs';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { AlertsEventsSelector } from '@app/components/shared/alertsevents/alertsevents.component';
import { Animations } from '@app/animations';
import { TimeRangeComponent } from '@app/components/shared/timerange/timerange.component';
import { ValidationErrors, FormArray } from '@angular/forms';
import { IStagingBulkEditAction, IBulkeditBulkEditItem } from '@sdk/v1/models/generated/staging';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';

@Component({
  selector: 'app-alertstable',
  templateUrl: './alertstable.component.html',
  styleUrls: ['./alertstable.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class AlertstableComponent extends DataComponent implements OnInit, OnChanges {
  @ViewChild('timeRangeComponent') timeRangeComponent: TimeRangeComponent;
  @ViewChild('alertsTable') alertsTable: PentableComponent;

  isTabComponent: boolean;
  disableTableWhenRowExpanded: boolean;
  exportFilename: string = 'PSM-alerts';

  // If provided, will only show alerts and events
  // where the source node matches
  @Input() selector: AlertsEventsSelector;
  @Input() searchedAlert: string;
  @Input() showAlertsAdvSearch: boolean = false;

  // this property indicate if user is authorized to update alerts
  alertUpdatable: boolean = true;

  // holds a subset (possibly all) of this.alerts
  // This are the alerts that will be displayed
  dataObjects: MonitoringAlert[] = [];
  dataObjectsBackUp: MonitoringAlert[] = [];

  fieldFormArray = new FormArray([]);
  maxSearchRecords: number = 4000;
  alertsLoading = false;
  advSearchCols: TableCol[] = [];

  alertSubscription: Subscription;

  // Alert State filters
  selectedStateFilters = [MonitoringAlertSpec_state_uihint.open];
  possibleFilterStates = Object.values(MonitoringAlertSpec_state_uihint);

  // The current alert severity filter, set to null if it is on All.
  currentAlertSeverityFilter: string;

  severityEnum: any = EventsEventAttributes_severity;

  cols: TableCol[] = [
    { field: 'meta.mod-time', header: 'Modification Time', class: 'alertsevents-column-date', sortable: true, width: '180px' },
    { field: 'meta.creation-time', header: 'Creation Time', class: 'alertsevents-column-date', sortable: true, width: 15 },
    { field: 'status.severity', header: 'Severity', class: 'alertsevents-column-severity', sortable: false, width: 8 },
    { field: 'status.message', header: 'Message', class: 'alertsevents-column-message', sortable: false, width: 25 },
    { field: 'spec.state', header: 'State', class: 'alertsevents-column-state', sortable: false, width: 5 },
    { field: 'status.source', header: 'Source Node & Component', class: 'alerts-column-source', sortable: false },
    { field: 'status.total-hits', header: 'Total Hits', class: 'alerts-column-totalhits', sortable: true },
  ];

  alertsIcon: Icon = {
    margin: {
      top: '0px',
      left: '0px',
    },
    matIcon: 'notifications'
  };

  // TimeRange for alerts
  alertsSelectedTimeRange: TimeRange;
  alertsTimeConstraints: string = '';

  // Will hold mapping from severity types to counts
  alertNumbers: { [severity in MonitoringAlertStatus_severity]: number } = {
    'info': 0,
    'warn': 0,
    'critical': 0,
  };

  // Holds all alerts
  alerts: ReadonlyArray<MonitoringAlert> = [];

  // Query params to send for watch
  alertQuery = {};

  // Used for processing watch stream events
  alertsEventUtility: HttpEventUtility<MonitoringAlert>;

  exportMap: CustomExportMap = {
    'status.source': (opts) => {
      return this.getSourceNodeAndComponentField(opts.data);
    }
  };

  selectedAlert: MonitoringAlert = null;

  constructor(protected _controllerService: ControllerService,
    protected _alerttableService: AlerttableService,
    protected uiconfigsService: UIConfigsService,
    protected searchService: SearchService,
    protected cdr: ChangeDetectorRef,
    protected monitoringService: MonitoringService,
  ) {
    super(_controllerService, uiconfigsService);
  }

  ngOnInit() {
    super.ngOnInit();
    this.penTable = this.alertsTable;
    this.alertUpdatable = this.uiconfigsService.isAuthorized(UIRolePermissions.monitoringalert_update);
    if (this.showAlertsAdvSearch) {
      this.buildAdvSearchCols();
    }
    this.genQueryBodies();
    // If get alerts/events wasn't triggered by on change
    if (!this.alertSubscription) {
      this.getAlerts();
    }
    if (this.searchedAlert) {
      this.getSearchedAlert();
    }
  }

  buildAdvSearchCols() {
    this.advSearchCols = this.cols.filter((col: TableCol) => {
      return (col.field !== 'status.source');
    });

    this.advSearchCols.push(
      { field: 'status.source.node-name', header: 'Source Node', kind: 'Alert' },
      { field: 'status.source.component', header: 'Source Component', kind: 'Alert' }
    );
  }

  clearAlertNumbersObject() {
    this.alertNumbers = {
      'info': 0,
      'warn': 0,
      'critical': 0,
    };
  }

  setAlertNumbersObject() {
    // Reset counters
    Object.keys(MonitoringAlertStatus_severity).forEach(severity => {
      this.alertNumbers[severity] = 0;
    });
    this.alerts.forEach(alert => {
      this.alertNumbers[alert.status.severity] += 1;
    });
  }

  getSearchedAlert() {
    const searchedAlertSubscription = this.monitoringService.GetAlert(this.searchedAlert).subscribe(
      response => {
        this.selectedAlert = response.body as MonitoringAlert;
      },
      (error) => {
        // User may tweak browser url and make invalid alert name in the url, we will catch and throw error.
        this.selectedAlert = null;
        this.controllerService.invokeRESTErrorToaster('Failed to fetch alert ' + this.searchedAlert, error);
      }
    );
    this.subscriptions.push(searchedAlertSubscription);
  }

  closeDetails() {
    this.selectedAlert = null;
  }

  getAlerts() {
    this.dataObjects = [];
    this.dataObjectsBackUp = [];
    this.alertsLoading = true;
    this.refreshGui(this.cdr);
    this.alertsEventUtility = new HttpEventUtility<MonitoringAlert>(MonitoringAlert);
    this.alerts = this.alertsEventUtility.array;
    if (this.alertSubscription) {
      this.alertSubscription.unsubscribe();
    }
    this.alertSubscription = this.monitoringService.WatchAlert(this.alertQuery).subscribe(
      response => {
        this.alertsLoading = false;
        if (this.showAlertsAdvSearch) {
          this.alertsTable.clearSearch();
        }
        this.alertsEventUtility.processEvents(response);
        this.dataObjectsBackUp = [...this.alerts];
        this.setAlertNumbersObject();
        this.filterAlerts();
      },
      (error) => {
        this.alertsLoading = false;
        this.refreshGui(this.cdr);
        this._controllerService.webSocketErrorHandler('Failed to get Alert');
      }
    );
    this.subscriptions.push(this.alertSubscription);
    setTimeout(() => {
      if (this.alerts.length === 0) {
        if (this.showAlertsAdvSearch) {
          this.alertsTable.clearSearch();
        }
        this.alertsLoading = false;
        this.refreshGui(this.cdr);
      }
    }, 5000);
  }

  /**
 * Submits an HTTP request to mark the alert as resolved
 * @param alert Alert to resolve
 */
  resolveAlert(alert: MonitoringAlert, $event) {
    const summary = 'Alert Resolved';
    const msg = 'Marked alert as resolved';
    this.updateAlertState(alert, MonitoringAlertSpec_state.resolved, summary, msg);
    $event.stopPropagation();
  }

  acknowledgeAlert(alert: MonitoringAlert, $event) {
    const summary = 'Alert Acknowledged';
    const msg = 'Marked alert as acknowledged';
    this.updateAlertState(alert, MonitoringAlertSpec_state.acknowledged, summary, msg);
    $event.stopPropagation();
  }

  openAlert(alert: MonitoringAlert, $event) {
    const summary = 'Alert Opened';
    const msg = 'Marked alert as open';
    this.updateAlertState(alert, MonitoringAlertSpec_state.open, summary, msg);
    $event.stopPropagation();
  }

  filterAlerts() {
    this.dataObjects = this.alerts.filter((item) => {
      // Checking severity filter
      if (this.currentAlertSeverityFilter != null && item.status.severity !== this.currentAlertSeverityFilter) {
        return false;
      }
      // checking state filter
      return this.selectedStateFilters.includes(MonitoringAlertSpec_state_uihint[item.spec.state]);
    });
    this.alertsLoading = false;
    this.refreshGui(this.cdr);
  }

  /**
   * This api serves html template
   */
  showBatchResolveIcon(): boolean {
    // we want selected alerts all NOT in RESOLVED state
    return this.showBatchIconHelper(MonitoringAlertSpec_state.resolved, true);
  }

  /**
 * This api serves html template
 */
  showBatchAcknowLedgeIcon(): boolean {
    // we want selected alerts all NOT in ACKNOWLEDGED state
    return this.showBatchIconHelper(MonitoringAlertSpec_state.acknowledged, true);
  }

  /**
   * This api serves html template
   */
  showBatchOpenIcon(): boolean {
    // we want selected alerts all NOT in open state
    return this.showBatchIconHelper(MonitoringAlertSpec_state.open, true);
  }

  invokeUpdateAlerts(newState: MonitoringAlertSpec_state, summary: string, successMsg: string) {
   // this.updateAlertOneByOne(newState, summary, msg);
   const failureMsg: string = 'Failed to update alerts';
   this.updateAlertBulkEdit (newState, successMsg, failureMsg );
  }

  updateAlertOneByOne(newState: MonitoringAlertSpec_state, summary: string, msg: string) {
    const observables = this.buildObservableList(newState);
    this.updateAlertListForkJoin(observables, summary, msg);
  }

  updateAlertBulkEdit(newState: MonitoringAlertSpec_state, summary: string, msg: string) {
    const alerts = this.getSelectedDataObjects();
    const successMsg: string = 'Updated ' + alerts.length + ' alerts';
    const failureMsg: string = 'Failed to update alerts';
    const stagingBulkEditAction = this.buildBulkEditUpdateAlertsPayload(alerts, newState);
    this.alertsLoading = true;
    this.bulkEditHelper(alerts, stagingBulkEditAction, successMsg, failureMsg);
    this.refreshGui(this.cdr);
  }

  onBulkEditSuccess(veniceObjects: any[], stagingBulkEditAction: IStagingBulkEditAction, successMsg: string, failureMsg: string) {
    this.alertsLoading = false;
    this.invokeTimeRangeValidator();
    this.refreshGui(this.cdr);
  }

  onBulkEditFailure(error: Error, veniceObjects: any[], stagingBulkEditAction: IStagingBulkEditAction, successMsg: string, failureMsg: string, ) {
    this.alertsLoading = false;
    this.dataObjects = [...this.dataObjectsBackUp];
    this.refreshGui(this.cdr);
  }

  buildBulkEditUpdateAlertsPayload(updateAlerts: MonitoringAlert[], newState: MonitoringAlertSpec_state, buffername: string = ''): IStagingBulkEditAction {

    const stagingBulkEditAction: IStagingBulkEditAction = Utility.buildStagingBulkEditAction(buffername);
    stagingBulkEditAction.spec.items = [];

    for (const alert of updateAlerts) {
      alert.spec.state = newState;
      const obj = {
        uri: '',
        method: 'update',
        object: alert.getModelValues()
      };
      stagingBulkEditAction.spec.items.push(obj as IBulkeditBulkEditItem);
    }
    return (stagingBulkEditAction.spec.items.length > 0) ? stagingBulkEditAction : null;
  }

  /**
 * This api serves html template
 */
  resolveSelectedAlerts() {
    const summary = 'Alerts Resolved';
    const msg = 'Marked selected alerts as resolved';
    const newState = MonitoringAlertSpec_state.resolved;
    this.invokeUpdateAlerts(newState, summary, msg);
  }

  /**
   * This api serves html template
   */
  acknowledgeSelectedAlerts() {
    const summary = 'Alerts Acknowledged';
    const msg = 'Marked selected alerts as acknowledged';
    const newState = MonitoringAlertSpec_state.acknowledged;
    this.invokeUpdateAlerts(newState, summary, msg);
  }

  /**
   * This api serves html template
   */
  openSelectedAlerts() {
    const summary = 'Alerts Opened';
    const msg = 'Marked selected alerts as open';
    const newState = MonitoringAlertSpec_state.open;
    this.invokeUpdateAlerts(newState, summary, msg);
  }

  /**
 * This API serves html template.
 * It will filter events displayed in table
 */
  onAlertNumberClick(severityType: string) {
    if (this.currentAlertSeverityFilter === severityType) {
      this.currentAlertSeverityFilter = null;
    } else {
      this.currentAlertSeverityFilter = severityType;
    }
    this.filterAlerts();
  }

  getAlertSourceNameLink(rowData: MonitoringAlert): string {
    return Utility.getAlertSourceLink(rowData);
  }

  getAlertSourceNameTooltip(rowData: MonitoringAlert): string {
    const cat = rowData.status['object-ref'].kind;
    return 'Go to ' + (cat ? cat.toLowerCase() + ' ' : '') + 'details page';
  }

  setAlertsTimeRange(timeRange: TimeRange) {
    // update query and call getEvents
    setTimeout(() => {
      if (this.timeRangeComponent.allowTimeRangeEvent) {
        this.alertsSelectedTimeRange = timeRange;
        const start = this.alertsSelectedTimeRange.getTime().startTime.toISOString() as any;
        const end = this.alertsSelectedTimeRange.getTime().endTime.toISOString() as any;
        this.alertsTimeConstraints = 'meta.mod-time<' + end + ',' + 'meta.mod-time>' + start;
        this.clearAlertNumbersObject();
        this.genQueryBodies();
        this.getAlerts();
      }
    }, 0);
  }

  displayColumn(data: MonitoringAlert, col: TableCol): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(data, fields);
    return Array.isArray(value) ? JSON.stringify(value, null, 2) : value;
  }

  /**
 * Submits an HTTP request to update the state of the alert
 * @param alert Alert to resolve
 */
  updateAlertState(alert: MonitoringAlert, newState: MonitoringAlertSpec_state, summary: string, msg: string) {
    // Create copy so that when we modify it doesn't change the view
    this.alertsLoading = true;
    this.refreshGui(this.cdr);
    const observable = this.buildUpdateAlertStateObservable(alert, newState);
    const subscription = observable.subscribe(
      response => {
        this._controllerService.invokeSuccessToaster(summary, msg);
        this.alertsLoading = false;
        this.invokeTimeRangeValidator(); // Gets new time and update table accordingly. Avoids using Stale Time Query
        this.refreshGui(this.cdr);
      },
      () => {
        this.alertsLoading = false;
        this._controllerService.restErrorHandler(summary + ' Failed');
        this.refreshGui(this.cdr);
      }
    );
    this.subscriptions.push(subscription);
  }

  invokeTimeRangeValidator() {
    const timeValidate = this.timeRangeComponent.timeRangeGroupValidator();
    const err: ValidationErrors = timeValidate(this.timeRangeComponent.timeFormGroup);
    if (err !== null) {
      this._controllerService.invokeErrorToaster('Time Range Error', err.message);
    }
  }

  buildUpdateAlertStateObservable(alert: MonitoringAlert, newState: MonitoringAlertSpec_state): Observable<any> {
    const payload = new MonitoringAlert(alert);
    payload.spec.state = newState;
    const observable = this.monitoringService.UpdateAlert(payload.meta.name, payload);
    return observable;
  }

  /**
   * This is a helper function
   * @param state
   * @param reversed
   * this.showBatchIconHelper(MonitoringAlertSpec_state.RESOLVED, true);
   *      means that we want selected alerts all NOT in RESOLVED state
   * this.showBatchIconHelper(MonitoringAlertSpec_state.open, false);
   *      means that we want selected alerts all in open state
   */
  showBatchIconHelper(state: MonitoringAlertSpec_state, reversed: boolean = true): boolean {
    const alerts = this.getSelectedDataObjects();
    if (!this.alertUpdatable || !alerts || alerts.length === 0) {
      return false;
    }
    for (let i = 0; i < alerts.length; i++) {
      const alert = alerts[i];
      if (!reversed) {
        if (alert.spec.state !== state) {
          return false;
        }
      } else {
        if (alert.spec.state === state) {
          return false;
        }
      }
    }
    return true;
  }

  updateAlertListForkJoin(observables: Observable<any>[], summary: string, msg: string) {
    if (observables.length <= 0) {
      return;
    }
    forkJoin(observables).subscribe(
      (results) => {
        const isAllOK = Utility.isForkjoinResultAllOK(results);
        if (isAllOK) {
          this._controllerService.invokeSuccessToaster(summary, msg);
          this.clearSelectedDataObjects();
          this.invokeTimeRangeValidator();
        } else {
          const error = Utility.joinErrors(results);
          this._controllerService.invokeRESTErrorToaster(summary, error);
        }
      },
      this._controllerService.restErrorHandler(summary + ' Failed')

    );
  }

  buildObservableList(newState: MonitoringAlertSpec_state): Observable<any>[] {
    const observables = [];
    const alerts = this.getSelectedDataObjects();
    for (let i = 0; alerts && i < alerts.length; i++) {
      const observable = this.buildUpdateAlertStateObservable(alerts[i], newState);
      observables.push(observable);
    }
    return observables;
  }

  genQueryBodies() {
    const fieldSelectorOptions = [];

    if (this.selector != null) {
      fieldSelectorOptions.push(this.selector.alertSelector.selector);
    }
    if (this.alertsTimeConstraints.length) {
      fieldSelectorOptions.push(this.alertsTimeConstraints);
    }

    this.alertQuery['field-selector'] = fieldSelectorOptions.join(',');
  }

  selectAlert($event) {
    if (this.selectedAlert && $event.rowData.meta.name === this.selectedAlert.meta.name) {
      this.selectedAlert = null;
    } else {
      this.selectedAlert = $event.rowData;
    }
  }


  getEventNameFromURI(uri) {
    const split_uri = uri.split('/');
    return split_uri[split_uri.length - 1];
  }

  ngOnChanges(change: SimpleChanges) {
    if (change.selector) {
      this.genQueryBodies();
      this.getAlerts();
    }
  }

  onCancelSearch() {
    this.controllerService.invokeInfoToaster('Information', 'Cleared search criteria, Table refreshed.');
    this.alerts = this.dataObjectsBackUp;
    this.setAlertNumbersObject();
    this.filterAlerts();
  }

  /**
   * Execute table search
   * @param field
   * @param order
   */
  onSearchAlerts(field = this.alertsTable.sortField, order = this.alertsTable.sortOrder) {
    this.alertsLoading = true;
    this.refreshGui(this.cdr);
    const searchResults = this.onSearchDataObjects(field, order, 'Alert', this.maxSearchRecords, this.advSearchCols, this.dataObjectsBackUp, this.alertsTable.advancedSearchComponent);
    if (searchResults && searchResults.length > 0) {
      this.alerts = searchResults;
      this.setAlertNumbersObject();
      this.filterAlerts();
    } else {
      this.alertsLoading = false;
      this.refreshGui(this.cdr);
    }
  }

  getSourceNodeAndComponentField(alert: MonitoringAlert): string {
    let text: string = '';
    if (this.sourceFieldExists(alert)) {
      const nodename = alert.status.source['node-name'];
      const component = alert.status.source.component;
      text = ((nodename && component) ? nodename + ' : ' + component : (nodename ? nodename : (component ? component : '')));
    }
    return text;
  }

  sourceFieldExists(alert: MonitoringAlert) {
    return alert && alert.status && alert.status.source;
  }

}
