import {ChangeDetectorRef, Component, Input, ViewEncapsulation, ViewChild, OnInit} from '@angular/core';
import {Animations} from '@app/animations';
import {Utility} from '@app/common/Utility';
import { TablevieweditAbstract, TablevieweditHTMLComponent } from '@app/components/shared/tableviewedit/tableviewedit.component';
import {ControllerService} from '@app/services/controller.service';
import {ObjstoreService} from '@app/services/generated/objstore.service';
import {RolloutService} from '@app/services/generated/rollout.service';
import {IApiStatus, IRolloutRollout, RolloutRollout, RolloutRolloutStatus_state} from '@sdk/v1/models/generated/rollout';
import { IObjstoreObjectList  } from '@sdk/v1/models/generated/objstore';
import {Observable} from 'rxjs';
import {Router} from '@angular/router';
import {HttpEventUtility} from '@common/HttpEventUtility';
import { ToolbarData } from '@app/models/frontend/shared/toolbar.interface';
import { TableCol, CustomExportMap } from '@app/components/shared/tableviewedit';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { RolloutUtil } from '@app/components/admin/systemupgrade/rollouts/RolloutUtil.ts';
import { DataComponent } from '@app/components/shared/datacomponent/datacomponent.component';
import { PentableComponent } from '@app/components/shared/pentable/pentable.component';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { IStagingBulkEditAction } from '@sdk/v1/models/generated/staging';
/**
 * This component let user manage Venice Rollouts.
 * User can run CRUD opeations.  As well, user can click on one rollout to watch rollout status.
 * User can also go to rollout images management UI in toolbar [IMAGES] button
 */

@Component({
  selector: 'app-rollouts',
  templateUrl: './rollouts.component.html',
  styleUrls: ['./rollouts.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None
})
export class RolloutsComponent extends DataComponent implements OnInit {
  @ViewChild('pendingRolloutTable') pendingRolloutTable: PentableComponent;
  @ViewChild('pastRolloutTable') pastRolloutTable: PentableComponent;
  tableLoading: boolean;

  dataObjects: ReadonlyArray<RolloutRollout> = [];
  dataObjectsBackUp: ReadonlyArray<RolloutRollout> = [];
  pendingRollouts: RolloutRollout[] = [];
  pastRollouts: RolloutRollout[] = [];

  isTabComponent: boolean = true;
  exportMap: CustomExportMap = {};
  disableTableWhenRowExpanded: boolean = true;
  selectedRollOut: RolloutRollout;
  rolloutImages: IObjstoreObjectList;
  shouldEnableCreate: boolean = false;


  bodyicon: any = {
    margin: {
      top: '9px',
      left: '8px'
    },
    matIcon: 'update'
  };

  cols: TableCol[] = [
    {field: 'meta.name', header: 'Name', class: '', sortable: true, width: 100},
    {field: 'meta.mod-time', header: 'Updated Time', class: '', sortable: true, width: '180px'},
    {field: 'meta.creation-time', header: 'Creation Time', class: '', sortable: true, width: '180px'},
    {field: 'spec.version', header: 'Version', class: '', sortable: true, width: 100},
    {
      field: 'spec.scheduled-start-time',
      header: 'Scheduled Start Time',
      class: '',
      sortable: true,
      width: '180px'
    },
    {field: 'spec.strategy', header: 'Strategy', class: '', sortable: true, width: 100},
    {field: 'spec.upgrade-type', header: 'Upgrade Type', class: '', sortable: true, width: 100},
    {field: 'status.prev-version', header: 'Prev Version', class: '', sortable: true, width: 100},
    {field: 'status.state', header: 'State', class: '', sortable: true, width: 200},
  ];

  exportFilename: string = 'PSM-rollouts';

  tabIndex: number = 0;

  currentIndex: number = 0;

  isFirstDataLoad: boolean = true;
  shouldEnableButtons: boolean = true;

  constructor(protected controllerService: ControllerService,
              protected cdr: ChangeDetectorRef,
              protected rolloutService: RolloutService,
              protected objstoreService: ObjstoreService,
              protected router: Router,
              protected uiconfigsService: UIConfigsService) {
    super(controllerService, uiconfigsService);
  }

  /**
   * NEED OF OVERRIDING:
   * In order to create a new rollout, we are switching to PENDING Tab if the control is on PAST Tab.
   * After creation of new rollout or canceling the creation, we need to switch back to original Tab.
   * This function would allow us to close the form as well as switch back to the original Tab.
   */

  creationFormClose() {
    super.creationFormClose();
    if (this.currentIndex !== this.tabIndex) {
       setTimeout(() => { this.changeTabAssignTable(this.currentIndex); }, 200);
    }
  }

  ngOnInit(): void {
    super.ngOnInit();
    this.penTable = this.tabIndex === 1 ? this.pastRolloutTable : this.pendingRolloutTable;
    this.getRollouts();
    this.getRolloutImages();
  }

  getRollouts() {
    const subscription = this.rolloutService.ListRolloutCache().subscribe(
      response => {
        if (response.connIsErrorState) {
          return;
        }
        this.dataObjects = response.data as ReadonlyArray<RolloutRollout>;
        this.setDefaultToolbar();
        this.splitRollouts();
        this.tableLoading = false;
        this.refreshGui(this.cdr);
      },
      () => {
        this.tableLoading = false;
        this.controllerService.restErrorHandler('Failed to fetch Rollouts.');
        this.refreshGui(this.cdr);
      }
    );
    this.subscriptions.push(subscription);
  }

  splitRollouts() {
    this.pendingRollouts = [];
    this.pastRollouts = [];
    for  (let i = 0; i < this.dataObjects.length; i++) {
      if ( RolloutUtil.isRolloutPending(this.dataObjects[i]) ) {
        this.pendingRollouts.push(this.dataObjects[i]);
      } else {
        this.pastRollouts.push(this.dataObjects[i]);
      }
    }
    if (this.isFirstDataLoad) {
       const newIndex = (this.pendingRollouts.length === 0) ? 1 : 0; // If there is no pending rollout, switch to past rollout tab
       this.changeTabAssignTable(newIndex);
       this.isFirstDataLoad = false;
    }
  }

  getRolloutImages() {
    const sub = this.objstoreService.ListObject(Utility.ROLLOUT_IMGAGE_NAMESPACE).subscribe(
      (response) => {
        this.rolloutImages = response.body as IObjstoreObjectList;
        if (this.rolloutImages.items && this.rolloutImages.items.length > 0) {
          this.rolloutImages.items = Utility.sortDate(this.rolloutImages.items, ['meta', 'mod-time'], -1);
          this.shouldEnableCreate = true;
          this.buildToolbarData();
        } else {
          this.controllerService.invokeInfoToaster('No images available', 'please upload image');
        }
      },
      (error) => {
        this.controllerService.invokeRESTErrorToaster('Failed to fetch rollout images.', error);
      }
    );
    this.subscriptions.push(sub);
  }

  setDefaultToolbar() {
    this.controllerService.setToolbarData(this.buildToolbarData());
  }

  /**
   * This function helps to build Toolbar Data.
   * CREATE ROLLOUT button allows to create new rollout.
   * It works as following:
   *  - It switches to PENDING Tab, if it is on PAST Tab
   *  - Once it is on PENDING Tab, it opens up the CREATION FORM
   */
  buildToolbarData(): ToolbarData {
    const toolbarData: ToolbarData = {
      breadcrumb: [{label: 'System Upgrade', url: Utility.getBaseUIUrl() + 'admin/upgrade'}],
      buttons: []
    };
    if (this.isToBuildCreateButton() && this.uiconfigsService.isAuthorized(UIRolePermissions.rolloutrollout_create)) {
      toolbarData.buttons.push(
        {
          cssClass: 'global-button-primary rollouts-button',
          text: 'CREATE ROLLOUT',
          // We show 'create button' in 'pending tab' and 'past tab'
          computeClass: () =>  this.shouldEnableCreate ? '' : 'global-button-disabled',
          callback: () => {
            // Switching to PENDING Tab for creation if on some other Tab
            this.currentIndex = this.tabIndex;
            if (this.tabIndex === 0) {
              this.penTable.createNewObject();
            } else {
              this.changeTabAssignTable(0);
              setTimeout(() => {this.penTable.createNewObject(); }, 200);
            }
          }
        }
      );
    }
    toolbarData.buttons.push(
      {
        cssClass: 'global-button-primary rollouts-button',
        text: 'ROLLOUT IMAGES',
        computeClass: () => this.shouldEnableButtons ? '' : 'global-button-disabled',
        callback: () => {
          this.invokeUploadImageUI();
        }
      }
    );
    return toolbarData;
  }

  /**
   * This API controls whether to build [Create Rollout] button
   */
  isToBuildCreateButton(): boolean {
    const isBuild = true;
    for  (let i = 0; i < this.dataObjects.length; i++) {
      if (this.dataObjects[i].status.state === RolloutRolloutStatus_state.progressing || this.dataObjects[i].status.state === RolloutRolloutStatus_state['precheck-in-progress']
          || this.dataObjects[i].status.state === RolloutRolloutStatus_state['suspend-in-progress'] || this.dataObjects[i].status.state === RolloutRolloutStatus_state['scheduled-for-retry']) {
        return false;
      }
    }
    return isBuild;
  }

  deleteRecord(object: RolloutRollout): Observable<{ body: IRolloutRollout | IApiStatus | Error, statusCode: number }> {
     return this.rolloutService.RemoveRollout(object);
  }

  postDeleteRecord () {
    this.splitRollouts();
  }

  generateDeleteConfirmMsg(object: IRolloutRollout) {
    return 'Are you sure you want to delete rollout ' + object.meta.name;
  }

  generateDeleteSuccessMsg(object: IRolloutRollout) {
    return 'Deleted rollout ' + object.meta.name;
  }

  invokeUploadImageUI() {
    this.router.navigateByUrl('admin/upgrade/imageupload');
  }

  // Row expansion toggle
  onRowClick(event, rowData) {
    if (this.selectedRollOut) {
      this.editFormClose(rowData);
      this.selectedRollOut = null;
    } else {
      this.selectedRollOut = rowData;
      this.expandRowRequest(event, rowData);
    }
  }

  displayRollOut(): string {
    return JSON.stringify(this.selectedRollOut, null, 1);
  }

  displayColumn(exportData, col): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(exportData, fields);
    const column = col.field;
    switch (column) {
      default:
        return value;
    }
  }

  selectedIndexChangeEvent(event) {
    // console.log('tab' + event);
    this.changeTabAssignTable(event);
  }

  changeTabAssignTable(newIndex: number) {
    this.tabIndex = newIndex;
    this.penTable =  this.tabIndex ? this.pastRolloutTable : this.pendingRolloutTable;
  }

  dump(obj): string {
    return JSON.stringify(obj, null, 1);
  }

  isToDisablePastTab(): boolean {
    return (this.pendingRolloutTable.creatingMode);
  }

  isDeletable(data): boolean {
    if (data.status.state === RolloutRolloutStatus_state.suspended || data.status.state === RolloutRolloutStatus_state.failure || data.status.state === RolloutRolloutStatus_state.success || data.status.state === RolloutRolloutStatus_state['deadline-exceeded']) {
      return true;
    } else {
      return false;
    }
  }

  // override method
  onBulkEditSuccess(veniceObjects: any[], stagingBulkEditAction: IStagingBulkEditAction, successMsg: string, failureMsg: string) {
    this.tableLoading = false;
  }

  onBulkEditFailure(error: Error, veniceObjects: any[], stagingBulkEditAction: IStagingBulkEditAction, successMsg: string, failureMsg: string, ) {
    this.tableLoading = false;
    this.dataObjects = Utility.getLodash().cloneDeepWith(this.dataObjectsBackUp);
  }
}
