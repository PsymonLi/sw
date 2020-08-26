import { Component, Input, OnInit, ViewEncapsulation, ViewChild, ChangeDetectionStrategy, ChangeDetectorRef, OnChanges, SimpleChanges } from '@angular/core';
import { FormArray } from '@angular/forms';
import { Animations } from '@app/animations';
import { Utility } from '@app/common/Utility';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { IMonitoringAlertDestination, MonitoringAlertDestination, MonitoringAuthConfig, MonitoringPrivacyConfig, MonitoringSNMPTrapServer, MonitoringSyslogExport } from '@sdk/v1/models/generated/monitoring';
import { SelectItem } from 'primeng';
import { SyslogComponent, ReturnObjectType } from '@app/components/shared/syslog/syslog.component';
import { FieldselectorComponent } from '@app/components/shared/fieldselector/fieldselector.component';
import { ValidatorFn } from '@angular/forms';
import { CreationForm } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { DestinationpolicyComponent } from '../destinations.component';

@Component({
  selector: 'app-newdestination',
  templateUrl: './newdestination.component.html',
  styleUrls: ['./newdestination.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class NewdestinationComponent extends CreationForm<IMonitoringAlertDestination, MonitoringAlertDestination> implements OnInit {
  @ViewChild('syslogComponent') syslogComponent: SyslogComponent;
  @ViewChild('fieldSelector') fieldSelector: FieldselectorComponent;

  enableSnmpTrap: boolean = false;
  maxTargets: number = DestinationpolicyComponent.MAX_TOTAL_TARGETS;

  @Input() maxNewTargets: number;
  @Input() isInline: boolean = false;
  @Input() existingObjects: MonitoringAlertDestination[] = [];

  versionOptions: SelectItem[] = Utility.convertEnumToSelectItem(MonitoringSNMPTrapServer.propInfo['version'].enum);

  authAlgoOptions: SelectItem[] = Utility.convertEnumToSelectItem(MonitoringAuthConfig.propInfo['algo'].enum);

  encryptAlgoOptions: SelectItem[] = Utility.convertEnumToSelectItem(MonitoringPrivacyConfig.propInfo['algo'].enum);

  syslogFormatOptions: SelectItem[] = Utility.convertEnumToSelectItem(MonitoringSyslogExport.propInfo['format'].enum);

  // This field should eventually come from the from group once the necessary proto changes go in
  selectedCredentialMethod = 'AUTHTYPE_USERNAMEPASSWORD';

  constructor(
    protected _controllerService: ControllerService,
    protected _monitoringService: MonitoringService,
    protected uiconfigsService: UIConfigsService,
    protected cdr: ChangeDetectorRef
  ) {
    super(_controllerService, uiconfigsService, cdr, MonitoringAlertDestination);
  }

  postNgInit(): void {
    if (this.isInline) {
      this.newObject.$formGroup.get(['meta', 'name']).disable();
      // on edit form, the maxium targets allowed is the current number of targets
      // plus the number of new targets allowed
      if (this.newObject.$formGroup.get(['spec', 'syslog-export']).value) {
        const targets = this.newObject.$formGroup.get(['spec', 'syslog-export', 'targets']).value;
        if (targets) {
          this.maxTargets = this.maxNewTargets + targets.length;
        }
      }
    } else {
      this.maxTargets = this.maxNewTargets;
    }
    this.setValidators(this.newObject);
  }

  isFormValid(): boolean {
    if (Utility.isEmpty(this.newObject.$formGroup.get(['meta', 'name']).value)) {
      this.submitButtonTooltip = 'Error: Name field is empty.';
      return false;
    }
    if (!this.isInline) {
      if (!this.newObject.$formGroup.get(['meta', 'name']).valid)  {
        this.submitButtonTooltip = 'Error: Name field is invalid or not unique';
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

  setValidators(newMonitoringAlertDestination: MonitoringAlertDestination) {
    newMonitoringAlertDestination.$formGroup.get(['meta', 'name']).setValidators([
      this.newObject.$formGroup.get(['meta', 'name']).validator,
      this.isNewDestinationNameValid(this.existingObjects)]);
  }

  isNewDestinationNameValid(existingObjects: IMonitoringAlertDestination[]): ValidatorFn {
    return Utility.isModelNameUniqueValidator(existingObjects, 'newDestination-name');
  }

  getSelectedCredentialMethod(syslog: any): string {
    return syslog.value.target.credentials['auth-type'];
  }

  getObjectValues(): IMonitoringAlertDestination {
    const obj = this.newObject.getFormGroupValues();
    obj.spec['syslog-export'] = this.syslogComponent.getValues();
    return obj;
  }

  addSnmpTrapConfig() {
    const snmpArray = this.newObject.$formGroup.get(['spec', 'snmp-trap-servers']) as FormArray;
    snmpArray.insert(0, new MonitoringSNMPTrapServer().$formGroup);
  }

  removeSnmpTrapConfig(index) {
    const snmpFormArray = this.newObject.$formGroup.get(['spec', 'snmp-trap-servers']) as FormArray;
    if (snmpFormArray.length > 1) {
      snmpFormArray.removeAt(index);
    }
  }

  setToolbar() {
    this.setCreationButtonsToolbar('CREATE DESTINATION',
        UIRolePermissions.monitoringalertdestination_create);
  }

  createObject(object: IMonitoringAlertDestination) {
    return this._monitoringService.AddAlertDestination(object);
  }
  updateObject(newObject: IMonitoringAlertDestination, oldObject: IMonitoringAlertDestination) {
    return this._monitoringService.UpdateAlertDestination(oldObject.meta.name, newObject, null, oldObject);
  }
  generateCreateSuccessMsg(object: IMonitoringAlertDestination): string {
    return 'Created destination ' + object.meta.name;
  }
  generateUpdateSuccessMsg(object: IMonitoringAlertDestination): string {
    return 'Updated destination ' + object.meta.name;
  }
}
