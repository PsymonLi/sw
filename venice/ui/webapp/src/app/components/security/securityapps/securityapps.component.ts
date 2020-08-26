import { ChangeDetectorRef, Component, OnDestroy, OnInit, ViewChild, ViewEncapsulation, ChangeDetectionStrategy } from '@angular/core';
import { ActivatedRoute, Router } from '@angular/router';
import { Animations } from '@app/animations';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { Utility } from '@app/common/Utility';
import { LazyrenderComponent } from '@app/components/shared/lazyrender/lazyrender.component';
import { PrettyDatePipe } from '@app/components/shared/Pipes/PrettyDate.pipe';
import { CustomExportMap, TableCol } from '@app/components/shared/tableviewedit';
import { TablevieweditAbstract } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { SecurityService } from '@app/services/generated/security.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { IApiStatus, ISecurityApp, SecurityApp } from '@sdk/v1/models/generated/security';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { Table } from 'primeng/table';
import { Observable, Subscription } from 'rxjs';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';

interface PartialTableCol {
  field: string;
}

@Component({
  selector: 'app-securityapps',
  templateUrl: './securityapps.component.html',
  styleUrls: ['./securityapps.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush
})
/**
 * This component displays security-apps UI.
 * TODO: 2018-12-20, there is no UX design for this page yet. It simply show records to help QA viewing data.
 */
export class SecurityappsComponent extends DataComponent implements OnInit, OnDestroy {

  @ViewChild('securityappsTable',  { static: true }) securityappsTable: PentableComponent;

  dataObjects: ReadonlyArray<SecurityApp> = [];

  disableTableWhenRowExpanded = true;
  subscriptions: Subscription[] = [];

  exportFilename: string = 'PSM-Apps';
  exportMap: CustomExportMap = {
    'spec.alg.type': (opts): string => {
      return opts.data.spec;
    }
  };

  selectedApp: SecurityApp = null;

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

  // Used for the table - when true there is a loading icon displayed
  tableLoading: boolean = false;

  cols: TableCol[] = [
    { field: 'meta.name', header: 'Name', class: 'securityapps-column-metaname', sortable: true, width: '400px' },
    { field: 'spec.alg.type', header: 'Configuration', class: 'securityapps-column-host-name', sortable: false, width: 100 },
    { field: 'meta.creation-time', header: 'Creation Time', class: 'securityapps-column-date', sortable: true, width: '175px' },
    { field: 'meta.mod-time', header: 'Modification Time', class: 'securityapps-column-date', sortable: true, width: '175px' },
  ];

  constructor(protected _controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected cdr: ChangeDetectorRef,
    protected securityService: SecurityService,
    private _route: ActivatedRoute,
    private router: Router
  ) {
    super(_controllerService, uiconfigsService);
  }

  ngOnInit() {
    super.ngOnInit();
    this.penTable = this.securityappsTable;
    this.tableLoading = true;
    this.registURLChange();
    this.getSecurityApps();
  }

  registURLChange() {
    const sub = this._route.queryParams.subscribe(params => {
      if (params.hasOwnProperty('app')) {
        // alerttab selected
        if (Utility.isValueOrArrayEmpty(this.dataObjects)) {
          this.getSearchedSecurityApp(params['app']);
        } else {
          this.selectedApp = this.findSecurityAppByName(params['app']);
          this.refreshGui(this.cdr);
        }
      } else {
        this.selectedApp = null;
        this.refreshGui(this.cdr);
      }
    });
    this.subscriptions.push(sub);
  }

  findSecurityAppByName(name: string) {
    if (Utility.isValueOrArrayEmpty(this.dataObjects)) {
      return null;
    }
    return this.dataObjects.find(app => app.meta.name === name);
  }

  setDefaultToolbar() {
    let buttons = [];
    if (this.uiconfigsService.isAuthorized(UIRolePermissions.securityapp_create)) {
      buttons = [
        {
          cssClass: 'global-button-primary security-new-app',
          text: 'ADD APP',
          computeClass: () =>  !(this.penTable.showRowExpand) ? '' : 'global-button-disabled',
          callback: () => { this.penTable.createNewObject(); }
        }
      ];
    }
    this._controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [{ label: 'Apps', url: Utility.getBaseUIUrl() + 'security/securityapps' }]
    });
  }


  generateDeleteConfirmMsg(object: ISecurityApp) {
    return 'Are you sure you want to delete security app : ' + object.meta.name;
  }

  generateDeleteSuccessMsg(object: ISecurityApp) {
    return 'Deleted security app ' + object.meta.name;
  }

  getSearchedSecurityApp(appname) {
    const subscription = this.securityService.GetApp(appname).subscribe(
      response => {
        this.selectedApp = response.body as SecurityApp;
        this.refreshGui(this.cdr);
      },
      this._controllerService.webSocketErrorHandler('Failed to get Apps')
    );
    this.subscriptions.push(subscription); // add subscription to list, so that it will be cleaned up when component is destroyed.
  }

  /**
   * Fetch security apps records
   */
  getSecurityApps() {
    const subscription = this.securityService.ListAppCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.dataObjects = response.data as SecurityApp[];
        this.tableLoading = false;
        // As server  keeps pushing records to UI and UI has a selected securityApp, we have to update the selected one.
        if (this.selectedApp) {
          this.selectedApp = this.findSecurityAppByName(this.selectedApp.meta.name);
        }
        this.refreshGui(this.cdr);
      },
      (error) => {
        this.tableLoading = false;
        this.controllerService.invokeRESTErrorToaster('Failed to get network security app', error);
        this.refreshGui(this.cdr);
      }
    );
    this.subscriptions.push(subscription); // add subscription to list, so that it will be cleaned up when component is destroyed.
  }

  deleteRecord(object: SecurityApp): Observable<{ body: SecurityApp | IApiStatus | Error, statusCode: number }> {
    return this.securityService.DeleteApp(object.meta.name);
  }

  displaySpecAlgType(rowData: SecurityApp, col: TableCol) {
    return this.displayColumn(rowData, col, false);
  }

  /**
   * col: TableCol|PartialTableCol make html (line 79) {{displayColumn(this.selectedApp, {'field': 'spec.alg.type'})}} pas compilation process.
   * @param rowData
   * @param col
   * @param hasUiHintMap
   */
  displayColumn(rowData: SecurityApp, col: TableCol|PartialTableCol, hasUiHintMap: boolean = true): any {
    const fields = col.field.split('.');
    if (fields.includes('alg')) {
      if (rowData.spec == null) {
        return '';
      }
    }
    let value = null;
    if (!fields.includes('alg') || rowData.spec.alg) {
      value = Utility.getObjectValueByPropertyPath(rowData, fields);
    }
    const column = col.field;
    if (fields.includes('alg')) {
      if (rowData.spec.alg == null) {
        value = '';
      } else {
        value = 'ALG Type: ' + value;
      }
      const protocolValues = Utility.getObjectValueByPropertyPath(rowData, ['spec', 'proto-ports']);
      if (protocolValues) {
        const protoarray = [];
        for (const i of protocolValues) {
          protoarray.push('Protocol: ' + i.protocol + ' Ports: ' + i.ports);
        }
        if (value) {
          value += '; ';
        }
        value +=  protoarray.join(', ');
      }
    }
    if (fields.includes('mod-time') || fields.includes('creation-time')) {
      value = new PrettyDatePipe('en-US').transform(value);
    }
    switch (column) {
      default:
        return Array.isArray(value) ? value.join(', ') : value;
    }
  }

  selectApp(event) {
    if ( this.selectedApp && event.rowData === this.selectedApp ) {
      this.closeDetails();
    } else {
      this.router.navigate(['/security/securityapps'],
        { queryParams: { app:  event.rowData.meta.name }});
    }
  }

  closeDetails() {
    this.router.navigate(['/security/securityapps']);
  }

  dateToString(date) {
    const prettyDate = new PrettyDatePipe('en-US');
    return prettyDate.transform(date);
  }

}
