/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { AuthRadiusServerStatus_result,  } from './enums';
import { AuthRadiusServer, IAuthRadiusServer } from './auth-radius-server.model';

export interface IAuthRadiusServerStatus {
    'result': AuthRadiusServerStatus_result;
    'message'?: string;
    'server'?: IAuthRadiusServer;
}


export class AuthRadiusServerStatus extends BaseModel implements IAuthRadiusServerStatus {
    'result': AuthRadiusServerStatus_result = null;
    'message': string = null;
    'server': AuthRadiusServer = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'result': {
            enum: AuthRadiusServerStatus_result,
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
        return AuthRadiusServerStatus.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return AuthRadiusServerStatus.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (AuthRadiusServerStatus.propInfo[prop] != null &&
                        AuthRadiusServerStatus.propInfo[prop].default != null &&
                        AuthRadiusServerStatus.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['server'] = new AuthRadiusServer();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['result'] != null) {
            this['result'] = values['result'];
        } else if (fillDefaults && AuthRadiusServerStatus.hasDefaultValue('result')) {
            this['result'] = <AuthRadiusServerStatus_result>  AuthRadiusServerStatus.propInfo['result'].default;
        } else {
            this['result'] = null
        }
        if (values && values['message'] != null) {
            this['message'] = values['message'];
        } else if (fillDefaults && AuthRadiusServerStatus.hasDefaultValue('message')) {
            this['message'] = AuthRadiusServerStatus.propInfo['message'].default;
        } else {
            this['message'] = null
        }
        if (values) {
            this['server'].setValues(values['server'], fillDefaults);
        } else {
            this['server'].setValues(null, fillDefaults);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'result': CustomFormControl(new FormControl(this['result'], [required, enumValidator(AuthRadiusServerStatus_result), ]), AuthRadiusServerStatus.propInfo['result'].description),
                'message': CustomFormControl(new FormControl(this['message']), AuthRadiusServerStatus.propInfo['message'].description),
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

