/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { MonitoringThresholds_operator,  } from './enums';
import { MonitoringThreshold, IMonitoringThreshold } from './monitoring-threshold.model';

export interface IMonitoringThresholds {
    'operator': MonitoringThresholds_operator;
    'values'?: Array<IMonitoringThreshold>;
    '_ui'?: any;
}


export class MonitoringThresholds extends BaseModel implements IMonitoringThresholds {
    /** Field for holding arbitrary ui state */
    '_ui': any = {};
    /** Operator to be applied when comparing metric values against the threshold values. */
    'operator': MonitoringThresholds_operator = null;
    /** List of threshold values to be acted against. Key should be one of the alert severity. */
    'values': Array<MonitoringThreshold> = null;
    public static propInfo: { [prop in keyof IMonitoringThresholds]: PropInfoItem } = {
        'operator': {
            enum: MonitoringThresholds_operator,
            default: 'less_or_equal_than',
            description:  `Operator to be applied when comparing metric values against the threshold values.`,
            required: true,
            type: 'string'
        },
        'values': {
            description:  `List of threshold values to be acted against. Key should be one of the alert severity.`,
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return MonitoringThresholds.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return MonitoringThresholds.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (MonitoringThresholds.propInfo[prop] != null &&
                        MonitoringThresholds.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['values'] = new Array<MonitoringThreshold>();
        this._inputValue = values;
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['_ui']) {
            this['_ui'] = values['_ui']
        }
        if (values && values['operator'] != null) {
            this['operator'] = values['operator'];
        } else if (fillDefaults && MonitoringThresholds.hasDefaultValue('operator')) {
            this['operator'] = <MonitoringThresholds_operator>  MonitoringThresholds.propInfo['operator'].default;
        } else {
            this['operator'] = null
        }
        if (values) {
            this.fillModelArray<MonitoringThreshold>(this, 'values', values['values'], MonitoringThreshold);
        } else {
            this['values'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'operator': CustomFormControl(new FormControl(this['operator'], [required, enumValidator(MonitoringThresholds_operator), ]), MonitoringThresholds.propInfo['operator']),
                'values': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<MonitoringThreshold>('values', this['values'], MonitoringThreshold);
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('values') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('values').get(field);
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
            this._formGroup.controls['operator'].setValue(this['operator']);
            this.fillModelArray<MonitoringThreshold>(this, 'values', this['values'], MonitoringThreshold);
        }
    }
}
