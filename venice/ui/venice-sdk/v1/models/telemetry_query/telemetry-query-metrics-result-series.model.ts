/**
 * The generated models for the series are incorrect due to APIGW removing the values field from
 * interfaceSlices. This model is the correct representation.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { BaseModel, PropInfoItem } from '../generated/telemetry_query/base-model';


export interface ITelemetry_queryMetricsResultSeries {
    'name'?: string;
    'tags'?: object;
    'columns'?: Array<string>;
    'values'?: Array<Array<any>>;
}


export class Telemetry_queryMetricsResultSeries extends BaseModel implements ITelemetry_queryMetricsResultSeries {
    'name': string = null;
    'tags': object = null;
    'columns': Array<string> = null;
    'values': Array<Array<any>> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'name': {
            required: true,
            type: 'string'
        },
        'tags': {
            required: true,
            type: 'object'
        },
        'columns': {
            required: true,
            type: 'Array<string>'
        },
        'values': {
            required: true,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return Telemetry_queryMetricsResultSeries.propInfo[propName];
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (Telemetry_queryMetricsResultSeries.propInfo[prop] != null &&
            Telemetry_queryMetricsResultSeries.propInfo[prop].default != null &&
            Telemetry_queryMetricsResultSeries.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any) {
        super();
        this['columns'] = new Array<string>();
        this['values'] = new Array<Array<any>>();
        this.setValues(values);
    }

    /**
     * set the values. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['name'] != null) {
            this['name'] = values['name'];
        } else if (fillDefaults && Telemetry_queryMetricsResultSeries.hasDefaultValue('name')) {
            this['name'] = Telemetry_queryMetricsResultSeries.propInfo['name'].default;
        }
        if (values && values['tags'] != null) {
            this['tags'] = values['tags'];
        } else if (fillDefaults && Telemetry_queryMetricsResultSeries.hasDefaultValue('tags')) {
            this['tags'] = Telemetry_queryMetricsResultSeries.propInfo['tags'].default;
        }
        if (values) {
            this.fillModelArray<string>(this, 'columns', values['columns']);
        }
        if (values && values['values'] != null) {
            this['values'] = values['values'];
        }
        this.setFormGroupValuesToBeModelValues();
    }
    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'name': new FormControl(this['name']),
                'tags': new FormControl(this['tags']),
                'columns': new FormControl(this['columns']),
                'values': new FormControl([[]]),
            });
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
            this._formGroup.controls['values'].setValue(this['values']);
        }
    }
}




