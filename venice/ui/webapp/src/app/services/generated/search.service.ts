import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { VeniceResponse } from '@app/models/frontend/shared/veniceresponse.interface';
import { ControllerService } from '@app/services/controller.service';
import { Searchv1Service } from '@sdk/v1/services/generated/searchv1.service';
import { Observable } from 'rxjs';
import { environment } from '../../../environments/environment';
import { Utility } from '../../common/Utility';
import { GenServiceUtility } from './GenUtility';
import { UIConfigsService } from '../uiconfigs.service';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { never } from 'rxjs';
import { MethodOpts } from '@sdk/v1/services/generated/abstract.service';

@Injectable()
export class SearchService extends Searchv1Service {
  // Attributes used by generated services
  protected O_Tenant: string = this.getTenant();
  protected baseUrlAndPort = window.location.protocol + '//' + window.location.hostname + ':' + window.location.port;
  protected oboeServiceMap: { [method: string]: Observable<VeniceResponse> } = {};
  protected serviceUtility: GenServiceUtility;

  constructor(protected _http: HttpClient,
              protected _controllerService: ControllerService,
              protected uiconfigsService: UIConfigsService) {
      super(_http);
      this.serviceUtility = new GenServiceUtility(
        _http,
        (payload) => { this.publishAJAXStart(payload); },
        (payload) => { this.publishAJAXEnd(payload); }
      );
  }

  /**
   * Get the service class-name
   */
  getClassName(): string {
    return this.constructor.name;
  }

  protected publishAJAXStart(eventPayload: any) {
    this._controllerService.publish(Eventtypes.AJAX_START, eventPayload);
  }

  protected publishAJAXEnd(eventPayload: any) {
    this._controllerService.publish(Eventtypes.AJAX_END, eventPayload);
  }

  protected invokeAJAX(method: string, url: string, payload: any, opts: MethodOpts, forceReal: boolean = false): Observable<VeniceResponse> {

    const key = this.convertEventID(opts);
    if (!this.uiconfigsService.isAuthorized(key)) {
      return never();
    }
    const isOnline = !this.isToMockData() || forceReal;
    return this.serviceUtility.invokeAJAX(method, url, payload, opts.eventID, isOnline);
  }

  convertEventID(opts: MethodOpts): UIRolePermissions {
    let key:string;
    if (opts.eventID.includes('Policy')) {
      key = 'securitysgpolicy' + '_' + 'read';
    } else {
      key = 'search' + '_' + 'read';
    }
    // All event operations are reads, even posts.
    return UIRolePermissions[key]
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
