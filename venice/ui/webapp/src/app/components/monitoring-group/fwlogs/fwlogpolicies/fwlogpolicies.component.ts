import { ChangeDetectorRef, Component, DoCheck, Input, IterableDiffer, IterableDiffers, OnDestroy, OnInit, ViewChild, ViewEncapsulation } from '@angular/core';
import { Animations } from '@app/animations';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { Utility } from '@app/common/Utility';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { MonitoringFwlogPolicy, IMonitoringFwlogPolicy } from '@sdk/v1/models/generated/monitoring';
import { Table } from 'primeng/table';

@Component({
  selector: 'app-fwlogpolicies',
  templateUrl: './fwlogpolicies.component.html',
  styleUrls: ['./fwlogpolicies.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None
})
export class FwlogpoliciesComponent implements OnInit, OnDestroy, DoCheck {
  @ViewChild('policiesTable') policytable: Table;
  subscriptions = [];

  fwlogPolicies: ReadonlyArray<IMonitoringFwlogPolicy> = [];

  fwlogPoliciesEventUtility: HttpEventUtility<MonitoringFwlogPolicy>;

  bodyIcon: any = {
    margin: {
      top: '9px',
      left: '8px'
    },
    url: '/assets/images/icons/monitoring/icon-firewall-policy-black.svg'
  };

  headerIcon: Icon = {
    margin: {
      top: '0px',
      left: '10px',
    },
    matIcon: 'grid_on'
  };

  arrayDiffers: IterableDiffer<MonitoringFwlogPolicy>;

  displayedPolicies: ReadonlyArray<MonitoringFwlogPolicy> = [];

  expandedRowData: any;
  creatingMode: boolean = false;
  showEditingForm: boolean = false;

  // If we receive new data, but the display is frozen (user editing),
  // this should be set to true so that when user exits editing, we can update the display
  hasNewData: boolean = true;

  // Whether the toolbar buttons should be enabled
  shouldEnableButtons: boolean = true;

  cols: any[] = [
    { field: 'meta.name', header: 'Targets', class: 'fwlogpolicies-column-name', sortable: false },
    { field: 'spec.filter', header: 'Exports', class: 'fwlogpolicies-column-filter', sortable: false, },
    { field: 'spec.targets', header: 'Targets', class: 'fwlogpolicies-column-targets', sortable: false, isLast: true },
  ];

  constructor(protected _controllerService: ControllerService,
    protected _iterableDiffers: IterableDiffers,
    private cdr: ChangeDetectorRef,
    protected _monitoringService: MonitoringService,
  ) {
    this.arrayDiffers = _iterableDiffers.find([]).create(HttpEventUtility.trackBy);
  }

  ngOnInit() {
    this.setDefaultToolbar();
    this.getFwlogPolicies();
    this.setTableData();
  }

  setDefaultToolbar() {
    this._controllerService.setToolbarData({
      buttons: [{
        cssClass: 'global-button-primary fwlogpolicies-button',
        text: 'ADD FIREWALL LOG POLICY',
        computeClass: () => this.shouldEnableButtons ? '' : 'global-button-disabled',
        callback: () => { this.createNewPolicy(); }
      },
      ],
      breadcrumb: [{ label: 'Firewall Logs', url: Utility.getBaseUIUrl() + 'monitoring/fwlogs' },
      { label: 'Firewall Log Policies', url: Utility.getBaseUIUrl() + 'monitoring/fwlogs/fwlogpolicies' }
      ]
    });
  }

  getFwlogPolicies() {
    this.fwlogPoliciesEventUtility = new HttpEventUtility<MonitoringFwlogPolicy>(MonitoringFwlogPolicy);
    this.fwlogPolicies = this.fwlogPoliciesEventUtility.array;
    const subscription = this._monitoringService.WatchFwlogPolicy().subscribe(
      (response) => {
        this.fwlogPoliciesEventUtility.processEvents(response);
      },
      this._controllerService.restErrorHandler('Failed to get Firewall Log Policies')
    );
    this.subscriptions.push(subscription);
  }

  setTableData() {
    /**
     * We copy the data so that the table doesn't
     * automatically update when the input binding is updated
     * This allows us to freeze the table when a user is doing inline
     * editing on a row entry
     */
    const _ = Utility.getLodash();
    const policies = _.cloneDeep(this.fwlogPolicies);
    if (policies == null) {
      this.displayedPolicies = [];
      return;
    }
    this.displayedPolicies = policies;
  }

  createNewPolicy() {
    // If a row is expanded, we shouldnt be able to open a create new policy form
    if (!this.isInEditMode()) {
      this.creatingMode = true;
    }
  }

  /**
   * We check if any of the objects in the array have changed
   * This kind of detection is not automatically done by angular
   * To improve efficiency, we check only the name and last mod time
   * (see trackBy function) instead of checking every object field.
   */
  ngDoCheck() {
    const changes = this.arrayDiffers.diff(this.fwlogPolicies as Array<any>);
    if (changes) {
      if (this.isInEditMode()) {
        this.hasNewData = true;
      } else {
        this.setTableData();
      }
    }
  }

  /**
   * Called when a row expand animation finishes
   * The animation happens when the row expands, and when it collapses
   * If it is expanding, then we are in ediitng mode (set in onUpdateRecord).
   * If it is collapsing, then editingMode should be false, (set in onUpdateRecord).
   * When it is collapsing, we toggle the row on the turbo table
   *
   * This is because we must wait for the animation to complete before toggling
   * the row on the turbo table for a smooth animation.
   * @param  $event Angular animation end event
   */
  rowExpandAnimationComplete($event) {
    if (!this.showEditingForm) {
      // we are exiting the row expand
      this.policytable.toggleRow(this.expandedRowData, event);
      this.expandedRowData = null;
      if (this.hasNewData) {
        this.setTableData();
      }
      // Needed to prevent "ExpressionChangedAfterItHasBeenCheckedError"
      // We force an additional change detection cycle
      this.cdr.detectChanges();
    }
  }

  onUpdateRecord(event, policy) {
    // If in creation mode, don't allow row expansion
    if (this.creatingMode) {
      return;
    }
    if (!this.isInEditMode()) {
      // Entering edit mode
      this.policytable.toggleRow(policy, event);
      this.expandedRowData = policy;
      this.showEditingForm = true;
      this.shouldEnableButtons = false;
    } else {
      this.showEditingForm = false;
      this.shouldEnableButtons = true;
      // We don't untoggle the row here, it will happen when rowExpandAnimationComplete
      // is called.
    }
  }

  onDeleteRecord(event, policy: MonitoringFwlogPolicy) {
    // Should not be able to delete any record while we are editing
    if (this.isInEditMode()) {
      return;
    }

    const msg = 'Deleted policy ' + policy.meta.name;
    const sub = this._monitoringService.DeleteFwlogPolicy(policy.meta.name).subscribe(
      response => {
        this._controllerService.invokeSuccessToaster('Delete Successful', msg);
      },
      this._controllerService.restErrorHandler('Delete Failed')
    );
    this.subscriptions.push(sub);
  }

  creationFormClose() {
    this.creatingMode = false;
  }

  isInEditMode() {
    return this.expandedRowData != null;
  }

  displayColumn(exportData, col): any {
    const fields = col.field.split('.');
    const value = Utility.getObjectValueByPropertyPath(exportData, fields);
    const column = col.field;
    switch (column) {
      case 'spec.targets':
        return value.map(item => item.destination).join(', ');
      default:
        return Array.isArray(value) ? value.join(', ') : value;
    }
  }

  /**
   * Component is about to exit
   */
  ngOnDestroy() {
    // publish event that AppComponent is about to be destroyed
    this._controllerService.publish(Eventtypes.COMPONENT_DESTROY, { 'component': 'Eventpolicy component', 'state': Eventtypes.COMPONENT_DESTROY });
    this.subscriptions.forEach(subscription => {
      subscription.unsubscribe();
    });
  }

}
