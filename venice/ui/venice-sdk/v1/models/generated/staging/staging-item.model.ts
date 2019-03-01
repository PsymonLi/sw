/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { ApiAny, IApiAny } from './api-any.model';

export interface IStagingItem {
    'uri'?: string;
    'method'?: string;
    'object'?: IApiAny;
}


export class StagingItem extends BaseModel implements IStagingItem {
    'uri': string = null;
    'method': string = null;
    'object': ApiAny = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'uri': {
            required: false,
            type: 'string'
        },
        'method': {
            required: false,
            type: 'string'
        },
        'object': {
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return StagingItem.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return StagingItem.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (StagingItem.propInfo[prop] != null &&
                        StagingItem.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['object'] = new ApiAny();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['uri'] != null) {
            this['uri'] = values['uri'];
        } else if (fillDefaults && StagingItem.hasDefaultValue('uri')) {
            this['uri'] = StagingItem.propInfo['uri'].default;
        } else {
            this['uri'] = null
        }
        if (values && values['method'] != null) {
            this['method'] = values['method'];
        } else if (fillDefaults && StagingItem.hasDefaultValue('method')) {
            this['method'] = StagingItem.propInfo['method'].default;
        } else {
            this['method'] = null
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
                'uri': CustomFormControl(new FormControl(this['uri']), StagingItem.propInfo['uri']),
                'method': CustomFormControl(new FormControl(this['method']), StagingItem.propInfo['method']),
                'object': CustomFormGroup(this['object'].$formGroup, StagingItem.propInfo['object'].required),
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('object') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('object').get(field);
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
            this._formGroup.controls['uri'].setValue(this['uri']);
            this._formGroup.controls['method'].setValue(this['method']);
            this['object'].setFormGroupValuesToBeModelValues();
        }
    }
}

