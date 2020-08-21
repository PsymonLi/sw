import { ChangeDetectorRef, ChangeDetectionStrategy, Component, OnDestroy, OnInit, ViewChild, ViewEncapsulation } from '@angular/core';
import { Animations } from '@app/animations';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { Utility } from '@app/common/Utility';
import { CustomExportMap, TableCol } from '@app/components/shared/tableviewedit';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { ClusterService } from '@app/services/generated/cluster.service';
import { SecurityService } from '@app/services/generated/security.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { ClusterDistributedServiceCard } from '@sdk/v1/models/generated/cluster';
import { IApiStatus, ISecurityFirewallProfile, SecurityFirewallProfile } from '@sdk/v1/models/generated/security';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { Observable } from 'rxjs';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';

@Component({
  selector: 'app-firewallprofiles',
  templateUrl: './firewallprofiles.component.html',
  styleUrls: ['./firewallprofiles.component.scss'],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush,
  animations: Animations
})
export class FirewallprofilesComponent  extends DataComponent implements OnInit, OnDestroy {
  @ViewChild('firewallprofilesTable') firewallprofilesTable: PentableComponent;

  isTabComponent = false;
  disableTableWhenRowExpanded = true;
  tableLoading: boolean = false;
  exportFilename: string = 'PSM-DSC-firewall-profiles';
  exportMap: CustomExportMap;

  dataObjects: ReadonlyArray<SecurityFirewallProfile> ;
  securityfirewallprofilesEventUtility: HttpEventUtility<SecurityFirewallProfile>;
  selectedFirewallProfile: SecurityFirewallProfile;

  viewPendingNaples: boolean = false;

  naplesList: ClusterDistributedServiceCard[] = [];
  macToNameMap: { [key: string]: string } = {};

  bodyIcon: any = {
    margin: {
      top: '9px',
      left: '8px',
    },
    url: '/assets/images/icons/security/ico-app-black.svg',
  };

  headerIcon: Icon = {
    margin: {
      top: '0px',
      left: '10px',
    },
    matIcon: 'grid_on'
  };


  cols: TableCol[] = [
    { field: 'meta.name', header: 'Name', class: 'firewallprofiles-column-metaname', sortable: true, width: 15 },
    { field: 'meta.mod-time', header: 'Modification Time', class: 'firewallprofiles-column-date', sortable: true, width: '175px' },
    { field: 'meta.creation-time', header: 'Creation Time', class: 'firewallprofiles-column-date', sortable: true, width: '175px' },
    // { field: 'spec.alg.type', header: 'Configuration', class: 'firewallprofiles-column-host-name', sortable: false, width: 35 },
  ];
  constructor(protected _controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected cdr: ChangeDetectorRef,
    protected securityService: SecurityService,
    private clusterService: ClusterService
  ) {
    super(_controllerService, uiconfigsService);
  }

  ngOnInit() {
    super.ngOnInit();
    this.penTable = this.firewallprofilesTable;
    this.getSecurityFirewallprofiles();
    this.getNaples();
    this.setDefaultToolbar();
  }

  /**
   * Fetch security apps records
   */
  getSecurityFirewallprofiles() {
    this.securityfirewallprofilesEventUtility = new HttpEventUtility<SecurityFirewallProfile>(SecurityFirewallProfile);
    this.dataObjects = this.securityfirewallprofilesEventUtility.array;
    const subscription = this.securityService.WatchFirewallProfile().subscribe(
      response => {
        this.securityfirewallprofilesEventUtility.processEvents(response);
        if (this.dataObjects && this.dataObjects.length > 0) {
          // There should be only one record
          this.selectedFirewallProfile = this.dataObjects[0];
        }
        this.refreshGui(this.cdr);
      },
      this._controllerService.webSocketErrorHandler('Failed to get firewall profiles')
    );
    this.subscriptions.push(subscription); // add subscription to list, so that it will be cleaned up when component is destroyed.
  }

  setDefaultToolbar() {
  let buttons = [];
    if (this.uiconfigsService.isAuthorized(UIRolePermissions.securityfirewallprofile_update)) {
      buttons = [
        {
          cssClass: 'global-button-primary firewallprofiles-update-button',
          text: 'UPDATE',
          callback: () => { this.firewallprofilesTable.createNewObject(); }
        }
      ];
    }
    this._controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [{ label: 'Firewall profiles', url: Utility.getBaseUIUrl() + 'security/firewallprofiles' }]
    });
  }

  generateDeleteConfirmMsg(object: ISecurityFirewallProfile) {
    return 'Are you sure you want to delete firewall profile: ' + object.meta.name;
  }

  generateDeleteSuccessMsg(object: ISecurityFirewallProfile) {
    return 'Deleted firewall profile ' + object.meta.name;
  }

  deleteRecord(object: SecurityFirewallProfile): Observable<{ body: SecurityFirewallProfile | IApiStatus | Error, statusCode: number }> {
    throw new Error('Method not implemented.');
  }

  displayColumn(rowData: SecurityFirewallProfile, col: TableCol, hasUiHintMap: boolean = true): any {
    const fields = col.field.split('.');

    const value = Utility.getObjectValueByPropertyPath(rowData, fields);
    const column = col.field;
    switch (column) {
      default:
        return Array.isArray(value) ? value.join(', ') : value;
    }
  }

  closeDetails() {
    this.selectedFirewallProfile = null;
  }

  viewPendingNaplesList() {
    this.viewPendingNaples = !this.viewPendingNaples;
  }

  getNaples() {
    const subscription = this.clusterService.ListDistributedServiceCardCache().subscribe(
      response => {
       /*  this.naplesEventUtility.processEvents(response);
        // mac-address to Name map
        this.macToNameMap = {};
        for (const smartnic of this.naples) {
          if (smartnic.spec.id != null && smartnic.spec.id !== '') {
            this.macToNameMap[smartnic.meta.name] = smartnic.spec.id;
          }
        } */
        if (response.connIsErrorState) {
          return;
        }
        this.naplesList = response.data as ClusterDistributedServiceCard[];
        for (const smartnic of this.naplesList) {
          if (smartnic.spec.id != null && smartnic.spec.id !== '') {
            this.macToNameMap[smartnic.meta.name] = smartnic.spec.id;
          }
        }
        this.refreshGui(this.cdr);
      },
      this._controllerService.webSocketErrorHandler('Failed to get DSCs info')
    );
    this.subscriptions.push(subscription); // add subscription to list, so that it will be cleaned up when component is destroyed.
  }

  getNaplesName(mac: string): string {
    return this.macToNameMap[mac];
  }
}
