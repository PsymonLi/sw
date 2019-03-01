/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';


export interface IMonitoringAlertPolicyStatus {
    'total-hits'?: number;
    'open-alerts'?: number;
    'acknowledged-alerts'?: number;
}


export class MonitoringAlertPolicyStatus extends BaseModel implements IMonitoringAlertPolicyStatus {
    'total-hits': number = null;
    'open-alerts': number = null;
    'acknowledged-alerts': number = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'total-hits': {
            required: false,
            type: 'number'
        },
        'open-alerts': {
            required: false,
            type: 'number'
        },
        'acknowledged-alerts': {
            required: false,
            type: 'number'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return MonitoringAlertPolicyStatus.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return MonitoringAlertPolicyStatus.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (MonitoringAlertPolicyStatus.propInfo[prop] != null &&
                        MonitoringAlertPolicyStatus.propInfo[prop].default != null);
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
        if (values && values['total-hits'] != null) {
            this['total-hits'] = values['total-hits'];
        } else if (fillDefaults && MonitoringAlertPolicyStatus.hasDefaultValue('total-hits')) {
            this['total-hits'] = MonitoringAlertPolicyStatus.propInfo['total-hits'].default;
        } else {
            this['total-hits'] = null
        }
        if (values && values['open-alerts'] != null) {
            this['open-alerts'] = values['open-alerts'];
        } else if (fillDefaults && MonitoringAlertPolicyStatus.hasDefaultValue('open-alerts')) {
            this['open-alerts'] = MonitoringAlertPolicyStatus.propInfo['open-alerts'].default;
        } else {
            this['open-alerts'] = null
        }
        if (values && values['acknowledged-alerts'] != null) {
            this['acknowledged-alerts'] = values['acknowledged-alerts'];
        } else if (fillDefaults && MonitoringAlertPolicyStatus.hasDefaultValue('acknowledged-alerts')) {
            this['acknowledged-alerts'] = MonitoringAlertPolicyStatus.propInfo['acknowledged-alerts'].default;
        } else {
            this['acknowledged-alerts'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'total-hits': CustomFormControl(new FormControl(this['total-hits']), MonitoringAlertPolicyStatus.propInfo['total-hits']),
                'open-alerts': CustomFormControl(new FormControl(this['open-alerts']), MonitoringAlertPolicyStatus.propInfo['open-alerts']),
                'acknowledged-alerts': CustomFormControl(new FormControl(this['acknowledged-alerts']), MonitoringAlertPolicyStatus.propInfo['acknowledged-alerts']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['total-hits'].setValue(this['total-hits']);
            this._formGroup.controls['open-alerts'].setValue(this['open-alerts']);
            this._formGroup.controls['acknowledged-alerts'].setValue(this['acknowledged-alerts']);
        }
    }
}

