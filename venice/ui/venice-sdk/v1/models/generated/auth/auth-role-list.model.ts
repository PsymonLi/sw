/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { ApiListMeta, IApiListMeta } from './api-list-meta.model';
import { AuthRole, IAuthRole } from './auth-role.model';

export interface IAuthRoleList {
    'kind'?: string;
    'api-version'?: string;
    'list-meta'?: IApiListMeta;
    'items'?: Array<IAuthRole>;
}


export class AuthRoleList extends BaseModel implements IAuthRoleList {
    'kind': string = null;
    'api-version': string = null;
    'list-meta': ApiListMeta = null;
    'items': Array<AuthRole> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'kind': {
            type: 'string'
        },
        'api-version': {
            type: 'string'
        },
        'list-meta': {
            type: 'object'
        },
        'items': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return AuthRoleList.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return AuthRoleList.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (AuthRoleList.propInfo[prop] != null &&
                        AuthRoleList.propInfo[prop].default != null &&
                        AuthRoleList.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['list-meta'] = new ApiListMeta();
        this['items'] = new Array<AuthRole>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['kind'] != null) {
            this['kind'] = values['kind'];
        } else if (fillDefaults && AuthRoleList.hasDefaultValue('kind')) {
            this['kind'] = AuthRoleList.propInfo['kind'].default;
        } else {
            this['kind'] = null
        }
        if (values && values['api-version'] != null) {
            this['api-version'] = values['api-version'];
        } else if (fillDefaults && AuthRoleList.hasDefaultValue('api-version')) {
            this['api-version'] = AuthRoleList.propInfo['api-version'].default;
        } else {
            this['api-version'] = null
        }
        if (values) {
            this['list-meta'].setValues(values['list-meta'], fillDefaults);
        } else {
            this['list-meta'].setValues(null, fillDefaults);
        }
        if (values) {
            this.fillModelArray<AuthRole>(this, 'items', values['items'], AuthRole);
        } else {
            this['items'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'kind': CustomFormControl(new FormControl(this['kind']), AuthRoleList.propInfo['kind'].description),
                'api-version': CustomFormControl(new FormControl(this['api-version']), AuthRoleList.propInfo['api-version'].description),
                'list-meta': this['list-meta'].$formGroup,
                'items': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<AuthRole>('items', this['items'], AuthRole);
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
            this['list-meta'].setFormGroupValuesToBeModelValues();
            this.fillModelArray<AuthRole>(this, 'items', this['items'], AuthRole);
        }
    }
}

