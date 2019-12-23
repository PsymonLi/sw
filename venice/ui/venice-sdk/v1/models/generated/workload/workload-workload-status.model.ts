/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { SecurityPropagationStatus, ISecurityPropagationStatus } from './security-propagation-status.model';
import { WorkloadWorkloadIntfStatus, IWorkloadWorkloadIntfStatus } from './workload-workload-intf-status.model';

export interface IWorkloadWorkloadStatus {
    'propagation-status'?: ISecurityPropagationStatus;
    'interfaces'?: Array<IWorkloadWorkloadIntfStatus>;
}


export class WorkloadWorkloadStatus extends BaseModel implements IWorkloadWorkloadStatus {
    /** The status of the configuration propagation to the Naples */
    'propagation-status': SecurityPropagationStatus = null;
    /** Status of all interfaces in the Workload identified by Primary MAC */
    'interfaces': Array<WorkloadWorkloadIntfStatus> = null;
    public static propInfo: { [prop in keyof IWorkloadWorkloadStatus]: PropInfoItem } = {
        'propagation-status': {
            description:  `The status of the configuration propagation to the Naples`,
            required: false,
            type: 'object'
        },
        'interfaces': {
            description:  `Status of all interfaces in the Workload identified by Primary MAC`,
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return WorkloadWorkloadStatus.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return WorkloadWorkloadStatus.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (WorkloadWorkloadStatus.propInfo[prop] != null &&
                        WorkloadWorkloadStatus.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['propagation-status'] = new SecurityPropagationStatus();
        this['interfaces'] = new Array<WorkloadWorkloadIntfStatus>();
        this._inputValue = values;
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values) {
            this['propagation-status'].setValues(values['propagation-status'], fillDefaults);
        } else {
            this['propagation-status'].setValues(null, fillDefaults);
        }
        if (values) {
            this.fillModelArray<WorkloadWorkloadIntfStatus>(this, 'interfaces', values['interfaces'], WorkloadWorkloadIntfStatus);
        } else {
            this['interfaces'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'propagation-status': CustomFormGroup(this['propagation-status'].$formGroup, WorkloadWorkloadStatus.propInfo['propagation-status'].required),
                'interfaces': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<WorkloadWorkloadIntfStatus>('interfaces', this['interfaces'], WorkloadWorkloadIntfStatus);
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('propagation-status') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('propagation-status').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('interfaces') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('interfaces').get(field);
                control.updateValueAndValidity();
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this['propagation-status'].setFormGroupValuesToBeModelValues();
            this.fillModelArray<WorkloadWorkloadIntfStatus>(this, 'interfaces', this['interfaces'], WorkloadWorkloadIntfStatus);
        }
    }
}

