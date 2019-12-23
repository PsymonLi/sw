/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';


export interface IClusterCPUInfo {
    'speed'?: string;
    'num-sockets'?: number;
    'num-cores'?: number;
    'num-threads'?: number;
}


export class ClusterCPUInfo extends BaseModel implements IClusterCPUInfo {
    /** CPU speed per core, eg: 2099998101 */
    'speed': string = null;
    /** Number of CPU sockets, eg: 2, 4 */
    'num-sockets': number = null;
    /** Number of physical CPU cores per socket, eg: 36 */
    'num-cores': number = null;
    /** Number of threads per core, eg: 2 */
    'num-threads': number = null;
    public static propInfo: { [prop in keyof IClusterCPUInfo]: PropInfoItem } = {
        'speed': {
            description:  `CPU speed per core, eg: 2099998101`,
            required: false,
            type: 'string'
        },
        'num-sockets': {
            description:  `Number of CPU sockets, eg: 2, 4`,
            required: false,
            type: 'number'
        },
        'num-cores': {
            description:  `Number of physical CPU cores per socket, eg: 36`,
            required: false,
            type: 'number'
        },
        'num-threads': {
            description:  `Number of threads per core, eg: 2`,
            required: false,
            type: 'number'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return ClusterCPUInfo.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return ClusterCPUInfo.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (ClusterCPUInfo.propInfo[prop] != null &&
                        ClusterCPUInfo.propInfo[prop].default != null);
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
        if (values && values['speed'] != null) {
            this['speed'] = values['speed'];
        } else if (fillDefaults && ClusterCPUInfo.hasDefaultValue('speed')) {
            this['speed'] = ClusterCPUInfo.propInfo['speed'].default;
        } else {
            this['speed'] = null
        }
        if (values && values['num-sockets'] != null) {
            this['num-sockets'] = values['num-sockets'];
        } else if (fillDefaults && ClusterCPUInfo.hasDefaultValue('num-sockets')) {
            this['num-sockets'] = ClusterCPUInfo.propInfo['num-sockets'].default;
        } else {
            this['num-sockets'] = null
        }
        if (values && values['num-cores'] != null) {
            this['num-cores'] = values['num-cores'];
        } else if (fillDefaults && ClusterCPUInfo.hasDefaultValue('num-cores')) {
            this['num-cores'] = ClusterCPUInfo.propInfo['num-cores'].default;
        } else {
            this['num-cores'] = null
        }
        if (values && values['num-threads'] != null) {
            this['num-threads'] = values['num-threads'];
        } else if (fillDefaults && ClusterCPUInfo.hasDefaultValue('num-threads')) {
            this['num-threads'] = ClusterCPUInfo.propInfo['num-threads'].default;
        } else {
            this['num-threads'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'speed': CustomFormControl(new FormControl(this['speed']), ClusterCPUInfo.propInfo['speed']),
                'num-sockets': CustomFormControl(new FormControl(this['num-sockets']), ClusterCPUInfo.propInfo['num-sockets']),
                'num-cores': CustomFormControl(new FormControl(this['num-cores']), ClusterCPUInfo.propInfo['num-cores']),
                'num-threads': CustomFormControl(new FormControl(this['num-threads']), ClusterCPUInfo.propInfo['num-threads']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['speed'].setValue(this['speed']);
            this._formGroup.controls['num-sockets'].setValue(this['num-sockets']);
            this._formGroup.controls['num-cores'].setValue(this['num-cores']);
            this._formGroup.controls['num-threads'].setValue(this['num-threads']);
        }
    }
}

