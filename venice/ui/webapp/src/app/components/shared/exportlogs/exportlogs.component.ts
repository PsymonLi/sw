import { Component, ViewEncapsulation, Input, Output, EventEmitter, ChangeDetectionStrategy, ChangeDetectorRef } from '@angular/core';
import { Utility } from '@app/common/Utility';
import { ControllerService } from '@app/services/controller.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { SecurityService } from '@app/services/generated/security.service';
import { IMonitoringArchiveQuery, MonitoringArchiveRequest, IMonitoringArchiveRequest, IApiStatus} from '@sdk/v1/models/generated/monitoring';
import { Observable } from 'rxjs';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { TimeRange } from '../timerange/utility';
import { CreationForm } from '@app/components/shared/tableviewedit/tableviewedit.component';

@Component({
  selector: 'app-exportlogs',
  templateUrl: './exportlogs.component.html',
  styleUrls: ['./exportlogs.component.scss'],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush,
})

export class ExportLogsComponent extends CreationForm<IMonitoringArchiveRequest, MonitoringArchiveRequest> {

  @Input() archiveQuery: IMonitoringArchiveQuery = {};
  @Input() queryType: string = 'AuditEvent';
  @Output() archiveQueryEmitter: EventEmitter<any> = new EventEmitter();

  validationMessage: string;
  archiveRequestsEventUtility: HttpEventUtility<MonitoringArchiveRequest>;
  exportedArchiveRequests: ReadonlyArray<MonitoringArchiveRequest>;
  watchQuery = {};
  archiveRequestTimeRange: TimeRange;
  showTimerange: boolean = false;
  displayQuery: boolean = false;
  archiveQueryFormatted: string = '';
  archiveSearchRequest: MonitoringArchiveRequest;

  constructor(protected controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected monitoringService: MonitoringService,
    protected securityService: SecurityService,
    protected cdr: ChangeDetectorRef) {
    super(controllerService, uiconfigsService, cdr, MonitoringArchiveRequest);
  }

  postNgInit(): void {
    this.genFormattedJSON();
  }

  genFormattedJSON() {
    if ((Object.keys(this.archiveQuery).length === 0) || (this.archiveQuery.fields !== undefined && this.archiveQuery.fields !== null && this.archiveQuery.fields.requirements !== undefined && this.archiveQuery.fields.requirements !== null && this.archiveQuery.fields.requirements.length === 0 && this.archiveQuery.labels.requirements.length === 0 && this.archiveQuery.texts.length === 1 && this.archiveQuery.texts[0].text.length === 0)) {
      this.archiveQueryFormatted = 'No Query Specified!';
    } else {
      this.archiveQueryFormatted = JSON.stringify(Utility.trimUIFields(this.archiveQuery), null, 1);
    }
  }

  setToolbar(): void {
    this.setCreationButtonsToolbar('CREATE ARCHIVE', null);
  }

  getArchiveTimeRange(timeRange) {
    setTimeout(() => {
      this.archiveRequestTimeRange = timeRange;
    }, 0);
  }

  showArchiveQuery() {
    this.displayQuery = true;
  }

  hideArchiveQuery() {
    this.displayQuery = false;
  }

  createObject() {
    const archiveSearchRequest: MonitoringArchiveRequest = new MonitoringArchiveRequest(null, false);
    const archiveSearchRequestJSON = archiveSearchRequest.getFormGroupValues();
    const archiveFormObj = this.newObject.getFormGroupValues();

    // Send start-time and end-time when user has specified them. User can specify timerange, use default 'Past Month' Setting or query for all by not specifying any timerange
    if (this.showTimerange) {
      if (this.archiveRequestTimeRange.startTime !== null) {
        this.archiveQuery['start-time'] =  this.archiveRequestTimeRange.getTime().startTime.toISOString() as any;
      }
      if (this.archiveRequestTimeRange.endTime !== null) {
        this.archiveQuery['end-time'] = this.archiveRequestTimeRange.getTime().endTime.toISOString() as any;
      }
    }

    archiveSearchRequestJSON.kind = 'ArchiveRequest';
    // archiveSearchRequestJSON.meta.name = startendtime.type + '_' + Utility.s4();
    archiveSearchRequestJSON.meta.name = archiveFormObj.meta.name;
    archiveSearchRequestJSON.spec = {
      'type': this.queryType,
      'query': this.archiveQuery
    };
    this.archiveSearchRequest = archiveSearchRequestJSON;
    return this.monitoringService.AddArchiveRequest(archiveSearchRequestJSON);
  }

  updateObject(newObject: IMonitoringArchiveRequest, oldObject: IMonitoringArchiveRequest): Observable<{body: IMonitoringArchiveRequest | IApiStatus | Error, statusCode: number}> {
    throw new Error('Method not supported');
  }

  generateCreateSuccessMsg(object: IMonitoringArchiveRequest): string {
    return 'Created archive request ' + object.meta.name;
  }

  generateUpdateSuccessMsg(object: IMonitoringArchiveRequest): string {
    return 'Updated archive request ' + object.meta.name;
  }

  isFormValid(): boolean {
    if (Utility.isEmpty(this.newObject.$formGroup.get(['meta', 'name']).value)) {
      this.submitButtonTooltip = 'Error: Archive File name is required.';
      return false;
    }
    this.submitButtonTooltip = 'Ready to submit';
    return true;
  }

  onSaveSuccess(isCreate: boolean) {
    if (isCreate === true) {
      this.archiveQueryEmitter.emit(this.archiveSearchRequest);
    }
  }

  onSaveFailure(isCreate: boolean) { }
}
