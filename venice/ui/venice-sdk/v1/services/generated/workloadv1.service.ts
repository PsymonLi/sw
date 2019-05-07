import { AbstractService } from './abstract.service';
import { HttpClient } from '../../../../webapp/node_modules/@angular/common/http';
import { Observable } from '../../../../webapp/node_modules/rxjs';
import { Injectable } from '../../../../webapp/node_modules/@angular/core';
import { TrimDefaultsAndEmptyFields } from '../../../v1/utils/utility';

import { IWorkloadEndpointList,WorkloadEndpointList,IApiStatus,ApiStatus,IWorkloadEndpoint,WorkloadEndpoint,IWorkloadWorkloadList,WorkloadWorkloadList,IWorkloadWorkload,WorkloadWorkload,IWorkloadAutoMsgEndpointWatchHelper,WorkloadAutoMsgEndpointWatchHelper,IWorkloadAutoMsgWorkloadWatchHelper,WorkloadAutoMsgWorkloadWatchHelper } from '../../models/generated/workload';

@Injectable()
export class Workloadv1Service extends AbstractService {
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

  /** List Endpoint objects */
  public ListEndpoint_1(queryParam: any = null, stagingID: string = ""):Observable<{body: IWorkloadEndpointList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/endpoints';
    const opts = {
      eventID: 'ListEndpoint_1',
      objType: 'WorkloadEndpointList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IWorkloadEndpointList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get Endpoint object */
  public GetEndpoint_1(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: IWorkloadEndpoint | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/endpoints/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetEndpoint_1',
      objType: 'WorkloadEndpoint',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IWorkloadEndpoint | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List Endpoint objects */
  public ListEndpoint(queryParam: any = null, stagingID: string = ""):Observable<{body: IWorkloadEndpointList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/tenant/{O.Tenant}/endpoints';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'ListEndpoint',
      objType: 'WorkloadEndpointList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IWorkloadEndpointList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get Endpoint object */
  public GetEndpoint(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: IWorkloadEndpoint | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/tenant/{O.Tenant}/endpoints/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetEndpoint',
      objType: 'WorkloadEndpoint',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IWorkloadEndpoint | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List Workload objects */
  public ListWorkload(queryParam: any = null, stagingID: string = ""):Observable<{body: IWorkloadWorkloadList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/tenant/{O.Tenant}/workloads';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'ListWorkload',
      objType: 'WorkloadWorkloadList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IWorkloadWorkloadList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create Workload object */
  public AddWorkload(body: IWorkloadWorkload, stagingID: string = "", trimObject: boolean = true):Observable<{body: IWorkloadWorkload | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/tenant/{O.Tenant}/workloads';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'AddWorkload',
      objType: 'WorkloadWorkload',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new WorkloadWorkload(body))
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IWorkloadWorkload | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get Workload object */
  public GetWorkload(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: IWorkloadWorkload | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/tenant/{O.Tenant}/workloads/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetWorkload',
      objType: 'WorkloadWorkload',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IWorkloadWorkload | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete Workload object */
  public DeleteWorkload(O_Name, stagingID: string = ""):Observable<{body: IWorkloadWorkload | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/tenant/{O.Tenant}/workloads/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteWorkload',
      objType: 'WorkloadWorkload',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: IWorkloadWorkload | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update Workload object */
  public UpdateWorkload(O_Name, body: IWorkloadWorkload, stagingID: string = "", trimObject: boolean = true):Observable<{body: IWorkloadWorkload | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/tenant/{O.Tenant}/workloads/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateWorkload',
      objType: 'WorkloadWorkload',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new WorkloadWorkload(body))
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: IWorkloadWorkload | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch Endpoint objects. Supports WebSockets or HTTP long poll */
  public WatchEndpoint_1(queryParam: any = null, stagingID: string = ""):Observable<{body: IWorkloadAutoMsgEndpointWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/watch/endpoints';
    const opts = {
      eventID: 'WatchEndpoint_1',
      objType: 'WorkloadAutoMsgEndpointWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IWorkloadAutoMsgEndpointWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch Endpoint objects. Supports WebSockets or HTTP long poll */
  public WatchEndpoint(queryParam: any = null, stagingID: string = ""):Observable<{body: IWorkloadAutoMsgEndpointWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/watch/tenant/{O.Tenant}/endpoints';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'WatchEndpoint',
      objType: 'WorkloadAutoMsgEndpointWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IWorkloadAutoMsgEndpointWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch Workload objects. Supports WebSockets or HTTP long poll */
  public WatchWorkload(queryParam: any = null, stagingID: string = ""):Observable<{body: IWorkloadAutoMsgWorkloadWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/watch/tenant/{O.Tenant}/workloads';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'WatchWorkload',
      objType: 'WorkloadAutoMsgWorkloadWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IWorkloadAutoMsgWorkloadWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch Workload objects. Supports WebSockets or HTTP long poll */
  public WatchWorkload_1(queryParam: any = null, stagingID: string = ""):Observable<{body: IWorkloadAutoMsgWorkloadWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/watch/workloads';
    const opts = {
      eventID: 'WatchWorkload_1',
      objType: 'WorkloadAutoMsgWorkloadWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IWorkloadAutoMsgWorkloadWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List Workload objects */
  public ListWorkload_1(queryParam: any = null, stagingID: string = ""):Observable<{body: IWorkloadWorkloadList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/workloads';
    const opts = {
      eventID: 'ListWorkload_1',
      objType: 'WorkloadWorkloadList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IWorkloadWorkloadList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create Workload object */
  public AddWorkload_1(body: IWorkloadWorkload, stagingID: string = "", trimObject: boolean = true):Observable<{body: IWorkloadWorkload | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/workloads';
    const opts = {
      eventID: 'AddWorkload_1',
      objType: 'WorkloadWorkload',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new WorkloadWorkload(body))
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IWorkloadWorkload | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get Workload object */
  public GetWorkload_1(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: IWorkloadWorkload | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/workloads/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetWorkload_1',
      objType: 'WorkloadWorkload',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IWorkloadWorkload | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete Workload object */
  public DeleteWorkload_1(O_Name, stagingID: string = ""):Observable<{body: IWorkloadWorkload | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/workloads/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteWorkload_1',
      objType: 'WorkloadWorkload',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: IWorkloadWorkload | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update Workload object */
  public UpdateWorkload_1(O_Name, body: IWorkloadWorkload, stagingID: string = "", trimObject: boolean = true):Observable<{body: IWorkloadWorkload | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/workload/v1/workloads/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateWorkload_1',
      objType: 'WorkloadWorkload',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new WorkloadWorkload(body))
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: IWorkloadWorkload | IApiStatus | Error, statusCode: number}>;
  }
  
}