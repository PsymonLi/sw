import { ChangeDetectorRef, Component, OnInit, ViewEncapsulation, ViewChild } from '@angular/core';
import { ActivatedRoute } from '@angular/router';
import { Animations } from '@app/animations';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { Utility } from '@app/common/Utility';
import { CustomExportMap, TableCol } from '@app/components/shared/tableviewedit';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { ClusterService } from '@app/services/generated/cluster.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { ClusterDistributedServiceCard, ClusterDSCProfile, IApiStatus, IClusterDSCProfile } from '@sdk/v1/models/generated/cluster';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { Observable } from 'rxjs';
import { PercentPipe } from '@angular/common';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { Eventtypes } from '@app/enum/eventtypes.enum';


interface DSCMacName {
  mac: string;
  name: string;
}
interface DSCProfileUiModel {
  associatedDSCS: ClusterDistributedServiceCard[];
  associatedDSCSPercentile: Number;
  pendingDSCNames?: DSCMacName[];
  deploymentTarget: string;
  featureSet: string;
}

/**
 * This DscprofilesComponent is for DSC unified mode feature.
 * A DSC-profile contains many DSC features.
 * User can configure a profile by pick and choose features.  Features are orgainzied in groups.  UI will validate feature combination.
 *
 * User can assign DSC-Profiles to DSCs.
 * If a profile is associated with DSCs, UI will controls whether this profile can be updated or deleted.
 *
 *  TODO: 2020-02-28
 *  Link DSC to profiles. (once backend updates dscprofile.proto and dsc-profile populated with DSC data)
 */


@Component({
  selector: 'app-dscprofiles',
  templateUrl: './dscprofiles.component.html',
  styleUrls: ['./dscprofiles.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None
})
export class DscprofilesComponent extends DataComponent implements OnInit {
  @ViewChild('dscProfilesTable') dscProfilesTable: PentableComponent;

  DEFAULT_DSCPROFILE = 'default';

  dataObjects: ReadonlyArray<ClusterDSCProfile> = [];
  selectedDSCProfile: ClusterDSCProfile = null;

  naplesList: ClusterDistributedServiceCard[] = [];

  isTabComponent: boolean = false;
  disableTableWhenRowExpanded: boolean = true; // this make toolbar button disabled when record is in edit-mode.
  exportFilename: string = 'PSM-DSC-Profiles';
  exportMap: CustomExportMap = {
    'DSCs': (opts): string => {
      return (opts.data._ui.associatedDSCS) ? opts.data._ui.associatedDSCS.map(dsc => (dsc.spec.id) ? dsc.spec.id : dsc.meta.name).join(', ') : '';
    },
    'utilization': (opts): string => {
      const percentpipe = new PercentPipe('en-US');
      return percentpipe.transform(opts.data._ui.associatedDSCSPercentile, '1.2-3' );
      // use percent-pipe to format string
    },
    'deploymentTarget': (opts): string => {
      return opts.data._ui.deploymentTarget;
    },
    'featureSet': (opts): string => {
      return opts.data._ui.featureSet;
    },
    'Propagation': (opts): string => {
      const rowData = opts.data;
      if (opts.data.status['propagation-status']['generation-id'] !== opts.data.meta['generation-id']) {
        return ' DSC profile not propagated';
      } else if (rowData.status['propagation-status']['generation-id'] === rowData.meta['generation-id'] && rowData.status['propagation-status'].pending > 0 ) {
        return rowData.status['propagation-status'].pending + 'Out Of ' +  rowData.status['propagation-status'].pending + rowData.status['propagation-status'].updated + ' Pending';
      } else if ( !(rowData.status['propagation-status'].updated === 0 && rowData.status['propagation-status'].pending === 0 ) && rowData.status['propagation-status']['generation-id'] === rowData.meta['generation-id'] && rowData.status['propagation-status'].pending === 0) {
        return 'Complete';
      } else {
        return 'N/A';
      }
    },
    'status.propagation-status.pending-dscs': (opts): string => {
      return (opts.data._ui.pendingDSCNames) ? opts.data._ui.pendingDSCNames.map(dsc => (dsc.spec.id) ? dsc.spec.id : dsc.meta.name).join(', ') : '';

    }
  };
  tableLoading: boolean = false;

  dscprofilesEventUtility: HttpEventUtility<ClusterDSCProfile>;

  maxDSCsPerRow: number = 8;

  bodyicon: any = {
    margin: {
      top: '9px',
      left: '8px',
    },
    url: '/assets/images/icons/cluster/profiles/dsc-profile-black.svg',
  };

  headerIcon: Icon = {
    margin: {
      top: '0px',
      left: '10px',
    },
    matIcon: 'grid_on'
  };

  cols: TableCol[] = [
    { field: 'meta.name', header: 'Name', class: 'dscprofiles-column-dscprofile-name', sortable: true, width: 15 },
    { field: 'DSCs', header: 'DSCs', class: 'dscprofiles-column-dscs', sortable: false, width: 30 },
    { field: 'utilization', header: 'Utilization', class: 'dscprofiles-column-utilization', sortable: false, width: 10 },
    { field: 'deploymentTarget', header: 'Deployment Target', class: 'dscprofiles-column-feature-set', sortable: true, width: 20 },
    { field: 'featureSet', header: 'Feature Set', class: 'dscprofiles-column-feature-set', sortable: true, width: 20 },
    { field: 'Propagation', header: 'Propagation Status', class: 'dscprofiles-column-propagation-status', sortable: true, width: 20 },
    { field: 'status.propagation-status.pending-dscs', header: 'Pending DSC', class: 'dscprofiles-column-status-pendig', sortable: true, width: 20 },
    { field: 'meta.mod-time', header: 'Modification Time', class: 'dscprofiles-column-date', sortable: true, width: '180px' },
    { field: 'meta.creation-time', header: 'Creation Time', class: 'dscprofiles-column-date', sortable: true, width: '180px' },
  ];

  macToNameMap: { [key: string]: string } = {};
  viewPendingNaples: boolean;

  constructor(protected _controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected cdr: ChangeDetectorRef,
    private clusterService: ClusterService,
    private _route: ActivatedRoute
  ) {
    super(_controllerService, uiconfigsService);
  }

  clearSelectedDataObjects() {
    this.dscProfilesTable.selectedDataObjects = [];
  }

  getSelectedDataObjects() {
    return this.dscProfilesTable.selectedDataObjects;
  }

  setDefaultToolbar() {
    let buttons = [];
    if (this.uiconfigsService.isAuthorized(UIRolePermissions.clusterdscprofile_create)) {
      buttons = [
        {
          cssClass: 'global-button-primary  dscprofiles-button ',
          text: 'ADD DSC PROFILE',
          computeClass: () => !this.dscProfilesTable.showRowExpand ? '' : 'global-button-disabled',
          callback: () => { this.dscProfilesTable.createNewObject(); }
        }
      ];
    }
    this._controllerService.setToolbarData({
      buttons: buttons,
      breadcrumb: [{ label: 'DSC Profiles', url: Utility.getBaseUIUrl() + 'cluster/dscprofiles' }]
    });
  }

  ngOnInit() {
    this._controllerService.publish(Eventtypes.COMPONENT_INIT, {
      'component': this.getClassName(), 'state': Eventtypes.COMPONENT_INIT
    });
    this._route.queryParams.subscribe(params => {
      if (params.hasOwnProperty('dscprofile')) {
        // alerttab selected
        this.getSearchedDSCProfile(params['dscprofile']);
      }
    });
    this.setDefaultToolbar();
    this.watchDSCProfiles();
    this.watchNaples();
  }

  getSearchedDSCProfile(interfacename) {
    const subscription = this.clusterService.GetDSCProfile(interfacename).subscribe(
      response => {
        const dscprofile = response.body as ClusterDSCProfile;
        this.selectedDSCProfile = new ClusterDSCProfile(dscprofile);
        this.updateSelectedDSCProfile();
      },
      this._controllerService.webSocketErrorHandler('Failed to get Network interface ' + interfacename)
    );
    this.subscriptions.push(subscription); // add subscription to list, so that it will be cleaned up when component is destroyed.
  }

  updateSelectedDSCProfile() {
    this.processOneDSCProfile(this.selectedDSCProfile );
  }

  /**
   * Fetch DSC Profiles records
   */
  watchDSCProfiles() {
    const subscription = this.clusterService.ListDSCProfileCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        this.dataObjects  = response.data;
        this.handleDataReady(); // process DSCProfile. Note: At this this moment, this.selectedObj may not be available
      },
      this._controllerService.webSocketErrorHandler('Failed to get DSC Profile')
    );
    this.subscriptions.push(subscription); // add subscription to list, so that it will be cleaned up when component is destroyed.
  }

  watchNaples() {
    const dscSubscription = this.clusterService.ListDistributedServiceCardCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        this.naplesList = response.data as ClusterDistributedServiceCard[];
        for (let j = 0; j < this.naplesList.length; j++) {
          const dsc: ClusterDistributedServiceCard = this.naplesList[j];
          this.macToNameMap[dsc.meta.name] = dsc.spec.id;
        }
        this.handleDataReady();
      }
    );
    this.subscriptions.push(dscSubscription);
  }

  handleDataReady() {
    // When naplesList and dataObjects (dscprofiles) are ready, build profile-dscs
    if (this.naplesList && this.dataObjects) {
      let matchSelectedDSCProfile: boolean = false; // We want to check if newly arrived DSCProfile objects include selected-Profile
      for (let i = 0; i < this.dataObjects.length; i++) {
        const dscProfile: ClusterDSCProfile = this.dataObjects[i];
        if ( this.selectedDSCProfile && this.selectedDSCProfile.meta.name === dscProfile.meta.name ) {
          this.selectedDSCProfile = dscProfile;
          matchSelectedDSCProfile = true;
        }
        this.processOneDSCProfile(dscProfile);
      }
      if (matchSelectedDSCProfile) {
        this.updateSelectedDSCProfile(); // update selected-profile VS-2010
      } else {
        this.selectedDSCProfile = null;   // dismiss detail-panel
      }
      this.dataObjects = [...this.dataObjects] ;  // tell angular to refresh references.
    }
  }

  private processOneDSCProfile( dscProfile: ClusterDSCProfile ) {
    if (! dscProfile) {
      return;
    }
    const dscsnames = [];
    for (let j = 0; j < this.naplesList.length; j++) {
      const dsc: ClusterDistributedServiceCard = this.naplesList[j];
      if (dscProfile.meta.name === dsc.spec.dscprofile) {
        dscsnames.push(dsc);
      }
    }
    const pendingDSCNames: DSCMacName[] = [];
    for (let k = 0; dscProfile.status['propagation-status']['pending-dscs'] && k < dscProfile.status['propagation-status']['pending-dscs'].length; k++) {
      const mac = dscProfile.status['propagation-status']['pending-dscs'][k];
      const name = this.macToNameMap[mac];
      pendingDSCNames.push({ mac: mac, name: name });
    }
    // this if block is temporary, once DSCProfile.proto bug fix, we don't need it.
    if (!dscProfile._ui) {
      dscProfile._ui = {};
    }
    const dscProfileUiModel: DSCProfileUiModel = {
      associatedDSCS: dscsnames,
      associatedDSCSPercentile: (dscsnames.length / this.naplesList.length),
      pendingDSCNames: pendingDSCNames,
      deploymentTarget: this.getDeploymentTarget(dscProfile),
      featureSet: this.getFeatureSet(dscProfile)

      // TODO: add after backend adds it
      // description: this.getFeatureDescription(dscProfile)
    };
    dscProfile._ui = dscProfileUiModel;
  }

  getDeploymentTarget(dscProfile: ClusterDSCProfile): string {
    // const dscProfileUI = DSCProfileUtil.convertUIModel(dscProfile);
    return dscProfile.spec['deployment-target'];
  }

  getFeatureSet(dscProfile: ClusterDSCProfile): string {
    // const dscProfileUI = DSCProfileUtil.convertUIModel(dscProfile);
    return dscProfile.spec['feature-set'];
  }

  // TODO: Feature set data. Wait for backend
/*   getFeatureDescription(dscProfile: ClusterDSCProfile): string {
    const item = this.getFeaturesetHelper(dscProfile);
    return (item) ? item.value.description : '';
  }

  getFeaturesetHelper(dscProfile: ClusterDSCProfile): SelectItem {
    const theOne = this.options.find((item: SelectItem) => {
      const keys  = this.getObjectKeys(dscProfile.spec['feature-set']);
      let matched = true;
      for (let i = 0; i < keys.length; i ++) {
        const key  = keys[i];
        const modelValue = (dscProfile.spec['feature-set'][key]) ? dscProfile.spec['feature-set'][key] : false;
        if (item.value[key] !== modelValue) {
          matched = false;
          break;
        }
      }
      return matched;
    });
    return theOne;
  } */

  getNaplesName(mac: string): string {
    return this.macToNameMap[mac];
  }

  getClassName(): string {
    return this.constructor.name;
  }

  deleteRecord(object: ClusterDSCProfile): Observable<{ body: IClusterDSCProfile | IApiStatus | Error, statusCode: number; }> {
    return this.clusterService.DeleteDSCProfile(object.meta.name);
  }
  generateDeleteConfirmMsg(object: ClusterDSCProfile): string {
    return 'Are you sure you want to delete DSC Profile ' + object.meta.name;
  }
  generateDeleteSuccessMsg(object: ClusterDSCProfile): string {
    return 'Deleted DSC Profile ' + object.meta.name;
  }

  onDeleteRecord(event, object) {
    this.dscProfilesTable.onDeleteRecord(
      event,
      object,
      this.generateDeleteConfirmMsg(object),
      this.generateDeleteSuccessMsg(object),
      this.deleteRecord.bind(this)
    );
  }

  areSelectedRowsDeletable(): boolean {
    if (!this.uiconfigsService.isAuthorized(UIRolePermissions.networknetworkinterface_delete)) {
      return false;
    }
    const selectedRows = this.getSelectedDataObjects();
    if (selectedRows.length === 0) {
      return false;
    }
    const defaultProfile  = selectedRows.find( (rowData: ClusterDSCProfile) => {
      return rowData.meta.name === this.DEFAULT_DSCPROFILE;
      }
    );
    if ( defaultProfile) {
      return false;
    }
    const list = selectedRows.filter((rowData: ClusterDSCProfile) => {
      const uiData: DSCProfileUiModel = rowData._ui as DSCProfileUiModel;
      return uiData.associatedDSCS && uiData.associatedDSCS.length > 0;
      }
    );
    return (list.length === 0);
  }

  /**
   * This API serves HTML template. When there are many DSCs in one profile, we don't list all DSCs. This API builds the tooltip text;
   * @param dscprofile
   */
  buildMoreDSCsTooltip(dscprofile: ClusterDSCProfile): string {
    const dscTips = [];
    const uiData: DSCProfileUiModel = dscprofile._ui as DSCProfileUiModel;
    const dscs = uiData.associatedDSCS;
    for (let i = 0; i < dscs.length; i++) {
      if (i >= this.maxDSCsPerRow) {
        const dsc = dscs[i];
        if (i <= 2 * this.maxDSCsPerRow) {
          // We don't want to lood too much records to tooltip. Just load another maxDSCsPerRow. Say maxDSCsPerRow=10, we list first 10 records in table. Tooltip text contains 11-20th records
          dscTips.push(dsc.meta.name);
        }
      }
    }
    return dscTips.join(' , ') + (dscs.length > 2 * this.maxDSCsPerRow) + ' ... more';
  }

  buildMorePendingDSCsTooltip(dscprofile: ClusterDSCProfile): string {
    const dscTips = [];
    const uiData: DSCProfileUiModel = dscprofile._ui as DSCProfileUiModel;
    const macname = uiData.pendingDSCNames;
    for (let i = 0; i < macname.length; i++) {
      if (i >= this.maxDSCsPerRow) {
        const dsc = macname[i];
        if (i <= 2 * this.maxDSCsPerRow) {
          // We don't want to lood too much records to tooltip. Just load another maxDSCsPerRow. Say maxDSCsPerRow=10, we list first 10 records in table. Tooltip text contains 11-20th records
          dscTips.push(dsc.name);
        }
      }
    }
    return dscTips.join(' , ') + (macname.length > 2 * this.maxDSCsPerRow) + ' ... more';
  }

  /**
   * This API serves html template.
   * @param dscProfile:ClusterDSCProfile
   */
  showDeleteButton(dscProfile: ClusterDSCProfile): boolean {
    // Per VS-2024, don't allow deleting 'default' profile
    if (dscProfile.meta.name  ===  this.DEFAULT_DSCPROFILE) {
      return false;
    }
    // If dscProfile has associated DSC, we can not delete this dscProfile
    const uiData: DSCProfileUiModel = dscProfile._ui as DSCProfileUiModel;
    return (uiData && uiData.associatedDSCS && uiData.associatedDSCS.length > 0) ? false : true;
  }

  /**
   * This API serves html template.
   * @param dscProfile:ClusterDSCProfile
   */
  showEditButton (dscProfile: ClusterDSCProfile): boolean {
    // user can update DSC profile
    return true;
  }

  selectDSCProfile(event) {
    if (this.selectedDSCProfile && event.rowData === this.selectedDSCProfile) {
      this.selectedDSCProfile = null;
    } else {
      if (!this.dscProfilesTable.showRowExpand) {
        this.selectedDSCProfile = event.rowData;
      }
    }
  }

  closeDetails() {
    this.selectedDSCProfile = null;
  }

  viewPendingNaplesList() {
    this.viewPendingNaples = !this.viewPendingNaples;
  }

  expandRowRequest(event, rowData) {
    if (!this.dscProfilesTable.showRowExpand) {
      this.dscProfilesTable.toggleRow(rowData, event);
    }
  }

  editFormClose(rowData) {
    if (this.dscProfilesTable.showRowExpand) {
      this.dscProfilesTable.toggleRow(rowData);
    }
  }

  creationFormClose() {
    this.dscProfilesTable.creationFormClose();
  }

  onColumnSelectChange(event) {
    this.dscProfilesTable.onColumnSelectChange(event);
  }
}
