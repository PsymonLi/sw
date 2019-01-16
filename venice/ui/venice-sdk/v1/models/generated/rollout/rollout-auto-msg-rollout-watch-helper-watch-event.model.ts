/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator } from './validators';
import { BaseModel, PropInfoItem } from './base-model';

import { RolloutRollout, IRolloutRollout } from './rollout-rollout.model';

export interface IRolloutAutoMsgRolloutWatchHelperWatchEvent {
    'type'?: string;
    'object'?: IRolloutRollout;
}


export class RolloutAutoMsgRolloutWatchHelperWatchEvent extends BaseModel implements IRolloutAutoMsgRolloutWatchHelperWatchEvent {
    'type': string = null;
    'object': RolloutRollout = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'type': {
            type: 'string'
        },
        'object': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return RolloutAutoMsgRolloutWatchHelperWatchEvent.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return RolloutAutoMsgRolloutWatchHelperWatchEvent.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (RolloutAutoMsgRolloutWatchHelperWatchEvent.propInfo[prop] != null &&
                        RolloutAutoMsgRolloutWatchHelperWatchEvent.propInfo[prop].default != null &&
                        RolloutAutoMsgRolloutWatchHelperWatchEvent.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['object'] = new RolloutRollout();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['type'] != null) {
            this['type'] = values['type'];
        } else if (fillDefaults && RolloutAutoMsgRolloutWatchHelperWatchEvent.hasDefaultValue('type')) {
            this['type'] = RolloutAutoMsgRolloutWatchHelperWatchEvent.propInfo['type'].default;
        }
        if (values) {
            this['object'].setValues(values['object']);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'type': new FormControl(this['type']),
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

