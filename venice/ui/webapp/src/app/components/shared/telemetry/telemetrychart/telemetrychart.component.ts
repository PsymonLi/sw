import { Component, ViewChild, ViewEncapsulation, OnInit, OnDestroy, Input, Output, EventEmitter, SimpleChanges, OnChanges } from '@angular/core';
import { Animations } from '@app/animations';
import { MetricsUtility } from '@app/common/MetricsUtility';
import { Utility } from '@app/common/Utility';
import { BaseComponent } from '@app/components/base/base.component';
import { ControllerService } from '@app/services/controller.service';
import { MetricsqueryService, TelemetryPollingMetricQueries, MetricsPollingQuery } from '@app/services/metricsquery.service';
import { ITelemetry_queryMetricsQueryResponse, ITelemetry_queryMetricsQueryResult, Telemetry_queryMetricsResultSeries } from '@sdk/v1/models/telemetry_query';
import { ChartOptions, ChartLegendLabelItem } from 'chart.js';
import { Observer, Subject, Subscription } from 'rxjs';
import { sourceFieldKey, getFieldData } from '../utility';
import { MetricMeasurement, MetricsMetadata } from '@sdk/metrics/generated/metadata';
import { ChartData, ChartDataSets, ColorTransform, DisplayLabelTransform, GroupByTransform, DataSource, MetricTransform, TransformDataset, TransformDatasets, GraphTransform, SourceMeta } from '../transforms';
import { AxisTransform } from '../transforms/axis.transform';
import { TimeRange } from '@app/components/shared/timerange/utility';
import { UIChartComponent } from '@app/components/shared/primeng-chart/chart';
import { FieldSelectorTransform } from '../transforms/fieldselector.transform';
import { FieldValueTransform, ValueMap, QueryMap } from '../transforms/fieldvalue.transform';
import { ClusterDistributedServiceCard, ClusterNode, ClusterDistributedServiceCardStatus_admission_phase, ClusterDSCProfile } from '@sdk/v1/models/generated/cluster';
import { ClusterService } from '@app/services/generated/cluster.service';
import { ITelemetry_queryMetricsQuerySpec, Telemetry_queryMetricsQuerySpec_function } from '@sdk/v1/models/generated/telemetry_query';
import { LabelSelectorTransform } from '../transforms/labelselector.transform';
import { AuthService } from '@app/services/generated/auth.service';
import { TelemetryPref, GraphConfig, DataTransformConfig } from '@app/models/frontend/shared/userpreference.interface';
import * as moment from 'moment';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { GraphTitleTransform } from '../transforms/graphtitle.transform';
import { GroupByTimeTransform } from '../transforms/groupbytime.transform';
import { RoundCountersTransform } from '../transforms/roundcounters.transform';
import { debounceTime } from 'rxjs/operators';
import { NetworkService } from '@app/services/generated/network.service';
import { NetworkNetworkInterface } from '@sdk/v1/models/generated/network';
import { ObjectsRelationsUtility } from '@app/common/ObjectsRelationsUtility';

/**
 * A data source allows a user to select a single measurement,
 * multiple fields, and other options such as filtering.
 *
 * A data source translates to one or more telemetry queries.
 * Each data source has a set of transforms registered. These transforms will
 * have their hooks called, and are able to modify the telemetry query, as well
 * as the response data set.
 *
 * Each transform should ideally be independent of each other, but it is
 * possible for transforms to read state from each other.
 *
 * Graph transforms are similar, but only affect graph level options.
 *
 * To see methods and hooks that transforms have access to, look at transforms/types.ts
 *
 */


@Component({
  selector: 'app-telemetrychart',
  templateUrl: './telemetrychart.component.html',
  styleUrls: ['./telemetrychart.component.scss'],
  encapsulation: ViewEncapsulation.None,
  animations: [Animations]
})
export class TelemetrychartComponent extends BaseComponent implements OnInit, OnChanges, OnDestroy {
  public static MAX_LEGEND_EDIT_MODE: number = 20;
  public static MAX_LEGEND_VIEW_MODE: number = 8;


  @ViewChild('pChart') chartContainer: UIChartComponent;
  @Input() chartConfig: GraphConfig;
  @Input() inEditMode: boolean = false;
  @Input() inDebugMode: boolean = false;
  @Input() selectedTimeRange: TimeRange;
  @Output() saveChartReq: EventEmitter<GraphConfig> = new EventEmitter<GraphConfig>();

  graphLoading: boolean = false;

  themeColor: string = '#b592e3';

  icon: Icon = {
    margin: {
      top: '8px',
      left: '0px'
    },
    matIcon: 'insert_chart_outlined'
  };

  telemetryPref: TelemetryPref = {
    items: [],
    configuredGraphs: {}
  };

  lineData: ChartData = {
    datasets: []
  };

  // The metric source that is currently selected for editing
  dataSources: DataSource[] = [];

  graphTypeOptions = [{
    label: 'Line Graph',
    value: 'Line Graph'
  }];
  selectedGraphType = 'Line Graph';

  metricsMetadata: { [key: string]: MetricMeasurement } = MetricsMetadata;

  graphOptions: ChartOptions = this.generateDefaultGraphOptions();

  // Holds last response from the metric poll
  metricData: ITelemetry_queryMetricsQueryResponse;

  // Subscription for the metric polling
  metricSubscription: Subscription;
  subscriptions: Subscription[] = [];

  // Metrics observer, used for debouncing metric requests
  metricsQueryObserver: Subject<TelemetryPollingMetricQueries> = new Subject();

  // Last query we sent to the poll utility
  lastQuery: TelemetryPollingMetricQueries = null;
  // last time range used for the last query
  lastTimeRange: TimeRange = null;

  // Subject to be passed into data sources so that they can request data
  getMetricsSubject: Subject<any> = new Subject<any>();

  // Subject to be passed into graph transforms so that they can request graph redraw
  reqRedrawSubject: Subject<any> = new Subject<any>();

  graphTransforms: GraphTransform<any>[] = [
    new GraphTitleTransform(),
    new AxisTransform()
    // comment out DerivativeTransform to avoid TCP Session to negative numbers
    // new DerivativeTransform(),
  ];

  networkInterfacesTypeMap: { [key: string]: NetworkNetworkInterface[] } = {};
  naples: ReadonlyArray<ClusterDistributedServiceCard> = [];
  networkInterfaces: NetworkNetworkInterface[] = [];
  dscProfiles: ReadonlyArray<ClusterDSCProfile> = [];
  nodes: ReadonlyArray<ClusterNode> = [];

  macAddrToName: { [key: string]: string; } = {};
  nameToMacAddr: { [key: string]: string; } = {};
  interfaceNameMap: { [key: string]: string; } = {};

  // Map from object kind to map from key to map of values to object names
  // that have that label
  labelMap:
    {
      [kind: string]:
      {
        [labelKey: string]:
        { [labelValue: string]: string[] }
      }
    } = {};


  fieldQueryMap: QueryMap = {};
  fieldValueMap: ValueMap = {};

  // Flag to indicate whether we have finished loading a user's config
  // Used to prevent fetching metrics before all configs are loaded
  configLoaded: boolean = false;

  constructor(protected controllerService: ControllerService,
    protected clusterService: ClusterService,
    protected authService: AuthService,
    protected telemetryqueryService: MetricsqueryService,
    protected networkService: NetworkService,
  ) {
    super(controllerService);
  }

  ngOnInit() {
    this.metricsQueryListener();
    this.setupValueOverrides();
    this.getNaples();
    this.getNodes();
    this.getDSCProfiles();

    if (this.chartConfig == null) {
      this.chartConfig = {
        id: '',
        graphTransforms: {
          transforms: {},
        },
        dataTransforms: []
      };
    }

    const metricsSubjectSubscription = this.getMetricsSubject.subscribe(() => {
      if (this.configLoaded) {
        this.getMetrics();
      }
    });
    this.subscriptions.push(metricsSubjectSubscription);

    const reqDrawSub = this.reqRedrawSubject.subscribe(() => {
      this.drawGraph();
    });
    this.subscriptions.push(reqDrawSub);

    // Populating graph transform reqRedraw
    this.graphTransforms.forEach((t) => {
      // Casting to any so we can set private variables
      (<any>t).reqRedraw = this.reqRedrawSubject;
    });

    if (this.chartConfig != null && this.chartConfig.id !== '') {
      this.loadConfig();
    }
    this.configLoaded = true;

    if (this.dataSources.length === 0) {
      this.addDataSource();
    }
  }

  setupValueOverrides() {
    const queryMapNaplesReporterID = (res: ITelemetry_queryMetricsQuerySpec) => {
      const field = 'reporterID';
      res.selector.requirements.forEach((req) => {
        if (req.key === field) {
          req.values = req.values.map((v) => {
            const mac = this.nameToMacAddr[v];
            if (mac != null) {
              return mac;
            }
            return v;
          });
        }
      });
    };
    const valueMapNaplesReporterID = (res: ITelemetry_queryMetricsQueryResult) => {
      const fieldDSC = 'reporterID';
      res.series.forEach((s: Telemetry_queryMetricsResultSeries) => {
        const sourceTags = s.tags as SourceMeta;
        if (sourceTags != null) {
          const tagValDSC = sourceTags[fieldDSC];
          if (tagValDSC != null && this.macAddrToName[tagValDSC] != null) {
            // VS-1600 FieldValueTransform.transformMetricData() call back to here.  Decorate Labels
            const dscName = this.macAddrToName[tagValDSC];
            const isAdmitted = this.isDSCAdmittedByDSCMac(tagValDSC);
            const dsc = this.naples.find( (nic: ClusterDistributedServiceCard) => nic.meta.name === tagValDSC);
            sourceTags[fieldDSC] = (dsc && isAdmitted) ? dscName : dscName +  ' (' + dsc.status['admission-phase'] + ')';
          } else if (sourceTags.name && this.interfaceNameMap[sourceTags.name])  { // for interface
            const dscMac = sourceTags.name.substring(0, 14);
            const naple = this.naples.find(dsc => dsc.meta.name === dscMac);
            let admitted: boolean = true;
            if (naple) {
              admitted = naple.status['admission-phase'] === ClusterDistributedServiceCardStatus_admission_phase.admitted;
            }
            sourceTags.name = admitted ? this.interfaceNameMap[sourceTags.name] :
                this.interfaceNameMap[sourceTags.name] + ' (' + naple.status['admission-phase'] + ')';
          }
        }
        const fieldIndex = s.columns.findIndex((f) => {
          return f.includes(fieldDSC);
        });
        s.values = s.values.map((v) => {
          const mac = v[fieldIndex];
          if (this.macAddrToName[mac] != null) {
            v[fieldIndex] = this.macAddrToName[mac];
          }
          return v;
        });
      });
    };
    Object.keys(MetricsMetadata).forEach((m) => {
      const measurement = MetricsMetadata[m];
      // add DSC and NI related meta-info into maps. FieldValueTransform needs it
      if (measurement.objectKind === 'DistributedServiceCard' || measurement.objectKind ===  'NetworkInterface') {
        this.fieldQueryMap[measurement.name] = queryMapNaplesReporterID;
        this.fieldValueMap[measurement.name] = valueMapNaplesReporterID;
      }
    });
  }

  ngOnChanges(changes: SimpleChanges) {
    // If the time range is changed, we refetch metrics.
    // We don't want this to run on first change because the data sources
    // might not be ready yet.
    if (changes.selectedTimeRange != null && !changes.selectedTimeRange.isFirstChange()) {
      this.getMetrics();
    }
    if (changes.inDebugMode) {
      this.dataSources.forEach((s) => {
        s.debugMode = this.inDebugMode;
      });
    }
  }

  loadConfig() {
    // loading graph transforms
    // We clone the preference before loading the config
    // so that incase the transform modifies the config,
    // if a user then cancels the graph edit the inital user
    // preference content is preserved
    const _ = Utility.getLodash();
    this.graphTransforms.forEach((t) => {
      t.load(_.cloneDeep(this.chartConfig.graphTransforms.transforms[t.transformName]));
    });

    this.chartConfig.dataTransforms.forEach((dataTransform) => {
      const source = this.addDataSource();
      source.load(_.cloneDeep(dataTransform));
    });
    this.configLoaded = true;
    this.getMetrics();
  }

  saveConfig() {
    // Saving graph config
    const graphTransforms = {};
    this.graphTransforms.forEach((t) => {
      graphTransforms[t.transformName] = t.save();
    });
    this.chartConfig.graphTransforms.transforms = graphTransforms;
    const dataSourceConfig: DataTransformConfig[] = [];
    this.dataSources.forEach(source => {
      if (source.measurement == null || source.fields.length === 0) {
        return; // Don't save an incomplete query
      }
      dataSourceConfig.push(source.save());
    });
    this.chartConfig.dataTransforms = dataSourceConfig;
    // The chart id will be populated by telemetry.component.ts before saving.

    // emit back to parent, which has the full user pref object.
    this.saveChartReq.emit(this.chartConfig);
  }


  getNodes() {
    const nodeSubscription = this.clusterService.ListNodeCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.nodes = response.data;
        this.labelMap['Node'] = Utility.getLabels(this.nodes as any[]);
        this.getMetrics();
      },
      this._controllerService.webSocketErrorHandler('Failed to get nodes')
    );
    this.subscriptions.push(nodeSubscription);
  }


  getNaples() {
    const dscSubscription = this.clusterService.ListDistributedServiceCardCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.naples = response.data as ClusterDistributedServiceCard[];
        // For testing VS-1600, force a dsc to be decommissioned. // this.naples.find( dsc => dsc.spec.id === 'oob-esxdual-1dc0').status['admission-phase'] = ClusterDistributedServiceCardStatus_admission_phase.decommissioned;
        this.labelMap['DistributedServiceCard'] = Utility.getLabels(this.naples as any[]);
        // mac-address to name map
        this.macAddrToName = {};
        this.nameToMacAddr = {};
        for (const nic of this.naples) {
          this.macAddrToName[nic.meta.name] = nic.spec.id;
          if (nic.spec.id != null || nic.spec.id.length > 0) {
            this.nameToMacAddr[nic.spec.id] = nic.meta.name;
          }
        }
        // have to load naples before interfaces to make sure the label of interfaces are correct
        this.getNetworkInterfaces();
        this.getMetrics();
      },
      this.controllerService.webSocketErrorHandler('Failed to get DSCs')
    );
    this.subscriptions.push(dscSubscription);
  }

 /*  hack_networkinterface() {
    const networkInterfaces = this.networkInterfaces.find( ni => ni.meta.name === 'oob-esxdual-1dc0-uplink-1-2');
  } */
  getNetworkInterfaces() {
    const sub = this.networkService.ListNetworkInterfaceCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.interfaceNameMap = {};
        this.networkInterfacesTypeMap = {};
        this.networkInterfaces = response.data as NetworkNetworkInterface[];
        for (const i of this.networkInterfaces) {
          if (!Utility.isInterfaceInValid(i)) {
            if (!this.networkInterfacesTypeMap[i.status.type]) {
              this.networkInterfacesTypeMap[i.status.type] = [];
            }
            this.networkInterfacesTypeMap[i.status.type].push(i);
            this.interfaceNameMap[i.meta.name] =
                ObjectsRelationsUtility.getNetworkinterfaceUIName(
                  i, this.naples as ClusterDistributedServiceCard[]);
          }
        }
      }
    );
    this.subscriptions.push(sub);
  }

  getDSCProfiles() {
    const subscription = this.clusterService.ListDSCProfileCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        this.dscProfiles = response.data as ClusterDSCProfile[];
      },
      this._controllerService.webSocketErrorHandler('Failed to get DSC Profile')
    );
    this.subscriptions.push(subscription); // add subscription to list, so that it will be cleaned up when component is destroyed.
  }

  checkIfQueriesSelectorChanged(newQuery: TelemetryPollingMetricQueries, oldQuery: TelemetryPollingMetricQueries): boolean {
    if (oldQuery == null || oldQuery.queries.length === 0 || newQuery == null || newQuery.queries.length === 0) {
      return false;
    }
    return oldQuery.queries.some((query, index) => {
      return !Utility.getLodash().isEqual(query.query.selector, newQuery.queries[index].query.selector);
    });
  }

  generateDefaultGraphOptions(): ChartOptions {
    const self = this;
    const options: ChartOptions = {
      title: {
        display: false,
      },
      legend: {
        display: true,
        position: 'bottom'
      },
      tooltips: {
        enabled: true,
        intersect: true,
        mode: 'nearest',
        titleFontFamily: 'Fira Sans Condensed',
        bodyFontFamily: 'Fira Sans Condensed'
      },
      layout: {
        padding: {
          left: 15,
          right: 15,
          top: 15,
          bottom: 0
        }
      },
      animation: {
        duration: 0
      },
      scales: {
        xAxes: [{
          id: 'telemetrychart-x-axis-time',
          type: 'time',
          time: {
            parser: data => moment.utc(data),
            tooltipFormat: 'YYYY-MM-DD HH:mm',
            displayFormats: {
              millisecond: 'HH:mm:ss.SSS',
              second: 'HH:mm:ss',
              minute: 'HH:mm',
              hour: 'HH'
            },
          },
          display: true,
          gridLines: {
            display: true
          },
          ticks: {
            callback: function (value, index, values) {
              if (!values[index]) {
                return;
              }

              const currMoment = moment.utc((values[index] as any).value);
              return currMoment.format('HH:mm');
            },
          },
        }, {
          id: 'telemetrychart-x-axis-date',
          type: 'time',
          gridLines: {
            display: false,
            drawBorder: false,
            drawTicks: false,
          },
          ticks: {
            callback: function (value, index, values) {
              if (!values[index]) {
                return;
              }

              const currMoment = moment.utc((values[index] as any).value);
              const prevMoment = index > 0 && values[index - 1] ? moment.utc((values[index - 1] as any).value) : null;
              const sameDay = prevMoment && currMoment.isSame(prevMoment, 'day');
              return sameDay ? '' : currMoment.format('MM/DD');
            },
          },
        }],
        yAxes: [{
          gridLines: {
            display: true
          },
          display: true,
          scaleLabel: {
            display: true
          },
          ticks: {
            display: false,
          }
        }]
      }
    };
    return options;
  }

  addDataSource(): DataSource {
    const source = new DataSource(this.getMetricsSubject as Observer<any>, [
      new RoundCountersTransform(),
      new DisplayLabelTransform(), // This needs to be before groupByTransform
      new ColorTransform(),
      new GroupByTransform(),
      new GroupByTimeTransform(),
      new FieldSelectorTransform(),
      new LabelSelectorTransform(this.labelMap),
      new FieldValueTransform(this.fieldQueryMap, this.fieldValueMap), // This needs to be last to transform the query properly
    ]);
    source.debugMode = this.inDebugMode;
    this.dataSources.push(source);
    return source;
  }

  removeDataSource(index: number) {
    if (this.dataSources.length > 1) {
      this.dataSources.splice(index, 1);
    }
    // Check if
    this.getMetrics();
  }

  setTimeRange(timeRange: TimeRange) {
    // Pushing into next event loop so that angular's change detection
    // doesn't complain.
    setTimeout(() => {
      if (this.selectedTimeRange == null || !timeRange.isSame(this.selectedTimeRange)) {
        this.selectedTimeRange = timeRange;
        this.getMetrics();
      }
    }, 0);
  }

  /**
   * This API build MetricsPollingQuery and let user customize MetricsPollingQuery
   * @param source
   */
  buildMetricsPollingQuery(source: DataSource): MetricsPollingQuery {
    const query = MetricsUtility.timeSeriesQueryPolling(source.measurement, source.fields);
    if (source.measurement === 'Cluster' || source.measurement === 'SessionSummaryMetrics') {  // measurement can be SessionSummaryMetrics, FteCPSMetrics, Cluster
      query.query.function = Telemetry_queryMetricsQuerySpec_function.last; // VS-741 use median function to show DSC count
    }
    return query;
  }

  generateMetricsQuery(): TelemetryPollingMetricQueries {
    const queryList: TelemetryPollingMetricQueries = {
      queries: [],
      tenant: Utility.getInstance().getTenant()
    };
    // Using variable so that this is not used inside updating timeSeriesQueryPolling
    // We may use an incorrect time instance if we do
    const timeRange = this.selectedTimeRange;
    this.dataSources.forEach((source) => {
      const query = source.generateQuery(timeRange);
      if (query != null) {
        queryList.queries.push(query);
      }
    });
    this.graphTransforms.forEach((t) => {
      t.transformQueries({
        queries: queryList
      });
    });
    return queryList;
  }

  areQueriesEqual(q1Input: TelemetryPollingMetricQueries, q2Input: TelemetryPollingMetricQueries): boolean {
    if (q1Input == null && q2Input == null) {
      return true;
    } else if (q1Input == null || q2Input == null) {
      return false;
    }
    if (q1Input.queries.length !== q2Input.queries.length) {
      return false;
    }
    const _ = Utility.getLodash();
    const queryUnequal = q1Input.queries.some((q1, index) => {
      const q2 = q2Input.queries[index];
      const q1Query = Utility.getLodash().cloneDeep(q1.query);
      const q2Query = _.cloneDeep(q2.query);
      // Replace time fields so they aren't checked
      q1Query['start-time'] = null;
      q1Query['end-time'] = null;
      q2Query['start-time'] = null;
      q2Query['end-time'] = null;
      return !_.isEqual(q1Query, q2Query);
    });
    if (queryUnequal) {
      return false;
    }
    return true;
  }

  isTimeRangeSame(t1: TimeRange, t2: TimeRange) {
    if (t1 == null && t2 == null) {
      return true;
    } else if (t1 == null || t2 == null) {
      return false;
    }
    return t1.isSame(t2);
  }

  getMetrics() {
    const queryList: TelemetryPollingMetricQueries = this.generateMetricsQuery();
    if (queryList.queries.length === 0) {
      this.metricData = null;
      this.lastQuery = queryList;
      this.drawGraph();
      return;
    }
    // If it's the same query we don't need to generate a new query
    if (this.areQueriesEqual(this.lastQuery, queryList) && this.isTimeRangeSame(this.selectedTimeRange, this.lastTimeRange)) {
      this.drawGraph();
      return;
    }

    if (this.metricSubscription) {
      // discarding previous subscription incase it isn't completed yet.
      this.metricSubscription.unsubscribe();
      this.metricSubscription = null;
    }
    this.graphLoading = true;
    this.lastQuery = queryList;
    this.lastTimeRange = this.selectedTimeRange;
    this.metricsQueryObserver.next(queryList);
  }

  metricsQueryListener() {
    // Buffers requests for 200ms before issuing a
    // metrics request. This way we limit multiple requests going out when the component is being initalized
    const subscription = this.metricsQueryObserver.pipe(debounceTime(200)).subscribe(
      (queryList) => {
        this.metricSubscription = this.telemetryqueryService.pollMetrics('telemetryExplore-' + Utility.s4() + Utility.s4(), queryList).subscribe(
          (data: ITelemetry_queryMetricsQueryResponse) => {
            // If tenant is null, then it is the default response from the behavior subject, and not from the backend
            if (data.tenant != null) {
              this.metricData = data;
              this.drawGraph();
            }
          },
          (err) => {
            // Err is already logged to a toaster by the polling service.
            // Drawing graph so that the graph is reset
            // and we turn off the spinner
            this.drawGraph();
          }
        );
        this.subscriptions.push(this.metricSubscription);
      }
    );
    this.subscriptions.push(subscription);
  }

  resetGraph() {
    this.lineData = { datasets: [] };
    this.graphOptions = this.generateDefaultGraphOptions();
  }

  getSourceMeta(s: Telemetry_queryMetricsResultSeries): SourceMeta {
    // s.tags looks like =>  {reporterID: "HP-srividhya-3.pensando.io"}
    return  (s.tags) ? s.tags as SourceMeta : {};
  }

   /**
   * Compute whether to auto deselect chart legend

    metricsResultSeries:
    columns:  ["time", "NumIcmpSessions"]
    name: "SessionSummaryMetrics_5minutes"

    tags: {reporterID: "HP-srividhya-3.pensando.io"}  - DSC
      or
          {name: "00ae.cd01.1dc0-uplink-1-2"} - networkinterface
    (tags is a SourceMeta)

    values: [ data ... ]

    const columns = s.columns;
    const name = s.name;
    */
  shouldLegendBeDeselected(metricsResultSeries: Telemetry_queryMetricsResultSeries, datasource: DataSource): boolean {
    const sourceMeta = this.getSourceMeta(metricsResultSeries);
    return this.shouldLegendBeDeselectedFromSourceMeta(sourceMeta, datasource);
  }

  /**
   * Learn from VS-1600, DSC/NI in existing chart may find associated DSCs are not in admitted state.
   * Thus, chart's data may be misleading. We use ChartJS's hidden property to auto deselect chart legend.
   * As well, we change chart legend label to inform US about DSC status.
   *
   * @param sourceMeta
   * @param datasource
   */
  shouldLegendBeDeselectedFromSourceMeta(sourceMeta: SourceMeta, datasource: DataSource): boolean {
    if (this.metricsMetadata[datasource.measurement].objectKind === 'DistributedServiceCard') {
      // Check on DSC
      const dscname = this.getDSCNameFromResultSeries(sourceMeta);
      if (dscname) {
        // If we find the DSC is not in admitted state, we return true --> the chart legend will be in hidden/de-select stage.
        return !this.isDSCExist( dscname) || !this.isDSCAdmittedByDSCName(dscname);
      }
    }  else if (this.metricsMetadata[datasource.measurement].objectKind === 'NetworkInterface') {
      // Check on NetworkNetworkInterface.
      const niName = this.getNetworkInterfaceNameFromResultSeries(sourceMeta);
      if (niName) {
        if (niName.indexOf(' (') > 0 && niName[niName.length - 1] === ')') {
          return true;
        }
        // Find network-interface object -> get dsc-name -> see if DSC object exists and is in admitted stage
        const networkInterface: NetworkNetworkInterface = this.networkInterfaces.find((ni: NetworkNetworkInterface) =>
            ni.meta.name === niName || this.interfaceNameMap[ni.meta.name] === niName);
        if (networkInterface) {
          const dscname = networkInterface.status['dsc-id'];
          const isDSCbad = !this.isDSCExist( dscname) || !this.isDSCAdmittedByDSCName(dscname); // return true if non-exist or is not-admitted
          const fieldNI = 'name';
          const dsc = this.naples.find( (nic: ClusterDistributedServiceCard) => nic.spec.id === dscname);
          // Update tags: { "name": "aaaa.bbbb.ccc-uplink-1-2 ( DSC-dsc_status"}
          sourceMeta[fieldNI] = (!isDSCbad) ? niName : niName +  ' (DSC-' + dsc.status['admission-phase'] + ')';
          return isDSCbad;
        } else {
          return true;
        }
      }
    }
    return false;
  }

  getDSCNameFromResultSeries(s: SourceMeta): string {
    return this.getItemNameFromResultSeries(s, 'reporterID');
  }

  getNetworkInterfaceNameFromResultSeries(s: SourceMeta): string {
    return this.getItemNameFromResultSeries(s, 'name');
  }

  getItemNameFromResultSeries(tags: SourceMeta, field: string): string {
    if (tags && tags.hasOwnProperty(field)) {
      return tags[field];
    }
    return null;
  }

  isDSCAdmittedByDSCName(dscname: string): boolean {
    const naple = this.naples.find(dsc => dsc.spec.id === dscname);
    return (naple  && naple.status['admission-phase'] === ClusterDistributedServiceCardStatus_admission_phase.admitted);
  }

  isDSCAdmittedByDSCMac(dscMac: string): boolean {
    const naple = this.naples.find(dsc => dsc.meta.name === dscMac);
    return (naple  && naple.status['admission-phase'] === ClusterDistributedServiceCardStatus_admission_phase.admitted);
  }

  isDSCExist(dscname: string): boolean {
    const naple = this.naples.find(dsc => (dsc.spec.id === dscname || dsc.meta.name === dscname) );
    return  (!!naple);
  }

  drawGraph() {
    if (this.metricData == null) {
      this.resetGraph();
      this.graphLoading = false;
      return;
    }

    if (!MetricsUtility.responseHasData(this.metricData)) {
      this.resetGraph();
      this.graphLoading = false;
      return;
    }

    // User can hide a dataset by clicking it's name on the legend
    // Create a map of which datasets are currently hidden, so that we can keep it.
    const hiddenDatasets = {};
    this.lineData.datasets.forEach((dataset, index) => {
      if (this.chartContainer.chart && !this.chartContainer.chart.isDatasetVisible(index)) {  // VS-745 (saw some console errror. add a check)
        const key = sourceFieldKey(dataset.sourceID, dataset.sourceMeasurement, dataset.sourceField) + dataset.label;
        hiddenDatasets[key] = true;
      }
    });

    const results = this.metricData.results;

    this.graphTransforms.forEach((t) => {
      t.transformMetricData({
        results: results
      });
    });

    // Start with a clean array so that we don't accidentally
    // have stale state transfer from the old options
    // All transforms should be relatively stateless
    const newGraphOptions: ChartOptions = this.generateDefaultGraphOptions();

    // Will be passed to graph transforms
    const allResultsDatasets = [];

    const resDataSets: ChartDataSets[] = [];
    results.forEach((res, index) => {
      if (!MetricsUtility.resultHasData(res)) {
        // TODO: add only in legend
        return;
      }
      // Should be one to one with response length
      const source = this.dataSources[index];

      source.transformMetricData({ result: res });

      const singleResultDatasets: TransformDatasets = [];
      res.series.forEach((s) => {
        source.fields.forEach((field) => {
          const fieldIndex = MetricsUtility.findFieldIndex(s.columns, field);
          const data = MetricsUtility.transformToChartjsTimeSeries(s, field);
          const fieldData = getFieldData(source.measurement, field);
          const dataset: ChartDataSets = {
            data: data,
            pointRadius: 2,
            lineTension: 0,
            fill: false,
            sourceID: source.id,
            sourceField: field,
            spanGaps: true,
            sourceMeasurement: source.measurement,
            hidden: this.shouldLegendBeDeselected(s as Telemetry_queryMetricsResultSeries, source ),  // VS-1600
            sourceMeta: this.getSourceMeta(s as Telemetry_queryMetricsResultSeries)
          };
          const opt: TransformDataset = {
            dataset: dataset,
            series: s,
            measurement: source.measurement,
            field: field,
            // We put unit in here. Transforms are allowed
            // to change the unit type (kb -> mb), and should update this unit
            // property if they do
            units: fieldData ? fieldData.units : null,
            fieldIndex: fieldIndex
          };
          source.transformDataset(opt);
          singleResultDatasets.push(opt);
          resDataSets.push(dataset);
        });
      });
      source.transformDatasets(singleResultDatasets);
      singleResultDatasets.forEach((opt) => {
        allResultsDatasets.push(opt);
      }
      );
    });

    this.graphTransforms.forEach((t) => {
      t.transformGraphOptions({
        data: allResultsDatasets,
        graphOptions: newGraphOptions,
        oldGraphOptions: this.graphOptions,
      });
    });

    // Hide any lines that are already hidden
    resDataSets.forEach((dataset, index) => {
      const key = sourceFieldKey(dataset.sourceID, dataset.sourceMeasurement, dataset.sourceField) + dataset.label;
      dataset.hidden = hiddenDatasets[key] != null ? hiddenDatasets[key] : dataset.hidden; // above line 804 - this.shouldLegendBeDeselected(). We want preserve "hidden" value
      dataset.steppedLine = (dataset.sourceMeasurement === 'Cluster') ? 'middle' : false; // VS-741 make cluster chart using stepped-line style
    });

    // modifications to legend after transforms
    if ((this.inEditMode && resDataSets.length > TelemetrychartComponent.MAX_LEGEND_EDIT_MODE) || (!this.inEditMode && resDataSets.length > TelemetrychartComponent.MAX_LEGEND_VIEW_MODE)) {
      newGraphOptions.legend.display = false;
    }

    this.graphOptions = newGraphOptions;

    this.lineData = {
      datasets: resDataSets
    };
    this.graphLoading = false;
  }

  ngOnDestroy() {
    this.subscriptions.forEach(subscription => {
      subscription.unsubscribe();
    });
    if (this.metricSubscription != null) {
      this.metricSubscription.unsubscribe();
    }
  }

}
