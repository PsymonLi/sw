import { AfterViewInit, Component, OnDestroy, OnInit, ViewEncapsulation, EventEmitter, Input, Output, OnChanges, SimpleChanges, ViewChildren, QueryList, ChangeDetectorRef } from '@angular/core';
import { FormArray, ValidatorFn, FormGroup } from '@angular/forms';
import { BaseComponent } from '@components/base/base.component';
import { RolloutService } from '@app/services/generated/rollout.service';
import { ControllerService } from '@app/services/controller.service';
import { ObjstoreService } from '@app/services/generated/objstore.service';
import { ToolbarData } from '@app/models/frontend/shared/toolbar.interface';
import { IApiStatus, IRolloutRollout, RolloutRollout, RolloutRolloutSpec } from '@sdk/v1/models/generated/rollout';
import { IObjstoreObjectList } from '@sdk/v1/models/generated/objstore';
import { RepeaterData, ValueType, RepeaterItem } from 'web-app-framework';
import { Animations } from '@app/animations';
import { SelectItem } from 'primeng/primeng';
import { Observable } from 'rxjs';
import { required } from '@sdk/v1/utils/validators';
import { Utility } from '@common/Utility';
import { RolloutImageLabel, EnumRolloutOptions, RolloutImageOption } from '../index';
import { RolloutUtil } from '../RolloutUtil';
import { SearchExpression } from '@app/components/search/index.ts';
import { LabelsSelector } from '@sdk/v1/models/generated/security';
import { SearchUtil } from '@app/components/search/SearchUtil';
import { SearchService } from '@app/services/generated/search.service';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { ClusterSmartNIC } from '@sdk/v1/models/generated/cluster';
import { ClusterService } from '@app/services/generated/cluster.service';

export class RolloutOrder {
  id: string;
  data: FormGroup;
  orderSummary: string = '';
  matchedSmartNICs: string[] = [];
  inEdit: boolean = false;
  duplicatesWith: number[] = [];
  incomplete: boolean = true;
}

/**
 * Description:

  When Naples are assigned labels we can specify a sequence for the rollout to progress in.
  This means that the 1st order group will be updated before the 2nd and so on.
  The UI allows the user to select NICs for group with the help of label selectors.

  When one or more labels are selected for group the updated label count is shown on the order HTML element.
  User can choose to add multiple such group though only one can be edited at one time.

  The 'Save Rollout' button is disabled until isAllInputsValidated() return true.

  Everytime a change is made, we built the duplicateMatchMap.
  It is a mapping from nic.meta.name to list of orderindices
  We iterate over all orders (and their matches NICs) and built the map.
  When we see that a matched NIC already has order(s) in the map, we show the info icon to notify the user.

 * Key functions:
  onInit() -> fetch all Naples
  setSpecOrderConstrains() - generates the order constraints
  buildRollout() - built the rollout object based on the form
  saveRollout() - call the rollout create api with the built rollout object
  getRuleResults() - checks the naplesRuleMap to find NICs matching the given labels
  repeaterValueChange(index) - updates the order summary and match count, when the repeater value changes
  isAllInputsValidated()

 */

@Component({
  selector: 'app-newrollout',
  templateUrl: './newrollout.component.html',
  styleUrls: ['./newrollout.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
})
export class NewrolloutComponent extends BaseComponent implements OnInit, OnDestroy, AfterViewInit, OnChanges {
  orders: RolloutOrder[] = [];
  repeaterList: RepeaterItem[] = [];
  newRollout: RolloutRollout;
  subscriptions = [];
  oldToolbarData: ToolbarData;
  strategyOptions: SelectItem[] = Utility.convertEnumToSelectItem(RolloutRolloutSpec.propInfo['strategy'].enum);
  upgradetypeOptions: SelectItem[] = Utility.convertEnumToSelectItem(RolloutRolloutSpec.propInfo['upgrade-type'].enum);
  rolloutImageOptions: RolloutImageOption[] = [];

  rolloutNicNodeTypes: SelectItem[] = Utility.convertEnumToSelectItem(EnumRolloutOptions);
  selectedRolloutNicNodeTypes: string = RolloutUtil.ROLLOUTTYPE_BOTH_NAPLES_VENICE;

  orderConstraintslabelData: RepeaterData[] = [];
  orderConstraintslabelFormArray = new FormArray([]);

  minDate: Date;

  @Input() isInline: boolean = false;
  @Input() selectedRolloutData: IRolloutRollout;
  @Input() existingRollouts: RolloutRollout[] = [];
  @Input() rolloutImages: IObjstoreObjectList;

  @Output() formClose: EventEmitter<any> = new EventEmitter();

  rolloutNowcheck: boolean = false;
  versiondescription: string = '';

  bodyicon: any = {
    margin: {
      top: '9px',
      left: '8px'
    },
    matIcon: 'update'
  };

  naplesEventUtility: HttpEventUtility<ClusterSmartNIC>;
  naples: ReadonlyArray<ClusterSmartNIC> = [];
  naplesIdMap: { [id: string]: ClusterSmartNIC } = {};
  naplesRuleMap: { [rule: string]: Set<string> } = {};
  duplicateMatchMap: { [nicid: string]: number[] } = {};

  repeaterAnimationEnabled: boolean = true;

  constructor(private rolloutService: RolloutService,
    private objstoreService: ObjstoreService,
    protected _searchService: SearchService,
    protected controllerService: ControllerService,
    protected clusterService: ClusterService,
    protected cd: ChangeDetectorRef,
  ) {
    super(controllerService);
  }

  getClassName(): string {
    return this.constructor.name;
  }

  onOrderChange() {
    this.buildDuplicationMap();
    // We disable animation while the re-order animation happens
    // to prevent app-repeater from stuttering
    this.repeaterAnimationEnabled = false;
    setTimeout(() => {
    this.repeaterAnimationEnabled = true;
    }, 500);
  }


  ngOnInit() {
    this.getNaplesLabelsKeyValues();
    // prepare for label-selector repeater
    if (!this.newRollout) {
      // below ngChange() should initialize rolloutData.
      this.initRolloutData();
    }
    this.addOrder();
    this.minDate = new Date();
  }

  // When we get a response containing all SmartNICs objects, we build labelData to be used for the label selector.
  // We also generate a Map of each label-rule to all the matchedSmartNICs.
  handleReadSmartNICsResponse(response) {
    this.orderConstraintslabelData = [];
    this.naplesEventUtility.processEvents(response);
    this.generateRuleAndIdMap();
    const labels = Utility.getLabels(this.naples as any[]);
    for (const key of Object.keys(labels)) {
      const valList = [];
      for (const val of Object.keys(labels[key])) {
        valList.push({ value: val, label: val });
      }
      const label = {
        key: { value: key, label: key },
        values: valList,
        operators: SearchUtil.stringOperators,
        fieldType: ValueType.singleSelect,
        valueType: ValueType.multiSelect
      };
      this.orderConstraintslabelData.push(label);
    }
  }

  // We also create naplesIdMap which can be used to show matched naples details in the future.
  generateRuleAndIdMap() {
    this.naples.forEach(element => {
      if (element.meta.labels) {
        for (const key of Object.keys(element.meta.labels)) {
          const keyValText = key + '=' + element.meta.labels[key];
          if (!(keyValText in this.naplesRuleMap)) {
            this.naplesRuleMap[keyValText] = new Set<string>();
          }
          this.naplesRuleMap[keyValText].add(element.meta.name);
          this.naplesIdMap[element.meta.name] = element;
        }
      }
    });
  }

  getNaplesLabelsKeyValues() {
    this.naplesEventUtility = new HttpEventUtility<ClusterSmartNIC>(ClusterSmartNIC);
    this.naples = this.naplesEventUtility.array as ReadonlyArray<ClusterSmartNIC>;
    const subscription = this.clusterService.WatchSmartNIC().subscribe(
      response => {
        this.handleReadSmartNICsResponse(response);
      },
      this.controllerService.webSocketErrorHandler('Failed to get labels')
    );
    this.subscriptions.push(subscription);
  }

  initRolloutData() {
    if (!this.isInline) {  // create mode
      this.newRollout = new RolloutRollout();
      this.newRollout.kind = 'rollout';
      const today = new Date().getTime() + 300 * 1000; // VS-331 set default time. Now + 5 min
      this.newRollout.spec['scheduled-start-time'] = new Date(today);
      this.newRollout.setFormGroupValuesToBeModelValues();
      this.newRollout.$formGroup.get(['spec', 'suspend']).disable();
      this.newRollout.$formGroup.get(['spec', 'scheduled-start-time']).enable();
    } else {   // edit mode
      const myrollout: IRolloutRollout = Utility.getLodash().cloneDeep(this.selectedRolloutData);
      myrollout.spec['scheduled-start-time'] = new Date(myrollout.spec['scheduled-start-time']);
      this.newRollout = new RolloutRollout(myrollout);
      this.newRollout.$formGroup.get(['meta', 'name']).disable();
      this.newRollout.$formGroup.get(['spec', 'version']).disable(); // disable version until version options are available.
      this.selectedRolloutNicNodeTypes = RolloutUtil.getRolloutNaplesVeniceType(this.newRollout);
      this.rolloutNowcheck = false;
      this.newRollout.$formGroup.get(['spec', 'scheduled-start-time']).enable();
    }
    // set validators. TODO: 2019-05-15 wait for server set validator in rollout.proto
    if (this.newRollout.$formGroup.validator) {
      this.newRollout.$formGroup.get(['meta', 'name']).setValidators([required, this.isRolloutNameValid(this.existingRollouts), this.newRollout.$formGroup.validator]);
    } else {
      this.newRollout.$formGroup.get(['meta', 'name']).setValidators([required, this.isRolloutNameValid(this.existingRollouts)]);
      this.newRollout.$formGroup.get(['spec', 'scheduled-start-time']).setValidators([required, this.isRolloutScheduleTimeValid()]);
      this.newRollout.$formGroup.get(['spec', 'version']).setValidators([required]);
    }
    this.newRollout.$formGroup.get(['spec', 'duration']).disable(); // TODO: Disable duration for this release 2019-05-02
  }


  ngAfterViewInit() {
    if (!this.isInline) {
      // If it is not inline, we change the toolbar buttons, and save the old one
      // so that we can set it back when we are done
      this.oldToolbarData = this.controllerService.getToolbarData();
      const newToolbarData = this.controllerService.getToolbarData();
      newToolbarData.buttons = [
        {
          cssClass: 'global-button-primary newrollout-toolbar-button',
          text: 'SAVE ROLLOUT',
          callback: () => {
            this.addRollout();
          },
          computeClass: () => this.isAllInputsValidated() ? '' : 'global-button-disabled',
        },
        {
          cssClass: 'global-button-neutral newrollout-toolbar-button',
          text: 'CANCEL',
          callback: () => {
            this.cancelRollout();
          },
        },
      ];
      this.controllerService.setToolbarData(newToolbarData);
    }
  }

  isRolloutScheduleTimeValid(): ValidatorFn {
    const greateThan = 1;
    const now = new Date();
    return Utility.dateValidator(now, greateThan, 'schedule-time', 'Can not schedule rollout to the past');
  }

  isRolloutNameValid(existingRollouts: RolloutRollout[]): ValidatorFn {
    return Utility.isModelNameUniqueValidator(existingRollouts, 'rollout-name');
  }

  countMatchedNICs() {
    let count = 0;
    for (const order of this.orders) {
      count += order.matchedSmartNICs.length;
    }
    return count;
  }

  getOrderConstraints() {
    return this.orders.map(x => x.data);
  }

  isAllInputsValidated(): boolean {
    if (this.newRollout.$formGroup['smartnic-must-match-constraint'] && this.countMatchedNICs() !== 0) {
      return false;
    }
    if (!this.newRollout || !this.newRollout.$formGroup) {
      return false;
    }
    if (this.newRollout.$formGroup.get(['spec', 'smartnic-must-match-constraint']).value) {

      const orders = this.getOrderConstraints();
      for ( let ix = 0; ix < orders.length; ix++) {
        const repeaterSearchExpression: SearchExpression[] = this.convertFormArrayToSearchExpression(orders[ix].value);
        if  (repeaterSearchExpression.length === 0) {
          return false;
        } else {
          for (let i = 0; i < repeaterSearchExpression.length; i++) {
            if (Utility.isEmpty(repeaterSearchExpression[i].key ) || Utility.isEmpty(repeaterSearchExpression[i].values)) {
              return false;
            }
          }
        }
      }
    }
    if (Utility.isEmpty(this.newRollout.$formGroup.get(['meta', 'name']).value)) {
      return false;
    }
    if (Utility.isEmpty(this.newRollout.$formGroup.get(['spec', 'version']).value)) {
      return false;
    }
    if (!this.rolloutNowcheck) {
      if (Utility.isEmpty(this.newRollout.$formGroup.get(['spec', 'scheduled-start-time']).value)) {
        return false;
      }
    }
    const hasFormGroupError = Utility.getAllFormgroupErrors(this.newRollout.$formGroup);
    return hasFormGroupError === null;
  }

  // Example:
  // orderValue = {
  // requirements: [{
  //   keyFormControl: "key",
  //   operatorFormControl: "in",
  //   valueFormControl: ["1"]}]
  // }
  convertFormArrayToSearchExpression(orderValue: any, addMetatag: boolean = false): SearchExpression[] {
    const data = orderValue.requirements;
    if (data == null) {
      return null;
    }

    let retData = data.filter((item) => {
      return !Utility.isEmpty(item.keyFormControl) && !Utility.isEmpty(item.valueFormControl) && item.valueFormControl.length !== 0;
    });
    // make sure the value field is an array
    retData = retData.map((item) => {
      const searchExpression: SearchExpression = {
        key: ((addMetatag) ? 'meta.labels.' : '')  + item.keyFormControl,
        operator: item.operatorFormControl,
        values: Array.isArray(item.valueFormControl) ? item.valueFormControl : item.valueFormControl.trim().split(',')
      };
      return searchExpression;
    });
    return retData;
  }

  onRolloutNowChange(checked) {
    this.rolloutNowcheck = checked;
    if (checked) {
      this.newRollout.$formGroup.get(['spec', 'scheduled-start-time']).disable();
    } else {
      this.newRollout.$formGroup.get(['spec', 'scheduled-start-time']).enable();
    }
  }

  ngOnDestroy() {
    this.subscriptions.forEach((subscription) => {
      subscription.unsubscribe();
    });
  }

  ngOnChanges(changes: SimpleChanges) {
    this.initRolloutData();
    this.populateRolloutVersions();
  }


  /**
   * Per back-end process, once user uploads a "bundle.tar" to Venice.
   *  https://venice:port/objstore/v1/tenant/default/images/objects result will contain
   *  [{
   *   "kind": "Object",
   *   "meta": {
   *     "name": "Bundle/0.10.0-130_img/metadata.json", // we are only interested in meta.name.endsWith('metadata.json')
   *     "generation-id": "",
   *     "labels": {
   *       "Releasedata": "",
   *       "Version": "0.10.0-130"   // get this version
   *    },
   * ...
   * ]
   */
  populateRolloutVersions() {
    const selectedVersion = this.newRollout.spec.version;
    this.rolloutImageOptions.length = 0;
    if (this.rolloutImages && this.rolloutImages.items) {
      this.rolloutImages.items.forEach(image => {
        if (image.meta.name.endsWith(RolloutUtil.ROLLOUT_METADATA_JSON)) {
          const imageLabel: RolloutImageLabel = image.meta.labels as RolloutImageLabel;
          if (imageLabel) {
            // rollout.spec.version is a string, so we set the selectedItem.value to string.
            const selectedItem: RolloutImageOption = {
              label: imageLabel.Version,
              value: imageLabel.Version,
              model: imageLabel // we need .model in getImageVersionData() api
            };
            this.rolloutImageOptions.push(selectedItem);
          }
        }
      });
      this.newRollout.$formGroup.get(['spec', 'version']).enable(); // enable version dropdown
    }
    this.computeVersionDescription(selectedVersion);
  }

  addRollout() {
    if (this.newRollout.$formGroup.value.spec['smartnic-must-match-constraint']) {
      let duplicate = false;
      for (const order of this.orders) {
        if (order.duplicatesWith.length > 0) {
          duplicate = true;
          break;
        }
      }
      if (duplicate) {
        this._controllerService.invokeConfirm({
          header: 'Multiple orders matching some DSCs.',
          message: 'The earliest order in the sequence will take preference for each of these DSCs. Do you want to make some changes?',
          acceptLabel: 'Yes',
          rejectLabel: 'No',
          reject: () => {
            this._saveRollout();
          }
        });
      } else {
        this._saveRollout();
      }
    } else {
      this._saveRollout();
    }
  }

  updateRollout() {
    this._saveRollout();
  }

  /**
   * Build rollout JSON.
   * according to RolloutUtil.getRolloutNaplesVeniceType() rules
   * sample working rollout ( upgrade some Naples)
   * {"kind":"rollout","api-version":null,"meta":{"name":"jeff-vs529-3"},"spec":{"version":"0.11.0-55","scheduled-start-time":"2019-06-25T22:56:21.145Z","strategy":"LINEAR","max-parallel":2,"max-nic-failures-before-abort":1,"order-constraints":[{"requirements":[{"key":"number","operator":"equals","values":["1"]}]}],"smartnics-only":false,"smartnic-must-match-constraint":true,"upgrade-type":"Disruptive"}}
   */
  buildRollout(): IRolloutRollout {
    const rollout: IRolloutRollout = this.newRollout.getFormGroupValues();
    if (this.rolloutNowcheck) {
      const today = new Date().getTime() + 30 * 1000; // If user wants to run rollout right the way, we give 30 seconds.
      rollout.spec['scheduled-start-time'] = new Date(today);
    }
    rollout.meta.name = (rollout.meta.name) ? rollout.meta.name : this.newRollout.meta.name;
    if (this.selectedRolloutNicNodeTypes === RolloutUtil.ROLLOUTTYPE_VENICE_ONLY) {
      rollout.spec['max-nic-failures-before-abort'] = null;
      rollout.spec['order-constraints'] = [];
      rollout.spec['smartnic-must-match-constraint'] = true;
      rollout.spec['smartnics-only'] = false;
    } else if (this.selectedRolloutNicNodeTypes === RolloutUtil.ROLLOUTTYPE_NAPLES_ONLY) {
      rollout.spec['smartnics-only'] = true;
      this.setSpecOrderConstrains(rollout);
      rollout.spec['max-parallel'] = null;
    } else if (this.selectedRolloutNicNodeTypes === RolloutUtil.ROLLOUTTYPE_BOTH_NAPLES_VENICE) {
      rollout.spec['smartnics-only'] = false;
      this.setSpecOrderConstrains(rollout);
    }
    return rollout;
  }

  /**
   *
   * @param rollout
   * order-constraints”: [
   *  {
   *    “requirements”: [
   *     {
   *      “key”: “number”,
   *       “operator”: “equals”,
   *       “values”: [
   *          “1"
   *       ]
   *     }
   *   ]
   * }
   */
  private setSpecOrderConstrains(rollout: IRolloutRollout) {
    if (rollout.spec['smartnic-must-match-constraint']) {
      const orderConstraints = [];
      const orders = this.getOrderConstraints();
      for ( let ix = 0; ix < orders.length; ix++) {
        const labelsSelectorCriteria = this.convertFormArrayToSearchExpression(orders[ix].value); // Some Naples will be updated.
        const requirements = labelsSelectorCriteria;
        const obj = { 'requirements' : requirements};
        orderConstraints.push(obj);
      }
      rollout.spec['order-constraints'] = orderConstraints;
    } else {
      rollout.spec['order-constraints'] = []; // All Naples will be updated.
    }
  }

  _saveRollout() {
    const rollout: IRolloutRollout = this.buildRollout();
    let handler: Observable<{ body: IRolloutRollout | IApiStatus | Error, statusCode: number }>;

    if (this.isInline) {
      // Using this.newRollout to get name, as the name is gone when we call getFormGroupValues
      // This is beacuse we disabled it in the form group to stop the user from editing it.
      // When you disable an angular control, in doesn't show up when you get the value of the group

      handler = this.rolloutService.UpdateRollout(rollout);
    } else {
      handler = this.rolloutService.CreateRollout(rollout);
    }
    const sub = handler.subscribe(
      (response) => {
        if (this.isInline) {
          this._controllerService.invokeSuccessToaster(Utility.UPDATE_SUCCESS_SUMMARY, 'Updated rollout ' + this.newRollout.meta.name);
        } else {
          this._controllerService.invokeSuccessToaster(Utility.CREATE_SUCCESS_SUMMARY, 'Created rollout ' + rollout.meta.name);
        }
        this.cancelRollout();
      },
      (error) => {
        if (this.isInline) {
          this._controllerService.invokeRESTErrorToaster(Utility.UPDATE_FAILED_SUMMARY, error);
        } else {
          this._controllerService.invokeRESTErrorToaster(Utility.CREATE_FAILED_SUMMARY, error);
        }
      }
    );
    this.subscriptions.push(sub);
  }

  cancelRollout() {
    if (!this.isInline) {
      // Need to reset the toolbar that we changed
      this.setPreviousToolbar();
    }
    this.formClose.emit(false);
  }

  /**
   * Sets the previously saved toolbar buttons
   * They should have been saved in the ngOnInit when we are inline.
   */
  setPreviousToolbar() {
    if (this.oldToolbarData != null) {
      this._controllerService.setToolbarData(this.oldToolbarData);
    }
  }

  onVersionChange($event) {
    const selectedVersion = $event.value;
    this.computeVersionDescription(selectedVersion);
  }

  computeVersionDescription(selectedVersion: string) {
    const rolloutImageLabel = this.getImageVersionData(selectedVersion);
    this.versiondescription = rolloutImageLabel ? this.buildVersionDisplay(rolloutImageLabel) : '';
  }

  buildVersionDisplay(rolloutImageLabel: RolloutImageLabel): string {
    let result = '';
    if (rolloutImageLabel.Description) {
      result += rolloutImageLabel.Description;
    }
    if (rolloutImageLabel.Releasedate) {
      result += ' Release: ' + rolloutImageLabel.Releasedate;
    }
    if (rolloutImageLabel.Environment) {
      result += ' Env: ' + rolloutImageLabel.Environment;
    }
    return result;
  }

  getImageVersionData(version: string): RolloutImageLabel {
    const targets: RolloutImageOption[] = this.rolloutImageOptions.filter(selectItem => {
      return (selectItem.value === version);
    });
    return (targets && targets.length > 0) ? targets[0].model : null;
  }

  /**
   * This API serves html template
   */
  onNicNodeTypeChange($event) {
    this.selectedRolloutNicNodeTypes = $event.value;
  }

  /**
   * This API serves html template
   */
  onNICsConstraintsChange($event) {
    // debug. console.log(this.getClassName() + '.onNICsConstraintsChange()' + event);
  }

  /**
   * This API serves html template
   */
  isToShowNodeDiv(): boolean {
    return (this.selectedRolloutNicNodeTypes !== RolloutUtil.ROLLOUTTYPE_NAPLES_ONLY);
  }

  /**
   * This API serves html template
   */
  isToShowNicDiv(): boolean {
    return (this.selectedRolloutNicNodeTypes !== RolloutUtil.ROLLOUTTYPE_VENICE_ONLY);
  }

  /**
   * This API serves html template
   */
  isToShowNicOrderDiv(): boolean {
    return this.newRollout.$formGroup.get(['spec', 'smartnic-must-match-constraint']).value;
  }

  /**
   * Customized placeholder for key
   * @param repeater
   * @param keyFormName
   */
  buildKeyPlaceholder(repeater: any, keyFormName: string): string {
    return 'key';
  }

  /**
   * Customized placeholder value
   * @param repeater
   * @param keyFormName
   */
  buildValuePlaceholder(repeater: any, keyFormName: string): string {
    return 'value';
  }

  // adds new HTML element with labelselectors
  // automatically collapses all open orders
  addOrder() {
    const newOrder = new RolloutOrder();
    newOrder.id = Utility.s4();
    newOrder.data = new LabelsSelector().$formGroup;
    this.orders.push(newOrder);
    this.makeOrderEditable(this.orders.length - 1, null);
    this.orders[this.orders.length - 1].incomplete = true;
  }

  // removes order
  deleteOrder(index) {
    this.orders.splice(index, 1);
  }

  // toggle order state between saved and editable
  // Make a saved order editable will make all other orders closed.
  toggleOrderState(index) {
    if (this.orders[index].inEdit && this.orders.length > 1) {
      this.makeOrderSaved(index);
    } else {
      this.makeOrderEditable(index, null);
    }
  }

  // Only one order can be edited at one time.
  // If we create a new order or edit an existing one, previously open order is automatically saved.
  makeOrderEditable(index, event) {
    for (let i = 0; i < this.orders.length; i++) {
      if (i !== index && this.orders[i].inEdit) {
        this.makeOrderSaved(i);
      } else if (i === index) {
        this.orders[index].inEdit = true;
      }
    }
    if (event) {
      event.stopPropagation();
    }
  }

  // "save" simply hides the label selectors
  // save updates the order summary string and the matchedNIC count.
  makeOrderSaved(index) {
    this.orders[index].inEdit = false;
    this.matchNICs(index);
    this.setOrderSummary(index);
    this.buildDuplicationMap();
  }

  // saved orders show a summary of the label selectors contained inside.
  setOrderSummary(index) {
    const data = this.orders[index].data.value.requirements;
    const reqs = Utility.formatRepeaterData(data);
    const stringForm = Utility.stringifyRepeaterData(reqs).join('    ');
    this.orders[index].orderSummary = stringForm;
  }

  moveOrder(index, direction) {
    const newIndex = index + direction;

    if (newIndex < 0 || newIndex >= this.orders.length) {
      return;
    }

    const tempC = this.orders[index];
    this.orders[index] = this.orders[newIndex];
    this.orders[newIndex] = tempC;
  }

  repeaterValueChange(index) {
    setTimeout(() => {
      this.matchNICs(index);
      this.setOrderSummary(index);
      this.buildDuplicationMap();
      const repeaterSearchExpression: SearchExpression[] = this.convertFormArrayToSearchExpression(this.orders[index].data.value);
      if ( repeaterSearchExpression.length > 0 ) {
        this.orders[index].incomplete = false;
        for (let i = 0; i < repeaterSearchExpression.length; i++) {
          if (Utility.isEmpty(repeaterSearchExpression[i].key ) || Utility.isEmpty(repeaterSearchExpression[i].values)) {
            this.orders[index].incomplete = true;
          }
        }
      } else {
        this.orders[index].incomplete = true;
      }
    }, 0);
  }

  matchNICs(index) {
    const repeaterValues = this.orders[index].data.value.requirements;
    const matchRules = [];
    for (let ix = 0; ix < repeaterValues.length; ix++) {
      matchRules.push([repeaterValues[ix].keyFormControl, repeaterValues[ix].operatorFormControl, repeaterValues[ix].valueFormControl]);
    }
    this.updateMatches(index, matchRules);
  }

  makeRuleMapKey(labelKey, labelValue) {
    return labelKey + '=' + labelValue;
  }

  // Compares the labelselector rules with the naplesRuleMap to get all matched NICs
  getRuleResults(matchRule) {
    const ok: boolean = true;
    let naplesNICSet: string[] = [];
    if (typeof matchRule[2] === 'object' && !!matchRule[2]) {
      for (const i of matchRule[2]) {
        const rule = this.makeRuleMapKey(matchRule[0], i);
        if (rule in this.naplesRuleMap) {
          naplesNICSet = naplesNICSet.concat(Array.from(this.naplesRuleMap[rule]));
        }
      }
    } else {
      const rule = this.makeRuleMapKey(matchRule[0], matchRule[2]);
      if (rule in this.naplesRuleMap) {
        naplesNICSet = Array.from(this.naplesRuleMap[rule]);
      }
    }

    if (naplesNICSet.length > 0) {
      return naplesNICSet;
    } else {
      return null;
    }
  }

  getAllNICs() {
    const naplesNICSet: Set<string> = new Set<string>();
    for (const item of this.naples) {
      naplesNICSet.add(item.meta.name);
    }
    return Array.from(naplesNICSet);
  }

  updateMatches(index, matchRules) {
    let naplesNICSet: string[] = null;
    for (const matchRule of matchRules) {
      const ret = this.getRuleResults(matchRule);
      if (ret === null) {
        break; // since all rules are 'AND'ed together. If one matches nothing, we dont have to check for the rest.
      }
      if (matchRule[1] === 'in') {
        if (naplesNICSet === null) {
          naplesNICSet = Array.from(ret);
        }
        naplesNICSet = naplesNICSet.filter(x => Array.from(ret).includes(x));
      } else {
        if (naplesNICSet === null) {
          naplesNICSet = this.getAllNICs();
        }
        naplesNICSet = naplesNICSet.filter(x => !Array.from(ret).includes(x));
      }
    }

    if (naplesNICSet === null) { this.orders[index].matchedSmartNICs = []; } else { this.orders[index].matchedSmartNICs = naplesNICSet; }
  }

  // If there are no labels added (for any of the NICs), the upgrade using labels toggle button is hidden.
  enableUpgradeByLabel() {
    if (Object.keys(this.naplesRuleMap).length > 0) {
      return true;
    } else {
      return false;
    }
  }

  duplicateMatchTooltip(index) {
    let msg = 'Has common matches with Order';
    msg += this.orders[index].duplicatesWith.length > 1 ? 's ' : ' ';
    const arr = this.orders[index].duplicatesWith.map( (value) => {
      return value + 1;
    } );
    msg += arr.join(', ');
    return msg;
  }

  buildDuplicationMap() {
    this.duplicateMatchMap = {};
    for (let ix = 0; ix < this.orders.length; ix++) {
      const order = this.orders[ix];
      this.orders[ix].duplicatesWith = [];
      for (const match of order.matchedSmartNICs) {
        if ( !(match in this.duplicateMatchMap) ) {
          this.duplicateMatchMap[match] = [ix];
        } else {
          this.orders[ix].duplicatesWith = this.orders[ix].duplicatesWith.concat([...this.duplicateMatchMap[match]]);
          this.duplicateMatchMap[match].push(ix);
        }
      }
      // sort and get unique
      this.orders[ix].duplicatesWith = this.orders[ix].duplicatesWith.sort().filter(function(el, i, a) {return i === a.indexOf(el); });
    }
  }
}
