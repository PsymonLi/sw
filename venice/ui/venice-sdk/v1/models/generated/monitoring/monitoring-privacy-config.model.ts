/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { MonitoringPrivacyConfig_algo,  } from './enums';

export interface IMonitoringPrivacyConfig {
    'algo': MonitoringPrivacyConfig_algo;
    'password'?: string;
}


export class MonitoringPrivacyConfig extends BaseModel implements IMonitoringPrivacyConfig {
    'algo': MonitoringPrivacyConfig_algo = null;
    /** Password contains the privacy password. */
    'password': string = null;
    public static propInfo: { [prop in keyof IMonitoringPrivacyConfig]: PropInfoItem } = {
        'algo': {
            enum: MonitoringPrivacyConfig_algo,
            default: 'des56',
            required: true,
            type: 'string'
        },
        'password': {
            description:  `Password contains the privacy password.`,
            required: false,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return MonitoringPrivacyConfig.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return MonitoringPrivacyConfig.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (MonitoringPrivacyConfig.propInfo[prop] != null &&
                        MonitoringPrivacyConfig.propInfo[prop].default != null);
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
        if (values && values['algo'] != null) {
            this['algo'] = values['algo'];
        } else if (fillDefaults && MonitoringPrivacyConfig.hasDefaultValue('algo')) {
            this['algo'] = <MonitoringPrivacyConfig_algo>  MonitoringPrivacyConfig.propInfo['algo'].default;
        } else {
            this['algo'] = null
        }
        if (values && values['password'] != null) {
            this['password'] = values['password'];
        } else if (fillDefaults && MonitoringPrivacyConfig.hasDefaultValue('password')) {
            this['password'] = MonitoringPrivacyConfig.propInfo['password'].default;
        } else {
            this['password'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'algo': CustomFormControl(new FormControl(this['algo'], [required, enumValidator(MonitoringPrivacyConfig_algo), ]), MonitoringPrivacyConfig.propInfo['algo']),
                'password': CustomFormControl(new FormControl(this['password']), MonitoringPrivacyConfig.propInfo['password']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['algo'].setValue(this['algo']);
            this._formGroup.controls['password'].setValue(this['password']);
        }
    }
}

