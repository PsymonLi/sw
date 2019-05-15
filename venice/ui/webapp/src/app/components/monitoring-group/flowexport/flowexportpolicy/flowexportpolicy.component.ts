import { ChangeDetectorRef, Component, Input, ViewEncapsulation } from '@angular/core';
import { Animations } from '@app/animations';
import { Utility } from '@app/common/Utility';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { IApiStatus, IMonitoringFlowExportPolicy, IMonitoringMatchRule, MonitoringFlowExportPolicy } from '@sdk/v1/models/generated/monitoring';
import { Observable } from 'rxjs';
import { TableCol, TablevieweditAbstract } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';

@Component({
  selector: 'app-flowexportpolicy',
  templateUrl: './flowexportpolicy.component.html',
  styleUrls: ['./flowexportpolicy.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None
})
export class FlowexportpolicyComponent extends TablevieweditAbstract<IMonitoringFlowExportPolicy, MonitoringFlowExportPolicy> {
  @Input() dataObjects: ReadonlyArray<MonitoringFlowExportPolicy> = [];

  headerIcon: Icon = {
    margin: {
      top: '0px',
      left: '10px',
    },
    matIcon: 'grid_on'
  };

  cols: TableCol[] = [
    { field: 'meta.name', header: 'Targets', class: 'flowexportpolicy-column-name', sortable: false, width: 25 },
    { field: 'spec.match-rules', header: 'Match Rules', class: 'flowexportpolicy-column-match-rules', sortable: false, width: 35 },
    { field: 'spec.exports', header: 'Targets', class: 'flowexportpolicy-column-targets', sortable: false, width: 40 },
  ];

  isTabComponent = false;
  disableTableWhenRowExpanded = true;

  constructor(protected controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected cdr: ChangeDetectorRef,
    protected monitoringService: MonitoringService) {
    super(controllerService, cdr);
  }

  setDefaultToolbar() {
    let buttons = [];
    if (this.uiconfigsService.isAuthorized(UIRolePermissions.monitoringflowexportpolicy_create)) {
      buttons = [{
        cssClass: 'global-button-primary flowexportpolicy-button',
        text: 'ADD FlOW EXPORT',
        computeClass: () => this.shouldEnableButtons ? '' : 'global-button-disabled',
        callback: () => { this.createNewObject(); }
      }];
    }
    this.controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [
        { label: 'Flow Export Policies', url: Utility.getBaseUIUrl() + 'monitoring/flowexport' }
      ]
    });
  }

  getClassName(): string {
    return this.constructor.name;
  }

  displayColumn(exportData, col): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(exportData, fields);
    const column = col.field;
    switch (column) {
      case 'spec.exports':
        return value.map(item => item.destination).join(', ');
      default:
        return Array.isArray(value) ? JSON.stringify(value, null, 2) : value;
    }
  }

  formatMatchRules(data: IMonitoringMatchRule[]) {
    if (data == null) {
      return '';
    }
    const retArr = [];
    data.forEach((req) => {
      let ret = '';

      ret += 'Src: ';
      let sources = [];
      for (const k in req.source) {
        if (req.source.hasOwnProperty(k)) {
          const v: string[] = req.source[k];
          sources = sources.concat(v);
        }
      }
      if (sources.length === 0) {
        ret += '*';
      } else {
        ret += sources.join(', ');
      }

      ret += '    Dest: ';
      let dest = [];
      for (const k in req.destination) {
        if (req.destination.hasOwnProperty(k)) {
          const v: string[] = req.destination[k];
          dest = dest.concat(v);
        }
      }
      if (dest.length === 0) {
        ret += '*';
      } else {
        ret += dest.join(', ');
      }

      ret += '    Apps: ';
      let apps = req['app-protocol-selectors'].applications || [];
      apps = apps.concat(req['app-protocol-selectors']['proto-ports']);
      if (apps.length === 0) {
        ret += '*';
      } else {
        ret += apps.join(', ');
      }
      retArr.push(ret);
    });
    return retArr;
  }

  deleteRecord(object: MonitoringFlowExportPolicy): Observable<{ body: IMonitoringFlowExportPolicy | IApiStatus | Error, statusCode: number }> {
    return this.monitoringService.DeleteFlowExportPolicy(object.meta.name);
  }

  generateDeleteConfirmMsg(object: IMonitoringFlowExportPolicy) {
    return 'Are you sure you want to delete policy ' + object.meta.name;
  }

  generateDeleteSuccessMsg(object: IMonitoringFlowExportPolicy) {
    return 'Deleted policy ' + object.meta.name;
  }

}
