import { HttpClient } from '@angular/common/http';
import { Injectable, OnDestroy } from '@angular/core';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { VeniceResponse } from '@app/models/frontend/shared/veniceresponse.interface';
import { ControllerService } from '@app/services/controller.service';
import { Diagnosticsv1Service } from '@sdk/v1/services/generated/diagnosticsv1.service';
import { Observable, Subscription } from 'rxjs';
import { environment } from '../../../environments/environment';
import { Utility } from '../../common/Utility';
import { GenServiceUtility } from './GenUtility';
import { UIConfigsService } from '../uiconfigs.service';
import { NEVER } from 'rxjs';
import { MethodOpts } from '@sdk/v1/services/generated/abstract.service';

@Injectable()
export class DiagnosticsService extends Diagnosticsv1Service implements OnDestroy {
  // Attributes used by generated services
  protected O_Tenant: string = this.getTenant();
  protected baseUrlAndPort = window.location.protocol + '//' + window.location.hostname + ':' + window.location.port;
  protected oboeServiceMap: { [method: string]: Observable<VeniceResponse> } = {};
  protected serviceUtility: GenServiceUtility;
  protected subscriptions: Subscription[] = [];

  constructor(protected _http: HttpClient,
              protected _controllerService: ControllerService,
              protected uiconfigsService: UIConfigsService) {
      super(_http);
      this.serviceUtility = new GenServiceUtility(
        _http,
        (payload) => { this.publishAJAXStart(payload); },
        (payload) => { this.publishAJAXEnd(payload); }
      );
      this.serviceUtility.setId(this.getClassName());
  }

  ngOnDestroy() {
    this.subscriptions.forEach(s => {
      s.unsubscribe();
    });
  }

  /**
   * Get the service class-name
   */
  getClassName(): string {
    return this.constructor.name;
  }

  protected createDataCache<T>(constructor: any, key: string, listFn: () => Observable<VeniceResponse>, watchFn: (query: any) => Observable<VeniceResponse>, bufferDelayMap: { [key: string]: number } = {}) {
    return this.serviceUtility.createDataCache(constructor, key, listFn, watchFn, bufferDelayMap);
  }

  protected getFromDataCache(kind: string, createCacheFn: any) {
    return this.serviceUtility.handleListFromCache(kind, createCacheFn);
  }

  protected publishAJAXStart(eventPayload: any) {
    this._controllerService.publish(Eventtypes.AJAX_START, eventPayload);
  }

  protected publishAJAXEnd(eventPayload: any) {
    this._controllerService.publish(Eventtypes.AJAX_END, eventPayload);
  }

  protected invokeAJAX(method: string, url: string, payload: any, opts: MethodOpts, forceReal: boolean = false): Observable<VeniceResponse> {

    const key = this.serviceUtility.convertEventID(opts);
    // VS-766 backend grant everyone staging buffer permission. UI will not check permission
    const isOnline = !this.isToMockData() || forceReal;
    return this.serviceUtility.invokeAJAX(method, url, payload, opts.eventID, isOnline);
  }

  /**
   * Override-able api
   */
  public isToMockData(): boolean {
    const isUseRealData = this._controllerService.useRealData;
    return (!isUseRealData) ? isUseRealData : environment.isRESTAPIReady;
  }

  /**
   * Get login user tenant information
   */
  getTenant(): string {
    return Utility.getInstance().getTenant();
  }
}
