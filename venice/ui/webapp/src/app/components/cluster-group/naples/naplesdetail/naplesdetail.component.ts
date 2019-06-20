import {Component, OnInit, OnDestroy} from '@angular/core';
import { BaseComponent } from '@app/components/base/base.component';
import { Animations } from '@app/animations';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ClusterSmartNIC, ClusterSmartNICStatus_admission_phase_uihint } from '@sdk/v1/models/generated/cluster';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { HeroCardOptions } from '@app/components/shared/herocard/herocard.component';
import { MetricsUtility } from '@app/common/MetricsUtility';
import { ITelemetry_queryMetricsQueryResponse } from '@sdk/v1/models/telemetry_query';
import { ControllerService } from '@app/services/controller.service';
import { ActivatedRoute } from '@angular/router';
import { ClusterService } from '@app/services/generated/cluster.service';
import { MetricsqueryService, MetricsPollingOptions, TelemetryPollingMetricQueries, MetricsPollingQuery } from '@app/services/metricsquery.service';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { Utility } from '@app/common/Utility';
import { Telemetry_queryMetricsQuerySpec, ITelemetry_queryMetricsQuerySpec, Telemetry_queryMetricsQuerySpec_function, Telemetry_queryMetricsQuerySpec_sort_order } from '@sdk/v1/models/generated/telemetry_query';
import { ITelemetry_queryMetricsQueryResult } from '@sdk/v1/models/telemetry_query';
import { AlertsEventsSelector } from '@app/components/shared/alertsevents/alertsevents.component';
import { StatArrowDirection, CardStates } from '@app/components/shared/basecard/basecard.component';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import {LabelEditorMetadataModel} from '@components/shared/labeleditor';
import { NaplesConditionValues } from '..';

@Component({
  selector: 'app-naplesdetail',
  templateUrl: './naplesdetail.component.html',
  styleUrls: ['./naplesdetail.component.scss'],
  animations: [Animations]
})
export class NaplesdetailComponent extends BaseComponent implements OnInit, OnDestroy {
  subscriptions = [];

  bodyicon: any = {
    margin: {
      top: '9px',
      left: '8px'
    },
    url: '/assets/images/icons/cluster/nodes/ico-node-black.svg'
  };

  cardColor = '#61b3a0';
  cardIcon: Icon = {
    margin: {
      top: '10px',
      left: '10px'
    },
    svgIcon: 'naples'
  };
  // Id of the object the user has navigated to
  selectedId: string;
  selectedObj: ClusterSmartNIC;

  showExpandedDetailsCard: boolean;

  // Holds all objects, should be only one item in the array
  objList: ReadonlyArray<ClusterSmartNIC>;
  objEventUtility: HttpEventUtility<ClusterSmartNIC>;

  // Whether we show a deletion overlay
  showDeletionScreen: boolean;

  // Whether we show a missing overlay
  showMissingScreen: boolean;

  lastUpdateTime: string = '';

  cpuChartData: HeroCardOptions = MetricsUtility.detailLevelCPUHeroCard(this.cardColor, this.cardIcon);

  memChartData: HeroCardOptions = MetricsUtility.detailLevelMemHeroCard(this.cardColor, this.cardIcon);

  diskChartData: HeroCardOptions = MetricsUtility.detailLevelDiskHeroCard(this.cardColor, this.cardIcon);

  admissionPhaseEnum = ClusterSmartNICStatus_admission_phase_uihint;

  heroCards = [
    this.cpuChartData,
    this.memChartData,
    this.diskChartData
  ];

  timeSeriesData: ITelemetry_queryMetricsQueryResult;
  avgData: ITelemetry_queryMetricsQueryResult;
  avgDayData: ITelemetry_queryMetricsQueryResult;
  clusterAvgData: ITelemetry_queryMetricsQueryResult;

  telemetryKind: string = 'SmartNIC';

  alertseventsSelector: AlertsEventsSelector;

  inLabelEditMode: boolean = false;

  labelEditorMetaData: LabelEditorMetadataModel;

  constructor(protected _controllerService: ControllerService,
    private _route: ActivatedRoute,
    protected clusterService: ClusterService,
    protected metricsqueryService: MetricsqueryService,
    protected UIConfigService: UIConfigsService
  ) {
    super(_controllerService, UIConfigService);
  }

  ngOnInit() {
    this.initializeData();
    this._controllerService.publish(Eventtypes.COMPONENT_INIT, { 'component': 'NapledetailComponent', 'state': Eventtypes.COMPONENT_INIT });
    this._route.paramMap.subscribe(params => {
      const id = params.get('id');
      this.selectedId = id;
      this.initializeData();
      this.getNaplesDetails();
      this._controllerService.setToolbarData({
        buttons: [],
        breadcrumb: [
          { label: 'Naples', url: Utility.getBaseUIUrl() + 'cluster/naples' },
          { label: id, url: Utility.getBaseUIUrl() + 'cluster/naples/' + id }]
      });
    });
  }

  initializeData() {
    // Initializing variables so that state is cleared between routing of different
    // sgpolicies
    // Ex. /cluster/naples/naples1-> /cluster/naples/naples2
    this.subscriptions.forEach(sub => {
      sub.unsubscribe();
    });
    this.subscriptions = [];
    this.objList = [];
    this.showDeletionScreen = false;
    this.showMissingScreen = false;
    this.timeSeriesData = null;
    this.avgData = null;
    this.avgDayData = null;
    this.clusterAvgData = null;
    this.objList = [];
    this.selectedObj = null;
    this.alertseventsSelector = null;
    this.showExpandedDetailsCard = false;

    this.heroCards.forEach((card) => {
      card.firstStat.value = null;
      card.secondStat.value = null;
      card.thirdStat.value = null;
      card.data = { x: [], y: [] };
      card.cardState = CardStates.LOADING;
    });
  }

  getNaplesDetails() {
    // We perform a get as well as a watch so that we can know if the object the user is
    // looking for exists or not.
    // Adding ':' as a temporary workaround of ApiGw not being able to
    // correctly parse smartNic names without it.
    const getSubscription = this.clusterService.GetSmartNIC(this.selectedId + ':').subscribe(
      response => {
        // We do nothing, and wait for the callback of the watch to populate the view
      },
      error => {
        // If we receive any error code we display object is missing
        // TODO: Update to be more descriptive based on error message
        this.showMissingScreen = true;
        this.heroCards.forEach(card => {
          card.cardState = CardStates.READY;
        });
      }
    );
    this.subscriptions.push(getSubscription);
    this.objEventUtility = new HttpEventUtility<ClusterSmartNIC>(ClusterSmartNIC);
    this.objList = this.objEventUtility.array;
    const subscription = this.clusterService.WatchSmartNIC({ 'field-selector': 'meta.name=' + this.selectedId }).subscribe(
      response => {
        this.objEventUtility.processEvents(response);
        if (this.objList.length > 1) {
          // because of the name selector, we should
          // have only got one object
          console.error(
            'Received more than one naples object. Expected '
            + this.selectedId + ', received ' +
            this.objList.map((naples) => naples.meta.name).join(', '));
        }
        if (this.selectedObj == null && this.objList.length > 0) {
          // In case object was deleted and then readded while we are on the same screen
          this.showDeletionScreen = false;
          // In case object wasn't created yet and then was added while we are on the same screen
          this.showMissingScreen = false;
          this.selectedObj = this.objList[0];
          this.alertseventsSelector = {
            eventSelector: {
              selector: 'object-ref.name=' + this.selectedObj.spec.id + ',object-ref.kind=SmartNIC',
              name: this.selectedObj.spec.id,
            },
            alertSelector: {
              selector: 'status.object-ref.name=' + this.selectedObj.spec.id + ',status.object-ref.kind=SmartNIC',
              name: this.selectedObj.spec.id
            }
          };
          this.startMetricPolls();
        } else if (this.objList.length === 0) {
          // Must have received a delete event.
          this.showDeletionScreen = true;
          this.heroCards.forEach(card => {
            card.cardState = CardStates.READY;
          });
          this.selectedObj = null;
        }
      },
    );
    this.subscriptions.push(subscription);
  }

  startMetricPolls() {
    const queryList: TelemetryPollingMetricQueries = {
      queries: [],
      tenant: Utility.getInstance().getTenant()
    };
    queryList.queries.push(this.timeSeriesQuery());
    queryList.queries.push(this.avgQuery());
    queryList.queries.push(this.avgDayQuery());
    queryList.queries.push(this.clusterAvgQuery());
    const sub = this.metricsqueryService.pollMetrics('naplesDetailCards', queryList).subscribe(
      (data: ITelemetry_queryMetricsQueryResponse) => {
        if (data && data.results && data.results.length === 4) {
          this.timeSeriesData = data.results[0];
          this.avgData = data.results[1];
          this.avgDayData = data.results[2];
          this.clusterAvgData = data.results[3];
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
    let name = this.selectedId;
    if (this.selectedObj != null) {
      name = MetricsUtility.generateNaplesReporterId(this.selectedObj);
    }
    return MetricsUtility.timeSeriesQueryPolling(this.telemetryKind, MetricsUtility.createNameSelector(name));
  }

  avgQuery(): MetricsPollingQuery {
    let name = this.selectedId;
    if (this.selectedObj != null) {
      name = MetricsUtility.generateNaplesReporterId(this.selectedObj);
    }
    return MetricsUtility.pastFiveMinAverageQueryPolling(this.telemetryKind, MetricsUtility.createNameSelector(name));
  }

  avgDayQuery(): MetricsPollingQuery {
    let name = this.selectedId;
    if (this.selectedObj != null) {
      name = MetricsUtility.generateNaplesReporterId(this.selectedObj);
    }
    return MetricsUtility.pastDayAverageQueryPolling(this.telemetryKind, MetricsUtility.createNameSelector(name));
  }

  clusterAvgQuery(): MetricsPollingQuery {
    const clusterAvgQuery: ITelemetry_queryMetricsQuerySpec = {
      'kind': this.telemetryKind,
      name: null,
      function: Telemetry_queryMetricsQuerySpec_function.MEAN,
      'sort-order': Telemetry_queryMetricsQuerySpec_sort_order.Ascending,
      // We don't specify the fields we need, as specifying more than one field
      // while using the average function isn't supported by the backend.
      // Instead we leave blank and get all fields
      fields: [],
      // We only look at the last 5 min bucket so that the max node reporting is never
      // lower than current
      'start-time': new Date(Utility.roundDownTime(5).getTime() - 1000 * 50 * 5).toISOString() as any,
      'end-time': Utility.roundDownTime(5).toISOString() as any
    };

    const query = new Telemetry_queryMetricsQuerySpec(clusterAvgQuery);
    const timeUpdate = (queryBody: ITelemetry_queryMetricsQuerySpec) => {
      queryBody['start-time'] = new Date(Utility.roundDownTime(5).getTime() - 1000 * 50 * 5).toISOString() as any,
        queryBody['end-time'] = Utility.roundDownTime(5).toISOString() as any;
    };

    const merge = (currData, newData) => {
      // since we round down to get the last 5 min bucket, there's a chance
      // that we can back null data, since no new metrics have been reported.
      // Data should have been filtered in metricsquery services's processData
      if (!MetricsUtility.resultHasData(newData)) {
        // no new data, keep old value
        return currData;
      }
      return newData;
    };

    const pollOptions = {
      timeUpdater: timeUpdate,
      mergeFunction: merge
    };

    return { query: query, pollingOptions: pollOptions };
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

  genCharts(fieldName, heroCard: HeroCardOptions) {
    // Time series graph
    if (MetricsUtility.resultHasData(this.timeSeriesData)) {
      const timeSeriesData = this.timeSeriesData;

      const index = timeSeriesData.series[0].columns.indexOf(fieldName);
      const data = Utility.transformToPlotly(timeSeriesData.series[0].values, 0, index);
      heroCard.data = data;
    }

    // Current stat calculation - we take the last point
    if (MetricsUtility.resultHasData(this.avgData)) {
      const index = this.avgData.series[0].columns.indexOf(fieldName);
      heroCard.firstStat.value = Math.round(this.avgData.series[0].values[0][index]) + '%';
    }

    // Avg
    const avgDayData = this.avgDayData;
    if (avgDayData.series[0].values.length !== 0) {
      const index = this.avgDayData.series[0].columns.indexOf(fieldName);
      heroCard.secondStat.value = Math.round(avgDayData.series[0].values[0][index]) + '%';
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

    // Cluster average
    if (this.clusterAvgData.series[0].values.length !== 0) {
      const index = this.clusterAvgData.series[0].columns.indexOf(fieldName);
      heroCard.thirdStat.value = Math.round(this.clusterAvgData.series[0].values[0][index]) + '%';
    }

    if (heroCard.cardState !== CardStates.READY) {
      heroCard.cardState = CardStates.READY;
    }
  }

  /**
   * Component is about to exit
   */
  ngOnDestroy() {
    // publish event that AppComponent is about to be destroyed
    this._controllerService.publish(Eventtypes.COMPONENT_DESTROY, { 'component': 'NaplesdetailComponent', 'state': Eventtypes.COMPONENT_DESTROY });
    this.subscriptions.forEach(subscription => {
      subscription.unsubscribe();
    });
  }

  editLabels($event) {
    this.labelEditorMetaData = {
      title: this.selectedObj.meta.name,
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

  handleEditCancel($event) {
    this.inLabelEditMode = false;
  }

  handleEditSave(labels: object) {
    const name = this.selectedObj.meta.name;
    this.selectedObj.meta.labels = labels;
    const sub = this.clusterService.UpdateSmartNIC(name, this.selectedObj).subscribe(response => {
      this._controllerService.invokeSuccessToaster(Utility.UPDATE_SUCCESS_SUMMARY, `Successfully updated ${this.selectedObj.meta.name}'s labels`);
    }, this._controllerService.restErrorHandler(Utility.UPDATE_FAILED_SUMMARY));
    this.subscriptions.push(sub);
    this.inLabelEditMode = false;
  }

  /**
   * TODO: have a local variable storing this to avoid repetitive computation
   */
  genKeys(): string[] {
    return Object.keys(this.selectedObj.meta.labels);
  }

  ifLabelExists(): boolean {
    if (this.selectedObj) {
      if (Utility.getLodash().isEmpty(this.selectedObj.meta.labels)) {
        return false;
      } else {
        return true;
      }
    }
    return false;
  }

  toggleDetailsCard() {
    this.showExpandedDetailsCard = !this.showExpandedDetailsCard;
  }

  helpDisplayCondition(data: ClusterSmartNIC): NaplesConditionValues {
    return Utility.getNaplesCondition(data);
  }

  helpDisplayReasons(data: ClusterSmartNIC): any {
    return Utility.displayReasons(data);
  }

  isNICHealthy(data: ClusterSmartNIC): boolean {
  if (Utility.isNaplesNICHealthy(data)) {
      return true;
    }
    return false;
  }
}
