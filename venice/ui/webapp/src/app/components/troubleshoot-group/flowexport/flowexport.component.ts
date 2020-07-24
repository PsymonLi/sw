import { Component, OnInit, ViewEncapsulation } from '@angular/core';
import { ControllerService } from '@app/services/controller.service';
import { BaseComponent } from '@app/components/base/base.component';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { ClusterService } from '@app/services/generated/cluster.service';

@Component({
  selector: 'app-flowexport',
  templateUrl: './flowexport.component.html',
  styleUrls: ['./flowexport.component.scss'],
  encapsulation: ViewEncapsulation.None
})
export class FlowexportComponent extends BaseComponent implements OnInit {
  constructor(
    protected controllerService: ControllerService,
    protected monitoringService: MonitoringService,
    protected clusterService: ClusterService
  ) {
    super(controllerService);
  }

  bodyIcon: any = {
    margin: {
      top: '9px',
      left: '8px'
    },
    url: '/assets/images/icons/monitoring/ico-export-policy-black.svg'
  };

  getClassName(): string {
    return this.constructor.name;
  }
}
