import { ChangeDetectorRef, Component, OnInit, ViewEncapsulation } from '@angular/core';
import { TableCol } from '@app/components/shared/tableviewedit';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { ClusterService } from '@app/services/generated/cluster.service';
import { HttpEventUtility } from '@common/HttpEventUtility';
import { Utility } from '@common/Utility';
import { Observable, Subscription } from 'rxjs';
import { Animations } from '@app/animations';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { TablevieweditAbstract } from '@components/shared/tableviewedit/tableviewedit.component';

import { IApiStatus } from '@sdk/v1/models/generated/security';
import { ISecuritySecurityGroup, SecuritySecurityGroup} from '@sdk/v1/models/generated/security/security-security-group.model';
import { SecurityService } from '@app/services/generated/security.service';
import { TableUtility } from '@app/components/shared/tableviewedit/tableutility';
import { ILabelsSelector } from '@sdk/v1/models/generated/monitoring/labels-selector.model';


@Component({
  selector: 'app-securitygroups',
  templateUrl: './securitygroups.component.html',
  styleUrls: ['./securitygroups.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
})
export class SecuritygroupsComponent extends TablevieweditAbstract<ISecuritySecurityGroup, SecuritySecurityGroup> implements OnInit {

  bodyicon: Icon = {
    margin: {
      top: '9px',
      left: '8px'
    },
    url: '/assets/images/icons/security/ico-security-group-black.svg',
  };

  headerIcon: Icon = {
    margin: {
      top: '0px',
      left: '10px',
    },
    matIcon: 'grid_on'  // TODO: need a new icon for securtiy-group table
  };

  subscriptions: Subscription[] = [];
  dataObjects: ReadonlyArray<SecuritySecurityGroup>;
  exportFilename: string = 'Venice-securitygroups';

  // It is important to set values.
  isTabComponent: boolean = false;
  disableTableWhenRowExpanded: boolean = true;

  cols: TableCol[] = [
    {field: 'meta.name', header: 'Name', class: 'securitygroups-column-securitygroups-name', sortable: true, width: 20},
    {field: 'meta.mod-time', header: 'Modification Time', class: 'securitygroups-column-date', sortable: true, width: 20},
    {field: 'meta.creation-time', header: 'Creation Time', class: 'securitygroups-column-date', sortable: true, width: 20},
    {field: 'spec.workload-selector', header: 'Workload Selectors', class: 'securitygroups-column-workload-selector', sortable: true, width: 20},
    {field: 'spec.service-labels', header: 'Services Lables', class: 'securitygroups-column-service-labels', sortable: true, width: 20},
    {field: 'spec.match-prefixes', header: 'Match Prefixes', class: 'securitygroups-column-match-prefixes', sortable: true, width: 20},
  ];

  securitygroupsEventUtility: HttpEventUtility<SecuritySecurityGroup>;


  constructor(private securityService: SecurityService,
    protected cdr: ChangeDetectorRef,
    protected uiconfigsService: UIConfigsService,
    protected controllerService: ControllerService) {
        super(controllerService, cdr, uiconfigsService);
  }

  postNgInit(): void {
    this.getSecuritygroups();
  }


  getClassName(): string {
    return this.constructor.name;
  }

  setDefaultToolbar() {

    let buttons = [];

    if (this.uiconfigsService.isAuthorized(UIRolePermissions.securitysecuritygroup_create)) {
      buttons = [{
        cssClass: 'global-button-primary securitygroups-button securitygroups-button-ADD',
        text: 'ADD SECURITY GROUP',
        callback: () => { this.createNewObject(); }
      }];
    }

    this.controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [{label: 'Security', url: Utility.getBaseUIUrl() + 'security/securitygroups'}]
    });
  }

  deleteRecord(object: SecuritySecurityGroup): Observable<{ body: ISecuritySecurityGroup | IApiStatus | Error | ISecuritySecurityGroup; statusCode: number }> {
    return this.securityService.DeleteSecurityGroup(object.meta.name);
  }

  generateDeleteConfirmMsg(object: SecuritySecurityGroup): string {
    return 'Are you sure you want to delete security group ' + object.meta.name;
  }
  generateDeleteSuccessMsg(object: SecuritySecurityGroup): string {
    return 'Deleted security group ' + object.meta.name;
  }

  getSecuritygroups() {
    this.securitygroupsEventUtility = new HttpEventUtility<SecuritySecurityGroup>(SecuritySecurityGroup, true);
    this.dataObjects = this.securitygroupsEventUtility.array as ReadonlyArray<SecuritySecurityGroup>;
    const subscription = this.securityService.WatchSecurityGroup().subscribe(
      response => {
        this.securitygroupsEventUtility.processEvents(response);
      },
      this.controllerService.webSocketErrorHandler('Failed to get Security Groups info')
    );
    this.subscriptions.push(subscription);
  }

  displayColumn(data, col): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(data, fields);
    const column = col.field;
    switch (column) {
      case 'spec.workload-selector':
        return this.displayColumn_workloadselectors(fields, value);
      default:
         return Array.isArray(value) ? value.join(',') : value;
    }
  }

  displayColumn_workloadselectors(fields: string[], value: ILabelsSelector): string {
    const list = Utility.convertOneLabelSelectorToStringList(value);
    return list.join(', ');
  }

}
