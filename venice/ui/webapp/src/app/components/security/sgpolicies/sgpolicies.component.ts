import { Component, OnInit, ViewEncapsulation, OnDestroy, ChangeDetectorRef, ViewChild, ChangeDetectionStrategy } from '@angular/core';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { Utility } from '@app/common/Utility';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { SecurityService } from '@app/services/generated/security.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { SecurityNetworkSecurityPolicy, ISecurityNetworkSecurityPolicy, IApiStatus, ISecurityNetworkSecurityPolicyList, SecurityApp, SecuritySecurityGroup } from '@sdk/v1/models/generated/security';
import { Observable } from 'rxjs';
import { TableCol, CustomExportMap } from '@app/components/shared/tableviewedit';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { SelectItem } from 'primeng/api';
import { WorkloadWorkload } from '@sdk/v1/models/generated/workload';
import { WorkloadService } from '@app/services/generated/workload.service';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';

@Component({
  selector: 'app-sgpolicies',
  templateUrl: './sgpolicies.component.html',
  styleUrls: ['./sgpolicies.component.scss'],
  changeDetection: ChangeDetectionStrategy.OnPush,
  encapsulation: ViewEncapsulation.None
})

export class SgpoliciesComponent extends DataComponent implements OnInit, OnDestroy {
  @ViewChild('securityPoliciesTable', { static: true }) securityPoliciesTable: PentableComponent;
  isTabComponent: boolean = false;
  disableTableWhenRowExpanded: boolean = true;
  dataObjects: ReadonlyArray<SecurityNetworkSecurityPolicy> = [];
  exportFilename: string = 'PSM-sgpolicies';
  exportMap: CustomExportMap = {};
  tableLoading: boolean = false;

  securityApps: ReadonlyArray<SecurityApp> = [];
  securityAppOptions: SelectItem[] = [];

  securityGroups: ReadonlyArray<SecuritySecurityGroup> = [];
  securityGroupOptions: SelectItem[] = [];

  workloads: ReadonlyArray<WorkloadWorkload> = [];
  // Map from IP to workload name
  ipOptions: any[] = [];

  // Currently venice supports only one security policy.
  MAX_POLICY_NUM: number = 1;
  EDIT_INLINE_MAX_RULES_LIMIT: number = 0;

  // Holds all policy objects
  sgPoliciesEventUtility: HttpEventUtility<SecurityNetworkSecurityPolicy>;

  // All columns are set as not sortable as it isn't currently supported
  cols: TableCol[] = [
    { field: 'meta.name', header: 'Policy Name', class: 'sgpolicies-column-name', sortable: true, width: 'auto' },
    { field: 'meta.mod-time', header: 'Modification Time', class: 'sgpolicies-column-date', sortable: true, width: '180px' },
    { field: 'meta.creation-time', header: 'Creation Time', class: 'sgpolicies-column-date', sortable: true, width: '25' },
  ];

  bodyIcon = {
    margin: {
      top: '9px',
      left: '8px'
    },
    url: '/assets/images/icons/security/icon-security-policy-black.svg'
  };

  tableIcon: Icon = {
    margin: {
      top: '0px',
      left: '0px',
    },
    matIcon: 'grid_on'
  };
  shouldEnableButtons: boolean;

  constructor(
    protected _controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected securityService: SecurityService,
    protected workloadService: WorkloadService,
    protected cdr: ChangeDetectorRef,
  ) {
    super(_controllerService, uiconfigsService);
    this.shouldEnableButtons = false;
  }

  ngOnInit() {
    super.ngOnInit();
    this.penTable = this.securityPoliciesTable;
    this.getSecurityPolicies();
    this.getSecurityApps();
    // this.getWorkloads();
    // this.getSecuritygroups();
  }

  setDefaultToolbar() {
    let buttons = [];
    if (this.uiconfigsService.isAuthorized(UIRolePermissions.securitynetworksecuritypolicy_create)) {
      buttons = [{
        cssClass: 'global-button-primary global-button-padding',
        text: 'ADD POLICY',
        computeClass: () => this.shouldEnableButtons ? '' : 'global-button-disabled',
        callback: () => { this.securityPoliciesTable.createNewObject(); },
        genTooltip: () => this.getTooltip(),
      }];
    }
    this._controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [{ label: 'Network Security Policies', url: Utility.getBaseUIUrl() + 'security/sgpolicies' }]
    });
  }

  getTooltip(): string {
    return this.dataObjects.length > 0 ? 'System allows one network security policy' : 'Add network security policy';
  }

  getSecurityPolicies() {
    this.tableLoading = true;
    this.refreshGui(this.cdr);
    this.securityService.ListNetworkSecurityPolicyCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.dataObjects = response.data as ReadonlyArray<SecurityNetworkSecurityPolicy> ;
        this.tableLoading = false;
        this.shouldEnableButtons = (this.dataObjects.length === 0);
        this.refreshGui(this.cdr);
      },
      (error) => {
        this.tableLoading = false;
        this.controllerService.invokeRESTErrorToaster('Failed to get network security policies', error);
        this.refreshGui(this.cdr);
      }
    );
  }

  getSecurityApps() {
    const sub = this.securityService.ListAppCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        this.securityApps = response.data;
        this.securityAppOptions = this.securityApps.map(item => {
          return {
            label: item.meta.name,
            value: item.meta.name
          };
        });
        this.refreshGui(this.cdr);
      },
      (error) => {
        this._controllerService.invokeRESTErrorToaster('Failed to get apps', error);
        }
    );
    this.subscriptions.push(sub);
  }

  getSecuritygroups() {
    const sub = this.securityService.ListSecurityGroupCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        this.securityGroups = response.data;
        this.securityAppOptions = this.securityGroups.map(item => {
          return {
            label: item.meta.name,
            value: item.meta.name
          };
        });
        this.refreshGui(this.cdr);
      },
      (error) => {
        this._controllerService.invokeRESTErrorToaster('Failed to get network security policy', error);
        }
    );
    this.subscriptions.push(sub);
  }


  getWorkloads() {
    const workloadSubscription = this.workloadService.ListWorkloadCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.workloads = response.data as WorkloadWorkload[];
        this.buildIPMap();
        this.refreshGui(this.cdr);
      },
      (error) => {
        this._controllerService.invokeRESTErrorToaster('Failed to get workloads', error);
        }
    );
    this.subscriptions.push(workloadSubscription);
  }

  buildIPMap() {
    const ipMap = {};
    this.ipOptions = [];
    // Taking IPs from spec, since status isn't always filled out currently
    // TODO: Take IPs from status
    this.workloads.forEach((w) => {
      w.spec.interfaces.forEach((intf) => {
        intf['ip-addresses'].forEach((ip) => {
          ipMap[ip] = w.meta.name;
        });
      });
    });
    Object.keys(ipMap).forEach(ip => {
      this.ipOptions.push({ ip: ip, workload: ipMap[ip] });
    });
  }

  displayColumn(data, col): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(data, fields);
    const column = col.field;
    switch (column) {
      default:
        return Array.isArray(value) ? JSON.stringify(value, null, 2) : value;
    }
  }

  deleteRecord(object: SecurityNetworkSecurityPolicy): Observable<{ body: ISecurityNetworkSecurityPolicy | IApiStatus | Error, statusCode: number }> {
    return this.securityService.DeleteNetworkSecurityPolicy(object.meta.name);
  }

  generateDeleteConfirmMsg(object: ISecurityNetworkSecurityPolicy) {
    return 'Are you sure you want to delete SG Policy ' + object.meta.name;
  }

  generateDeleteSuccessMsg(object: ISecurityNetworkSecurityPolicy) {
    return 'Deleted SG Policy ' + object.meta.name;
  }

  editTooltip(rowData) {
    if (rowData.spec.rules.length > this.EDIT_INLINE_MAX_RULES_LIMIT) {
      return 'Edit Policy on Details Page';
    } else {
      return 'Edit Policy Inline';
    }
  }

  attemptInlineEdit(rowData): boolean {
    if (rowData.spec.rules.length > this.EDIT_INLINE_MAX_RULES_LIMIT) {
      this._controllerService.navigate(['/security', 'sgpolicies', rowData.meta.name]);
    }
    return true;
  }

  onEditPolicy($event , policy: ISecurityNetworkSecurityPolicy) {
    // The path is => /security/sgpolicies/dp-security-policyÍ
    this._controllerService.navigate(['/security', 'sgpolicies', policy.meta.name]);
  }

}
