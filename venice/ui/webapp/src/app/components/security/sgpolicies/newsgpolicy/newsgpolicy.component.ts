import { Component, OnInit, ViewEncapsulation, AfterViewInit, Input, Output, EventEmitter, ChangeDetectionStrategy, ChangeDetectorRef } from '@angular/core';
import { ISecurityNetworkSecurityPolicy, SecurityNetworkSecurityPolicy, SecuritySGRule, SecurityProtoPort, SecurityApp, SecuritySecurityGroup, ISecuritySecurityGroup } from '@sdk/v1/models/generated/security';
import { ControllerService } from '@app/services/controller.service';
import { SecurityService } from '@app/services/generated/security.service';
import { CreationForm } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { Animations } from '@app/animations';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { OrderedItem } from '@app/components/shared/orderedlist/orderedlist.component';
import { Utility } from '@app/common/Utility';
import { SelectItem } from 'primeng/api';
import { IPUtility } from '@app/common/IPUtility';
import { AbstractControl, FormGroup, FormArray, FormControl, ValidatorFn, ValidationErrors } from '@angular/forms';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';

@Component({
  selector: 'app-newsgpolicy',
  templateUrl: './newsgpolicy.component.html',
  styleUrls: ['./newsgpolicy.component.scss'],
  encapsulation: ViewEncapsulation.None,
  animations: Animations,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class NewsgpolicyComponent extends CreationForm<ISecurityNetworkSecurityPolicy, SecurityNetworkSecurityPolicy> implements OnInit, AfterViewInit {
  // TODO: Add validation
  // TODO: either prevent adding new rule while one is incomplete,
  //       or add indicator that a rule is invalid/incomplete

  IPS_LABEL: string = 'IP Addresses';
  IPS_ERRORMSG: string = 'Invalid IP addresses';
  IPS_TOOLTIP: string = 'Type in ip address and hit enter or space key to add more.';

  ATTACH_TENANT: string = 'tenant';
  ATTACH_SG: string = 'securityGroups';

  PROTO_PORTS_OPTION: string = 'proto-ports';
  APPS_OPTION: string = 'apps';

  createButtonTooltip: string = '';

  attachOptions = [
    { label: 'TENANT', value: this.ATTACH_TENANT },
    // Product Release-A constraints
    // { label: 'SECURITY GROUPS', value: this.ATTACH_SG },
  ];

  selectedAttachOption: string = this.ATTACH_TENANT;

  fakeSecurityGroups = [
    { label: 'security-group-1', value: 'security-group-1' },
    { label: 'security-group-2', value: 'security-group-2' },
    { label: 'security-group-3', value: 'security-group-3' },
  ];

  fakeApps = [
    { label: 'app-1', value: 'security-group-1' },
    { label: 'app-2', value: 'security-group-2' },
    { label: 'app-3', value: 'security-group-3' },
  ];

  protoAppOptions = [
    { label: 'PROTO-PORTS', value: this.PROTO_PORTS_OPTION },
    { label: 'APPS', value: this.APPS_OPTION },
  ];

  repeaterAnimationEnabled = true;

  actionEnum = SecuritySGRule.propInfo.action.enum;
  actionOptions: SelectItem[] = Utility.convertEnumToSelectItem(SecuritySGRule.propInfo.action.enum);

  rules: OrderedItem<{ rule: SecuritySGRule, selectedProtoAppOption: FormControl }>[] = [];

  sgpolicyOptions;

  // TODO: must be a number
  // TODO: must be required
  ruleNumberEditControl = new FormControl('');

  // Angular has an issue with stuttering the animation
  // for collapsing (exit animation) after a drag and drop event
  // To smooth the experience, we disable animation on that index only
  indexToSkipAnimation: number;

  @Input() isInline: boolean = false;
  @Input() existingObjects: ISecurityNetworkSecurityPolicy[] = [];
  @Input() securityAppOptions: SelectItem[] = [];
  @Input() securityGroupOptions: SelectItem[] = [];
  @Input() ipOptions: any[] = [];
  @Input() objectData: ISecurityNetworkSecurityPolicy;
  @Output() formClose: EventEmitter<any> = new EventEmitter();

  constructor(protected _controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected securityService: SecurityService,
    protected cdr: ChangeDetectorRef
  ) {
    super(_controllerService, uiconfigsService, cdr, SecurityNetworkSecurityPolicy);
  }

  // Empty Hook
  postNgInit() {
    if (this.isInline) {
      // TODO: comment out me and implement editing policy
      this.objectData.spec.rules.forEach((rule: SecuritySGRule) => {
        const uiIRule = new SecuritySGRule(rule);
        let selectedProtoAppOption = this.PROTO_PORTS_OPTION;
        if (uiIRule.$formGroup.get(['apps']).value.length > 0) {
          selectedProtoAppOption = this.APPS_OPTION;
        }
        const formCtrl = new FormControl();
        formCtrl.setValue(selectedProtoAppOption);
        this.rules.push({
          id: Utility.s4(),
          data: { rule: uiIRule, selectedProtoAppOption: formCtrl },
          inEdit: false,
        });
      });
      this.rules.forEach((r, idx) => {
        if ((r.data.rule.$formGroup.get('proto-ports') as FormArray).length === 0) {
          this.addProtoTarget(idx);
        }
      });
    }

    if (this.rules.length === 0) {
      this.addRule();
    }

    this.setValidationRules();
  }

  setValidationRules() {
    this.newObject.$formGroup.get(['meta', 'name']).setValidators([
      this.newObject.$formGroup.get(['meta', 'name']).validator,
      this.isNewNetworkSecurityPolicyNameValid(this.existingObjects)]);
  }

  isNewNetworkSecurityPolicyNameValid(existingObjects: ISecurityNetworkSecurityPolicy[]): ValidatorFn {
    // checks if name field is valid
    return Utility.isModelNameUniqueValidator(existingObjects, 'newPolicy-name');
  }

  // Empty Hook
  isFormValid() {
    if (Utility.isEmpty(this.newObject.$formGroup.get(['meta', 'name']).value)) {
      this.submitButtonTooltip = 'Error: Name field is empty.';
      return false;
    }
    if (this.newObject.$formGroup.get(['meta', 'name']).invalid)  {
      this.submitButtonTooltip = 'Error: Name field is invalid.';
      return false;
    }

    for (let i = 0; i < this.rules.length; i++) {
      const rule: SecuritySGRule = this.rules[i].data.rule;
      const fromIpField: AbstractControl = rule.$formGroup.get(['from-ip-addresses']);
      if (Utility.isEmpty(fromIpField.value)) {
        this.submitButtonTooltip =
          'Error: Rule ' + (i + 1) + ' source IP addresses are empty.';
        return false;
      }
      if (!fromIpField.valid) {
        this.submitButtonTooltip =
          'Error: Rule ' + (i + 1) + ' source IP addresses are invalid.';
        return false;
      }
      const toIpField: AbstractControl = rule.$formGroup.get(['to-ip-addresses']);
      if (Utility.isEmpty(toIpField.value)) {
        this.submitButtonTooltip =
          'Error: Rule ' + (i + 1) + ' destination IP addresses are empty.';
        return false;
      }
      if (!toIpField.valid) {
        this.submitButtonTooltip =
          'Error: Rule ' + (i + 1) + ' destination IP addresses are invalid.';
        return false;
      }
      if (this.rules[i].data.selectedProtoAppOption.value === this.APPS_OPTION) {
        const appField: AbstractControl = rule.$formGroup.get(['apps']);
        if (Utility.isEmpty(appField.value) || appField.value.length === 0) {
          this.submitButtonTooltip =
            'Error: Rule ' + (i + 1) + ' applications are empty.';
          return false;
        }
      }
      if (this.rules[i].data.selectedProtoAppOption.value === this.PROTO_PORTS_OPTION) {
        const protoArr: FormArray = rule.$formGroup.get(['proto-ports']) as FormArray;
        if (protoArr.length === 0) {
          this.submitButtonTooltip =
            'Error: Rule ' + (i + 1) + ' protocol-ports are empty.';
          return false;
        }
        for (let j = 0; j < protoArr.length; j++) {
          const protocolField: AbstractControl = protoArr.controls[j].get(['protocol']);
          if (Utility.isEmpty(protocolField.value)) {
            this.submitButtonTooltip =
              'Error: Rule ' + (i + 1) + ' protocol ' + (j + 1) + ' is empty.';
            return false;
          }
          if (!protocolField.valid) {
            this.submitButtonTooltip =
            'Error: Rule ' + (i + 1) + ' protocol ' + (j + 1) + ' is invalid.';
            return false;
          }
          const portsField: AbstractControl = protoArr.controls[j].get(['ports']);
          if (protocolField.value &&
                (protocolField.value.toLowerCase() === 'tcp' ||
                protocolField.value.toLowerCase() === 'udp')) {
            if (Utility.isEmpty(portsField.value)) {
              this.submitButtonTooltip =
                'Error: Rule ' + (i + 1) + ' ports ' + (j + 1) + ' are empty.';
              return false;
            }
            if (!portsField.valid) {
              this.submitButtonTooltip =
              'Error: Rule ' + (i + 1) + ' ports ' + (j + 1) + ' are invalid.';
              return false;
            }
          }
          if (protocolField.value === 'any') {
            if (!portsField.valid) {
              this.submitButtonTooltip =
              'Error: Rule ' + (i + 1) + ' ports ' + (j + 1) + ' are invalid.';
              return false;
            }
          }
        }
      }
    }
    this.submitButtonTooltip = 'Ready to submit';
    return true;
  }

  setToolbar() {
    this.setCreationButtonsToolbar('CREATE POLICY',
      UIRolePermissions.securitynetworksecuritypolicy_create);
  }

  createObject(object: ISecurityNetworkSecurityPolicy) {
    return this.securityService.AddNetworkSecurityPolicy(object);
  }

  updateObject(newObject: ISecurityNetworkSecurityPolicy, oldObject: ISecurityNetworkSecurityPolicy) {
    return this.securityService.UpdateNetworkSecurityPolicy(oldObject.meta.name, newObject, null, oldObject);
  }

  generateCreateSuccessMsg(object: ISecurityNetworkSecurityPolicy) {
    return 'Created policy ' + object.meta.name;
  }

  generateUpdateSuccessMsg(object: ISecurityNetworkSecurityPolicy) {
    return 'Updated policy ' + object.meta.name;
  }

  getObjectValues() {
    const currValue = this.newObject.getFormGroupValues();
    if (this.selectedAttachOption === this.ATTACH_TENANT) {
      currValue.spec['attach-tenant'] = true;
    }
    currValue.spec.rules = this.rules.map(r => {
      const formValues = r.data.rule.getFormGroupValues();
      if (r.data.selectedProtoAppOption.value === this.PROTO_PORTS_OPTION) {
        formValues['apps'] = null;
        // trim protocol value vs-1569
        if (formValues['proto-ports'] && formValues['proto-ports'].length > 0) {
          for (let i = 0; i < formValues['proto-ports'].length; i++) {
            const item = formValues['proto-ports'][i];
            if (item.protocol) {
              item.protocol = item.protocol.trim();
            }
          }
        }
      } else {
        formValues['proto-ports'] = null;
      }
      return formValues;
    });
    return currValue;
  }

  addRule() {
    const rule = new SecuritySGRule();
    const formCtrl = new FormControl();
    formCtrl.setValue(this.PROTO_PORTS_OPTION);
    this.rules.push({
      id: Utility.s4(),
      data: { rule: rule, selectedProtoAppOption: formCtrl },
      inEdit: false,
    });
    this.rules.forEach( (r, i) => {
      if (i === this.rules.length - 1) {
        r.inEdit = true;
        if ((r.data.rule.$formGroup.get('proto-ports') as FormArray).length === 0) {
          this.addProtoTarget(this.rules.length - 1);
        }
      } else {
        r.inEdit = false;
      }
    });
  }

  deleteRule(index) {
    this.rules.splice(index, 1);
  }

  isValidIP(ip: string) {
    if (ip && ip.trim() === 'any') {
      return true;
    }
    return IPUtility.isValidIPWithOptionalMask(ip);
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

  isPortRequired(formGroup: any): boolean {
    const protocol: string = formGroup.get(['protocol']).value;
    const portsCtrl: FormControl = formGroup.get(['ports']);
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
    const val = portsCtrl.value;
    if (val && val.trim()) {
      return false;
    }
    return protocol && (protocol.trim().toLowerCase() === 'tcp' ||
        protocol.trim().toLowerCase() === 'udp');
  }

  addProtoTarget(ruleIndex: number) {
    const tempTargets = this.rules[ruleIndex].data.rule.$formGroup.get(['proto-ports']) as FormArray;
    const newFormGroup: FormGroup = new SecurityProtoPort().$formGroup;
    const ctrl: AbstractControl = newFormGroup.get(['ports']);
    ctrl.disable();
    this.addFieldValidator(ctrl, this.isPortsFieldValid());
    const ctrl2: AbstractControl = newFormGroup.get(['protocol']);
    this.addFieldValidator(ctrl2, this.isProtocolFieldValid());
    tempTargets.insert(ruleIndex + 1, newFormGroup);
  }

  removeProtoTarget(ruleIndex: number, index: number) {
    const tempTargets = this.rules[ruleIndex].data.rule.$formGroup.get(['proto-ports']) as FormArray;
    if (tempTargets.length > 1) {
      tempTargets.removeAt(index);
    }
  }

  displayArrayField(rule, field) {
    if (field === 'action') {
      return this.actionEnum[rule.rule.$formGroup.get('action').value];
    }
    if (field === 'proto-ports') {
      const protoPorts = rule.rule.$formGroup.get(field).value.filter((entry) => {
        return entry.protocol != null;
      }).map((entry) => {
        if (entry.ports == null || entry.ports.length === 0) {
          return entry.protocol;
        }
        return entry.protocol + '/' + entry.ports;
      });
      return protoPorts.join(', ');
    }
    if (field === 'apps') {
      return rule.rule.$formGroup.get(field).value.join(', ');
    }
    return rule.rule.$formGroup.get(field).value.join(', ');
  }
}
