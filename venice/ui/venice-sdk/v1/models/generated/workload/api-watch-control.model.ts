/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';


export interface IApiWatchControl {
    'code'?: number;
    'message'?: string;
    '_ui'?: any;
}


export class ApiWatchControl extends BaseModel implements IApiWatchControl {
    /** Field for holding arbitrary ui state */
    '_ui': any = {};
    'code': number = null;
    'message': string = null;
    public static propInfo: { [prop in keyof IApiWatchControl]: PropInfoItem } = {
        'code': {
            required: false,
            type: 'number'
        },
        'message': {
            required: false,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return ApiWatchControl.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return ApiWatchControl.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (ApiWatchControl.propInfo[prop] != null &&
                        ApiWatchControl.propInfo[prop].default != null);
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
        if (values && values['code'] != null) {
            this['code'] = values['code'];
        } else if (fillDefaults && ApiWatchControl.hasDefaultValue('code')) {
            this['code'] = ApiWatchControl.propInfo['code'].default;
        } else {
            this['code'] = null
        }
        if (values && values['message'] != null) {
            this['message'] = values['message'];
        } else if (fillDefaults && ApiWatchControl.hasDefaultValue('message')) {
            this['message'] = ApiWatchControl.propInfo['message'].default;
        } else {
            this['message'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'code': CustomFormControl(new FormControl(this['code']), ApiWatchControl.propInfo['code']),
                'message': CustomFormControl(new FormControl(this['message']), ApiWatchControl.propInfo['message']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['code'].setValue(this['code']);
            this._formGroup.controls['message'].setValue(this['message']);
        }
    }
}
