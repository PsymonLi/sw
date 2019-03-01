/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';


export interface INetworkServiceStatus {
    'workloads'?: Array<string>;
}


export class NetworkServiceStatus extends BaseModel implements INetworkServiceStatus {
    'workloads': Array<string> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'workloads': {
            required: false,
            type: 'Array<string>'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return NetworkServiceStatus.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return NetworkServiceStatus.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (NetworkServiceStatus.propInfo[prop] != null &&
                        NetworkServiceStatus.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['workloads'] = new Array<string>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['workloads'] != null) {
            this['workloads'] = values['workloads'];
        } else if (fillDefaults && NetworkServiceStatus.hasDefaultValue('workloads')) {
            this['workloads'] = [ NetworkServiceStatus.propInfo['workloads'].default];
        } else {
            this['workloads'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'workloads': CustomFormControl(new FormControl(this['workloads']), NetworkServiceStatus.propInfo['workloads']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['workloads'].setValue(this['workloads']);
        }
    }
}

