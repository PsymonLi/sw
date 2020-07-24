import { AfterViewInit, Component, EventEmitter, Input, OnInit, Output, ViewChild, ViewEncapsulation, ChangeDetectionStrategy, ChangeDetectorRef } from '@angular/core';
import { Animations } from '@app/animations';
import { FieldselectorComponent } from '@app/components/shared/fieldselector/fieldselector.component';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { MonitoringEventPolicy, IMonitoringEventPolicy, IApiStatus, } from '@sdk/v1/models/generated/monitoring';
import { SyslogComponent, ReturnObjectType } from '@app/components/shared/syslog/syslog.component';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { Utility } from '@app/common/Utility';
import { ValidatorFn } from '@angular/forms';
import { CreationPushForm } from '@app/components/shared/pentable/penpushtable.component';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { EventpolicyComponent } from '../eventpolicy.component';

@Component({
  selector: 'app-neweventpolicy',
  templateUrl: './neweventpolicy.component.html',
  styleUrls: ['./neweventpolicy.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class NeweventpolicyComponent extends CreationPushForm<IMonitoringEventPolicy, MonitoringEventPolicy> implements OnInit, AfterViewInit {
  @ViewChild('fieldSelector') fieldSelector: FieldselectorComponent;
  @ViewChild('syslogComponent') syslogComponent: SyslogComponent;
  @Input() existingObjects: MonitoringEventPolicy[] = [];
  @Input() isInline: boolean = false;
  @Input() maxNewTargets: number;

  maxTargets: number = EventpolicyComponent.MAX_TOTAL_TARGETS;

  constructor(
    protected _controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected _monitoringService: MonitoringService,
    protected cdr: ChangeDetectorRef
  ) {
    super(_controllerService, uiconfigsService, cdr, MonitoringEventPolicy);
  }

  // Empty Hook
  postNgInit() {
    if (this.isInline) {
      this.newObject.$formGroup.get(['meta', 'name']).disable();
      // on edit form, the maxium targets allowed is the current number of targets
      // plus the number of new targets allowed
      const targets = this.newObject.$formGroup.get(['spec', 'targets']).value;
      if (targets) {
        this.maxTargets = this.maxNewTargets + targets.length;
      }
    } else {
      this.maxTargets = this.maxNewTargets;

    }
    this.setValidators(this.newObject);
  }

  // Empty Hook
  isFormValid(): boolean {
    if (Utility.isEmpty(this.newObject.$formGroup.get(['meta', 'name']).value)) {
      this.submitButtonTooltip = 'Eventpolicy name is required';
      return false;
    }

    if (!this.isInline) {
      if (!this.newObject.$formGroup.get(['meta', 'name']).valid) {
        this.submitButtonTooltip = 'Eventpolicy name is invalid or not unique';
        return false;
      }
    }

    if (this.syslogComponent) {
      const syslogFormReturnValue: ReturnObjectType = this.syslogComponent.isSyLogFormValid();
      if (!syslogFormReturnValue.valid) {
        this.submitButtonTooltip = syslogFormReturnValue.errorMessage;
        return false;
      }
    }
    this.submitButtonTooltip = 'Ready to submit';
    return true;
  }

  setValidators(newMonitoringFlowExportPolicy: MonitoringEventPolicy) {
    if (!this.isInline) {
      newMonitoringFlowExportPolicy.$formGroup.get(['meta', 'name']).setValidators([
        this.newObject.$formGroup.get(['meta', 'name']).validator,
        this.isNewFlowExportPolicyNameValid(this.existingObjects)]);
    }
  }

  isNewFlowExportPolicyNameValid(existingObjects: IMonitoringEventPolicy[]): ValidatorFn {
    return Utility.isModelNameUniqueValidator(existingObjects, 'newEvent-Policy-name');
  }

  setToolbar() {
    this.setCreationButtonsToolbar('CREATE EVENT POLICY',
      UIRolePermissions.monitoringeventpolicy_create);
  }

  getObjectValues(): IMonitoringEventPolicy {
    const obj = this.newObject.getFormGroupValues();
    const syslogValues = this.syslogComponent.getValues();
    Object.keys(syslogValues).forEach((key) => {
      obj.spec[key] = syslogValues[key];
    });
    return obj;
  }

  createObject(object: IMonitoringEventPolicy) {
    return this._monitoringService.AddEventPolicy(object);
  }

  updateObject(newObject: IMonitoringEventPolicy, oldObject: IMonitoringEventPolicy) {
    return this._monitoringService.UpdateEventPolicy(oldObject.meta.name, newObject, null, oldObject);
  }

  generateCreateSuccessMsg(object: IMonitoringEventPolicy) {
    return 'Created policy ' + object.meta.name;
  }

  generateUpdateSuccessMsg(object: IMonitoringEventPolicy) {
    return 'Updated policy ' + object.meta.name;
  }

}
