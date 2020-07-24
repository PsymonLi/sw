/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';


export interface ISecuritySunrpc {
    'program-id'?: string;
    'timeout': string;
    '_ui'?: any;
}


export class SecuritySunrpc extends BaseModel implements ISecuritySunrpc {
    /** Field for holding arbitrary ui state */
    '_ui': any = {};
    /** RPC Program identifier. */
    'program-id': string = null;
    /** Timeout for this program id. Should be a valid time duration. */
    'timeout': string = null;
    public static propInfo: { [prop in keyof ISecuritySunrpc]: PropInfoItem } = {
        'program-id': {
            description:  `RPC Program identifier.`,
            required: false,
            type: 'string'
        },
        'timeout': {
            description:  `Timeout for this program id. Should be a valid time duration.`,
            hint:  '60s',
            required: true,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return SecuritySunrpc.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return SecuritySunrpc.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (SecuritySunrpc.propInfo[prop] != null &&
                        SecuritySunrpc.propInfo[prop].default != null);
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
        if (values && values['program-id'] != null) {
            this['program-id'] = values['program-id'];
        } else if (fillDefaults && SecuritySunrpc.hasDefaultValue('program-id')) {
            this['program-id'] = SecuritySunrpc.propInfo['program-id'].default;
        } else {
            this['program-id'] = null
        }
        if (values && values['timeout'] != null) {
            this['timeout'] = values['timeout'];
        } else if (fillDefaults && SecuritySunrpc.hasDefaultValue('timeout')) {
            this['timeout'] = SecuritySunrpc.propInfo['timeout'].default;
        } else {
            this['timeout'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'program-id': CustomFormControl(new FormControl(this['program-id']), SecuritySunrpc.propInfo['program-id']),
                'timeout': CustomFormControl(new FormControl(this['timeout'], [required, ]), SecuritySunrpc.propInfo['timeout']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['program-id'].setValue(this['program-id']);
            this._formGroup.controls['timeout'].setValue(this['timeout']);
        }
    }
}

