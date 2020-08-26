import { AfterViewInit, Component, Input, OnInit, ViewChild, ViewEncapsulation, OnChanges, SimpleChanges, ChangeDetectorRef, ChangeDetectionStrategy } from '@angular/core';
import { Animations } from '@app/animations';
import { Utility } from '@app/common/Utility';
import { FieldselectorComponent } from '@app/components/shared/fieldselector/fieldselector.component';
import { ToolbarButton } from '@app/models/frontend/shared/toolbar.interface';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { IMonitoringAlertDestination, IMonitoringAlertPolicy, MonitoringAlertPolicy, MonitoringAlertPolicySpec, MonitoringAlertDestination } from '@sdk/v1/models/generated/monitoring';
import { SelectItem } from 'primeng';
import { CreationForm } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { ValidatorFn } from '@angular/forms';

@Component({
  selector: 'app-neweventalertpolicy',
  templateUrl: './neweventalertpolicy.component.html',
  styleUrls: ['./neweventalertpolicy.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class NeweventalertpolicyComponent extends CreationForm<IMonitoringAlertPolicy, MonitoringAlertPolicy> implements OnInit, AfterViewInit , OnChanges {

  @ViewChild('fieldSelector', { static: true }) fieldSelector: FieldselectorComponent;

  @Input() destinations: IMonitoringAlertDestination[] = [];
  @Input() existingObjects: MonitoringAlertDestination[] = [];

  alertOptions: SelectItem[] = Utility.convertEnumToSelectItem(MonitoringAlertPolicySpec.propInfo['severity'].enum);

  destinationOptions: SelectItem[] = [];

  oldButtons: ToolbarButton[] = [];

  constructor(protected controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected _monitoringService: MonitoringService,
    protected cdr: ChangeDetectorRef,
  ) {
    super(controllerService, uiconfigsService, cdr, MonitoringAlertPolicy);
  }

  ngOnChanges(changes: SimpleChanges): void {
    this.setDestinationDropDownState();
    super.ngOnChanges(changes);
  }

  postNgInit() {
    if (this.objectData == null) {
      this.newObject.spec.enable = true;
    }

    if (!this.isInline) {
      this.addFieldValidator(this.newObject.$formGroup.get(['meta', 'name']),
        this.isEventAlertPolicyNameValid(this.existingObjects));
    }

    this.newObject.$formGroup.get(['spec', 'resource']).setValue('Event');

    this.destinations.forEach((destination) => {
      this.destinationOptions.push({
        label: destination.meta.name,
        value: destination.meta.name,
      });
    });

    this.setDestinationDropDownState();
  }

  isEventAlertPolicyNameValid(existingObjects: MonitoringAlertDestination[]): ValidatorFn {
    return Utility.isModelNameUniqueValidator(existingObjects, 'event-alert-policy-name');
  }

  setDestinationDropDownState() {
    if (this.newObject && this.destinations && this.destinations.length > 0) {
      this.newObject.$formGroup.get(['spec', 'destinations']).enable();
    } else  if (this.newObject && this.destinations && this.destinations.length === 0) {
      this.newObject.$formGroup.get(['spec', 'destinations']).disable();
    }
    setTimeout(() => {this.cdr.detectChanges(); }, 100);
  }

  // Empty Hook
  isFormValid(): boolean {
    if (Utility.isEmpty(this.newObject.$formGroup.get(['meta', 'name']).value)) {
      this.submitButtonTooltip = 'Error: Name field is empty.';
      return false;
    }
    if (this.newObject.$formGroup.get(['meta', 'name']).invalid)  {
      this.submitButtonTooltip = 'Error: Name field is invalid.';
      return false;
    }
    this.submitButtonTooltip = 'Ready to submit';
    return true;
  }

  setToolbar() {
    this.setCreationButtonsToolbar('CREATE ALERT POLICY', null);
  }

  getObjectValues(): IMonitoringAlertPolicy {
    const obj = this.newObject.getFormGroupValues();
    obj.spec.requirements = this.fieldSelector.getValues();
    return obj;
  }

  createObject(object: IMonitoringAlertPolicy) {
    return this._monitoringService.AddAlertPolicy(object);
  }

  updateObject(newObject: IMonitoringAlertPolicy, oldObject: IMonitoringAlertPolicy) {
    return this._monitoringService.UpdateAlertPolicy(oldObject.meta.name, newObject, null, oldObject);
  }

  generateCreateSuccessMsg(object: IMonitoringAlertPolicy) {
    return 'Created policy ' + object.meta.name;
  }

  generateUpdateSuccessMsg(object: IMonitoringAlertPolicy) {
    return 'Updated policy ' + object.meta.name;
  }

}
