import { ChangeDetectorRef, Component, OnInit, ViewEncapsulation, ViewChild, ChangeDetectionStrategy } from '@angular/core';
import { Animations } from '@app/animations';
import { CustomExportMap, TableCol } from '@app/components/shared/tableviewedit';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { OrchestrationService } from '@app/services/generated/orchestration.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { Utility } from '@common/Utility';
import { TablevieweditAbstract } from '@components/shared/tableviewedit/tableviewedit.component';
import { IApiStatus, OrchestrationOrchestrator, IOrchestrationOrchestrator } from '@sdk/v1/models/generated/orchestration';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import * as _ from 'lodash';
import { Observable, Subscription } from 'rxjs';
import { WorkloadWorkload } from '@sdk/v1/models/generated/workload';
import { WorkloadService } from '@app/services/generated/workload.service';
import { VcenterWorkloadsTuple, ObjectsRelationsUtility } from '@app/common/ObjectsRelationsUtility';
import { WorkloadUtility, WorkloadNameInterface } from '@app/common/WorkloadUtility';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { AdvancedSearchComponent } from '@app/components/shared/advanced-search/advanced-search.component';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { FormArray } from '@angular/forms';
import { FieldsRequirement } from '@sdk/v1/models/generated/search';
import { TableUtility } from '@app/components/shared/tableviewedit/tableutility';
import { SearchUtil } from '@app/components/search/SearchUtil';

interface VcenterUIModel {
  associatedWorkloads: WorkloadWorkload[];
  associatedDatacenters: string[];
}

/**
 * vCenter page.
 * UI fetches all vcenter objects.
 *
 */
@Component({
  selector: 'app-vcenter-integrations',
  encapsulation: ViewEncapsulation.None,
  templateUrl: './vcenterIntegrations.component.html',
  styleUrls: ['./vcenterIntegrations.component.scss'],
  changeDetection: ChangeDetectionStrategy.OnPush,
  animations: [Animations]
})

export class VcenterIntegrationsComponent extends DataComponent implements OnInit {

  @ViewChild('advancedSearchComponent') advancedSearchComponent: AdvancedSearchComponent;
  @ViewChild('vCenterIntegrationTable') vCenterIntegrationTable: PentableComponent;

  bodyicon: Icon = {
    margin: {
      top: '9px',
      left: '8px'
    },
    svgIcon: 'vcenter'
  };

  headerIcon: Icon = {
    margin: {
      top: '0px',
      left: '10px',
    },
    matIcon: 'computer'
  };

  exportFilename: string = 'PSM-vcenter-integrations';

  exportMap: CustomExportMap = {
    'associatedWorkloads': (opts): string => {
      return (opts.data._ui.associatedWorkloads) ? opts.data._ui.associatedWorkloads.map(wkld => wkld.meta.name).join(', ') : '';
    },
    ['spec.manage-namespaces'] : (opts): string => {
      console.log(opts.data);
      return (opts.data._ui.associatedDatacenters) ? opts.data._ui.associatedDatacenters.join(', ') : '';
    }
  };

  subscriptions: Subscription[] = [];
  dataObjects: ReadonlyArray<OrchestrationOrchestrator> = [];

  workloadList: WorkloadWorkload[] = [];

  disableTableWhenRowExpanded: boolean = true;
  isTabComponent: boolean = false;

  // Used for the table - when true there is a loading icon displayed
  tableLoading: boolean = false;

  cols: TableCol[] = [
    { field: 'meta.name', header: 'Name', class: 'vcenter-integration-column-name', sortable: true, width: '150px' },
    { field: 'meta.creation-time', header: 'Creation Time', class: 'vcenter-integration-column-date', sortable: true, width: '180px' },
    { field: 'spec.uri', header: 'FQDN', class: 'vcenter-integration-column-url', sortable: true, width: '150px' },
    { field: 'spec.manage-namespaces', header: 'Managed Datacenters', class: 'vcenter-integration-column-namespaces', sortable: false, width: 30 },
    { field: 'associatedWorkloads', header: 'Workloads', class: '', sortable: false, localSearch: true, width: 40 },
    { field: 'status.orch-id', header: 'Orchestrator ID', class: 'vcenter-integration-column-orchid', sortable: true, width: '120px' },
    { field: 'status.connection-status', header: 'Connection Status', class: 'vcenter-integration-column-status', sortable: true, width: 30 },
    { field: 'status.last-transition-time', header: 'Connection Transition Time', class: 'vcenter-integration-column-lastconnected', sortable: true, width: '180px' },
  ];
  maxSearchRecords: number = 8000;
  advSearchCols: TableCol[] = [];
  fieldFormArray = new FormArray([]);

  dataObjectsBackUp: ReadonlyArray<OrchestrationOrchestrator> = [];

  constructor(private orchestrationService: OrchestrationService,
    protected cdr: ChangeDetectorRef,
    protected uiconfigsService: UIConfigsService,
    protected workloadService: WorkloadService,
    protected controllerService: ControllerService) {
    super(controllerService, uiconfigsService);
  }

  ngOnInit() {
    super.ngOnInit();
    this.penTable = this.vCenterIntegrationTable;
    this.tableLoading = true;
    this.getVcenterIntegrations();
    this.watchWorkloads();
    this.buildAdvSearchCols();
  }

   /**
  * Execute table search
  * @param field
  * @param order
  */
  onSearchvCenterIntegrations(field = this.vCenterIntegrationTable.sortField, order = this.vCenterIntegrationTable.sortOrder) {
    const searchResults = this.onSearchDataObjects(field, order, 'Orchestrator', this.maxSearchRecords, this.advSearchCols, this.dataObjectsBackUp, this.vCenterIntegrationTable.advancedSearchComponent);
    if (searchResults && searchResults.length > 0) {
      this.dataObjects = searchResults;
    }
  }

   // advance search APIs
  onCancelSearch($event) {
    this.controllerService.invokeInfoToaster('Information', 'Cleared search criteria, Table refreshed.');
    this.dataObjects = this.dataObjectsBackUp;
  }

  getVcenterIntegrations() {
    const sub = this.orchestrationService.ListOrchestratorCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        this.dataObjectsBackUp = response.data;
        this.buildVCenterDatacenters();
        this.buildVCenterWorkloadsMap();
        this.tableLoading = false;
        if (!this.vCenterIntegrationTable.isDisabled()) {
          this.dataObjects = [...this.dataObjectsBackUp];
        }
        this.cdr.detectChanges();
      },
      this.controllerService.webSocketErrorHandler('Failed to get vCenters')
    );
    this.subscriptions.push(sub);
  }

  setDefaultToolbar() {
    let buttons = [];
    if (this.uiconfigsService.isAuthorized(UIRolePermissions.orchestrationorchestrator_create)) {
      buttons = [{
        cssClass: 'global-button-primary vcenter-integrations-button vcenter-integrations-button-ADD',
        text: 'ADD VCENTER',
        computeClass: () => !this.vCenterIntegrationTable.showRowExpand ? '' : 'global-button-disabled',
        callback: () => { this.vCenterIntegrationTable.createNewObject(); }
      }];
    }
    this.controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [{ label: 'vCenter ', url: Utility.getBaseUIUrl() + 'controller/vcenterintegrations' }]
    });
  }

  displayColumn(rowData: OrchestrationOrchestrator, col: TableCol): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(rowData, fields);
    const column = col.field;
    switch (column) {
      default:
        return Array.isArray(value) ? JSON.stringify(value, null, 2) : value;
    }
  }

  hasWorkloads(rowData: OrchestrationOrchestrator): boolean {
    const workloads = rowData._ui.associatedWorkloads;
    return workloads && workloads.length > 0;
  }

  /**
   * Fetch workloads.
   */
  watchWorkloads() {
    const workloadSubscription = this.workloadService.ListWorkloadCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.workloadList = response.data as WorkloadWorkload[];
        this.buildVCenterWorkloadsMap();
        if (!this.vCenterIntegrationTable.isDisabled()) {
          this.dataObjects = [...this.dataObjectsBackUp];
        }
        this.cdr.detectChanges();
      }
    );
    this.subscriptions.push(workloadSubscription);
  }

  buildVCenterDatacenters() {
    if (!Utility.isValueOrArrayEmpty(this.dataObjectsBackUp)) {
      this.dataObjectsBackUp.forEach(vcenter => {
        let datacenters: string[] = vcenter.spec['manage-namespaces'];
        if (datacenters && datacenters.length === 1 && datacenters[0] === 'all_namespaces') {
          datacenters = ['All Datacenters'];
        }
        const uiModel: VcenterUIModel = {
          associatedDatacenters: datacenters,
          associatedWorkloads: vcenter._ui.associatedWorkloads
        };
        vcenter._ui = uiModel;
      });
      if (!this.vCenterIntegrationTable.isShowRowExpand()) {
        this.dataObjects = this.dataObjectsBackUp;
        this.cdr.detectChanges();
      }
    }
  }

  buildVCenterWorkloadsMap() {
    if (!Utility.isValueOrArrayEmpty(this.dataObjectsBackUp) &&
        !Utility.isValueOrArrayEmpty(this.workloadList)) {
      const vcenterWorkloadsTuple: VcenterWorkloadsTuple =
        ObjectsRelationsUtility.buildVcenterWorkloadsMap(this.workloadList, this.dataObjects);
      this.dataObjectsBackUp = this.dataObjectsBackUp.map(vcenter => {
        const associatedWorkloads: WorkloadWorkload[] =
          vcenterWorkloadsTuple[vcenter.meta.name] || [];
        const uiModel: VcenterUIModel = {
          associatedWorkloads,
          associatedDatacenters: vcenter._ui.associatedDatacenters
        };
        vcenter._ui = uiModel;
        return vcenter;
      });
      if (!this.vCenterIntegrationTable.isShowRowExpand()) {
        this.dataObjects = this.dataObjectsBackUp;
        this.cdr.detectChanges();
      }
    }
  }

  deleteRecord(object: OrchestrationOrchestrator): Observable<{ body: IOrchestrationOrchestrator | IApiStatus | Error; statusCode: number }> {
    return this.orchestrationService.DeleteOrchestrator(object.meta.name);
  }

  buildAdvSearchCols() {
    this.advSearchCols = this.cols.filter((col: TableCol) => {
      return (col.field !== 'meta.creation-time'  && col.field !== 'status.last-transition-time'  && col.field !== 'associatedWorkloads' );
    });

    this.advSearchCols.push({
      field: 'associatedWorkloads', header: 'Workloads', localSearch: true, kind: 'Orchestrator',
      filterfunction: this.searchWorkloads,
      advancedSearchOperator: SearchUtil.stringOperators
    });
  }

  searchWorkloads(requirement: FieldsRequirement, data = this.dataObjects): any[] {
    const outputs: any[] = [];
    for (let i = 0; data && i < data.length; i++) {
      const workloads = (data[i]._ui as VcenterUIModel).associatedWorkloads;
      for (let k = 0; k < workloads.length; k++) {
        const recordValueName = _.get(workloads[k], ['meta', 'name']);
        const searchValues = requirement.values;
        let operator = String(requirement.operator);
        operator = TableUtility.convertOperator(operator);
        for (let j = 0; j < searchValues.length; j++) {
          const activateFunc = TableUtility.filterConstraints[operator];
          if (activateFunc && activateFunc(recordValueName, searchValues[j])) {
            outputs.push(data[i]);
          }
        }
      }
    }
    return outputs;
  }

  editFormClose(rowData) {
    if (this.vCenterIntegrationTable.showRowExpand) {
      this.vCenterIntegrationTable.toggleRow(rowData);
      if (this.dataObjectsBackUp !== this.dataObjects) {
        this.dataObjects = this.dataObjectsBackUp;
      }
    }
  }
}
