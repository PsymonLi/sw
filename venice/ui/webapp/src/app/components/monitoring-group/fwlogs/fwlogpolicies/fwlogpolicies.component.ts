import { ChangeDetectorRef, Component, DoCheck, Input, IterableDiffer, IterableDiffers,  OnInit, ViewChild, ViewEncapsulation } from '@angular/core';
import { Animations } from '@app/animations';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { Utility } from '@app/common/Utility';
import { TablevieweditAbstract } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { MonitoringFwlogPolicy, IMonitoringFwlogPolicy, IApiStatus, IMonitoringExportConfig } from '@sdk/v1/models/generated/monitoring';
import { Table } from 'primeng/table';
import { Observable } from 'rxjs';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { TableCol, CustomExportMap } from '@app/components/shared/tableviewedit';

@Component({
  selector: 'app-fwlogpolicies',
  templateUrl: './fwlogpolicies.component.html',
  styleUrls: ['./fwlogpolicies.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None
})
export class FwlogpoliciesComponent extends TablevieweditAbstract<IMonitoringFwlogPolicy, MonitoringFwlogPolicy> implements OnInit, DoCheck {
  public static MAX_TARGETS_PER_POLICY = 2;
  public static MAX_TOTAL_TARGETS = 8;
  @ViewChild('policiesTable') policytable: Table;

  isTabComponent = false;
  disableTableWhenRowExpanded = true;

  dataObjects: ReadonlyArray<MonitoringFwlogPolicy> = [];
  maxNewTargets: number = FwlogpoliciesComponent.MAX_TARGETS_PER_POLICY;

  fwlogPoliciesEventUtility: HttpEventUtility<MonitoringFwlogPolicy>;

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

  cols: TableCol[] = [
    { field: 'meta.name', header: 'Name', class: 'fwlogpolicies-column-name', sortable: true, width: 20 },
    { field: 'spec.psm-target.enable', header: 'Enabled PSM Targets', class: 'fwlogpolicies-column-targets', sortable: true, width: 20 },
    { field: 'spec.filter', header: 'Exports', class: 'fwlogpolicies-column-filter', sortable: false, width: 30 },
    { field: 'spec.targets', header: 'Targets', class: 'fwlogpolicies-column-targets', sortable: false, width: 30 },
  ];
  arrayDiffers: IterableDiffer<any>;


  exportFilename: string = 'PSM-fwlog-policies';
  exportMap: CustomExportMap = {
    'meta.name': (opts): string => {
      return opts.data.meta.name;
    },
    'spec.filter': (opts): string => {
      const value = Utility.getObjectValueByPropertyPath(opts.data, opts.field);
      return value.join(',');
    },
    'spec.targets': (opts): string => {
      const value = Utility.getObjectValueByPropertyPath(opts.data, opts.field);
      const resArr =  Utility.formatTargets(value, true);
      return resArr.toString();
    }
  };

  constructor(protected controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected cdr: ChangeDetectorRef,
    protected monitoringService: MonitoringService,
    protected _iterableDiffers: IterableDiffers
  ) {
    super(controllerService, cdr, uiconfigsService);
    this.arrayDiffers = _iterableDiffers.find([]).create(HttpEventUtility.trackBy);
  }

  postNgInit() {
    this.getFwlogPolicies();
    this.maxNewTargets = this.computeTargets();
  }

  ngDoCheck() {
    const changes = this.arrayDiffers.diff(this.dataObjects);
    if (changes) {
      this.maxNewTargets = this.computeTargets();
    }
  }

  getClassName(): string {
    return this.constructor.name;
  }

  setDefaultToolbar() {
    let buttons = [];
    if (this.uiconfigsService.isAuthorized(UIRolePermissions.monitoringfwlogpolicy_create)) {
      buttons = [{
        cssClass: 'global-button-primary fwlogpolicies-button fwlogpolicies-button-ADD',
        text: 'ADD FIREWALL LOG EXPORT POLICY',
        genTooltip: () => this.getTooltip(),
        computeClass: () => this.shouldEnableButtons && this.maxNewTargets > 0 ? '' : 'global-button-disabled',
        callback: () => { this.createNewObject(); }
      }];
    }
    this.controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [
        { label: 'Firewall Logs', url: Utility.getBaseUIUrl() + 'monitoring/fwlogs' },
        { label: 'Firewall Log Export Policies', url: Utility.getBaseUIUrl() + 'monitoring/fwlogs/fwlogpolicies' }
      ]
    });
  }
  getTooltip(): string {
    return this.maxNewTargets === 0 ? 'Cannot exceed 8 total targets across firewall log export policies' : '';
  }

  getFwlogPolicies() {
    this.fwlogPoliciesEventUtility = new HttpEventUtility<MonitoringFwlogPolicy>(MonitoringFwlogPolicy);
    this.dataObjects = this.fwlogPoliciesEventUtility.array;
    const subscription = this.monitoringService.WatchFwlogPolicy().subscribe(
      (response) => {
        this.fwlogPoliciesEventUtility.processEvents(response);
      },
      this.controllerService.webSocketErrorHandler('Failed to get Policies')
    );
    this.subscriptions.push(subscription);
  }


  displayColumn(exportData, col): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(exportData, fields);
    const column = col.field;
    switch (column) {
      case 'spec.targets':
        return Utility.formatTargets(value);
      default:
        return Array.isArray(value) ? value.join(', ') : value;
    }
  }

  deleteRecord(object: MonitoringFwlogPolicy): Observable<{ body: MonitoringFwlogPolicy | IApiStatus | Error, statusCode: number }> {
    return this.monitoringService.DeleteFwlogPolicy(object.meta.name);
  }

  generateDeleteConfirmMsg(object: IMonitoringFwlogPolicy) {
    return 'Are you sure you want to delete firewall log export policy ' + object.meta.name;
  }

  generateDeleteSuccessMsg(object: MonitoringFwlogPolicy) {
    return 'Deleted firewall log export policy ' + object.meta.name;
  }

  computeTargets(): number {
    let totaltargets: number = 0;
    for (const policy of this.dataObjects) {
      if (policy.spec.targets !== null) {
        totaltargets += policy.spec.targets.length;
      }
    }
    const remainder = FwlogpoliciesComponent.MAX_TOTAL_TARGETS - totaltargets;
    return Math.min(remainder, FwlogpoliciesComponent.MAX_TARGETS_PER_POLICY);
  }

}
