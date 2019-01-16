/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator } from './validators';
import { BaseModel, PropInfoItem } from './base-model';


export interface IApiStatusResult {
    'Str'?: string;
}


export class ApiStatusResult extends BaseModel implements IApiStatusResult {
    'Str': string = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'Str': {
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return ApiStatusResult.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return ApiStatusResult.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (ApiStatusResult.propInfo[prop] != null &&
                        ApiStatusResult.propInfo[prop].default != null &&
                        ApiStatusResult.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['Str'] != null) {
            this['Str'] = values['Str'];
        } else if (fillDefaults && ApiStatusResult.hasDefaultValue('Str')) {
            this['Str'] = ApiStatusResult.propInfo['Str'].default;
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'Str': new FormControl(this['Str']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['Str'].setValue(this['Str']);
        }
    }
}

