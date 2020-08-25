import { ChangeDetectorRef, Component, OnDestroy, OnInit, ViewChild, ViewEncapsulation, ChangeDetectionStrategy } from '@angular/core';
import { FormArray } from '@angular/forms';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { MetricsUtility } from '@app/common/MetricsUtility';
import { DSCWorkloadsTuple, ObjectsRelationsUtility } from '@app/common/ObjectsRelationsUtility';
import { Utility, VeniceObjectCache } from '@app/common/Utility';
import { CardStates, StatArrowDirection } from '@app/components/shared/basecard/basecard.component';
import { HeroCardOptions } from '@app/components/shared/herocard/herocard.component';
import { PrettyDatePipe } from '@app/components/shared/Pipes/PrettyDate.pipe';
import { CustomExportMap, TableCol } from '@app/components/shared/tableviewedit';
import { TableUtility } from '@app/components/shared/tableviewedit/tableutility';
import { TablevieweditAbstract } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { ClusterService } from '@app/services/generated/cluster.service';
import { SearchService } from '@app/services/generated/search.service';
import { WorkloadService } from '@app/services/generated/workload.service';
import { MetricsPollingQuery, MetricsqueryService, TelemetryPollingMetricQueries } from '@app/services/metricsquery.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { SearchUtil } from '@components/search/SearchUtil';
import { AdvancedSearchComponent } from '@components/shared/advanced-search/advanced-search.component';
import { LabelEditorMetadataModel } from '@components/shared/labeleditor';
import { ClusterDistributedServiceCard, ClusterDistributedServiceCardSpec_mgmt_mode, ClusterDistributedServiceCardStatus_admission_phase, ClusterDSCProfile, IClusterDistributedServiceCard, ClusterHost, ClusterDSCProfileSpec_feature_set, ClusterDSCProfileSpec_deployment_target} from '@sdk/v1/models/generated/cluster';
import { IApiStatus } from '@sdk/v1/models/generated/monitoring';
import { FieldsRequirement, ISearchSearchResponse, SearchSearchRequest, SearchSearchResponse } from '@sdk/v1/models/generated/search';
import { IBulkeditBulkEditItem, IStagingBulkEditAction } from '@sdk/v1/models/generated/staging';
import { WorkloadWorkload } from '@sdk/v1/models/generated/workload';
import { ITelemetry_queryMetricsQueryResponse, ITelemetry_queryMetricsQueryResult } from '@sdk/v1/models/telemetry_query';
import * as _ from 'lodash';
import { SelectItem } from 'primeng/api';
import { forkJoin, Observable, Subscription } from 'rxjs';
import { RepeaterData, ValueType } from 'web-app-framework';
import { NaplesCondition, NaplesConditionValues } from '.';
import { WorkloadUtility, WorkloadNameInterface } from '@app/common/WorkloadUtility';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';
import { Router } from '@angular/router';
import { throttleTime } from 'rxjs/operators';

interface ChartData {
  // macs or ids of the dsc
  labels: string[];
  // values of the card
  datasets: { [key: string]: any }[];
}

interface DSCUiModel {
  associatedConditionStatus?: {
    dscCondStr: string;
    dscNeedReboot: boolean
  };
  associatedWorkloads?: WorkloadWorkload[];
}

@Component({
  selector: 'app-naples',
  encapsulation: ViewEncapsulation.None,
  templateUrl: './naples.component.html',
  styleUrls: ['./naples.component.scss'],
  changeDetection: ChangeDetectionStrategy.OnPush,
})

/**
 * Added advanced search to naples table.
 *
 * When WatchDistributedServiceCard sends a response, we create a map mapping naples name (spec.id) to the naples object.
 * Whenever advanced search is used, a new searchrequest is created using this.advancedSearchComponent.getSearchRequest
 * Then we make an api call to get all the matching DSCs using _callSearchRESTAPI.
 * These results do not contain spec information, so we lookup the original naples object from the naplesMap.
 * The matching naples objects are added to this.filteredNaples which is used to render the table.
 *
 * 2020-04-17
 * We added bulkedit to update DSC labels and assign DSC-profile to DSCs.
 * Note: this.stagingService is from parent class  TablevieweditAbstract
 */



export class NaplesComponent extends DataComponent implements OnInit {

  @ViewChild('dscTable') dscTable: PentableComponent;

  dataObjects: ReadonlyArray<ClusterDistributedServiceCard> = [];
  dataObjectsBackUp: ReadonlyArray<ClusterDistributedServiceCard> = [];
  inLabelEditMode: boolean = false;
  labelEditorMetaData: LabelEditorMetadataModel;

  inProfileAssigningMode: boolean = false;

  // Used for processing the stream events
  naplesEventUtility: HttpEventUtility<ClusterDistributedServiceCard>;
  naplesMap: { [napleName: string]: ClusterDistributedServiceCard };

  fieldFormArray = new FormArray([]);
  maxSearchRecords: number = 8000;

  workloadEventUtility: HttpEventUtility<WorkloadWorkload>;
  dscsWorkloadsTuple: { [dscKey: string]: DSCWorkloadsTuple; };
  maxWorkloadsPerRow: number = 10;

  isTabComponent: boolean = false;
  // Used for the table - when true there is a loading icon displayed
  tableLoading: boolean = false;
  hasAdmittedDSC: boolean = false;  // indicate whether PSM has admitted DSC.  We show DSC hero charts only when there are admitted DSCs

  cols: TableCol[] = [
    { field: 'spec.id', header: 'Name/Spec.id', class: '', sortable: true, width: 100 },
    { field: 'status.primary-mac', header: 'MAC Address', class: '', sortable: true, width: '110px' },
    { field: 'status.DSCVersion', header: 'Version', class: '', sortable: true, width: '80px' },
    { field: 'spec.dscprofile', header: 'Profile', class: '', sortable: true, width: '80px' },
    { field: 'status.ip-config.ip-address', header: 'Management IP Address', class: '', sortable: false, width: '100px' },
    { field: 'spec.admit', header: 'Admit', class: '', sortable: false, localSearch: true, width: '60px', filterfunction: this.searchAdmits },
    { field: 'status.admission-phase', header: 'Phase', class: '', sortable: false, width: '80px' },
    {
      field: 'status.conditions', header: 'Condition', class: '', sortable: true, localSearch: true, width: '70px',
      filterfunction: this.searchConditions
    },
    { field: 'status.host', header: 'Host', class: '', sortable: true, width: 100 },
    { field: 'meta.labels', header: 'Labels', class: '', sortable: true, width: 100 },
    { field: 'status.control-plane-status', header: 'Control Plane Status', class: '', sortable: true, width: 100 },
    { field: 'workloads', header: 'Workloads', class: '', sortable: false, localSearch: true, width: 100 },
    { field: 'meta.mod-time', header: 'Modification Time', class: '', sortable: true, width: '170px' },
    { field: 'meta.creation-time', header: 'Creation Time', class: '', sortable: true, width: '170px' },
  ];
  exportMap: CustomExportMap = {
    'workloads': (opts): string => {
      return (opts.data._ui.associatedWorkloads) ? opts.data._ui.associatedWorkloads.map(wkld => wkld.meta.name).join(', ') : '';
    },
    'status.conditions': (opts): string => {
      return Utility.getNaplesConditionObject(opts.data).condition.toLowerCase();
    },
    'meta.labels': (opts): string => {
      return this.formatLabels(opts.data.meta.labels);
    }
  };

  advSearchCols: TableCol[] = [];

  bodyicon: any = {
    margin: {
      top: '9px',
      left: '8px',
    },
    url: '/assets/images/icons/cluster/naples/ico-dsc-black.svg',
  };

  cardColor = '#b592e3';
  cpuColor: string = '#b592e3';
  memoryColor: string = '#e57553';
  storageColor: string = '#70bab4';

  cardIcon: Icon = {
    margin: {
      top: '10px',
      left: '10px'
    },
    svgIcon: 'naples'
  };

  naplesIcon: Icon = {
    margin: {
      top: '0px',
      left: '0px',
    },
    matIcon: 'grid_on'
  };

  lastUpdateTime: string = '';

  cpuChartData: HeroCardOptions = MetricsUtility.clusterLevelCPUHeroCard(this.cpuColor, this.cardIcon);

  memChartData: HeroCardOptions = MetricsUtility.clusterLevelMemHeroCard(this.memoryColor, this.cardIcon);

  diskChartData: HeroCardOptions = MetricsUtility.clusterLevelDiskHeroCard(this.storageColor, this.cardIcon);

  heroCards = [
    this.cpuChartData,
    this.memChartData,
    this.diskChartData
  ];

  timeSeriesData: ITelemetry_queryMetricsQueryResult;
  avgData: ITelemetry_queryMetricsQueryResult;
  avgDayData: ITelemetry_queryMetricsQueryResult;
  maxObjData: ITelemetry_queryMetricsQueryResult;

  telemetryKind: string = 'DistributedServiceCard';

  customQueryOptions: RepeaterData[];
  multiSelectFields: Array<string> = ['Condition'];

  cancelSearch: boolean = false;

  disableTableWhenRowExpanded: boolean = false;
  exportFilename: string = 'PSM-DistributedServiceCards';

  hostObjects: ReadonlyArray<ClusterHost>;
  workloadList: WorkloadWorkload[] = [];
  searchDSCsCount: number = 0;

  // this map is used to make sure the same card always has the same color
  dscMacToColorMap: { [key: string]: string } = {};
  top10CardTelemetryData: ITelemetry_queryMetricsQueryResponse = null;
  top10CardChartData: ChartData[];
  top10CardChartOptions: { [key: string]: any }[];
  chosenCardMac: string;
  chosenCard: ClusterDistributedServiceCard;
  chartLastUpdateTime: string = '';


  dscprofileOptions: SelectItem[] = [];
  selectedDSCProfiles: SelectItem;
  dscprofiles: ReadonlyArray<ClusterDSCProfile> = [];

  saveDSCProfileOperationDone: boolean;
  saveLabelsOperationDone: boolean;

  maxDSCcontrolplanestatusRow: number = 4;

  constructor(private clusterService: ClusterService,
    protected controllerService: ControllerService,
    protected metricsqueryService: MetricsqueryService,
    protected searchService: SearchService,
    protected workloadService: WorkloadService,
    protected router: Router,
    protected cdr: ChangeDetectorRef,
    protected uiconfigsService: UIConfigsService
  ) {
    super(controllerService, uiconfigsService);
  }


  deleteRecord(object: ClusterDistributedServiceCard): Observable<{ body: IClusterDistributedServiceCard | IApiStatus | Error; statusCode: number; }> {
    return this.clusterService.DeleteDistributedServiceCard(object.meta.name);
  }

  generateDeleteConfirmMsg(object: ClusterDistributedServiceCard): string {
    let confirmMsg = 'Are you sure you want to delete DSC ';
    if (object.spec['mgmt-mode'] === ClusterDistributedServiceCardSpec_mgmt_mode.host &&
      object.status['admission-phase'] === ClusterDistributedServiceCardStatus_admission_phase.admitted) {
      confirmMsg = 'DSC decommissioning is not completed, are you sure you want to delete DSC ';
    }
    return confirmMsg + object.meta.name + '?';
  }

  generateDeleteSuccessMsg(object: ClusterDistributedServiceCard): string {
    return 'Deleted DSC ' + object.meta.name;
  }

  filterColumns() {
    // if backend is Venice-for-cloud, don't show [workload  dscProfile]
    // if backend is Venice-for-enterprise, don't show [status.control-plane-status, status.ip-config.ip-address]
    if (this.uiconfigsService.isFeatureEnabled('enterprise')) {
      this.cols = this.cols.filter((col: TableCol) => {
        return !( col.field === 'status.control-plane-status' || col.field === 'status.ip-config.ip-address');
      });
    }
    if (this.uiconfigsService.isFeatureEnabled('cloud')) {
      this.cols = this.cols.filter((col: TableCol) => {
        return ! (col.field === 'workloads' || col.field === 'spec.dscprofile' );
      });
    }
  }

  /**
   * 2020-02-15
   * We use data-cache strategy.
   * getDSCTotalCount() -> will invoke watchAll()
   */
  ngOnInit() {
    super.ngOnInit();
    this.penTable = this.dscTable;
    this.tableLoading = true;
    this.filterColumns(); //  If backend is a Venice-for-cloud, we want to exclude some columns
    this.buildAdvSearchCols();
    this.provideCustomOptions();
    this.getTop10CardsDSCAndActiveSessions();
    this.getDSCTotalCount();  // start retrieving data.
  }

  setDefaultToolbar() {
    this.controllerService.setToolbarData({
      buttons: [],
      breadcrumb: [{ label: 'Distributed Services Cards', url: Utility.getBaseUIUrl() + 'cluster/dscs' }]
    });
  }

  generateChartOptions(addSmallestUnit: boolean = false) {
    const dataObjects = this.dataObjects;
    const scales = {
      yAxes: [{
        ticks: {
          beginAtZero: true
        }
      }]
    };
    if (addSmallestUnit) {
      scales.yAxes[0].ticks['stepSize'] = 1;
    }
    return {
      responsive: true,
      scales,
      legend: {
        display: true,
        labels: {
          boxWidth: 0,
          fontSize: 12
        }
      },
      hover: {
        onHover: function (e) {
          const point = this.getElementAtEvent(e);
          e.target.style.cursor = (point && point.length) ? 'pointer' : 'default';
        }
      },
      tooltips: {
        intersect: false,
        callbacks: {
          title: function() {
            return null;
          },
          label: function (tooltipItem) {
            const lines = [tooltipItem.xLabel + ':   ' + tooltipItem.yLabel];
            if (dataObjects && dataObjects.length > 0) {
              const card: ClusterDistributedServiceCard =
                dataObjects.find(item => item.spec.id === tooltipItem.xLabel ||
                  item.meta.name === tooltipItem.xLabel);
              if (card && card._ui.associatedWorkloads &&
                  card._ui.associatedWorkloads.length > 0) {
                const workloadNames = WorkloadUtility.getWorkloadNames(
                    card._ui.associatedWorkloads);
                lines.push('Workloads:');
                for (let i = 0; i < workloadNames.length && i < 8; i++) {
                  lines.push(workloadNames[i].fullname);
                }
                if (workloadNames.length > 8) {
                  lines .push('... ' + (workloadNames.length - 8) + ' more');
                }
              }
            }
            return lines;
          }
        },
        backgroundColor: '#FFF',
        titleFontSize: 12,
        bodyFontColor: '#000',
        bodyFontSize: 12,
        displayColors: false,
        borderColor: '#6ba0e5',
        borderWidth: 1
      },
      onClick: (e, data) => {
        // only display workloads when cards and workloads are loaded
        if (this.dataObjects && this.dataObjects.length >= this.searchDSCsCount &&
          data && data.length > 0 && data[0]) {
          const index = data[0]._index;
          const chart = data[0]._chart;
          if (index >= 0 && chart && chart.config && chart.config.data) {
            const chartData = chart.config.data;
            if (chartData.labels.length > 0 && chartData.labels[index] &&
              this.chosenCardMac !== chartData.labels[index]) {
              this.chosenCardMac = chartData.labels[index];
              const card: ClusterDistributedServiceCard =
                this.dataObjects.find((item: ClusterDistributedServiceCard) => (item.spec.id === this.chosenCardMac) || (item.meta.name === this.chosenCardMac));
              if (card) {
                this.router.navigate(['/', 'cluster', 'dscs', card.meta.name]);
              }
            }
          }
        }
      }
    };
  }

  generateChartData(labels: string[], values: number[], title: string) {
    const numOfColors = Utility.allColors.length;
    const bgColors: string[] = labels.map((label: string, idx: number) => {
      // selected card has 1 as opacity
      const opacity = this.matchCardMacToCardId(label) === this.chosenCardMac ? 1 : 0.3;
      let color = this.dscMacToColorMap[label];
      if (!color) {
        const mapSize = Object.keys(this.dscMacToColorMap).length;
        color = Utility.allColors[mapSize % numOfColors];
        this.dscMacToColorMap[label] = color;
      }
      return Utility.convertHexColorToRGBAColor(color, opacity);
    });
    return {
      labels: labels.map(mac => this.matchCardMacToCardId(mac)),
      datasets: [
        {
          label: title,
          backgroundColor: bgColors,
          data: values
        }
      ]
    };
  }

  matchCardMacToCardId(mac: string): string {
    if (this.dataObjects) {
      const card: ClusterDistributedServiceCard =
        this.dataObjects.find(item => item.meta && item.meta.name === mac);
      if (card) {
        return card.spec.id;
      }
    }
    return mac;
  }

  getTop10CardsDSCAndActiveSessions() {
    const queryList: TelemetryPollingMetricQueries = {
      queries: [],
      tenant: Utility.getInstance().getTenant()
    };
    const query1: MetricsPollingQuery = MetricsUtility.topTenQueryPolling(
      'SessionSummaryMetrics', ['TotalActiveSessions']);
    const query2: MetricsPollingQuery = MetricsUtility.topTenQueryPolling(
      'FteCPSMetrics', ['ConnectionsPerSecond']);
    queryList.queries.push(query1);
    queryList.queries.push(query2);

    // refresh every 5 minutes
    const sub = this.metricsqueryService.pollMetrics
      ('top10nCards', queryList, MetricsUtility.FIVE_MINUTES).subscribe(
        (data: ITelemetry_queryMetricsQueryResponse) => {
          if (data && data.results && data.results.length === queryList.queries.length) {
            this.top10CardTelemetryData = data;
            if (this.dataObjects && this.dataObjects.length > 0) {
              this.processTop10CardTelemetryData(data);
            }
          }
        },
        (err) => {
          this.controllerService.invokeErrorToaster('Error', 'Failed to load Top 10 DSC metrics.');
        }
      );
  }

  processTop10CardTelemetryData(data: ITelemetry_queryMetricsQueryResponse) {
    if (data && data.results) {
      this.top10CardChartData = [];
      this.top10CardChartOptions = [];
      data.results.forEach((result, idx) => {
        if (MetricsUtility.resultHasData(result)) {
          const chartS = result.series[0];
          chartS.values.sort((a, b) => b[1] - a[1]);
          const cardMacs: string[] = [];
          const cardValues: number[] = [];
          chartS.values.forEach(metric => {
            if (metric[1] && metric[1] > 0) {
              cardMacs.push(metric[2]);
              cardValues.push(metric[1]);
            }
          });
          if (cardValues.length > 0) {
            this.chartLastUpdateTime = new Date().toISOString();
            const title = idx === 0 ? 'Active Sessions' : 'Connection Per Second';
            const graphTitle = `${title} - Top ${cardValues.length} DSCs`;
            this.top10CardChartData.push(this.generateChartData(cardMacs, cardValues, graphTitle));
            this.top10CardChartOptions.push(this.generateChartOptions(cardValues[0] <= 5));
          }
        }
      });
    }
  }

  /**
   * Generate a search query and get total number of admitted DSC.
   */
  getDSCTotalCount() {
    const query: SearchSearchRequest = Utility.buildObjectTotalSearchQuery('DistributedServiceCard');
    const newQuery = query.getFormGroupValues();
    const fieldCriteria = {
      'key': 'status.admission-phase',
      'operator': 'equals',
      'values': [
        'admitted'
      ]
    };
    newQuery.query.fields.requirements.push(fieldCriteria);
    const searchDSCTotalSubscription = this.searchService.PostQuery(newQuery).subscribe(
      (resp) => {
        if (resp) {
          const body = resp.body as ISearchSearchResponse;
          let dscTotal = 0;
           dscTotal = parseInt(body['total-hits'], 10);
          // To test VS-1129, hard code -- dscTotal = 0;
          if (dscTotal > 0) {
            this.searchDSCsCount = dscTotal;
            this.hasAdmittedDSC = true;
            this.getMetrics();
          } else {
            this.tableLoading = false;
            // this.controllerService.invokeInfoToaster('Information', 'There is no admitted DSC found in PSM');  // this line cause a lot problem for Shrey
            console.error('There is no admitted DSC found in PSM elastic search system');
          }
          this.invokeWatch();
        }
      },
      (error) => {
        // In case search failed, we still want to invoke watch DSCs.
        this.controllerService.invokeRESTErrorToaster('Failed to search DSCs', error);
        this.tableLoading = false;
        this.invokeWatch();
      }
      // this._controllerService.webSocketErrorHandler('Failed to search DSCs'),
    );
    this.subscriptions.push(searchDSCTotalSubscription);
  }

  invokeWatch() {
    this.watchWorkloads();
    this.watchNaples();
    this.watchDSCProfiles();
    this.watchHosts();
  }

  buildAdvSearchCols() {
    this.advSearchCols = this.cols.filter((col: TableCol) => {
      return ! (col.field === 'workloads' || col.field === 'status.control-plane-status');
    });
    if (!this.uiconfigsService.isFeatureEnabled('cloud')) {
      this.advSearchCols.push(
        {
          field: 'workloads', header: 'Workloads', localSearch: true, kind: 'Workload',
          filterfunction: this.searchWorkloads,
          advancedSearchOperator: SearchUtil.stringOperators
        }
      );
    } else {
      this.advSearchCols.push(
        {
          field: 'status.control-plane-status', header: 'Control Plane Status', localSearch: true, kind: 'ControlPlaneStatus',
          filterfunction: this.searchControlPlaneStatus,
          advancedSearchOperator: SearchUtil.stringOperators
        }
      );
    }
  }

  searchControlPlaneStatus (requirement: FieldsRequirement, data = this.dataObjects): any[] {
    const outputs: any[] = [];
    let isNot = false;
    for (let i = 0; data && i < data.length; i++) {
      const bgpstatus = data[i].status['control-plane-status']['bgp-status'];
      for (let k = 0; k < bgpstatus.length; k++) {
        const recordValue = _.get(bgpstatus[k], ['peer-address']);
        const searchValues = requirement.values;
        let operator = String(requirement.operator);
        // if notcontains or notEquals, use the regular operator (contains/equals) and flip the results
        isNot = (operator.slice(0, 3) === 'not');
        operator = TableUtility.convertOperator(operator);
        const operatorAdjusted = isNot ? operator.slice(3).toLocaleLowerCase() : operator;
        const activateFunc = TableUtility.filterConstraints[operatorAdjusted];
        for (let j = 0; j < searchValues.length; j++) {
          if (activateFunc && activateFunc(recordValue, searchValues[j])) {
            outputs.push(data[i]);
          }
        }
      }
    }
    return isNot ? _.differenceWith(data, outputs, _.isEqual) : outputs;
  }

  watchHosts() {
    const hostSubscription = this.clusterService.ListHostCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.hostObjects = response.data;
        this.refreshGui(this.cdr);
      }
    );
    this.subscriptions.push(hostSubscription);
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
        this.buildDSCWorkloadsMap(this.workloadList, this.dataObjects);
        this.refreshGui(this.cdr);
      }
    );
    this.subscriptions.push(workloadSubscription);
  }

  /**
   * This is a key API.  It computes the DSC-Workloads map and updates each DSC's workload list
   * @param myworkloads
   * @param dscs
   */
  buildDSCWorkloadsMap(myworkloads: ReadonlyArray<WorkloadWorkload> | WorkloadWorkload[],
    dscs: ReadonlyArray<ClusterDistributedServiceCard> | ClusterDistributedServiceCard[]) {
    if (myworkloads && dscs) {
      this.dscsWorkloadsTuple = ObjectsRelationsUtility.buildDscWorkloadsMaps(myworkloads, dscs);
      this.dataObjects = this.dataObjects.map((naple: ClusterDistributedServiceCard) => {
        ((naple._ui) as DSCUiModel).associatedWorkloads = this.getDSCWorkloads(naple);
        return naple;
      });
      this.refreshGui(this.cdr);
    }
  }

  getHostFullName(hostName: string): string {
    if (!this.hostObjects || this.hostObjects.length === 0) {
      return hostName;
    }
    const hostObj: ClusterHost =
      this.hostObjects.find((item: ClusterHost) => item.meta.name === hostName);
    if (!hostObj) {
      return hostName;
    }
    if (!hostObj.meta.labels || !hostObj.meta.labels['io.pensando.vcenter.display-name'] ||
        !hostObj.meta.labels['io.pensando.orch-name']) {
      return hostName;
    }
    return hostObj.meta.labels['io.pensando.vcenter.display-name'] + '(' + hostName + ')';
  }


  provideCustomOptions() {
    this.customQueryOptions = [
      {
        key: { label: 'Condition', value: 'Condition' },
        operators: SearchUtil.stringOperators,
        valueType: ValueType.singleSelect,
        values: [
          { label: 'Healthy', value: NaplesConditionValues.HEALTHY },
          { label: 'Unhealthy', value: NaplesConditionValues.UNHEALTHY },
          { label: 'Unknown', value: NaplesConditionValues.UNKNOWN },
          { label: 'Empty', value: 'empty' },
          { label: 'Reboot Needed', value: NaplesConditionValues.REBOOT_NEEDED }
        ],
      }
    ];
  }

  /**
   * We use DSC maps for local search. In the backend, DSC status may changed. So we must clean up map every time we get updated data from watch.api
   * This is a helper function to clear up maps.
   */
  _clearDSCMaps() {
    this.naplesMap = {};
  }

  /**
   * Watches NapDistributed Services Cards data on KV Store and fetch new DSC data
   * Generates column based search object, currently facilitates condition search
   */
  watchNaples() {
    this._clearDSCMaps();
    const dscSubscription = this.clusterService.ListDistributedServiceCardCache().pipe(throttleTime(5000)).subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.dataObjects = response.data;
        this.processDSCrecords();
        if (!this.top10CardChartData || this.top10CardChartData.length === 0) {
          this.processTop10CardTelemetryData(this.top10CardTelemetryData);
        }
        this.refreshGui(this.cdr);
      }
    );
    this.subscriptions.push(dscSubscription);
  }

  watchDSCProfiles() {
    const subscription = this.clusterService.ListDSCProfileCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        this.dscprofiles = response.data;
        if (this.dscprofiles && this.dscprofiles.length > 0) {
          this.dscprofiles.forEach((dscprofile: ClusterDSCProfile) => {
            const obj: SelectItem = {
              value: dscprofile.meta.name,
              label: dscprofile.meta.name
            };
            this.dscprofileOptions.push(obj);
          });
          this.refreshGui(this.cdr);
        }
      },
      this._controllerService.webSocketErrorHandler('Failed to get DSC Profile')
    );
    this.subscriptions.push(subscription); // add subscription to list, so that it will be cleaned up when component is destroyed.
  }

  /**
   * For VS-1448
   * We verify there is at least one DSC admitted.
   */
  isAtleastOneDSCAdmitted(): boolean {
    const admittedDSCs = this.dataObjects.find((dsc: ClusterDistributedServiceCard) => dsc.status['admission-phase'] === ClusterDistributedServiceCardStatus_admission_phase.admitted);
    return (!!admittedDSCs);
  }

  processDSCrecords() {
    this.tableLoading = false;
    this._clearDSCMaps(); // VS-730.  Want to clear maps when we get updated data.
    this.buildDSCWorkloadsMap(this.workloadList, this.dataObjects);
    if (this.dataObjects && this.dataObjects.length > 0  ) {
      const tmpHasAdmittedDSC = this.isAtleastOneDSCAdmitted();
      // In a scenario where Venice has no DSC, this page is in idle.  When we receive any new DSC and at least one DSC admitted, it will turn on hero-cards.
      if (tmpHasAdmittedDSC && ! this.hasAdmittedDSC ) {
        this.hasAdmittedDSC  = tmpHasAdmittedDSC;
        this.getMetrics();
      }
    }
    if (this.dataObjects.length >= this.searchDSCsCount) {
      this.searchDSCsCount = this.dataObjects.length;
    }
    for (const naple of this.dataObjects) {
      this.naplesMap[naple.meta.name] = naple;
      const dscHealthCond: NaplesCondition = Utility.getNaplesConditionObject(naple);
      const uiData: DSCUiModel = {};
      uiData.associatedConditionStatus = {
        dscCondStr: dscHealthCond.condition.toLowerCase(),
        dscNeedReboot: dscHealthCond.rebootNeeded
      };
      uiData.associatedWorkloads = this.getDSCWorkloads(naple);
      naple._ui = uiData;
    }
    this.tryGenCharts();
    // backup dataObjects
    this.dataObjectsBackUp = Utility.getLodash().cloneDeepWith(this.dataObjects);
  }

  getDSCWorkloads(naple: ClusterDistributedServiceCard): WorkloadWorkload[] {
    if (this.dscsWorkloadsTuple[naple.meta.name]) {
      return this.dscsWorkloadsTuple[naple.meta.name].workloads;
    } else {
      return [];
    }
  }

  displayColumn(data: ClusterDistributedServiceCard, col: TableCol): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(data, fields);
    const column = col.field;
    switch (column) {
      default:
        return Array.isArray(value) ? JSON.stringify(value, null, 2) : value;
    }
  }

  hasWorkloads(rowData: ClusterDistributedServiceCard): boolean {
    const workloads = rowData._ui.associatedWorkloads;
    return workloads && workloads.length > 0;
  }

  isNICHealthy(data: ClusterDistributedServiceCard): boolean {
    const uiData: DSCUiModel = data._ui;
    return uiData.associatedConditionStatus && uiData.associatedConditionStatus.dscCondStr === NaplesConditionValues.HEALTHY;
  }

  isNICUnhealthy(data: ClusterDistributedServiceCard): boolean {
    const uiData: DSCUiModel = data._ui;
    return uiData.associatedConditionStatus && uiData.associatedConditionStatus.dscCondStr === NaplesConditionValues.UNHEALTHY;
  }

  isNICHealthUnknown(data: ClusterDistributedServiceCard): boolean {
    const uiData: DSCUiModel = data._ui;
    return uiData.associatedConditionStatus && uiData.associatedConditionStatus.dscCondStr === NaplesConditionValues.UNKNOWN;
  }

  isNicNeedReboot(data: ClusterDistributedServiceCard): boolean {
    const uiData: DSCUiModel = data._ui;
    return uiData.associatedConditionStatus && uiData.associatedConditionStatus.dscNeedReboot;
  }

  displayReasons(data: ClusterDistributedServiceCard): any {
    return Utility.displayReasons(data);
  }

  // isNICNotAdmitted(data: ClusterDistributedServiceCard): boolean {
  //   return Utility.isNICConditionNotAdmitted(data);
  // }

  /**
   * We start 3 metric polls.
   * Time series poll
   *  - Fetches the past 24 hours, averaging in
   *    5 min buckets (starting at the hour) across all nodes
   *  - Used for line graph as well as the current value stat
   * AvgPoll
   *   - Fetches the average over the past 24 hours across all nodes
   * MaxNode poll
   *   - Fetches the average over the past 5 min bucket. Grouped by node
   *     - Rounds down to the nearest 5 min increment.
   *       Ex. 11:52 -> fetches data from 11:50 till current time
   *
   * When you query for 5 min intervals, they are automatically aligned by the hour
   * We always round down to the last completed 5 min interval.
   *
   * Distributed Services Cards overview level display
   *  - Time series graph of all the nodes averaged together, avg into 5 min buckets
   *  - Current avg of last 5 min
   *  - Average of past day
   *  - Naple using the most in the last 5 min
   */
  getMetrics() {
    const queryList: TelemetryPollingMetricQueries = {
      queries: [],
      tenant: Utility.getInstance().getTenant()
    };
    queryList.queries.push(this.timeSeriesQuery());
    queryList.queries.push(this.avgQuery());
    queryList.queries.push(this.avgDayQuery());
    queryList.queries.push(this.maxNaplesQuery());

    const sub = this.metricsqueryService.pollMetrics('naplesCards', queryList).subscribe(
      (data: ITelemetry_queryMetricsQueryResponse) => {
        if (data && data.results && data.results.length === 4) {
          this.timeSeriesData = data.results[0];
          this.avgData = data.results[1];
          this.avgDayData = data.results[2];
          this.maxObjData = data.results[3];
          this.lastUpdateTime = new Date().toISOString();
          this.tryGenCharts();
        }
      },
      (err) => {
        MetricsUtility.setCardStatesFailed(this.heroCards);
      }
    );
    this.subscriptions.push(sub);
  }

  timeSeriesQuery(): MetricsPollingQuery {
    return MetricsUtility.timeSeriesQueryPolling(this.telemetryKind, ['CPUUsedPercent', 'MemUsedPercent', 'DiskUsedPercent']);
  }

  avgQuery(): MetricsPollingQuery {
    return MetricsUtility.pastFiveMinAverageQueryPolling(this.telemetryKind);
  }

  avgDayQuery(): MetricsPollingQuery {
    return MetricsUtility.pastDayAverageQueryPolling(this.telemetryKind);
  }

  maxNaplesQuery(): MetricsPollingQuery {
    return MetricsUtility.maxObjQueryPolling(this.telemetryKind);
  }

  private tryGenCharts() {
    this.genCharts('CPUUsedPercent', this.cpuChartData);
    this.genCharts('MemUsedPercent', this.memChartData);
    this.genCharts('DiskUsedPercent', this.diskChartData);
    this.refreshGui(this.cdr); // VS-2178, trigger rendering CPU/MEM/Disk charts
  }

  private genCharts(fieldName: string, heroCard: HeroCardOptions) {
    const timeSeriesDataLoaded = MetricsUtility.resultHasData(this.timeSeriesData);
    const avgDataLoaded = MetricsUtility.resultHasData(this.avgData);
    const avgDayDataLoaded = MetricsUtility.resultHasData(this.avgDayData);
    const maxObjDataLoaded = MetricsUtility.resultHasData(this.maxObjData);

    // Time series graph
    if (timeSeriesDataLoaded) {
      const timeSeriesData = this.timeSeriesData;

      const data = MetricsUtility.transformToChartjsTimeSeries(timeSeriesData.series[0], fieldName);
      heroCard.lineData.data = data;
    }

    // Current stat calculation - we take the last point
    if (avgDataLoaded) {
      const index = MetricsUtility.findFieldIndex(this.avgData.series[0].columns, fieldName);
      heroCard.firstStat.value = Math.round(this.avgData.series[0].values[0][index]) + '%';
      heroCard.firstStat.numericValue = Math.round(this.avgData.series[0].values[0][index]);
    }

    // Avg
    if (avgDayDataLoaded) {
      const avgDayData = this.avgDayData;
      if (avgDayData.series[0].values.length !== 0) {
        const index = MetricsUtility.findFieldIndex(this.avgDayData.series[0].columns, fieldName);
        heroCard.secondStat.value = Math.round(avgDayData.series[0].values[0][index]) + '%';
        heroCard.secondStat.numericValue = Math.round(avgDayData.series[0].values[0][index]);
      }
    }

    // For determining arrow direction, we compare the current value to the average value
    if (heroCard.firstStat == null || heroCard.firstStat.value == null || heroCard.secondStat == null || heroCard.secondStat.value == null) {
      heroCard.arrowDirection = StatArrowDirection.HIDDEN;
    } else if (heroCard.firstStat.value > heroCard.secondStat.value) {
      heroCard.arrowDirection = StatArrowDirection.UP;
    } else if (heroCard.firstStat.value < heroCard.secondStat.value) {
      heroCard.arrowDirection = StatArrowDirection.DOWN;
    } else {
      heroCard.arrowDirection = StatArrowDirection.HIDDEN;
    }

    // Max naples
    if (maxObjDataLoaded) {
      const maxNaples = MetricsUtility.maxObjUtility(this.maxObjData, fieldName);
      if (maxNaples == null || maxNaples.max === -1) {
        heroCard.thirdStat.value = null;
      } else {
        const thirdStatNaples = this.getNaplesByKey(maxNaples.name);
        if ((thirdStatNaples) != null) {
          const thirdStatName = thirdStatNaples.spec.id;
          let thirdStat: string = thirdStatName;
          if (thirdStat.length > 0) {
            // VS-736 start
            thirdStat = Utility.getHeroCardDisplayValue(thirdStat);
            Utility.customizeHeroCardThirdStat(thirdStat, heroCard, thirdStatName);
            // VS-736 end
            thirdStat += ' (' + Math.round(maxNaples.max) + '%)';
            heroCard.thirdStat.value = thirdStat;
            heroCard.thirdStat.url = '/cluster/dscs/' + thirdStatNaples.meta.name;
            heroCard.thirdStat.numericValue = Math.round(maxNaples.max);
          }
        } else {
          heroCard.thirdStat.value = null;
        }
      }
    }

    // begin showing card when any of the metrics are available
    if (heroCard.cardState !== CardStates.READY && (timeSeriesDataLoaded || avgDataLoaded || avgDayDataLoaded || maxObjDataLoaded)) {
      heroCard.cardState = CardStates.READY;
    }
  }

  getNaplesByKey(name: string): ClusterDistributedServiceCard {
    for (let index = 0; index < this.dataObjects.length; index++) {
      const naple = this.dataObjects[index];
      if (naple.meta.name === name) {
        return naple;
      }
    }
    return null;
  }

  /**
   * Generates Local and Remote search queries. Later, calls Local and Remote Search Function
   * @param field Field by which to sort data
   * @param order Sort order (ascending = 1/descending = -1)
   *
   * plug in <app-advanced-search  (searchEmitter)="getDistributedServiceCards()"
   */
  // getDistributedServiceCards(field = this.dscTable.sortField,
  //   order = this.dscTable.sortOrder) {
  //   let searchSearchRequest = this.advancedSearchComponent.getSearchRequest(field, order, 'DistributedServiceCard', true, this.maxSearchRecords);
  //   // VS-1008, we customize searchRequest.
  //   searchSearchRequest = this.customizeSearchRequest(searchSearchRequest);
  //   const localSearchResult = this.advancedSearchComponent.getLocalSearchResult(field, order, this.searchObject);
  //   if (localSearchResult.err) {
  //     this.controllerService.invokeErrorToaster('Error', localSearchResult.errString);
  //     return;
  //   }
  //   if ((searchSearchRequest.query != null && (searchSearchRequest.query.fields != null && searchSearchRequest.query.fields.requirements != null
  //     && searchSearchRequest.query.fields.requirements.length > 0) || (searchSearchRequest.query.texts != null
  //       && searchSearchRequest.query.texts[0].text.length > 0)) || this.cancelSearch) {
  //     if (this.cancelSearch) { this.cancelSearch = false; }
  //     this._callSearchRESTAPI(searchSearchRequest, localSearchResult.searchRes);
  //   } else {
  //     // This case executed when only local search is required
  //     this.dataObjects = this.generateFilteredNaples(localSearchResult.searchRes);
  //     if (this.dataObjects.length === 0) {
  //       this.controllerService.invokeInfoToaster('Information', 'No record found. Please change search criteria.');
  //     }
  //   }
  // }

  /**
   * Per VS-1008, we want to change
   * status.DSCVersion → status.DSCVersion.keyword
   * spec.id → spec.id.keyword
   */
  // "fields": {
  //   "requirements": [
  //     {
  //       "key": "meta.name",  <-- change this per VS-1008
  //       "operator": "notIn",
  //       "values": [
  //         "hw",
  //         "u1"
  //       ]
  //     }
  //   ]
  // }
  // customizeSearchRequest(searchSearchRequest: SearchSearchRequest): SearchSearchRequest {
  //   const requirements = (searchSearchRequest.query.fields && searchSearchRequest.query.fields.requirements) ? searchSearchRequest.query.fields.requirements : null;
  //   for (let i = 0; requirements && i < requirements.length; i++) {
  //     const requirement = requirements[i];
  //     const key = requirement.key;
  //     if (key === 'spec.id') {
  //       requirement.key = 'spec.id.keyword';
  //     }
  //     if (key === 'status.DSCVersion') {
  //       requirement.key = 'status.DSCVersion.keyword';
  //     }
  //   }
  //   return searchSearchRequest;
  // }

  /**
   * Generates aggregate of local and/or remote advanced search filtered naples
   * @param tempNaples Array of local and/or remote advanced search filtered naples meta.name
   */
  // generateFilteredNaples(tempNaples: string[]): ClusterDistributedServiceCard[] {
  //   let resultNics: ClusterDistributedServiceCard[] = [];
  //   this.lastUpdateTime = new Date().toISOString();
  //   if (tempNaples != null && tempNaples.length > 0) {
  //     resultNics = this.filterNaplesByName(tempNaples);
  //   }
  //   return resultNics;
  // }

  // populateFieldSelector() {
  //   this.fieldFormArray = new FormArray([]);
  // }

  /**
 * This serves HTML API. It clear naples search and refresh data.
 * @param $event
 */
  onCancelSearch($event) {
    /*  Previous remote search code. Comment out for now.
     this.populateFieldSelector();
     this.cancelSearch = true;
     this.getDistributedServiceCards();
      */
    this.controllerService.invokeInfoToaster('Information', 'Cleared search criteria, DSCs refreshed.');
    this.dataObjects = this.dataObjectsBackUp;
  }

  /**
   * API to perform remote search
   * @param searchSearchRequest remote search query
   * @param localSearchResult Array of filtered naples name after local search
   */
  // private _callSearchRESTAPI(searchSearchRequest: SearchSearchRequest, localSearchResult: Array<string>) {
  //   const subscription = this.searchService.PostQuery(searchSearchRequest).subscribe(
  //     response => {
  //       const data: SearchSearchResponse = response.body as SearchSearchResponse;
  //       let objects = data.entries;
  //       if (!objects || objects.length === 0) {
  //         this.controllerService.invokeInfoToaster('Information', 'No record found. Please change search criteria.');
  //         objects = [];
  //       }
  //       const entries = [];
  //       const remoteSearchResult = [];
  //       for (let k = 0; k < objects.length; k++) {
  //         entries.push(objects[k].object); // objects[k] is a SearchEntry object
  //         remoteSearchResult.push(entries[k].meta.name);
  //       }
  //       const searchResult = this.advancedSearchComponent.generateAggregateResults(remoteSearchResult, localSearchResult);
  //       this.dataObjects = this.generateFilteredNaples(searchResult);
  //     },
  //     (error) => {
  //       this.controllerService.invokeRESTErrorToaster('Failed to get naples', error);
  //     }
  //   );
  //   this.subscriptions.push(subscription);
  // }

  /**
   * Provides DSC objects for DSC meta.name
   * @param entries Array of DSC meta.name
   */
  // filterNaplesByName(entries: string[]): ClusterDistributedServiceCard[] {
  //   const tmpMap = {};
  //   entries.forEach(ele => {
  //     tmpMap[ele] = this.naplesMap[ele];
  //   });
  //   return Object.values(tmpMap);
  // }

  editLabels() {
    this.labelEditorMetaData = {
      title: 'Editing DSC objects',
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

  assignDSCProfile() {
    const updatedNaples = this.dscTable.selectedDataObjects;
    const systemGenDSC = [];
    for (const napleObject of updatedNaples) {
      if (ObjectsRelationsUtility.isDSCInDSCProfileFeatureSet(napleObject, this.dscprofiles as ClusterDSCProfile[], ClusterDSCProfileSpec_feature_set.flowaware_firewall )
      && ObjectsRelationsUtility.isDSCInDSCProfileDeploymentTarget(napleObject, this.dscprofiles as ClusterDSCProfile[], ClusterDSCProfileSpec_deployment_target.virtualized)) {
        if (Utility.doesObjectHaveSystemLabel(napleObject)) {
          systemGenDSC.push(napleObject.spec.id ? napleObject.spec.id : napleObject.meta.name);
          }
        }
    }

    if (systemGenDSC.length > 0) {
      const displayLen = 5;
      const printMessage = `${systemGenDSC.length} of ${updatedNaples.length} selected DSC(s) are managed by an orchestrator:<br />
      ${systemGenDSC.slice(0, displayLen).join(', ')}${systemGenDSC.length > displayLen ? ', ...' : ''}<br /><br />
      Changing their profiles will negatively impact integration features.`;
      this.controllerService.invokeConfirm({
        header: 'Are you sure you want to change the profile(s)?',
        message: printMessage,
        acceptLabel: 'Yes',
        rejectLabel: 'No',
        accept: () => {
          if (!this.inProfileAssigningMode) {
            this.inProfileAssigningMode = true;
          }
        }
      });
    } else {
      if (!this.inProfileAssigningMode) {
        this.inProfileAssigningMode = true;
      }
    }
  }

  // updateDSCLabelsWithForkjoin(updatedNaples: ClusterDistributedServiceCard[]) {
  //   const observables: Observable<any>[] = [];
  //   for (const naplesObject of updatedNaples) {
  //     const name = naplesObject.meta.name;
  //     const sub = this.clusterService.UpdateDistributedServiceCard(name, naplesObject, '', this.naplesMap[name].$inputValue);
  //     observables.push(sub);
  //   }

  //   const summary = 'Distributed Services Card update';
  //   const objectType = 'DSC';
  //   this.handleForkJoin(observables, summary, objectType);
  // }

  // private handleForkJoin(observables: Observable<any>[], summary: string, objectType: string) {
  //   forkJoin(observables).subscribe((results: any[]) => {
  //     let successCount: number = 0;
  //     let failCount: number = 0;
  //     const errors: string[] = [];
  //     this.saveLabelsOperationDone = true;
  //     for (let i = 0; i < results.length; i++) {
  //       if (results[i]['statusCode'] === 200) {
  //         successCount += 1;
  //       } else {
  //         failCount += 1;
  //         errors.push(results[i].body.message);
  //       }
  //     }
  //     if (successCount > 0) {
  //       const msg = 'Successfully updated ' + successCount.toString() + ' ' + objectType + '.';
  //       this._controllerService.invokeSuccessToaster(summary, msg);
  //       this.onInvokeAPIonMultipleRecordsSuccess();
  //     }
  //     if (failCount > 0) {
  //       this._controllerService.invokeRESTErrorToaster(summary, errors.join('\n'));
  //       this.selectedDSCProfiles = null;
  //     }
  //   },
  //     error => {
  //       this._controllerService.invokeRESTErrorToaster(Utility.UPDATE_FAILED_SUMMARY, error);
  //       this.inProfileAssigningMode = false;
  //       this.saveLabelsOperationDone = true;
  //     }
  //   );
  // }

  onInvokeAPIonMultipleRecordsSuccess() {
    this.inLabelEditMode = false;
    this.inProfileAssigningMode = false;
    this.dscTable.selectedDataObjects = [];

    this.selectedDSCProfiles = null;
  }

  onBulkEditSuccess(veniceObjects: any[], stagingBulkEditAction: IStagingBulkEditAction, successMsg: string, failureMsg: string) {
    this.setSavebuttonState(true);
    this.onInvokeAPIonMultipleRecordsSuccess();
  }

  private setSavebuttonState(flag: boolean) {
    this.saveDSCProfileOperationDone = flag;
    this.saveLabelsOperationDone = flag;
  }

  onBulkEditFailure(error: Error, veniceObjects: any[], stagingBulkEditAction: IStagingBulkEditAction, successMsg: string, failureMsg: string, ) {
    // A DSC used to have "InsertionFWProfile" profile. If user change it to "default" profile. Sever will reject, we restore data here
    this.setSavebuttonState(true);
    this.dataObjects = Utility.getLodash().cloneDeepWith(this.dataObjectsBackUp);
  }


  // The save emitter from labeleditor returns the updated objects here.
  // We use forkjoin to update all the naples.
  handleEditSave(updatedNaples: ClusterDistributedServiceCard[]) {
     this.updateDSCLabelsWithBulkEdit(updatedNaples);  // USE this for bulkedit when backend is ready
  }

  handleEditCancel($event) {
    this.inLabelEditMode = false;
  }

  // buildMoreWorkloadTooltip(dsc: ClusterDistributedServiceCard): string {
  //   const wltips = [];
  //   const workloads = (dsc._ui as DSCUiModel).associatedWorkloads;
  //   for (let i = 0; i < workloads.length; i++) {
  //     if (i >= this.maxWorkloadsPerRow) {
  //       const workload = workloads[i];
  //       wltips.push(workload.meta.name);
  //     }
  //   }
  //   return wltips.join(' , ');
  // }

  onDecommissionCard(event, object: ClusterDistributedServiceCard) {
    if (event) {
      event.stopPropagation();
    }
    const printMessage = `Once you decommission DSC ${object.spec.id ? object.spec.id : object.meta.name}, it can not be undone.`;
    this.controllerService.invokeConfirm({
      header: 'Are you sure that you want to decommission the card?',
      message: printMessage,
      acceptLabel: 'Decommission',
      accept: () => {
        const updatedObject: ClusterDistributedServiceCard = new ClusterDistributedServiceCard(object);
        updatedObject.spec['mgmt-mode'] = ClusterDistributedServiceCardSpec_mgmt_mode.host;
        this.invokeUpdateCard(updatedObject, object,
          'Decommissioning DSC ' + object.meta.name + ' request has been submitted.', 'Decommision Service');
      }
    });
  }

  onAdmitCard(event, object: ClusterDistributedServiceCard) {
    if (event) {
      event.stopPropagation();
    }
    const printMessage = `Once you admit DSC ${object.spec.id ? object.spec.id : object.meta.name}, it can not be undone.`;
    this.controllerService.invokeConfirm({
      header: 'Are you sure that you want to admit the card?',
      message: printMessage,
      acceptLabel: 'Admit',
      accept: () => {
        const updatedObject: ClusterDistributedServiceCard = new ClusterDistributedServiceCard(object);
        updatedObject.spec.admit = true;
        this.invokeUpdateCard(updatedObject, object,
          'Admitting DSC ' + object.meta.name + ' request has been submitted.', 'Admit Service');
      }
    });
  }

  invokeUpdateCard(updatedObject: ClusterDistributedServiceCard,
    oldObject: ClusterDistributedServiceCard,
    successMsg: string,
    actionType: string) {
    const sub = this.clusterService.UpdateDistributedServiceCard(updatedObject.meta.name, updatedObject, null,
      oldObject, false).subscribe(
        () => {
          this.controllerService.invokeInfoToaster(Utility.CHANGE_REQUEST_SUCCESS_SUMMIT_SUMMARY,
            successMsg);
        },
        (error) => {
          if (error.body instanceof Error) {
            console.error(actionType + ' returned code: ' + error.statusCode + ' data: ' + <Error>error.body);
          } else {
            console.error(actionType + ' returned code: ' + error.statusCode + ' data: ' + <IApiStatus>error.body);
          }
          this.controllerService.invokeRESTErrorToaster(Utility.CHANGE_REQUEST_FAILURE_SUMMIT_SUMMARY, error);
        }
      );
    this.subscriptions.push(sub);
  }

  showAdmissionButton(rowData: ClusterDistributedServiceCard): boolean {
    if (rowData.status['admission-phase'] === ClusterDistributedServiceCardStatus_admission_phase.rejected) {
      return false;   // VS-2187
    }
    return !rowData.spec.admit;
  }

  showDecommissionButton(rowData: ClusterDistributedServiceCard): boolean {

    return (rowData.spec['mgmt-mode'] === ClusterDistributedServiceCardSpec_mgmt_mode.network) && (rowData.status['admission-phase'] ===
      ClusterDistributedServiceCardStatus_admission_phase.admitted);
  }

  showDeleteButton(rowData: ClusterDistributedServiceCard): boolean {
    return (rowData.spec['mgmt-mode'] === ClusterDistributedServiceCardSpec_mgmt_mode.host) || rowData.status['admission-phase'] ===
        ClusterDistributedServiceCardStatus_admission_phase.decommissioned || rowData.status['admission-phase'] ===
        ClusterDistributedServiceCardStatus_admission_phase.rejected  || rowData.status['admission-phase'] ===
        ClusterDistributedServiceCardStatus_admission_phase.pending;
  }

  onSearchDSCs(field = this.dscTable.sortField, order = this.dscTable.sortOrder) {
    const searchResults = this.onSearchDataObjects(field, order, 'DistributedServiceCard', this.maxSearchRecords, this.advSearchCols, this.dataObjectsBackUp, this.dscTable.advancedSearchComponent);
    if (searchResults && searchResults.length > 0) {
      this.dataObjects = [];
      this.dataObjects = searchResults;
    }
  }

  searchWorkloads(requirement: FieldsRequirement, data = this.dataObjects): any[] {
    const outputs: any[] = [];
    const searchValues = requirement.values || [];
    const operator = TableUtility.convertOperator(String(requirement.operator));
     // if notcontains or notEquals, use the regular operator (contains/equals) and flip the results
     const isNot = operator.slice(0, 3) === 'not';
     const operatorAdjusted = isNot ? operator.slice(3).toLocaleLowerCase() : operator;
     const activateFunc = TableUtility.filterConstraints[operatorAdjusted];

    for (let i = 0; data && i < data.length; i++) {
      const workloads = (data[i]._ui).associatedWorkloads || [];
      // check for workload name
      const foundWorkloadName = workloads.some(workload => searchValues.some(value => activateFunc && activateFunc(workload.meta.name, value)));
      if (foundWorkloadName) {
        outputs.push(data[i]);
      } else if (data[i].meta.labels && data[i].meta.labels['io.pensando.orch-name']) {
        const foundWorkloadVM = workloads.some(workload => searchValues.some(value => activateFunc && activateFunc(workload.meta.labels['io.pensando.orch-name'], value)));
        if (foundWorkloadVM) {
          outputs.push(data[i]);
        }
      }
    }
    return isNot ? _.differenceWith(data, outputs, _.isEqual) : outputs;
  }

  searchConditions(requirement: FieldsRequirement, data = this.dataObjects): any[] {
    const outputs: any[] = [];
    for (let i = 0; data && i < data.length; i++) {
      let found = false;
      // datat[i].associatedConditionStatus is  {dscCondStr: "healthy", dscNeedReboot: true}
      const uiData: DSCUiModel = data[i]._ui;
      const recordValueStr = uiData.associatedConditionStatus.dscCondStr;
      const recordValueReboot = uiData.associatedConditionStatus.dscNeedReboot;
      const searchValues = requirement.values;
      let operator = String(requirement.operator);
      operator = TableUtility.convertOperator(operator);
      for (let j = 0; j < searchValues.length; j++) {
        const searchValue = searchValues[j];
        // special case empty string
        if (searchValue === 'empty') {
          if ((operator === 'contains' || operator === 'equals') && (recordValueStr === NaplesConditionValues.EMPTY || recordValueStr === NaplesConditionValues.NOTADMITTED)) {
            found = true;
          } else if (operator === 'notcontains' || operator === 'notEquals') {
            found = (recordValueStr !== NaplesConditionValues.EMPTY && recordValueStr !== NaplesConditionValues.NOTADMITTED);
          }
        } else {
          const activateFunc = TableUtility.filterConstraints[operator];
          const activateFuncValue = activateFunc && activateFunc(recordValueStr, searchValue);

          if ((operator === 'contains' || operator === 'equals') && activateFuncValue) {
            found = true;
          } else if (operator === 'notcontains' || operator === 'notEquals') {
            found = activateFuncValue;
          }
        }

        // check reboot condition
        if (searchValues[j] === NaplesConditionValues.REBOOT_NEEDED) {
          if ((operator === 'contains' || operator === 'equals') && recordValueReboot) {
            found = true;
          } else if (operator === 'notcontains' || operator === 'notEquals') {
            found = !recordValueReboot;
          }
        }
      }

      if (found) {
        outputs.push(data[i]);
      }
    }
    return outputs;
  }

  searchAdmits(requirement: FieldsRequirement, data = this.dataObjects): any[] {
    const outputs: any[] = [];
    for (let i = 0; data && i < data.length; i++) {
      const recordValue = String(data[i].spec.admit);
      const searchValues = requirement.values;
      let operator = String(requirement.operator);
      operator = TableUtility.convertOperator(operator);
      for (let j = 0; j < searchValues.length; j++) {
        const activateFunc = TableUtility.filterConstraints[operator];
        if (activateFunc && activateFunc(recordValue, searchValues[j])) {
          outputs.push(data[i]);
        }
      }
    }
    return outputs;
  }

  handleSetDSCProfileSave(option: SelectItem) {
    this.selectedDSCProfiles = option;
    this.onSaveProfileToDSCs();
  }

  handleSetDSCProfileCancel($event) {
    this.inProfileAssigningMode = false;
  }

  // cancelDSCProfileDialog() {
  //   this.inProfileAssigningMode = false;
  //   this.selectedDSCProfiles = null;
  // }

  onSaveProfileToDSCs() {
    const updatedNaples = this.dscTable.selectedDataObjects;
    if (this.selectedDSCProfiles) {
      // this.updateDSCProfilesWithForkjoin(updatedNaples);
      this.updateDSCProfilesWithBulkEdit(updatedNaples);
    }
  }


  buildBulkEditDSCProfilePayload(updatedNaples: ClusterDistributedServiceCard[], buffername: string = ''): IStagingBulkEditAction {

    const stagingBulkEditAction: IStagingBulkEditAction = Utility.buildStagingBulkEditAction(buffername);
    stagingBulkEditAction.spec.items = [];

    for (const dscObj of updatedNaples) {
      if (dscObj.spec.dscprofile !== this.selectedDSCProfiles.value) {
        dscObj.spec.dscprofile = this.selectedDSCProfiles.value;
        const obj = {
          uri: '',
          method: 'update',
          object: dscObj.getModelValues()
        };
        stagingBulkEditAction.spec.items.push(obj as IBulkeditBulkEditItem);
      }
    }
    return (stagingBulkEditAction.spec.items.length > 0) ? stagingBulkEditAction : null;
  }

  updateDSCProfilesWithBulkEdit(updatedDSCs: ClusterDistributedServiceCard[]) {
    const successMsg: string = 'updated ' + updatedDSCs.length + ' DSCs profiles ';
    const failureMsg: string = 'Failed to udpate DSCs profile';
    const stagingBulkEditAction = this.buildBulkEditDSCProfilePayload(updatedDSCs);
    if (stagingBulkEditAction) {
      this.saveDSCProfileOperationDone = null;
      this.bulkEditHelper(updatedDSCs, stagingBulkEditAction, successMsg, failureMsg);
    } else {
      this._controllerService.invokeInfoToaster('No update neccessary', 'All selected DSCs are assigned ' + this.selectedDSCProfiles.value + ' DSC profile already.');
      this.inProfileAssigningMode = false;
      return;
    }
  }

  updateDSCLabelsWithBulkEdit(updatedDSCs: ClusterDistributedServiceCard[]) {
    const successMsg: string = 'updated ' + updatedDSCs.length + ' DSCs labels ';
    const failureMsg: string = 'Failed to udpate DSC labels';
    const stagingBulkEditAction = this.buildBulkEditLabelsPayload(updatedDSCs);
    this.saveLabelsOperationDone = null;
    this.bulkEditHelper(updatedDSCs, stagingBulkEditAction, successMsg, failureMsg);
  }


  // updateDSCProfilesWithForkjoin(updatedNaples: ClusterDistributedServiceCard[]) {
  //   const observables: Observable<any>[] = [];
  //   for (const naplesObject of updatedNaples) {
  //     const name = naplesObject.meta.name;
  //     if (naplesObject.spec.dscprofile !== this.selectedDSCProfiles.value) {
  //       naplesObject.spec.dscprofile = this.selectedDSCProfiles.value;
  //       const sub = this.clusterService.UpdateDistributedServiceCard(name, naplesObject, '', this.naplesMap[name].$inputValue, false);
  //       observables.push(sub);
  //     }
  //   }
  //   if (observables.length === 0) {
  //     this._controllerService.invokeInfoToaster('No update neccessary', 'All selected DSCs are assigned ' + this.selectedDSCProfiles.value + ' DSC profile already.');
  //     this.inProfileAssigningMode = false;
  //     return;
  //   }
  //   const summary = 'Distributed Services Card update profile';
  //   const objectType = 'DSC';
  //   this.saveLabelsOperationDone = null;
  //   this.handleForkJoin(observables, summary, objectType);
  // }

  showDSCControlPlaneStatusHelper(stringMsgs: string, header: string, msgHeader: string) {
     // we don't want the panel collapse when clicking on "view reasons"
     event.stopPropagation();
     event.preventDefault();
     const delimiter = '<br/>';
     const msg = stringMsgs;
     this._controllerService.invokeConfirm({
       icon: 'pi pi-info-circle',
       header: header ,
       message: msgHeader + delimiter + msg,
       acceptLabel: 'Close',
       acceptVisible: true,
       rejectVisible: false,
       accept: () => {
         // When a primeng alert is created, it tries to "focus" on a button, not adding a button returns an error.
         // So we create a button but hide it later.
       }
     });
  }

  onShowDSCControlPlaneStatus($event, rowData: ClusterDistributedServiceCard) {
    if (!rowData.status['control-plane-status']) {
      return;
    }
    const reasons = this.getJSONStringList(rowData.status['control-plane-status']);
    const dscName = (rowData.spec.id) ? rowData.spec.id : rowData.meta.name;
    this.showDSCControlPlaneStatusHelper(reasons, 'Control Plane Status - ' + dscName, 'Peers') ;

  }

  onShowControlPlaneStatusPeer($event, rowData: ClusterDistributedServiceCard, w) {
    const reasons = this.getJSONStringList(w);
    const dscName = (rowData.spec.id) ? rowData.spec.id : rowData.meta.name;
    this.showDSCControlPlaneStatusHelper(reasons, 'Control Plane Status Peer - ' + dscName, 'Configuration') ;
  }
}
