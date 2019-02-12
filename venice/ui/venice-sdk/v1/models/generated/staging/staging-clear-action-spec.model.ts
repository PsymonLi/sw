/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { StagingItemId, IStagingItemId } from './staging-item-id.model';

export interface IStagingClearActionSpec {
    'items'?: Array<IStagingItemId>;
}


export class StagingClearActionSpec extends BaseModel implements IStagingClearActionSpec {
    'items': Array<StagingItemId> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'items': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return StagingClearActionSpec.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return StagingClearActionSpec.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (StagingClearActionSpec.propInfo[prop] != null &&
                        StagingClearActionSpec.propInfo[prop].default != null &&
                        StagingClearActionSpec.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['items'] = new Array<StagingItemId>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values) {
            this.fillModelArray<StagingItemId>(this, 'items', values['items'], StagingItemId);
        } else {
            this['items'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'items': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<StagingItemId>('items', this['items'], StagingItemId);
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this.fillModelArray<StagingItemId>(this, 'items', this['items'], StagingItemId);
        }
    }
}

