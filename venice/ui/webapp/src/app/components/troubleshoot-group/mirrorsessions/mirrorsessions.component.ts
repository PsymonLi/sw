import { ChangeDetectorRef, Component, OnInit, ViewEncapsulation, ViewChild, ChangeDetectionStrategy } from '@angular/core';
import { Animations } from '@app/animations';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { DSCsNameMacMap, ObjectsRelationsUtility } from '@app/common/ObjectsRelationsUtility';
import { Utility } from '@app/common/Utility';
import { CustomExportMap, TableCol } from '@app/components/shared/tableviewedit';
import { TablevieweditAbstract } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { ClusterService } from '@app/services/generated/cluster.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { SecurityService } from '@app/services/generated/security.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { ClusterDistributedServiceCard } from '@sdk/v1/models/generated/cluster';
import { IApiStatus, IMonitoringMirrorSession, MonitoringAppProtoSelector, MonitoringMatchRule, MonitoringMatchSelector, MonitoringMirrorCollector, MonitoringMirrorSession } from '@sdk/v1/models/generated/monitoring';
import { ILabelsSelector } from '@sdk/v1/models/generated/monitoring/labels-selector.model';
import { SecurityApp } from '@sdk/v1/models/generated/security';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { SelectItem } from 'primeng/api';
import { Observable } from 'rxjs';
import { NetworkService } from '@app/services/generated/network.service';
import { NetworkNetworkInterface } from '@sdk/v1/models/generated/network';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';

interface MirrorsessionsUIModel {
  pendingDSCmacnameMap?: {[key: string]: string };
 }

@Component({
  selector: 'app-mirrorsessions',
  templateUrl: './mirrorsessions.component.html',
  styleUrls: ['./mirrorsessions.component.scss'],
  animations: [Animations],
  changeDetection: ChangeDetectionStrategy.OnPush,
  encapsulation: ViewEncapsulation.None
})
export class MirrorsessionsComponent extends DataComponent implements OnInit {

  @ViewChild('mirrorSessionTable') mirrorSessionTable: PentableComponent;

  dataObjects: ReadonlyArray<MonitoringMirrorSession> = [];

  bodyicon: any = {
    margin: {
      top: '9px',
      left: '8px',
    },
    url: '/assets/images/icons/monitoring/ico-mirror-session-black.svg',
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
    { field: 'meta.name', header: 'Name', sortable: true, width: '125px', notReorderable: true },
    { field: 'spec.span-id', header: 'ERSPAN ID', sortable: true, width: '80px' },
    { field: 'spec.packet-size', header: 'Packet Size', sortable: true, width: '80px' },
    // { field: 'spec.packet-filters', header: 'Packet Filters', sortable: false, width: 10 },
    { field: 'spec.collectors', header: 'Collectors', sortable: false, width: 100 },
    { field: 'spec.match-rules', header: 'Match Rules', sortable: false, width: 100 },
    { field: 'spec.interfaces.selectors', header: 'Uplink Interface Selectors', sortable: false, width: 100 },
    { field: 'status.propagation-status', header: 'Propagation Status', sortable: true, width: 100 },
    { field: 'meta.creation-time', header: 'Creation Time', sortable: true, width: '180px', notReorderable: true },
    { field: 'meta.mod-time', header: 'Update Time', sortable: true, width: '180px', notReorderable: true },
    // { field: 'status.oper-state', header: 'OP Status', sortable: true, width: '175px' }
  ];

  exportFilename: string = 'PSM-mirrorsessons';
  exportMap: CustomExportMap = {
    'spec.collectors': (opts): string => {
      const rowData = opts.data;
      return this.displayColumn_collectors(rowData.spec.collectors, true);
    },
    'spec.match-rules': (opts): string => {
      const rowData = opts.data;
      return this.displayColumn_matchRules(rowData.spec['match-rules'], true);
    },
    'status.propagation-status': (opts): string => {
      const rowData = opts.data;
      return this.displayColumn_propagation(
        rowData.status['propagation-status'],
        rowData._ui.pendingDSCmacnameMap, false);
    },
    'spec.interfaces.selectors': (opts): string => {
      const rowData = opts.data;
      if (rowData.spec.interfaces && Array.isArray(rowData.spec.interfaces.selectors) &&
          rowData.spec.interfaces.selectors.length) {
        return this.displayColumn_interfaceselectors(
          rowData.spec.interfaces.direction,
          rowData.spec.interfaces.selectors[0], false);
      }
      return '';
    }
  };

  isTabComponent = false;
  disableTableWhenRowExpanded = true;

  securityApps: ReadonlyArray<SecurityApp> = [];
  securityAppOptions: SelectItem[] = [];
  naplesList: ClusterDistributedServiceCard[] = [];
  interfaces: NetworkNetworkInterface[] = [];

  constructor(protected controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected cdr: ChangeDetectorRef,
    protected securityService: SecurityService,
    protected clusterService: ClusterService,
    protected networkService: NetworkService,
    protected monitoringService: MonitoringService) {
    super(controllerService, uiconfigsService);
  }

  ngOnInit() {
    super.ngOnInit();
    this.penTable = this.mirrorSessionTable;
    this.tableLoading = true;
    this.getMirrorSessions();
    this.getDSCs();
    this.getNetworkInterfaces();
  }

  getDSCs() {
    const dscSubscription = this.clusterService.ListDistributedServiceCardCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.naplesList = response.data as ClusterDistributedServiceCard[];
        this.handleDataReady();
        this.refreshGui(this.cdr);
      }
    );
    this.subscriptions.push(dscSubscription);
  }

  getMirrorSessions() {
    const sub = this.monitoringService.ListMirrorSessionCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        this.tableLoading = false;
        this.dataObjects = response.data as MonitoringMirrorSession[];
        this.handleDataReady();
        this.refreshGui(this.cdr);
      },
      this.controllerService.webSocketErrorHandler('Failed to get Mirror Sessions')
    );
    this.subscriptions.push(sub);
  }

  getNetworkInterfaces() {
    const sub = this.networkService.ListNetworkInterfaceCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.interfaces = response.data as NetworkNetworkInterface[];
        this.refreshGui(this.cdr);
      }
    );
    this.subscriptions.push(sub);
  }

  /**
   * This API is used in getXXX().
   * Once mirrorsession (dataObjects[i]) has status['propagation-status']['pending-dscs'] = [aaaa.bbbb.cccc, xxxx.yyyy.zzz]
   * We build map { mac : dsc-name}
   */
  handleDataReady() {
    // When naplesList is ready, build DSC-maps
    if (this.naplesList && this.naplesList.length > 0 && this.dataObjects && this.dataObjects.length > 0) {
      const _myDSCnameToMacMap: DSCsNameMacMap = ObjectsRelationsUtility.buildDSCsNameMacMap(this.naplesList);
      const macToNameMap = _myDSCnameToMacMap.macToNameMap;
      this.dataObjects.forEach ( (flowExportPoliy) => {
        if (flowExportPoliy.status['propagation-status'] && flowExportPoliy.status['propagation-status']['pending-dscs']) {
          const propagationStatusDSCName = {};
          flowExportPoliy.status['propagation-status']['pending-dscs'].forEach( (mac: string)  => {
            propagationStatusDSCName[mac] = macToNameMap[mac];
          });
          const uiModel: MirrorsessionsUIModel = { pendingDSCmacnameMap : propagationStatusDSCName};
          flowExportPoliy._ui = uiModel;
        }
      });
      this.dataObjects = [...this.dataObjects];
    }
  }

  setDefaultToolbar(): void {
    let buttons = [];
    if (this.uiconfigsService.isAuthorized(UIRolePermissions.monitoringmirrorsession_create)) {
      buttons = [{
        cssClass: 'global-button-primary mirrorsessions-toolbar-button mirrorsessions-toolbar-button-ADD',
        text: 'ADD MIRROR SESSION',
        computeClass: () => this.dataObjects.length < 8 && !this.mirrorSessionTable.showRowExpand ? '' : 'global-button-disabled',
        callback: () => { this.mirrorSessionTable.createNewObject(); }
      }];
    }
    this.controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [{ label: 'Mirror Sessions', url: Utility.getBaseUIUrl() + 'troubleshoot/mirrorsessions' }]
    });
  }

  deleteRecord(object: MonitoringMirrorSession): Observable<{ body: IMonitoringMirrorSession | IApiStatus | Error; statusCode: number; }> {
    return this.monitoringService.DeleteMirrorSession(object.meta.name);
  }

  displayColumn(data: MonitoringMirrorSession, col: TableCol): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(data, fields);
    const column = col.field;
    switch (column) {
      case 'spec.interfaces.selectors':
        // currently we pick the 1st selector from array
        if (Array.isArray(value) && value.length) {
          return this.displayColumn_interfaceselectors(data.spec.interfaces.direction, value[0]);
        }
        return '';
      case 'spec.collectors':
        return this.displayColumn_collectors(value, false);
      case 'spec.match-rules':
        return this.displayColumn_matchRules(value);
      case 'status.propagation-status':
        return this.displayColumn_propagation(value, data._ui.pendingDSCmacnameMap );
      default:
         return Array.isArray(value) ? value.join(', ') : value;
    }
  }
  displayColumn_propagation(data,  dscMacNameMap: {[key: string]: string }, htmlFormat: boolean = true ) {
    return this.displayListInColumn(Utility.formatPropagationColumn(data, dscMacNameMap, htmlFormat));
  }
  displayMatchRules(rule: MonitoringMatchRule): string {
    const arr: string[] = [];
    if (rule) {
      const source: MonitoringMatchSelector = rule.source;
      const destination: MonitoringMatchSelector = rule.destination;
      const appProt: MonitoringAppProtoSelector = rule['app-protocol-selectors'];
      if (source['ip-addresses'] && source['ip-addresses'].length > 0) {
        arr.push('Source IPs: ' + source['ip-addresses'].join(', '));
      }
      if (source['mac-addresses'] && source['mac-addresses'].length > 0) {
        arr.push('Source MACs: ' + source['mac-addresses'].join(', '));
      }
      if (destination['ip-addresses'] && destination['ip-addresses'].length > 0) {
        arr.push('Destination IPs: ' + destination['ip-addresses'].join(', '));
      }
      if (destination['mac-addresses'] && destination['mac-addresses'].length > 0) {
        arr.push('Destination MACs: ' + destination['mac-addresses'].join(', '));
      }
      if (appProt['proto-ports'] && appProt['proto-ports'].length > 0) {
        arr.push('Protocols: ' + appProt['proto-ports'].join(', '));
      } else if (appProt.applications && appProt.applications.length > 0 &&
          this.securityAppOptions.length > 0) {
        const appNames: string[] = [];
        appProt.applications.forEach(appId => {
          const appObj: SelectItem = this.securityAppOptions.find(item => item.value === appId);
          if (appObj) {
            appNames.push(appObj.label);
          }
        });
        if (appNames.length > 0) {
          arr.push('Applications: ' + appNames.join(', '));
        }
      }
    }
    return arr.join('; ');
  }

  displayColumn_matchRules(value: MonitoringMatchRule[], isForExport: boolean = false): string {
    const list: string[] = value.map(item => this.displayMatchRules(item));
    return isForExport ? this.displayListInLinesInCvsColumn(list) : this.displayListInLinesInColumn(list);
  }

  displayColumn_collectors(value: MonitoringMirrorCollector[], isForExport: boolean = false): string {
    const list: string[] = value.map(item => {
      let str: string = item.type;
      if (item['strip-vlan-hdr']) {
        str += ', Strip VLAN';
      }
      str += ', Destination: ' + item['export-config'].destination;
      if (item['export-config'].gateway) {
        str += ', Gateway: ' + item['export-config'].gateway;
      }
      return str;
    });
    if (isForExport) {
      return this.displayListInLinesInCvsColumn(list);
    }
    return this.displayListInLinesInColumn(list);
  }

  displayColumn_interfaceselectors(direction: string, value: ILabelsSelector, htmlFormat: boolean = true): string {
    const result: string[] = [];
    if (value && value.requirements && value.requirements.length > 0) {
      value.requirements.forEach(selector => {
        const icon = (selector.operator === 'in') ?
            '<div title="In" class="mirrorsessions-column-in-icon"></div>' :
            '<div title="Not In" class="mirrorsessions-column-notin-icon"></div>';
        if (htmlFormat) {
          result.push('<span>' + selector.key + icon + '[' + selector.values.join(',') + ']</span>');
        } else {
          result.push(selector.key + ' ' + selector.operator + ' [' + selector.values.join(',') + ']');
        }
      });
    }
    if (htmlFormat) {
      let directionIcon = '<div title="Direction: Both" class="mirrorsessions-column-txrx-icon"></div>';
      if (direction === 'tx') {
        directionIcon = '<div title="Direction: TX" class="mirrorsessions-column-tx-icon"></div>';
      } else if (direction === 'rx') {
        directionIcon = '<div title="Direction: RX" class="mirrorsessions-column-rx-icon"></div>';
      }
      return directionIcon + ', ' + result.join(', ');
    }
    return 'Direction: ' + direction + ', ' + result.join(', ');
  }

  displayListInLinesInColumn(list: string[]): string {
    return (list && list.length > 0) ? list.join('<br>') : '';
  }

  displayListInLinesInCvsColumn(list: string[]): string {
    return (list && list.length > 0) ? list.join('\n') : '';
  }

  displayListInColumn(list: string[]): string {
    return (list && list.length > 0) ?
        list.reduce((accum: string, item: string) => accum + ' ' + item ) : '';
  }
}
