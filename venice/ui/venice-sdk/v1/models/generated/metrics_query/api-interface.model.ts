/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator } from './validators';
import { BaseModel, PropInfoItem } from './base-model';

import { ApiInterfaceSlice, IApiInterfaceSlice } from './api-interface-slice.model';

export interface IApiInterface {
    'Str'?: string;
    'Int64'?: string;
    'Bool'?: boolean;
    'Float'?: number;
    'Interfaces'?: IApiInterfaceSlice;
}


export class ApiInterface extends BaseModel implements IApiInterface {
    'Str': string = null;
    'Int64': string = null;
    'Bool': boolean = null;
    'Float': number = null;
    'Interfaces': ApiInterfaceSlice = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'Str': {
            type: 'string'
        },
        'Int64': {
            type: 'string'
        },
        'Bool': {
            type: 'boolean'
        },
        'Float': {
            type: 'number'
        },
        'Interfaces': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return ApiInterface.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return ApiInterface.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (ApiInterface.propInfo[prop] != null &&
                        ApiInterface.propInfo[prop].default != null &&
                        ApiInterface.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['Interfaces'] = new ApiInterfaceSlice();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['Str'] != null) {
            this['Str'] = values['Str'];
        } else if (fillDefaults && ApiInterface.hasDefaultValue('Str')) {
            this['Str'] = ApiInterface.propInfo['Str'].default;
        }
        if (values && values['Int64'] != null) {
            this['Int64'] = values['Int64'];
        } else if (fillDefaults && ApiInterface.hasDefaultValue('Int64')) {
            this['Int64'] = ApiInterface.propInfo['Int64'].default;
        }
        if (values && values['Bool'] != null) {
            this['Bool'] = values['Bool'];
        } else if (fillDefaults && ApiInterface.hasDefaultValue('Bool')) {
            this['Bool'] = ApiInterface.propInfo['Bool'].default;
        }
        if (values && values['Float'] != null) {
            this['Float'] = values['Float'];
        } else if (fillDefaults && ApiInterface.hasDefaultValue('Float')) {
            this['Float'] = ApiInterface.propInfo['Float'].default;
        }
        if (values) {
            this['Interfaces'].setValues(values['Interfaces']);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'Str': new FormControl(this['Str']),
                'Int64': new FormControl(this['Int64']),
                'Bool': new FormControl(this['Bool']),
                'Float': new FormControl(this['Float']),
                'Interfaces': this['Interfaces'].$formGroup,
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
            this._formGroup.controls['Int64'].setValue(this['Int64']);
            this._formGroup.controls['Bool'].setValue(this['Bool']);
            this._formGroup.controls['Float'].setValue(this['Float']);
            this['Interfaces'].setFormGroupValuesToBeModelValues();
        }
    }
}

