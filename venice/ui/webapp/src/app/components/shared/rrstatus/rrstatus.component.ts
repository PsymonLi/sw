import { Component, OnInit, Input, OnDestroy } from '@angular/core';
import { Subscription } from 'rxjs';
import { ClusterService } from '@app/services/generated/cluster.service';
import { ControllerService } from '@app/services/controller.service';
import { ClusterCluster } from '@sdk/v1/models/generated/cluster';
import { RoutingHealth } from '@sdk/v1/models/generated/routing';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { Utility } from '@app/common/Utility';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { BaseComponent } from '@app/components/base/base.component';
import { runInThisContext } from 'vm';
import { MatIconRegistry } from '@angular/material';
import { MaterialdesignModule } from '@app/lib/materialdesign.module';

@Component({
  selector: 'app-rrstatus',
  templateUrl: './rrstatus.component.html',
  styleUrls: ['./rrstatus.component.scss']
})
export class RrstatusComponent extends BaseComponent implements OnInit, OnDestroy {
  @Input() nodeHealth: RoutingHealth;
  @Input() last_updated: string;
  clustername: string;
  subscriptions: Subscription[] = [];
  routinghealthlist: RoutingHealth[];
  uiconfigsService: any;

  constructor(protected clusterService: ClusterService,
    protected _controllerService: ControllerService,
    protected uiconfigService: UIConfigsService,
  ) { super(_controllerService, uiconfigService); }


  ngOnInit() {
    this.getCluster();
  }
  getCluster() {
    const sub = this.clusterService.GetCluster().subscribe(
      (response) => {
        const cluster = response.body as ClusterCluster;
        this.clustername = cluster.meta.name;
      },
      (error) => {
        this._controllerService.invokeRESTErrorToaster('Error', 'Failed to get cluster ');
      }
    );
    this.subscriptions.push(sub);
  }
  isRoutingHealthNodeHealthy(): boolean {
    return (this.nodeHealth.status['internal-peers'].established === this.nodeHealth.status['internal-peers'].configured) &&
      (this.nodeHealth.status['external-peers'].established === this.nodeHealth.status['external-peers'].configured) &&
      (this.nodeHealth.status['unexpected-peers'] === 0);
  }
  isPeerHealthy(type): boolean {
    return this.nodeHealth.status[type + '-peers'].established === this.nodeHealth.status[type + '-peers'].configured;
  }
  ngOnDestroy() {
    this.subscriptions.forEach(sub => {
      sub.unsubscribe();
    });
  }

  onRRNodeHealthClick() {
    event.stopPropagation();
    event.preventDefault();
    const delimiter = '<br/>';
    const msg = Utility.routerHealthNodeStatus(this.nodeHealth);
    const healthStatus = (this.isRoutingHealthNodeHealthy()) ? ' Healthy' : 'Unhealthy' ;
    this._controllerService.invokeConfirm({
      icon: 'pi pi-info-circle',
      header: 'Route Reflector [' + this.nodeHealth.meta.name + '] ',
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
}
