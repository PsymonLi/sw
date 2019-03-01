/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { AuthAutoMsgAuthenticationPolicyWatchHelperWatchEvent, IAuthAutoMsgAuthenticationPolicyWatchHelperWatchEvent } from './auth-auto-msg-authentication-policy-watch-helper-watch-event.model';

export interface IAuthAutoMsgAuthenticationPolicyWatchHelper {
    'events'?: Array<IAuthAutoMsgAuthenticationPolicyWatchHelperWatchEvent>;
}


export class AuthAutoMsgAuthenticationPolicyWatchHelper extends BaseModel implements IAuthAutoMsgAuthenticationPolicyWatchHelper {
    'events': Array<AuthAutoMsgAuthenticationPolicyWatchHelperWatchEvent> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'events': {
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return AuthAutoMsgAuthenticationPolicyWatchHelper.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return AuthAutoMsgAuthenticationPolicyWatchHelper.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (AuthAutoMsgAuthenticationPolicyWatchHelper.propInfo[prop] != null &&
                        AuthAutoMsgAuthenticationPolicyWatchHelper.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['events'] = new Array<AuthAutoMsgAuthenticationPolicyWatchHelperWatchEvent>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values) {
            this.fillModelArray<AuthAutoMsgAuthenticationPolicyWatchHelperWatchEvent>(this, 'events', values['events'], AuthAutoMsgAuthenticationPolicyWatchHelperWatchEvent);
        } else {
            this['events'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'events': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<AuthAutoMsgAuthenticationPolicyWatchHelperWatchEvent>('events', this['events'], AuthAutoMsgAuthenticationPolicyWatchHelperWatchEvent);
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('events') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('events').get(field);
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
            this.fillModelArray<AuthAutoMsgAuthenticationPolicyWatchHelperWatchEvent>(this, 'events', this['events'], AuthAutoMsgAuthenticationPolicyWatchHelperWatchEvent);
        }
    }
}

