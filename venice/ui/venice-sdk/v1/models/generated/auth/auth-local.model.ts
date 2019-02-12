/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';


export interface IAuthLocal {
    'enabled'?: boolean;
}


export class AuthLocal extends BaseModel implements IAuthLocal {
    'enabled': boolean = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'enabled': {
            type: 'boolean'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return AuthLocal.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return AuthLocal.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (AuthLocal.propInfo[prop] != null &&
                        AuthLocal.propInfo[prop].default != null &&
                        AuthLocal.propInfo[prop].default != '');
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
        if (values && values['enabled'] != null) {
            this['enabled'] = values['enabled'];
        } else if (fillDefaults && AuthLocal.hasDefaultValue('enabled')) {
            this['enabled'] = AuthLocal.propInfo['enabled'].default;
        } else {
            this['enabled'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'enabled': CustomFormControl(new FormControl(this['enabled']), AuthLocal.propInfo['enabled'].description),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['enabled'].setValue(this['enabled']);
        }
    }
}

