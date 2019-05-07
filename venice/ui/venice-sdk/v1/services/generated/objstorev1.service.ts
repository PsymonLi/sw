import { AbstractService } from './abstract.service';
import { HttpClient } from '../../../../webapp/node_modules/@angular/common/http';
import { Observable } from '../../../../webapp/node_modules/rxjs';
import { Injectable } from '../../../../webapp/node_modules/@angular/core';
import { TrimDefaultsAndEmptyFields } from '../../../v1/utils/utility';

import { IObjstoreStreamChunk,ObjstoreStreamChunk,IObjstoreObjectList,ObjstoreObjectList,IApiStatus,ApiStatus,IObjstoreObject,ObjstoreObject,IObjstoreAutoMsgObjectWatchHelper,ObjstoreAutoMsgObjectWatchHelper } from '../../models/generated/objstore';

@Injectable()
export class Objstorev1Service extends AbstractService {
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

  public GetDownloadFile(O_Namespace,O_Name, queryParam: any = null):Observable<{body: IObjstoreStreamChunk | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/objstore/v1/downloads/tenant/{O.Tenant}/{O.Namespace}/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Namespace}', O_Namespace);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetDownloadFile',
      objType: 'ObjstoreStreamChunk',
      isStaging: false,
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IObjstoreStreamChunk | Error, statusCode: number}>;
  }
  
  public GetDownloadFile_1(O_Namespace,O_Name, queryParam: any = null):Observable<{body: IObjstoreStreamChunk | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/objstore/v1/downloads/{O.Namespace}/{O.Name}';
    url = url.replace('{O.Namespace}', O_Namespace);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetDownloadFile_1',
      objType: 'ObjstoreStreamChunk',
      isStaging: false,
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IObjstoreStreamChunk | Error, statusCode: number}>;
  }
  
  /** List Object objects */
  public ListObject(O_Namespace, queryParam: any = null):Observable<{body: IObjstoreObjectList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/objstore/v1/tenant/{O.Tenant}/{O.Namespace}/objects';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Namespace}', O_Namespace);
    const opts = {
      eventID: 'ListObject',
      objType: 'ObjstoreObjectList',
      isStaging: false,
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IObjstoreObjectList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get Object object */
  public GetObject(O_Namespace,O_Name, queryParam: any = null):Observable<{body: IObjstoreObject | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/objstore/v1/tenant/{O.Tenant}/{O.Namespace}/objects/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Namespace}', O_Namespace);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetObject',
      objType: 'ObjstoreObject',
      isStaging: false,
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IObjstoreObject | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete Object object */
  public DeleteObject(O_Namespace,O_Name):Observable<{body: IObjstoreObject | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/objstore/v1/tenant/{O.Tenant}/{O.Namespace}/objects/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Namespace}', O_Namespace);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteObject',
      objType: 'ObjstoreObject',
      isStaging: false,
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: IObjstoreObject | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch Object objects. Supports WebSockets or HTTP long poll */
  public WatchObject(O_Namespace, queryParam: any = null):Observable<{body: IObjstoreAutoMsgObjectWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/objstore/v1/watch/tenant/{O.Tenant}/{O.Namespace}/objects';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Namespace}', O_Namespace);
    const opts = {
      eventID: 'WatchObject',
      objType: 'ObjstoreAutoMsgObjectWatchHelper',
      isStaging: false,
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IObjstoreAutoMsgObjectWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch Object objects. Supports WebSockets or HTTP long poll */
  public WatchObject_1(O_Namespace, queryParam: any = null):Observable<{body: IObjstoreAutoMsgObjectWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/objstore/v1/watch/{O.Namespace}/objects';
    url = url.replace('{O.Namespace}', O_Namespace);
    const opts = {
      eventID: 'WatchObject_1',
      objType: 'ObjstoreAutoMsgObjectWatchHelper',
      isStaging: false,
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IObjstoreAutoMsgObjectWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List Object objects */
  public ListObject_1(O_Namespace, queryParam: any = null):Observable<{body: IObjstoreObjectList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/objstore/v1/{O.Namespace}/objects';
    url = url.replace('{O.Namespace}', O_Namespace);
    const opts = {
      eventID: 'ListObject_1',
      objType: 'ObjstoreObjectList',
      isStaging: false,
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IObjstoreObjectList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get Object object */
  public GetObject_1(O_Namespace,O_Name, queryParam: any = null):Observable<{body: IObjstoreObject | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/objstore/v1/{O.Namespace}/objects/{O.Name}';
    url = url.replace('{O.Namespace}', O_Namespace);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetObject_1',
      objType: 'ObjstoreObject',
      isStaging: false,
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IObjstoreObject | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete Object object */
  public DeleteObject_1(O_Namespace,O_Name):Observable<{body: IObjstoreObject | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/objstore/v1/{O.Namespace}/objects/{O.Name}';
    url = url.replace('{O.Namespace}', O_Namespace);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteObject_1',
      objType: 'ObjstoreObject',
      isStaging: false,
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: IObjstoreObject | IApiStatus | Error, statusCode: number}>;
  }
  
}