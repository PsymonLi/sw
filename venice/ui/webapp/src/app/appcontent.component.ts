import { OverlayContainer } from '@angular/cdk/overlay';
import { TemplatePortalDirective } from '@angular/cdk/portal';
import { AfterViewInit, Component, HostBinding, OnDestroy, OnInit, ViewChild, ViewEncapsulation, ChangeDetectorRef } from '@angular/core';
import { MatDialog } from '@angular/material';
import { MatSidenav, MatSidenavContainer } from '@angular/material/sidenav';
import { NavigationEnd, Router } from '@angular/router';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { BaseComponent } from '@app/components/base/base.component';
import { logout } from '@app/core';
import { AlerttableService } from '@app/services/alerttable.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { RolloutService } from '@app/services/generated/rollout.service';
import { LogService } from '@app/services/logging/log.service';
import { Features, UIConfigsService } from '@app/services/uiconfigs.service';
import { IdleWarningComponent } from '@app/widgets/idlewarning/idlewarning.component';
import { ToolbarComponent } from '@app/widgets/toolbar/toolbar.component';
import { DEFAULT_INTERRUPTSOURCES, Idle } from '@ng-idle/core';
import { Store } from '@ngrx/store';
import { IApiStatus, IAuthUser } from '@sdk/v1/models/generated/auth';
import { ClusterCluster, ClusterLicense, ClusterVersion } from '@sdk/v1/models/generated/cluster';
import { MonitoringAlert } from '@sdk/v1/models/generated/monitoring';
import { RolloutRollout, RolloutRolloutStatus_state } from '@sdk/v1/models/generated/rollout';
import { FieldsRequirement_operator, ISearchSearchResponse, SearchSearchRequest, SearchSearchRequest_mode } from '@sdk/v1/models/generated/search';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import * as moment from 'moment';
import { ConfirmDialog } from 'primeng/primeng';
import { Subject, Subscription, Observable, forkJoin } from 'rxjs';
import { filter, map, takeUntil } from 'rxjs/operators';
import { SideNavItem, sideNavMenu } from './appcontent.sidenav';
import { Utility } from './common/Utility';
import { selectorSettings } from './components/settings-group';
import { Eventtypes } from './enum/eventtypes.enum';
import { ControllerService } from './services/controller.service';
import { AuthService } from './services/generated/auth.service';
import { ClusterService } from './services/generated/cluster.service';
import { SearchService } from './services/generated/search.service';
import { WorkloadService } from './services/generated/workload.service';
import { HelpoverlayComponent } from './widgets/helpcontent/helpoverlay.component';
import { Animations } from '@app/animations';
import { RoutingService } from './services/generated/routing.service';
import { RoutingHealth } from '@sdk/v1/models/generated/routing';
import { DiagnosticsService } from './services/generated/diagnostics.service';

export interface GetUserObjRequest {
  success: (resp: { body: IAuthUser | IApiStatus | Error; statusCode: number; }) => void;
  err: (err: IAuthUser | IApiStatus | Error) => void;
}

const SIDE_MENU_INFO: string = 'SIDE_MENU_INFO';
/**
 * This is the entry point component of Pensando-Venice Web-Application
 */
@Component({
  selector: 'app-content',
  templateUrl: './appcontent.component.html',
  styleUrls: ['./appcontent.component.scss', './appcontent.component.sidenav.scss', './appcontent.component.toast.scss'],
  encapsulation: ViewEncapsulation.None,
  animations: [Animations],
  providers: []
})
export class AppcontentComponent extends BaseComponent implements OnInit, OnDestroy, AfterViewInit {
  os = '';
  browsertype = '';
  browserversion = '';

  docLink: string = Utility.getDocURL();

  _currentComponent: any;

  // is Left-hand-side item click function registered?
  private _boolInitApp = false;

  isSideNavExpanded = true;
  isSideNavShown = true;

  subscriptions: Subscription[] = [];

  rolloutsEventUtility: HttpEventUtility<RolloutRollout>;
  rolloutObjects: ReadonlyArray<RolloutRollout>;
  currentRollout: RolloutRollout = null;
  cluster: ClusterCluster = null;
  clusterLicence: ClusterLicense = null;

  // idling
  showIdleWarning = false;
  idleDialogRef: any;

  // alerts related variables

  alertGetSubscription: Subscription;
  alertWatchSubscription: Subscription;
  alerts: ReadonlyArray<MonitoringAlert> = [];
  startingAlertCount = 0;
  alertNumbers = 0;
  alertHighestSeverity: string;
  alertQuery = {};

  sideNavMenu: SideNavItem[] = sideNavMenu;
  openedMenuItems = {};
  scrollTop: number = 0;
  // debounce event for scroll change
  scrollChanged: Subject<string> = new Subject<string>();

  userName: string = '';

  versionEventUtility: HttpEventUtility<ClusterVersion>;
  versionArray: ReadonlyArray<ClusterVersion> = [];
  version: ClusterVersion = new ClusterVersion();
  versionSubscription: Subscription = null;
  showVersionDialog: boolean = false;

  clock: any; // VS-838 build a UTC clock in Venice-UI
  clocktimer: any = null;
  hourDifference: number | string;

  @HostBinding('class') componentCssClass;
  private unsubscribeStore$: Subject<void> = new Subject<void>();
  @ViewChild('sidenav') _sidenav: MatSidenav;
  @ViewChild('rightSideNav') _rightSideNav: MatSidenav;
  @ViewChild('container') _container: MatSidenavContainer;
  @ViewChild('breadcrumbToolbar') _breadcrumbToolbar: ToolbarComponent;
  @ViewChild('helpOverlay') helpOverlay: HelpoverlayComponent;
  @ViewChild('AppHelp') helpTemplate: TemplatePortalDirective;
  @ViewChild('confirmDialog') confirmDialog: ConfirmDialog;

  protected _rightSideNavIndicator = 'notifications';

  // isScreenBlocked generates an greyed-out overlay over the entire screen, prevents the user clicking anything else.
  // isUINavigationBlocked stops the sidenav and other elements from rendering.
  isScreenBlocked: boolean = false;
  isUINavigationBlocked: boolean = false;

  constructor(
    protected _controllerService: ControllerService,
    protected _logService: LogService,
    protected _alerttableSerivce: AlerttableService,
    public overlayContainer: OverlayContainer,
    private store: Store<any>,
    private idle: Idle,
    public dialog: MatDialog,
    protected uiconfigsService: UIConfigsService,
    protected monitoringService: MonitoringService,
    protected searchService: SearchService,
    protected clusterService: ClusterService,
    protected workloadService: WorkloadService,
    protected authService: AuthService,
    protected rolloutService: RolloutService,
    protected routingService: RoutingService,
    protected diagnosticsService: DiagnosticsService,
    private cdr: ChangeDetectorRef,
    protected router: Router
  ) {
    super(_controllerService, uiconfigsService);
  }
  /**
   * Component life cycle event hook
   * It publishes event that AppComponent is about to instantiate
  */
  ngOnInit() {
    this.logger = Utility.getInstance().getLogService();

    this.os = Utility.getOperatingSystem();
    const browserObj = Utility.getBrowserInfomation();
    this.browsertype = browserObj['browserName'];
    this.browserversion = browserObj['browserName'] + browserObj['majorVersion'];
    this.retrieveMenuInfo();
    this._initAppData();
    this.getVersion();
    this.getCluster();
    this._subscribeToEvents();
    this._bindtoStore();
    this.userName = Utility.getInstance().getLoginName();

    this.setMomentJSSettings();

    this.waitfForFeatureReady();  // wait for uiConfig.service have feature info ready.

    this.clocktimer = setInterval(() => {
      this.clock = new Date();
      this.hourDifference = (this.clock.getTimezoneOffset()) / 60;
      if (this.hourDifference > 0) {
        this.hourDifference = -(this.hourDifference);
      } else {
        this.hourDifference = '+' + -(this.hourDifference);
      }
    }, 1000);

    if (this.scrollChanged && this.scrollChanged.debounceTime) {
      this.scrollChanged
        .debounceTime(200)
        .subscribe(scrollTop => {
          this.scrollTop = parseInt(scrollTop, 10);
          this.storeMenuInfo();
        });
    }
    this.cdr.detectChanges();
  }

  onScroll(event) {
    this.scrollChanged.next(event.target.scrollTop);
  }


  updateRolloutUIBlock() {
    this.isUINavigationBlocked = true;
    if (this.uiconfigsService.isAuthorized(UIRolePermissions.rolloutrollout_read)) {
      this.getRollouts();
      this.isScreenBlocked = false;
    } else {
      this.isScreenBlocked = true;
      this._controllerService.invokeConfirm({
        header: 'PSM Unavailable',
        message: 'A rollout is in progress. PSM is temporarily unavailable.',
        acceptVisible: true,
        rejectVisible: false,
        accept: () => {
          // When a primeng alert is created, it tries to "focus" on a button, not adding a button returns an error.
          // So we create a button but hide it later.
        }
      });
      setTimeout(() => {
        this.confirmDialog.acceptVisible = false;
      }, 0);
    }
  }

  removeRolloutUIBlock() {
    this.isScreenBlocked = false;
    this.isUINavigationBlocked = false;
  }

  getRollouts() {
    this.rolloutsEventUtility = new HttpEventUtility<RolloutRollout>(RolloutRollout, true);
    this.rolloutObjects = this.rolloutsEventUtility.array as ReadonlyArray<RolloutRollout>;
    const subscription = this.rolloutService.WatchRollout().subscribe(
      response => {
        this.rolloutsEventUtility.processEvents(response);
        this.checkRollouts();
      },
      this._controllerService.restErrorHandler('Failed to get Rollouts info')
    );
    this.subscriptions.push(subscription);
  }

  checkRollouts() {
    for (let i = 0; i < this.rolloutObjects.length; i++) {
      if (this.rolloutObjects[i].status.state === RolloutRolloutStatus_state.progressing ||
        this.rolloutObjects[i].status.state === RolloutRolloutStatus_state['suspend-in-progress']) {
        // VS-842. Venice appliance may reboot while suspending rollout. At that monment, rollout is not done.
        this.currentRollout = this.rolloutObjects[i];
        break;
      } else if (this.rolloutObjects[i].status.state === RolloutRolloutStatus_state.success && this.rolloutObjects[i] === this.currentRollout) {
        this.onRolloutComplete();
      }
    }
    if (this.currentRollout) {
      // Set current rollout only if it exists
      Utility.getInstance().setCurrentRollout(this.currentRollout);
      this._controllerService.navigate(['/maintenance']);
    }
  }

  setMomentJSSettings() {
    moment.updateLocale('en', {
      relativeTime: {
        future: 'in %s',
        past: '%s ago',
        s: 'one seconds',
        ss: '%d seconds',
        m: 'one minute',
        mm: '%d minutes',
        h: 'one hour',
        hh: '%d hours',
        d: 'one day',
        dd: '%d days',
        M: 'one month',
        MM: '%d months',
        y: 'one year',
        yy: '%d years'
      }
    });
    moment.relativeTimeThreshold('ss', 0);
    moment.relativeTimeThreshold('s', 60);
    moment.relativeTimeThreshold('m', 60);
    moment.relativeTimeThreshold('h', 24);
    moment.relativeTimeThreshold('d', 31);
    moment.relativeTimeThreshold('M', 12);
  }


  ngAfterViewInit() {
    // Example usage of setting help content
    // Once actual help data is in place
    // this should be removed.
    // TODO: Remove setting help template
    // this._controllerService.setHelpOverlayData({
    //   // template: this.helpTemplate
    //   id: 'authpolicy'
    // });
   // /*
  }

  /**
   * This api use setInterval technique to wait for uiconfigsService have feature info ready. Then it will invoke getModole() API
   */
  waitfForFeatureReady() {
    const id = setInterval(checkFeatureReady, 1000);
    const self = this;
    function checkFeatureReady() {
      if (self.uiconfigsService.isFeatureEnabled('cloud')) {
        self.getModules();
        clearInterval(id);
      } else if (self.uiconfigsService.isFeatureEnabled('enterprise')) {
        clearInterval(id);
      } else {
        // debug console.log(self.getClassName(), ' waitfForFeatureReady() waiting' );
      }
    }
  }

  getContentClass(): string {
    if (this.isSideNavExpanded && this.isSideNavShown) {
      return 'menufullopened';
    }
    if (this.isSideNavShown) {
      return 'menupartialopened';
    }
    return 'menuclosed';
  }

  redirectHome() {
    if (!this.isUINavigationBlocked) {
      this.uiconfigsService.navigateToHomepage();
    }
  }

  /**
   * This API handles Venice version object update from web-socket.
   * Per VS-932 , If Venice version.status['rollout-build-version']=== "",  it means rollout Venice part is completed, DSC upgrades may be still in progress.
   * In such case, it is OK to logout user and open Venice UI to all users.
   * @param response
   */
  handleWatchVersionResponse(response) {
    this.versionEventUtility.processEvents(response);
    // When latest Venice.version.status changes, UI acts on it.
    if (this.versionArray.length > 0) {
      if (!Utility.isEmpty(this.versionArray[0].status['rollout-build-version'])) {
        // Rollout is still in progress.  UI gets status update. We still want to stay in maintenance mode.
        this.updateRolloutUIBlock();
        Utility.getInstance().setMaintenanceMode(true);
      } else {
        // set maintenance mode back to false when version object is updated.
        // UI expects this.versionArray[0].status['rollout-build-version']== "". Rollout venice part is done, DSC upgrade may be still in progress.
        if (Utility.getInstance().getMaintenanceMode() === true) {
          Utility.getInstance().setMaintenanceMode(false);
          this.removeRolloutUIBlock();
          this.onRolloutComplete();
        }
      }
    }
    // In the very first time, record the first version data into this.version object.
    this.version = this.versionArray[0];
  }

  getCluster() {
    const sub = this.clusterService.GetCluster().subscribe(
      (response) => {
        this.cluster = response.body as ClusterCluster;
      },
      (error) => {
        this._controllerService.invokeRESTErrorToaster('Error', 'Failed to get cluster ');
      }
    );
    this.subscriptions.push(sub);
  }

  getVersion() {
    this.versionEventUtility = new HttpEventUtility<ClusterVersion>(ClusterVersion, true);
    this.versionArray = this.versionEventUtility.array as ReadonlyArray<ClusterVersion>;
    if (this.versionSubscription) {
      this.versionSubscription.unsubscribe();
    } else {
      this.versionSubscription = this.clusterService.WatchVersion().subscribe(
        response => {
          this.handleWatchVersionResponse(response);
        },
        (error) => {
          if (Utility.getInstance().getMaintenanceMode()) {
            setTimeout(() => {
              this.getVersion();  // Watch ends during rollout, try again so new updates can be shown on UI
            }, 3000);
          } else {
            this._controllerService.webSocketErrorToaster('Failed to get Cluster Version', error);
          }
        },
        () => {
          setTimeout(() => {
            this.version = new ClusterVersion();
            this.getVersion();
          }, 3000);
        }
      );
      this.subscriptions.push(this.versionSubscription);
    }
  }

  /**
   * Component life cycle event hook
   * It publishes event that AppComponent is about to exit
  */
  ngOnDestroy() {
    this.unsubscribeStore$.next();
    this.unsubscribeStore$.complete();
    this.subscriptions.forEach((sub) => {
      sub.unsubscribe();
    });
    this._boolInitApp = false;
    this.idle.stop();

    if (this.clocktimer) {
      clearInterval(this.clocktimer);
    }
  }

  /**
   * Overide super's API
   * It will return AppComponent
   */
  getClassName(): string {
    return this.constructor.name;
  }

  private _bindtoStore() {
    this.store
      .select(selectorSettings)
      .pipe(
        takeUntil(this.unsubscribeStore$),
        map(({ theme }) => theme.toLowerCase())
      )
      .subscribe(theme => {
        this.componentCssClass = theme;
        this.overlayContainer.getContainerElement().classList.add(theme);
      });
  }

  /***** Routing *****/

  public navigate(paths: string[]) {
    this._controllerService.navigate(paths);
  }

  /**
   * handles case when user logout event occurs
   */
  private onLogout(payload: any) {
    this.store.dispatch(logout());
    this._boolInitApp = false;
    this.navigate(['/login']);
  }


  private onSearchResultReady(payload) {
    if (payload['id'] === 'searchresult') {
      this.navigate(['/searchresult']);
    } else {
      console.error(this.getClassName() + '.onSearchResultReady() payload.id not supported \n' + payload);
    }
  }


  private _subscribeToEvents() {
    this.subscriptions[Eventtypes.IDLE_CHANGE] = this._controllerService.subscribe(Eventtypes.IDLE_CHANGE, (payload) => {
      this.handleIdleChange(payload);
    });

    this.subscriptions[Eventtypes.LOGOUT] = this._controllerService.subscribe(Eventtypes.LOGOUT, (payload) => {
      this.onLogout(payload);
    });

    this.subscriptions[Eventtypes.COMPONENT_INIT] = this._controllerService.subscribe(Eventtypes.COMPONENT_INIT, (payload) => {
      this._handleComponentStateChangeInit(payload);
    });
    this.subscriptions[Eventtypes.COMPONENT_DESTROY] = this._controllerService.subscribe(Eventtypes.COMPONENT_DESTROY, (payload) => {
      this._handleComponentStateChangeDestroy(payload);
    });
    this.subscriptions[Eventtypes.SEARCH_RESULT_LOAD_REQUEST] = this._controllerService.subscribe(Eventtypes.SEARCH_RESULT_LOAD_REQUEST, (payload) => {
      this.onSearchResultReady(payload);
    });

    this.subscriptions[Eventtypes.FETCH_USER_OBJ] = this._controllerService.subscribe(Eventtypes.FETCH_USER_OBJ, (payload) => {
      this.getUserObj(payload);
    });

    this.subscriptions[Eventtypes.SIDENAV_NAVIGATION_END] = this.router.events.pipe(filter(event => event instanceof NavigationEnd))
      .subscribe((event: NavigationEnd) => {
        const linkBase = `/${event.url.split('/')[1]}`;
        const isMenu = sideNavMenu.find(item => item.link[0] === linkBase && Array.isArray(item.children) && item.children.length > 0);
        if (isMenu) {
          this.openedMenuItems[linkBase] = true;
          this.storeMenuInfo();
        }
      });
  }

  getUserObj(req: GetUserObjRequest) {
    this.authService.GetUser(this.userName).subscribe(req.success, req.err);
  }

  _handleComponentStateChangeInit(payload: any) {
    this._currentComponent = payload;
  }

  _initAppData() {
    if (this._boolInitApp === true) {
      return;
    } else {
      // As we use listAlertCache() to fetch alerts.  this.getAlerts() is not needed.
      this.getAlertsWatch();
    }
    this._boolInitApp = true;
    this._setupIdle();
  }

  _setupIdle() {
    this.idle.setIdle(this._controllerService.idleTime);
    this.idle.setTimeout(10);
    this.idle.setInterrupts(DEFAULT_INTERRUPTSOURCES);
    this.idle.onIdleEnd.subscribe(() => {
      if (this.idleDialogRef) {
        this.idleDialogRef.close();
      }
      this.showIdleWarning = false;
      this.idleDialogRef = null;
    });
    this.idle.onTimeout.subscribe(() => {
      if (this.idleDialogRef) {
        this.idleDialogRef.close();
      }
      this.showIdleWarning = false;
      this.idleDialogRef = null;
      this._controllerService.publish(Eventtypes.LOGOUT, { 'reason': 'Idle timeout' });
    });
    this.idle.onTimeoutWarning.subscribe((countdown) => {
      if (!this.showIdleWarning) {
        this.showIdleWarning = true;
        this.idleDialogRef = this.dialog.open(IdleWarningComponent, {
          width: '400px',
          hasBackdrop: true,
          data: { countdown: countdown }
        });
      } else {
        if (this.idleDialogRef != null && this.idleDialogRef.componentInstance != null) {
          this.idleDialogRef.componentInstance.updateCountdown(countdown);
        }
      }
    });
    if (this._controllerService.enableIdle) {
      this.idle.watch();
    }
  }

  handleIdleChange(payload: any) {
    if (payload.active != null) {
      if (payload.active) {
        this.idle.watch();
      } else {
        this.idle.stop();
      }
    }
    if (payload.time != null) {
      this.idle.setIdle(payload.time);
    }
  }

  _handleComponentStateChangeDestroy(payload: any) {
    this._currentComponent = null;
  }

  isSubMenuShown(menuLink: string) {
    return this.openedMenuItems[menuLink] ? true : false;
  }

  onMenuItemClick(event, itemLink) {
    if (this.openedMenuItems[itemLink]) {
      delete this.openedMenuItems[itemLink];
    } else {
      this.openedMenuItems[itemLink] = true;
    }
    this.storeMenuInfo();
    event.stopPropagation();
  }

  // put openedMenuItems and scroll top into session storage on destroy
  // and read it back on initialize
  storeMenuInfo() {
    sessionStorage.setItem(SIDE_MENU_INFO, JSON.stringify({
      menuItiems: this.openedMenuItems,
      menuScrollTop: this.scrollTop,
      sideNavStatus: this.isSideNavExpanded,
      sideNavShown: this.isSideNavShown
    }));
  }

  retrieveMenuInfo() {
    const menuInfo = JSON.parse(sessionStorage.getItem(SIDE_MENU_INFO));
    if (menuInfo) {
      if (menuInfo.menuItiems) {
        this.openedMenuItems = menuInfo.menuItiems;
      }
      if (menuInfo.menuScrollTop) {
        this.scrollTop = parseInt(menuInfo.menuScrollTop, 10);
      }
      this.isSideNavShown = menuInfo.sideNavShown;
      this.isSideNavExpanded = menuInfo.sideNavStatus;
    }
  }

  onSideNavToggle() {
    this.isSideNavShown = !this.isSideNavShown;
    this.storeMenuInfo();
  }

  /**
   * This function serves html template.
   * It transforms the SideNav panel to other state
   * @param event
   */
  onSidebarCollapseClick(event) {
    this.isSideNavExpanded = !this.isSideNavExpanded;
    this.storeMenuInfo();
  }

  onLogoutClick() {
    this._controllerService.publish(Eventtypes.LOGOUT, { 'reason': 'User logged out' });
  }

  onRolloutComplete() {
    // Per VS-932. We want to logout user if rollout succeeded.
    let isRolloutSucceeded: boolean = false;
    let msg = 'Rollout completed'; // VS-849
    if (this.currentRollout) {
      if (this.currentRollout.status.state === RolloutRolloutStatus_state.suspended) {
        msg = 'Rollout is suspended';
      } else if (this.currentRollout.status.state === RolloutRolloutStatus_state.success) {
        // In a rollout, Venice upgrade may be done, but DSC upgrade may be still in progress.  QA wants different message.
        if (!this.currentRollout.status['dscs-status'] || this.currentRollout.status['dscs-status'].length === 0) {
          msg = 'Rollout is completed';
        } else {
          msg = 'PSM rollout is completed. DSC upgrades are still in progress.';
        }
        isRolloutSucceeded = true;
      } else if (this.currentRollout.status.state === RolloutRolloutStatus_state.failure) {
        msg = 'Rollout failed';
      } else if (Utility.isEmpty(this.versionArray[0].status['rollout-build-version1']) && this.currentRollout.status.state === RolloutRolloutStatus_state.progressing) {
        // Per VS-932, rollout venice part is done, DSC upgrades are in progress. We consider rollout is successful.
        msg = 'PSM rollout is completed. DSC upgrades are still in progress.';
        isRolloutSucceeded = true;
      } else {
        msg = 'Rollout status ' + this.currentRollout.status.state;
      }
      this.currentRollout = null;
      Utility.getInstance().setCurrentRollout(null);
      if (isRolloutSucceeded === true) {
        this._controllerService.invokeInfoToaster('Rollout', msg + ' You will be logged out in 5 seconds. Please do a hard refresh on all open browsers to reload updated data from the server.');
        setTimeout(() => {
          this._controllerService.publish(Eventtypes.LOGOUT, { 'reason': 'PSM Rollout succeeded' });
        }, 5000);
      } else {
        this._controllerService.invokeInfoToaster('Rollout', msg + ' Page will reload in 5 seconds.');
        setTimeout(() => {
          window.location.reload();
        }, 5000);
      }
    } else {
      // Code should not got into this block.
      console.error(this.getClassName() + '.onRolloutComplete() this.currentRollout ', this.currentRollout);
      this._controllerService.invokeInfoToaster('Rollout', msg + ' Page will reload in 5 seconds.');
      setTimeout(() => {
        window.location.reload();
      }, 5000);
    }
  }

  /**
   * This API serves html template
   * It tells the right-sideNav which ng-template to use
   */
  whichRightSideNav() {
    return this._rightSideNavIndicator;
  }

  /**
   * This function serves html template.
   * @param  $event
   * @param id
   */
  onToolbarIconClick($event, id) {
    // special case for help window popout overlay
    if (this.helpOverlay && this.helpOverlay.helpOverlayRef && this.helpOverlay.helpOverlayRef.hasAttached()) {
      this.helpOverlay.closeHelp();
      if (id !== 'help') {
        this.handleRightSideNavToggle($event, id);
      }
    } else {
      this.handleRightSideNavToggle($event, id);
    }
  }

  handleRightSideNavToggle($event, id) {
    if (this._rightSideNavIndicator === id) {
      this._rightSideNav.toggle();
    } else {
      this._rightSideNavIndicator = id;
      this._rightSideNav.open();
    }
    setTimeout(() => {
      // programmatically trigger window resize to tell sideNav container adjust widow size
      window.dispatchEvent(new Event('resize'));
    }, 500);
  }

  updateHighestSeverity() {
    this.alertHighestSeverity = 'info';
    let alert: MonitoringAlert;
    for (let i = 0; i < this.alerts.length; i++) {
      alert = this.alerts[i];
      if (alert.status.severity === 'critical') {
        this.alertHighestSeverity = 'critical';
        break;
      } else if (alert.status.severity === 'warn' && this.alertHighestSeverity !== 'critical') {
        this.alertHighestSeverity = 'warn';
      }
    }
  }

  getAlertNotificationClass() {
    if (this.alertHighestSeverity === 'critical') {
      return 'app-notification-critical';
    } else if (this.alertHighestSeverity === 'warn') {
      return 'app-notification-warn';
    } else if (this.alertHighestSeverity === 'info') {
      return 'app-notification-info';
    }
  }

   /**
   * Fetch diagnostics module
   */
  getModules() {
    const subscription = this.diagnosticsService.ListModuleCache().subscribe(
      (response) => {
        /* module json looks like this
         {
            "kind": "Module",
            "api-version": "v1",
            "meta": {
                "name": "node2-pen-pegasus"
            },
            "spec": {
                "log-level": "info"
            },
            "status": {
                "node": "node2",
                "module": "pen-pegasus",
                "category": "venice",
                "service": "pen-pegasus-676fcbcc64-sc97j",
                "service-ports": [
                    {
                        "name": "pen-pegasus",
                        "port": 179
                    },
                    {
                        "name": "pen-pegasus-cxm",
                        "port": 8001
                    }
                ]
            }
         },
        */
        const pegasusNodes = response.data.filter(module => module.status.module === 'pen-pegasus');
        // find nodes that host "pen-pegasus" module
        // user routing.proto HealthStatus to figure out RR health
        const nodeNames = pegasusNodes.map( node => node.status.node);
        const observables: Observable<any>[] = [];
        nodeNames.forEach( (n: string ) => {
          // build url  /routing/v1/node2/health // node2 is nodeNames[i];
          if (!Utility.isEmpty(n)) {
              observables.push(this.routingService.GetHealthZ(n));
          }
        });
        if (observables.length === 0 ) {
          return;
        }
        const sub = forkJoin(observables).subscribe(
          (results) => {
            const isAllOK = Utility.isForkjoinResultAllOK(results);
            if (isAllOK) {
              // process RRStatus    /routing/v1/node2/health return json like below
              /*
                 {
                     "kind": "",
                     "meta": {
                         "name": "",     <=============  no name
                         "generation-id": "",
                         "creation-time": "",
                         "mod-time": ""
                     },
                     "spec": {},
                     "status": {
                         "router-id": "192.168.30.12",
                         "internal-peers": {
                             "configured": 2,
                             "established": 2   <-------- this is healthy configured_# = established_#
                         },
                         "external-peers": {
                             "configured": 3,
                             "established": 0   <--------- this is not healthy configured_# != established_#
                         },
                         "unexpected-peers": 0
                     }
                 }
              */
              const routinghealthlist = results.map((r, i) => {
                const obj = new RoutingHealth(r.body);
                obj.meta.name = obj.meta.name ? obj.meta.name : obj.status['router-id']; // patch name
                obj._ui = { node: nodeNames[i] };
                return obj;
              });
              Utility.getInstance().setRoutinghealthlist(routinghealthlist);
              this._controllerService.publish(Eventtypes.RR_HEALTH_STATUS, routinghealthlist);
            } else {
              const error = Utility.joinErrors(results);
              this._controllerService.invokeRESTErrorToaster('Failure', error);
            }
          },
          (error) => {
            this._controllerService.invokeRESTErrorToaster('Failure', error);
          }
        );
        this.subscriptions.push(sub);

      },
      (error) => {
        this._controllerService.invokeRESTErrorToaster('Error', 'Failed to fetch modules');
      }
    );
    this.subscriptions.push(subscription);
  }

  /**
   * Call server to fetch all alerts using search API to populate RHS alert-list
   *
   */
  getAlerts() {
    if (this.alertGetSubscription) {
      this.alertGetSubscription.unsubscribe();
    }
    const query: SearchSearchRequest = new SearchSearchRequest({
      query: {
        kinds: ['Alert'],
        fields: {
          'requirements': [{
            key: 'spec.state',
            operator: FieldsRequirement_operator.equals,
            values: ['open']
          }]
        }
      },
      'max-results': 2,
      aggregate: false,
      mode: SearchSearchRequest_mode.preview,
    });
    this.alertGetSubscription = this.searchService.PostQuery(query).subscribe(
      resp => {
        const body = resp.body as ISearchSearchResponse;
        this.startingAlertCount = parseInt(body['total-hits'], 10);
        this.alertNumbers = this.startingAlertCount;
        // Now that we have the count already in the system, we start the watch
        this.getAlertsWatch();
      },
      this._controllerService.webSocketErrorHandler('Failed to get Alerts'),
    );
    this.subscriptions.push(this.alertGetSubscription);
  }

  /**
   * We use ListAlertCache to fetch alerts
   */
  getAlertsWatch() {
    if (this.alertWatchSubscription) {
      this.alertWatchSubscription.unsubscribe();
    }
    this.alertWatchSubscription = this.monitoringService.ListAlertCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        const myAlerts = response.data as MonitoringAlert[];
        this.alerts = myAlerts.filter((alert: MonitoringAlert) => {
          return (this.isAlertInOpenState(alert));
        });

        // VS-630 sort the alerts in desc order
        this.alerts = Utility.sortDate(this.alerts, ['meta', 'creation-time'], -1);
        this.updateHighestSeverity();


        // We are watching alerts. So when there are new alerts coming in, we display a toaster.
        if (this.alertNumbers > 0 && this.alertNumbers < this.alerts.length) {
          const diff = this.alerts.length - this.alertNumbers;
          const alertMsg = (diff === 1) ? diff + ' new alert arrived' : diff + ' new alerts arrived';
          const tooManyOpenAlerts: boolean = (this.alerts.length > Utility.DEFAULT_OPEN_ALERTS_NUMBER_TO_STOP_TOASTER);
          // User has no way to see alerts if Venice is in maintenance mode.
          // If there are too many open alerts already and alerts keep coming from web-socket connection, it is likely system is running alert script or system is big problem.
          // There is no point to pop alert toasters
          if (!Utility.getInstance().getMaintenanceMode() && !tooManyOpenAlerts) {
            this._controllerService.invokeInfoToaster('Alert', alertMsg);
          }
        }
        if (this.startingAlertCount <= 0) {
          this.alertNumbers = this.alerts.length;
        } else {
          this.alertNumbers = Math.max(this.startingAlertCount, this.alerts.length);
          if (this.alerts.length >= this.startingAlertCount) {   // VS-843, Once websocket gives more alerts than startingAlertCount, we reset startingAlertCount. WS may increment alerts by more than 1.(thus, using >=)
            // Have received all alerts that exist
            // no longer rely on startingAlertCount since we may receive delete events
            this.startingAlertCount = 0;
          }
        }
      },
      this._controllerService.webSocketErrorHandler('Failed to get Alerts'),
    );
    this.subscriptions.push(this.alertWatchSubscription);
  }


  isAlertInOpenState(alert: MonitoringAlert): boolean {
    return (alert.spec.state === 'open');
  }

  onRightSideNavClose() {
    this._rightSideNav.close();
  }

  /**
   * This API serves html template
   * It response to user request of expanding all alerts (in RHS alert-list panel)
   * @param  alertlist
   */
  onExpandAllAlertsClick(alertlist) {
    this.navigate(['/monitoring', 'alertsevents']);
    this._rightSideNav.close();

  }

  settingsNavigate(route) {
    this._rightSideNav.close();
    this.navigate(route);
  }

  _componentSetup(component) {
    component.instance.style_class = 'wlWidget-positive';
    component.instance.color = '#7db1ea';
    component.instance.id = 'test_id';
    component.instance.title = 'Test';
    component.instance.label = '10';
    component.instance.styleType = 'positive';
    component.icon = {
      margin: {
        top: '5px',
        right: '5px',
        url: 'test.com'
      }
    };
    const dataset = {
      x: [1, 2, 3, 4, 5],
      y: [1, 3, 2, 3, 8]
    };

    component.instance.data = dataset;
    component.instance.updateData();
  }

  alertType(event) {
    this.onToolbarIconClick(event.event, this._rightSideNavIndicator);
  }
}
