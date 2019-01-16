/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator } from './validators';
import { BaseModel, PropInfoItem } from './base-model';

import { AuthAutoMsgUserWatchHelperWatchEvent, IAuthAutoMsgUserWatchHelperWatchEvent } from './auth-auto-msg-user-watch-helper-watch-event.model';

export interface IAuthAutoMsgUserWatchHelper {
    'events'?: Array<IAuthAutoMsgUserWatchHelperWatchEvent>;
}


export class AuthAutoMsgUserWatchHelper extends BaseModel implements IAuthAutoMsgUserWatchHelper {
    'events': Array<AuthAutoMsgUserWatchHelperWatchEvent> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'events': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return AuthAutoMsgUserWatchHelper.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return AuthAutoMsgUserWatchHelper.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (AuthAutoMsgUserWatchHelper.propInfo[prop] != null &&
                        AuthAutoMsgUserWatchHelper.propInfo[prop].default != null &&
                        AuthAutoMsgUserWatchHelper.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['events'] = new Array<AuthAutoMsgUserWatchHelperWatchEvent>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values) {
            this.fillModelArray<AuthAutoMsgUserWatchHelperWatchEvent>(this, 'events', values['events'], AuthAutoMsgUserWatchHelperWatchEvent);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'events': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<AuthAutoMsgUserWatchHelperWatchEvent>('events', this['events'], AuthAutoMsgUserWatchHelperWatchEvent);
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this.fillModelArray<AuthAutoMsgUserWatchHelperWatchEvent>(this, 'events', this['events'], AuthAutoMsgUserWatchHelperWatchEvent);
        }
    }
}

