import { Component, OnInit, ViewEncapsulation } from '@angular/core';
import { ControllerService } from '@app/services/controller.service';
import { BaseComponent } from '@app/components/base/base.component';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { IMonitoringFlowExportPolicy, MonitoringFlowExportPolicy } from '@sdk/v1/models/generated/monitoring';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { Subscription } from 'rxjs';
import { ClusterService } from '@app/services/generated/cluster.service';
import { ClusterDistributedServiceCard } from '@sdk/v1/models/generated/cluster';
import { DSCsNameMacMap, ObjectsRelationsUtility } from '@app/common/ObjectsRelationsUtility';

interface FlowExportPolicyUIModel {
     pendingDSCmacnameMap?: {[key: string]: string };
}

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
  flowExportPolicies: ReadonlyArray<IMonitoringFlowExportPolicy> = [];

  subscriptions: Subscription[] = [];
  naplesList: ClusterDistributedServiceCard[] = [];

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

  ngOnInit() {
    // Setting the toolbar of the app
    this.controllerService.setToolbarData({
      buttons: [],
      breadcrumb: []
    });
    this.getFlowExportPolicies();
    this.getDSCs();
  }

  /**
   * This API is used in getXXX().
   * Once flowExportPolicies has status['propagation-status']['pending-dscs'] = [aaaa.bbbb.cccc, xxxx.yyyy.zzz]
   * We build map { mac : dsc-name}
   */
  handleDataReady() {
    // When naplesList is ready, build DSC-maps
    if (this.naplesList && this.naplesList.length > 0 && this.flowExportPolicies && this.flowExportPolicies.length > 0) {
      const _myDSCnameToMacMap: DSCsNameMacMap = ObjectsRelationsUtility.buildDSCsNameMacMap(this.naplesList);
      const macToNameMap = _myDSCnameToMacMap.macToNameMap;
      this.flowExportPolicies.forEach ( (flowExportPoliy) => {
        if (flowExportPoliy.status['propagation-status'] && flowExportPoliy.status['propagation-status']['pending-dscs']) {
          const propagationStatusDSCName = {};
          flowExportPoliy.status['propagation-status']['pending-dscs'].forEach( (mac: string)  => {
            propagationStatusDSCName[mac] = macToNameMap[mac];
          });
          const uiModel: FlowExportPolicyUIModel = { pendingDSCmacnameMap : propagationStatusDSCName};
          flowExportPoliy._ui = uiModel;
        }
      });
      this.flowExportPolicies = [...this.flowExportPolicies]; // tell angular to refreh refreence
    }
  }

  getDSCs() {
    const dscSubscription = this.clusterService.ListDistributedServiceCardCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.naplesList = response.data as ClusterDistributedServiceCard[];
        this.handleDataReady();
      }
    );
    this.subscriptions.push(dscSubscription);

 }

  getFlowExportPolicies() {
    const dscSubscription = this.monitoringService.ListFlowExportPolicyCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.flowExportPolicies = response.data as MonitoringFlowExportPolicy[];
        this.handleDataReady();
      }
    );
    this.subscriptions.push(dscSubscription);
  }

}
