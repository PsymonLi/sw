/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';


export interface ISearchTenantPreview {
    'tenants'?: object;
}


export class SearchTenantPreview extends BaseModel implements ISearchTenantPreview {
    'tenants': object = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'tenants': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return SearchTenantPreview.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return SearchTenantPreview.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (SearchTenantPreview.propInfo[prop] != null &&
                        SearchTenantPreview.propInfo[prop].default != null &&
                        SearchTenantPreview.propInfo[prop].default != '');
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
        if (values && values['tenants'] != null) {
            this['tenants'] = values['tenants'];
        } else if (fillDefaults && SearchTenantPreview.hasDefaultValue('tenants')) {
            this['tenants'] = SearchTenantPreview.propInfo['tenants'].default;
        } else {
            this['tenants'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'tenants': CustomFormControl(new FormControl(this['tenants']), SearchTenantPreview.propInfo['tenants'].description),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['tenants'].setValue(this['tenants']);
        }
    }
}

