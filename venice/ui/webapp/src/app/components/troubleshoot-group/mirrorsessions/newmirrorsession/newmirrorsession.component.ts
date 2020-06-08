import { Component, OnInit, ViewEncapsulation, AfterViewInit, Input, ChangeDetectionStrategy, ChangeDetectorRef } from '@angular/core';
import { ControllerService } from '@app/services/controller.service';
import { SecurityService } from '@app/services/generated/security.service';
import { CreationForm } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { Animations } from '@app/animations';
import {
  IMonitoringMirrorSession, MonitoringMirrorSession,
  MonitoringMirrorSessionSpec, MonitoringMirrorCollector, MonitoringMatchRule,
  MonitoringMirrorSessionSpec_packet_filters, LabelsRequirement
} from '@sdk/v1/models/generated/monitoring';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { AbstractControl, FormArray, ValidatorFn, ValidationErrors } from '@angular/forms';
import { IPUtility } from '@app/common/IPUtility';
import { Utility } from '@app/common/Utility';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { OrderedItem } from '@app/components/shared/orderedlist/orderedlist.component';
import { SelectItem } from 'primeng/api';
import { NetworkNetworkInterface, NetworkNetworkInterfaceSpec_type } from '@sdk/v1/models/generated/network';
import { NetworkService } from '@app/services/generated/network.service';
import { LabelsSelector } from '@sdk/v1/models/generated/monitoring/labels-selector.model';
import { MonitoringInterfaceMirror } from '@sdk/v1/models/generated/monitoring/monitoring-interface-mirror.model';

export interface MatchRule {
  key: string;
  operator: string;
  values: string[];
}

const PACKET_FILTERS_ERRORMSG: string =
  'At least one match rule must be specified';
  // Since Packet Filter UI control is not shown, this part of the string is not to be shown yet"
  // + 'if packet filter is set to All Packets.';

@Component({
  selector: 'app-newmirrorsession',
  templateUrl: './newmirrorsession.component.html',
  styleUrls: ['./newmirrorsession.component.scss'],
  encapsulation: ViewEncapsulation.None,
  animations: Animations,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class NewmirrorsessionComponent extends CreationForm<IMonitoringMirrorSession, MonitoringMirrorSession> implements OnInit, AfterViewInit {

  @Input() existingObjects: MonitoringMirrorSession[] = [];

  MAX_COLLECTORS_ALLOWED: number = 2;

  IPS_LABEL: string = 'IP Addresses';
  IPS_ERRORMSG: string = 'Invalid IP addresses';
  IPS_TOOLTIP: string = 'Type in ip address and hit enter or space key to add more.';
  MACS_LABEL: string = 'MAC Addresses';
  MACS_ERRORMSG: string = 'Invalid MAC addresses. It should be aaaa.bbbb.cccc format.';
  MACS_TOOLTIP: string = 'Type in mac address and hit enter or space key to add more.';
  PROTS_ERRORMSG: string = 'Invalid Protocol/Port';
  PROTS_TOOLTIP: string = 'Type in valid layer3 or layer 4 protocol and protocol/port, ' +
    'hit enter or space key to add more. Port can be individual or range.' +
    'for example: icmp, any/2345, tcp/60001-60100...';
  LABEL_HELP: string = ' To create a new uplink interface, please go to the Networks Overview page to select uplink interfaces and create labels. These labels will be used to reference their respective network interfaces.';
  LABELVALUES_TOOLTIP: string = 'Type in uplink interface label values. Press enter or space key to add multiple values.' + this.LABEL_HELP;
  LABELVALUES_SELECT_TOOLTIP: string = 'Select uplink interface label values.' + this.LABEL_HELP;
  LABELKEY_TOOLTIP: string = 'Type in or select an uplink interface label key.' + this.LABEL_HELP;
  AND_BUTTON_TOOLTIP: string = 'Refine the uplink interface selection by adding a new selector using the "and" operation.';

  minDate: Date = new Date();
  defaultDate: Date = new Date();

  packetFilterOptions = Utility.convertEnumToSelectItem(MonitoringMirrorSessionSpec.propInfo['packet-filters'].enum);
  collectorTypeOptions = Utility.convertEnumToSelectItem(MonitoringMirrorCollector.propInfo['type'].enum).filter(
    item => item.label && !item.label.endsWith('deprecated'));
  interfaceDirectionOptions = Utility.convertEnumToSelectItem(MonitoringInterfaceMirror.propInfo['direction'].enum);

  rules: OrderedItem<any>[] = [];

  PROTO_PORTS_OPTION: string = 'proto-ports';
  APPS_OPTION: string = 'applications';

  protoAppOptions = [
    { label: 'PROTO-PORTS', value: this.PROTO_PORTS_OPTION },
    { label: 'APPS', value: this.APPS_OPTION },
  ];

  labelMatchCount: number = 0;
  matchMap: Map<string, any> = new Map<string, any>();
  allNIList: string[] = [];
  labelOperatorOptions: SelectItem[] = [
    { label: 'In', value: 'in' },
    { label: 'Not In', value: 'notin' }
  ];
  labelKeyOptions: SelectItem[] = [];
  labelValueOptionsMap: {key: string, values: SelectItem[]} = {} as {key: string, values: SelectItem[]};

  radioSelection: string = 'rules';

  constructor(protected _controllerService: ControllerService,
    protected _monitoringService: MonitoringService,
    protected securityService: SecurityService,
    protected uiconfigsService: UIConfigsService,
    protected networkService: NetworkService,
    private cdr: ChangeDetectorRef
  ) {
    super(_controllerService, uiconfigsService, MonitoringMirrorSession);
  }

  getClassName(): string {
    return this.constructor.name;
  }

  postNgInit(): void {
    this.getNILabels();

   // currently backend does not support any drop packets
    // UI temporarily drop those choices.
    // once they are supported, pls uncomment out the next lines
    this.packetFilterOptions = this.packetFilterOptions.filter(item =>
      item.value === MonitoringMirrorSessionSpec_packet_filters['all-packets']
    );

    if (this.isInline) {

      const interfaceObj = this.newObject.$formGroup.get(['spec', 'interfaces']).value;
      if (interfaceObj && interfaceObj.selectors && interfaceObj.selectors.length > 0) {
        this.radioSelection = 'labels';
      } else {
        this.radioSelection = 'rules';
      }

      // process match rules
      this.rules = [];
      if (this.newObject.spec['match-rules'].length > 0) {
        this.newObject.spec['match-rules'].forEach(rule => {
          rule.$formGroup.valueChanges.subscribe(() => {
            this.newObject.$formGroup.get(['spec', 'packet-filters']).updateValueAndValidity();
          });
          let selectedProtoAppOption = this.PROTO_PORTS_OPTION;
          if (rule['app-protocol-selectors'].applications.length > 0) {
            selectedProtoAppOption = this.APPS_OPTION;
          }
          this.rules.push({
            id: Utility.s4(),
            data: { rule, selectedProtoAppOption },
            inEdit: false
          });
        });
      }
      // disable name field
      this.newObject.$formGroup.get(['meta', 'name']).disable();
    }

    this.newObject.$formGroup.get(['spec', 'packet-size']).setValidators([this.packetSizeValidator()]);

    // due to currently backend does not support all drops, comment out next lines
    /*
    this.newObject.$formGroup.get(['spec', 'packet-filters']).setValidators([
      Validators.required,
      this.packetFiltersValidator()]);
    */

    // Add one collectors if it doesn't already have one
    const collectors = this.newObject.$formGroup.get(['spec', 'collectors']) as FormArray;
    if (collectors.length === 0) {
      this.addCollector();
    }

    // Add one interface selector if it doesn't already have one
    const selectors = this.newObject.$formGroup.get(['spec', 'interfaces', 'selectors']) as FormArray;
    if (selectors.length === 0) {
      const newSelector = new LabelsSelector().$formGroup;
      selectors.push(newSelector);
      this.addInterfaceSelector();
    }

    // Add one matchrule if it doesn't already have one
    if (this.rules.length === 0) {
      this.addRule();
    }
  }

  // due to currently backend does not support all drops, comment out next lines
  /*
  getPacketFiltersTooltip(): string {
    const packetFiltersField: FormControl =
      this.newObject.$formGroup.get(['spec', 'packet-filters']) as FormControl;
    if (Utility.isEmpty(packetFiltersField.value)) {
      return '';
    }
    if (!packetFiltersField.valid) {
      return PACKET_FILTERS_ERRORMSG;
    }
    return '';
  }
  */

  isSpanIDAlreadyUsed(existingObjects: IMonitoringMirrorSession[]): ValidatorFn {
    return (control: AbstractControl): ValidationErrors | null => {
      if (!control.value && control.value !== 0) {
        return null;
      }
      const mirrorSpan: IMonitoringMirrorSession =
        this.existingObjects.find((item: IMonitoringMirrorSession) =>
          item.spec['span-id'] === control.value);
      if (mirrorSpan && mirrorSpan.meta.name !== this.newObject.$formGroup.get(['meta', 'name']).value) {
        return {
          objectname: {
            required: true,
            message: 'ERSPAN ID must be unique, already used by ' + mirrorSpan.meta.name
          }
        };
      }
      return null;
    };
  }

  isFormValid(): boolean {
    // vs-1021 packet-filters can be empty
    // due to currently backend does not support all drops, comment out next lines
    /*
    if (!this.newObject.$formGroup.get(['spec', 'packet-filters']).valid) {
      this.submitButtonTooltip = PACKET_FILTERS_ERRORMSG;
      return false;
    }
    */

   if (Utility.isEmpty(this.newObject.$formGroup.get(['meta', 'name']).value)) {
      this.submitButtonTooltip = 'Error: Name field is empty.';
      return false;
    }

    if (this.newObject.$formGroup.get(['meta', 'name']).invalid) {
      this.submitButtonTooltip =  'Error: Name field is invalid.';
      return false;
    }

    if (!this.newObject.$formGroup.get(['spec', 'packet-size']).valid) {
      this.submitButtonTooltip = 'Invalid Packet Size';
      return false;
    }

    const collectors = this.controlAsFormArray(
      this.newObject.$formGroup.get(['spec', 'collectors'])).controls;
    for (let i = 0; i < collectors.length; i++) {
      const collector = collectors[i];
      if (Utility.isEmpty(collector.get(['export-config', 'destination']).value)) {
        this.submitButtonTooltip = 'Error: Collector Destination is required.';
        return false;
      }
    }

    if (this.radioSelection === 'labels') {
      if (!this.getAllInterfaceSelectorsValues()) {
        this.submitButtonTooltip = 'At least one label is incomplete.';
        return false;
      }
    } else {
      // validate rules
      if (this.areAllRulesEmpty()) {
        this.submitButtonTooltip = 'At least one match rule must be specified.';
        return false;
      }

      for (let i = 0; i < this.rules.length; i++) {
        const rule: MonitoringMatchRule = this.rules[i].data.rule;
        if (!(rule.$formGroup.get(['source', 'ip-addresses']).valid)) {
          this.submitButtonTooltip =
            'Error: Rule ' + (i + 1) + ' source IP adresses are invalid.';
          return false;
        }
        if (!(rule.$formGroup.get(['source', 'mac-addresses']).valid)) {
          this.submitButtonTooltip =
            'Error: Rule ' + (i + 1) + ' source MAC adresses are invalid.';
          return false;
        }
        if (!(rule.$formGroup.get(['destination', 'ip-addresses']).valid)) {
          this.submitButtonTooltip =
            'Error: Rule ' + (i + 1) + ' destination IP adresses are invalid.';
          return false;
        }
        if (!(rule.$formGroup.get(['destination', 'mac-addresses']).valid)) {
          this.submitButtonTooltip =
            'Error: Rule ' + (i + 1) + ' destination MAC adresses are invalid.';
          return false;
        }
        if (!(rule.$formGroup.get(['app-protocol-selectors', 'proto-ports']).valid)) {
          this.submitButtonTooltip =
            'Error: Rule ' + (i + 1) + ' protocol/ports are invalid.';
          return false;
        }
      }
    }
    if (!this.newObject.$formGroup.valid) {
      this.submitButtonTooltip = 'Error: Form is invalid.';
      return false;
    }

    this.submitButtonTooltip = '';
    return true;
  }

  getObjectValues() {
    const currValue: IMonitoringMirrorSession = this.newObject.getFormGroupValues();
    if (this.radioSelection === 'labels') {
      currValue.spec['match-rules'] = [];
      currValue.spec['packet-filters'] = [];
    } else {
      currValue.spec['interfaces'] = null;
      currValue.spec['match-rules'] = this.rules.map(r => {
        return r.data.rule.getFormGroupValues();
      });
    }

    return currValue;
  }

  isMirrorsessionNameValid(existingTechSupportRequest: MonitoringMirrorSession[]): ValidatorFn {
    return Utility.isModelNameUniqueValidator(existingTechSupportRequest, 'Mirror-session-name');
  }

  packetFiltersValidator() {
    return (control: AbstractControl): ValidationErrors | null => {
      const arr: string[] = control.value;
      if (arr.indexOf('all-packets') > -1 && this.areAllRulesEmpty()) {
        return {
          packetFilters: {
            required: true,
            message: PACKET_FILTERS_ERRORMSG
          }
        };
      }
      return null;
    };
  }

  packetSizeValidator() {
    return (control: AbstractControl): ValidationErrors | null => {
      if (control.value) {
        const actual: number = Number(control.value);
        if (actual === 0) {
          return null;
        }
        if (actual < 64) {
          return {
              minValue: {
                  valid: false,
                  required: false,
                  actual: actual,
                  message: 'Value must be greater than or equal 64'
              }
          };
        }
        if (actual > 2048) {
          return {
              minValue: {
                  valid: false,
                  required: false,
                  actual: actual,
                  message: 'Value must be less than or equal 2048'
              }
          };
        }
      }
      return null;
    };
  }

  setToolbar() {
    const currToolbar = this.controllerService.getToolbarData();
    currToolbar.buttons = [
      {
        cssClass: 'global-button-primary newmirrorsession-toolbar-button newmirrorsession-toolbar-SAVE',
        text: 'CREATE MIRROR SESSION',
        matTooltipClass: 'validation_error_tooltip',
        callback: () => { this.saveObject(); },
        computeClass: () => this.computeFormSubmitButtonClass(),
        genTooltip: () => this.getSubmitButtonToolTip(),
      },
      {
        cssClass: 'global-button-neutral newmirrorsession-toolbar-button newmirrorsession-toolbar-CANCEL',
        text: 'CANCEL',
        callback: () => { this.cancelObject(); }
      },
    ];

    this._controllerService.setToolbarData(currToolbar);
  }

  addCollector() {
    const collectors = this.newObject.$formGroup.get(['spec', 'collectors']) as FormArray;
    const newCollector = new MonitoringMirrorCollector().$formGroup;
    newCollector.get(['export-config', 'destination']).setValidators([
      newCollector.get(['export-config', 'destination']).validator, IPUtility.isValidIPValidator]);
    newCollector.get(['export-config', 'gateway']).setValidators([IPUtility.isValidIPValidator]);
    collectors.insert(collectors.length, newCollector);
  }

  removeCollector(index: number) {
    const collectors = this.newObject.$formGroup.get(['spec', 'collectors']) as FormArray;
    if (collectors.length > 1) {
      collectors.removeAt(index);
    }
  }

  onRadioButtonChange(event, data) {
    if (event.value === 'applications') {
      data.rule.$formGroup.get(['app-protocol-selectors', 'proto-ports']).setValue([]);
    } else {
      data.rule.$formGroup.get(['app-protocol-selectors', 'applications']).setValue([]);
    }
    data.selectedProtoAppOption = event.value;
  }

  addRule() {
    const rule = new MonitoringMatchRule();
    this.rules.push({
      id: Utility.s4(),
      data: { rule: rule, selectedProtoAppOption: this.PROTO_PORTS_OPTION },
      inEdit: false
    });
    this.rules.forEach((r, i) => {
      if (i === this.rules.length - 1) {
        r.inEdit = true;
      } else {
        r.inEdit = false;
      }
    });
  }

  deleteRule(index) {
    this.rules.splice(index, 1);
  }

  areAllRulesEmpty() {
    for (let i = 0; i < this.rules.length; i++) {
      if (!this.isRuleEmpty(this.rules[i].data.rule)) {
        return false;
      }
    }
    return true;
  }

  isRuleEmpty(rule: MonitoringMatchRule) {
    const fields = [
      rule.$formGroup.get(['source', 'ip-addresses']),
      rule.$formGroup.get(['source', 'mac-addresses']),
      rule.$formGroup.get(['destination', 'ip-addresses']),
      rule.$formGroup.get(['destination', 'mac-addresses']),
      rule.$formGroup.get(['app-protocol-selectors', 'proto-ports']),
      rule.$formGroup.get(['app-protocol-selectors', 'applications'])
    ];
    for (let i = 0; i < fields.length; i++) {
      if (fields[i].valid && !Utility.isEmpty(fields[i].value)) {
        return false;
      }
    }
    return true;
  }

  getDataArray(data: any, type: string, field: string) {
    return data.rule.$formGroup.get([type, field]).value;
  }

  isValidIP(ip: string) {
    return IPUtility.isValidIPWithOptionalMask(ip);
  }

  isValidMAC(mac: string) {
    return Utility.MACADDRESS_REGEX.test(mac);
  }

  isValidProto(chip: string) {
    const arr: string[] = chip.split('/');
    const prot: string = arr[0];
    if (arr.length > 2) {
      return false;
    }
    if (!prot) {
      return false;
    }
    if (arr.length === 1) {
      return Utility.isProtocolNoPortsValid(prot);
    }
    if (!Utility.isProtocolHasPortsValid(prot)) {
      return false;
    }
    return Utility.isPortRangeValid(arr[1]);
  }

  createObject(newObject: IMonitoringMirrorSession) {
    return this._monitoringService.AddMirrorSession(newObject);
  }

  updateObject(newObject: IMonitoringMirrorSession, oldObject: IMonitoringMirrorSession) {
    return this._monitoringService.UpdateMirrorSession(oldObject.meta.name, newObject, null, oldObject);
  }

  generateCreateSuccessMsg(object: IMonitoringMirrorSession): string {
    return 'Created mirror session ' + object.meta.name;
  }

  generateUpdateSuccessMsg(object: IMonitoringMirrorSession): string {
    return 'Updated mirror session ' + object.meta.name;
  }

  isShowAddCollector(): boolean {
    const collectors = this.newObject.$formGroup.get(['spec', 'collectors']) as FormArray;
    if (collectors.length < this.MAX_COLLECTORS_ALLOWED) {
      return true;
    }
    return false;
  }

  // this function join two array and return a new araay with items appears
  // on both array
  joinArry(arr1: any[], arr2: any[]): any[] {
    if (arr1.length === 0) {
      return [...arr2];
    }
    if (arr2.length === 0) {
      return [...arr1];
    }
    return [...arr1].filter(i => arr2.includes(i));
  }

  getMatches(selectors: any[]) {
    let matchList = [];
    for (const selctor of selectors) {
      if (selctor.key && selctor.operator && selctor.values) {
        matchList = this.joinArry(matchList, this.getRuleMatches(selctor));
      }
    }
    return matchList;
  }

  getRuleMatches(rule: MatchRule): string[] {
    const key = rule.key;
    const op = rule.operator;
    const val = rule.values;
    let matches = [];
    if (!this.matchMap[key]) {
      return [];
    }
    const keys = Object.keys(this.matchMap[key]);
    if (keys.length === 0) {
      return [];
    }
    if (op === 'in') {
      if (!val || val.length === 0) {
        return [];
      }
      val.forEach((item: string) => {
        if (this.matchMap[key][item]) {
          matches = matches.concat(this.matchMap[key][item]);
        }
      });
      return [...Array.from(new Set(matches).values())];
    }

    // op === not_in
    keys.forEach((item: string) => {
      if (!val.includes(item) && this.matchMap[key][item]) {
        matches = matches.concat(this.matchMap[key][item]);
      }
    });
    return [...Array.from(new Set(matches).values())];
  }

  getNILabels() {
    const sub = this.networkService.ListNetworkInterfaceCache().subscribe(
      (response) => {
        if (response.connIsErrorState) {
          return;
        }
        const allInterfaces: NetworkNetworkInterface[] = response.data as NetworkNetworkInterface[];
        this.matchMap = new Map<string, any>();
        this.labelKeyOptions = [];
        this.labelValueOptionsMap = {} as {key: string, values: SelectItem[]};
        this.allNIList = [];
        this.buildLabelInformation(allInterfaces);
        this.buildLabelData();
        this.cdr.detectChanges();
      }
    );
    this.subscriptions.push(sub);
  }

  buildLabelInformationFromMirrorSessions() {
    for (const obj of this.existingObjects) {
      if (obj.spec.interfaces && obj.spec.interfaces.selectors.length && obj.spec.interfaces.selectors[0]) {
        for (const i of obj.spec.interfaces.selectors[0].requirements ) {
          const key = i.key;
          if (this.matchMap[key] == null) {
            this.matchMap[key] = new Map<string, any>();
          }
          for (const v of i.values) {
            if (this.matchMap[key][v] == null) {
              this.matchMap[key][v] = [];
            }
          }
        }
      }
    }
  }

  buildLabelInformation(networkInterfaces: NetworkNetworkInterface[]) {
    if (networkInterfaces.length) {
      networkInterfaces.forEach((item: NetworkNetworkInterface) => {
        if (item.meta && item.meta.labels &&
            item.spec.type === NetworkNetworkInterfaceSpec_type['uplink-eth'] &&
            !Utility.isInterfaceInValid(item)) {
          for (const key of Object.keys(item.meta.labels)) {
            if (this.matchMap[key] == null) {
              this.matchMap[key] = new Map<string, any>();
            }
            if (this.matchMap[key][item.meta.labels[key]] == null) {
              this.matchMap[key][item.meta.labels[key]] = [];
            }
            this.matchMap[key][item.meta.labels[key]].push(item.meta.name);
          }
          this.allNIList.push(item.meta.name);
        }
      });
    }
  }

  buildLabelData() {
    for (const key of Object.keys(this.matchMap)) {
      const valList = [];
      for (const val of Object.keys(this.matchMap[key])) {
        valList.push({ value: val, label: val });
      }
      this.labelKeyOptions.push({label: key, value: key});
      this.labelValueOptionsMap[key] = valList;
    }
    this.labelKeyOptions.push({ label: '', value: null });
  }

  addInterfaceSelector() {
    const selectors = this.newObject.$formGroup.get(['spec', 'interfaces', 'selectors', 0, 'requirements']) as FormArray;
    const newSelector = new LabelsRequirement().$formGroup;
    newSelector.get(['operator']).setValue('in');
    selectors.insert(selectors.length, newSelector);
  }

  removeInterfaceSelector(index: number) {
    const selectors = this.newObject.$formGroup.get(['spec', 'interfaces', 'selectors', 0, 'requirements']) as FormArray;
    if (selectors.length > 1) {
      selectors.removeAt(index);
    }
  }

  getAllInterfaceSelectorsValues(): MatchRule[] {
    const result: MatchRule[] = [];
    const selectors = this.newObject.$formGroup.get(['spec', 'interfaces', 'selectors', 0, 'requirements']) as FormArray;
    for (let i = 0; i < selectors.length; i++) {
      const item = selectors.controls[i].value;
      if (!item.key || !item.values || item.values.length === 0) {
        return null;
      }
      if (!this.matchMap[item.key]) {
        return [];
      }
      result.push(item);
    }
    return result;
  }

  getLabelMatchInfo() {
    const ifValues = this.getAllInterfaceSelectorsValues();
    if (!ifValues || ifValues.length === 0) {
      return { count: 0, title: '' };
    }
    const matches = this.getMatches(ifValues);
    const list = [];
    for (let i = 0; i < Math.min(10, matches.length); i++) {
      list.push(matches[i]);
    }
    let title = list.join('\n');
    if (matches.length > 10) {
      title += '\n...';
    }
    return {count: matches.length, title };
  }

  isValidLabelValue(name: string): boolean {
    // put a place holder here for the future validation
    return true;
  }

  onInterfaceKeyChange(ifSelector: any) {
    ifSelector.get(['values']).setValue(null);
  }
}
