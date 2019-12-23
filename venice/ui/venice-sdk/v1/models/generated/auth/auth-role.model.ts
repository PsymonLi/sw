/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { ApiObjectMeta, IApiObjectMeta } from './api-object-meta.model';
import { AuthRoleSpec, IAuthRoleSpec } from './auth-role-spec.model';
import { AuthRoleStatus, IAuthRoleStatus } from './auth-role-status.model';

export interface IAuthRole {
    'kind'?: string;
    'api-version'?: string;
    'meta'?: IApiObjectMeta;
    'spec'?: IAuthRoleSpec;
    'status'?: IAuthRoleStatus;
}


export class AuthRole extends BaseModel implements IAuthRole {
    'kind': string = null;
    'api-version': string = null;
    'meta': ApiObjectMeta = null;
    /** Spec contains the configuration of the role. */
    'spec': AuthRoleSpec = null;
    /** Status contains the current state of the role. */
    'status': AuthRoleStatus = null;
    public static propInfo: { [prop in keyof IAuthRole]: PropInfoItem } = {
        'kind': {
            required: false,
            type: 'string'
        },
        'api-version': {
            required: false,
            type: 'string'
        },
        'meta': {
            required: false,
            type: 'object'
        },
        'spec': {
            description:  `Spec contains the configuration of the role.`,
            required: false,
            type: 'object'
        },
        'status': {
            description:  `Status contains the current state of the role.`,
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return AuthRole.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return AuthRole.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (AuthRole.propInfo[prop] != null &&
                        AuthRole.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['meta'] = new ApiObjectMeta();
        this['spec'] = new AuthRoleSpec();
        this['status'] = new AuthRoleStatus();
        this._inputValue = values;
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['kind'] != null) {
            this['kind'] = values['kind'];
        } else if (fillDefaults && AuthRole.hasDefaultValue('kind')) {
            this['kind'] = AuthRole.propInfo['kind'].default;
        } else {
            this['kind'] = null
        }
        if (values && values['api-version'] != null) {
            this['api-version'] = values['api-version'];
        } else if (fillDefaults && AuthRole.hasDefaultValue('api-version')) {
            this['api-version'] = AuthRole.propInfo['api-version'].default;
        } else {
            this['api-version'] = null
        }
        if (values) {
            this['meta'].setValues(values['meta'], fillDefaults);
        } else {
            this['meta'].setValues(null, fillDefaults);
        }
        if (values) {
            this['spec'].setValues(values['spec'], fillDefaults);
        } else {
            this['spec'].setValues(null, fillDefaults);
        }
        if (values) {
            this['status'].setValues(values['status'], fillDefaults);
        } else {
            this['status'].setValues(null, fillDefaults);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'kind': CustomFormControl(new FormControl(this['kind']), AuthRole.propInfo['kind']),
                'api-version': CustomFormControl(new FormControl(this['api-version']), AuthRole.propInfo['api-version']),
                'meta': CustomFormGroup(this['meta'].$formGroup, AuthRole.propInfo['meta'].required),
                'spec': CustomFormGroup(this['spec'].$formGroup, AuthRole.propInfo['spec'].required),
                'status': CustomFormGroup(this['status'].$formGroup, AuthRole.propInfo['status'].required),
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('meta') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('meta').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('spec') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('spec').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('status') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('status').get(field);
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
            this._formGroup.controls['kind'].setValue(this['kind']);
            this._formGroup.controls['api-version'].setValue(this['api-version']);
            this['meta'].setFormGroupValuesToBeModelValues();
            this['spec'].setFormGroupValuesToBeModelValues();
            this['status'].setFormGroupValuesToBeModelValues();
        }
    }
}

