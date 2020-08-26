import { Component, OnInit, Input, Output, DoCheck, EventEmitter, AfterViewInit, ViewEncapsulation, ChangeDetectionStrategy, ChangeDetectorRef,  } from '@angular/core';
import { Animations } from '@app/animations';
import { ISecurityApp, SecurityApp, SecurityALG, SecurityALG_type, SecuritySunrpc, SecurityMsrpc} from '@sdk/v1/models/generated/security';
import { ControllerService } from '@app/services/controller.service';
import { SecurityService } from '@app/services/generated/security.service';
import { SelectItem } from 'primeng';
import { Utility } from '@app/common/Utility';
import { FormArray, FormGroup, AbstractControl, ValidatorFn, ValidationErrors, FormControl } from '@angular/forms';
import { SecurityAppOptions} from '@app/components/security';
import { SecurityProtoPort } from '@sdk/v1/models/generated/search';
import { CreationForm } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { maxValueValidator, minValueValidator } from '@sdk/v1/utils/validators';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';

@Component({
  selector: 'app-newsecurityapp',
  templateUrl: './newsecurityapp.component.html',
  styleUrls: ['./newsecurityapp.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class NewsecurityappComponent extends CreationForm<ISecurityApp, SecurityApp> implements OnInit, AfterViewInit {

  @Input() existingApps: SecurityApp[] = [];
  securityForm: FormGroup;
  secOptions: SelectItem[] = Utility.convertEnumToSelectItem(SecurityAppOptions);
  pickedOption: FormControl = new FormControl();
  securityOptions = SecurityAppOptions;
  algEnumOptions = SecurityALG_type;
  algOptions: SelectItem[] = Utility.convertEnumToSelectItem(SecurityALG.propInfo['type'].enum);

  selectedType = SecurityALG_type.icmp;

  ftpTimeoutTooltip = 'Timeout for this program ID. Should be a valid time duration.\n' +
                    'Example: 1h5m2s, 100ns... ';
  dnsTimeoutTooltip = ' Timeout for DNS Query, default 60s. Should be a valid time duration.\n' +
                    'Example: 1h5m2s, 100ns... ';

  constructor(protected _controllerService: ControllerService,
    protected _securityService: SecurityService,
    protected uiconfigsService: UIConfigsService,
    protected cdr: ChangeDetectorRef,
  ) {
    super(_controllerService, uiconfigsService, cdr, SecurityApp);
  }

  postNgInit() {
    this.secOptions.forEach(option => {
      option.value = option.label;
    });
    if (this.objectData != null) {
      this.setRadio();
    } else {
      this.pickedOption.setValue(SecurityAppOptions.PROTOCOLSANDPORTS);
      this.addFieldValidator(this.newObject.$formGroup.get(['meta', 'name']),
        this.isAppNameValid(this.existingApps));
    }
    this.securityForm = this.newObject.$formGroup;
    this.setUpTargets();
    const dnsTimeout: AbstractControl = this.securityForm.get(['spec', 'alg', 'dns', 'query-response-timeout']);
    this.addFieldValidator(dnsTimeout, this.isTimeoutValid('dnsTimeout'));
    const icmpType: AbstractControl = this.securityForm.get(['spec', 'alg', 'icmp', 'type']);
    this.addFieldValidator(icmpType, minValueValidator( 0));
    this.addFieldValidator(icmpType, maxValueValidator(255));
    const icmpCode: AbstractControl = this.securityForm.get(['spec', 'alg', 'icmp', 'code']);
    this.addFieldValidator(icmpCode, minValueValidator(0));
    this.addFieldValidator(icmpCode, maxValueValidator(18));
    if (this.isInline) {
      const allTargets = this.securityForm.get(['spec', 'proto-ports']) as FormArray;
      allTargets.controls.forEach((formGroup: FormGroup) => {
        const ctrl2: AbstractControl = formGroup.get(['protocol']);
        this.addFieldValidator(ctrl2, this.isProtocolFieldValid());
        const protocol: string = ctrl2.value;
        const ctrl: AbstractControl = formGroup.get(['ports']);
        this.addFieldValidator(ctrl, this.isPortsFieldValid());
        if (!protocol || (protocol.toLowerCase() !== 'tcp' &&
            protocol.toLowerCase() !== 'udp')) {
          ctrl.disable();
        }
      });
    }
  }

  generateCreateSuccessMsg(object: ISecurityApp) {
    return 'Created app ' + object.meta.name;
  }

  generateUpdateSuccessMsg(object: ISecurityApp) {
    return 'Updated app ' + object.meta.name;
  }

  // tcp,icmp,udp, any + 1< x < 255
  updateObject(newObject: ISecurityApp, oldObject: ISecurityApp) {
    return this._securityService.UpdateApp(oldObject.meta.name, newObject, null, oldObject, true, false);
  }

  setToolbar() {
    this.setCreationButtonsToolbar('CREATE APP', UIRolePermissions.securityapp_create);
  }

  isAppNameValid(existingApps: SecurityApp[]): ValidatorFn {
    return Utility.isModelNameUniqueValidator(existingApps, 'security-app-name');
  }

  setUpTargets() {
    const tempProto: any = this.securityForm.get(['spec', 'proto-ports']);
    if (!tempProto.controls || tempProto.controls.length === 0) {
      this.addProtoTarget();
    }
    const tempSun: any = this.securityForm.get(['spec', 'alg', 'sunrpc']);
    if (!tempSun.controls || tempSun.controls.length === 0) {
      this.addSunRPCTarget();
    }
    const tempMSRPC: any = this.securityForm.get(['spec', 'alg', 'msrpc']);
    if (!tempMSRPC.controls || tempMSRPC.controls.length === 0) {
      this.addMSRPCTarget();
    }
  }

  createObject(object: ISecurityApp) {
    return this._securityService.AddApp(object, '', true, false);
  }

  setRadio() {
    if (!this.objectData.spec['alg'].type) {
      this.pickedOption.setValue(SecurityAppOptions.PROTOCOLSANDPORTS);
    } else if (this.objectData.spec['alg'].type &&
      Utility.isValueOrArrayEmpty(this.objectData.spec['proto-ports'])) {
      this.pickedOption.setValue(SecurityAppOptions.ALGONLY);
      this.selectedType = this.objectData.spec.alg.type;
    } else if (this.objectData.spec['alg'].type &&
      !Utility.isValueOrArrayEmpty(this.objectData.spec['proto-ports'])) {
      this.pickedOption.setValue(SecurityAppOptions.BOTH);
      this.selectedType = this.objectData.spec.alg.type;
    } else {
      this.pickedOption.setValue(SecurityAppOptions.PROTOCOLSANDPORTS);
    }
  }

  addProtoTarget() {
    const tempTargets = this.securityForm.get(['spec', 'proto-ports']) as FormArray;
    const newFormGroup: FormGroup = new SecurityProtoPort().$formGroup;
    const ctrl: AbstractControl = newFormGroup.get(['ports']);
    ctrl.disable();
    this.addFieldValidator(ctrl, this.isPortsFieldValid());
    const ctrl2: AbstractControl = newFormGroup.get(['protocol']);
    this.addFieldValidator(ctrl2, this.isProtocolFieldValid());
    tempTargets.push(newFormGroup);
  }

  removeProtoTarget(index) {
    const tempTargets = this.securityForm.get(['spec', 'proto-ports']) as FormArray;
    if (tempTargets.length > 1) {
      tempTargets.removeAt(index);
    }
  }

  addSunRPCTarget() {
    const tempTargets = this.securityForm.get(['spec', 'alg', 'sunrpc']) as FormArray;
    const newFormGroup: FormGroup = new SecuritySunrpc().$formGroup;
    const ctrl: AbstractControl = newFormGroup.get(['timeout']);
    this.addFieldValidator(ctrl, this.isTimeoutValid('sunrpcTimeout'));
    tempTargets.push(newFormGroup);
  }

  removeSunRPCTarget(index) {
    const tempTargets = this.securityForm.get(['spec', 'alg', 'sunrpc']) as FormArray;
    if (tempTargets.length > 1) {
      tempTargets.removeAt(index);
    }
  }

  addMSRPCTarget() {
    const tempTargets = this.securityForm.get(['spec', 'alg', 'msrpc']) as FormArray;
    const newFormGroup: FormGroup = new SecurityMsrpc().$formGroup;
    const ctrl: AbstractControl = newFormGroup.get(['timeout']);
    this.addFieldValidator(ctrl, this.isTimeoutValid('msrpcTimeout'));
    tempTargets.push(newFormGroup);
  }

  removeMSRPCTarget(index) {
    const tempTargets = this.securityForm.get(['spec', 'alg', 'msrpc']) as FormArray;
    if (tempTargets.length > 1) {
      tempTargets.removeAt(index);
    }
  }

  onPickedOptionChange() {
    if (this.pickedOption.value === SecurityAppOptions.ALGONLY) {
      this.selectedType = SecurityALG_type.icmp;
      this.securityForm.get(['spec', 'alg', 'type']).setValue(SecurityALG_type.icmp);
      this.processALGValues();
    }
  }

  processALGValues() {
    if (this.pickedOption.value === SecurityAppOptions.PROTOCOLSANDPORTS) {
      this.resetDNSValues();
      this.resetFTPValues();
      this.resetSunRPC();
      this.resetMSRPC();
      this.resetICMPValues();
      this.resetALG();
      return;
    }
    if (this.selectedType === SecurityALG_type.icmp) {
      this.resetDNSValues();
      this.resetFTPValues();
      this.resetSunRPC();
      this.resetMSRPC();
    } else if (this.selectedType === SecurityALG_type.dns) {
      this.resetICMPValues();
      this.resetFTPValues();
      this.resetSunRPC();
      this.resetMSRPC();
    } else if (this.selectedType === SecurityALG_type.ftp) {
      this.resetICMPValues();
      this.resetDNSValues();
      this.resetSunRPC();
      this.resetMSRPC();
    } else if (this.selectedType === SecurityALG_type.msrpc) {
      this.resetICMPValues();
      this.resetDNSValues();
      this.resetFTPValues();
      this.resetSunRPC();
    } else if (this.selectedType === SecurityALG_type.sunrpc) {
      this.resetICMPValues();
      this.resetDNSValues();
      this.resetFTPValues();
      this.resetMSRPC();
    } else if (this.selectedType === SecurityALG_type.rtsp ||
              this.selectedType === SecurityALG_type.tftp) {
      this.resetICMPValues();
      this.resetDNSValues();
      this.resetFTPValues();
      this.resetSunRPC();
      this.resetMSRPC();
    }
  }

  processFormValues() {
    if (this.pickedOption.value === SecurityAppOptions.ALGONLY) {
      this.resetProto();
    }
    this.processALGValues();
  }

  /*
   * when user choose alg, ui will automatically create proto-ports based on
   * which alg it is. Basically Naple card api demands both alg and proto-ports.
   * Backend does not create default proto-ports by the alg type, they ask ui and
   * rest api to do that.
   * For icmp, user does not need to specify proto-ports. Backend will create
   * protocol: icmp port null for this. That is the only proto-portsbackend creates
   * For ftp: UI creates     Protocol: tcp Port 21
   * For dns: UI creates     Protocol: udp Port 53
   * For tftp: UI creates    Protocol: udp Port 69
   * For rstp: UI creates    Protocol: tcp Port 554
   * For sunrpc and msrpc    user has to specify because protocol can be either ftp or udp
   *                         plus ports should be a range
   */
  onTypeChange($event) {
    this.selectedType = $event;
    if (this.selectedType !== SecurityALG_type.icmp) {
      this.pickedOption.setValue(SecurityAppOptions.BOTH);
      this.resetProto();
      this.addProtoTarget();
      const firstProtGroup: FormGroup = this.securityForm.get(['spec', 'proto-ports', 0]) as FormGroup;
      if (this.selectedType === SecurityALG_type.ftp) {
        firstProtGroup.get(['protocol']).setValue('tcp');
        firstProtGroup.get(['protocol']).updateValueAndValidity();
        firstProtGroup.get(['ports']).enable();
        firstProtGroup.get(['ports']).setValue('21');
      } else if (this.selectedType === SecurityALG_type.dns) {
        firstProtGroup.get(['protocol']).setValue('udp');
        firstProtGroup.get(['protocol']).updateValueAndValidity();
        firstProtGroup.get(['ports']).enable();
        firstProtGroup.get(['ports']).setValue('53');
      } else if (this.selectedType === SecurityALG_type.tftp) {
        firstProtGroup.get(['protocol']).setValue('udp');
        firstProtGroup.get(['protocol']).updateValueAndValidity();
        firstProtGroup.get(['ports']).enable();
        firstProtGroup.get(['ports']).setValue('69');
      } else if (this.selectedType === SecurityALG_type.rtsp) {
        firstProtGroup.get(['protocol']).setValue('tcp');
        firstProtGroup.get(['protocol']).updateValueAndValidity();
        firstProtGroup.get(['ports']).enable();
        firstProtGroup.get(['ports']).setValue('554');
      } else if (this.selectedType === SecurityALG_type.sunrpc) {
        const tempSun: any = this.securityForm.get(['spec', 'alg', 'sunrpc']);
        if (!tempSun.controls || tempSun.controls.length === 0) {
          this.addSunRPCTarget();
        }
      } else if (this.selectedType === SecurityALG_type.msrpc) {
        const tempMSRPC: any = this.securityForm.get(['spec', 'alg', 'msrpc']);
        if (!tempMSRPC.controls || tempMSRPC.controls.length === 0) {
          this.addMSRPCTarget();
        }
      }
    } else {
      this.pickedOption.setValue(SecurityAppOptions.ALGONLY);
    }
  }

  resetProto() {
    const tempproto = this.securityForm.get(['spec', 'proto-ports']) as FormArray;
    while (tempproto && tempproto.length !== 0) {
      tempproto.removeAt(0);
    }
  }

  resetALG() {
    this.securityForm.get(['spec', 'alg', 'type']).setValue(null);
  }

  resetICMPValues() {
    this.securityForm.get(['spec', 'alg', 'icmp', 'type']).setValue(null);
    this.securityForm.get(['spec', 'alg', 'icmp', 'code']).setValue(null);
  }

  resetDNSValues() {
    this.securityForm.get(['spec', 'alg', 'dns', 'drop-multi-question-packets']).setValue(null);
    this.securityForm.get(['spec', 'alg', 'dns', 'drop-large-domain-name-packets']).setValue(null);
    this.securityForm.get(['spec', 'alg', 'dns', 'drop-long-label-packets']).setValue(null);
    this.securityForm.get(['spec', 'alg', 'dns', 'max-message-length']).setValue(null);
    this.securityForm.get(['spec', 'alg', 'dns', 'query-response-timeout']).setValue(null);
  }

  resetFTPValues() {
    this.securityForm.get(['spec', 'alg', 'ftp', 'allow-mismatch-ip-address']).setValue(null);
  }

  resetSunRPC() {
    const tempSunrpc = this.securityForm.get(['spec', 'alg', 'sunrpc']) as FormArray;
    while (tempSunrpc && tempSunrpc.length !== 0) {
      tempSunrpc.removeAt(0);
    }
  }

  resetMSRPC() {
    const tempMSRPC = this.securityForm.get(['spec', 'alg', 'msrpc']) as FormArray;
    while (tempMSRPC && tempMSRPC.length !== 0) {
      tempMSRPC.removeAt(0);
    }
  }

  saveObject() {
    this.processFormValues();
    super.saveObject();
  }

  isFormValid(): boolean {
    if (Utility.isEmpty(this.newObject.$formGroup.get(['meta', 'name']).value)) {
      this.submitButtonTooltip = 'Error: Name field is empty.';
      return false;
    }
    if (this.newObject.$formGroup.get(['meta', 'name']).invalid)  {
      this.submitButtonTooltip = 'Error: Name field is invalid.';
      return false;
    }

    if (this.pickedOption.value === SecurityAppOptions.PROTOCOLSANDPORTS) {
      const isValid: boolean = this.validatingProtoInputs() && !this.isProtoInputsEmpty();
      if (isValid) {
        this.submitButtonTooltip = 'Ready to submit';
      }
      return isValid;
    }
    if (this.pickedOption.value === SecurityAppOptions.ALGONLY) {
      return this.validatingALGinputs();
    }
    if (!this.validatingALGinputs()) {
      return false;
    }
    if (!this.validatingProtoInputs()) {
      return false;
    }
    if (this.selectedType === SecurityALG_type.icmp) {
      this.submitButtonTooltip = 'Ready to submit';
      return true;
    }

    const result: boolean = this.isProtoInputsEmpty();
    if (!result) {
      this.submitButtonTooltip = 'Ready to submit';
    }
    return !result;
  }

  isProtoInputsEmpty() {
    const tempProto: FormArray = this.controlAsFormArray(
      this.securityForm.get(['spec', 'proto-ports']));
    for (let i = 0; i < tempProto.controls.length; i++) {
      const formGroup: FormGroup = tempProto.controls[i] as FormGroup;
      if (formGroup.value && formGroup.value.protocol) {
        if (formGroup.value.protocol.toLowerCase() !== 'tcp' &&
            formGroup.value.protocol.toLowerCase() !== 'udp') {
          return false;
        }
        if (formGroup.valid) {
          return false;
        }
      }
    }
    this.submitButtonTooltip = 'Error: Protocols and ports are empty.';
    return true;
  }

  validatingProtoInputs() {
    const tempProto: FormArray = this.securityForm.get(['spec', 'proto-ports']) as FormArray;
    const formGroups: FormGroup[] = tempProto.controls as FormGroup[];
    for (let i = 0; i < formGroups.length; i++) {
      const formGroup: FormGroup = formGroups[i];
      if (Utility.isEmpty(formGroup.value.protocol)) {
        this.submitButtonTooltip = 'Error: Protocol ' + (i + 1) + ' is empty.';
        return false;
      }
      if (formGroup.value && formGroup.value.protocol &&
          (formGroup.value.protocol.toLowerCase() === 'tcp' ||
          formGroup.value.protocol.toLowerCase() === 'udp')
          && Utility.isEmpty(formGroup.value.ports, true)) {
        this.submitButtonTooltip = 'Error: Port ' + (i + 1) + ' is empty.';
        return false;
      }
      if (!formGroup.valid) { // validate error
        this.submitButtonTooltip = 'Error: Protocal and port ' + (i + 1) + ' are invalid.';
        return false;
      }
    }
    return true;
  }

  validatingALGinputs() {
    if (this.selectedType === SecurityALG_type.icmp) {
      if (!this.securityForm.get(['spec', 'alg', 'icmp']).valid) {
        this.submitButtonTooltip = 'Error: ICMP values are invalid.';
        return false;
      }
    } else if (this.selectedType === SecurityALG_type.dns) {
      if (!this.securityForm.get(['spec', 'alg', 'dns']).valid) {
        this.submitButtonTooltip = 'Error: DNS values are invalid.';
        return false;
      }
    } else if (this.selectedType === SecurityALG_type.sunrpc) {
      const formArray: FormArray = this.controlAsFormArray(
        this.securityForm.get(['spec', 'alg', 'sunrpc']));
      for (let i = 0; i < formArray.controls.length; i++) {
        const formGroup: FormGroup = formArray.controls[i] as FormGroup;
        if (!formGroup.value || !formGroup.value['program-id'] ||
            !formGroup.value.timeout) {
          this.submitButtonTooltip = 'Error: SUNRPC program ID or timeout ' + (i + 1) + ' is empty.';
          return false;
        }
        if (!formGroup.valid) {
          this.submitButtonTooltip = 'Error: SUNRPC program ID or timeout ' + (i + 1) + ' is invalid.';
          return false;
        }
      }
    } else if (this.selectedType === SecurityALG_type.msrpc) {
      const formArray: FormArray = this.controlAsFormArray(
        this.securityForm.get(['spec', 'alg', 'msrpc']));
      for (let i = 0; i < formArray.controls.length; i++) {
        const formGroup: FormGroup = formArray.controls[i] as FormGroup;
        if (!formGroup.value || !formGroup.value['program-uuid'] ||
            !formGroup.value.timeout) {
          this.submitButtonTooltip = 'Error: MSRPC program UUID or timeout ' + (i + 1) + ' is empty.';
          return false;
        }
        if (!formGroup.valid) {
          this.submitButtonTooltip = 'Error: MSRPC program UUID or timeout ' + (i + 1) + ' is invalid.';
          return false;
        }
      }
    }
    return true;
  }

  onProtocolChange(formGroup: FormGroup) {
    const protocol = formGroup.get(['protocol']).value;
    const portsCtrl: FormControl = formGroup.get(['ports']) as FormControl;
    const shouldEnable: boolean = protocol &&
        (protocol.trim().toLowerCase() === 'tcp' ||
          protocol.trim().toLowerCase() === 'udp' ||
          protocol.trim().toLowerCase() === 'any');
    if (shouldEnable) {
      portsCtrl.enable();
    } else {
      portsCtrl.setValue(null);
      portsCtrl.disable();
    }
  }

  isPortRequired(formGroup: FormGroup): boolean {
    const protocol = formGroup.get(['protocol']).value;
    return protocol && (protocol.trim().toLowerCase() === 'tcp' ||
        protocol.trim().toLowerCase() === 'udp');
  }

  isProtocolFieldValid(): ValidatorFn {
    return (control: AbstractControl): ValidationErrors | null => {
      let val: string = control.value;
      if (!val || !val.trim()) {
        return null;
      }
      val = val.trim();
      if (Utility.isProtocolValid(val)) {
        return null;
      }
      return {
        fieldProtocol: {
          required: false,
          message: 'Invalid Protocol. Only tcp, udp, icmp and any are allowed.'
        }
      };
    };
  }

  isPortsFieldValid(): ValidatorFn {
    return (control: AbstractControl): ValidationErrors | null => {
      const val: string = control.value;
      if (!val || !val.trim()) {
        return null;
      }
      const errorMsg = Utility.isPortsValid(val);
      if (errorMsg) {
        return {
          fieldPort: {
            required: false,
            message: errorMsg
          }
        };
      }
      return null;
    };
  }

}

