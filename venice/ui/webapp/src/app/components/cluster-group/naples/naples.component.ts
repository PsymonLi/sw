import { Component, OnDestroy, OnInit, ViewChild, ViewEncapsulation } from '@angular/core';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { MetricsUtility } from '@app/common/MetricsUtility';
import { Utility } from '@app/common/Utility';
import { BaseComponent } from '@app/components/base/base.component';
import { HeroCardOptions } from '@app/components/shared/herocard/herocard.component';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { ClusterService } from '@app/services/generated/cluster.service';
import { MetricsPollingOptions, MetricsqueryService, TelemetryPollingMetricQueries, MetricsPollingQuery } from '@app/services/metricsquery.service';
import { ClusterDistributedServiceCard, IClusterDistributedServiceCard } from '@sdk/v1/models/generated/cluster';
import { Telemetry_queryMetricsQuerySpec } from '@sdk/v1/models/generated/telemetry_query';
import { Table } from 'primeng/table';
import { Subscription, forkJoin } from 'rxjs';
import { ITelemetry_queryMetricsQueryResponse, ITelemetry_queryMetricsQueryResult } from '@sdk/v1/models/telemetry_query';
import { StatArrowDirection, CardStates } from '@app/components/shared/basecard/basecard.component';
import { NaplesConditionValues } from '.';
import { AdvancedSearchComponent } from '@components/shared/advanced-search/advanced-search.component';
import { FormArray } from '@angular/forms';
import { SearchSearchRequest, SearchSearchResponse } from '@sdk/v1/models/generated/search';
import { SearchService } from '@app/services/generated/search.service';
import { LabelEditorMetadataModel } from '@components/shared/labeleditor';
import { RepeaterData, ValueType} from 'web-app-framework';
import { SearchUtil } from '@components/search/SearchUtil';
import * as _ from 'lodash';

@Component({
  selector: 'app-naples',
  encapsulation: ViewEncapsulation.None,
  templateUrl: './naples.component.html',
  styleUrls: ['./naples.component.scss']
})

/**
 * Added advanced search to naples table.
 *
 * When WatchDistributedServiceCard sends a response, we create a map mapping naples name (spec.id) to the naples object.
 * Whenever advanced search is used, a new searchrequest is created using this.advancedSearchComponent.getSearchRequest
 * Then we make an api call to get all the matching NICs using _callSearchRESTAPI.
 * These results do not contain spec information, so we lookup the original naples object from the naplesMap.
 * The matching naples objects are added to this.filteredNaples which is used to render the table.
 */

export class NaplesComponent extends BaseComponent implements OnInit, OnDestroy {
  @ViewChild('naplesTable') naplesTurboTable: Table;
  @ViewChild('advancedSearchComponent') advancedSearchComponent: AdvancedSearchComponent;

  naples: ReadonlyArray<ClusterDistributedServiceCard> = [];
  filteredNaples: ReadonlyArray<ClusterDistributedServiceCard> = [];
  selectedNaples: ClusterDistributedServiceCard[] = [];
  inLabelEditMode: boolean = false;
  labelEditorMetaData: LabelEditorMetadataModel;

  // Used for processing the stream events
  naplesEventUtility: HttpEventUtility<ClusterDistributedServiceCard>;
  naplesMap: { [napleName: string]: ClusterDistributedServiceCard };
  searchObject: { [field: string]: any} = {};
  conditionNaplesMap: { [condition: string]: Array<string> };

  fieldFormArray = new FormArray([]);
  maxRecords: number = 8000;

  cols: any[] = [
    { field: 'spec.id', header: 'Name', class: 'naples-column-date', sortable: true },
    { field: 'status.primary-mac', header: 'MAC Address', class: 'naples-column-id-name', sortable: true },
    { field: 'status.DSCVersion', header: 'Version', class: 'naples-column-version', sortable: true },
    { field: 'status.ip-config.ip-address', header: 'Management IP Address', class: 'naples-column-mgmt-cidr', sortable: false },
    { field: 'status.admission-phase', header: 'Phase', class: 'naples-column-phase', sortable: false },
    { field: 'status.conditions', header: 'Condition', class: 'naples-column-condition', sortable: true, localSearch: true},
    { field: 'status.host', header: 'Host', class: 'naples-column-host', sortable: true},
    { field: 'meta.mod-time', header: 'Modification Time', class: 'naples-column-date', sortable: true },
    { field: 'meta.creation-time', header: 'Creation Time', class: 'naples-column-date', sortable: true },
  ];
  subscriptions: Subscription[] = [];

  bodyicon: any = {
    margin: {
      top: '9px',
      left: '8px',
    },
    url: '/assets/images/icons/cluster/naples/ico-dsc-black.svg',
  };

  cardColor = '#b592e3';

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

  cpuChartData: HeroCardOptions = MetricsUtility.clusterLevelCPUHeroCard(this.cardColor, this.cardIcon);

  memChartData: HeroCardOptions = MetricsUtility.clusterLevelMemHeroCard(this.cardColor, this.cardIcon);

  diskChartData: HeroCardOptions = MetricsUtility.clusterLevelDiskHeroCard(this.cardColor, this.cardIcon);

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

  cancelSearch: boolean = false;

  constructor(private clusterService: ClusterService,
    protected controllerService: ControllerService,
    protected metricsqueryService: MetricsqueryService,
    protected searchService: SearchService,
  ) {
    super(controllerService);
  }


  ngOnInit() {
    this.getNaples();
    this.getMetrics();
    this.provideCustomOptions();
    this.controllerService.setToolbarData({
      buttons: [],
      breadcrumb: [{ label: 'Distributed Services Cards', url: Utility.getBaseUIUrl() + 'cluster/naples' }]
    });
  }

  updateSelectedNaples() {
    const tempMap = new Map<string, ClusterDistributedServiceCard>();
    for (const obj of this.naples) {
      tempMap[obj.meta.name] = obj;
    }
    const updatedSelectedObjects: ClusterDistributedServiceCard[] = [];
    for (const selObj of this.selectedNaples) {
      if (selObj.meta.name in tempMap) {
        updatedSelectedObjects.push(tempMap[selObj.meta.name]);
      }
    }
    this.selectedNaples = updatedSelectedObjects;
  }

  provideCustomOptions() {
    this.customQueryOptions = [
      {
        key: {label: 'Condition', value: 'Condition'},
        operators: SearchUtil.stringOperators,
        valueType: ValueType.multiSelect,
        values: [
          { label: 'Healthy', value: NaplesConditionValues.HEALTHY },
          { label: 'Unhealthy', value: NaplesConditionValues.UNHEALTHY },
          { label: 'Unknown', value: NaplesConditionValues.UNKNOWN },
          { label: 'Empty', value: NaplesConditionValues.EMPTY },
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
    this.conditionNaplesMap = {};
  }

  /**
   * Watches NapDistributed Services Cards data on KV Store and fetch new nic data
   * Generates column based search object, currently facilitates condition search
   */
  getNaples() {
    this._clearDSCMaps();
    this.naplesEventUtility = new HttpEventUtility<ClusterDistributedServiceCard>(ClusterDistributedServiceCard);
    this.naples = this.naplesEventUtility.array as ReadonlyArray<ClusterDistributedServiceCard>;
    this.filteredNaples = this.naplesEventUtility.array as ReadonlyArray<ClusterDistributedServiceCard>;
    const subscription = this.clusterService.WatchDistributedServiceCard().subscribe(
      response => {
        this.naplesEventUtility.processEvents(response);
        this._clearDSCMaps(); // VS-730.  Want to clear maps when we get updated data.
        for (const naple of this.naples) {
          this.naplesMap[naple.meta.name] = naple;
          // Create search object for condition
          switch (this.displayCondition(naple).toLowerCase()) {
            case NaplesConditionValues.HEALTHY:
                (this.conditionNaplesMap[NaplesConditionValues.HEALTHY] || (this.conditionNaplesMap[NaplesConditionValues.HEALTHY] = [])).push(naple.meta.name);
                break;
            case NaplesConditionValues.UNHEALTHY:
                (this.conditionNaplesMap[NaplesConditionValues.UNHEALTHY] || (this.conditionNaplesMap[NaplesConditionValues.UNHEALTHY] = [])).push(naple.meta.name);
                break;
            case NaplesConditionValues.UNKNOWN:
                (this.conditionNaplesMap[NaplesConditionValues.UNKNOWN] || (this.conditionNaplesMap[NaplesConditionValues.UNKNOWN] = [])).push(naple.meta.name);
                break;
            case '':
                (this.conditionNaplesMap['empty'] || (this.conditionNaplesMap['empty'] = [])).push(naple.meta.name);
                break;
          }
        }
        if (this.selectedNaples.length > 0) {
          this.updateSelectedNaples();
        }
        this.searchObject['status.conditions'] = this.conditionNaplesMap;
      },
      this._controllerService.webSocketErrorHandler('Failed to get NAPLES')
    );
    this.subscriptions.push(subscription); // add subscription to list, so that it will be cleaned up when component is destroyed.
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


  displayCondition(data: ClusterDistributedServiceCard): string {
    return Utility.getNaplesCondition(data);
  }

  isNICHealthy(data: ClusterDistributedServiceCard): boolean {
    if (Utility.isNaplesNICHealthy(data)) {
      return true;
    }
    return false;
  }

  displayReasons(data: ClusterDistributedServiceCard): any {
    return Utility.displayReasons(data);
  }

  isNICNotAdmitted(data: ClusterDistributedServiceCard): boolean {
    if (Utility.isNICConditionEmpty(data)) {
      return true;
    }
    return false;
  }

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
    return MetricsUtility.timeSeriesQueryPolling(this.telemetryKind);
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
    // Only require avg 5 min data and avg day data
    // before we show the cards
    if (MetricsUtility.resultHasData(this.avgData) &&
      MetricsUtility.resultHasData(this.avgDayData)) {
      this.genCharts('mean_CPUUsedPercent', this.cpuChartData);
      this.genCharts('mean_MemUsedPercent', this.memChartData);
      this.genCharts('mean_DiskUsedPercent', this.diskChartData);
    } else {
      MetricsUtility.setCardStatesNoData(this.heroCards);
    }
  }

  private genCharts(fieldName: string, heroCard: HeroCardOptions) {
    // Time series graph
    if (MetricsUtility.resultHasData(this.timeSeriesData)) {
      const timeSeriesData = this.timeSeriesData;

      const data = MetricsUtility.transformToChartjsTimeSeries(timeSeriesData.series[0], fieldName);
      heroCard.lineData.data = data;
    }

    // Current stat calculation - we take the last point
    if (MetricsUtility.resultHasData(this.avgData)) {
      const index = this.avgData.series[0].columns.indexOf(fieldName);
      heroCard.firstStat.value = Math.round(this.avgData.series[0].values[0][index]) + '%';
      heroCard.firstStat.numericValue = Math.round(this.avgData.series[0].values[0][index]);
    }

    // Avg
    const avgDayData = this.avgDayData;
    if (avgDayData.series[0].values.length !== 0) {
      const index = this.avgDayData.series[0].columns.indexOf(fieldName);
      heroCard.secondStat.value = Math.round(avgDayData.series[0].values[0][index]) + '%';
      heroCard.secondStat.numericValue = Math.round(avgDayData.series[0].values[0][index]);
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
    if (MetricsUtility.resultHasData(this.maxObjData)) {
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
            heroCard.thirdStat.url = '/cluster/naples/' + thirdStatNaples.meta.name;
            heroCard.thirdStat.numericValue = Math.round(maxNaples.max);
          }
        } else {
          heroCard.thirdStat.value = null;
        }
      }
    }

    if (heroCard.cardState !== CardStates.READY) {
      heroCard.cardState = CardStates.READY;
    }
  }

  getNaplesByKey(name: string): ClusterDistributedServiceCard {
    for (let index = 0; index < this.naples.length; index++) {
      const naple = this.naples[index];
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
   */
  getDistributedServiceCards(field = this.naplesTurboTable.sortField,
    order = this.naplesTurboTable.sortOrder) {
    const searchSearchRequest = this.advancedSearchComponent.getSearchRequest(field, order, 'DistributedServiceCard', true, this.maxRecords);
    const localSearchResult = this.advancedSearchComponent.getLocalSearchResult(field, order, this.searchObject);
    if (localSearchResult.err) {
      this.controllerService.invokeInfoToaster('Invalid', 'Length of search values don\'t match with accepted length');
      return;
    }
    if ((searchSearchRequest.query != null &&  (searchSearchRequest.query.fields != null && searchSearchRequest.query.fields.requirements != null
       && searchSearchRequest.query.fields.requirements.length > 0) || (searchSearchRequest.query.texts != null
         && searchSearchRequest.query.texts[0].text.length > 0)) || this.cancelSearch) {
          if (this.cancelSearch) {this.cancelSearch = false; }
          this._callSearchRESTAPI(searchSearchRequest, localSearchResult.searchRes);
    } else {
        // This case executed when only local search is required
        this.filteredNaples = this.generateFilteredNaples(localSearchResult.searchRes);
        if (this.filteredNaples.length === 0) {
          this.controllerService.invokeInfoToaster('Information', 'No NICs found. Please change search criteria.');
        }
    }
  }

  /**
   * Generates aggregate of local and/or remote advanced search filtered naples
   * @param tempNaples Array of local and/or remote advanced search filtered naples meta.name
   */
  generateFilteredNaples(tempNaples: string[]): ClusterDistributedServiceCard[] {
    let resultNics: ClusterDistributedServiceCard[] = [];
    this.lastUpdateTime = new Date().toISOString();
    if (tempNaples != null && tempNaples.length > 0) {
      resultNics = this.filterNaplesByName(tempNaples);
    }
    return resultNics;
  }

  populateFieldSelector() {
    this.fieldFormArray = new FormArray([]);
  }

    /**
   * This serves HTML API. It clear naples search and refresh data.
   * @param $event
   */
  onCancelSearch($event) {
    this.populateFieldSelector();
    this.cancelSearch = true;
    this.getDistributedServiceCards();
    this.controllerService.invokeInfoToaster('Information', 'Cleared search criteria, NICs refreshed.');
  }

  /**
   * API to perform remote search
   * @param searchSearchRequest remote search query
   * @param localSearchResult Array of filtered naples name after local search
   */
  private _callSearchRESTAPI(searchSearchRequest: SearchSearchRequest, localSearchResult: Array<string>) {
    const subscription = this.searchService.PostQuery(searchSearchRequest).subscribe(
      response => {
        const data: SearchSearchResponse = response.body as SearchSearchResponse;
        let objects = data.entries;
        if (!objects || objects.length === 0) {
          this.controllerService.invokeInfoToaster('Information', 'No NICs found. Please change search criteria.');
          objects = [];
        }
        const entries = [];
        const remoteSearchResult = [];
        for (let k = 0; k < objects.length; k++) {
          entries.push(objects[k].object); // objects[k] is a SearchEntry object
          remoteSearchResult.push(entries[k].meta.name);
        }
        const searchResult = this.advancedSearchComponent.generateAggregateResults(remoteSearchResult, localSearchResult);
        this.filteredNaples = this.generateFilteredNaples(searchResult);
      },
      (error) => {
        this.controllerService.invokeRESTErrorToaster('Failed to get naples', error);
      }
    );
    this.subscriptions.push(subscription);
  }

  /**
   * Provides nic objects for nic meta.name
   * @param entries Array of nic meta.name
   */
  filterNaplesByName(entries: string[]): ClusterDistributedServiceCard[] {
    const tmpMap = {};
    entries.forEach(ele => {
      tmpMap[ele] = this.naplesMap[ele];
    });
    return Object.values(tmpMap);
}

  editLabels() {
    this.labelEditorMetaData = {
      title: 'Editing naples objects',
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

  updateWithForkjoin(updatedNaples) {
    const observables = [];
    for (const naplesObject of updatedNaples) {
      const name = naplesObject.meta.name;
      const sub = this.clusterService.UpdateDistributedServiceCard(name, naplesObject, '', this.naplesMap[name].$inputValue);
      observables.push(sub);
    }

    const summary = 'Distributed Services Card update';

    forkJoin(observables).subscribe(
      (results: any[]) => {

        let successCount: number = 0;
        let failCount: number = 0;
        const errors: string[] = [];
        for (let i = 0; i < results.length; i++) {
          if (results[i]['statusCode'] === 200) {
            successCount += 1;
          } else {
            failCount += 1;
            errors.push(results[i].body.message);
          }
        }

        if (successCount > 0) {
          const msg = 'Successfully updated ' + successCount.toString() + ' naples.';
          this._controllerService.invokeSuccessToaster(summary, msg);
          this.inLabelEditMode = false;
        }
        if (failCount > 0) {
          this._controllerService.invokeRESTErrorToaster(summary, errors.join('\n'));
        }
    },
    this._controllerService.restErrorHandler(summary + ' Failed')
    );
  }

  // The save emitter from labeleditor returns the updated objects here.
  // We use forkjoin to update all the naples.
  handleEditSave(updatedNaples: ClusterDistributedServiceCard[]) {
    this.updateWithForkjoin(updatedNaples);
  }

  handleEditCancel($event) {
    this.inLabelEditMode = false;
  }

  ngOnDestroy() {
    this.subscriptions.forEach(subscription => {
      subscription.unsubscribe();
    });
  }
}
