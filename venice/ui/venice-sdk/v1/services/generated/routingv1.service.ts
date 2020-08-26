import { AbstractService, ServerEvent } from './abstract.service';
import { HttpClient } from '../../../../webapp/node_modules/@angular/common/http';
import { Observable } from '../../../../webapp/node_modules/rxjs';
import { Injectable } from '../../../../webapp/node_modules/@angular/core';
import { TrimDefaultsAndEmptyFields, TrimUIFields } from '../../../v1/utils/utility';

import { IRoutingHealth,RoutingHealth,IRoutingNeighborList,RoutingNeighborList,IRoutingRouteList,RoutingRouteList,RoutingRouteFilter,IRoutingRouteFilter } from '../../models/generated/routing';

export class Routingv1Service extends AbstractService {
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

  public GetHealthZ(Instance, queryParam: any = null):Observable<{body: IRoutingHealth | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/routing/v1/{Instance}/health';
    url = url.replace('{Instance}', Instance);
    const opts = {
      eventID: 'GetHealthZ',
      objType: 'RoutingHealth',
      isStaging: false,
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IRoutingHealth | Error, statusCode: number}>;
  }
  
  public GetListNeighbors(Instance, queryParam: any = null):Observable<{body: IRoutingNeighborList | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/routing/v1/{Instance}/neighbors';
    url = url.replace('{Instance}', Instance);
    const opts = {
      eventID: 'GetListNeighbors',
      objType: 'RoutingNeighborList',
      isStaging: false,
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IRoutingNeighborList | Error, statusCode: number}>;
  }
  
  public GetListRoutes_1(Instance, queryParam: any = null):Observable<{body: IRoutingRouteList | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/routing/v1/{Instance}/routes';
    url = url.replace('{Instance}', Instance);
    const opts = {
      eventID: 'GetListRoutes_1',
      objType: 'RoutingRouteList',
      isStaging: false,
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IRoutingRouteList | Error, statusCode: number}>;
  }
  
  public PostListRoutes(Instance, body: IRoutingRouteFilter, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IRoutingRouteList | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/routing/v1/{Instance}/routes';
    url = url.replace('{Instance}', Instance);
    const opts = {
      eventID: 'PostListRoutes',
      objType: 'RoutingRouteList',
      isStaging: false,
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new RoutingRouteFilter(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IRoutingRouteList | Error, statusCode: number}>;
  }
  
}