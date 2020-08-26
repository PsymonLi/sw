import { Component, OnInit, Input, ChangeDetectorRef, SimpleChanges, OnChanges, OnDestroy, ViewEncapsulation, ViewChild, IterableDiffer, IterableDiffers, AfterViewInit, DoCheck, ChangeDetectionStrategy } from '@angular/core';
import { Observable, forkJoin, throwError, Subscription } from 'rxjs';
import { EventsEvent_severity, EventsEventAttributes_severity, IApiListWatchOptions, IEventsEvent, EventsEvent, ApiListWatchOptions_sort_order, EventsEventList } from '@sdk/v1/models/generated/events';
import { TableCol, CustomExportMap } from '@app/components/shared/tableviewedit';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { TimeRange } from '@app/components/shared/timerange/utility';
import { EventsService } from '@app/services/events.service';
import { FormControl, FormArray } from '@angular/forms';
import { FieldsRequirement, FieldsRequirement_operator, ISearchSearchResponse, SearchSearchRequest, SearchTextRequirement, SearchSearchResponse } from '@sdk/v1/models/generated/search';
import { SearchService } from '@app/services/generated/search.service';
import { Utility } from '@app/common/Utility';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { ControllerService } from '@app/services/controller.service';
import { debounceTime, distinctUntilChanged, switchMap, first } from 'rxjs/operators';
import { AlertsEventsSelector } from '@app/components/shared/alertsevents/alertsevents.component';
import { IApiStatus } from '@sdk/v1/models/generated/search';
import { Animations } from '@app/animations';
import { IMonitoringArchiveQuery, MonitoringArchiveRequest, MonitoringArchiveRequestStatus_status, IMonitoringCancelArchiveRequest } from '@sdk/v1/models/generated/monitoring';
import { ExportLogsComponent } from '../../exportlogs/exportlogs.component';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { TimeRangeComponent } from '@app/components/shared/timerange/timerange.component';
import { PentableComponent } from '../../pentable/pentable.component';

@Component({
  selector: 'app-eventstable',
  templateUrl: './eventstable.component.html',
  styleUrls: ['./eventstable.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class EventstableComponent extends DataComponent implements OnInit, OnChanges, OnDestroy, DoCheck {
  @ViewChild('timeRangeComponent') timeRangeComponent: TimeRangeComponent;
  @ViewChild('exportLogsComponent') exportLogsComponent: ExportLogsComponent;
  @ViewChild('eventsTable', { static: true }) eventsTable: PentableComponent;

  isTabComponent: boolean = false;
  disableTableWhenRowExpanded: boolean = false;
  exportFilename: string = 'PSM-events';

  // Used as the key for uniquely identifying poll requests.
  // If there are going to be two of these components alive at the same time,
  // this field is required for them to have independent queries.
  @Input() pollingServiceKey: string = 'alertsevents';
  // If provided, will only show alerts and events
  // where the source node matches
  @Input() selector: AlertsEventsSelector;
  @Input() showEventsAdvSearch: boolean = false;
  @Input() searchedEvent: string;

  currentEventSeverityFilter = null;
  showDebugEvents: boolean = false;
  eventsSubscription: Subscription;
  eventSearchFormControl: FormControl = new FormControl();
  severityEnum = EventsEventAttributes_severity;
  fieldFormArray = new FormArray([]);
  eventArchiveQuery: IMonitoringArchiveQuery = {};
  watchArchiveSubscription: Subscription;

  // EVENTS
  // Used for the table - when true there is a loading icon displayed
  eventsLoading = false;

  // Holds all events
  events: ReadonlyArray<EventsEvent> = [];
  // Contains the total number of events.
  // Count does not include debug events if show debug events is not selected
  eventsTotalCount = 0;

  // Mapping from meta.name to event object. Used for mapping entries from elastic
  // to the event objects we have.
  eventMap = {};

  // holds a subset (possibly all) of this.events
  // This are the events that will be displayed
  dataObjects: EventsEvent[] = [];
  dataObjectsBackup: EventsEvent[] = [];

  eventsPostBody: IApiListWatchOptions = { 'sort-order': ApiListWatchOptions_sort_order.none };

  // we only fetch the past 1000 events, so we don't know if there are more in the
  // system for our search criteria
  eventLimit: number = 1000;


  // All columns are set as not sortable as it isn't currently supported
  // TODO: Support sorting columns
  cols: TableCol[] = [
    { field: 'severity', header: 'Severity', class: '', sortable: false, width: '100px' },
    { field: 'type', header: 'Type', class: '', sortable: true, width: '200px' },
    { field: 'message', header: 'Message', class: '', sortable: false, width: 180 },
    { field: 'object-ref', header: 'Object Ref', class: '', sortable: false, width: '150px' },
    { field: 'count', header: 'Count', class: '', sortable: false, width: '50px' },
    { field: 'source', header: 'Source Node & Component', class: '', sortable: false, width: 100},
    { field: 'meta.mod-time', header: 'Time', class: '', sortable: true, width: '180px' }
  ];

  // Will hold mapping from severity types to counts
  eventNumbers: { [severity in EventsEvent_severity]: number } = {
    'info': 0,
    'warn': 0,
    'critical': 0,
    'debug': 0
  };

  eventsIcon: Icon = {
    margin: {
      top: '0px',
      left: '0px',
    },
    matIcon: 'event'
  };

  // TimeRange for events and alerts
  eventsSelectedTimeRange: TimeRange;
  eventsTimeConstraints: string = '';

  exportMap: CustomExportMap = {
    'object-ref': (opts) => {
      return this.getObjectRefField(opts.data);
    },
    'source': (opts) => {
      return this.getSourceNodeAndComponentField(opts.data);
    }
  };

  selectedEvent: EventsEvent = null;
  maxSearchRecords: number = 4000;

  displayArchPanel: boolean = false;
  archiveRequestsEventUtility: HttpEventUtility<MonitoringArchiveRequest>;
  exportedArchiveRequests: ReadonlyArray<MonitoringArchiveRequest> = [];
  currentArchReqLength: number = 0;
  archiveStatusMsg: string = '';
  anyQueryAfterRefresh: boolean = false;
  enableExport: boolean = true;
  archiveQuery: MonitoringArchiveRequest;

  arrayDiffers: IterableDiffer<any>;
  requestStatus: MonitoringArchiveRequestStatus_status;
  requestName: string = '';
  firstElem: MonitoringArchiveRequest = null;
  advSearchCols: TableCol[] = [];

  constructor(protected eventsService: EventsService,
    protected searchService: SearchService,
    protected uiconfigsService: UIConfigsService,
    protected monitoringService: MonitoringService,
    protected controllerService: ControllerService,
    protected cdr: ChangeDetectorRef,
    protected iterableDiffers: IterableDiffers
    ) {
    super(controllerService, uiconfigsService);
    this.arrayDiffers = iterableDiffers.find([]).create(HttpEventUtility.trackBy);
  }

  ngOnInit() {
    super.ngOnInit();
    this.penTable = this.eventsTable;

    if (this.showEventsAdvSearch) {
      this.buildAdvSearchCols();
      this.watchArchiveObject();
    }
    this.genQueryBodies();
    // Disabling search to reduce scope for august release
    // Adding <any> to prevent typescript compilation from failing due to unreachable code
    if (<any>false) {
      // After user stops typing for 1 second, we invoke a search request to elastic
      const subscription =
        this.eventSearchFormControl.valueChanges.pipe(
          debounceTime(1000),
          distinctUntilChanged()
        ).subscribe(
          value => {
            this.invokeEventsSearch();
          }
        );
      this.subscriptions.push(subscription);
    }

    // If get alerts/events wasn't triggered by on change
    if (!this.eventsSubscription) {
      this.getEvents();
    }

    if (this.searchedEvent) {
      this.getSearchedEvent();
    }
  }

  buildAdvSearchCols() {
    this.advSearchCols = this.cols.filter((col: TableCol) => {
      return (col.field !== 'meta.mod-time' && col.field !== 'source' && col.field !== 'object-ref' );
    });

    this.advSearchCols.push(
      { field: 'object-ref.kind', header: 'Object-Ref Kind', kind: 'Event' },
      { field: 'object-ref.name', header: 'Object-Ref Name', kind: 'Event' },
      { field: 'source.node-name', header: 'Source Node', kind: 'Event' },
      { field: 'source.component', header: 'Source Component', kind: 'Event' }
    );
  }

  /**
   * User will only be allowed to fire only one archive Event request at a time
   * Show the status of the archive request in display panel
   * Allow user to download files on success or cancel when the request is running
   */
  ngDoCheck() {
    if (this.showEventsAdvSearch) {
      const changes = this.arrayDiffers.diff(this.exportedArchiveRequests);
      if (changes) {
        this.handleArchiveLogChange();
        this.refreshGui(this.cdr);
      }
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
            // disable the archive request button, show cancel
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

  /**
   * Cancel any Running Archive Request, TODO: Currently Cancel API not ready
   */
  onCancelRecord(event) {
    const object = this.firstElem;
    if (event) {
      event.stopPropagation();
    }
    // Should not be able to cancel any record while we are editing
    if (this.eventsTable.showRowExpand) {
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
   * Watch on Archive Event Requests
   * Show any running archive Event request when the user comes on this page
   */
  watchArchiveObject() {
    this.archiveRequestsEventUtility = new HttpEventUtility<MonitoringArchiveRequest>(MonitoringArchiveRequest);
    this.exportedArchiveRequests = this.archiveRequestsEventUtility.array;

    if (this.watchArchiveSubscription) {
      this.watchArchiveSubscription.unsubscribe();
    }
    this.watchArchiveSubscription = this.monitoringService.WatchArchiveRequest({ 'field-selector': 'spec.type=event' }).subscribe(
      (response) => {
        this.currentArchReqLength = this.exportedArchiveRequests.length;
        this.archiveRequestsEventUtility.processEvents(response);
        if (this.exportedArchiveRequests.length > 0 && this.exportedArchiveRequests[0].status.status === MonitoringArchiveRequestStatus_status.running) {
          this.enableExport = false;
          this.displayArchPanel = true;
        }
        this.refreshGui(this.cdr);
      },
      this.controllerService.webSocketErrorHandler('Failed to get Event Archive Requests')
    );
    this.subscriptions.push(this.watchArchiveSubscription);
  }

  getArchiveQuery(archQuer: IMonitoringArchiveQuery) {
    this.eventArchiveQuery = archQuer;
  }

  showArchiveStatusPanel() {
    this.displayArchPanel = true;
  }

  /**
   * This serves HTML API. It clear audit-event search and refresh data.
   * @param $event
   */
  onCancelSearch($event) {
    this.controllerService.invokeInfoToaster('Infomation', 'Cleared search criteria, events refreshed.');
    this.events = [...this.dataObjectsBackup];
    this.setEventNumbersObject();
    this.filterEvents();
  }

   /**
   * Execute table search
   * @param field
   * @param order
   */
  onSearchEvents(field = this.eventsTable.sortField, order = this.eventsTable.sortOrder) {
    this.eventsLoading = true;
    this.refreshGui(this.cdr);
    const searchResults = this.onSearchDataObjects(field, order, 'Event', this.maxSearchRecords, this.advSearchCols, this.dataObjectsBackup, this.eventsTable.advancedSearchComponent);
    if (searchResults && searchResults.length > 0) {
      this.events = searchResults;
      this.currentEventSeverityFilter = null;
      this.setEventNumbersObject();
      this.filterEvents();
    } else {
      this.eventsLoading = false;
      this.refreshGui(this.cdr);
    }
  }

  getSearchedEvent() {
    this.eventsService.GetGetEvent(this.searchedEvent).subscribe(
      (response) => {
        this.selectedEvent = response.body as EventsEvent;
      },
      (error) => {
        // User may tweak browser url and make invalid event name in the url, we will catch and throw error.
        this.selectedEvent = null;
        this.controllerService.invokeRESTErrorToaster('Failed to fetch event ' + this.searchedEvent, error);
      }
    );
  }

  closeDetails() {
    this.selectedEvent = null;
  }

  setEventNumbersObject() {
    // Reset counters
    Object.keys(EventsEvent_severity).forEach(severity => {
      this.eventNumbers[severity] = 0;
    });
    this.events.forEach(event => {
      this.eventNumbers[event.severity] += 1;
    });
  }

  getEvents() {
    this.eventsLoading = true;
    this.refreshGui(this.cdr);
    if (this.eventsSubscription) {
      this.eventsSubscription.unsubscribe();
      this.eventsService.pollingUtility.terminatePolling(this.pollingServiceKey, true);
    }
    this.eventsSubscription = this.eventsService.pollEvents(this.pollingServiceKey, this.eventsPostBody).subscribe(
      (data) => {
        this.eventsLoading = false; // turn off spinning circle whenever REST call responds
        if (this.showEventsAdvSearch) {
          this.eventsTable.clearSearch();
        }
        if (data == null) {
          this.refreshGui(this.cdr);
          return;
        }
        // Check that there is new data
        if (this.events.length === data.length) {
          // Both sets of data are empty
          if (this.events.length === 0) {
            this.refreshGui(this.cdr);
            return;
          }
        }
        this.events = data;
        this.dataObjectsBackup = Utility.getLodash().cloneDeepWith(this.events) as EventsEvent[];
        this.setEventNumbersObject();
        this.filterEvents();
      },
      (error) => {
        this.eventsLoading = false;
        this.controllerService.webSocketErrorHandler('Failed to get Events');
        this.refreshGui(this.cdr);
      }
    );
    this.subscriptions.push(this.eventsSubscription);
  }

  /**
   * This API serves html template.
   * It will filter events displayed in table
   */
  onEventNumberClick(event, severityType: string) {
    if (this.currentEventSeverityFilter === severityType) {
      this.currentEventSeverityFilter = null;
    } else {
      this.currentEventSeverityFilter = severityType;
    }
    // Disabling search to reduce scope for august release
    // Adding <any> to prevent typescript compilation from failing due to unreachable code
    if (<any>false) {
      // TODO: Add support for searching for events through elastic
      this.invokeEventsSearch();
    }
  }

  showDebugPressed() {
    this.filterEvents();
  }

  filterEvents() {
    this.dataObjects = (this.showDebugEvents ? this.events : this.events.filter(item => item.severity !== EventsEvent_severity.debug)) as EventsEvent[];
    this.eventsTotalCount = this.dataObjects.length;
    if (this.currentEventSeverityFilter != null) {
      this.dataObjects = this.dataObjects.filter(item => item.severity === this.currentEventSeverityFilter);
    }
    this.eventsLoading = false;
    this.refreshGui(this.cdr);
  }

  /**
   * Makes a request to elastic using the current filters
   * If there are no filters, the function sets filteredEvents
   * to be all events and returns.
   *
   * Unused for now to reduce scope for August release
   */
  invokeEventsSearch() {
    // If no text or severity filters, we set to all events
    // and skip making a query to elastic
    if (this.currentEventSeverityFilter == null &&
      (this.eventSearchFormControl.value == null ||
        this.eventSearchFormControl.value.trim().length === 0)) {
      this.dataObjects = this.events as EventsEvent[];
      this.refreshGui(this.cdr);
      return;
    }

    this.eventsLoading = true;
    this.refreshGui(this.cdr);
    const body = new SearchSearchRequest();
    body['max-results'] = 100;
    body.query.kinds = ['Event'];
    body['sort-by'] = 'meta.mod-time';

    if (this.currentEventSeverityFilter != null) {
      const requirement = new FieldsRequirement();
      requirement.key = 'severity';
      requirement.operator = FieldsRequirement_operator.equals;
      requirement.values = [this.currentEventSeverityFilter];
      body.query.fields.requirements = [requirement];
    }
    // Don't add search requirement if its empty space or not set
    if (this.eventSearchFormControl.value != null
      && this.eventSearchFormControl.value.trim().length !== 0) {
      const text = new SearchTextRequirement();
      const input = this.eventSearchFormControl.value.trim();
      if (input.split(' ').length > 1) {
        text.text = input.split(' ');
      } else {
        // adding wild card matching to the end as user
        // may not be done with search
        text.text = [input + '*'];
      }
      body.query.texts = [text];
    }

    this.searchService.PostQuery(body).subscribe(
      (data) => {
        const respBody = data.body as ISearchSearchResponse;
        this.dataObjects = [];
        if (respBody.entries != null) {
          // We reverse the entries to get the sorted order to be latest time first
          respBody.entries.reverse().forEach(entry => {
            const event = entry.object as IEventsEvent;
            const match = this.eventMap[event.meta.name];
            if (match != null) {
              this.dataObjects.push(match);
            }
          });
        }
        this.eventsLoading = false;
        this.dataObjects = [...this.dataObjects]; // VS-1576. Force table re-render
        this.refreshGui(this.cdr);
      },
      (error) => {
        this.controllerService.invokeRESTErrorToaster('Error', error);
        this.eventsLoading = false;
        this.filterEvents();
      }
    );
  }

  setEventsTimeRange(timeRange: TimeRange) {
    // update query and call getEvents
    setTimeout(() => {
      if (this.timeRangeComponent.allowTimeRangeEvent) {
        this.eventsSelectedTimeRange = timeRange;
        const start = this.eventsSelectedTimeRange.getTime().startTime.toISOString() as any;
        const end = this.eventsSelectedTimeRange.getTime().endTime.toISOString() as any;
        this.eventsTimeConstraints = 'meta.creation-time<' + end + ',' + 'meta.creation-time>' + start;
        this.genQueryBodies();
        this.getEvents();
      }
    }, 0);
  }

  genQueryBodies() {

    const fieldSelectorOptions = [];

    if (this.selector != null) {
     fieldSelectorOptions.push(this.selector.eventSelector.selector);
    }
    if (this.eventsTimeConstraints.length) {
      fieldSelectorOptions.push(this.eventsTimeConstraints);
    }

    this.eventsPostBody = {
      'field-selector': fieldSelectorOptions.join(','),
      'sort-order': ApiListWatchOptions_sort_order.none
    };
  }

  displayColumn(data, col): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(data, fields);
    return Array.isArray(value) ? JSON.stringify(value, null, 2) : value;
  }

  ngOnChanges(change: SimpleChanges) {
    if (change.selector) {
      this.genQueryBodies();
      this.getEvents();
    }
    if (change.searchedEvent && this.searchedEvent) {
      this.getSearchedEvent();
    }
  }

  OnDestroyHook() {
    if (this.eventsSubscription != null) {
      this.eventsSubscription.unsubscribe();
    }
  }

  selectEvent($event) {
    if ( this.selectedEvent && $event.rowData.meta.name === this.selectedEvent.meta.name ) {
      this.selectedEvent = null;
    } else {
      this.selectedEvent = $event.rowData;
    }
  }

  getObjectRefField(eve: EventsEvent): string {
    let text: string = '';
    if (eve && eve['object-ref']) {
      const kind = eve['object-ref'].kind;
      const oname = eve['object-ref'].name;
      text = ((kind && oname) ? kind + ' : ' + oname : (kind ? kind : (oname ? oname : '')));
    }
    return text;
  }

  getSourceNodeAndComponentField(eve: EventsEvent): string {
    let text: string = '';
    if (eve && eve.source) {
      const nodename = eve.source['node-name'];
      const component = eve.source.component;
      text = ((nodename && component) ? nodename + ' : ' + component : (nodename ? nodename : (component ? component : '')));
    }
    return text;
  }
}
