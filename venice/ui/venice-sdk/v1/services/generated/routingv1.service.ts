import { AbstractService, ServerEvent } from './abstract.service';
import { HttpClient } from '../../../../webapp/node_modules/@angular/common/http';
import { Observable } from '../../../../webapp/node_modules/rxjs';
import { Injectable } from '../../../../webapp/node_modules/@angular/core';
import { TrimDefaultsAndEmptyFields, TrimUIFields } from '../../../v1/utils/utility';

import { IRoutingHealth,RoutingHealth,IRoutingNeighborList,RoutingNeighborList,IRoutingNeighbor,RoutingNeighbor } from '../../models/generated/routing';

@Injectable()
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
  
  public GetGetNeighbor(Instance,Neighbor, queryParam: any = null):Observable<{body: IRoutingNeighbor | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/routing/v1/{Instance}/neighbors/{Neighbor}';
    url = url.replace('{Instance}', Instance);
    url = url.replace('{Neighbor}', Neighbor);
    const opts = {
      eventID: 'GetGetNeighbor',
      objType: 'RoutingNeighbor',
      isStaging: false,
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IRoutingNeighbor | Error, statusCode: number}>;
  }
  
}