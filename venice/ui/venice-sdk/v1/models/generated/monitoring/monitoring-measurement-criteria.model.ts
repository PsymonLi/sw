/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { MonitoringMeasurementCriteria_function,  } from './enums';

export interface IMonitoringMeasurementCriteria {
    'window'?: string;
    'function': MonitoringMeasurementCriteria_function;
    '_ui'?: any;
}


export class MonitoringMeasurementCriteria extends BaseModel implements IMonitoringMeasurementCriteria {
    /** Field for holding arbitrary ui state */
    '_ui': any = {};
    /** The length of time the metric will be monitored/observed before running the values against thresholds for alert creation/resolution. ui-hint: Allowed values - 5m, 10m, 30m, 1h. */
    'window': string = null;
    /** Aggregate function to be applied on the metric values that were monitored over a window/interval. */
    'function': MonitoringMeasurementCriteria_function = null;
    public static propInfo: { [prop in keyof IMonitoringMeasurementCriteria]: PropInfoItem } = {
        'window': {
            description:  `The length of time the metric will be monitored/observed before running the values against thresholds for alert creation/resolution. ui-hint: Allowed values - 5m, 10m, 30m, 1h.`,
            required: false,
            type: 'string'
        },
        'function': {
            enum: MonitoringMeasurementCriteria_function,
            default: 'min',
            description:  `Aggregate function to be applied on the metric values that were monitored over a window/interval.`,
            required: true,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return MonitoringMeasurementCriteria.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return MonitoringMeasurementCriteria.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (MonitoringMeasurementCriteria.propInfo[prop] != null &&
                        MonitoringMeasurementCriteria.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
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
        if (values && values['window'] != null) {
            this['window'] = values['window'];
        } else if (fillDefaults && MonitoringMeasurementCriteria.hasDefaultValue('window')) {
            this['window'] = MonitoringMeasurementCriteria.propInfo['window'].default;
        } else {
            this['window'] = null
        }
        if (values && values['function'] != null) {
            this['function'] = values['function'];
        } else if (fillDefaults && MonitoringMeasurementCriteria.hasDefaultValue('function')) {
            this['function'] = <MonitoringMeasurementCriteria_function>  MonitoringMeasurementCriteria.propInfo['function'].default;
        } else {
            this['function'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'window': CustomFormControl(new FormControl(this['window']), MonitoringMeasurementCriteria.propInfo['window']),
                'function': CustomFormControl(new FormControl(this['function'], [required, enumValidator(MonitoringMeasurementCriteria_function), ]), MonitoringMeasurementCriteria.propInfo['function']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['window'].setValue(this['window']);
            this._formGroup.controls['function'].setValue(this['function']);
        }
    }
}
