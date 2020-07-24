import { Component, Input, OnInit, ViewEncapsulation, ViewChild, ChangeDetectionStrategy, ChangeDetectorRef } from '@angular/core';
import { Animations } from '@app/animations';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { IMonitoringFwlogPolicy, MonitoringFwlogPolicy, MonitoringFwlogPolicySpec, IMonitoringPSMExportTarget, MonitoringPSMExportTarget, MonitoringFwlogPolicySpec_filter } from '@sdk/v1/models/generated/monitoring';
import { SelectItem, MultiSelect } from 'primeng/primeng';
import { SyslogComponent, ReturnObjectType } from '@app/components/shared/syslog/syslog.component';
import { Utility } from '@app/common/Utility';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { ValidatorFn } from '@angular/forms';
import { CreationPushForm } from '@app/components/shared/pentable/penpushtable.component';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { FwlogpoliciesComponent } from '../fwlogpolicies.component';

@Component({
  selector: 'app-newfwlogpolicy',
  templateUrl: './newfwlogpolicy.component.html',
  styleUrls: ['./newfwlogpolicy.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class NewfwlogpolicyComponent extends CreationPushForm<IMonitoringFwlogPolicy, MonitoringFwlogPolicy> implements OnInit {
  public static LOGOPTIONS_ALL = MonitoringFwlogPolicySpec_filter.all ;
  public static LOGOPTIONS_NONE = MonitoringFwlogPolicySpec_filter.none;
  public static PSM_TARGET = 'psm-target';

  @ViewChild('syslogComponent') syslogComponent: SyslogComponent;

  @Input() maxNewTargets: number;
  @Input() existingObjects: MonitoringFwlogPolicy[] = [];

  maxTargets: number = FwlogpoliciesComponent.MAX_TOTAL_TARGETS;
  filterOptions: SelectItem[] = Utility.convertEnumToSelectItem(MonitoringFwlogPolicySpec.propInfo['filter'].enum, [NewfwlogpolicyComponent.LOGOPTIONS_NONE]);
  filterAllOption: SelectItem = this.filterOptions[0];

  constructor(protected _controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected _monitoringService: MonitoringService,
    protected cdr: ChangeDetectorRef
  ) {
    super(_controllerService, uiconfigsService, cdr, MonitoringFwlogPolicy);
  }

  postNgInit() {
    this.filterOptions.shift();
    // Existing fwl-policies may not have spec['psm-target'],  so we  build it
    if (!this.newObject.spec[NewfwlogpolicyComponent.PSM_TARGET]) {
      this.newObject.spec[NewfwlogpolicyComponent.PSM_TARGET] =  new MonitoringPSMExportTarget(this.newObject.spec[NewfwlogpolicyComponent.PSM_TARGET]);
      this.newObject.setFormGroupValuesToBeModelValues();
    }
    if (this.newObject.$formGroup.get(['spec', 'psm-target', 'enable']).value === null) {
      this.newObject.$formGroup.get(['spec', 'psm-target', 'enable']).setValue(true);
    }
    if (this.isInline) {
      // on edit form, the maxium targets allowed is the current number of targets
      // plus the number of new targets allowed
      const targets = this.newObject.$formGroup.get(['spec', 'targets']).value;
      if (targets) {
        this.maxTargets = this.maxNewTargets + targets.length;
      }
      console.log(this.maxTargets);
    } else {
      this.maxTargets = this.maxNewTargets;
    }
    this.setValidators(this.newObject);
  }

  postViewInit() {
    this.setSyslogTargetRequired();
  }

  // Empty Hook
  isFormValid(): boolean {
    if (Utility.isEmpty(this.newObject.$formGroup.get(['meta', 'name']).value)) {
      this.submitButtonTooltip = 'Policy name is required';
      return false;
    }
    if (!this.isInline) {
      if (!this.newObject.$formGroup.get(['meta', 'name']).valid) {
        this.submitButtonTooltip = 'Policy name is invalid or not unique';
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

  setValidators(newMonitoringFwlogPolicy: MonitoringFwlogPolicy) {
    newMonitoringFwlogPolicy.$formGroup.get(['meta', 'name']).setValidators([
      this.newObject.$formGroup.get(['meta', 'name']).validator,
      this.isNewFlowExportPolicyNameValid(this.existingObjects)]);
  }

  isNewFlowExportPolicyNameValid(existingObjects: IMonitoringFwlogPolicy[]): ValidatorFn {
    return Utility.isModelNameUniqueValidator(existingObjects, 'new-Firewall-Log-Policy-name');
  }

  getObjectValues(): IMonitoringFwlogPolicy {
    const obj = this.newObject.getFormGroupValues();
    const syslogValues = this.syslogComponent.getValues();
    Object.keys(syslogValues).forEach((key) => {
      obj.spec[key] = syslogValues[key];
    });
    return obj;
  }

  onToggleChange() {
    this.setSyslogTargetRequired();
  }

  setSyslogTargetRequired() {
    const value = this.newObject.$formGroup.get(['spec', 'psm-target', 'enable']).value;
    this.syslogComponent.syslogTargetRequired = !value;
    this.cdr.detectChanges();
  }

  setToolbar() {
    this.setCreationButtonsToolbar('CREATE EVENT POLICY',
      UIRolePermissions.monitoringfwlogpolicy_create);
  }

  createObject(object: IMonitoringFwlogPolicy) {
    return this._monitoringService.AddFwlogPolicy(object);
  }

  updateObject(newObject: IMonitoringFwlogPolicy, oldObject: IMonitoringFwlogPolicy) {
    return this._monitoringService.UpdateFwlogPolicy(oldObject.meta.name, newObject, null, oldObject);
  }

  generateCreateSuccessMsg(object: IMonitoringFwlogPolicy) {
    return 'Created policy ' + object.meta.name;
  }

  generateUpdateSuccessMsg(object: IMonitoringFwlogPolicy) {
    return 'Updated policy ' + object.meta.name;
  }
}
