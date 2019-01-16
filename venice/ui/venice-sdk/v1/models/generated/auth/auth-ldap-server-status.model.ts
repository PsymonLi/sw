/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator } from './validators';
import { BaseModel, PropInfoItem } from './base-model';

import { AuthLdapServerStatus_result,  } from './enums';
import { AuthLdapServer, IAuthLdapServer } from './auth-ldap-server.model';

export interface IAuthLdapServerStatus {
    'result'?: AuthLdapServerStatus_result;
    'message'?: string;
    'server'?: IAuthLdapServer;
}


export class AuthLdapServerStatus extends BaseModel implements IAuthLdapServerStatus {
    'result': AuthLdapServerStatus_result = null;
    'message': string = null;
    'server': AuthLdapServer = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'result': {
            enum: AuthLdapServerStatus_result,
            default: 'Connect_Success',
            type: 'string'
        },
        'message': {
            type: 'string'
        },
        'server': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return AuthLdapServerStatus.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return AuthLdapServerStatus.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (AuthLdapServerStatus.propInfo[prop] != null &&
                        AuthLdapServerStatus.propInfo[prop].default != null &&
                        AuthLdapServerStatus.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['server'] = new AuthLdapServer();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['result'] != null) {
            this['result'] = values['result'];
        } else if (fillDefaults && AuthLdapServerStatus.hasDefaultValue('result')) {
            this['result'] = <AuthLdapServerStatus_result>  AuthLdapServerStatus.propInfo['result'].default;
        }
        if (values && values['message'] != null) {
            this['message'] = values['message'];
        } else if (fillDefaults && AuthLdapServerStatus.hasDefaultValue('message')) {
            this['message'] = AuthLdapServerStatus.propInfo['message'].default;
        }
        if (values) {
            this['server'].setValues(values['server']);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'result': new FormControl(this['result'], [required, enumValidator(AuthLdapServerStatus_result), ]),
                'message': new FormControl(this['message']),
                'server': this['server'].$formGroup,
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['result'].setValue(this['result']);
            this._formGroup.controls['message'].setValue(this['message']);
            this['server'].setFormGroupValuesToBeModelValues();
        }
    }
}

