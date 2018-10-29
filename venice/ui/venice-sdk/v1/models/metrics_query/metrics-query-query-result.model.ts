/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, enumValidator } from '../generated/metrics_query/validators';
import { BaseModel, PropInfoItem } from '../generated/metrics_query/base-model';

import { Metrics_queryResultSeries, IMetrics_queryResultSeries } from './metrics-query-result-series.model';

export interface IMetrics_queryQueryResult {
    'statement_id'?: number;
    'series'?: Array<IMetrics_queryResultSeries>;
}


export class Metrics_queryQueryResult extends BaseModel implements IMetrics_queryQueryResult {
    'statement_id': number = null;
    'series': Array<Metrics_queryResultSeries> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'statement_id': {
            type: 'number'
        },
        'series': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return Metrics_queryQueryResult.propInfo[propName];
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (Metrics_queryQueryResult.propInfo[prop] != null &&
            Metrics_queryQueryResult.propInfo[prop].default != null &&
            Metrics_queryQueryResult.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any) {
        super();
        this['series'] = new Array<Metrics_queryResultSeries>();
        this.setValues(values);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['statement_id'] != null) {
            this['statement_id'] = values['statement_id'];
        } else if (fillDefaults && Metrics_queryQueryResult.hasDefaultValue('statement_id')) {
            this['statement_id'] = Metrics_queryQueryResult.propInfo['statement_id'].default;
        }
        if (values) {
            this.fillModelArray<Metrics_queryResultSeries>(this, 'series', values['series'], Metrics_queryResultSeries);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'statement_id': new FormControl(this['statement_id']),
                'series': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<Metrics_queryResultSeries>('series', this['series'], Metrics_queryResultSeries);
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['statement_id'].setValue(this['statement_id']);
            this.fillModelArray<Metrics_queryResultSeries>(this, 'series', this['series'], Metrics_queryResultSeries);
        }
    }
}

