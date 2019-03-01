/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { AuthPermission, IAuthPermission } from './auth-permission.model';

export interface IAuthRoleSpec {
    'permissions'?: Array<IAuthPermission>;
}


export class AuthRoleSpec extends BaseModel implements IAuthRoleSpec {
    'permissions': Array<AuthPermission> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'permissions': {
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return AuthRoleSpec.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return AuthRoleSpec.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (AuthRoleSpec.propInfo[prop] != null &&
                        AuthRoleSpec.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['permissions'] = new Array<AuthPermission>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values) {
            this.fillModelArray<AuthPermission>(this, 'permissions', values['permissions'], AuthPermission);
        } else {
            this['permissions'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'permissions': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<AuthPermission>('permissions', this['permissions'], AuthPermission);
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('permissions') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('permissions').get(field);
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
            this.fillModelArray<AuthPermission>(this, 'permissions', this['permissions'], AuthPermission);
        }
    }
}

