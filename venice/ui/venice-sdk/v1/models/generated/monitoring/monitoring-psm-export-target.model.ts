/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';


export interface IMonitoringPSMExportTarget {
    'enable'?: boolean;
    '_ui'?: any;
}


export class MonitoringPSMExportTarget extends BaseModel implements IMonitoringPSMExportTarget {
    /** Field for holding arbitrary ui state */
    '_ui': any = {};
    /** Enable is for enabling the log export. Its default value is false. */
    'enable': boolean = null;
    public static propInfo: { [prop in keyof IMonitoringPSMExportTarget]: PropInfoItem } = {
        'enable': {
            description:  `Enable is for enabling the log export. Its default value is false.`,
            required: false,
            type: 'boolean'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return MonitoringPSMExportTarget.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return MonitoringPSMExportTarget.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (MonitoringPSMExportTarget.propInfo[prop] != null &&
                        MonitoringPSMExportTarget.propInfo[prop].default != null);
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
        if (values && values['enable'] != null) {
            this['enable'] = values['enable'];
        } else if (fillDefaults && MonitoringPSMExportTarget.hasDefaultValue('enable')) {
            this['enable'] = MonitoringPSMExportTarget.propInfo['enable'].default;
        } else {
            this['enable'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'enable': CustomFormControl(new FormControl(this['enable']), MonitoringPSMExportTarget.propInfo['enable']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['enable'].setValue(this['enable']);
        }
    }
}
