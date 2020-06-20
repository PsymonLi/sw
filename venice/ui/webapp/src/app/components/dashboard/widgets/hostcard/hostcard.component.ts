import { Component, Input, OnInit, ViewEncapsulation } from '@angular/core';
import { OnDestroy } from '@angular/core/src/metadata/lifecycle_hooks';
import { Router } from '@angular/router';
import { Animations } from '@app/animations';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { StatArrowDirection, CardStates, Stat } from '@app/components/shared/basecard/basecard.component';
import { FlipState } from '@app/components/shared/flip/flip.component';
import { Subscription } from 'rxjs';
import { ClusterHost } from '@sdk/v1/models/generated/cluster';
import { ClusterService } from '@app/services/generated/cluster.service';

@Component({
  selector: 'app-dsbdhosts',
  templateUrl: './hostcard.component.html',
  styleUrls: ['./hostcard.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None
})
export class HostCardComponent implements OnInit, OnDestroy {
  cardStates = CardStates;

  title: string = 'Host';
  statData: Stat = {
    value: '0',
    description: 'TOTAL HOSTS',
    arrowDirection: StatArrowDirection.HIDDEN,
    statColor: '#61B3A0',
    tooltip: 'Click to go to Hosts Overview',
    onClick: () => {
      this.goToHosts();
    }
  };

  themeColor: string = '#61B3A0';

  backgroundIcon: Icon = {
    svgIcon: 'host',
    margin: {}
  };

  icon: Icon = {
    margin: {
      top: '10px',
      left: '10px'
    },
    svgIcon: 'host'
  };

  @Input() lastUpdateTime: string;
  @Input() timeRange: string;
  // When set to true, card contents will fade into view
  @Input() cardState: CardStates = CardStates.LOADING;

  flipState: FlipState = FlipState.front;

  subscriptions: Subscription[] = [];

  menuItems = [
    {
      text: 'Navigate to Hosts Overview', onClick: () => {
        this.goToHosts();
      }
    }
  ];

  hosts: ClusterHost[] = [];

  constructor(private router: Router,
              protected clusterService: ClusterService) { }

  goToHosts() {
    this.router.navigateByUrl('/cluster/hosts');
  }

  ngOnInit() {
    this.getHosts();
  }

  getHosts() {
    const subscription = this.clusterService.ListHostCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          this.cardState = CardStates.FAILED;
          return;
        }
        this.hosts = response.data as ClusterHost[];
        this.statData.value = this.hosts.length.toString();
        this.cardState = CardStates.READY;
      },
    );
    this.subscriptions.push(subscription);
  }

  ngOnDestroy() {
    if (this.subscriptions != null) {
      this.subscriptions.forEach(subscription => {
        subscription.unsubscribe();
      });
    }
  }

}
