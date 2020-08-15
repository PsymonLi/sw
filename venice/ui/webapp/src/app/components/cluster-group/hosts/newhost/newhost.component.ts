import { AfterViewInit, ChangeDetectionStrategy, Component, EventEmitter, Input, OnDestroy, OnInit, Output, ViewEncapsulation, ChangeDetectorRef, SimpleChanges, OnChanges } from '@angular/core';
import { AbstractControl, FormArray, FormGroup, ValidationErrors, ValidatorFn, FormControl } from '@angular/forms';
import { Animations } from '@app/animations';
import { Utility } from '@app/common/Utility';
import { TableCol } from '@app/components/shared/tableviewedit';
import { ControllerService } from '@app/services/controller.service';
import { ClusterService } from '@app/services/generated/cluster.service';
import { StagingService } from '@app/services/generated/staging.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { ClusterDistributedServiceCard, IApiStatus } from '@sdk/v1/models/generated/cluster';
import { ClusterDistributedServiceCardID } from '@sdk/v1/models/generated/cluster/cluster-distributed-service-card-id.model';
import { ClusterHost, IClusterHost } from '@sdk/v1/models/generated/cluster/cluster-host.model';
import { StagingBuffer, StagingCommitAction } from '@sdk/v1/models/generated/staging';
import { patternValidator } from '@sdk/v1/utils/validators';
import { forkJoin, Observable, throwError } from 'rxjs';
import { switchMap } from 'rxjs/operators';
import { CreationForm } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { SelectItem } from 'primeng/api';


export interface DSCIDuiModel {
  isUseID: boolean;
}
/**
 * NewhostComponent extends CreationPushForm
 * It enables adding and updating host object.  User must specify to use "ID" or "MAC" in UI-html
 * Internally, 'radioValue' holds the selected value (ID or MAC)
 *
 *  postNgInit() -> getPreSelectedDistributedServiceCardID() // when in edit mode, we compute the radio value from data
 *
 */
/**
 * Host edit logic 2020-05-13
 *
 * Server does not allow user to change DSC already assigned to a host.
 * If host already has two DSCs. User can only delete the host, but can not change DSCs.
 *
 * If a host has only one DSC, user can add another DSC.
 */

/*
By 2020-03-24, one host can host tow DSCs.  We can mix DSC id or DSC mac-address as shown below. We can use either id or mac-address, but not both
We use this.radioValues[i] to keep track wheterh the i-th DSC is using id or address.
Host{
  ...
  "spec": {
         "dscs": [
           {
             "mac-address": “00ae.cd00.0009”  // DSC-A’s mac
           },
            {
             “id": “DSC-B  “  // using DSC-B’s id
           }
         ]
       }
*/
@Component({
  selector: 'app-newhost',
  templateUrl: './newhost.component.html',
  styleUrls: ['./newhost.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class NewhostComponent extends CreationForm<IClusterHost, ClusterHost> implements OnInit, AfterViewInit, OnChanges, OnDestroy {

  public static KEYS_ID = 'id';
  public static KEYS_MACADDRESS = 'mac-address';

  public static MACADDRESS_REGEX: string = '^([0-9a-fA-F]{4}[.]){2}([0-9a-fA-F]{4})$';
  public static MACADDRESS_MESSAGE: string = 'MAC address must be of the format aaaa.bbbb.cccc';

  @Input() isInline: boolean = false;
  @Input() existingObjects: IClusterHost[] = [];
  @Input() objectData: IClusterHost;
  @Input() naplesWithoutHosts: ClusterDistributedServiceCard[] = [];
  @Input() notAdmittedCount: number = 0;
  @Input() allDSCs: ClusterDistributedServiceCard[] = [];
  @Output() formClose: EventEmitter<any> = new EventEmitter();

  newHostForm: FormGroup;
  addNICoptions: SelectItem[] = [
    {label: 'ID', value: 'id'},
    {label: 'MAC ADDRESS', value: 'mac-address'}
  ];
  originalDSCcount: number = 0;

  dscIds: string[] = [];
  dscMacs: string[] = [];

  idSuggestions: string[] = [];
  macSuggestions: string[] = [];

  cols: TableCol[] = [
    { field: 'meta.name', header: 'DSC Name', class: '', sortable: true, width: 20 },
    { field: 'meta.mac-address', header: 'Mac Address', class: '', sortable: true, width: 20 },
    { field: 'add_host', header: 'Add Host', class: '', sortable: true, width: 60 },
  ];

  objectMap: any = {};

  constructor(protected _controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected _clusterService: ClusterService,
    protected stagingService: StagingService,
    protected cdr: ChangeDetectorRef,
  ) {
    super(_controllerService, uiconfigsService, cdr, ClusterHost);
  }

  postNgInit() {
    this.newHostForm = this.newObject.$formGroup;
    const smartNICIDs: any = this.newHostForm.get(['spec', 'dscs']);
    if (!this.isInline) {
      this.processNaples();
      this.addFieldValidator(this.newObject.$formGroup.get(['meta', 'name']),
        this.isNewHostNameValid(this.existingObjects));
    } else {
      this.newObject.$formGroup.get(['meta', 'name']).disable();
      // in edit mode.  We compute the radio (ID/MAC) value
      for (let i = 0; i < smartNICIDs.controls.length; i++) {
        const formGroup = smartNICIDs.controls[i];
        formGroup.addControl('addDscType', new FormControl());
        if (formGroup.get(['id']).value) {
          formGroup.get(['addDscType']).setValue('id');
        } else {
          formGroup.get(['addDscType']).setValue('mac-address');
        }
        formGroup.get(['addDscType']).disable();
        formGroup.get(['id']).disable();
        formGroup.get(['mac-address']).disable();
      }
    }

    if (smartNICIDs.controls.length === 0) {
      this.addDSCID(this.newObject);
    }
  }

  ngOnChanges(changes: SimpleChanges) {
    if (changes.allDSCs) {
      this.dscIds = [];
      this.dscMacs = [];
      if (this.allDSCs) {
        this.allDSCs.forEach(dsc => {
          this.dscIds.push(dsc.spec.id);
          this.dscMacs.push(dsc.meta.name);
        });
      }
    }
    super.ngOnChanges(changes);
  }

  searchDscIDs(event) {
    const query: string = event.query;
    if (query && query.trim().length >= 2) {
      this.idSuggestions = this.searchStringWithinArray(query.trim(), this.dscIds);
    } else {
      this.idSuggestions = [];
    }
  }

  searchDscMACs(event) {
    const query: string = event.query;
    if (query && query.length >= 2) {
      this.macSuggestions = this.searchStringWithinArray(query.trim(), this.dscMacs);
    } else {
      this.macSuggestions = [];
    }
  }

  searchStringWithinArray(str: string, array: string[]) {
    if (!array || array.length === 0) {
      return [];
    }
    const result: string[] = array.filter(val =>
      val.toLowerCase().indexOf(str.toLowerCase()) > -1);
    const newResult = [];
    for (let i = 0; i < result.length && i < 11; i++) {
      newResult.push(result[i]);
    }
    return newResult;
  }

  /**
   * Set input validator
   *   - new host.meta.name must be unique
   *   - set validator to host.spec.dscs ( id or mac-address fields)
   * @param newObject
   */
  setValidators(newObject: ClusterHost) {
    newObject.$formGroup.get(['meta', 'name']).setValidators([
      this.newObject.$formGroup.get(['meta', 'name']).validator,
      this.isNewHostNameValid(this.existingObjects)]);
    const smartNICIDs: any = this.newHostForm.get(['spec', 'dscs']);
    for (let i = 0; i < smartNICIDs.controls.length; i++) {
      this.setDSCValidator(newObject, i);
    }
  }

  /**
   * Set validator to host.spcc.dscs[index]
   *
   * host.spcc.dscs[index]['mac-address'] has to be unique across all hosts
   * host.spcc.dscs[index]['id'] has to be unique across all hosts
   *
   * Within one host,  dscs[i] and dscs[j] can not have duplicate values id/mac-addrsess
   * "dscs": // this is wrong (two dsc share the mac-address/id values within one host)
   *     [
   *        {
   *          "mac-address": “00ae.cd00.0009”  // id : abc
   *        },
   *        {
   *         “mac-address“  "00ae.cd00.0009"   // id : abc
   *         }
   *     ]
   * @param newObject
   * @param index
   */
  private setDSCValidator(newObject: ClusterHost, index: number) {
    newObject.$formGroup.get(['spec', 'dscs', index, 'mac-address']).setValidators([
      patternValidator(Utility.MACADDRESS_REGEX, NewhostComponent.MACADDRESS_MESSAGE),
      this.dscMacValidator(this.existingObjects, index)
    ]);
    newObject.$formGroup.get(['spec', 'dscs', index, 'id']).setValidators([
      this.dscIDValidator(this.existingObjects, index)
    ]);
  }

  dscIDValidator(hosts: IClusterHost[], dscIndex: number): ValidatorFn {
    return (control: AbstractControl): ValidationErrors | null => {
      if (this.isDSCIdOccupied(control.value, hosts) || this.isDSCIdOrMacDuplicateWithinHost(control as FormControl, 'id')) {
        return {
          objectname: {
            required: true,
            message: 'DSC ID is already assigned to a host'
          }
        };
      }
      return null;
    };
  }

  dscMacValidator(hosts: IClusterHost[], dscIndex: number): ValidatorFn {
    return (control: AbstractControl): ValidationErrors | null => {
      if (this.isDSCMacOccupied(control.value, hosts) || this.isDSCIdOrMacDuplicateWithinHost(control  as FormControl, 'mac-address')) {
        return {
          objectname: {
            required: true,
            message: 'DSC MAC address is already assigned to a host'
          }
        };
      }
      return null;
    };
  }

  isDSCIdOrMacDuplicateWithinHost(ctrl: FormControl, field: string): boolean {
    const len = this.newHostForm.get(['spec', 'dscs'])['length'];
    if (len <= 1) {
      return false;
    }
    for (let i = 0; i < len; i++) {
      const eachCtrl = this.newHostForm.get(['spec', 'dscs', i, field]);
      if (ctrl !== eachCtrl) {
        if (eachCtrl && eachCtrl.value === ctrl.value) {
          return true;
        }
      } else {

      }
    }
    return false;
  }

  /**
   *
   * @param inputvalue
   * @param hosts
   * @param field
   */
  isDSCIdOrMacOccupied(inputvalue: string, hosts: IClusterHost[], field: string): boolean {
    const matchedDSCs = hosts.findIndex(host => {
      return host.spec.dscs.findIndex(dsc => dsc[field] === inputvalue) > -1;
    });
    return (matchedDSCs > -1);
  }

  /**
   * Say user inputs dscId (using id radio button)
   * This dscId can not be equal to  any  host.spec.dscs[i].id and dsc.spec.id
   * @param dscId
   * @param hosts
   */
  isDSCIdOccupied(dscId: string, hosts: IClusterHost[]): boolean {
    if (this.isDSCIdOrMacOccupied(dscId, hosts, 'id')) {
      return true;
    }
    if (Utility.isValueOrArrayEmpty(this.allDSCs)) {
      return false;
    }
    const dsc = this.allDSCs.find(item => item.spec.id === dscId);
    if (!dsc) {
      return false;
    }
    return this.isDSCIdOrMacOccupied(dsc.meta.name, hosts, 'mac-address');
  }

  isDSCMacOccupied(mac: string, hosts: IClusterHost[]): boolean {
    if (this.isDSCIdOrMacOccupied(mac, hosts, 'mac-address')) {
      return true;
    }
    if (Utility.isValueOrArrayEmpty(this.allDSCs)) {
      return false;
    }
    const dsc = this.allDSCs.find(item => item.meta.name === mac);
    if (!dsc) {
      return false;
    }
    return this.isDSCIdOrMacOccupied(dsc.spec.id, hosts, 'id');
  }

  isShowDeleteButton(index: number) {
    if (this.isInline) {
      return index > 0;
    }
    return (this.newHostForm.get(['spec', 'dscs']) as FormArray).length > 1;
  }

  processNaples() {
    this.objectMap = {};
    for (const dsc of this.naplesWithoutHosts) {
      if (!dsc.status.host) {
        const newHost = new ClusterHost();
        this.addDSCID(newHost);
        this.setValidators(newHost);
        newHost.$formGroup.get(['spec', 'dscs', 0, 'mac-address']).setValue(dsc.status['primary-mac']);
        this.objectMap[dsc.meta.name] = newHost;
      }
    }
  }

  getFormGroup(rowData: ClusterHost): FormGroup {
    return this.objectMap[rowData.meta.name].$formGroup;
  }

  addDSCID(newObject: ClusterHost) {
    // updates the form
    const smartNICIDs = newObject.$formGroup.get(['spec', 'dscs']) as FormArray;
    const len = smartNICIDs.length;
    const clusterDistributedServiceCardID: ClusterDistributedServiceCardID = new ClusterDistributedServiceCardID();
    smartNICIDs.push(clusterDistributedServiceCardID.$formGroup);
    clusterDistributedServiceCardID.$formGroup.get(['mac-address']).setValidators([
      patternValidator(Utility.MACADDRESS_REGEX, NewhostComponent.MACADDRESS_MESSAGE),
      this.dscMacValidator(this.existingObjects, len)
    ]);
    clusterDistributedServiceCardID.$formGroup.get(['id']).setValidators([
      this.dscIDValidator(this.existingObjects, len)
    ]);
    clusterDistributedServiceCardID.$formGroup.addControl('addDscType', new FormControl());
    clusterDistributedServiceCardID.$formGroup.get(['addDscType']).setValue('id');
  }

  computeFormSubmitButtonClass() {
    let enable = false;
    if (this.notAdmittedCount !== 0) {
      for (const key of Object.keys(this.objectMap)) {
        const i = this.objectMap[key].$formGroup;
        if (!Utility.isEmpty(i.get('meta.name').value) && i.get('meta.name').status === 'VALID') {
          enable = true;
        }
      }
    } else if (this.isFormValid()) {
      enable = true;
    }

    if (enable) {
      return '';
    }
    return 'global-button-disabled';
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

    const smartNICs: FormArray = this.newHostForm.get(['spec', 'dscs']) as FormArray;
    for (let i = 0; i < smartNICs.length; i++) {
      const formGroup: FormGroup = smartNICs.get([i]) as FormGroup;
      const type = formGroup.get('addDscType').value;
      if (type === 'id') {
        if (this.isFieldEmpty(formGroup.get('id'))) {
          this.submitButtonTooltip = 'Error: Field ' + (i + 1) + ' ID is empty.';
          return false;
        }
        if (formGroup.get('id').invalid) {
          this.submitButtonTooltip = 'Error: Field ' + (i + 1) + ' ID is invalid.';
          return false;
        }
      } else if (type === 'mac-address') {
        if (this.isFieldEmpty(formGroup.get('mac-address'))) {
          this.submitButtonTooltip = 'Error: Field ' + (i + 1) + ' MAC Address is empty.';
          return false;
        }
        if (formGroup.get('mac-address').invalid) {
          this.submitButtonTooltip = 'Error: Field ' + (i + 1) + ' MAC Address is invalid.';
          return false;
        }
      }
    }

    this.submitButtonTooltip = 'Ready to submit';
    return true;
  }

  isNewHostNameValid(existingObjects: IClusterHost[]): ValidatorFn {
    // checks if name field is valid
    return Utility.isModelNameUniqueValidator(existingObjects, 'newHost-name');
  }

  getObjectValues(): IClusterHost {
    const newClusterHost: IClusterHost = this.newObject.getFormGroupValues();
    for (let i = 0; i < newClusterHost.spec.dscs.length; i++) {
      const dsc: any = newClusterHost.spec.dscs[i];
      if (dsc.addDscType === 'id') {
        delete dsc['mac-address'];
      } else {
        delete dsc.id;
      }
      delete dsc.addDscType;
    }
    return newClusterHost;
  }

  setToolbar(): void {
    if (!this.isInline) {
      // If it is not inline, we change the toolbar buttons, and save the old one
      // so that we can set it back when we are done
      const currToolbar = this._controllerService.getToolbarData();
      this.oldButtons = currToolbar.buttons;
      currToolbar.buttons = [
        {
          cssClass: 'global-button-primary psm-form-creation-button',
          text: 'CREATE HOSTS',
          genTooltip: () => this.getSubmitButtonToolTip(),
          callback: () => { this.saveObjects(); },
          computeClass: () => this.computeFormSubmitButtonClass()
        },
        {
          cssClass: 'global-button-neutral psm-form-creation-button',
          text: 'CANCEL',
          callback: () => {
            this.cancelObject();
          }
        },
      ];

      this._controllerService.setToolbarData(currToolbar);
    }
  }

  /**
   * override super api
   * We make up the JSON object
   */
  createObject(newObject: IClusterHost): Observable<{ body: IClusterHost | IApiStatus | Error; statusCode: number; }> {
    return this._clusterService.AddHost(newObject);
  }

  /**
   * override super api
   * We make up the JSON object
   */
  updateObject(newObject: IClusterHost, oldObject: IClusterHost): Observable<{ body: IClusterHost | IApiStatus | Error; statusCode: number; }> {
    if (newObject && newObject.spec && newObject.spec.dscs && newObject.spec.dscs.length > 0 &&
      newObject.spec.dscs[0]) {
      if (oldObject && oldObject.spec && oldObject.spec.dscs) {
        oldObject.spec.dscs.push(newObject.spec.dscs[0]);
      }
    }
    return this._clusterService.UpdateHost(oldObject.meta.name, oldObject);
  }

  generateCreateSuccessMsg(object: IClusterHost): string {
    return 'Created host ' + object.meta.name;
  }

  generateUpdateSuccessMsg(object: IClusterHost): string {
    return 'Updated host ' + object.meta.name;
  }

  onSaveFailure(isCreate: boolean) {
    // reset original value
    this.newObject.setFormGroupValuesToBeModelValues();
  }

  commitStagingBuffer(buffername: string): Observable<any> {
    const commitBufferBody: StagingCommitAction = Utility.buildCommitBufferCommit(buffername);
    return this.stagingService.Commit(buffername, commitBufferBody);
  }

  createStagingBuffer(): Observable<any> {
    const stagingBuffer: StagingBuffer = Utility.buildCommitBuffer();
    return this.stagingService.AddBuffer(stagingBuffer);
  }

  deleteStagingBuffer(buffername: string, reason: string, isToshowToaster: boolean = true) {
    if (buffername == null) {
      return;
    }

    // Whenever, we have to call delete buffer, there must be error occurred. We print out the buffer detail here.
    this.stagingService.GetBuffer(buffername).subscribe((res) => {
      console.error(this.getClassName() + '.deleteStagingBuffer() API. Invoke GetBuffer():', res);
    });
    this.stagingService.DeleteBuffer(buffername).subscribe(
      response => {
        if (isToshowToaster) {
          this._controllerService.invokeSuccessToaster('Successfully deleted buffer', 'Deleted Buffer ' + buffername + '\n' + reason);
        }
      },
      this._controllerService.restErrorHandler('Delete Staging Buffer Failed')
    );
  }

  saveBatchMode() {
    let createdBuffer: StagingBuffer = null;
    let bufferName: string;
    const observables: Observable<any>[] = [];
    this.createStagingBuffer()
      .pipe(
        switchMap(responseBuffer => {
          createdBuffer = responseBuffer.body as StagingBuffer;
          bufferName = createdBuffer.meta.name;
          for (const key of Object.keys(this.objectMap)) {
            const object = this.objectMap[key];
            const policy = object.getFormGroupValues();

            const fg = object.$formGroup;
            if (Utility.isEmpty(fg.get('meta.name').value) || fg.get('meta.name').status !== 'VALID') {
              continue;
            }

            const observable = this._clusterService.AddHost(policy, bufferName);
            if (observable == null) {
              continue;
            }
            observables.push(observable);
          }

          if (observables.length > 0) {
            return forkJoin(observables)
              .pipe(
                switchMap(results => {
                  const isAllOK = Utility.isForkjoinResultAllOK(results);
                  if (isAllOK) {
                    return this.commitStagingBuffer(bufferName);
                  } else {
                    const error = Utility.joinErrors(results);
                    return throwError(error);
                  }
                })
              );
          }
        })
      ).subscribe((responseCommitBuffer) => {
        this._controllerService.invokeSuccessToaster(Utility.CREATE_SUCCESS_SUMMARY, 'Successfully added Hosts');
        this.deleteStagingBuffer(bufferName, 'Batch add successful', false);
        this.cancelObject();
      },
        (error) => {
          // any error in (A) (B) or (C), error will land here
          this._controllerService.invokeRESTErrorToaster(Utility.CREATE_FAILED_SUMMARY, error);
          this.deleteStagingBuffer(bufferName, 'Failed to add hosts', false);
        }
      );
  }

  saveSingleAddMode() {
    this.saveObject();
  }

  saveObjects() {
    if (this.notAdmittedCount === 0) {
      this.saveSingleAddMode();
    } else {
      this.saveBatchMode();
    }
  }

  /**
   * When user call + to add a new  DSC
   */
  addDSC() {
    this.addDSCID(this.newObject);
  }

  /**
   * When uesr click delete icon to remvoe a DSC
   * @param index
   */
  removeDSC(index) {
    const tempTargets = this.newHostForm.get(['spec', 'dscs']) as FormArray;
    if (tempTargets.length > 1) {
      tempTargets.removeAt(index);
    }
  }
}
