/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { ObjstoreAutoMsgObjectWatchHelperWatchEvent, IObjstoreAutoMsgObjectWatchHelperWatchEvent } from './objstore-auto-msg-object-watch-helper-watch-event.model';

export interface IObjstoreAutoMsgObjectWatchHelper {
    'events'?: Array<IObjstoreAutoMsgObjectWatchHelperWatchEvent>;
}


export class ObjstoreAutoMsgObjectWatchHelper extends BaseModel implements IObjstoreAutoMsgObjectWatchHelper {
    'events': Array<ObjstoreAutoMsgObjectWatchHelperWatchEvent> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'events': {
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return ObjstoreAutoMsgObjectWatchHelper.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return ObjstoreAutoMsgObjectWatchHelper.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (ObjstoreAutoMsgObjectWatchHelper.propInfo[prop] != null &&
                        ObjstoreAutoMsgObjectWatchHelper.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['events'] = new Array<ObjstoreAutoMsgObjectWatchHelperWatchEvent>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values) {
            this.fillModelArray<ObjstoreAutoMsgObjectWatchHelperWatchEvent>(this, 'events', values['events'], ObjstoreAutoMsgObjectWatchHelperWatchEvent);
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
            this.fillFormArray<ObjstoreAutoMsgObjectWatchHelperWatchEvent>('events', this['events'], ObjstoreAutoMsgObjectWatchHelperWatchEvent);
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
            this.fillModelArray<ObjstoreAutoMsgObjectWatchHelperWatchEvent>(this, 'events', this['events'], ObjstoreAutoMsgObjectWatchHelperWatchEvent);
        }
    }
}

