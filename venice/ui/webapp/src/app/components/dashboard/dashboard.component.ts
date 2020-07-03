import { Component, OnDestroy, OnInit, QueryList, ViewChildren, ViewEncapsulation } from '@angular/core';
import { Utility } from '@app/common/Utility';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { DashboardWidgetData, PinnedDashboardWidgetData } from '@app/models/frontend/dashboard/dashboard.interface';
import { BaseComponent } from '@components/base/base.component';

import { ControllerService } from '../../services/controller.service';
import { MetricsqueryService, TelemetryPollingMetricQueries, MetricsPollingQuery, MetricsPollingOptions } from '@app/services/metricsquery.service';
import { Telemetry_queryMetricsQuerySpec, ITelemetry_queryMetricsQuerySpec, Telemetry_queryMetricsQuerySpec_function, Telemetry_queryMetricsQuerySpec_sort_order, Telemetry_queryMetricsQueryResult } from '@sdk/v1/models/generated/telemetry_query';
import { MetricsUtility } from '@app/common/MetricsUtility';
import { ITelemetry_queryMetricsQueryResponse, ITelemetry_queryMetricsQueryResult } from '@sdk/v1/models/telemetry_query';
import { CardStates } from '../shared/basecard/basecard.component';
import { ClusterService } from '@app/services/generated/cluster.service';
import { ClusterDistributedServiceCard, ClusterHost } from '@sdk/v1/models/generated/cluster';
import { WorkloadWorkload } from '@sdk/v1/models/generated/workload';
import { WorkloadService } from '@app/services/generated/workload.service';
import { MonitoringAlert, MonitoringAlertStatus_severity } from '@sdk/v1/models/generated/monitoring';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { FwlogFwLogQuery, FwlogFwLogQuery_sort_order, FwlogFwLogList } from '@sdk/v1/models/generated/fwlog';
import { FwlogService } from '@app/services/generated/fwlog.service';
import { interval } from 'rxjs';


/**
 * This is Dashboard Component for VeniceUI
 * Dashboard.component.html has two gridster widgets and has multiple ng-template registered.  This component's widgets[] will map to ng-template by "id" field.
 *
 * We use gridster widget to enable grid layout and enable grid-item drag-and-drop
 * DashboardComponent.gridsterOptions defines the grid (2x6) matrix. Each cell is XXXpx height. css .dsbd-main-gridster-content defines the gridster height as YYY.
 * Thus, we must be careful to create widget which will be hosted in a grid-item in XXXpx height.
 *
 * We use canvas API to draw extra text in charts.  We are using chart.js charts as it offers better animation look & feel.  See "Naple and Workload" configurations.
 *
 * Other UI pages may pin their widgets to Dashboard. getPinnedData() is the API that loads those pinned-in widgets and add to Dashboard.
 * The UI pages that invoke "pin-to-dashboard" operations are responsible for provide widget invokation seeting.  See workload.component.ts for how it is done.
 *
 * Note:
 * To enable chart animation, we use gridster.itemInitCallback() api.  HTML template includes '*ngIf="item.dsbdGridsterItemIsReady" ' to manage timing.
 * Dashboard includes default widgets, such as "system capacity", "software version", etc.
 * Object in _buildSystemCapacity() is carefully wired to expected configuration of systemcapacity.component.ts
 *
 */

const MS_PER_MINUTE: number = 60000;

@Component({
  selector: 'app-dashboard',
  templateUrl: './dashboard.component.html',
  styleUrls: ['./dashboard.component.scss'],
  encapsulation: ViewEncapsulation.None
})
export class DashboardComponent extends BaseComponent implements OnInit, OnDestroy {
  subscriptions = [];
  alerts: ReadonlyArray<MonitoringAlert> = [];
  naples: ReadonlyArray<ClusterDistributedServiceCard> = [];
  workloads: ReadonlyArray<WorkloadWorkload> = [];
  hosts: ReadonlyArray<ClusterHost> = [];
  pifData: Telemetry_queryMetricsQueryResult = null;
  topDscsChartData: any = null;
  topDscsChartOption: any = null;
  gridsterOptions: any = {
    gridType: 'fixed',
    fixedRowHeight: 275,
    fixedColWidth: 275,
    compactType: 'compactUp&Left',
    margin: 5,
    draggable: {
      enabled: true,
    },
    itemInitCallback: this.gridsterItemInitCallback,
    displayGrid: 'none'
  };
  pinnedGridsterOptions: any = {};
  title = 'Venice UI Dashboard';

  widgets: DashboardWidgetData[];
  pinnedWidgets: PinnedDashboardWidgetData[];

  lastUpdateTime: string = '';
  MetricsPollingFrequency: number = MetricsUtility.FIVE_MINUTES;

  timeSeriesData: ITelemetry_queryMetricsQueryResult;
  currentData: ITelemetry_queryMetricsQueryResult;
  prevData: ITelemetry_queryMetricsQueryResult;
  avgDayData: ITelemetry_queryMetricsQueryResult;

  systemCapacity = {
    cardState: CardStates.LOADING
  };

  hideOldWidgets: boolean = false;

  naplesDisplayData = {
    title: 'Naples',
    lastUpdateTime: '',
    alerts: [0, 0, 0],
    rows: [
      {title: '', value: '', unit: 'TOTAL NAPLES'},
      {title: 'BANDWIDTH', value: '', unit: 'TX & RX'},
      {title: 'TOTAL PACKETS', value: '', unit: 'TX & RX'},
      {title: 'CONNECTION PER SECOND', value: '', unit: ''},
    ]
  };

  workloadsDisplayData = {
    title: 'Workloads',
    lastUpdateTime: '',
    alerts: [0, 0, 0],
    rows: [
      {title: '', value: '', unit: 'VCENTER / TOTAL WORKLOADS'},
      {title: 'BANDWIDTH', value: '', unit: 'TX & RX'},
      {title: 'DROP PACKETS', value: '', unit: 'TX & RX'},
      {title: 'VCENTER / TOTAL HOSTS', value: '', unit: ''},
    ]
  };

  servicesDisplayData = {
    title: 'Services',
    lastUpdateTime: '',
    rows: [
      {title: '', value: '', unit: 'TOTAL SERVICES'},
      {title: 'SESSIONS', value: '', unit: 'ACTIVE & DENIED'},
      {title: 'RULES', value: '', unit: 'ALLOW & DENY'},
      {title: 'SESSION PROTOCOL', value: '', unit: ''},
    ]
  };

  @ViewChildren('pinnedGridster') pinnedGridster: QueryList<any>;

  constructor(protected _controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected metricsqueryService: MetricsqueryService,
    protected monitoringService: MonitoringService,
    private workloadService: WorkloadService,
    protected clusterService: ClusterService,
    protected fwlogService: FwlogService,
  ) {
    super(_controllerService);
  }

  /**
   * Component enters init stage. It is about to show up
   */
  ngOnInit() {
    this.hideOldWidgets = this.uiconfigsService.isFeatureEnabled('showDashboardHiddenCharts');

    this._controllerService.publish(Eventtypes.COMPONENT_INIT, { 'component': 'DashboardComponent', 'state': Eventtypes.COMPONENT_INIT });
    this._controllerService.setToolbarData({
      buttons: [
        // Commenting out for now as they are not hooked up
        // {
        //   cssClass: 'global-button-primary dbsd-refresh-button',
        //   text: 'Help',
        //   callback: () => {
        //     console.log(this.getClassName() + 'toolbar button help call()');
        //   }
        // }
      ],
      splitbuttons: [
        // Commenting out for now as they are not hooked up
        // {
        //   text: 'Refresh',
        //   icon: 'pi pi-refresh',
        //   callback: (event, sbutton) => {
        //     console.log('dashboard toolbar splitbutton refresh');
        //   },
        //   items: [
        //     {
        //       label: '5 days', icon: 'pi pi-cog', command: () => {
        //         console.log('dashboard toolbar menuitem 5-days');
        //       }
        //     },
        //     {
        //       label: '10 days', icon: 'pi pi-times', command: () => {
        //         console.log('dashboard toolbar menuitem 10-days');
        //       }
        //     }
        //   ]
        // }
      ],
      breadcrumb: [{ label: 'Dashboard', url: Utility.getBaseUIUrl() + 'dashboard' }]
    });

    this.pinnedGridsterOptions = Object.assign({}, this.gridsterOptions);
    this.pinnedGridsterOptions.maxRows = 1;

    this.getDefaultDashboardWidgets();
    this.getPinnedData();

    this.getSystemCapacityMetrics();

    if (this.hideOldWidgets) {
      this.getAlertsWatch();
      this.getRecords();
      this.startInterfaceStatsPoll();

      const source = interval(1000 * 1 * 60);
      const sub = source.subscribe( (val) => {this.getFWLogs(); });
      this.subscriptions.push(sub);
      this.getFWLogs();
    }
  }

  getFWLogs() {
    const query = new FwlogFwLogQuery(null, false);
    query['max-results'] = 8192;
    const endTime = new Date(Date.now());
    const startTime = new Date(Date.now() - MS_PER_MINUTE * 60 * 24);

    query['start-time'] = startTime.toISOString() as any;
    query['end-time'] = endTime.toISOString() as any;
    query['sort-order'] = FwlogFwLogQuery_sort_order.descending;
    this.fwlogQueryListener(query);
  }

  fwlogQueryListener(query) {
    const searchSubscription = this.fwlogService.PostGetLogs(query).subscribe(  //  use POST
      (resp) => {
        const body = resp.body as FwlogFwLogList;
        let logs = null;
        if (body.items !== undefined && body.items !== null) {
          logs = body.items;
        }
        if (logs != null) {
          this.processLogs(logs);
        }
      },
      (error) => {
      }
    );
    this.subscriptions.push(searchSubscription);
  }

  processLogs(logs) {
    const sessions = {'allow': {}, 'deny': {}};
    const destinations = {};
    const rules = {'allow': {}, 'deny': {}};
    const protocol = {};
    for (const l of logs) {

      if (!sessions['allow'][l['session-id']] && !sessions['deny'][l['session-id']]) {
        if (!protocol[l.protocol]) {
          protocol[l.protocol] = 1;
        } else {
          protocol[l.protocol] += 1;
        }
      }

      if (l.action === 'allow') {
        sessions['allow'][l['session-id']] = true;
      } else {
        sessions['deny'][l['session-id']] = true;
      }

      if (l['destination-ip'] && l['destination-port']) {
        destinations[l['destination-ip'] + ':' + l['destination-port'].toString()] = true;
      } else {
        destinations[l['app-id']] = true;
      }

      if (l.action === 'allow') {
        rules['allow'][l['rule-id']] = true;
      } else {
        rules['deny'][l['rule-id']] = true;
      }
    }
    this.servicesDisplayData.lastUpdateTime = new Date(Date.now()).toString();
    this.servicesDisplayData.rows[0].value = '' + Object.keys(destinations).length;
    this.servicesDisplayData.rows[1].value = '' + Object.keys(sessions['allow']).length + ' & ' + Object.keys(sessions['deny']).length;
    this.servicesDisplayData.rows[2].value = '' + Object.keys(rules['allow']).length + ' & ' + Object.keys(rules['deny']).length;
    this.generateProtocolText(protocol);
  }

  generateProtocolText(protocolMap: {[key: string]: number} ) {
    if (Object.keys(protocolMap).length === 1) {
      const k = Object.keys(protocolMap)[0];
      const protocolStr = protocolMap[k] + ' ' + k;
      this.servicesDisplayData.rows[3].value = protocolStr;
    } else if (Object.keys(protocolMap).length >= 2) {
      const protocolArr = [];
      for (const k of Object.keys(protocolMap)) {
        protocolArr.push(protocolMap[k] + ' ' + k);
      }
      const last = protocolArr.pop();
      const protocolStr = protocolArr.join(', ') + ' & ' + last;
      this.servicesDisplayData.rows[3].value = protocolStr;
    }
    this.servicesDisplayData = { ...this.servicesDisplayData };
  }

  getAlertsWatch() {
    const alertWatchSubscription = this.monitoringService.ListAlertCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        const myAlerts = response.data as MonitoringAlert[];
        this.alerts = myAlerts.filter((alert: MonitoringAlert) => {
          return (this.isAlertInOpenState(alert));
        });

        let dscCritical: number = 0;
        let dscWarning: number = 0;
        let dscInfo: number = 0;
        let workloadCritical: number = 0;
        let workloadWarning: number = 0;
        let workloadInfo: number = 0;

        if (this.alerts) {
          this.alerts.forEach(alert => {
            if (alert && alert.status && alert.status['object-ref']) {
              if (alert.status['object-ref'].kind === 'DistributedServiceCard') {
                if (alert.status.severity === MonitoringAlertStatus_severity.critical) {
                  dscCritical += 1;
                } else if (alert.status.severity === MonitoringAlertStatus_severity.warn) {
                  dscWarning += 1;
                } else if (alert.status.severity === MonitoringAlertStatus_severity.info) {
                  dscInfo += 1;
                }
              } else if (alert.status['object-ref'].kind === 'Workload') {
                if (alert.status.severity === MonitoringAlertStatus_severity.critical) {
                  workloadCritical += 1;
                } else if (alert.status.severity === MonitoringAlertStatus_severity.warn) {
                  workloadWarning += 1;
                } else if (alert.status.severity === MonitoringAlertStatus_severity.info) {
                  workloadInfo += 1;
                }
              }
            }
          });
        }
        this.naplesDisplayData.alerts = [dscCritical, dscWarning, dscInfo];
        this.naplesDisplayData = { ...this.naplesDisplayData };
        this.workloadsDisplayData.alerts = [workloadCritical, workloadWarning, workloadInfo];
        this.workloadsDisplayData = { ...this.workloadsDisplayData };
      },
      this._controllerService.webSocketErrorHandler('Failed to get Alerts'),
    );
    this.subscriptions.push(alertWatchSubscription);
  }

  isAlertInOpenState(alert: MonitoringAlert): boolean {
    return (alert.spec.state === 'open');
  }

  getRecords() {
    const subscription = this.clusterService.ListDistributedServiceCardCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        this.naples = response.data;
        this.naplesDisplayData.rows[0].value = '' + this.naples.length;
        this.naplesDisplayData = { ...this.naplesDisplayData };
      },
    );
    this.subscriptions.push(subscription);
    const workloadSubscription = this.workloadService.ListWorkloadCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.workloads = response.data as WorkloadWorkload[];
        let vcenterWorkloads: number = 0;
        if (this.workloads) {
          this.workloads.forEach(workload => {
            if (Utility.isWorkloadSystemGenerated(workload)) {
              vcenterWorkloads += 1;
            }
          });
        }
        this.workloadsDisplayData.rows[0].value = vcenterWorkloads + ' / ' + this.workloads.length;
        this.workloadsDisplayData = { ...this.workloadsDisplayData };
      }
    );
    this.subscriptions.push(workloadSubscription);
    const hostSubscription = this.clusterService.ListHostCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.hosts = response.data;
        this.getTop5HostBandwidth();
        let vcenterHosts: number = 0;
        if (this.hosts) {
          this.hosts.forEach(host => {
            if (host && host.meta.labels &&
                host.meta.labels['io.pensando.vcenter.display-name']) {
              vcenterHosts += 1;
            }
          });
        }
        this.workloadsDisplayData.rows[3].value = vcenterHosts + ' / ' + this.hosts.length;
        this.workloadsDisplayData = { ...this.workloadsDisplayData };
      }
    );
    this.subscriptions.push(hostSubscription);
  }

  updateInterfacesStats(results) {
    const lifData = results[0];
    const pifData = results[1];
    const cpsData = results[2];
    let lastUpdateTime: string = null;
    if (lifData && lifData.series && lifData.series.length > 0) {
      lastUpdateTime = lifData.series[0].values[0][0];
      let totalRxBandWidth: number = 0;
      let totalTxBandWidth: number = 0;
      let totalRxDrop: number = 0;
      let totalTxDrop: number = 0;
      lifData.series.forEach(item => {
        totalRxBandWidth += item.values[0][17];
        totalTxBandWidth += item.values[0][18];
        const newRxDrop = item.values[0][1] + item.values[0][2] + item.values[0][3];
        totalRxDrop += newRxDrop;
        const newTxDrop = item.values[0][4] + item.values[0][5] + item.values[0][6];
        totalTxDrop += newTxDrop;
      });
      this.workloadsDisplayData.rows[1].value = Utility.formatBytes(totalTxBandWidth, 2)
          + '/s & ' + Utility.formatBytes(totalRxBandWidth) + '/s';
      this.workloadsDisplayData.rows[2].value = Utility.formatBytes(totalTxDrop, 2)
          + ' & ' + Utility.formatBytes(totalRxDrop);
    }
    if (pifData && pifData.series && pifData.series.length > 0) {
      if (!lastUpdateTime) {
        lastUpdateTime = pifData.series[0].values[0][0];
      }
      let totalRxBandWidth: number = 0;
      let totalTxBandWidth: number = 0;
      let totalRx: number = 0;
      let totalTx: number = 0;
      pifData.series.forEach(item => {
        totalRxBandWidth += item.values[0][5];
        totalTxBandWidth += item.values[0][6];
        totalRx += item.values[0][1];
        totalTx += item.values[0][2];
      });
      this.naplesDisplayData.rows[1].value = Utility.formatBytes(totalTxBandWidth, 2)
          + '/s & ' + Utility.formatBytes(totalRxBandWidth) + '/s';
      this.naplesDisplayData.rows[2].value = Utility.formatBytes(totalTx, 2)
          + ' & ' + Utility.formatBytes(totalRx);
      this.pifData = pifData;
      this.getTop5HostBandwidth();
    }

    if (cpsData && cpsData.series && cpsData.series.length > 0) {
      if (!lastUpdateTime) {
        lastUpdateTime = cpsData.series[0].values[0][0];
      }
      let totalCpsCount = 0;
      cpsData.series.forEach(item => {
        totalCpsCount += item.values[0][1];
      });
      this.naplesDisplayData.rows[3].value = totalCpsCount.toString();
    }

    this.naplesDisplayData = { ...this.naplesDisplayData, lastUpdateTime };
    this.workloadsDisplayData = { ...this.workloadsDisplayData, lastUpdateTime };
  }

  getTop5HostBandwidth() {
    if (this.hosts && this.pifData) {
      this.hosts.forEach(host => {
        host._ui.bandwidth = {
          rx: 0,
          tx: 0
        };
      });
      this.pifData.series.forEach(item => {
        if (item && item.tags && item.values && item.values.length > 0) {
          const tags: any = item.tags;
          if (tags.name) {
            const dscMac = tags.name.split('-')[0];
            const targethost: ClusterHost = this.hosts.find(host => {
              if (host.spec && host.spec.dscs && host.spec.dscs.length > 0) {
                for (let i = 0; i < host.spec.dscs.length; i++) {
                  const dsc = host.spec.dscs[i];
                  if (dsc['mac-address'] === dscMac) {
                    return true;
                  }
                }
              }
              return false;
            });
            if (targethost) {
              targethost._ui.bandwidth.rx += item.values[0][5];
              targethost._ui.bandwidth.tx += item.values[0][6];
            }
          }
        }
      });
      (this.hosts as ClusterHost[]).sort((a, b) =>
        a._ui.bandwidth.rx + a._ui.bandwidth.tx >
        b._ui.bandwidth.rx + b._ui.bandwidth.tx ? -1 : 1);
      const topDscs = {rxs: [], txs: [], names: []};
      for (let i = 0; i < this.hosts.length && i < 5; i++) {
        if (this.hosts[i]._ui.bandwidth.rx > 0 &&
            this.hosts[i]._ui.bandwidth.tx > 0) {
          topDscs.rxs.push(this.hosts[i]._ui.bandwidth.rx);
          topDscs.txs.push(this.hosts[i]._ui.bandwidth.tx);
          topDscs.names.push(this.hosts[i].meta.name);
        }
      }
      this.topDscsChartOption = {
        responsive: true,
        scales: {
          xAxes: [{
            stacked: true,
            gridLines: { display: false },
          }],
          yAxes: [{
            stacked: true,
            ticks: {
              callback: function(value) { return Utility.formatBytes(value) + ' /s'; },
             },
          }],
        },
        legend: {
          display: true,
        },
        tooltips: {
          callbacks: {
            intersect: false,
            label: (tooltipItem, data) => {
              const rxValue = this.topDscsChartData.datasets[0].data[tooltipItem.index];
              const txValue = this.topDscsChartData.datasets[1].data[tooltipItem.index];
              return  ['RX: ' + Utility.formatBytes(rxValue) + ' /s',
                      'TX: ' + Utility.formatBytes(txValue) + ' /s',
                      'Total: ' + Utility.formatBytes(rxValue + txValue) + ' /s'];
            }
          }
        }
      };
      this.topDscsChartData = {
        labels: topDscs.names,
        datasets: [
          {
            label: 'RX',
            backgroundColor: 'rgba(165,236,201, 1)',
            data: topDscs.rxs
          },
          {
            label: 'TX',
            backgroundColor: 'rgba(200,234,165, 1)',
            data: topDscs.txs
          }
        ]
      };
    }
  }

  startInterfaceStatsPoll() {
    const queryList: TelemetryPollingMetricQueries = {
        queries: [],
        tenant: Utility.getInstance().getTenant()
      };

    const query: MetricsPollingQuery = this.topologyInterfaceQuery(
      'LifMetrics', ['RxDropBroadcastBytes', 'RxDropMulticastBytes', 'RxDropUnicastBytes',
      'TxDropBroadcastBytes', 'TxDropMulticastBytes', 'TxDropUnicastBytes', 'RxBytes',
      'TxBytes', 'RxDropBroadcastPackets', 'RxDropMulticastPackets', 'RxDropUnicastPackets',
      'TxDropBroadcastPackets', 'TxDropMulticastPackets', 'TxDropUnicastPackets',
      'RxPkts', 'TxPkts', 'RxBytesps', 'TxBytesps', 'RxPps', 'TxPps']);
    queryList.queries.push(query);

    const query2: MetricsPollingQuery = this.topologyInterfaceQuery(
      'MacMetrics', ['OctetsRxOk', 'OctetsTxOk', 'FramesRxOk', 'FramesTxOk',
      'RxBytesps', 'TxBytesps', 'RxPps', 'TxPps']);
    queryList.queries.push(query2);

    const query3: MetricsPollingQuery = this.cpsQuery(
      'FteCPSMetrics', ['ConnectionsPerSecond']);
    queryList.queries.push(query3);

    const sub = this.metricsqueryService.pollMetrics('topologyInterfaces', queryList, this.MetricsPollingFrequency).subscribe(
      (data: ITelemetry_queryMetricsQueryResponse) => {
        if (data && data.results && data.results.length === queryList.queries.length) {
          this.updateInterfacesStats(data.results);
        }
      },
      (err) => {
        this._controllerService.invokeErrorToaster('Error', 'Failed to load interface metrics.');
      }
    );
    this.subscriptions.push(sub);
  }

  topologyInterfaceQuery(kind: string, fields: string[]): MetricsPollingQuery {
    const query: ITelemetry_queryMetricsQuerySpec = {
      'kind': kind,
      'name': null,
      'selector': null,
      'function': Telemetry_queryMetricsQuerySpec_function.last,
      'group-by-field': 'name',
      'group-by-time': null,
      'fields': fields != null ? fields : [],
      'sort-order': Telemetry_queryMetricsQuerySpec_sort_order.descending,
      'start-time': 'now() - 1m' as any,
      'end-time': 'now()' as any,
    };
    return { query: new Telemetry_queryMetricsQuerySpec(query), pollingOptions: {} };
  }

  cpsQuery(kind: string, fields: string[]): MetricsPollingQuery {
    const query: ITelemetry_queryMetricsQuerySpec = {
      'kind': kind,
      'name': null,
      'selector': null,
      'function': Telemetry_queryMetricsQuerySpec_function.last,
      'group-by-field': 'reporterID',
      'group-by-time': null,
      'fields': fields != null ? fields : [],
      'sort-order': Telemetry_queryMetricsQuerySpec_sort_order.descending,
      'start-time': 'now() - 1m' as any,
      'end-time': 'now()' as any,
    };
    return { query: new Telemetry_queryMetricsQuerySpec(query), pollingOptions: {} };
  }

  /**
   * Used for alerting the item content when its surrounding gridster
   * is ready. Allows animations to happen after the widget is viewable.
   */
  gridsterItemInitCallback(gridsterItem, gridsterItemComponent) {
    gridsterItem.dsbdGridsterItemIsReady = true;
  }

  /**
   * Component is about to exit
   */
  ngOnDestroy() {
    // publish event that AppComponent is about to exist
    this._controllerService.publish(Eventtypes.COMPONENT_DESTROY, { 'component': 'dashboardComponent', 'state': Eventtypes.COMPONENT_DESTROY });
    this.subscriptions.forEach(subscription => {
      subscription.unsubscribe();
    });
  }

  timeSeriesQuery(): MetricsPollingQuery {
    // TODO: Optimize query. Don't pull all stats
    return MetricsUtility.timeSeriesQueryPolling('Node', []);
  }

  currentQuery(): MetricsPollingQuery {
    const query: Telemetry_queryMetricsQuerySpec = MetricsUtility.pastFiveMinAverageQuery('Node');
    const pollOptions: MetricsPollingOptions = {
      timeUpdater: MetricsUtility.pastFiveMinQueryUpdate,
    };

    return { query: query, pollingOptions: pollOptions };
  }

  // Gets between 10 to 5 min ago
  prevQuery(): MetricsPollingQuery {
    const start = 'now() - 10m';
    const end = 'now() - 5m';
    const query: Telemetry_queryMetricsQuerySpec = MetricsUtility.intervalAverageQuery('Node', start, end);
    const pollOptions: MetricsPollingOptions = {
      timeUpdater: MetricsUtility.genIntervalAverageQueryUpdate(start, end),
    };

    return { query: query, pollingOptions: pollOptions };
  }

  avgDayQuery(): MetricsPollingQuery {
    const query: Telemetry_queryMetricsQuerySpec = MetricsUtility.pastDayAverageQuery('Node');
    const pollOptions: MetricsPollingOptions = {
      timeUpdater: MetricsUtility.pastDayAverageQueryUpdate,
    };
    return { query: query, pollingOptions: pollOptions };
  }

  getSystemCapacityMetrics() {
    const queryList: TelemetryPollingMetricQueries = {
      queries: [],
      tenant: Utility.getInstance().getTenant()
    };
    queryList.queries.push(this.timeSeriesQuery());
    queryList.queries.push(this.currentQuery());
    queryList.queries.push(this.prevQuery());
    queryList.queries.push(this.avgDayQuery());
    const sub = this.metricsqueryService.pollMetrics('dsbdCards', queryList).subscribe(
      (data: ITelemetry_queryMetricsQueryResponse) => {
        if (data && data.results && data.results.length === queryList.queries.length) {
          if (MetricsUtility.resultHasData(data.results[1])) {
            this.timeSeriesData = data.results[0];
            this.currentData = data.results[1];
            this.prevData = data.results[2];
            this.avgDayData = data.results[3];
            this.lastUpdateTime = new Date().toISOString();
            this.enableSystemCapacityCard();
          } else {
            this.enableSystemCapacityNoData();
          }
        }
      },
      (err) => {
        this.setSystemCapacityErrorState();
      }
    );
    this.subscriptions.push(sub);
  }

  enableSystemCapacityCard() {
    this.systemCapacity.cardState = CardStates.READY;
  }

  setSystemCapacityErrorState() {
    this.systemCapacity.cardState = CardStates.FAILED;
  }

  enableSystemCapacityNoData() {
    this.systemCapacity.cardState = CardStates.NO_DATA;
  }

  /**
   * Overide super's API
   * It will return this Component name
   */
  getClassName(): string {
    return this.constructor.name;
  }

  getPinnedData() {
    this.pinnedWidgets = [];
    const $ = Utility.getJQuery();
    const items = this._controllerService.getPinnedDashboardItems();

    items.forEach(elem => {
      this._controllerService.buildComponentFromModule(elem.module,
        elem.component)
        .then((component: any) => {
          // change component properties and append component to UI view
          for (const key of Object.keys(elem.args)) {
            component.instance[key] = elem.args[key];
          }
          component.instance.dashboardSetup();
          const settingsWidget = component.instance.getDashboardSettings();
          settingsWidget.id = 'gridsterItem' + component.instance.id;
          settingsWidget.initCallback = () => {
            this._controllerService.appendComponentToDOMElement(component, $('#' + settingsWidget.id).get(0));
          };
          this.pinnedWidgets.push(settingsWidget);
        });
    });
  }

  private _buildSoftwareVersion(): DashboardWidgetData {
    return {
      id: 'software_version',
      x: 4, y: 0,
      // w: 2, h: 1,
      rows: 1, cols: 1,
      dsbdGridsterItemIsReady: false,
    };
  }

  private _buildSystemCapacity(): DashboardWidgetData {
    const ret = {
      x: 0, y: 0,
      rows: 1, cols: 2,
      dsbdGridsterItemIsReady: false,
      id: 'system_capacity',
    };
    return ret;
  }

  private _buildPoliciesHealth(): DashboardWidgetData {
    return {
      x: 3, y: 1,
      rows: 1, cols: 2,
      dsbdGridsterItemIsReady: false,
      id: 'policy_health',
    };
  }

  private _buildWorkloads(): DashboardWidgetData {
    return {
      x: 2, y: 0,
      rows: 1, cols: 2,
      dsbdGridsterItemIsReady: false,
      id: 'workloads',
    };
  }

  private _buildNaples(): DashboardWidgetData {
    return {
      x: 1, y: 0,
      rows: 1, cols: 3,
      dsbdGridsterItemIsReady: false,
      id: 'naples',
    };
  }

  protected getDefaultDashboardWidgets() {
    if (!this.widgets) {
      this.widgets = [];
    } else {
      this.widgets.length = 0;
    }
    this.widgets.push(this._buildSystemCapacity());
    this.widgets.push(this._buildWorkloads());
    this.widgets.push(this._buildNaples());
    this.widgets.push(this._buildPoliciesHealth());
    this.widgets.push(this._buildSoftwareVersion());
  }

  setWidth(widget: any, size: number, e: MouseEvent) {
    e.stopPropagation();
    e.preventDefault();
    if (size < 1) {
      size = 1;
    }
    widget.w = size;

    return false;
  }

  setHeight(widget: any, size: number, e: MouseEvent) {
    e.stopPropagation();
    e.preventDefault();
    if (size < 1) {
      size = 1;
    }
    widget.h = size;
    return false;
  }



}
