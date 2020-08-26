import { AbstractService, ServerEvent } from './abstract.service';
import { HttpClient } from '../../../../webapp/node_modules/@angular/common/http';
import { Observable } from '../../../../webapp/node_modules/rxjs';
import { Injectable } from '../../../../webapp/node_modules/@angular/core';
import { TrimDefaultsAndEmptyFields, TrimUIFields } from '../../../v1/utils/utility';

import { IClusterCluster,ClusterCluster,IApiStatus,ApiStatus,ClusterClusterAuthBootstrapRequest,IClusterClusterAuthBootstrapRequest,ClusterUpdateTLSConfigRequest,IClusterUpdateTLSConfigRequest,ApiLabel,IApiLabel,IClusterSnapshotRestore,ClusterSnapshotRestore,IClusterConfigurationSnapshot,ClusterConfigurationSnapshot,ClusterConfigurationSnapshotRequest,IClusterConfigurationSnapshotRequest,IClusterDistributedServiceCardList,ClusterDistributedServiceCardList,IClusterDistributedServiceCard,ClusterDistributedServiceCard,IClusterDSCProfileList,ClusterDSCProfileList,IClusterDSCProfile,ClusterDSCProfile,IClusterHostList,ClusterHostList,IClusterHost,ClusterHost,IClusterLicense,ClusterLicense,IClusterNodeList,ClusterNodeList,IClusterNode,ClusterNode,IClusterTenantList,ClusterTenantList,IClusterTenant,ClusterTenant,IClusterVersion,ClusterVersion,IClusterAutoMsgClusterWatchHelper,ClusterAutoMsgClusterWatchHelper,IClusterAutoMsgConfigurationSnapshotWatchHelper,ClusterAutoMsgConfigurationSnapshotWatchHelper,IClusterAutoMsgDistributedServiceCardWatchHelper,ClusterAutoMsgDistributedServiceCardWatchHelper,IClusterAutoMsgDSCProfileWatchHelper,ClusterAutoMsgDSCProfileWatchHelper,IClusterAutoMsgHostWatchHelper,ClusterAutoMsgHostWatchHelper,IClusterAutoMsgNodeWatchHelper,ClusterAutoMsgNodeWatchHelper,IClusterAutoMsgTenantWatchHelper,ClusterAutoMsgTenantWatchHelper,IClusterAutoMsgVersionWatchHelper,ClusterAutoMsgVersionWatchHelper } from '../../models/generated/cluster';

export class Clusterv1Service extends AbstractService {
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
    'ClusterDistributedServiceCard': 100,
    'ClusterDSCProfile': 100,
    'ClusterHost': 100,
    'ClusterNode': 100,
    'ClusterTenant': 100,
  }

  /** Get Cluster object */
  public GetCluster(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterCluster | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/cluster';
    const opts = {
      eventID: 'GetCluster',
      objType: 'ClusterCluster',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterCluster | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update Cluster object */
  public UpdateCluster(body: IClusterCluster, stagingID: string = "", previousVal: IClusterCluster = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterCluster | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/cluster';
    const opts = {
      eventID: 'UpdateCluster',
      objType: 'ClusterCluster',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterCluster(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: IClusterCluster | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Mark bootstrapping as complete for the cluster */
  public AuthBootstrapComplete(body: IClusterClusterAuthBootstrapRequest, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterCluster | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/cluster/AuthBootstrapComplete';
    const opts = {
      eventID: 'AuthBootstrapComplete',
      objType: 'ClusterCluster',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterClusterAuthBootstrapRequest(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterCluster | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update TLS Configuration for cluster */
  public UpdateTLSConfig(body: IClusterUpdateTLSConfigRequest, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterCluster | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/cluster/UpdateTLSConfig';
    const opts = {
      eventID: 'UpdateTLSConfig',
      objType: 'ClusterCluster',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterUpdateTLSConfigRequest(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterCluster | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label Cluster object */
  public LabelCluster(body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterCluster | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/cluster/label';
    const opts = {
      eventID: 'LabelCluster',
      objType: 'ClusterCluster',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ApiLabel(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterCluster | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get SnapshotRestore object */
  public GetSnapshotRestore(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterSnapshotRestore | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/config-restore';
    const opts = {
      eventID: 'GetSnapshotRestore',
      objType: 'ClusterSnapshotRestore',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterSnapshotRestore | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Restore Configuration */
  public Restore(body: IClusterSnapshotRestore, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterSnapshotRestore | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/config-restore/restore';
    const opts = {
      eventID: 'Restore',
      objType: 'ClusterSnapshotRestore',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterSnapshotRestore(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterSnapshotRestore | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get ConfigurationSnapshot object */
  public GetConfigurationSnapshot(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterConfigurationSnapshot | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/config-snapshot';
    const opts = {
      eventID: 'GetConfigurationSnapshot',
      objType: 'ClusterConfigurationSnapshot',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterConfigurationSnapshot | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete ConfigurationSnapshot object */
  public DeleteConfigurationSnapshot(stagingID: string = ""):Observable<{body: IClusterConfigurationSnapshot | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/config-snapshot';
    const opts = {
      eventID: 'DeleteConfigurationSnapshot',
      objType: 'ClusterConfigurationSnapshot',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: IClusterConfigurationSnapshot | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create ConfigurationSnapshot object */
  public AddConfigurationSnapshot(body: IClusterConfigurationSnapshot, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterConfigurationSnapshot | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/config-snapshot';
    const opts = {
      eventID: 'AddConfigurationSnapshot',
      objType: 'ClusterConfigurationSnapshot',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterConfigurationSnapshot(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterConfigurationSnapshot | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update ConfigurationSnapshot object */
  public UpdateConfigurationSnapshot(body: IClusterConfigurationSnapshot, stagingID: string = "", previousVal: IClusterConfigurationSnapshot = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterConfigurationSnapshot | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/config-snapshot';
    const opts = {
      eventID: 'UpdateConfigurationSnapshot',
      objType: 'ClusterConfigurationSnapshot',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterConfigurationSnapshot(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: IClusterConfigurationSnapshot | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label ConfigurationSnapshot object */
  public LabelConfigurationSnapshot(body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterConfigurationSnapshot | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/config-snapshot/label';
    const opts = {
      eventID: 'LabelConfigurationSnapshot',
      objType: 'ClusterConfigurationSnapshot',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ApiLabel(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterConfigurationSnapshot | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Perform a Configuation Snapshot */
  public Save(body: IClusterConfigurationSnapshotRequest, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterConfigurationSnapshot | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/config-snapshot/save';
    const opts = {
      eventID: 'Save',
      objType: 'ClusterConfigurationSnapshot',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterConfigurationSnapshotRequest(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterConfigurationSnapshot | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List DistributedServiceCard objects */
  public ListDistributedServiceCard(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterDistributedServiceCardList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/distributedservicecards';
    const opts = {
      eventID: 'ListDistributedServiceCard',
      objType: 'ClusterDistributedServiceCardList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterDistributedServiceCardList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get DistributedServiceCard object */
  public GetDistributedServiceCard(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterDistributedServiceCard | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/distributedservicecards/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetDistributedServiceCard',
      objType: 'ClusterDistributedServiceCard',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterDistributedServiceCard | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete DistributedServiceCard object */
  public DeleteDistributedServiceCard(O_Name, stagingID: string = ""):Observable<{body: IClusterDistributedServiceCard | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/distributedservicecards/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteDistributedServiceCard',
      objType: 'ClusterDistributedServiceCard',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: IClusterDistributedServiceCard | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update DistributedServiceCard object */
  public UpdateDistributedServiceCard(O_Name, body: IClusterDistributedServiceCard, stagingID: string = "", previousVal: IClusterDistributedServiceCard = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterDistributedServiceCard | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/distributedservicecards/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateDistributedServiceCard',
      objType: 'ClusterDistributedServiceCard',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterDistributedServiceCard(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: IClusterDistributedServiceCard | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label DistributedServiceCard object */
  public LabelDistributedServiceCard(O_Name, body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterDistributedServiceCard | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/distributedservicecards/{O.Name}/label';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'LabelDistributedServiceCard',
      objType: 'ClusterDistributedServiceCard',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ApiLabel(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterDistributedServiceCard | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List DSCProfile objects */
  public ListDSCProfile(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterDSCProfileList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/dscprofiles';
    const opts = {
      eventID: 'ListDSCProfile',
      objType: 'ClusterDSCProfileList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterDSCProfileList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create DSCProfile object */
  public AddDSCProfile(body: IClusterDSCProfile, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterDSCProfile | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/dscprofiles';
    const opts = {
      eventID: 'AddDSCProfile',
      objType: 'ClusterDSCProfile',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterDSCProfile(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterDSCProfile | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get DSCProfile object */
  public GetDSCProfile(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterDSCProfile | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/dscprofiles/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetDSCProfile',
      objType: 'ClusterDSCProfile',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterDSCProfile | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete DSCProfile object */
  public DeleteDSCProfile(O_Name, stagingID: string = ""):Observable<{body: IClusterDSCProfile | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/dscprofiles/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteDSCProfile',
      objType: 'ClusterDSCProfile',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: IClusterDSCProfile | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update DSCProfile object */
  public UpdateDSCProfile(O_Name, body: IClusterDSCProfile, stagingID: string = "", previousVal: IClusterDSCProfile = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterDSCProfile | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/dscprofiles/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateDSCProfile',
      objType: 'ClusterDSCProfile',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterDSCProfile(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: IClusterDSCProfile | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label DSCProfile object */
  public LabelDSCProfile(O_Name, body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterDSCProfile | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/dscprofiles/{O.Name}/label';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'LabelDSCProfile',
      objType: 'ClusterDSCProfile',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ApiLabel(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterDSCProfile | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List Host objects */
  public ListHost(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterHostList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/hosts';
    const opts = {
      eventID: 'ListHost',
      objType: 'ClusterHostList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterHostList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create Host object */
  public AddHost(body: IClusterHost, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterHost | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/hosts';
    const opts = {
      eventID: 'AddHost',
      objType: 'ClusterHost',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterHost(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterHost | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get Host object */
  public GetHost(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterHost | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/hosts/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetHost',
      objType: 'ClusterHost',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterHost | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete Host object */
  public DeleteHost(O_Name, stagingID: string = ""):Observable<{body: IClusterHost | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/hosts/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteHost',
      objType: 'ClusterHost',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: IClusterHost | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update Host object */
  public UpdateHost(O_Name, body: IClusterHost, stagingID: string = "", previousVal: IClusterHost = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterHost | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/hosts/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateHost',
      objType: 'ClusterHost',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterHost(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: IClusterHost | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label Host object */
  public LabelHost(O_Name, body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterHost | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/hosts/{O.Name}/label';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'LabelHost',
      objType: 'ClusterHost',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ApiLabel(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterHost | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get License object */
  public GetLicense(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterLicense | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/licenses';
    const opts = {
      eventID: 'GetLicense',
      objType: 'ClusterLicense',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterLicense | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create License object */
  public AddLicense(body: IClusterLicense, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterLicense | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/licenses';
    const opts = {
      eventID: 'AddLicense',
      objType: 'ClusterLicense',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterLicense(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterLicense | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update License object */
  public UpdateLicense(body: IClusterLicense, stagingID: string = "", previousVal: IClusterLicense = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterLicense | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/licenses';
    const opts = {
      eventID: 'UpdateLicense',
      objType: 'ClusterLicense',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterLicense(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: IClusterLicense | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label License object */
  public LabelLicense(body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterLicense | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/licenses/label';
    const opts = {
      eventID: 'LabelLicense',
      objType: 'ClusterLicense',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ApiLabel(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterLicense | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List Node objects */
  public ListNode(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterNodeList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/nodes';
    const opts = {
      eventID: 'ListNode',
      objType: 'ClusterNodeList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterNodeList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create Node object */
  public AddNode(body: IClusterNode, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterNode | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/nodes';
    const opts = {
      eventID: 'AddNode',
      objType: 'ClusterNode',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterNode(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterNode | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get Node object */
  public GetNode(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterNode | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/nodes/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetNode',
      objType: 'ClusterNode',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterNode | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete Node object */
  public DeleteNode(O_Name, stagingID: string = ""):Observable<{body: IClusterNode | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/nodes/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteNode',
      objType: 'ClusterNode',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: IClusterNode | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update Node object */
  public UpdateNode(O_Name, body: IClusterNode, stagingID: string = "", previousVal: IClusterNode = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterNode | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/nodes/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateNode',
      objType: 'ClusterNode',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterNode(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: IClusterNode | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label Node object */
  public LabelNode(O_Name, body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterNode | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/nodes/{O.Name}/label';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'LabelNode',
      objType: 'ClusterNode',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ApiLabel(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterNode | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List Tenant objects */
  public ListTenant(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterTenantList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/tenants';
    const opts = {
      eventID: 'ListTenant',
      objType: 'ClusterTenantList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterTenantList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create Tenant object */
  public AddTenant(body: IClusterTenant, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterTenant | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/tenants';
    const opts = {
      eventID: 'AddTenant',
      objType: 'ClusterTenant',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterTenant(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterTenant | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get Tenant object */
  public GetTenant(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterTenant | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/tenants/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetTenant',
      objType: 'ClusterTenant',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterTenant | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete Tenant object */
  public DeleteTenant(O_Name, stagingID: string = ""):Observable<{body: IClusterTenant | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/tenants/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteTenant',
      objType: 'ClusterTenant',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: IClusterTenant | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update Tenant object */
  public UpdateTenant(O_Name, body: IClusterTenant, stagingID: string = "", previousVal: IClusterTenant = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterTenant | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/tenants/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateTenant',
      objType: 'ClusterTenant',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ClusterTenant(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: IClusterTenant | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label Tenant object */
  public LabelTenant(O_Name, body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: IClusterTenant | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/tenants/{O.Name}/label';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'LabelTenant',
      objType: 'ClusterTenant',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new ApiLabel(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: IClusterTenant | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get Version object */
  public GetVersion(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterVersion | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/version';
    const opts = {
      eventID: 'GetVersion',
      objType: 'ClusterVersion',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterVersion | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch Cluster objects. Supports WebSockets or HTTP long poll */
  public WatchCluster(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterAutoMsgClusterWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/watch/cluster';
    const opts = {
      eventID: 'WatchCluster',
      objType: 'ClusterAutoMsgClusterWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterAutoMsgClusterWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch ConfigurationSnapshot objects. Supports WebSockets or HTTP long poll */
  public WatchConfigurationSnapshot(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterAutoMsgConfigurationSnapshotWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/watch/config-snapshot';
    const opts = {
      eventID: 'WatchConfigurationSnapshot',
      objType: 'ClusterAutoMsgConfigurationSnapshotWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterAutoMsgConfigurationSnapshotWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch DistributedServiceCard objects. Supports WebSockets or HTTP long poll */
  public WatchDistributedServiceCard(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterAutoMsgDistributedServiceCardWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/watch/distributedservicecards';
    const opts = {
      eventID: 'WatchDistributedServiceCard',
      objType: 'ClusterAutoMsgDistributedServiceCardWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterAutoMsgDistributedServiceCardWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch DSCProfile objects. Supports WebSockets or HTTP long poll */
  public WatchDSCProfile(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterAutoMsgDSCProfileWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/watch/dscprofiles';
    const opts = {
      eventID: 'WatchDSCProfile',
      objType: 'ClusterAutoMsgDSCProfileWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterAutoMsgDSCProfileWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch Host objects. Supports WebSockets or HTTP long poll */
  public WatchHost(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterAutoMsgHostWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/watch/hosts';
    const opts = {
      eventID: 'WatchHost',
      objType: 'ClusterAutoMsgHostWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterAutoMsgHostWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch Node objects. Supports WebSockets or HTTP long poll */
  public WatchNode(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterAutoMsgNodeWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/watch/nodes';
    const opts = {
      eventID: 'WatchNode',
      objType: 'ClusterAutoMsgNodeWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterAutoMsgNodeWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch Tenant objects. Supports WebSockets or HTTP long poll */
  public WatchTenant(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterAutoMsgTenantWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/watch/tenants';
    const opts = {
      eventID: 'WatchTenant',
      objType: 'ClusterAutoMsgTenantWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterAutoMsgTenantWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch Version objects. Supports WebSockets or HTTP long poll */
  public WatchVersion(queryParam: any = null, stagingID: string = ""):Observable<{body: IClusterAutoMsgVersionWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/cluster/v1/watch/version';
    const opts = {
      eventID: 'WatchVersion',
      objType: 'ClusterAutoMsgVersionWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: IClusterAutoMsgVersionWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  protected createListDistributedServiceCardCache(): Observable<ServerEvent<ClusterDistributedServiceCard>> {
    return this.createDataCache<ClusterDistributedServiceCard>(ClusterDistributedServiceCard, `ClusterDistributedServiceCard`, () => this.ListDistributedServiceCard(), (body: any) => this.WatchDistributedServiceCard(body), this.bufferDelayMap);
  }

  public ListDistributedServiceCardCache(): Observable<ServerEvent<ClusterDistributedServiceCard>> {
    return this.getFromDataCache(`ClusterDistributedServiceCard`, () => { return this.createListDistributedServiceCardCache() });
  }
  
  protected createListDSCProfileCache(): Observable<ServerEvent<ClusterDSCProfile>> {
    return this.createDataCache<ClusterDSCProfile>(ClusterDSCProfile, `ClusterDSCProfile`, () => this.ListDSCProfile(), (body: any) => this.WatchDSCProfile(body), this.bufferDelayMap);
  }

  public ListDSCProfileCache(): Observable<ServerEvent<ClusterDSCProfile>> {
    return this.getFromDataCache(`ClusterDSCProfile`, () => { return this.createListDSCProfileCache() });
  }
  
  protected createListHostCache(): Observable<ServerEvent<ClusterHost>> {
    return this.createDataCache<ClusterHost>(ClusterHost, `ClusterHost`, () => this.ListHost(), (body: any) => this.WatchHost(body), this.bufferDelayMap);
  }

  public ListHostCache(): Observable<ServerEvent<ClusterHost>> {
    return this.getFromDataCache(`ClusterHost`, () => { return this.createListHostCache() });
  }
  
  protected createListNodeCache(): Observable<ServerEvent<ClusterNode>> {
    return this.createDataCache<ClusterNode>(ClusterNode, `ClusterNode`, () => this.ListNode(), (body: any) => this.WatchNode(body), this.bufferDelayMap);
  }

  public ListNodeCache(): Observable<ServerEvent<ClusterNode>> {
    return this.getFromDataCache(`ClusterNode`, () => { return this.createListNodeCache() });
  }
  
  protected createListTenantCache(): Observable<ServerEvent<ClusterTenant>> {
    return this.createDataCache<ClusterTenant>(ClusterTenant, `ClusterTenant`, () => this.ListTenant(), (body: any) => this.WatchTenant(body), this.bufferDelayMap);
  }

  public ListTenantCache(): Observable<ServerEvent<ClusterTenant>> {
    return this.getFromDataCache(`ClusterTenant`, () => { return this.createListTenantCache() });
  }
  
}