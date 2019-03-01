/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';


export interface IClusterTenantSpec {
    'admin-user'?: string;
}


export class ClusterTenantSpec extends BaseModel implements IClusterTenantSpec {
    'admin-user': string = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'admin-user': {
            required: false,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return ClusterTenantSpec.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return ClusterTenantSpec.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (ClusterTenantSpec.propInfo[prop] != null &&
                        ClusterTenantSpec.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['admin-user'] != null) {
            this['admin-user'] = values['admin-user'];
        } else if (fillDefaults && ClusterTenantSpec.hasDefaultValue('admin-user')) {
            this['admin-user'] = ClusterTenantSpec.propInfo['admin-user'].default;
        } else {
            this['admin-user'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'admin-user': CustomFormControl(new FormControl(this['admin-user']), ClusterTenantSpec.propInfo['admin-user']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['admin-user'].setValue(this['admin-user']);
        }
    }
}

