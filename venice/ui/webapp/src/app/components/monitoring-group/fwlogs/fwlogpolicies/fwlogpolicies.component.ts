import { ChangeDetectorRef, Component, IterableDiffer, IterableDiffers,  OnInit, ViewChild, ViewEncapsulation } from '@angular/core';
import { Animations } from '@app/animations';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { Utility } from '@app/common/Utility';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { MonitoringFwlogPolicy, IMonitoringFwlogPolicy, IApiStatus } from '@sdk/v1/models/generated/monitoring';
import { Table } from 'primeng/table';
import { Observable } from 'rxjs';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { TableCol, CustomExportMap } from '@app/components/shared/tableviewedit';
import { SyslogUtility } from '@app/common/SyslogUtility';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';

@Component({
  selector: 'app-fwlogpolicies',
  templateUrl: './fwlogpolicies.component.html',
  styleUrls: ['./fwlogpolicies.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None
})
export class FwlogpoliciesComponent extends DataComponent implements OnInit {
  public static MAX_TARGETS_PER_POLICY = 2;
  public static MAX_TOTAL_TARGETS = 8;
  @ViewChild('fwpLogPolicyTable') fwpLogPolicyTable: PentableComponent;

  isTabComponent = false;
  disableTableWhenRowExpanded = true;
  tableLoading = false;

  totalTargets: number = 0;
  maxNewTargets: number = FwlogpoliciesComponent.MAX_TOTAL_TARGETS;

  dataObjects: ReadonlyArray<MonitoringFwlogPolicy> = [];

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

  exportFilename: string = 'PSM-fwlog-policies';
  exportMap: CustomExportMap = {
    'meta.name': (opts): string => {
      return opts.data.meta.name;
    },
    'spec.filter': (opts): string => {
      const value = Utility.getObjectValueByPropertyPath(opts.data, opts.field);
      const trimmedValues = Utility.trimUIFields(value);
      return trimmedValues.join(',');
    },
    'spec.targets': (opts): string => {
      const value = Utility.getObjectValueByPropertyPath(opts.data, opts.field);
      const resArr =  SyslogUtility.formatTargets(value, true);
      return resArr.toString();
    }
  };

  constructor(protected controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected cdr: ChangeDetectorRef,
    protected monitoringService: MonitoringService,
    protected _iterableDiffers: IterableDiffers
  ) {
    super(controllerService, uiconfigsService);
  }

  ngOnInit() {
    super.ngOnInit();
    this.penTable = this.fwpLogPolicyTable;
    this.tableLoading = true;
    this.getFwlogPolicies();
  }

  getFwlogPolicies() {
    const dscSubscription = this.monitoringService.ListFwlogPolicyCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.tableLoading = false;
        this.dataObjects = response.data as MonitoringFwlogPolicy[];
        this.computeTargets();
        this.refreshGui(this.cdr);
      }
    );
    this.subscriptions.push(dscSubscription);
  }

  setDefaultToolbar() {
    let buttons = [];
    if (this.uiconfigsService.isAuthorized(UIRolePermissions.monitoringfwlogpolicy_create)) {
      buttons = [{
        cssClass: 'global-button-primary fwlogpolicies-button fwlogpolicies-button-ADD',
        text: 'ADD FIREWALL LOG EXPORT POLICY',
        genTooltip: () => this.getTooltip(),
        computeClass: () =>  !this.penTable.showRowExpand &&
          this.totalTargets < FwlogpoliciesComponent.MAX_TOTAL_TARGETS ?
          '' : 'global-button-disabled',
        callback: () => { this.penTable.createNewObject(); }
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
    const remainder = FwlogpoliciesComponent.MAX_TOTAL_TARGETS - totaltargets;
    this.maxNewTargets = Math.min(remainder, FwlogpoliciesComponent.MAX_TARGETS_PER_POLICY);
  }

  displayColumn(exportData, col): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(exportData, fields);
    const column = col.field;
    switch (column) {
      case 'spec.targets':
        return SyslogUtility.formatTargets(value);
      default:
        return Array.isArray(value) ? value.join(', ') : value;
    }
  }

  deleteRecord(object: MonitoringFwlogPolicy): Observable<{ body: MonitoringFwlogPolicy | IApiStatus | Error, statusCode: number }> {
    return this.monitoringService.DeleteFwlogPolicy(object.meta.name);
  }
}
