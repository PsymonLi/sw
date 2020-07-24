import { Component, ViewEncapsulation, OnInit, ViewChild, OnChanges } from '@angular/core';
import { Animations } from '@app/animations';
import { Utility } from '@app/common/Utility';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { IApiStatus, IMonitoringArchiveRequest, MonitoringArchiveRequest, MonitoringArchiveRequestStatus_status, IMonitoringArchiveRequestStatus, IMonitoringCancelArchiveRequest } from '@sdk/v1/models/generated/monitoring';
import { Observable } from 'rxjs';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { TableCol, RowClickEvent, CustomExportMap } from '@app/components/shared/tableviewedit';
import { TableUtility } from '@app/components/shared/tableviewedit/tableutility';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { Eventtypes } from '@app/enum/eventtypes.enum';


@Component({
  selector: 'app-archivelog',
  templateUrl: './archivelog.component.html',
  styleUrls: ['./archivelog.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None
})

export class ArchivelogComponent extends DataComponent implements OnInit {
  public static AL_DOWNLOAD = 'archivelogsdownload'; // Will contain URL for archive request download

  @ViewChild('archiveTable') archiveTable: PentableComponent;

  dataObjects: MonitoringArchiveRequest[] = [];
  dataObjectsBackUp: MonitoringArchiveRequest[] = [];

  bodyicon: any = {
    margin: {
      top: '9px',
      left: '8px',
    },
    url: '/assets/images/icons/monitoring/ico-arch-log-b.svg',
  };

  headerIcon: Icon = {
    margin: {
      top: '0px',
      left: '10px',
    },
    matIcon: 'grid_on'
  };

  cols: TableCol[] = [
    { field: 'meta.name', header: 'Name', class: 'archiverequests-column-name', sortable: true, width: 12, notReorderable: true },
    { field: 'meta.creation-time', header: 'Creation Time', class: 'archiverequests-column-date', sortable: true, width: 12, notReorderable: true },
    { field: 'spec.type', header: 'Logs Type', class: 'archiverequests-column-log-type', sortable: true, width: 10 },
    { field: 'spec.query.start-time', header: 'Logs From', class: 'archiverequests-column-date', sortable: true, width: 12},
    { field: 'spec.query.end-time', header: 'Logs Till', class: 'archiverequests-column-date', sortable: true, width: 12},
    { field: 'status.status', header: 'Status', class: 'archiverequests-column-status_status', sortable: true }
  ];

  exportFilename: string = 'PSM-archive-logs-requests';
  exportMap: CustomExportMap = {};

  isTabComponent = false;
  disableTableWhenRowExpanded = true;
  tableLoading: boolean = false;

  constructor(protected controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected monitoringService: MonitoringService) {
    super(controllerService, uiconfigsService);
  }

  getSelectedDataObjects() {
    return this.archiveTable.selectedDataObjects;
  }

  clearSelectedDataObjects() {
    this.archiveTable.selectedDataObjects = [];
  }

  /**
  * Overide super's API
  * It will return this Component name
  */
  getClassName(): string {
    return this.constructor.name;
  }

  ngOnInit() {
    this._controllerService.publish(Eventtypes.COMPONENT_INIT, {
      'component': this.getClassName(), 'state': Eventtypes.COMPONENT_INIT
    });
    this.setDefaultToolbar();
    this.getArchiveRequests();
  }

  setDefaultToolbar() {
    const buttons = [];
    this.controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [{ label: 'Archive Logs', url: Utility.getBaseUIUrl() + 'monitoring/archive' }]
    });
  }

  /**
   * List and Watch Archive Requests
   */
  getArchiveRequests() {
    this.tableLoading = true;
    const sub = this.monitoringService.ListArchiveRequestCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        const archiveRequests = response.data as MonitoringArchiveRequest[];
        this.tableLoading = false;
        this.dataObjects = [...archiveRequests];
        this.dataObjectsBackUp = [...archiveRequests];
      },
      error => {
        this.tableLoading = false;
        this._controllerService.invokeRESTErrorToaster(Utility.UPDATE_FAILED_SUMMARY, error);
      }
    );
    this.subscriptions.push(sub);
  }

  displayArchiveRequest(rowData): string {
    return JSON.stringify(Utility.trimUIFields(rowData.spec.query), null, 4);
  }

  /**
   * Handle logics when user click the row
   * @param event
   */
  showArchiveQuery(event: RowClickEvent) {
    this.expandRowRequest(event.event, event.rowData);
  }

  expandRowRequest(event, rowData) {
    if (!this.archiveTable.showRowExpand) {
      this.archiveTable.toggleRow(rowData, event);
    }
  }

  editFormClose(rowData) {
    if (this.archiveTable.showRowExpand) {
      this.archiveTable.toggleRow(rowData);
    }
  }

  /**
   * Cancel Running Archive Request
   * @param event
   * @param object
   */
  onCancelRecord(event, object: MonitoringArchiveRequest) {
    if (event) {
      event.stopPropagation();
    }
    // Should not be able to cancel any record while we are editing
    if (this.archiveTable.showRowExpand) {
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

  displayColumn(data, col): any {
    return TableUtility.displayColumn(data, col);
  }

  isArchiveCompleted(rowData: MonitoringArchiveRequest, col): boolean {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(rowData, fields);
    const isComplete = (value === MonitoringArchiveRequestStatus_status.completed);
    if (isComplete && !rowData[ArchivelogComponent.AL_DOWNLOAD]) {
      // when AL.status is complete, we build UI model to list available download links. We build only once
      this.buildALDownload(rowData);
    }
    return isComplete;
  }

  timeoutOrFailure(rowData: MonitoringArchiveRequest): boolean {
    return (this.isALFailure(rowData) || this.isALTimeout(rowData));
  }

  isALFailure(rowData: MonitoringArchiveRequest): boolean {
    return (rowData.status.status === MonitoringArchiveRequestStatus_status.failed) ? true : false;
  }

  isALTimeout(rowData: MonitoringArchiveRequest): boolean {
    return (rowData.status.status === MonitoringArchiveRequestStatus_status.timeout) ? true : false;
  }

  displayMessage(rowData: MonitoringArchiveRequest): any {
    const status: IMonitoringArchiveRequestStatus = rowData.status;
    return (status != null && this.timeoutOrFailure(rowData) && status.reason !== undefined && status.reason !== null) ? status.reason : '';
  }


  /**
   * Builds Archive Request Download URL
   *
   * Archive Request server response JSON object.status looks like:
   * {
   *  "status": "completed",
   *  "uri": "/objstore/v1/downloads/auditevents/xxxx.gz"
   * }
  */

  buildALDownload(rowData: MonitoringArchiveRequest) {
    const status: IMonitoringArchiveRequestStatus = rowData.status;
    if (status.uri !== undefined && status.uri !== null) {
      rowData._ui[ArchivelogComponent.AL_DOWNLOAD] = status.uri;
    } else {
      rowData._ui[ArchivelogComponent.AL_DOWNLOAD] = '';
    }
  }

  triggerBuildDownloadZip(rowData: MonitoringArchiveRequest): boolean {
    this.buildALDownload(rowData);
    return true;
  }

  displayColumn_status(fields, value): string {
    return (value === MonitoringArchiveRequestStatus_status.completed) ? '' : value;
  }

  deleteRecord(object: MonitoringArchiveRequest): Observable<{ body: IMonitoringArchiveRequest | IApiStatus | Error, statusCode: number }> {
    return this.monitoringService.DeleteArchiveRequest(object.meta.name);
  }

  generateDeleteConfirmMsg(object: IMonitoringArchiveRequest) {
    return 'Are you sure to delete archive request: ' + object.meta.name;
  }

  generateDeleteSuccessMsg(object: IMonitoringArchiveRequest) {
    return 'Deleted archive request ' + object.meta.name;
  }

  showCancelIcon(rowData: MonitoringArchiveRequest): boolean {
    return (rowData.status.status === MonitoringArchiveRequestStatus_status.running);
  }

  /**
   * This API serves html template
   * @param rowData
   */
  showDeleteIcon(rowData: MonitoringArchiveRequest): boolean {
    // when the status is null, the user shouldn't be able to perform any action (cancel/delete)
    return (rowData.status.status !== MonitoringArchiveRequestStatus_status.running && rowData.status.status !== null);
  }

  onInvokeAPIonMultipleRecordsSuccess() {
    this.tableLoading = false;
  }

  onInvokeAPIonMultipleRecordsFailure() {
    this.tableLoading = false;
    this.dataObjects = Utility.getLodash().cloneDeep(this.dataObjectsBackUp);
  }

  showBulkDeleteIcon(): boolean {
    const selected = this.getSelectedDataObjects();
    for (const archReq of selected) {
      if (!this.showDeleteIcon(archReq)) {
        return false;
      }
    }
    return true;
  }

  deleteMultipleRecords() {
    const selected = this.getSelectedDataObjects();
    const observables: Observable<any>[] = [];
    for (const archReq of selected) {
      const sub = this.deleteRecord(archReq);
      observables.push(sub);
    }
    if (observables.length > 0) {
      const allSuccessSummary = 'Delete';
      const partialSuccessSummary = 'Partially Delete';
      const msg = `Deleted ${selected.length} Selected ${selected.length === 1 ? 'Record' : 'Records'}.`;
      this.invokeAPIonMultipleRecords(observables, allSuccessSummary, partialSuccessSummary, msg);
    }
  }

  handleDelete() {
    const selected = this.getSelectedDataObjects();
    if (selected.length <= 0) {
      return;
    }
    this.controllerService.invokeConfirm({
      header: `Delete ${selected.length} selected ${selected.length === 1 ? 'record' : 'records'}?`,
      message: 'This action cannot be reversed',
      acceptLabel: 'Delete',
      accept: () => {
        this.tableLoading = true;
        this.deleteMultipleRecords();
      }
    });
  }

  onDeleteRecord(event, object) {
    this.archiveTable.onDeleteRecord(
      event,
      object,
      this.generateDeleteConfirmMsg(object),
      this.generateDeleteSuccessMsg(object),
      this.deleteRecord.bind(this)
    );
  }

  onColumnSelectChange(event) {
    this.archiveTable.onColumnSelectChange(event);
  }

  onDestroyHook() {
    this._controllerService.publish(Eventtypes.COMPONENT_DESTROY, {
      'component': this.getClassName(), 'state': Eventtypes.COMPONENT_DESTROY
    });
  }

}
