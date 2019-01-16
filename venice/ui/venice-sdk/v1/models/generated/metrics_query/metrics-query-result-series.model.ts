/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator } from './validators';
import { BaseModel, PropInfoItem } from './base-model';

import { ApiInterfaceSlice, IApiInterfaceSlice } from './api-interface-slice.model';

export interface IMetrics_queryResultSeries {
    'name'?: string;
    'tags'?: object;
    'columns'?: Array<string>;
    'values'?: Array<IApiInterfaceSlice>;
}


export class Metrics_queryResultSeries extends BaseModel implements IMetrics_queryResultSeries {
    'name': string = null;
    'tags': object = null;
    'columns': Array<string> = null;
    'values': Array<ApiInterfaceSlice> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'name': {
            type: 'string'
        },
        'tags': {
            type: 'object'
        },
        'columns': {
            type: 'Array<string>'
        },
        'values': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return Metrics_queryResultSeries.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return Metrics_queryResultSeries.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (Metrics_queryResultSeries.propInfo[prop] != null &&
                        Metrics_queryResultSeries.propInfo[prop].default != null &&
                        Metrics_queryResultSeries.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['columns'] = new Array<string>();
        this['values'] = new Array<ApiInterfaceSlice>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['name'] != null) {
            this['name'] = values['name'];
        } else if (fillDefaults && Metrics_queryResultSeries.hasDefaultValue('name')) {
            this['name'] = Metrics_queryResultSeries.propInfo['name'].default;
        }
        if (values && values['tags'] != null) {
            this['tags'] = values['tags'];
        } else if (fillDefaults && Metrics_queryResultSeries.hasDefaultValue('tags')) {
            this['tags'] = Metrics_queryResultSeries.propInfo['tags'].default;
        }
        if (values && values['columns'] != null) {
            this['columns'] = values['columns'];
        } else if (fillDefaults && Metrics_queryResultSeries.hasDefaultValue('columns')) {
            this['columns'] = [ Metrics_queryResultSeries.propInfo['columns'].default];
        }
        if (values) {
            this.fillModelArray<ApiInterfaceSlice>(this, 'values', values['values'], ApiInterfaceSlice);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'name': new FormControl(this['name']),
                'tags': new FormControl(this['tags']),
                'columns': new FormControl(this['columns']),
                'values': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<ApiInterfaceSlice>('values', this['values'], ApiInterfaceSlice);
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['name'].setValue(this['name']);
            this._formGroup.controls['tags'].setValue(this['tags']);
            this._formGroup.controls['columns'].setValue(this['columns']);
            this.fillModelArray<ApiInterfaceSlice>(this, 'values', this['values'], ApiInterfaceSlice);
        }
    }
}

