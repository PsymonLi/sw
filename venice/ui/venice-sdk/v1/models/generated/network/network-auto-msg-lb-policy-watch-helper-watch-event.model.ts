/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { NetworkLbPolicy, INetworkLbPolicy } from './network-lb-policy.model';

export interface INetworkAutoMsgLbPolicyWatchHelperWatchEvent {
    'type'?: string;
    'object'?: INetworkLbPolicy;
}


export class NetworkAutoMsgLbPolicyWatchHelperWatchEvent extends BaseModel implements INetworkAutoMsgLbPolicyWatchHelperWatchEvent {
    'type': string = null;
    'object': NetworkLbPolicy = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'type': {
            type: 'string'
        },
        'object': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return NetworkAutoMsgLbPolicyWatchHelperWatchEvent.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return NetworkAutoMsgLbPolicyWatchHelperWatchEvent.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (NetworkAutoMsgLbPolicyWatchHelperWatchEvent.propInfo[prop] != null &&
                        NetworkAutoMsgLbPolicyWatchHelperWatchEvent.propInfo[prop].default != null &&
                        NetworkAutoMsgLbPolicyWatchHelperWatchEvent.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['object'] = new NetworkLbPolicy();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['type'] != null) {
            this['type'] = values['type'];
        } else if (fillDefaults && NetworkAutoMsgLbPolicyWatchHelperWatchEvent.hasDefaultValue('type')) {
            this['type'] = NetworkAutoMsgLbPolicyWatchHelperWatchEvent.propInfo['type'].default;
        } else {
            this['type'] = null
        }
        if (values) {
            this['object'].setValues(values['object'], fillDefaults);
        } else {
            this['object'].setValues(null, fillDefaults);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'type': CustomFormControl(new FormControl(this['type']), NetworkAutoMsgLbPolicyWatchHelperWatchEvent.propInfo['type'].description),
                'object': this['object'].$formGroup,
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['type'].setValue(this['type']);
            this['object'].setFormGroupValuesToBeModelValues();
        }
    }
}

