import { Component, OnDestroy, OnInit, ViewEncapsulation } from '@angular/core';
import { CommonComponent } from '@app/common.component';
import * as authActions from '@app/core';
import { Store } from '@ngrx/store';
import { Utility } from '../../common/Utility';
import { Eventtypes } from '../../enum/eventtypes.enum';
import { AuthService } from '../../services/auth.service';
import { ControllerService } from '../../services/controller.service';
import { Router } from '@angular/router';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { LocalStorageService } from '@app/core';

@Component({
  selector: 'app-login',
  templateUrl: './login.component.html',
  styleUrls: ['./login.component.scss'],

  encapsulation: ViewEncapsulation.None
})
export class LoginComponent extends CommonComponent implements OnInit, OnDestroy {
  credentials = { username: '', password: '' };
  successMessage = '';
  errorMessage = '';
  returnUrl: string;
  // Used to ensure we are only sending one request at a
  // time. Otherwise the key we store for the user will be outdated
  // due to the second request.
  loginInProgress: boolean = false;

  docLink: string = Utility.getDocURL();

  constructor(
    private _authService: AuthService,
    private _controllerService: ControllerService,
    private uiconfigService: UIConfigsService,
    private router: Router,
    private store$: Store<any>,
    private localStorage: LocalStorageService
  ) {
    super();
  }

  /**
   * Component enters init stage. It is about to show up
   */
  ngOnInit() {
    if (this._controllerService.isUserLogin()) {
      this.redirect();
      return;
    }
    this.subscriptions[Utility.USER_DATA_OBSERVABLE] = this.localStorage.getUserdataObservable().subscribe(
      (data) => {
        if (this._controllerService.isUserLogin()) {
          this.redirect();
          return;
        }
      },
    );
    this._controllerService.publish(Eventtypes.COMPONENT_INIT, { 'component': 'LoginComponent', 'state': Eventtypes.COMPONENT_INIT });

    // setting up subscription
    this.subscriptions[Eventtypes.LOGIN_FAILURE] = this._controllerService.subscribe(Eventtypes.LOGIN_FAILURE, (payload) => {
      this.onLoginFailure(payload);
    });
    this.subscriptions[Eventtypes.LOGIN_SUCCESS] = this._controllerService.subscribe(Eventtypes.LOGIN_SUCCESS, (payload) => {
      this.onLoginSuccess(payload);
    });
  }

  /**
   * Component is about to exit
   */
  ngOnDestroy() {
    // publish event that AppComponent is about to exist
    this._controllerService.publish(Eventtypes.COMPONENT_DESTROY, { 'component': 'LoginComponent', 'state': Eventtypes.COMPONENT_DESTROY });
    this.unsubscribeAll();
  }

  /**
   * This api serves html template
   */
  isLoginInputValided(): boolean {
    return (!Utility.isEmpty(this.credentials.username) && !Utility.isEmpty(this.credentials.password));
  }

  /**
  * This api serves html template
  *
  * We are using anulgar ngRX store/effect. When login, we have the store to dispatch an action
  */
  login(): any {
    if (this.loginInProgress) {
      return;
    }
    this.loginInProgress = true;
    const payload = {
      username: this.credentials.username,
      password: this.credentials.password,
      tenant: Utility.getInstance().getTenant()
    };
    this.store$.dispatch(authActions.login(payload));
  }

  onLoginFailure(errPayload) {
    this.loginInProgress = false;
    this.successMessage = '';
    this.errorMessage = this.getErrorMessage(errPayload);
  }

  onLoginSuccess(payload) {
    // Not setting loginInProgress back to false because we should be getting redirected
    // and this component will be destroyed.
    this.redirect();
  }

  redirect() {
    const redirect = this._authService.redirectUrl;
    if (redirect && this.uiconfigService.canActivateSubRoute(redirect)) {
      this.router.navigateByUrl(redirect);
    } else {
      this.uiconfigService.navigateToHomepage();
    }
  }

  /**
   * Get login error message
   * @param errPayload
   */
  getErrorMessage(errPayload: any): string {
    const msgFailtoLogin = 'Failed to login! ';
    const msgConsultAdmin = 'Please consult system administrator';
    if (!errPayload) {
      return msgFailtoLogin + 'for unknown reason. ' + msgConsultAdmin;
    }
    if (errPayload.message && errPayload.message.status === 0) {
      // This handles case where user is not connected in nework and using the browser cached Venice-UI. Give user-friendly message
      return msgFailtoLogin + ' Please refresh browser and ensure you have network connection';
    } else if (errPayload.message && errPayload.message.status === 401) {
      // handle status =401 authentication failure.
      return msgFailtoLogin + ' Incorrect credentials';
    } else if (! Utility.isEmpty(errPayload.message) && typeof errPayload.message === 'string') {
      return msgFailtoLogin + ' ' + errPayload.message;
    } else if (! Utility.isEmpty(errPayload.message)) {
      return msgFailtoLogin + ' ' + errPayload.message.error;
    }
    // Stringifying the object may be ugly. We console the error, and display a generic Server error message
    console.error('Login component received error: ' + errPayload);
    return msgFailtoLogin + 'Server error. ' + msgConsultAdmin;
  }
}
