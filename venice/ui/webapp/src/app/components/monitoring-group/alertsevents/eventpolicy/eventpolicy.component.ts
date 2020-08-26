import { ChangeDetectorRef, Component, OnInit, ViewEncapsulation, DoCheck, IterableDiffer, IterableDiffers, ChangeDetectionStrategy, ViewChild } from '@angular/core';
import { Animations } from '@app/animations';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { Utility } from '@app/common/Utility';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { EventsEvent } from '@sdk/v1/models/generated/events';
import { FieldsRequirement, IApiStatus, IMonitoringEventPolicy, MonitoringEventPolicy, IMonitoringExportConfig } from '@sdk/v1/models/generated/monitoring';
import { Observable } from 'rxjs';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { TableCol, CustomExportMap } from '@app/components/shared/tableviewedit';
import { SyslogUtility } from '@app/common/SyslogUtility';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';

@Component({
  selector: 'app-eventpolicy',
  templateUrl: './eventpolicy.component.html',
  styleUrls: ['./eventpolicy.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class EventpolicyComponent extends DataComponent implements OnInit {
  public static MAX_TOTAL_TARGETS = 8;

  @ViewChild('eventPolicyTable', { static: true }) eventPolicyTable: PentableComponent;

  dataObjects: ReadonlyArray<MonitoringEventPolicy> = [];

  bodyIcon: Icon = {
    margin: {
      top: '8px',
      left: '10px',
    },
    matIcon: 'send'
  };

  headerIcon: Icon = {
    margin: {
      top: '0px',
      left: '10px',
    },
    matIcon: 'grid_on'
  };

  // Used for the table - when true there is a loading icon displayed
  tableLoading: boolean = false;

  totalTargets: number = 0;
  maxNewTargets: number = EventpolicyComponent.MAX_TOTAL_TARGETS;

  cols: TableCol[] = [
    { field: 'meta.name', header: 'Name', class: 'eventpolicy-column-name', sortable: true, width: 25 },
    { field: 'spec', header: 'Syslog Exports', class: 'eventpolicy-column-syslog', sortable: false, width: 25 },
    // Commenting out as it is not currently supported by backend
    // { field: 'spec.selector', header: 'Filters', class: 'eventpolicy-column-name', sortable: false, width: 30 },
    { field: 'spec.targets', header: 'Targets', class: 'eventpolicy-column-targets-destination', sortable: false, width: 25 },
    // { field: 'spec.targets', header: 'Gateway', class: 'eventpolicy-column-target-gateway', sortable: false, width: 25 },
    // { field: 'spec.targets.transport', header: 'Transport', class: 'eventpolicy-column-transport', sortable: false, width: 25 }
  ];

  exportFilename: string = 'PSM-event-policies';
  exportMap: CustomExportMap = {
    'spec': (opts): string => {
      const value = Utility.getObjectValueByPropertyPath(opts.data, opts.field);
      const trimmedValues = Utility.trimUIFields(value);
      const resArr =  SyslogUtility.formatSyslogExports(trimmedValues);
      return resArr.toString();
    },
    'spec.targets': (opts): string => {
      const value = Utility.getObjectValueByPropertyPath(opts.data, opts.field);
      const resArr =  SyslogUtility.formatTargets(value, true);
      return resArr.toString();
    }
  };

  isTabComponent = false;
  disableTableWhenRowExpanded = true;


  constructor(protected controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected cdr: ChangeDetectorRef,
    protected monitoringService: MonitoringService,
    protected _iterableDiffers: IterableDiffers) {
    super(controllerService, uiconfigsService);
  }

  ngOnInit() {
    super.ngOnInit();
    this.penTable = this.eventPolicyTable;
    this.tableLoading = true;
    this.getEventPolicy();
  }

  getEventPolicy() {
    const dscSubscription = this.monitoringService.ListEventPolicyCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.tableLoading = false;
        this.dataObjects = response.data as MonitoringEventPolicy[];
        this.computeTargets();
        this.refreshGui(this.cdr);
      }
    );
    this.subscriptions.push(dscSubscription);
  }

  setDefaultToolbar() {
    let buttons = [];
    if (this.uiconfigsService.isAuthorized(UIRolePermissions.monitoringeventpolicy_create)) {
      buttons = [{
        cssClass: 'global-button-primary eventpolicy-button',
        text: 'ADD EVENT POLICY',
        genTooltip: () => this.getTooltip(),
        computeClass: () => !this.penTable.showRowExpand &&
          this.totalTargets < EventpolicyComponent.MAX_TOTAL_TARGETS ?
          '' : 'global-button-disabled',
        callback: () => { this.penTable.createNewObject(); }
      }];
    }
    this.controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [
        { label: 'Alerts & Events', url: Utility.getBaseUIUrl() + 'monitoring/alertsevents' },
        { label: 'Event Policy', url: Utility.getBaseUIUrl() + 'monitoring/alertsevents/eventpolicy' }
      ]
    });
  }

  getTooltip(): string {
    return this.maxNewTargets === 0 ? 'Cannot exceed 8 targets across policies' : '';
  }

  computeTargets() {
    let totaltargets: number = 0;
    for (const policy of this.dataObjects) {
      if (policy.spec.targets !== null) {
        totaltargets += policy.spec.targets.length;
      }
    }
    this.totalTargets = totaltargets;
    const remainder = EventpolicyComponent.MAX_TOTAL_TARGETS - totaltargets;
    this.maxNewTargets = remainder < 0 ? 0 : remainder;
  }

  displayColumn(exportData, col): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(exportData, fields);
    const column = col.field;
    switch (column) {
      case 'spec':
        return SyslogUtility.formatSyslogExports(value);
      case 'spec.email-list':
        return JSON.stringify(value, null, 2);
      case 'spec.snmp-trap-servers':
        return JSON.stringify(value, null, 2);
      case 'spec.targets':
        return SyslogUtility.formatTargets(value);
      default:
        return Array.isArray(value) ? JSON.stringify(value, null, 2) : value;
    }
  }

  formatRequirements(data: FieldsRequirement[]) {
    if (data == null) {
      return '';
    }
    const retArr = [];
    data.forEach((req) => {
      let ret = '';
      ret += req['key'] + ' ';
      ret += Utility.getFieldOperatorSymbol(req.operator);

      ret += ' ';

      if (req.values != null) {
        let values = [];
        const enumInfo = Utility.getNestedPropInfo(new EventsEvent(), req.key).enum;
        values = req.values.map((item) => {
          if (enumInfo != null && [item] != null) {
            return enumInfo[item];
          }
          return item;
        });
        ret += values.join(' or ');
      }
      retArr.push(ret);
    });
    return retArr;
  }

  deleteRecord(object: MonitoringEventPolicy): Observable<{ body: IMonitoringEventPolicy | IApiStatus | Error, statusCode: number }> {
    return this.monitoringService.DeleteEventPolicy(object.meta.name);
  }

}
