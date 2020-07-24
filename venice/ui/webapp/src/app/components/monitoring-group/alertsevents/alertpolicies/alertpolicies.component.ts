import { Component, OnInit, ViewEncapsulation, OnDestroy, ChangeDetectionStrategy, ChangeDetectorRef, ViewChild, AfterViewInit } from '@angular/core';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { BaseComponent } from '@app/components/base/base.component';
import { IMonitoringAlertDestination, IMonitoringAlertPolicy, IApiStatus, MonitoringAlertPolicy, MonitoringAlertDestination } from '@sdk/v1/models/generated/monitoring';
import { Subscription } from 'rxjs';
import { Utility } from '@app/common/Utility';
import {Router} from '@angular/router';
import { EventalertpolicyComponent } from './eventalertpolicies/eventalertpolicies.component';
import { DestinationpolicyComponent } from './destinations/destinations.component';

@Component({
  selector: 'app-alertpolicies',
  templateUrl: './alertpolicies.component.html',
  styleUrls: ['./alertpolicies.component.scss'],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class AlertpoliciesComponent extends BaseComponent implements OnInit, AfterViewInit, OnDestroy {
  @ViewChild('eventPolicy') eventPolicyComponent: EventalertpolicyComponent;
  @ViewChild('eventDestination') eventDestinationComponent: DestinationpolicyComponent;

  eventPolicies: ReadonlyArray<IMonitoringAlertPolicy> = [];
  destinations: ReadonlyArray<IMonitoringAlertDestination> = [];
  subscriptions: Subscription[] = [];
  selectedIndex = 0;

  constructor(protected _controllerService: ControllerService,
    protected _monitoringService: MonitoringService,
    protected cdr: ChangeDetectorRef,
    protected router: Router
  ) {
    super(_controllerService);
  }

  ngOnInit() {
    this.activeTab();
    this._controllerService.publish(Eventtypes.COMPONENT_INIT, {
      'component': 'AlertpoliciesComponent', 'state':
        Eventtypes.COMPONENT_INIT
    });
    this.getAlertPolicies();
    this.getDestinations();
  }

  ngAfterViewInit() {
    this.updateBreadCrumb(this.selectedIndex);
  }

  updateBreadCrumb(tabindex: number) {
    this._controllerService.setToolbarData({
      buttons: [
      ],
      breadcrumb: [
        { label: 'Alerts & Events', url: Utility.getBaseUIUrl() + 'monitoring/alertsevents' },
        { label: this.getSecondCrumbLabel(tabindex), url: Utility.getBaseUIUrl() + this.getSecondCrumbUrl(tabindex) }
      ]
    });
    if (tabindex === 0) {
      if (this.eventPolicyComponent) {
        this.eventPolicyComponent.updateToolbar();
      }
    } else {
      if (this.eventDestinationComponent) {
        this.eventDestinationComponent.updateToolbar();
      }
    }
  }

  getSecondCrumbLabel(tabindex: number) {
    if (tabindex === 0) {
      return 'Alert Policies';
    } else {
      return 'Destinations';
    }
  }

  getSecondCrumbUrl(tabindex: number) {
    if (tabindex === 0) {
      return 'monitoring/alertsevents/alertpolicies';
    } else {
      return 'monitoring/alertsevents/alertdestinations';
    }
  }

  tabSwitched(event) {
    if (event === 0) {
      this.router.navigate(['/monitoring/alertsevents/alertpolicies']);
    } else {
      this.router.navigate(['/monitoring/alertsevents/alertdestinations']);
    }
    this.activeTab();
    this.updateBreadCrumb(event);
  }

  activeTab() {
    if (this.router.url.endsWith('alertdestinations')) {
      this.selectedIndex = 1;
    } else {
      this.selectedIndex = 0;
    }
  }

  /**
  * Overide super's API
  * It will return this Component name
  */
  getClassName(): string {
    return this.constructor.name;
  }

  getAlertPolicies() {
    const sub = this._monitoringService.ListAlertPolicyCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        const policies = response.data as MonitoringAlertPolicy[] || [];
        this.eventPolicies = policies.filter((policy: MonitoringAlertPolicy) =>
          policy && policy.spec.resource === 'Event');
        this.cdr.detectChanges();
      },
      this._controllerService.webSocketErrorHandler('Failed to get Alert policies')
    );
    this.subscriptions.push(sub);
  }

  getDestinations() {
    const sub = this._monitoringService.ListAlertDestinationCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        this.destinations = response.data as MonitoringAlertDestination[];
        this.cdr.detectChanges();
      },
      this._controllerService.webSocketErrorHandler('Failed to get Alert destinations')
    );
    this.subscriptions.push(sub);
  }

  ngOnDestroy() {
    this.subscriptions.forEach(subscription => {
      subscription.unsubscribe();
    });
    this._controllerService.publish(Eventtypes.COMPONENT_DESTROY, {
      'component': 'AlertpoliciesComponent', 'state':
        Eventtypes.COMPONENT_DESTROY
    });
  }

}
