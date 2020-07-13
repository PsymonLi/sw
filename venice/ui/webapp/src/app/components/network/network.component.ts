import { Component, OnInit, ViewEncapsulation, ChangeDetectorRef, ViewChild, ChangeDetectionStrategy } from '@angular/core';
import { Animations } from '@app/animations';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { IApiStatus, NetworkNetwork, INetworkNetwork, NetworkOrchestratorInfo } from '@sdk/v1/models/generated/network';
import { CustomExportMap, TableCol } from '@app/components/shared/tableviewedit';
import { Subscription, Observable } from 'rxjs';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { NetworkService } from '@app/services/generated/network.service';
import { OrchestrationService } from '@app/services/generated/orchestration.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { ControllerService } from '@app/services/controller.service';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { Utility } from '@app/common/Utility';
import { OrchestrationOrchestrator } from '@sdk/v1/models/generated/orchestration';
import { SelectItem } from 'primeng/api';
import { WorkloadWorkload } from '@sdk/v1/models/generated/workload';
import { WorkloadService } from '@app/services/generated/workload.service';
import { NetworkWorkloadsTuple, ObjectsRelationsUtility } from '@app/common/ObjectsRelationsUtility';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { FormArray } from '@angular/forms';
import { FieldsRequirement } from '@sdk/v1/models/generated/search';
import { TableUtility } from '@app/components/shared/tableviewedit/tableutility';
import { SearchUtil } from '@app/components/search/SearchUtil';
import { IStagingBulkEditAction } from '@sdk/v1/models/generated/staging';
import { LabelEditorMetadataModel } from '@app/components/shared/labeleditor';
import * as _ from 'lodash';

interface NetworkUIModel {
  associatedWorkloads: WorkloadWorkload[];
}

@Component({
  selector: 'app-network',
  encapsulation: ViewEncapsulation.None,
  templateUrl: './network.component.html',
  styleUrls: ['./network.component.scss'],
  changeDetection: ChangeDetectionStrategy.OnPush,
  animations: [Animations]
})

export class NetworkComponent extends DataComponent implements OnInit {

  @ViewChild('networkTable') networkTable: PentableComponent;

  bodyicon: Icon = {
    margin: {
      top: '9px',
      left: '8px'
    },
    svgIcon: 'host'
  };

  headerIcon: Icon = {
    margin: {
      top: '0px',
      left: '10px',
    },
    matIcon: 'computer'
  };

  exportFilename: string = 'PSM-networks';

  exportMap: CustomExportMap = {
    'associatedWorkloads': (opts): string => {
      return (opts.data._ui.associatedWorkloads) ? opts.data._ui.associatedWorkloads.map(wkld => wkld.meta.name).join(', ') : '';
    },
    'spec.orchestrators': (opts): string => {
      return (opts.data.spec.orchestrators) ? opts.data.spec.orchestrators.map(or => or['orchestrator-name'] + ' Datacenter: ' + or.namespace).join(', ') : '';
    },
    'meta.labels': (opts): string => {
      return this.formatLabels(opts.data.meta.labels);
    }
  };

  maxSearchRecords: number = 8000;

  vcenters: ReadonlyArray<OrchestrationOrchestrator> = [];
  vcenterOptions: SelectItem[] = [];

  workloadList: WorkloadWorkload[] = [];

  subscriptions: Subscription[] = [];
  dataObjects: ReadonlyArray<NetworkNetwork> = [];
  dataObjectsBackup: ReadonlyArray<NetworkNetwork> = [];
  networkEventUtility: HttpEventUtility<NetworkNetwork>;

  disableTableWhenRowExpanded: boolean = true;
  isTabComponent: boolean = false;
  inLabelEditMode: boolean = false;
  labelEditorMetaData: LabelEditorMetadataModel;


  // Used for the table - when true there is a loading icon displayed
  tableLoading: boolean = false;

  cols: TableCol[] = [
    { field: 'meta.name', header: 'Name', class: 'network-column-name', sortable: true, width: 20 },
    { field: 'spec.vlan-id', header: 'VLAN', class: 'network-column-vlan', sortable: true, width: '80px'},
    { field: 'spec.orchestrators', header: 'Orchestrators', class: 'network-column-orchestrators', sortable: false, width: 35 },
    { field: 'associatedWorkloads', header: 'Workloads', class: '', sortable: false, width: 35 },
    // { field: 'meta.labels', header: 'Labels', class: '', sortable: true, width: 100 },
    { field: 'meta.creation-time', header: 'Creation Time', class: 'vcenter-integration-column-date', sortable: true, width: '180px' }
  ];

  // advance search variables
  advSearchCols: TableCol[] = [];
  fieldFormArray = new FormArray([]);

  constructor(private networkService: NetworkService,
    protected cdr: ChangeDetectorRef,
    protected uiconfigsService: UIConfigsService,
    protected orchestrationService: OrchestrationService,
    protected workloadService: WorkloadService,
    protected controllerService: ControllerService) {
    super(controllerService, uiconfigsService);
  }

  getNetworks() {
    const hostSubscription = this.networkService.ListNetworkCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.dataObjectsBackup = response.data;
        this.tableLoading = false;
        this.buildNetworkWorkloadsMap();
      },
      (error) => {
        this.tableLoading = false;
        this.controllerService.invokeRESTErrorToaster('Error', 'Failed to get networks');
      }
    );
    this.subscriptions.push(hostSubscription);
  }

  getVcenterIntegrations() {
    const sub = this.orchestrationService.ListOrchestratorCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        this.vcenters = response.data;
        this.vcenterOptions = this.vcenters.map(vcenter => {
          return {
            label: vcenter.meta.name,
            value: vcenter.meta.name
          };
        });
        this.cdr.detectChanges();
      },
      this.controllerService.webSocketErrorHandler('Failed to get vCenters')
    );
    this.subscriptions.push(sub);
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
        this.buildNetworkWorkloadsMap();
      }
    );
    this.subscriptions.push(workloadSubscription);
  }

  setDefaultToolbar() {
    let buttons = [];
    if (this.uiconfigsService.isAuthorized(UIRolePermissions.networknetwork_create)) {
      buttons = [{
        cssClass: 'global-button-primary networks-button networks-button-ADD',
        text: 'ADD NETWORK',
        computeClass: () =>  !(this.networkTable.showRowExpand) ? '' : 'global-button-disabled',
        callback: () => { this.networkTable.createNewObject(); this.cdr.detectChanges(); }
      }];
    }
    this.controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [{ label: 'Network ', url: Utility.getBaseUIUrl() + 'networks' }]
    });
  }

  displayColumn(rowData: NetworkNetwork, col: TableCol): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(rowData, fields);
    const column = col.field;
    switch (column) {
      case 'spec.orchestrators':
        return this.displayColumn_orchestrators(value);
      case 'spec.vlan-id':
        return value ? value : 0;
      case 'meta.labels':
            return this.formatLabels(rowData.meta.labels);
      default:
        return Array.isArray(value) ? JSON.stringify(value, null, 2) : value;
    }
  }

  hasWorkloads(rowData: NetworkNetwork): boolean {
    const workloads = rowData._ui.associatedWorkloads;
    return workloads && workloads.length > 0;
  }

  displayColumn_orchestrators(values: NetworkOrchestratorInfo[]): any {
    const map: {'vCenter': string, 'dataCenters': string[]} = {} as any;
    values.forEach((value: NetworkOrchestratorInfo) => {
      if (!map[value['orchestrator-name']]) {
        map[value['orchestrator-name']] = [value.namespace];
      } else {
        map[value['orchestrator-name']].push(value.namespace);
      }
    });
    let result: string = '';
    for (const key of Object.keys(map)) {
      const eachRow: string = 'vCenter: ' + key + ', Datacenter: ' + map[key].join(', ');
      result += '<div class="ellipsisText" title="' + eachRow + '">' + eachRow + '</div>';
    }
    return result;
  }

  ngOnInit() {
    this.tableLoading = true;
    this.setDefaultToolbar();
    this.buildAdvSearchCols();
    this.getNetworks();
    this.getVcenterIntegrations();
    this.watchWorkloads();
  }

  creationFormClose() {
    this.networkTable.creationFormClose();
  }

  editFormClose(rowData) {
    if (this.networkTable.showRowExpand) {
      this.networkTable.toggleRow(rowData);
      if (this.dataObjectsBackup !== this.dataObjects) {
        this.dataObjects = this.dataObjectsBackup;
      }
    }
  }

  expandRowRequest(event, rowData) {
    if (!this.networkTable.showRowExpand) {
      this.networkTable.toggleRow(rowData, event);
    }
  }

  onColumnSelectChange(event) {
    this.networkTable.onColumnSelectChange(event);
  }

  onDeleteRecord(event, object) {
    this.networkTable.onDeleteRecord(
      event,
      object,
      this.generateDeleteConfirmMsg(object),
      this.generateDeleteSuccessMsg(object),
      this.deleteRecord.bind(this),
      () => {
        this.networkTable.selectedDataObjects = [];
      }
    );
  }

  buildNetworkWorkloadsMap() {
    if (!Utility.isValueOrArrayEmpty(this.dataObjectsBackup)) {
      if (!Utility.isValueOrArrayEmpty(this.workloadList)) {
        const networkWorkloadsTuple: NetworkWorkloadsTuple =
          ObjectsRelationsUtility.buildNetworkWorkloadsMap(this.workloadList, this.dataObjectsBackup);
        this.dataObjectsBackup.forEach(network => {
          const associatedWorkloads: WorkloadWorkload[] =
          networkWorkloadsTuple[network.meta.name] || [];
          const uiModel: NetworkUIModel = { associatedWorkloads };
          network._ui = uiModel;
          return network;
        });
      }
      if (!this.networkTable.isShowRowExpand()) {
        this.dataObjects = this.dataObjectsBackup;
        this.cdr.detectChanges();
      }
    }
  }

  deleteRecord(object: NetworkNetwork): Observable<{ body: INetworkNetwork | IApiStatus | Error; statusCode: number }> {
    return this.networkService.DeleteNetwork(object.meta.name);
  }

  generateDeleteConfirmMsg(object: INetworkNetwork): string {
    return 'Are you sure you want to delete network ' + object.meta.name;
  }

  generateDeleteSuccessMsg(object: INetworkNetwork): string {
    return 'Deleted network ' + object.meta.name;
  }

  getSelectedDataObjects(): any[] {
    return this.networkTable.selectedDataObjects;
  }

  clearSelectedDataObjects() {
    this.networkTable.selectedDataObjects = [];
  }

  // api overrides for pentable and adv search

  // advance search columns
  buildAdvSearchCols() {
    this.advSearchCols = this.cols.filter((col: TableCol) => {
      return (col.field !== 'meta.creation-time' && col.field !== 'associatedWorkloads' && col.field !== 'spec.orchestrators');
    });
    this.advSearchCols.push({
      field: 'associatedWorkloads', header: 'Workloads', localSearch: true, kind: 'Workload',
      filterfunction: this.searchWorkloadNames,
      advancedSearchOperator: SearchUtil.stringOperators
    });
    this.advSearchCols.push({
      field: 'spec.orchestrators', header: 'Orchestrators', localSearch: true, kind: 'Orchestrator',
      filterfunction: this.searchOrchestrators,
      advancedSearchOperator: SearchUtil.stringOperators
    });
  }

  onCancelSearch() {
    this.controllerService.invokeInfoToaster('Information', 'Cleared search criteria, Table refreshed.');
    this.dataObjects = this.dataObjectsBackup;
  }

  searchWorkloadNames(requirement: FieldsRequirement, data = this.dataObjects): any[] {
    const outputs: any[] = [];
    const searchValues = requirement.values || [];
    const operator = TableUtility.convertOperator(String(requirement.operator));
    // if notcontains or notEquals, use the regular operator (contains/equals) and flip the results
    const isNot = operator.slice(0, 3) === 'not';
    const operatorAdjusted = isNot ? operator.slice(3).toLocaleLowerCase() : operator;
    const activateFunc = TableUtility.filterConstraints[operatorAdjusted];
    for (let i = 0; data && i < data.length; i++) {
      const workloads = (data[i]._ui as NetworkUIModel).associatedWorkloads || [];
      const found = workloads.some(workload => searchValues.some(value => activateFunc && activateFunc(workload.meta.name, value)));
      if (found) {
        outputs.push(data[i]);
      }
    }
    return isNot ? _.differenceWith(data, outputs, _.isEqual) : outputs;
  }

  searchOrchestrators(requirement: FieldsRequirement, data = this.dataObjects): any[] {
    const outputs: any[] = [];
    const searchValues = requirement.values || [];
    const operator = TableUtility.convertOperator(String(requirement.operator));
    // if notcontains or notEquals, use the regular operator (contains/equals) and flip the results
    const isNot = operator.slice(0, 3) === 'not';
    const operatorAdjusted = isNot ? operator.slice(3).toLocaleLowerCase() : operator;
    const activateFunc = TableUtility.filterConstraints[operatorAdjusted];
    for (let i = 0; data && i < data.length; i++) {
      const orchestrators = data[i].spec.orchestrators || [];
      const foundData = orchestrators.some(orch => {
        return searchValues.some(value => {
          return activateFunc && (activateFunc(orch['orchestrator-name'], value) || activateFunc(orch['namespace'], value));
        });
      });
      if (foundData) {
        outputs.push(data[i]);
      }
    }
    return isNot ? _.differenceWith(data, outputs, _.isEqual) : outputs;
  }

  /**
   * Execute table search
   * @param field
   * @param order
   */
  onSearchNetworks(field = this.networkTable.sortField, order = this.networkTable.sortOrder) {
    const searchResults = this.onSearchDataObjects(field, order, 'Network', this.maxSearchRecords, this.advSearchCols, this.dataObjectsBackup, this.networkTable.advancedSearchComponent);
    if (searchResults && searchResults.length > 0) {
      this.dataObjects = [];
      this.dataObjects = searchResults;
    }
  }

  onDeleteSelectedNetworks(event) {
    this.onDeleteSelectedRows(event);
  }

  /**
   * Override super's API.
   *
   * @param vObject
   * @param model
   * @param previousVal
   * @param trimDefaults
   */
  trimObjectForBulkeditItem(vObject: any, model, previousVal = null, trimDefaults = true ): any {
    const body = (vObject.hasOwnProperty('getModelValue')) ? vObject.getModelValues() : vObject;
    return Utility.trimDefaultsAndEmptyFieldsByModel( body, vObject, previousVal, trimDefaults);
  }

  editLabels() {
    this.labelEditorMetaData = {
      title: 'Edit Network Labels',
      keysEditable: true,
      valuesEditable: true,
      propsDeletable: true,
      extendable: true,
      save: true,
      cancel: true,
    };

    if (!this.inLabelEditMode) {
      this.inLabelEditMode = true;
    }
  }

  handleEditSave(networks: NetworkNetwork[]) {
    this.bulkeditLabels(networks);
  }
    /**
   * Invoke changing meta.lablels using bulkedit API
   * @param networkinterfaces
   */
  bulkeditLabels(networks: NetworkNetwork[]) {

    const successMsg: string = 'Updated ' + networks.length + ' network labels';
    const failureMsg: string = 'Failed to update network labels';
    const stagingBulkEditAction = this.buildBulkEditLabelsPayload(networks);
    this.bulkEditHelper(networks, stagingBulkEditAction, successMsg, failureMsg );
  }

  handleEditCancel($event) {
    this.inLabelEditMode = false;
  }

  isNetworkInternal(network: NetworkNetwork) {
    return network && network.meta && network.meta.labels &&
        network.meta.labels['io.pensando.psm-internal'];
  }

  hideDeleteIcon(network: NetworkNetwork): boolean {
    return this.isNetworkInternal(network);
  }

  disableMultupleDeleteIcon() {
    if (!this.hasSelectedRows()) {
      return true;
    }
    const selectedObjects = this.getSelectedDataObjects();
    for (let i = 0; i < selectedObjects.length; i++) {
      const network: NetworkNetwork = selectedObjects[i] as NetworkNetwork;
      if (this.isNetworkInternal(network)) {
        return true;
      }
    }
    return false;
  }

  onBulkEditSuccess(veniceObjects: any[], stagingBulkEditAction: IStagingBulkEditAction, successMsg: string, failureMsg: string) {
    this.inLabelEditMode = false;
  }

  onBulkEditFailure(error: Error, veniceObjects: any[], stagingBulkEditAction: IStagingBulkEditAction, successMsg: string, failureMsg: string, ) {
      this.dataObjects = Utility.getLodash().cloneDeepWith(this.dataObjectsBackup);
  }

}
