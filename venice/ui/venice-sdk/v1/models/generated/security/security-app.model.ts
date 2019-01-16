/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator } from './validators';
import { BaseModel, PropInfoItem } from './base-model';

import { ApiObjectMeta, IApiObjectMeta } from './api-object-meta.model';
import { SecurityAppSpec, ISecurityAppSpec } from './security-app-spec.model';
import { SecurityAppStatus, ISecurityAppStatus } from './security-app-status.model';

export interface ISecurityApp {
    'kind'?: string;
    'api-version'?: string;
    'meta'?: IApiObjectMeta;
    'spec'?: ISecurityAppSpec;
    'status'?: ISecurityAppStatus;
}


export class SecurityApp extends BaseModel implements ISecurityApp {
    'kind': string = null;
    'api-version': string = null;
    'meta': ApiObjectMeta = null;
    'spec': SecurityAppSpec = null;
    'status': SecurityAppStatus = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'kind': {
            type: 'string'
        },
        'api-version': {
            type: 'string'
        },
        'meta': {
            type: 'object'
        },
        'spec': {
            type: 'object'
        },
        'status': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return SecurityApp.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return SecurityApp.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (SecurityApp.propInfo[prop] != null &&
                        SecurityApp.propInfo[prop].default != null &&
                        SecurityApp.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['meta'] = new ApiObjectMeta();
        this['spec'] = new SecurityAppSpec();
        this['status'] = new SecurityAppStatus();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['kind'] != null) {
            this['kind'] = values['kind'];
        } else if (fillDefaults && SecurityApp.hasDefaultValue('kind')) {
            this['kind'] = SecurityApp.propInfo['kind'].default;
        }
        if (values && values['api-version'] != null) {
            this['api-version'] = values['api-version'];
        } else if (fillDefaults && SecurityApp.hasDefaultValue('api-version')) {
            this['api-version'] = SecurityApp.propInfo['api-version'].default;
        }
        if (values) {
            this['meta'].setValues(values['meta']);
        }
        if (values) {
            this['spec'].setValues(values['spec']);
        }
        if (values) {
            this['status'].setValues(values['status']);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'kind': new FormControl(this['kind']),
                'api-version': new FormControl(this['api-version']),
                'meta': this['meta'].$formGroup,
                'spec': this['spec'].$formGroup,
                'status': this['status'].$formGroup,
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

