import { AbstractService, ServerEvent } from './abstract.service';
import { HttpClient } from '../../../../webapp/node_modules/@angular/common/http';
import { Observable } from '../../../../webapp/node_modules/rxjs';
import { Injectable } from '../../../../webapp/node_modules/@angular/core';
import { TrimDefaultsAndEmptyFields, TrimUIFields } from '../../../v1/utils/utility';

import { ITelemetry_queryMetricsQueryResponse,Telemetry_queryMetricsQueryResponse,Telemetry_queryMetricsQueryList,ITelemetry_queryMetricsQueryList } from '../../models/generated/telemetry_query';

@Injectable()
export class Telemetry_queryv1Service extends AbstractService {
  constructor(protected _http: HttpClient) {
    super(_http);
  }

  /**
   * Override super
   * Get the service class-name
  */
  getClassName(): string {
    return this.constructor.name;
  }
  bufferDelayMap: { [key: string]: number } = {
  }

  /** telemetry metrics query */
  public GetMetrics_1(queryParam: any = null):Observable<{body: ITelemetry_queryMetricsQueryResponse | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/telemetry/v1/metrics';
    const opts = {
      eventID: 'GetMetrics_1',
      objType: 'Telemetry_queryMetricsQueryResponse',
      isStaging: false,
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: ITelemetry_queryMetricsQueryResponse | Error, statusCode: number}>;
  }
  
  /** telemetry metrics query */
  public PostMetrics(body: ITelemetry_queryMetricsQueryList, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: ITelemetry_queryMetricsQueryResponse | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/telemetry/v1/metrics';
    const opts = {
      eventID: 'PostMetrics',
      objType: 'Telemetry_queryMetricsQueryResponse',
      isStaging: false,
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new Telemetry_queryMetricsQueryList(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: ITelemetry_queryMetricsQueryResponse | Error, statusCode: number}>;
  }
  
}