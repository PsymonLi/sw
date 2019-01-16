/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator } from './validators';
import { BaseModel, PropInfoItem } from './base-model';


export interface IMonitoringAlertSource {
    'component'?: string;
    'node-name'?: string;
}


export class MonitoringAlertSource extends BaseModel implements IMonitoringAlertSource {
    'component': string = null;
    'node-name': string = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'component': {
            type: 'string'
        },
        'node-name': {
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return MonitoringAlertSource.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return MonitoringAlertSource.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (MonitoringAlertSource.propInfo[prop] != null &&
                        MonitoringAlertSource.propInfo[prop].default != null &&
                        MonitoringAlertSource.propInfo[prop].default != '');
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
        if (values && values['component'] != null) {
            this['component'] = values['component'];
        } else if (fillDefaults && MonitoringAlertSource.hasDefaultValue('component')) {
            this['component'] = MonitoringAlertSource.propInfo['component'].default;
        }
        if (values && values['node-name'] != null) {
            this['node-name'] = values['node-name'];
        } else if (fillDefaults && MonitoringAlertSource.hasDefaultValue('node-name')) {
            this['node-name'] = MonitoringAlertSource.propInfo['node-name'].default;
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'component': new FormControl(this['component']),
                'node-name': new FormControl(this['node-name']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['component'].setValue(this['component']);
            this._formGroup.controls['node-name'].setValue(this['node-name']);
        }
    }
}

