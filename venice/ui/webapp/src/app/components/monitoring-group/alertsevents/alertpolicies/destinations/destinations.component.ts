import { ChangeDetectorRef, Component, Input, ViewEncapsulation, DoCheck, OnInit, IterableDiffer, IterableDiffers, ChangeDetectionStrategy, ViewChild, SimpleChanges, OnChanges } from '@angular/core';
import { Animations } from '@app/animations';
import { Utility } from '@app/common/Utility';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { IApiStatus, IMonitoringAlertDestination, MonitoringAlertDestination, MonitoringAlertPolicy, IMonitoringExportConfig } from '@sdk/v1/models/generated/monitoring';
import { Observable } from 'rxjs';
import { TablevieweditAbstract } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { TableCol, CustomExportMap } from '@app/components/shared/tableviewedit';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { SyslogUtility } from '@app/common/SyslogUtility';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';

@Component({
  selector: 'app-destinations',
  templateUrl: './destinations.component.html',
  styleUrls: ['./destinations.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class DestinationpolicyComponent extends DataComponent implements OnInit, OnChanges {
  public static MAX_TOTAL_TARGETS = 8;

  @ViewChild('alertDestinationTable') alertDestinationTable: PentableComponent;

  @Input() dataObjects: MonitoringAlertDestination[] = [];
  @Input() eventPolices: MonitoringAlertPolicy[] = [];

  isTabComponent = true;
  disableTableWhenRowExpanded = true;
  tableLoading = false;

  totalTargets: number = 0;
  maxNewTargets: number = DestinationpolicyComponent.MAX_TOTAL_TARGETS;

  headerIcon: Icon = {
    margin: {
      top: '0px',
      left: '10px'
    },
    matIcon: 'send'
  };

  cols: TableCol[] = [
    { field: 'meta.name', header: 'Policy Name', class: 'destinations-column-name', sortable: true, width: 25},
    { field: 'spec.syslog-export', header: 'Syslog Exports', class: 'destinations-column-syslog', sortable: false, width: 25 },
    { field: 'spec.syslog-export.targets', header: 'Targets', class: 'destinations-column-syslog', sortable: false, width: '390px' },
    { field: 'status.total-notifications-sent', header: 'Notifications Sent', class: 'destinations-column-notifications-sent', sortable: false, width: 20 },
    // Following fields are currently not supported
    // { field: 'spec.email-list', header: 'Email List', class: 'destinationpolicy-column-email-list', sortable: true },
    // { field: 'spec.snmp-trap-servers', header: 'SNMP TRAP Servers', class: 'destinationpolicy-column-snmp_trap_servers', sortable: false },
  ];

  exportFilename: string = 'PSM-alert-destinations';
  exportMap: CustomExportMap = {
    'spec.syslog-export': (opts): string => {
      const value = Utility.getObjectValueByPropertyPath(opts.data, opts.field);
      const trimmedValues = Utility.trimUIFields(value);
      const resArr =  SyslogUtility.formatSyslogExports(trimmedValues);
      return resArr.toString();
    },
    'spec.syslog-export.targets': (opts): string => {
      const value = Utility.getObjectValueByPropertyPath(opts.data, opts.field);
      const resArr =  SyslogUtility.formatTargets(value, true);
      return resArr.toString();
    }
  };

  constructor(protected controllerService: ControllerService,
    protected cdr: ChangeDetectorRef,
    protected uiconfigsService: UIConfigsService,
    protected monitoringService: MonitoringService
    ) {
    super(controllerService, uiconfigsService);
  }

  ngOnInit() {
    super.ngOnInit();
    this.penTable = this.alertDestinationTable;
  }

  ngOnChanges(changes: SimpleChanges) {
    if (changes.dataObjects) {
      this.computeTargets();
    }
  }

  updateToolbar() {
    const currToolbar = this.controllerService.getToolbarData();
    currToolbar.buttons = [];
    if (this.uiconfigsService.isAuthorized(UIRolePermissions.monitoringalertdestination_create)) {
      currToolbar.buttons = [
        {
          cssClass: 'global-button-primary destinations-button',
          text: 'ADD DESTINATION',
          genTooltip: () => this.getTooltip(),
          computeClass: () =>  !this.penTable.showRowExpand &&
            this.totalTargets < DestinationpolicyComponent.MAX_TOTAL_TARGETS ?
            '' : 'global-button-disabled',
          callback: () => { this.penTable.createNewObject(); }
        },
      ];
    }
    this.controllerService.setToolbarData(currToolbar);
  }

  getTooltip(): string {
    return this.maxNewTargets === 0 ? 'Cannot exceed 8 targets across policies' : '';
  }

  computeTargets() {
    let totaltargets: number = 0;
    for (const policy of this.dataObjects) {
      if (policy.spec['syslog-export'].targets !== null) {
        totaltargets += policy.spec['syslog-export'].targets.length;
      }
    }
    this.totalTargets = totaltargets;
    const remainder = DestinationpolicyComponent.MAX_TOTAL_TARGETS - totaltargets;
    this.maxNewTargets = remainder < 0 ? 0 : remainder;
  }

  displayColumn(alerteventpolicies, col): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(alerteventpolicies, fields);
    const column = col.field;
    switch (column) {
      case 'spec.syslog-export':
        return SyslogUtility.formatSyslogExports(value);
      case 'spec.syslog-export.targets':
        return SyslogUtility.formatTargets(value);
      case 'spec.email-list':
        return JSON.stringify(value, null, 2);
      case 'spec.snmp-trap-servers':
        return JSON.stringify(value, null, 2);
      default:
        return Array.isArray(value) ? JSON.stringify(value, null, 2) : value;
    }
  }

  deleteRecord(object: MonitoringAlertDestination): Observable<{ body: IMonitoringAlertDestination | IApiStatus | Error, statusCode: number }> {
    return this.monitoringService.DeleteAlertDestination(object.meta.name);
  }

  showUpdateButtons(rowData: MonitoringAlertDestination): boolean {
    const isOKtoDelete: boolean  = true;
    for (let i = 0; i < this.eventPolices.length; i ++) {
      const policy: MonitoringAlertPolicy = this.eventPolices[i];
      const destinations: string[] = policy.spec.destinations;
      const matched  = destinations.find( (destination: string) =>  destination === rowData.meta.name);
      if (matched ) {
        return false;
      }
    }
     return isOKtoDelete;
  }

}
