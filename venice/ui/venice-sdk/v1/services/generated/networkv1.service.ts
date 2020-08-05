import { AbstractService, ServerEvent } from './abstract.service';
import { HttpClient } from '../../../../webapp/node_modules/@angular/common/http';
import { Observable } from '../../../../webapp/node_modules/rxjs';
import { Injectable } from '../../../../webapp/node_modules/@angular/core';
import { TrimDefaultsAndEmptyFields, TrimUIFields } from '../../../v1/utils/utility';

import { INetworkIPAMPolicyList,NetworkIPAMPolicyList,IApiStatus,ApiStatus,INetworkIPAMPolicy,NetworkIPAMPolicy,ApiLabel,IApiLabel,INetworkNetworkInterfaceList,NetworkNetworkInterfaceList,INetworkNetworkInterface,NetworkNetworkInterface,INetworkNetworkList,NetworkNetworkList,INetworkNetwork,NetworkNetwork,INetworkRouteTableList,NetworkRouteTableList,INetworkRouteTable,NetworkRouteTable,INetworkRoutingConfigList,NetworkRoutingConfigList,INetworkRoutingConfig,NetworkRoutingConfig,INetworkVirtualRouterPeeringGroupList,NetworkVirtualRouterPeeringGroupList,INetworkVirtualRouterPeeringGroup,NetworkVirtualRouterPeeringGroup,INetworkVirtualRouterList,NetworkVirtualRouterList,INetworkVirtualRouter,NetworkVirtualRouter,INetworkAutoMsgIPAMPolicyWatchHelper,NetworkAutoMsgIPAMPolicyWatchHelper,INetworkAutoMsgNetworkInterfaceWatchHelper,NetworkAutoMsgNetworkInterfaceWatchHelper,INetworkAutoMsgNetworkWatchHelper,NetworkAutoMsgNetworkWatchHelper,INetworkAutoMsgRouteTableWatchHelper,NetworkAutoMsgRouteTableWatchHelper,INetworkAutoMsgRoutingConfigWatchHelper,NetworkAutoMsgRoutingConfigWatchHelper,INetworkAutoMsgVirtualRouterPeeringGroupWatchHelper,NetworkAutoMsgVirtualRouterPeeringGroupWatchHelper,INetworkAutoMsgVirtualRouterWatchHelper,NetworkAutoMsgVirtualRouterWatchHelper } from '../../models/generated/network';

@Injectable()
export class Networkv1Service extends AbstractService {
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
    'NetworkNetworkInterface': 100,
    'NetworkRoutingConfig': 100,
    'NetworkIPAMPolicy': 100,
    'NetworkNetwork': 100,
    'NetworkRouteTable': 100,
    'NetworkVirtualRouterPeeringGroup': 100,
    'NetworkVirtualRouter': 100,
  }

  /** List IPAMPolicy objects */
  public ListIPAMPolicy_1(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkIPAMPolicyList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/ipam-policies';
    const opts = {
      eventID: 'ListIPAMPolicy_1',
      objType: 'NetworkIPAMPolicyList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkIPAMPolicyList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create IPAMPolicy object */
  public AddIPAMPolicy_1(body: INetworkIPAMPolicy, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/ipam-policies';
    const opts = {
      eventID: 'AddIPAMPolicy_1',
      objType: 'NetworkIPAMPolicy',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkIPAMPolicy(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get IPAMPolicy object */
  public GetIPAMPolicy_1(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/ipam-policies/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetIPAMPolicy_1',
      objType: 'NetworkIPAMPolicy',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete IPAMPolicy object */
  public DeleteIPAMPolicy_1(O_Name, stagingID: string = ""):Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/ipam-policies/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteIPAMPolicy_1',
      objType: 'NetworkIPAMPolicy',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update IPAMPolicy object */
  public UpdateIPAMPolicy_1(O_Name, body: INetworkIPAMPolicy, stagingID: string = "", previousVal: INetworkIPAMPolicy = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/ipam-policies/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateIPAMPolicy_1',
      objType: 'NetworkIPAMPolicy',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkIPAMPolicy(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label IPAMPolicy object */
  public LabelIPAMPolicy_1(O_Name, body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/ipam-policies/{O.Name}/label';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'LabelIPAMPolicy_1',
      objType: 'NetworkIPAMPolicy',
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
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List NetworkInterface objects */
  public ListNetworkInterface(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkNetworkInterfaceList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/networkinterfaces';
    const opts = {
      eventID: 'ListNetworkInterface',
      objType: 'NetworkNetworkInterfaceList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkNetworkInterfaceList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get NetworkInterface object */
  public GetNetworkInterface(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkNetworkInterface | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/networkinterfaces/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetNetworkInterface',
      objType: 'NetworkNetworkInterface',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkNetworkInterface | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update NetworkInterface object */
  public UpdateNetworkInterface(O_Name, body: INetworkNetworkInterface, stagingID: string = "", previousVal: INetworkNetworkInterface = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkNetworkInterface | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/networkinterfaces/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateNetworkInterface',
      objType: 'NetworkNetworkInterface',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkNetworkInterface(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: INetworkNetworkInterface | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label NetworkInterface object */
  public LabelNetworkInterface(O_Name, body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkNetworkInterface | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/networkinterfaces/{O.Name}/label';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'LabelNetworkInterface',
      objType: 'NetworkNetworkInterface',
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
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkNetworkInterface | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List Network objects */
  public ListNetwork_1(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkNetworkList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/networks';
    const opts = {
      eventID: 'ListNetwork_1',
      objType: 'NetworkNetworkList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkNetworkList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create Network object */
  public AddNetwork_1(body: INetworkNetwork, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/networks';
    const opts = {
      eventID: 'AddNetwork_1',
      objType: 'NetworkNetwork',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkNetwork(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get Network object */
  public GetNetwork_1(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/networks/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetNetwork_1',
      objType: 'NetworkNetwork',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete Network object */
  public DeleteNetwork_1(O_Name, stagingID: string = ""):Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/networks/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteNetwork_1',
      objType: 'NetworkNetwork',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update Network object */
  public UpdateNetwork_1(O_Name, body: INetworkNetwork, stagingID: string = "", previousVal: INetworkNetwork = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/networks/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateNetwork_1',
      objType: 'NetworkNetwork',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkNetwork(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label Network object */
  public LabelNetwork_1(O_Name, body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/networks/{O.Name}/label';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'LabelNetwork_1',
      objType: 'NetworkNetwork',
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
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List RouteTable objects */
  public ListRouteTable_1(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkRouteTableList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/route-tables';
    const opts = {
      eventID: 'ListRouteTable_1',
      objType: 'NetworkRouteTableList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkRouteTableList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get RouteTable object */
  public GetRouteTable_1(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkRouteTable | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/route-tables/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetRouteTable_1',
      objType: 'NetworkRouteTable',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkRouteTable | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List RoutingConfig objects */
  public ListRoutingConfig(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkRoutingConfigList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/routing-config';
    const opts = {
      eventID: 'ListRoutingConfig',
      objType: 'NetworkRoutingConfigList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkRoutingConfigList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create RoutingConfig object */
  public AddRoutingConfig(body: INetworkRoutingConfig, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkRoutingConfig | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/routing-config';
    const opts = {
      eventID: 'AddRoutingConfig',
      objType: 'NetworkRoutingConfig',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkRoutingConfig(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkRoutingConfig | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get RoutingConfig object */
  public GetRoutingConfig(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkRoutingConfig | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/routing-config/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetRoutingConfig',
      objType: 'NetworkRoutingConfig',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkRoutingConfig | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete RoutingConfig object */
  public DeleteRoutingConfig(O_Name, stagingID: string = ""):Observable<{body: INetworkRoutingConfig | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/routing-config/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteRoutingConfig',
      objType: 'NetworkRoutingConfig',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: INetworkRoutingConfig | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update RoutingConfig object */
  public UpdateRoutingConfig(O_Name, body: INetworkRoutingConfig, stagingID: string = "", previousVal: INetworkRoutingConfig = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkRoutingConfig | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/routing-config/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateRoutingConfig',
      objType: 'NetworkRoutingConfig',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkRoutingConfig(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: INetworkRoutingConfig | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label RoutingConfig object */
  public LabelRoutingConfig(O_Name, body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkRoutingConfig | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/routing-config/{O.Name}/label';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'LabelRoutingConfig',
      objType: 'NetworkRoutingConfig',
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
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkRoutingConfig | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List IPAMPolicy objects */
  public ListIPAMPolicy(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkIPAMPolicyList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/ipam-policies';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'ListIPAMPolicy',
      objType: 'NetworkIPAMPolicyList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkIPAMPolicyList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create IPAMPolicy object */
  public AddIPAMPolicy(body: INetworkIPAMPolicy, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/ipam-policies';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'AddIPAMPolicy',
      objType: 'NetworkIPAMPolicy',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkIPAMPolicy(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get IPAMPolicy object */
  public GetIPAMPolicy(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/ipam-policies/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetIPAMPolicy',
      objType: 'NetworkIPAMPolicy',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete IPAMPolicy object */
  public DeleteIPAMPolicy(O_Name, stagingID: string = ""):Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/ipam-policies/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteIPAMPolicy',
      objType: 'NetworkIPAMPolicy',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update IPAMPolicy object */
  public UpdateIPAMPolicy(O_Name, body: INetworkIPAMPolicy, stagingID: string = "", previousVal: INetworkIPAMPolicy = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/ipam-policies/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateIPAMPolicy',
      objType: 'NetworkIPAMPolicy',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkIPAMPolicy(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label IPAMPolicy object */
  public LabelIPAMPolicy(O_Name, body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/ipam-policies/{O.Name}/label';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'LabelIPAMPolicy',
      objType: 'NetworkIPAMPolicy',
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
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkIPAMPolicy | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List Network objects */
  public ListNetwork(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkNetworkList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/networks';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'ListNetwork',
      objType: 'NetworkNetworkList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkNetworkList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create Network object */
  public AddNetwork(body: INetworkNetwork, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/networks';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'AddNetwork',
      objType: 'NetworkNetwork',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkNetwork(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get Network object */
  public GetNetwork(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/networks/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetNetwork',
      objType: 'NetworkNetwork',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete Network object */
  public DeleteNetwork(O_Name, stagingID: string = ""):Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/networks/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteNetwork',
      objType: 'NetworkNetwork',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update Network object */
  public UpdateNetwork(O_Name, body: INetworkNetwork, stagingID: string = "", previousVal: INetworkNetwork = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/networks/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateNetwork',
      objType: 'NetworkNetwork',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkNetwork(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label Network object */
  public LabelNetwork(O_Name, body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/networks/{O.Name}/label';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'LabelNetwork',
      objType: 'NetworkNetwork',
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
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkNetwork | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List RouteTable objects */
  public ListRouteTable(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkRouteTableList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/route-tables';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'ListRouteTable',
      objType: 'NetworkRouteTableList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkRouteTableList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get RouteTable object */
  public GetRouteTable(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkRouteTable | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/route-tables/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetRouteTable',
      objType: 'NetworkRouteTable',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkRouteTable | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List VirtualRouterPeeringGroup objects */
  public ListVirtualRouterPeeringGroup(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkVirtualRouterPeeringGroupList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/virtual-router-peering-groups';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'ListVirtualRouterPeeringGroup',
      objType: 'NetworkVirtualRouterPeeringGroupList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkVirtualRouterPeeringGroupList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create VirtualRouterPeeringGroup object */
  public AddVirtualRouterPeeringGroup(body: INetworkVirtualRouterPeeringGroup, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/virtual-router-peering-groups';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'AddVirtualRouterPeeringGroup',
      objType: 'NetworkVirtualRouterPeeringGroup',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkVirtualRouterPeeringGroup(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get VirtualRouterPeeringGroup object */
  public GetVirtualRouterPeeringGroup(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/virtual-router-peering-groups/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetVirtualRouterPeeringGroup',
      objType: 'NetworkVirtualRouterPeeringGroup',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete VirtualRouterPeeringGroup object */
  public DeleteVirtualRouterPeeringGroup(O_Name, stagingID: string = ""):Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/virtual-router-peering-groups/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteVirtualRouterPeeringGroup',
      objType: 'NetworkVirtualRouterPeeringGroup',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update VirtualRouterPeeringGroup object */
  public UpdateVirtualRouterPeeringGroup(O_Name, body: INetworkVirtualRouterPeeringGroup, stagingID: string = "", previousVal: INetworkVirtualRouterPeeringGroup = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/virtual-router-peering-groups/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateVirtualRouterPeeringGroup',
      objType: 'NetworkVirtualRouterPeeringGroup',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkVirtualRouterPeeringGroup(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label VirtualRouterPeeringGroup object */
  public LabelVirtualRouterPeeringGroup(O_Name, body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/virtual-router-peering-groups/{O.Name}/label';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'LabelVirtualRouterPeeringGroup',
      objType: 'NetworkVirtualRouterPeeringGroup',
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
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List VirtualRouter objects */
  public ListVirtualRouter(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkVirtualRouterList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/virtualrouters';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'ListVirtualRouter',
      objType: 'NetworkVirtualRouterList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkVirtualRouterList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create VirtualRouter object */
  public AddVirtualRouter(body: INetworkVirtualRouter, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/virtualrouters';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'AddVirtualRouter',
      objType: 'NetworkVirtualRouter',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkVirtualRouter(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get VirtualRouter object */
  public GetVirtualRouter(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/virtualrouters/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetVirtualRouter',
      objType: 'NetworkVirtualRouter',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete VirtualRouter object */
  public DeleteVirtualRouter(O_Name, stagingID: string = ""):Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/virtualrouters/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteVirtualRouter',
      objType: 'NetworkVirtualRouter',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update VirtualRouter object */
  public UpdateVirtualRouter(O_Name, body: INetworkVirtualRouter, stagingID: string = "", previousVal: INetworkVirtualRouter = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/virtualrouters/{O.Name}';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateVirtualRouter',
      objType: 'NetworkVirtualRouter',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkVirtualRouter(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label VirtualRouter object */
  public LabelVirtualRouter(O_Name, body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/tenant/{O.Tenant}/virtualrouters/{O.Name}/label';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'LabelVirtualRouter',
      objType: 'NetworkVirtualRouter',
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
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List VirtualRouterPeeringGroup objects */
  public ListVirtualRouterPeeringGroup_1(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkVirtualRouterPeeringGroupList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/virtual-router-peering-groups';
    const opts = {
      eventID: 'ListVirtualRouterPeeringGroup_1',
      objType: 'NetworkVirtualRouterPeeringGroupList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkVirtualRouterPeeringGroupList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create VirtualRouterPeeringGroup object */
  public AddVirtualRouterPeeringGroup_1(body: INetworkVirtualRouterPeeringGroup, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/virtual-router-peering-groups';
    const opts = {
      eventID: 'AddVirtualRouterPeeringGroup_1',
      objType: 'NetworkVirtualRouterPeeringGroup',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkVirtualRouterPeeringGroup(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get VirtualRouterPeeringGroup object */
  public GetVirtualRouterPeeringGroup_1(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/virtual-router-peering-groups/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetVirtualRouterPeeringGroup_1',
      objType: 'NetworkVirtualRouterPeeringGroup',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete VirtualRouterPeeringGroup object */
  public DeleteVirtualRouterPeeringGroup_1(O_Name, stagingID: string = ""):Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/virtual-router-peering-groups/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteVirtualRouterPeeringGroup_1',
      objType: 'NetworkVirtualRouterPeeringGroup',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update VirtualRouterPeeringGroup object */
  public UpdateVirtualRouterPeeringGroup_1(O_Name, body: INetworkVirtualRouterPeeringGroup, stagingID: string = "", previousVal: INetworkVirtualRouterPeeringGroup = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/virtual-router-peering-groups/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateVirtualRouterPeeringGroup_1',
      objType: 'NetworkVirtualRouterPeeringGroup',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkVirtualRouterPeeringGroup(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label VirtualRouterPeeringGroup object */
  public LabelVirtualRouterPeeringGroup_1(O_Name, body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/virtual-router-peering-groups/{O.Name}/label';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'LabelVirtualRouterPeeringGroup_1',
      objType: 'NetworkVirtualRouterPeeringGroup',
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
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkVirtualRouterPeeringGroup | IApiStatus | Error, statusCode: number}>;
  }
  
  /** List VirtualRouter objects */
  public ListVirtualRouter_1(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkVirtualRouterList | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/virtualrouters';
    const opts = {
      eventID: 'ListVirtualRouter_1',
      objType: 'NetworkVirtualRouterList',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkVirtualRouterList | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Create VirtualRouter object */
  public AddVirtualRouter_1(body: INetworkVirtualRouter, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/virtualrouters';
    const opts = {
      eventID: 'AddVirtualRouter_1',
      objType: 'NetworkVirtualRouter',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkVirtualRouter(body), null, trimDefaults)
    }
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Get VirtualRouter object */
  public GetVirtualRouter_1(O_Name, queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/virtualrouters/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'GetVirtualRouter_1',
      objType: 'NetworkVirtualRouter',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Delete VirtualRouter object */
  public DeleteVirtualRouter_1(O_Name, stagingID: string = ""):Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/virtualrouters/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'DeleteVirtualRouter_1',
      objType: 'NetworkVirtualRouter',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXDeleteCall(url, opts) as Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Update VirtualRouter object */
  public UpdateVirtualRouter_1(O_Name, body: INetworkVirtualRouter, stagingID: string = "", previousVal: INetworkVirtualRouter = null, trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/virtualrouters/{O.Name}';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'UpdateVirtualRouter_1',
      objType: 'NetworkVirtualRouter',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    body = TrimUIFields(body)
    if (trimObject) {
      body = TrimDefaultsAndEmptyFields(body, new NetworkVirtualRouter(body), previousVal, trimDefaults)
    }
    return this.invokeAJAXPutCall(url, body, opts) as Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Label VirtualRouter object */
  public LabelVirtualRouter_1(O_Name, body: IApiLabel, stagingID: string = "", trimObject: boolean = true, trimDefaults: boolean = true):Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/virtualrouters/{O.Name}/label';
    url = url.replace('{O.Name}', O_Name);
    const opts = {
      eventID: 'LabelVirtualRouter_1',
      objType: 'NetworkVirtualRouter',
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
    return this.invokeAJAXPostCall(url, body, opts) as Observable<{body: INetworkVirtualRouter | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch IPAMPolicy objects. Supports WebSockets or HTTP long poll */
  public WatchIPAMPolicy_1(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkAutoMsgIPAMPolicyWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/watch/ipam-policies';
    const opts = {
      eventID: 'WatchIPAMPolicy_1',
      objType: 'NetworkAutoMsgIPAMPolicyWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkAutoMsgIPAMPolicyWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch NetworkInterface objects. Supports WebSockets or HTTP long poll */
  public WatchNetworkInterface(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkAutoMsgNetworkInterfaceWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/watch/networkinterfaces';
    const opts = {
      eventID: 'WatchNetworkInterface',
      objType: 'NetworkAutoMsgNetworkInterfaceWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkAutoMsgNetworkInterfaceWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch Network objects. Supports WebSockets or HTTP long poll */
  public WatchNetwork_1(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkAutoMsgNetworkWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/watch/networks';
    const opts = {
      eventID: 'WatchNetwork_1',
      objType: 'NetworkAutoMsgNetworkWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkAutoMsgNetworkWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch RouteTable objects. Supports WebSockets or HTTP long poll */
  public WatchRouteTable_1(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkAutoMsgRouteTableWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/watch/route-tables';
    const opts = {
      eventID: 'WatchRouteTable_1',
      objType: 'NetworkAutoMsgRouteTableWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkAutoMsgRouteTableWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch RoutingConfig objects. Supports WebSockets or HTTP long poll */
  public WatchRoutingConfig(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkAutoMsgRoutingConfigWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/watch/routing-config';
    const opts = {
      eventID: 'WatchRoutingConfig',
      objType: 'NetworkAutoMsgRoutingConfigWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkAutoMsgRoutingConfigWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch IPAMPolicy objects. Supports WebSockets or HTTP long poll */
  public WatchIPAMPolicy(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkAutoMsgIPAMPolicyWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/watch/tenant/{O.Tenant}/ipam-policies';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'WatchIPAMPolicy',
      objType: 'NetworkAutoMsgIPAMPolicyWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkAutoMsgIPAMPolicyWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch Network objects. Supports WebSockets or HTTP long poll */
  public WatchNetwork(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkAutoMsgNetworkWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/watch/tenant/{O.Tenant}/networks';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'WatchNetwork',
      objType: 'NetworkAutoMsgNetworkWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkAutoMsgNetworkWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch RouteTable objects. Supports WebSockets or HTTP long poll */
  public WatchRouteTable(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkAutoMsgRouteTableWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/watch/tenant/{O.Tenant}/route-tables';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'WatchRouteTable',
      objType: 'NetworkAutoMsgRouteTableWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkAutoMsgRouteTableWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch VirtualRouterPeeringGroup objects. Supports WebSockets or HTTP long poll */
  public WatchVirtualRouterPeeringGroup(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkAutoMsgVirtualRouterPeeringGroupWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/watch/tenant/{O.Tenant}/virtual-router-peering-groups';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'WatchVirtualRouterPeeringGroup',
      objType: 'NetworkAutoMsgVirtualRouterPeeringGroupWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkAutoMsgVirtualRouterPeeringGroupWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch VirtualRouter objects. Supports WebSockets or HTTP long poll */
  public WatchVirtualRouter(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkAutoMsgVirtualRouterWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/watch/tenant/{O.Tenant}/virtualrouters';
    url = url.replace('{O.Tenant}', this['O_Tenant']);
    const opts = {
      eventID: 'WatchVirtualRouter',
      objType: 'NetworkAutoMsgVirtualRouterWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkAutoMsgVirtualRouterWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch VirtualRouterPeeringGroup objects. Supports WebSockets or HTTP long poll */
  public WatchVirtualRouterPeeringGroup_1(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkAutoMsgVirtualRouterPeeringGroupWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/watch/virtual-router-peering-groups';
    const opts = {
      eventID: 'WatchVirtualRouterPeeringGroup_1',
      objType: 'NetworkAutoMsgVirtualRouterPeeringGroupWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkAutoMsgVirtualRouterPeeringGroupWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  /** Watch VirtualRouter objects. Supports WebSockets or HTTP long poll */
  public WatchVirtualRouter_1(queryParam: any = null, stagingID: string = ""):Observable<{body: INetworkAutoMsgVirtualRouterWatchHelper | IApiStatus | Error, statusCode: number}> {
    let url = this['baseUrlAndPort'] + '/configs/network/v1/watch/virtualrouters';
    const opts = {
      eventID: 'WatchVirtualRouter_1',
      objType: 'NetworkAutoMsgVirtualRouterWatchHelper',
      isStaging: false,
    }
    if (stagingID != null && stagingID.length != 0) {
      url = url.replace('configs', 'staging/' + stagingID);
      opts.isStaging = true;
    }
    return this.invokeAJAXGetCall(url, queryParam, opts) as Observable<{body: INetworkAutoMsgVirtualRouterWatchHelper | IApiStatus | Error, statusCode: number}>;
  }
  
  protected createListNetworkInterfaceCache(): Observable<ServerEvent<NetworkNetworkInterface>> {
    return this.createDataCache<NetworkNetworkInterface>(NetworkNetworkInterface, `NetworkNetworkInterface`, () => this.ListNetworkInterface(), (body: any) => this.WatchNetworkInterface(body), this.bufferDelayMap);
  }

  public ListNetworkInterfaceCache(): Observable<ServerEvent<NetworkNetworkInterface>> {
    return this.getFromDataCache(`NetworkNetworkInterface`, () => { return this.createListNetworkInterfaceCache() });
  }
  
  protected createListRoutingConfigCache(): Observable<ServerEvent<NetworkRoutingConfig>> {
    return this.createDataCache<NetworkRoutingConfig>(NetworkRoutingConfig, `NetworkRoutingConfig`, () => this.ListRoutingConfig(), (body: any) => this.WatchRoutingConfig(body), this.bufferDelayMap);
  }

  public ListRoutingConfigCache(): Observable<ServerEvent<NetworkRoutingConfig>> {
    return this.getFromDataCache(`NetworkRoutingConfig`, () => { return this.createListRoutingConfigCache() });
  }
  
  protected createListIPAMPolicyCache(): Observable<ServerEvent<NetworkIPAMPolicy>> {
    return this.createDataCache<NetworkIPAMPolicy>(NetworkIPAMPolicy, `NetworkIPAMPolicy`, () => this.ListIPAMPolicy(), (body: any) => this.WatchIPAMPolicy(body), this.bufferDelayMap);
  }

  public ListIPAMPolicyCache(): Observable<ServerEvent<NetworkIPAMPolicy>> {
    return this.getFromDataCache(`NetworkIPAMPolicy`, () => { return this.createListIPAMPolicyCache() });
  }
  
  protected createListNetworkCache(): Observable<ServerEvent<NetworkNetwork>> {
    return this.createDataCache<NetworkNetwork>(NetworkNetwork, `NetworkNetwork`, () => this.ListNetwork(), (body: any) => this.WatchNetwork(body), this.bufferDelayMap);
  }

  public ListNetworkCache(): Observable<ServerEvent<NetworkNetwork>> {
    return this.getFromDataCache(`NetworkNetwork`, () => { return this.createListNetworkCache() });
  }
  
  protected createListRouteTableCache(): Observable<ServerEvent<NetworkRouteTable>> {
    return this.createDataCache<NetworkRouteTable>(NetworkRouteTable, `NetworkRouteTable`, () => this.ListRouteTable(), (body: any) => this.WatchRouteTable(body), this.bufferDelayMap);
  }

  public ListRouteTableCache(): Observable<ServerEvent<NetworkRouteTable>> {
    return this.getFromDataCache(`NetworkRouteTable`, () => { return this.createListRouteTableCache() });
  }
  
  protected createListVirtualRouterPeeringGroupCache(): Observable<ServerEvent<NetworkVirtualRouterPeeringGroup>> {
    return this.createDataCache<NetworkVirtualRouterPeeringGroup>(NetworkVirtualRouterPeeringGroup, `NetworkVirtualRouterPeeringGroup`, () => this.ListVirtualRouterPeeringGroup(), (body: any) => this.WatchVirtualRouterPeeringGroup(body), this.bufferDelayMap);
  }

  public ListVirtualRouterPeeringGroupCache(): Observable<ServerEvent<NetworkVirtualRouterPeeringGroup>> {
    return this.getFromDataCache(`NetworkVirtualRouterPeeringGroup`, () => { return this.createListVirtualRouterPeeringGroupCache() });
  }
  
  protected createListVirtualRouterCache(): Observable<ServerEvent<NetworkVirtualRouter>> {
    return this.createDataCache<NetworkVirtualRouter>(NetworkVirtualRouter, `NetworkVirtualRouter`, () => this.ListVirtualRouter(), (body: any) => this.WatchVirtualRouter(body), this.bufferDelayMap);
  }

  public ListVirtualRouterCache(): Observable<ServerEvent<NetworkVirtualRouter>> {
    return this.getFromDataCache(`NetworkVirtualRouter`, () => { return this.createListVirtualRouterCache() });
  }
  
}