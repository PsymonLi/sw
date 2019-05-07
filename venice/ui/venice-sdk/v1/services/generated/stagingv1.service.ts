import { AbstractService } from './abstract.service';
import { HttpClient } from '../../../../webapp/node_modules/@angular/common/http';
import { Observable } from '../../../../webapp/node_modules/rxjs';
import { Injectable } from '../../../../webapp/node_modules/@angular/core';
import { TrimDefaultsAndEmptyFields } from '../../../v1/utils/utility';

import { IStagingBufferList,StagingBufferList,IApiStatus,ApiStatus,IStagingBuffer,StagingBuffer,IStagingClearAction,StagingClearAction,IStagingCommitAction,StagingCommitAction } from '../../models/generated/staging';

@Injectable()
export class Stagingv1Service extends AbstractService {
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

  /** List Buffer objects */
  public ListBuffer_1(queryParam: any = null, stagingID: string = ""):Observable<{body: IStagingBufferList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/staging/v1/buffers';
    const opts = {
      eventID: 'ListBuffer_1',
      objType: 'StagingBufferList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IStagingBufferList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create Buffer object */
  public AddBuffer_1(body: IStagingBuffer, stagingID: string = "", trimObject: boolean = true):Observable<{body: IStagingBuffer | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/staging/v1/buffers';
    const opts = {
      eventID: 'AddBuffer_1',
      objType: 'StagingBuffer',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new StagingBuffer(body))
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IStagingBuffer | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get Buffer object */
  public GetBuffer_1(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: IStagingBuffer | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/staging/v1/buffers/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetBuffer_1',
      objType: 'StagingBuffer',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IStagingBuffer | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete Buffer object */
  public DeleteBuffer_1(O_Name, stagingID: string = ""):Observable<{body: IStagingBuffer | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/staging/v1/buffers/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteBuffer_1',
      objType: 'StagingBuffer',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: IStagingBuffer | IApiStatus | Error, statusCode: number}>;
  }
  
  public Clear_1(O_Name, body: IStagingClearAction, stagingID: string = "", trimObject: boolean = true):Observable<{body: IStagingClearAction | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/staging/v1/buffers/{O.Name}/clear';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'Clear_1',
      objType: 'StagingClearAction',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new StagingClearAction(body))
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IStagingClearAction | IApiStatus | Error, statusCode: number}>;
  }
  
  public Commit_1(O_Name, body: IStagingCommitAction, stagingID: string = "", trimObject: boolean = true):Observable<{body: IStagingCommitAction | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/staging/v1/buffers/{O.Name}/commit';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'Commit_1',
      objType: 'StagingCommitAction',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new StagingCommitAction(body))
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IStagingCommitAction | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List Buffer objects */
  public ListBuffer(queryParam: any = null, stagingID: string = ""):Observable<{body: IStagingBufferList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/staging/v1/tenant/{O.Tenant}/buffers';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'ListBuffer',
      objType: 'StagingBufferList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IStagingBufferList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create Buffer object */
  public AddBuffer(body: IStagingBuffer, stagingID: string = "", trimObject: boolean = true):Observable<{body: IStagingBuffer | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/staging/v1/tenant/{O.Tenant}/buffers';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'AddBuffer',
      objType: 'StagingBuffer',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new StagingBuffer(body))
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IStagingBuffer | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get Buffer object */
  public GetBuffer(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: IStagingBuffer | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/staging/v1/tenant/{O.Tenant}/buffers/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetBuffer',
      objType: 'StagingBuffer',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IStagingBuffer | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete Buffer object */
  public DeleteBuffer(O_Name, stagingID: string = ""):Observable<{body: IStagingBuffer | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/staging/v1/tenant/{O.Tenant}/buffers/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteBuffer',
      objType: 'StagingBuffer',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: IStagingBuffer | IApiStatus | Error, statusCode: number}>;
  }
  
  public Clear(O_Name, body: IStagingClearAction, stagingID: string = "", trimObject: boolean = true):Observable<{body: IStagingClearAction | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/staging/v1/tenant/{O.Tenant}/buffers/{O.Name}/clear';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'Clear',
      objType: 'StagingClearAction',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new StagingClearAction(body))
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IStagingClearAction | IApiStatus | Error, statusCode: number}>;
  }
  
  public Commit(O_Name, body: IStagingCommitAction, stagingID: string = "", trimObject: boolean = true):Observable<{body: IStagingCommitAction | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/staging/v1/tenant/{O.Tenant}/buffers/{O.Name}/commit';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'Commit',
      objType: 'StagingCommitAction',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new StagingCommitAction(body))
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IStagingCommitAction | IApiStatus | Error, statusCode: number}>;
  }
  
}