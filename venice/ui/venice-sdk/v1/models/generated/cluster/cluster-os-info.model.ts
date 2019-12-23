/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';


export interface IClusterOsInfo {
    'type'?: string;
    'kernel-release'?: string;
    'kernel-version'?: string;
    'processor'?: string;
}


export class ClusterOsInfo extends BaseModel implements IClusterOsInfo {
    /** OS Name
Eg: GNU/Linux */
    'type': string = null;
    /** Kernel release
Eg: 3.10.0-514.10.2.el7.x86_64 */
    'kernel-release': string = null;
    /** Kernel version
Eg: #1 SMP Fri Mar 3 00:04:05 UTC 2017 */
    'kernel-version': string = null;
    /** Processor Info
Eg: x86_64 */
    'processor': string = null;
    public static propInfo: { [prop in keyof IClusterOsInfo]: PropInfoItem } = {
        'type': {
            description:  `OS Name Eg: GNU/Linux`,
            required: false,
            type: 'string'
        },
        'kernel-release': {
            description:  `Kernel release Eg: 3.10.0-514.10.2.el7.x86_64`,
            required: false,
            type: 'string'
        },
        'kernel-version': {
            description:  `Kernel version Eg: #1 SMP Fri Mar 3 00:04:05 UTC 2017`,
            required: false,
            type: 'string'
        },
        'processor': {
            description:  `Processor Info Eg: x86_64`,
            required: false,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return ClusterOsInfo.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return ClusterOsInfo.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (ClusterOsInfo.propInfo[prop] != null &&
                        ClusterOsInfo.propInfo[prop].default != null);
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
        if (values && values['type'] != null) {
            this['type'] = values['type'];
        } else if (fillDefaults && ClusterOsInfo.hasDefaultValue('type')) {
            this['type'] = ClusterOsInfo.propInfo['type'].default;
        } else {
            this['type'] = null
        }
        if (values && values['kernel-release'] != null) {
            this['kernel-release'] = values['kernel-release'];
        } else if (fillDefaults && ClusterOsInfo.hasDefaultValue('kernel-release')) {
            this['kernel-release'] = ClusterOsInfo.propInfo['kernel-release'].default;
        } else {
            this['kernel-release'] = null
        }
        if (values && values['kernel-version'] != null) {
            this['kernel-version'] = values['kernel-version'];
        } else if (fillDefaults && ClusterOsInfo.hasDefaultValue('kernel-version')) {
            this['kernel-version'] = ClusterOsInfo.propInfo['kernel-version'].default;
        } else {
            this['kernel-version'] = null
        }
        if (values && values['processor'] != null) {
            this['processor'] = values['processor'];
        } else if (fillDefaults && ClusterOsInfo.hasDefaultValue('processor')) {
            this['processor'] = ClusterOsInfo.propInfo['processor'].default;
        } else {
            this['processor'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'type': CustomFormControl(new FormControl(this['type']), ClusterOsInfo.propInfo['type']),
                'kernel-release': CustomFormControl(new FormControl(this['kernel-release']), ClusterOsInfo.propInfo['kernel-release']),
                'kernel-version': CustomFormControl(new FormControl(this['kernel-version']), ClusterOsInfo.propInfo['kernel-version']),
                'processor': CustomFormControl(new FormControl(this['processor']), ClusterOsInfo.propInfo['processor']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['type'].setValue(this['type']);
            this._formGroup.controls['kernel-release'].setValue(this['kernel-release']);
            this._formGroup.controls['kernel-version'].setValue(this['kernel-version']);
            this._formGroup.controls['processor'].setValue(this['processor']);
        }
    }
}

