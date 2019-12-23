/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { AuthAuthenticators_authenticator_order,  AuthAuthenticators_authenticator_order_uihint  } from './enums';
import { AuthLdap, IAuthLdap } from './auth-ldap.model';
import { AuthLocal, IAuthLocal } from './auth-local.model';
import { AuthRadius, IAuthRadius } from './auth-radius.model';

export interface IAuthAuthenticators {
    'authenticator-order': Array<AuthAuthenticators_authenticator_order>;
    'ldap'?: IAuthLdap;
    'local'?: IAuthLocal;
    'radius'?: IAuthRadius;
}


export class AuthAuthenticators extends BaseModel implements IAuthAuthenticators {
    /** Order in which authenticators are applied. If an authenticator returns success, others are skipped */
    'authenticator-order': Array<AuthAuthenticators_authenticator_order> = null;
    'ldap': AuthLdap = null;
    'local': AuthLocal = null;
    'radius': AuthRadius = null;
    public static propInfo: { [prop in keyof IAuthAuthenticators]: PropInfoItem } = {
        'authenticator-order': {
            enum: AuthAuthenticators_authenticator_order_uihint,
            default: 'local',
            description:  `Order in which authenticators are applied. If an authenticator returns success, others are skipped`,
            required: true,
            type: 'Array<string>'
        },
        'ldap': {
            required: false,
            type: 'object'
        },
        'local': {
            required: false,
            type: 'object'
        },
        'radius': {
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return AuthAuthenticators.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return AuthAuthenticators.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (AuthAuthenticators.propInfo[prop] != null &&
                        AuthAuthenticators.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['authenticator-order'] = new Array<AuthAuthenticators_authenticator_order>();
        this['ldap'] = new AuthLdap();
        this['local'] = new AuthLocal();
        this['radius'] = new AuthRadius();
        this._inputValue = values;
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['authenticator-order'] != null) {
            this['authenticator-order'] = values['authenticator-order'];
        } else if (fillDefaults && AuthAuthenticators.hasDefaultValue('authenticator-order')) {
            this['authenticator-order'] = [ AuthAuthenticators.propInfo['authenticator-order'].default];
        } else {
            this['authenticator-order'] = [];
        }
        if (values) {
            this['ldap'].setValues(values['ldap'], fillDefaults);
        } else {
            this['ldap'].setValues(null, fillDefaults);
        }
        if (values) {
            this['local'].setValues(values['local'], fillDefaults);
        } else {
            this['local'].setValues(null, fillDefaults);
        }
        if (values) {
            this['radius'].setValues(values['radius'], fillDefaults);
        } else {
            this['radius'].setValues(null, fillDefaults);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'authenticator-order': CustomFormControl(new FormControl(this['authenticator-order']), AuthAuthenticators.propInfo['authenticator-order']),
                'ldap': CustomFormGroup(this['ldap'].$formGroup, AuthAuthenticators.propInfo['ldap'].required),
                'local': CustomFormGroup(this['local'].$formGroup, AuthAuthenticators.propInfo['local'].required),
                'radius': CustomFormGroup(this['radius'].$formGroup, AuthAuthenticators.propInfo['radius'].required),
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('ldap') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('ldap').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('local') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('local').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('radius') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('radius').get(field);
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
            this._formGroup.controls['authenticator-order'].setValue(this['authenticator-order']);
            this['ldap'].setFormGroupValuesToBeModelValues();
            this['local'].setFormGroupValuesToBeModelValues();
            this['radius'].setFormGroupValuesToBeModelValues();
        }
    }
}

