/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { AuthUserSpec_type,  AuthUserSpec_type_uihint  } from './enums';

export interface IAuthUserSpec {
    'fullname'?: string;
    'email'?: string;
    'password'?: string;
    'type': AuthUserSpec_type;
}


export class AuthUserSpec extends BaseModel implements IAuthUserSpec {
    'fullname': string = null;
    'email': string = null;
    'password': string = null;
    'type': AuthUserSpec_type = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'fullname': {
            required: false,
            type: 'string'
        },
        'email': {
            required: false,
            type: 'string'
        },
        'password': {
            required: false,
            type: 'string'
        },
        'type': {
            enum: AuthUserSpec_type_uihint,
            default: 'Local',
            required: true,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return AuthUserSpec.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return AuthUserSpec.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (AuthUserSpec.propInfo[prop] != null &&
                        AuthUserSpec.propInfo[prop].default != null);
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
        if (values && values['fullname'] != null) {
            this['fullname'] = values['fullname'];
        } else if (fillDefaults && AuthUserSpec.hasDefaultValue('fullname')) {
            this['fullname'] = AuthUserSpec.propInfo['fullname'].default;
        } else {
            this['fullname'] = null
        }
        if (values && values['email'] != null) {
            this['email'] = values['email'];
        } else if (fillDefaults && AuthUserSpec.hasDefaultValue('email')) {
            this['email'] = AuthUserSpec.propInfo['email'].default;
        } else {
            this['email'] = null
        }
        if (values && values['password'] != null) {
            this['password'] = values['password'];
        } else if (fillDefaults && AuthUserSpec.hasDefaultValue('password')) {
            this['password'] = AuthUserSpec.propInfo['password'].default;
        } else {
            this['password'] = null
        }
        if (values && values['type'] != null) {
            this['type'] = values['type'];
        } else if (fillDefaults && AuthUserSpec.hasDefaultValue('type')) {
            this['type'] = <AuthUserSpec_type>  AuthUserSpec.propInfo['type'].default;
        } else {
            this['type'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'fullname': CustomFormControl(new FormControl(this['fullname']), AuthUserSpec.propInfo['fullname']),
                'email': CustomFormControl(new FormControl(this['email']), AuthUserSpec.propInfo['email']),
                'password': CustomFormControl(new FormControl(this['password']), AuthUserSpec.propInfo['password']),
                'type': CustomFormControl(new FormControl(this['type'], [required, enumValidator(AuthUserSpec_type), ]), AuthUserSpec.propInfo['type']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['fullname'].setValue(this['fullname']);
            this._formGroup.controls['email'].setValue(this['email']);
            this._formGroup.controls['password'].setValue(this['password']);
            this._formGroup.controls['type'].setValue(this['type']);
        }
    }
}

