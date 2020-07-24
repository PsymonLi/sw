import { Component, OnInit, Input, ViewEncapsulation, ViewChild } from '@angular/core';
import { Animations } from '@app/animations';
import { SelectItem } from 'primeng/primeng';
import { Utility } from '@app/common/Utility';
import { MonitoringSyslogExport, MonitoringExternalCred, IMonitoringSyslogExport, MonitoringExportConfig, MonitoringSyslogExportConfig } from '@sdk/v1/models/generated/monitoring';
import { FormArray, FormGroup, AbstractControl, ValidatorFn, ValidationErrors } from '@angular/forms';
import { BaseComponent } from '@app/components/base/base.component';
import { ControllerService } from '@app/services/controller.service';
import { IPUtility } from '@app/common/IPUtility';

export interface ReturnObjectType {
  errorMessage: string;
  valid: boolean;
}

@Component({
  selector: 'app-syslog',
  templateUrl: './syslog.component.html',
  styleUrls: ['./syslog.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None
})
export class SyslogComponent extends BaseComponent implements OnInit {
  @Input() syslogExport: IMonitoringSyslogExport;
  @Input() showSyslogOptions: boolean = true;
  @Input() showSyslogPrefix: boolean = true;
  @Input() showTargets: boolean = true;
  @Input() targetTransport: String = '<protocol>/<port>';
  @Input() gatewayLabel: String = 'Gateway';
  @Input() formatOptions: SelectItem[] = Utility.convertEnumToSelectItem(MonitoringSyslogExport.propInfo['format'].enum);
  @Input() maxTargets: number = -1;
  @Input() customizedValidator: () => boolean = null;
  @Input() isInline: boolean;
  @Input() syslogFieldsetWidth: string;

  syslogTargetRequired: boolean = true;

  syslogServerForm: FormGroup;

  syslogOverrideOptions: SelectItem[] = Utility.convertEnumToSelectItem(MonitoringSyslogExportConfig.propInfo['facility-override'].enum);

  syslogCredentialOptions: SelectItem[] = Utility.convertEnumToSelectItem(MonitoringExternalCred.propInfo['auth-type'].enum);

  targets: any;

  constructor(protected _controllerService: ControllerService) {
    super(_controllerService);
  }

  ngOnInit() {
    let syslogExport;
    if (this.syslogExport != null) {
      syslogExport = new MonitoringSyslogExport(this.syslogExport);
    } else {
      syslogExport = new MonitoringSyslogExport();
    }
    this.syslogServerForm = syslogExport.$formGroup;
    const targets: any = this.syslogServerForm.get('targets');
    // When there is no target, we will add an target. We set validators for all targets.
    if (targets.controls.length === 0) {
      this.addTarget();
    } else {
      this.setValidatorToTargets();
    }
    this.targets = (<any>this.syslogServerForm.get(['targets'])).controls;
  }

  getValues(): IMonitoringSyslogExport {
    // remove syslog configs if they are off
    if (this.showSyslogOptions) {
      return this.syslogServerForm.value;
    } else {
      const _ = Utility.getLodash();
      // Cloning so that we don't change the form group object
      const formValues = _.cloneDeep(this.syslogServerForm.value);
      delete formValues.config;
      return formValues;
    }
  }

  getSelectedCredentialMethod(index: number): string {
    return this.syslogServerForm.get(['targets', index, 'credentials', 'auth-type']).value;
  }

  addTarget() {
    const targets = this.syslogServerForm.get('targets') as FormArray;
    targets.insert(targets.length, new MonitoringExportConfig().$formGroup);
    this.setValidatorToTargets();
  }

  setValidatorToTargets() {
    const targetArr = (<any>this.syslogServerForm.get(['targets'])).controls;
    for (let i = 0; i < targetArr.length; i++) {
      this.syslogServerForm.get(['targets', i]).get('transport').setValidators([
        this.isTransportFieldValid()]);
      this.syslogServerForm.get(['targets', i]).get('gateway').setValidators([
        IPUtility.isValidIPValidator]);
      this.syslogServerForm.get(['targets', i]).get('destination').setValidators([
        IPUtility.isValidIPValidator]);
    }
  }

  removeTarget(index) {
    const targets = this.syslogServerForm.get('targets') as FormArray;
    if (targets.length > 1) {
      targets.removeAt(index);
    }
  }

  isSyslogValueEmpty(val) {
    return Utility.isEmpty(val) || val.trim().length === 0;
  }

  isFieldDestinationRequired (target): boolean {
    if (this.syslogTargetRequired) {
      return true;
    }
    return !this.isSyslogValueEmpty(target.controls['transport'].value) ||
      !this.isSyslogValueEmpty(target.controls['gateway'].value);
  }

  isFieldTransportRequired (target): boolean {
    if (this.syslogTargetRequired) {
      return true;
    }
    return !this.isSyslogValueEmpty(target.controls['destination'].value) ||
      !this.isSyslogValueEmpty(target.controls['gateway'].value);
  }

  isSyLogFormValid(): ReturnObjectType {
    const returnObject: ReturnObjectType = {
      errorMessage: '',
      valid: true
    };

    const formArray: FormArray = this.syslogServerForm.get('targets') as FormArray;
    for (let i = 0; i < formArray.controls.length; i++) {
      const target: FormGroup = formArray.controls[i] as FormGroup;
      const destination = target.get('destination').value;
      const gateway = target.get('gateway').value;
      const transport = target.get('transport').value;
      if (this.syslogTargetRequired) {
        if (this.isSyslogValueEmpty(destination)) {
          return {
            errorMessage: 'Destination of target ' + (i + 1) + ' is required.',
            valid: false
          };
        }
        if (this.isSyslogValueEmpty(transport)) {
          return {
            errorMessage: 'Transport of target ' + (i + 1) + ' is required.',
            valid: false
          };
        }
      } else {
        if (this.isSyslogValueEmpty(destination) &&
            (!this.isSyslogValueEmpty(gateway) || !this.isSyslogValueEmpty(transport))) {
          return {
            errorMessage: 'Destination of target ' + (i + 1) + ' is required.',
            valid: false
          };
        }
        if (this.isSyslogValueEmpty(transport) &&
          (!this.isSyslogValueEmpty(gateway) || !this.isSyslogValueEmpty(destination))) {
          return {
            errorMessage: 'Transport of target ' + (i + 1) + ' is required.',
            valid: false
          };
        }
      }
      if (target.get('destination').invalid) {
        return {
          errorMessage: 'Destination of target ' + (i + 1) + ' is invalid.',
          valid: false
        };
      }
      if (target.get('gateway').invalid) {
        return {
          errorMessage: 'Gateway of target ' + (i + 1) + ' is invalid.',
          valid: false
        };
      }
      if (target.get('transport').invalid) {
        return {
          errorMessage: 'Transport of target ' + (i + 1) + ' is invalid.',
          valid: false
        };
      }
    }

    return returnObject;
  }

  isTransportFieldValid(): ValidatorFn {
    return (control: AbstractControl): ValidationErrors | null => {
      const val: string = control.value;
      let protocolVal: string;
      let port: string;
      if (val) {
        protocolVal = val.split('/')[0];
        port = val.split('/')[1];
      }
      if (!val) {
        return null;
      }
      if ((protocolVal.toLowerCase() === 'udp') && (val.includes('/')) && (Utility.isPortValid(port))) {
        return null;
      }
      return {
        fieldProtocol: {
          required: false,
          message: 'Invalid Protocol. Only udp is allowed, port should be a number between 1 to 65536.'
        }
      };
    };
  }
}
