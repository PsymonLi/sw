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

  constructor(protected _controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected securityService: SecurityService,
    protected cdr: ChangeDetectorRef
  ) {
    super(_controllerService, uiconfigsService, SecurityNetworkSecurityPolicy);
  }

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

  selectedProtoAppOption: string = this.PROTO_PORTS_OPTION;

  repeaterAnimationEnabled = true;

  actionEnum = SecuritySGRule.propInfo.action.enum;
  actionOptions: SelectItem[] = Utility.convertEnumToSelectItem(SecuritySGRule.propInfo.action.enum);

  rules: OrderedItem<{ rule: SecuritySGRule, selectedProtoAppOption: string }>[] = [];

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


  getClassName() {
    return this.constructor.name;
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
        this.rules.push({
          id: Utility.s4(),
          data: { rule: uiIRule, selectedProtoAppOption: selectedProtoAppOption },
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
    // Add a default protocol port if its the default option
    // if (this.selectedProtoAppOption === this.PROTO_PORTS_OPTION)
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
      return false;
    }
    if (!this.newObject.$formGroup.get(['meta', 'name']).disabled &&
        !this.newObject.$formGroup.get(['meta', 'name']).valid)  {
      return false;
    }
    for (let i = 0; i < this.rules.length; i++) {
      const rule: SecuritySGRule = this.rules[i].data.rule;
      const fromIpField: AbstractControl = rule.$formGroup.get(['from-ip-addresses']);
      if (Utility.isEmpty(fromIpField.value)) {
        this.createButtonTooltip =
          'Error: Rule ' + (i + 1) + ' source IP addresses are empty.';
        return false;
      }
      if (!fromIpField.valid) {
        this.createButtonTooltip =
          'Error: Rule ' + (i + 1) + ' source IP addresses are invalid.';
        return false;
      }
      const toIpField: AbstractControl = rule.$formGroup.get(['to-ip-addresses']);
      if (Utility.isEmpty(toIpField.value)) {
        this.createButtonTooltip =
          'Error: Rule ' + (i + 1) + ' destination IP addresses are empty.';
        return false;
      }
      if (!toIpField.valid) {
        this.createButtonTooltip =
          'Error: Rule ' + (i + 1) + ' destination IP addresses are invalid.';
        return false;
      }
      if (this.rules[i].data.selectedProtoAppOption === this.APPS_OPTION) {
        const appField: AbstractControl = rule.$formGroup.get(['apps']);
        if (Utility.isEmpty(appField.value) || appField.value.length === 0) {
          this.createButtonTooltip =
            'Error: Rule ' + (i + 1) + ' applications are empty.';
          return false;
        }
      }
      if (this.rules[i].data.selectedProtoAppOption === this.PROTO_PORTS_OPTION) {
        const protoArr: FormArray = rule.$formGroup.get(['proto-ports']) as FormArray;
        if (protoArr.length === 0) {
          this.createButtonTooltip =
            'Error: Rule ' + (i + 1) + ' protocol-ports are empty.';
          return false;
        }
        for (let j = 0; j < protoArr.length; j++) {
          const protocolField: AbstractControl = protoArr.controls[j].get(['protocol']);
          if (Utility.isEmpty(protocolField.value)) {
            this.createButtonTooltip =
              'Error: Rule ' + (i + 1) + ' protocol ' + (j + 1) + ' is empty.';
            return false;
          }
          if (!protocolField.valid) {
            this.createButtonTooltip =
            'Error: Rule ' + (i + 1) + ' protocol ' + (j + 1) + ' is invalid.';
            return false;
          }
          const portsField: AbstractControl = protoArr.controls[j].get(['ports']);
          if (protocolField.value === 'tcp' || protocolField.value === 'udp') {
            if (Utility.isEmpty(portsField.value)) {
              this.createButtonTooltip =
                'Error: Rule ' + (i + 1) + ' ports ' + (j + 1) + ' are empty.';
              return false;
            }
            if (!portsField.valid) {
              this.createButtonTooltip =
              'Error: Rule ' + (i + 1) + ' ports ' + (j + 1) + ' are invalid.';
              return false;
            }
          }
          if (protocolField.value === 'any') {
            if (!portsField.valid) {
              this.createButtonTooltip =
              'Error: Rule ' + (i + 1) + ' ports ' + (j + 1) + ' are invalid.';
              return false;
            }
          }
        }
      }
    }
    this.createButtonTooltip = '';
    return true;
  }

  getTooltip(): string {
    if (Utility.isEmpty(this.newObject.$formGroup.get(['meta', 'name']).value)) {
      return 'Error: Name field is empty.';
    }
    if (!this.newObject.$formGroup.get(['meta', 'name']).disabled &&
        !this.newObject.$formGroup.get(['meta', 'name']).valid)  {
      return 'Error: Name field is invalid.';
    }
    return Utility.isEmpty(this.createButtonTooltip) ? 'Ready to save new policy' :
            this.createButtonTooltip;
  }

  setToolbar() {
    const currToolbar = this.controllerService.getToolbarData();
    currToolbar.buttons = [
      {
        cssClass: 'global-button-primary global-button-padding',
        text: 'CREATE POLICY',
        genTooltip: () => this.getTooltip(),
        callback: () => { this.saveObject(); },
        computeClass: () => this.computeButtonClass()
      },
      {
        cssClass: 'global-button-neutral global-button-padding',
        text: 'CANCEL',
        callback: () => { this.cancelObject(); }
      },
    ];

    this._controllerService.setToolbarData(currToolbar);
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
      if (r.data.selectedProtoAppOption === this.PROTO_PORTS_OPTION) {
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
    this.rules.push({
      id: Utility.s4(),
      data: { rule: rule, selectedProtoAppOption: this.PROTO_PORTS_OPTION },
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
    if (this.rules.length > 1) {
      this.rules.splice(index, 1);
    }
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
      if (val === 'tcp' || val === 'udp' || val === 'icmp' || val === 'any') {
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
    const protocol = formGroup.get(['protocol']).value;
    const portsCtrl: FormControl = formGroup.get(['ports']);
    const shouldEnable: boolean = protocol && (protocol.trim() === 'tcp' || protocol.trim() === 'udp' || protocol.trim() === 'any');
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
    return protocol && (protocol.trim() === 'tcp' || protocol.trim() === 'udp');
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
