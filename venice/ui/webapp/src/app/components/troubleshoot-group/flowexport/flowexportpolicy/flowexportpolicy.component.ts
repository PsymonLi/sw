import { ChangeDetectorRef, Component, Input, ViewEncapsulation, IterableDiffer, IterableDiffers, DoCheck, ChangeDetectionStrategy, ViewChild, OnInit } from '@angular/core';
import { Animations } from '@app/animations';
import { Utility } from '@app/common/Utility';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { IApiStatus, IMonitoringFlowExportPolicy, IMonitoringMatchRule, MonitoringFlowExportPolicy, IMonitoringExportConfig } from '@sdk/v1/models/generated/monitoring';
import { Observable, Subscription } from 'rxjs';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { TableCol, CustomExportMap } from '@app/components/shared/tableviewedit';
import { SyslogUtility } from '@app/common/SyslogUtility';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';
import { ClusterDistributedServiceCard } from '@sdk/v1/models/generated/cluster';
import { DSCsNameMacMap, ObjectsRelationsUtility } from '@app/common/ObjectsRelationsUtility';
import { ClusterService } from '@app/services/generated/cluster.service';

interface FlowExportPolicyUIModel {
  pendingDSCmacnameMap?: {[key: string]: string };
}

@Component({
  selector: 'app-flowexportpolicy',
  templateUrl: './flowexportpolicy.component.html',
  styleUrls: ['./flowexportpolicy.component.scss'],
  animations: [Animations],
  changeDetection: ChangeDetectionStrategy.OnPush,
  encapsulation: ViewEncapsulation.None
})
export class FlowexportpolicyComponent extends DataComponent implements OnInit {
  public static MAX_TARGETS_PER_POLICY = 2;
  public static MAX_TOTAL_TARGETS = 8;

  @ViewChild('flowExportPolicyTable') flowExportPolicyTable: PentableComponent;

  dataObjects: ReadonlyArray<MonitoringFlowExportPolicy> = [];

  subscriptions: Subscription[] = [];
  naplesList: ClusterDistributedServiceCard[] = [];

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
    { field: 'meta.name', header: 'Name', class: 'flowexportpolicy-column-name', sortable: false, width: '150px' },
    { field: 'spec.interval', header: 'Interval', class: 'flowexportpolicy-column-interval', sortable: false, width: '100px' },
    { field: 'spec.template-interval', header: 'Template Refresh Rate', class: 'flowexportpolicy-template-interval', sortable: false, width: '145px' },
    { field: 'spec.match-rules', header: 'Match Rules', class: 'flowexportpolicy-column-match-rules', sortable: false, width: 100 },
    { field: 'spec.exports', header: 'Targets', class: 'flowexportpolicy-column-targets', sortable: false, width: 100 },
    { field: 'status.propagation-status', header: 'Propagation Status', sortable: true, width: 100 }
  ];

  exportFilename: string = 'PSM-flow-export-policies';
  exportMap: CustomExportMap = {
    'spec.match-rules': (opts): string => {
      const value = Utility.getObjectValueByPropertyPath(opts.data, opts.field);
      const resArr: any = this.formatMatchRules(value);
      return resArr ? resArr.join('\n') : '';
    },
    'spec.exports': (opts): string => {
      const value = Utility.getObjectValueByPropertyPath(opts.data, opts.field);
      const resArr =  SyslogUtility.formatTargets(value, true);
      return resArr.toString();
    },
    'status.propagation-status': (opts): string => {
      const rowData = opts.data;
      const value = Utility.getObjectValueByPropertyPath(opts.data, opts.field);
      return this.displayColumn_propagation(value, rowData._ui.pendingDSCmacnameMap, false);
    }
  };
  isTabComponent = false;
  disableTableWhenRowExpanded = true;
  maxNewTargets: number = FlowexportpolicyComponent.MAX_TARGETS_PER_POLICY;

  constructor(protected controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected cdr: ChangeDetectorRef,
    protected monitoringService: MonitoringService,
    protected clusterService: ClusterService) {
    super(controllerService, uiconfigsService);
  }

  ngOnInit() {
    super.ngOnInit();
    this.penTable = this.flowExportPolicyTable;
    this.tableLoading = true;
    this.getFlowExportPolicies();
    this.getDSCs();
  }

  /**
   * This API is used in getXXX().
   * Once flowExportPolicies has status['propagation-status']['pending-dscs'] = [aaaa.bbbb.cccc, xxxx.yyyy.zzz]
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
          const uiModel: FlowExportPolicyUIModel = { pendingDSCmacnameMap : propagationStatusDSCName};
          flowExportPoliy._ui = uiModel;
        }
      });
      this.dataObjects = [...this.dataObjects]; // tell angular to refreh refreence
      this.maxNewTargets = this.computetargets();
    }
  }

  computetargets(): number {
    let totaltargets: number = 0;
    for (const i of this.dataObjects) {
      totaltargets += i.spec.exports.length;
    }
    const remainder = FlowexportpolicyComponent.MAX_TOTAL_TARGETS - totaltargets;
    return Math.min(remainder, FlowexportpolicyComponent.MAX_TARGETS_PER_POLICY);
  }

  getDSCs() {
    const dscSubscription = this.clusterService.ListDistributedServiceCardCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.naplesList = response.data as ClusterDistributedServiceCard[];
        this.handleDataReady();
        this.cdr.detectChanges();
      }
    );
    this.subscriptions.push(dscSubscription);
 }

  getFlowExportPolicies() {
    const dscSubscription = this.monitoringService.ListFlowExportPolicyCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.tableLoading = false;
        this.dataObjects = response.data as MonitoringFlowExportPolicy[];
        this.handleDataReady();
        this.cdr.detectChanges();
      }
    );
    this.subscriptions.push(dscSubscription);
  }

  setDefaultToolbar() {
    let buttons = [];
    if (this.uiconfigsService.isAuthorized(UIRolePermissions.monitoringflowexportpolicy_create)) {
      buttons = [{
        cssClass: 'global-button-primary flowexportpolicy-button',
        text: 'ADD FlOW EXPORT',
        genTooltip: () => this.getTooltip(),
        computeClass: () => !this.flowExportPolicyTable.showRowExpand && this.maxNewTargets > 0 ? '' : 'global-button-disabled',
        callback: () => { this.flowExportPolicyTable.createNewObject(); }
      }];
    }
    this.controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [
        { label: 'Flow Export Policies', url: Utility.getBaseUIUrl() + 'troubleshoot/flowexport' }
      ]
    });
  }

  getTooltip(): string {
    return this.maxNewTargets === 0 ? 'Cannot exceed 8 targets across policies' : '';
  }

  deleteRecord(object: MonitoringFlowExportPolicy): Observable<{ body: IMonitoringFlowExportPolicy | IApiStatus | Error, statusCode: number }> {
    return this.monitoringService.DeleteFlowExportPolicy(object.meta.name);
  }

  displayColumn(exportData: MonitoringFlowExportPolicy, col: TableCol): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(exportData, fields);
    const column = col.field;
    switch (column) {
      case 'spec.exports':
        return SyslogUtility.formatTargets(value);
      case 'status.propagation-status':
        return this.displayColumn_propagation(value, exportData._ui.pendingDSCmacnameMap, true);
      default:
        return Array.isArray(value) ? JSON.stringify(value, null, 2) : value;
    }
  }

  displayColumn_propagation(data,  dscMacNameMap: {[key: string]: string}, htmlFormat: boolean = true) {
    return this.displayListInColumn(Utility.formatPropagationColumn(data, dscMacNameMap, htmlFormat));
  }

  displayListInColumn(list: string[]): string {
    return (list && list.length > 0) ?
        list.reduce((accum: string, item: string) => accum + item ) : '';
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
        if (req.source.hasOwnProperty(k) && k !== '_ui') {
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
        if (req.destination.hasOwnProperty(k)  && k !== '_ui') {
          const v: string[] = req.destination[k];
          dest = dest.concat(v);
        }
      }
      if (dest.length === 0) {
        ret += '*';
      } else {
        ret += dest.join(', ');
      }

      let apps = [];
      apps = apps.concat(req['app-protocol-selectors']['proto-ports']);
      if (apps.length > 0) {
        ret += '    Protocol / Ports: ' + apps.join(', ');
      }
      retArr.push(ret);
    });
    return retArr;
  }

}
