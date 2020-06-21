import { ChangeDetectionStrategy, Component, OnInit } from '@angular/core';
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
import { forkJoin, Observable } from 'rxjs';
import { FlipState } from '@app/components/shared/flip/flip.component';
import { BaseComponent } from '@app/components/base/base.component';

const MS_PER_MINUTE: number = 60000;

@Component({
  selector: 'app-rrhealthcard',
  templateUrl: './rrhealthcard.component.html',
  styleUrls: ['./rrhealthcard.component.scss'],
})
export class RrhealthcardComponent extends BaseComponent implements OnInit {

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

  routingHealthList: RoutingHealth[];
  alertCountMap: {[key: string]: number} = {'critical': 0, 'warn': 0};

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
      if (this.routingHealthList) {
        this.invokeSearchAlert();
      }
      this.controllerService.subscribe(Eventtypes.RR_HEALTH_STATUS, (payload) => {
        this.routingHealthList = Utility.getInstance().getRoutinghealthlist();
        this.invokeSearchAlert();
      });
    }
  }

  invokeSearchAlert() {
    this.searchAlerts();
    this.loading = false;
    this.cardState = CardStates.READY;
    this.lastUpdateTime = new Date().toISOString();
  }

  searchAlerts() {
    const start = new Date(Date.now() - MS_PER_MINUTE * 60 * 24).toISOString() as any;
    const end = new Date(Date.now()).toISOString() as any;
    const nodeNames = this.routingHealthList.map( (r: RoutingHealth) => {
      return r.meta.name;
    });
    const query: SearchSearchRequest = new SearchSearchRequest({
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
            key: 'meta.mod-time',
            operator: FieldsRequirement_operator.lte,
            values: [end]
          },
          {
            key: 'meta.mod-time',
            operator: FieldsRequirement_operator.gte,
            values: [start]
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
      mode: SearchSearchRequest_mode.full,
    });

    const observables: Observable<any>[] = [];

    query.query.fields.requirements[0].values = ['critical'];
    observables.push(this.searchService.PostQuery(query));

    query.query.fields.requirements[0].values = ['warn'];
    observables.push(this.searchService.PostQuery(query));

    forkJoin(observables).subscribe(
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
    });
  }

  getExternalPeerHealth(node: RoutingHealth): boolean {
    return node.status['external-peers'].configured === node.status['external-peers'].established;
  }

  getInternalPeerHealth(node: RoutingHealth): boolean {
    return node.status['internal-peers'].configured === node.status['internal-peers'].established;
  }

  getNodeHealth(node: RoutingHealth): boolean {
    return this.getInternalPeerHealth(node) && this.getExternalPeerHealth(node);
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

    /**
   * This API get routingHealth node status info.
   * @param rrNode
   */
  getRoutingHealthNodeStatus(rrNode: RoutingHealth): string {
    const obj = Utility.trimUIFields(rrNode.status.getModelValues());
    const list = [];
    Utility.traverseJSONObject(obj, 0, list, this);
    return list.join('<br/>');
  }

  unhealthyPeerStatusMessage(rrNode: RoutingHealth, key: string) {
    const peerInfo: IHealthStatusPeeringStatus = rrNode.status[key];
    return peerInfo.established + ' established peers out of ' + peerInfo.configured + ' configured peers';
  }

}
