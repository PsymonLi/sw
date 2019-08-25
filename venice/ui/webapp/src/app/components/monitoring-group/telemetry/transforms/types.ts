import { ITelemetry_queryMetricsQuerySpec } from '@sdk/v1/models/generated/telemetry_query';
import { ITelemetry_queryMetricsResultSeries, ITelemetry_queryMetricsQueryResult } from '@sdk/v1/models/telemetry_query';
import { ChartData as ChartJSData, ChartDataSets as ChartJSDataSets, ChartOptions } from 'chart.js';
import { Observer } from 'rxjs';
import { Utility } from '@app/common/Utility';
import { MetricsMetadata } from '@sdk/metrics/generated/metadata';
import { DataTransformConfig  } from '@app/models/frontend/shared/userpreference.interface';
import { TelemetryPollingMetricQueries } from '@app/services/metricsquery.service';

export enum TransformNames {
  GroupByTime = 'GroupByTime',
  AxisTransform = 'AxisTransform',
  ColorTransform = 'ColorTransform',
  DisplayLabelTransform = 'DisplayLabel',
  FieldSelector = 'FieldSelector',
  FieldValue = 'FieldValue',
  GraphTitle = 'GraphTitle',
  GroupBy = 'GroupBy',
  LabelSelector = 'LabelSelector',
  RoundCounters = 'RoundCounters',
  Derivative = 'Derivative',
}

export interface ChartDataSets extends ChartJSDataSets {
  sourceID: string;
  sourceMeasurement: string;
  sourceField: string;
}

export interface ChartData extends ChartJSData {
  datasets: ChartDataSets[];
}

export interface TransformQuery {
  query: ITelemetry_queryMetricsQuerySpec;
}

export interface TransformMetricData {
  result: ITelemetry_queryMetricsQueryResult;
}

export interface TransformDataset {
  dataset: ChartDataSets;
  series: ITelemetry_queryMetricsResultSeries;
  measurement: string;
  field: string;
  units: string;
  fieldIndex: number;
}

export interface TransformGraphOptions {
  data: TransformDataset[];
  graphOptions: ChartOptions;
  oldGraphOptions: ChartOptions;
}

export interface TransformMetricDatasets {
  results: ITelemetry_queryMetricsQueryResult[];
}

export interface TransformQueries {
  queries: TelemetryPollingMetricQueries;
}

export interface TransformDatasets extends Array<TransformDataset> {}

export abstract class MetricTransform<T> {
  abstract transformName: TransformNames;

  // Fields that will be populated by MetricSource
  private reqMetrics: Observer<any>;
  measurement: string;
  fields: string[];
  debugMode: boolean;
  getTransform: (transformName: string) => MetricTransform<any>;


  abstract load(config: T);
  abstract save(): T;

  // Hooks to be overridden
  onMeasurementChange() {}
  onFieldChange() {}
  onDebugModeChange() {}
  // Hook called before query goes to Venice
  transformQuery(opts: TransformQuery) {}
  // Hook called before data is converted to chart js datasets
  transformMetricData(opts: TransformMetricData) {}
  // Hook called after each dataset is created
  transformDataset(opts: TransformDataset) {}
  // Hook called after all datasets have been created for a data source
  transformDatasets(opts: TransformDatasets) {}

  // Utility functions
  requestMetrics() {
    this.reqMetrics.next(true);
  }

  getFieldData(measurement, field) {
    return MetricsMetadata[measurement].fields.find(x => x.name === field);
  }
}

export abstract class GraphTransform<T> {
  abstract transformName: string;

  // Fields that will be populated by MetricSource
  private reqRedraw: Observer<any>;


  abstract load(config: T);
  abstract save(): T;


  // Hooks to be overridden
  // Hook to set graph options after all datasets have been created for all results
  transformGraphOptions(opts: TransformGraphOptions) {}

  // Hook called before data is converted to chart js datasets
  transformQueries(opts: TransformQueries) {}

  // Hook called before data is converted to chart js datasets
  transformMetricData(opts: TransformMetricDatasets) {}

  // Utility functions
  getFieldData(measurement, field) {
    return MetricsMetadata[measurement].fields.find(x => x.name === field);
  }
  requestRedraw() {
    this.reqRedraw.next(true);
  }
}


/**
 * Data source object is used for storing user input
 * of graph options/data, and for data transforms to alter
 * the metric queries and rendering
 *
 * There can be multiple data sources and they function independently.
 * Each data source has one measurement, and any number of fields under
 * that measurement.
 * Transforms can also be registered during creation, and the data transform
 * hooks will be called on each registered transform.
 */
export class DataSource {
  private _measurement: string;
  private _fields: string[] = [];
  private _debugMode: boolean = false;

  // Unique id is used to differentiate datasets that are exactly the same,
  // but come from two different data sources
  id: string = Utility.s4() + '-' + Utility.s4() ;

  // List of transforms registered
  transforms: MetricTransform<any>[] = [];

  // Map from transform name to the instance
  transformMap: { [transformName: string]: MetricTransform<any> };

  // Observer that will be taken in during construction. Will emit
  // whenever it detects we need to execute a new metric request or change the rendering options.
  reqMetrics: Observer<any>;

  get measurement() {
    return this._measurement;
  }

  set measurement(value) {
    // Update all transforms and null the field values
    this._measurement = value;
    this._fields = [];
    this.transforms.forEach( (t) => {
      t.measurement = this.measurement;
      t.fields = null;
      t.onMeasurementChange();
      t.onFieldChange();
    });
    this.reqMetrics.next(true);
  }

  get fields() {
    return this._fields;
  }

  set fields(value: string[]) {
    if (value == null) {
      value = [];
    }
    this._fields = value;
    this.transforms.forEach( (t) => {
      t.fields = this.fields;
      t.onFieldChange();
    });
    this.reqMetrics.next(true);
  }

  get debugMode() {
    return this._debugMode;
  }

  set debugMode(value: boolean) {
    if (this._debugMode === value) {
      return;
    }
    this._debugMode = value;
    this.transforms.forEach( (t) => {
      t.debugMode = value;
      t.onDebugModeChange();
    });
  }

  getTransform(transformName: string): MetricTransform<any> {
    return this.transformMap[transformName];
  }

  constructor(reqMetrics: Observer<any>, transforms: MetricTransform<any>[] = []) {
    this.transforms = transforms;
    this.reqMetrics = reqMetrics;
    this.transformMap = {};
    // For each transform we populate fields so that transforms can see
    // the state of other transforms, and can request metrics.
    transforms.forEach( (t) => {
      // Casting to any so we can set private variables
      (<any>t).reqMetrics = reqMetrics;
      this.transformMap[t.transformName] = t;
      t.getTransform = (name: string) => {
        return this.getTransform(name);
      };
    });
  }

  load(config: DataTransformConfig) {
    this.measurement = config.measurement;
    this.fields = config.fields;
    this.transforms.forEach( (t) => {
      t.load(config.transforms[t.transformName]);
    });
  }

  save(): DataTransformConfig {
    const ret = {};
    this.transforms.forEach( (t) => {
      ret[t.transformName] = t.save();
    });
    return {
      transforms: ret,
      measurement: this.measurement,
      fields: this.fields,
     };
  }

  transformQuery(opts: TransformQuery) {
    this.transforms.forEach( (t) => {
      t.transformQuery(opts);
    });
  }

  transformMetricData(opts: TransformMetricData) {
    this.transforms.forEach( (t) => {
      t.transformMetricData(opts);
    });
  }

  transformDataset(opts: TransformDataset) {
    this.transforms.forEach( (t) => {
      t.transformDataset(opts);
    });
  }

  transformDatasets(opts: TransformDatasets) {
    this.transforms.forEach( (t) => {
      t.transformDatasets(opts);
    });
  }
}
