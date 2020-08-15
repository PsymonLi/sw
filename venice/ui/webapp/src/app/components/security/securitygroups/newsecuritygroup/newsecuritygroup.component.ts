import { AfterViewInit, Component, EventEmitter, Input, OnInit, Output, ViewEncapsulation, OnDestroy, ViewChild, ChangeDetectorRef, ChangeDetectionStrategy} from '@angular/core';
import { Animations } from '@app/animations';
import { BaseComponent } from '@app/components/base/base.component';
import { ToolbarButton } from '@app/models/frontend/shared/toolbar.interface';
import { ControllerService } from '@app/services/controller.service';
import { IApiStatus, ILabelsRequirement } from '@sdk/v1/models/generated/security';
import { ISecuritySecurityGroup, SecuritySecurityGroup} from '@sdk/v1/models/generated/security/security-security-group.model';
import { WorkloadWorkload } from '@sdk/v1/models/generated/workload';
import { SecurityService } from '@app/services/generated/security.service';
import { Observable } from 'rxjs';
import { Utility } from '@app/common/Utility';
import { SearchUtil } from '@app/components/search/SearchUtil';
import { FormGroup, FormArray, ValidatorFn, FormControl } from '@angular/forms';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { IPUtility } from '@app/common/IPUtility';
import { CreationForm } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { RepeaterData, ValueType } from 'web-app-framework';

@Component({
  selector: 'app-newsecuritygroup',
  templateUrl: './newsecuritygroup.component.html',
  styleUrls: ['./newsecuritygroup.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush,
})

/**
 * NewsecuritygroupComponent lets user to create and update security-group object.
 *
 * There app-chips and app-repeater in html template.
 * When repeater's value change NewsecuritygroupComponent.labelOutput is updated.
 *
 * Key APIs
 *  buildSecurityGroupObject() // It builds object for create and update operations.
 */
export class NewsecuritygroupComponent  extends CreationForm<ISecuritySecurityGroup, SecuritySecurityGroup> implements OnInit, AfterViewInit, OnDestroy {


  @Input() existingObjects: ISecuritySecurityGroup[] = [];
  @Input() workloads: WorkloadWorkload [] = [];

  newSecuritygroupForm: FormGroup;
  labelData: RepeaterData[] = [];
  labelOutput: any;
  labelFormArray = new FormArray([]);

  constructor(protected _controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected _securityService: SecurityService,
    protected cdr: ChangeDetectorRef
  ) {
    super(_controllerService, uiconfigsService,  cdr, SecuritySecurityGroup);
  }

  postNgInit() {
    this.labelData = [
      {
        key: { label: 'text', value: 'text' },
        operators: [
          { label: 'equals', value: 'in' }
        ],
        fieldType: ValueType.inputField,
        valueType: ValueType.inputField
      }
    ];
    if (this.objectData != null) {
      const list = this.buildLabelFormControlList(this.objectData.spec['workload-selector'].requirements);
      this.labelFormArray = new FormArray(list);
    }

    this.newSecuritygroupForm = this.newObject.$formGroup;

    if (this.isInline) {
      // disable name field
      this.newObject.$formGroup.get(['meta', 'name']).disable();
    }

    this.newObject.$formGroup.get(['meta', 'name']).setValidators([
      this.newObject.$formGroup.get(['meta', 'name']).validator,
      this.isNewSecurityGroupNameValid(this.existingObjects) ]);
  }

  isNewSecurityGroupNameValid(existingObjects: ISecuritySecurityGroup[]): ValidatorFn {
    // checks if name field is valid
    return Utility.isModelNameUniqueValidator(existingObjects, 'New-SecurityGroup-name');
  }

  protected buildLabelFormControlList(searchExpressons: ILabelsRequirement[]): FormControl[] {
    const list = [];
    searchExpressons.forEach((item) => {
      const op = item.operator;
      const formControl = new FormControl({ keyFormControl: 'text', operatorFormControl: op, valueFormControl: (item.values) ? item.values : [], keytextFormName: item.key });
      list.push(formControl);
    });
    return list;
  }

  setToolbar(): void {
    this.setCreationButtonsToolbar('CREATE SECURITY GROUP', null);
  }

  /**
   * Build Security Object using repeater value
   */
  buildSecurityGroupObject(): ISecuritySecurityGroup {
    const sggroup: ISecuritySecurityGroup = this.newObject.getFormGroupValues();
    const repeaterSearchExpression = SearchUtil.convertFormArrayToSearchExpression(this.labelOutput);
    sggroup.spec['workload-selector'].requirements = repeaterSearchExpression as ILabelsRequirement[];
    return sggroup;
  }

  createObject(object: ISecuritySecurityGroup): Observable<{ body: any; statusCode: number; }> {
    return this._securityService.AddSecurityGroup(object);
  }


  updateObject(newObject: ISecuritySecurityGroup, oldObject: ISecuritySecurityGroup): Observable<{ body: any; statusCode: number; }> {
    return this._securityService.UpdateSecurityGroup(oldObject.meta.name, newObject);
 }

  generateCreateSuccessMsg(object: ISecuritySecurityGroup): string {
    return 'Created security group ' + object.meta.name;
  }

  generateUpdateSuccessMsg(object: ISecuritySecurityGroup): string {
    return 'Updated security group ' + object.meta.name;
  }

  isFormValid(): boolean {
    if (!Utility.isEmpty(this.newObject.$formGroup.get('meta.name').value)
        && this.newSecuritygroupForm.valid
    ) {
      return true;
    }
    return false;
  }

  getObjectValues(): ISecuritySecurityGroup {
    return this.buildSecurityGroupObject();
  }

   /**
   * This API serves html template. When repeater's value is changed, we update labelOutput value.
   */
  handleLabelRepeaterData(values: any) {
    this.labelOutput = values;
  }

  isValidIP(ip: string) {
    return IPUtility.isValidIP(ip);
  }
}
