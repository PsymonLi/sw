import { ITelemetry_queryMetricsQuerySpec, Telemetry_queryMetricsQuerySpec_function, Telemetry_queryMetricsQuerySpec, FieldsRequirement_operator, IFieldsSelector, Telemetry_queryMetricsQuerySpec_sort_order } from '@sdk/v1/models/generated/telemetry_query';
import { Utility } from './Utility';
import { ITelemetry_queryMetricsQueryResponse, ITelemetry_queryMetricsQueryResult, ITelemetry_queryMetricsResultSeries } from '@sdk/v1/models/telemetry_query';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { HeroCardOptions } from '@app/components/shared/herocard/herocard.component';
import { CardStates, StatArrowDirection } from '@app/components/shared/basecard/basecard.component';
import { IClusterSmartNIC } from '@sdk/v1/models/generated/cluster';
import { MetricsPollingOptions, MetricsPollingQuery } from '@app/services/metricsquery.service';

/**
 * serverData is in the following form:
 * "results" [
 *   "series": [
        {
          "name": "Node",
          "columns": [
            "time",
            "CPUUsedPercent",
            "Kind",
            "MemAvailable",
            "MemFree",
            "MemTotal",
            "MemUsed",
            "MemUsedPercent",
            "Name",
            "reporterID"
          ],
          "values": [
            [
              "2018-10-04T01:40:16.71348949Z",
              72.9289625253088,
              "Node",
              13900562432,
              1176416256,
              16826892288,
              2942144512,
              17.484776521082107,
              "node1",
              "Node-node1"
            ],
            [
              "2018-10-04T01:42:52.780420968Z",
              69.15385763317215,
              "Node",
              12755824640,
              1896194048,
              16826892288,
              4062240768,
              24.141360736569066,
              "node2",
              "Node-node2"
            ],
            ...
  *
  * If it is grouped by a field then it is of the following form:
    "results": [
        {
            "statement_id": 0,
            "series": [
                {
                    "name": "Node",
                    "tags": { // Grouped field is here
                        "reporterID": "Node-node1"
                    },
                    "columns": [
                        "time",
                        ...
                    ],
                    "values": [
                        [
                            "2018-10-31T21:24:57.000000001Z",
                            ...
                        ]
                    ]
                }
            ]
        },
        {
            "statement_id": 0,
            "series": [
                {
                    "name": "Node",
                    "tags": {
                        "reporterID": "Node-node2"
                    },
                    "columns": [
                        "time",
                        ...
                    ],
                    "values": [
                        [
                            "2018-10-31T21:24:57.000000001Z",
                            ...
                        ]
                    ]
                }
            ]
        }
      ]
    }
  */
export class MetricsUtility {

  public static getStatArrowDirection(prev: number, curr: number): StatArrowDirection {
    if (prev < curr) {
      return StatArrowDirection.UP;
    } else if (prev > curr) {
      return StatArrowDirection.DOWN;
    } else {
      return StatArrowDirection.HIDDEN;
    }
  }

  public static timeSeriesQuery(kind: string, selector: IFieldsSelector = null): Telemetry_queryMetricsQuerySpec {
    const timeSeriesQuery: ITelemetry_queryMetricsQuerySpec = {
      'kind': kind,
      'name': null,
      'selector': selector,
      function: Telemetry_queryMetricsQuerySpec_function.MEAN,
      'group-by-time': '5m',
      // We don't specify the fields we need, as specifying more than one field
      // while using the average function isn't supported by the backend.
      // Instead we leave blank and get all fields
      fields: [],
      'sort-order': Telemetry_queryMetricsQuerySpec_sort_order.Ascending,
      'start-time': 'now() - 24h' as any,
      // Round down so we don't pick up an incomplete bucket
      'end-time': Utility.roundDownTime(5).toISOString() as any,
    };

    return new Telemetry_queryMetricsQuerySpec(timeSeriesQuery);
  }

  public static timeSeriesQueryPolling(kind: string, selector: IFieldsSelector = null): MetricsPollingQuery {
    const pollOptions: MetricsPollingOptions = {
      timeUpdater: MetricsUtility.timeSeriesQueryUpdate,
      mergeFunction: MetricsUtility.timeSeriesQueryMerge
    };
    return { query: MetricsUtility.timeSeriesQuery(kind, selector), pollingOptions: pollOptions };
  }

  // Since we are averaging over 5 min buckets, we always query from the last 5 min window increment
  public static timeSeriesQueryUpdate(queryBody: ITelemetry_queryMetricsQuerySpec) {
    queryBody['start-time'] = queryBody['end-time'];
    queryBody['end-time'] = Utility.roundDownTime(5).toISOString() as any;
  }

  public static timeSeriesQueryMerge(currData: ITelemetry_queryMetricsQueryResult, newData: ITelemetry_queryMetricsQueryResult) {
    // Drops any values that are older than 24 hours from the current time.
    // We then add on the newer values.
    const _ = Utility.getLodash();
    const window = 24 * 60;
    if (MetricsUtility.resultHasData(currData)) {
      const moment = Utility.getMomentJS();
      const windowStart = moment().subtract(window, 'minutes');
      const filteredValues = _.dropWhile(currData.series[0].values, (item) => {
        // Assuming time is the first index.
        return windowStart.diff(moment(item[0]), 'minutes') > 0;
      });
      currData.series[0].values = _.cloneDeep(filteredValues);
    }

    // Checking if there is new data
    if (MetricsUtility.resultHasData(newData)) {
      const data = newData.series[0].values;
      if (MetricsUtility.resultHasData(currData)) {
        // Push on the new data
        currData.series[0].values.push(...data);
      } else {
        return newData;
      }
    }
    return currData;
  }

  public static currentFiveMinQuery(kind: string, selector: IFieldsSelector = null): Telemetry_queryMetricsQuerySpec {
    const query: ITelemetry_queryMetricsQuerySpec = {
      'kind': kind,
      'name': null,
      'selector': selector,
      function: Telemetry_queryMetricsQuerySpec_function.MEAN,
      'sort-order': Telemetry_queryMetricsQuerySpec_sort_order.Ascending,
      // We don't specify the fields we need, as specifying more than one field
      // while using the average function isn't supported by the backend.
      // Instead we leave blank and get all fields
      fields: [],
      'start-time': Utility.roundDownTime(5).toISOString() as any,
      'end-time': 'now()' as any,
    };
    return new Telemetry_queryMetricsQuerySpec(query);
  }

  public static currentFiveMinPolling(kind: string, selector: IFieldsSelector = null): MetricsPollingQuery {
    const pollOptions: MetricsPollingOptions = {
      timeUpdater: MetricsUtility.currentFiveMinQueryUpdate,
    };
    return { query: MetricsUtility.currentFiveMinQuery(kind, selector), pollingOptions: pollOptions };
  }

  public static currentFiveMinQueryUpdate(queryBody: ITelemetry_queryMetricsQuerySpec) {
    queryBody['start-time'] = Utility.roundDownTime(5).toISOString() as any,
    queryBody['end-time'] = 'now()' as any;
  }

  public static pastFiveMinAverageQuery(kind: string, selector: IFieldsSelector = null): Telemetry_queryMetricsQuerySpec {
    const avgQuery: ITelemetry_queryMetricsQuerySpec = {
      'kind': kind,
      'name': null,
      'selector': selector,
      function: Telemetry_queryMetricsQuerySpec_function.MEAN,
      'sort-order': Telemetry_queryMetricsQuerySpec_sort_order.Ascending,
      // We don't specify the fields we need, as specifying more than one field
      // while using the average function isn't supported by the backend.
      // Instead we leave blank and get all fields
      fields: [],
      'start-time': 'now() - 5m' as any,
      'end-time': 'now()' as any,
    };

    return new Telemetry_queryMetricsQuerySpec(avgQuery);
  }

  public static pastFiveMinAverageQueryPolling(kind: string, selector: IFieldsSelector = null): MetricsPollingQuery {
    const pollOptions: MetricsPollingOptions = {
      timeUpdater: MetricsUtility.pastFiveMinQueryUpdate,
    };
    return { query: MetricsUtility.pastFiveMinAverageQuery(kind, selector), pollingOptions: pollOptions };
  }

  public static pastFiveMinQueryUpdate(queryBody: ITelemetry_queryMetricsQuerySpec) {
    queryBody['start-time'] = 'now() - 5m' as any;
    queryBody['end-time'] = 'now()' as any;
  }

  public static intervalAverageQuery(kind: string, startTime, endTime, selector: IFieldsSelector = null): Telemetry_queryMetricsQuerySpec {
    const avgQuery: ITelemetry_queryMetricsQuerySpec = {
      'kind': kind,
      'name': null,
      'selector': selector,
      function: Telemetry_queryMetricsQuerySpec_function.MEAN,
      'sort-order': Telemetry_queryMetricsQuerySpec_sort_order.Ascending,
      // We don't specify the fields we need, as specifying more than one field
      // while using the average function isn't supported by the backend.
      // Instead we leave blank and get all fields
      fields: [],
      'start-time': startTime,
      'end-time': endTime
    };

    return new Telemetry_queryMetricsQuerySpec(avgQuery);
  }

  public static genIntervalAverageQueryUpdate(startTime, endTime) {
    return (queryBody: ITelemetry_queryMetricsQuerySpec) => {
      queryBody['start-time'] = startTime;
      queryBody['end-time'] = endTime;
    };
  }


  public static pastDayAverageQuery(kind: string, selector: IFieldsSelector = null): Telemetry_queryMetricsQuerySpec {
    const avgQuery: ITelemetry_queryMetricsQuerySpec = {
      'kind': kind,
      'name': null,
      'selector': selector,
      function: Telemetry_queryMetricsQuerySpec_function.MEAN,
      'sort-order': Telemetry_queryMetricsQuerySpec_sort_order.Ascending,
      // We don't specify the fields we need, as specifying more than one field
      // while using the average function isn't supported by the backend.
      // Instead we leave blank and get all fields
      fields: [],
      'start-time': 'now() - 24h' as any,
      'end-time': 'now()' as any,
    };

    return new Telemetry_queryMetricsQuerySpec(avgQuery);
  }

  public static pastDayAverageQueryPolling(kind: string, selector: IFieldsSelector = null): MetricsPollingQuery {
    const query: Telemetry_queryMetricsQuerySpec = MetricsUtility.pastDayAverageQuery(kind, selector);
    const pollOptions: MetricsPollingOptions = {
      timeUpdater: MetricsUtility.pastDayAverageQueryUpdate,
    };

    return { query: query, pollingOptions: pollOptions };
  }

  public static pastDayAverageQueryUpdate(queryBody: ITelemetry_queryMetricsQuerySpec) {
    queryBody['start-time'] = 'now() - 24h' as any;
    queryBody['end-time'] = 'now()' as any;
  }

  public static maxObjQuery(kind: string, selector: IFieldsSelector = null): Telemetry_queryMetricsQuerySpec {
    const maxNodeQuery: ITelemetry_queryMetricsQuerySpec = {
      'kind': kind,
      'name': null,
      'selector': selector,
      function: Telemetry_queryMetricsQuerySpec_function.MEAN,
      'sort-order': Telemetry_queryMetricsQuerySpec_sort_order.Ascending,
      // We don't specify the fields we need, as specifying more than one field
      // while using the average function isn't supported by the backend.
      // Instead we leave blank and get all fields
      fields: [],
      'group-by-field': 'reporterID',
      // We only look at the last 5 min bucket so that the max node reporting is never
      // lower than current
      'start-time': new Date(Utility.roundDownTime(5).getTime() - 1000 * 50 * 5).toISOString() as any,
      'end-time': Utility.roundDownTime(5).toISOString() as any
    };
    return new Telemetry_queryMetricsQuerySpec(maxNodeQuery);
  }

  public static maxObjQueryPolling(kind: string, selector: IFieldsSelector = null): MetricsPollingQuery {
    const query: Telemetry_queryMetricsQuerySpec = MetricsUtility.maxObjQuery(kind, selector);
    const pollOptions: MetricsPollingOptions = {
      timeUpdater: MetricsUtility.maxObjQueryUpdate,
      mergeFunction: MetricsUtility.maxObjQueryMerge
    };

    return { query: query, pollingOptions: pollOptions };
  }

  public static maxObjQueryUpdate(queryBody: ITelemetry_queryMetricsQuerySpec) {
    queryBody['start-time'] = new Date(Utility.roundDownTime(5).getTime() - 1000 * 50 * 5).toISOString() as any,
      queryBody['end-time'] = Utility.roundDownTime(5).toISOString() as any;
  }

  public static maxObjQueryMerge(currData: ITelemetry_queryMetricsQueryResult, newData: ITelemetry_queryMetricsQueryResult) {
    // since we round down to get the last 5 min bucket, there's a chance
    // that we can back null data, since no new metrics have been reported.
    // Data should have been filtered in metricsquery services's processData
    if (!MetricsUtility.resultHasData(newData)) {
      // no new data, keep old value
      return currData;
    }
    return newData;
  }

  /**
   * Returns the name and avg of the node with the highest avg
   */
  public static maxObjUtility(data: ITelemetry_queryMetricsQueryResult, fieldName: string): { name: string, max: number } {
    if (!MetricsUtility.resultHasData(data) || fieldName == null || fieldName === '') {
      return null;
    }
    const objMaxList = [];
    data.series.forEach((s) => {
      const index = s.columns.indexOf(fieldName);
      const tags = s.tags as any;
      const key = tags.reporterID;
      let max;
      if (index !== -1 &&
        index < s.values[0].length &&
        key != null &&
        s.values != null &&
        s.values.length !== 0) {
        max = s.values[0][index];
        objMaxList.push({ name: key, max: max });
      }
    });
    if (objMaxList.length === 0) {
      return null;
    }
    const maxObj = Utility.getLodash().maxBy(objMaxList, 'max');
    return maxObj;
  }

  public static responseHasData(resp: ITelemetry_queryMetricsQueryResponse) {
    if (resp && resp.results &&
      resp.results.length !== 0 &&
      this.resultHasData(resp.results[0])) {
      return true;
    } else {
      return false;
    }
  }

  public static resultHasData(resp: ITelemetry_queryMetricsQueryResult) {
    if (resp && resp.series &&
      resp.series.length !== 0 &&
      resp.series[0].values &&
      resp.series[0].values.length !== 0) {
      return true;
    } else {
      return false;
    }
  }

  public static createNameSelector(name: string): IFieldsSelector {
    if (name != null) {
      return {
        'requirements': [
          {
            'key': 'Name',
            'operator': FieldsRequirement_operator.equals,
            'values': [name]
          }
        ]
      };
    }
  }

  public static clusterLevelCPUHeroCard(themeColor: string, icon: Icon): HeroCardOptions {
    return {
      title: 'CPU',
      firstStat: {
        numericValue: null,
        value: null,
        description: 'CPU Usage',
        tooltip: 'Averaged over past 5m'
      },
      secondStat: {
        numericValue: null,
        value: null,
        description: '24h Avg',
        tooltip: 'Averaged over past 24h'
      },
      thirdStat: {
        numericValue: null,
        value: null,
        description: 'Highest CPU Usage',
        tooltip: 'Averaged over past 5m'
      },
      data: {
        x: [],
        y: []
      },
      backgroundIcon: {
        svgIcon: 'cpu',
        margin: {}
      },
      themeColor: themeColor,
      icon: icon,
      cardState: CardStates.LOADING
    };
  }

  public static clusterLevelMemHeroCard(themeColor: string, icon: Icon): HeroCardOptions {
    return {
      title: 'Memory',
      firstStat: {
        numericValue: null,
        value: null,
        description: 'Memory Usage',
        tooltip: 'Averaged over past 5m'
      },
      secondStat: {
        numericValue: null,
        value: null,
        description: '24h Avg',
        tooltip: 'Averaged over past 24h'
      },
      thirdStat: {
        numericValue: null,
        value: null,
        description: 'Highest Memory Usage',
        tooltip: 'Averaged over past 5m'
      },
      data: {
        x: [],
        y: []
      },
      backgroundIcon: {
        svgIcon: 'memory',
        margin: {}
      },
      themeColor: themeColor,
      icon: icon,
      cardState: CardStates.LOADING
    };
  }

  public static clusterLevelDiskHeroCard(themeColor: string, icon: Icon): HeroCardOptions {
    return {
      title: 'Storage',
      firstStat: {
        numericValue: null,
        value: null,
        description: 'Disk Usage',
        tooltip: 'Averaged over past 5m'
      },
      secondStat: {
        numericValue: null,
        value: null,
        description: '24h Avg',
        tooltip: 'Averaged over past 24h'
      },
      thirdStat: {
        numericValue: null,
        value: null,
        description: 'Highest Disk Usage',
        tooltip: 'Averaged over past 5m'
      },
      data: {
        x: [],
        y: []
      },
      backgroundIcon: {
        svgIcon: 'storage',
        margin: {}
      },
      themeColor: themeColor,
      icon: icon,
      cardState: CardStates.LOADING
    };
  }

  public static detailLevelCPUHeroCard(themeColor: string, icon: Icon): HeroCardOptions {
    return {
      title: 'CPU',
      firstStat: {
        numericValue: null,
        value: null,
        description: 'CPU Usage',
        tooltip: 'Averaged over past 5m'
      },
      secondStat: {
        numericValue: null,
        value: null,
        description: '24h Avg',
        tooltip: 'Averaged over past 24h'
      },
      thirdStat: {
        numericValue: null,
        value: null,
        description: 'Cluster Usage',
        tooltip: 'Averaged over past 5m'
      },
      data: {
        x: [],
        y: []
      },
      backgroundIcon: {
        svgIcon: 'cpu',
        margin: {}
      },
      themeColor: themeColor,
      icon: icon
    };
  }

  public static detailLevelMemHeroCard(themeColor: string, icon: Icon): HeroCardOptions {
    return {
      title: 'Memory',
      firstStat: {
        numericValue: null,
        value: null,
        description: 'Memory Usage',
        tooltip: 'Averaged over past 5m'
      },
      secondStat: {
        numericValue: null,
        value: null,
        description: '24h Avg',
        tooltip: 'Averaged over past 24h'
      },
      thirdStat: {
        numericValue: null,
        value: null,
        description: 'Cluster Usage',
        tooltip: 'Averaged over past 5m'
      },
      data: {
        x: [],
        y: []
      },
      backgroundIcon: {
        svgIcon: 'memory',
        margin: {}
      },
      themeColor: themeColor,
      icon: icon
    };
  }

  public static detailLevelDiskHeroCard(themeColor: string, icon: Icon): HeroCardOptions {
    return {
      title: 'Storage',
      firstStat: {
        numericValue: null,
        value: null,
        description: 'Disk Usage',
        tooltip: 'Averaged over past 5m'
      },
      secondStat: {
        numericValue: null,
        value: null,
        description: '24h Avg',
        tooltip: 'Averaged over past 24h'
      },
      thirdStat: {
        numericValue: null,
        value: null,
        description: 'Cluster Usage',
        tooltip: 'Averaged over past 5m'
      },
      data: {
        x: [],
        y: []
      },
      backgroundIcon: {
        svgIcon: 'storage',
        margin: {}
      },
      themeColor: themeColor,
      icon: icon
    };
  }

  public static generateNaplesReporterId(naples: IClusterSmartNIC) {
    return naples.meta.name;
  }

  public static setCardStatesFailed(cards: HeroCardOptions[]) {
    cards.forEach((c) => {
      c.cardState = CardStates.FAILED;
    });
  }

  public static setCardStatesNoData(cards: HeroCardOptions[]) {
    cards.forEach((c) => {
      c.cardState = CardStates.NO_DATA;
    });
  }

  public static transformToChartjsTimeSeries(data: ITelemetry_queryMetricsResultSeries, fieldName, toRound: boolean = false): { t: Date, y: any }[] {
    const retData = [];
    const yFieldIndex = MetricsUtility.findFieldIndex(data.columns, fieldName);
    if (yFieldIndex < 0) {
      console.error('MetricsUtility transformToChartjsTimeSeries: given column name ' + fieldName + ' was not found');
    }
    data.values.forEach((item) => {
      let val = item[yFieldIndex];
      if (toRound) {
        val = Math.round(val);
      }
      retData.push({ t: item[0], y:  val });
    });
    return retData;
  }

  public static findFieldIndex(cols: string[], field: string) {
    return cols.findIndex((f) => {
      return f.toLowerCase().includes(field.toLowerCase());
    });
  }

}
