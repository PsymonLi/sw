import { ChangeDetectionStrategy, Component, OnInit, OnDestroy } from '@angular/core';
import { Utility } from '@app/common/Utility';
import { CardStates } from '@app/components/shared/basecard/basecard.component';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { SearchService } from '@app/services/generated/search.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { RoutingHealth, IHealthStatusPeeringStatus } from '@sdk/v1/models/generated/routing';
import { FieldsRequirement_operator, SearchSearchRequest, SearchSearchRequest_mode } from '@sdk/v1/models/generated/search';
import { forkJoin, Observable, Subscription } from 'rxjs';
import { FlipState } from '@app/components/shared/flip/flip.component';
import { BaseComponent } from '@app/components/base/base.component';

@Component({
  selector: 'app-rrhealthcard',
  templateUrl: './rrhealthcard.component.html',
  styleUrls: ['./rrhealthcard.component.scss'],
})
export class RrhealthcardComponent extends BaseComponent implements OnInit, OnDestroy {

  flipState: FlipState = FlipState.front;

  // app-basecard
  title: string = 'Route Reflectors';
  cardState: CardStates = CardStates.LOADING;
  backgroundIcon: Icon = {
    svgIcon: 'node',
    margin: {},
  };
  icon: Icon = {
    margin: {
      top: '10px',
      left: '10px'
    },
    svgIcon: 'node'
  };
  themeColor: string = '#b592e3';
  loading: boolean = true;
  lastUpdateTime: string = '';
  menuItems = [];

  routingHealthList: RoutingHealth[] = [];
  alertCountMap: {[key: string]: number} = {'critical': 0, 'warn': 0};
  searchQuery: SearchSearchRequest;
  isRRListNotConfigured: boolean = true;
  alertSubscription: Subscription;

  constructor(protected uiconfigsService: UIConfigsService,
              protected controllerService: ControllerService,
              protected monitoringService: MonitoringService,
              protected searchService: SearchService,
              ) {
                super(controllerService);
              }

  ngOnInit() {
    this.cardState = CardStates.LOADING;
    setTimeout(() => {
      this.getRoutingHealth();
    }, 2000);
    this.buildMenuItems();
  }

  buildMenuItems() {
    this.menuItems.push({
      text: 'Go to Cluster Page', onClick: () => {
        this.redirectToClusterPage();
      }
    });
    this.menuItems.push({
      text: 'Export RR Health JSON', onClick: () => {
        this.exportJson();
      }
    });
  }

  exportJson() {
    if (this.routingHealthList) {
      const exportObj = {
        'nodes': this.routingHealthList.map(node => {
          return Utility.trimUIFields(node.status.getModelValues());
        })
      };
      const fieldName = 'rr-health.json';
      Utility.exportContent(JSON.stringify(exportObj, null, 2), 'text/json;charset=utf-8;', fieldName);
      Utility.getInstance().getControllerService().invokeInfoToaster('Data exported', 'Please find ' + fieldName + ' in your downloads folder');
    } else {
      setTimeout(() => {
        this.exportJson();
      }, 1000);
    }
  }

  redirectToClusterPage() {
    this.controllerService.navigate(['cluster', 'cluster']);
  }

  getRoutingHealth() {
    if (this.uiconfigsService.isFeatureEnabled('cloud')) {
      this.routingHealthList = Utility.getInstance().getRoutinghealthlist();  // appcontent.component.ts should have fetched RR data and store in Utility
      this.isRRListNotConfigured = Utility.isRRNodeListNotConfigured(this.routingHealthList);
      if (this.routingHealthList) {
        this.invokeSearchAlert();
      }
      this.controllerService.subscribe(Eventtypes.RR_HEALTH_STATUS, (payload) => {
        this.routingHealthList = Utility.getInstance().getRoutinghealthlist();
        this.isRRListNotConfigured = Utility.isRRNodeListNotConfigured(this.routingHealthList);
        this.invokeSearchAlert();
      });
    }
  }

  invokeSearchAlert() {
    this.searchAlerts();
    this.loading = false;
    this.cardState = CardStates.READY;
  }

  searchAlerts() {
    if (this.isRRListNotConfigured) {
       // Don't run search if node-names list is empty
      return;
    }
    let nodeNames = this.routingHealthList.map( (r: RoutingHealth) => {
      return r.meta.name;
    });
    nodeNames = nodeNames.filter((name: string) => name !== ''); // Empty string means RR not configured on that node
    this.searchQuery = new SearchSearchRequest({
      query: {
        kinds: ['Alert'],
        fields: {
          'requirements': [
          {
            key: 'status.severity',
            operator: FieldsRequirement_operator.equals,
            values: ['-'] // query is modified and used for both critical and warn alerts
          },
          {
            key: 'status.object-ref.name',
            operator: FieldsRequirement_operator.in,
            values: nodeNames
          },
          {
            key: 'status.object-ref.kind',
            operator: FieldsRequirement_operator.equals,
            values: ['Node']
          },
          {
            key: 'spec.state',
            operator: FieldsRequirement_operator.equals,
            values: ['open']
          },
        ]
        }
      },
      'max-results': 2,
      aggregate: false,
      mode: SearchSearchRequest_mode.preview,
    });
    this.pollAlerts();
  }

  pollAlerts() {
    const observables: Observable<any>[] = [];

    this.searchQuery.query.fields.requirements[0].values = ['critical'];
    observables.push(this.searchService.PostQuery(this.searchQuery));

    this.searchQuery.query.fields.requirements[0].values = ['warn'];
    observables.push(this.searchService.PostQuery(this.searchQuery));

    if (this.alertSubscription) {
      this.alertSubscription.unsubscribe();
    }
    this.alertSubscription = forkJoin(observables).subscribe(
      (results) => {
        this.alertCountMap = {'critical': 0, 'warn': 0};
        // 2 forkJoined queries, result length should be same
        if (Utility.isForkjoinResultAllOK(results) && results && results.length === 2) {
          const criticalAlertResult = results[0];
          const warnAlertResult = results[1];

          if (criticalAlertResult.body && criticalAlertResult.body['total-hits'] ) {
            this.alertCountMap['critical'] = criticalAlertResult.body['total-hits'];
          }
          if (warnAlertResult.body && warnAlertResult.body['total-hits'] ) {
            this.alertCountMap['warn'] = warnAlertResult.body['total-hits'];
          }
        } else {
          this.controllerService.invokeRESTErrorToaster('Failure', 'Failed to search alerts');
        }
    },
    (error) => {
      this.controllerService.invokeRESTErrorToaster('Failure', 'Failed to search alerts');
    },
    () => {
      this.lastUpdateTime = new Date().toISOString();
      setTimeout(() => {
        this.pollAlerts();
      }, 30000);
    });
    this.subscriptions.push(this.alertSubscription);
  }

  getExternalPeerHealth(node: RoutingHealth): boolean {
    return Utility.getExternalPeerHealth(node);
  }

  getInternalPeerHealth(node: RoutingHealth): boolean {
    return Utility.getInternalPeerHealth(node);
  }

  getNodeHealth(node: RoutingHealth): boolean {
    return Utility.getRoutingNodeHealth(node);
  }

  isNodeConfigured(node: RoutingHealth): boolean {
    return Utility.isRRNodeConfigured(node);
  }

  selectNodeAndShowDialog(rrNode: RoutingHealth) {
    event.stopPropagation();
    event.preventDefault();
    const delimiter = '<br/>';
    const msg = this.getRoutingHealthNodeStatus(rrNode);
    const healthStatus = (this.getNodeHealth(rrNode)) ? ' Healthy' : 'Unhealthy' ;
    this.controllerService.invokeConfirm({
      icon: 'pi pi-info-circle',
      header: 'Route Reflector [' + rrNode.meta.name + '] ',
      message: 'Status: ' +  healthStatus  + delimiter + msg,
      acceptLabel: 'Close',
      acceptVisible: true,
      rejectVisible: false,
      accept: () => {
        // When a primeng alert is created, it tries to "focus" on a button, not adding a button returns an error.
        // So we create a button but hide it later.
      }
    });
  }

  showPSMHealthDialogBox() {
    event.stopPropagation();
    event.preventDefault();
    const delimiter = '<br/>';
    const healthStatus = this.getPSMHeatlhStatus() === 'HEALTHY' ? 'Healthy' : 'Unhealthy';
    const msg = healthStatus === 'Unhealthy' ? this.generatePSMHealthSummary() : '';
    this.controllerService.invokeConfirm({
      icon: 'pi pi-info-circle',
      header: 'PSM Route Reflector Health Summary',
      message: 'Status: ' +  healthStatus + delimiter + msg,
      acceptLabel: 'Close',
      acceptVisible: true,
      rejectVisible: false,
      accept: () => {
        // When a primeng alert is created, it tries to "focus" on a button, not adding a button returns an error.
        // So we create a button but hide it later.
      }
    });
  }

  generatePSMHealthSummary() {
    const delimiter = '<br/>';
    const tab = '&emsp;';
    let rrConfigLength: number = 0;
    let msg: string = delimiter + 'Reason/s: ' + delimiter;

    for ( let i = 0; i < this.routingHealthList.length ; i++) {
      if (this.isNodeConfigured(this.routingHealthList[i])) {
        let nodeMsg  = '';
        // if (this.routingHealthList[i].status['router-id'] === '') {
        //   nodeMsg += tab + tab + ' - Router ID not configured!' + delimiter;
        // }
        if (!Utility.getUnexpectedPeerHealth(this.routingHealthList[i])) {
          nodeMsg += tab + tab + ' - Unexpected Peers Present!' + delimiter;
        }
        if (!this.getExternalPeerHealth(this.routingHealthList[i])) {
          nodeMsg += tab + tab + ' - External Peers 0 or not established!' + delimiter;
        }
        if (!this.getInternalPeerHealth(this.routingHealthList[i])) {
          nodeMsg += tab + tab + ' - Internal Peers 0 or not established!' + delimiter;
        }
        msg += (nodeMsg !== '' ? tab + '- [' + this.routingHealthList[i].meta.name + '] -' + delimiter : '') + nodeMsg;
        rrConfigLength += 1;
      }
    }

    if (rrConfigLength < 2) {
      msg += tab + '- There should be two RR nodes in PSM!' + delimiter;
    }

    if (rrConfigLength > 0) {
      msg += delimiter + 'Please click respective node/s summary to know more.';
    }
    return msg;
  }

    /**
   * This API get routingHealth node status info.
   * @param rrNode
   */
  getRoutingHealthNodeStatus(rrNode: RoutingHealth): string {
    return Utility.routerHealthNodeStatus(rrNode);
  }

  unhealthyPeerStatusMessage(rrNode: RoutingHealth, key: string) {
    const peerInfo: IHealthStatusPeeringStatus = rrNode.status[key];
    return peerInfo.established + ' established peers out of ' + peerInfo.configured + ' configured peers';
  }

  getPSMHeatlhStatus(): string {
    if (this.routingHealthList.length === 2) {
      return this.getNodeHealth(this.routingHealthList[0]) && this.getNodeHealth(this.routingHealthList[1]) ? 'HEALTHY' : 'UNHEALTHY';
    }
    return 'UNHEALTHY';
  }

  ngOnDestroy() {
    this.subscriptions.forEach(
      subscription => {
        subscription.unsubscribe();
      }
    );
  }

}
