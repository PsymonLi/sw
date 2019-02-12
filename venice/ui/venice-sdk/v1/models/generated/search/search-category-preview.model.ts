/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';


export interface ISearchCategoryPreview {
    'categories'?: object;
}


export class SearchCategoryPreview extends BaseModel implements ISearchCategoryPreview {
    'categories': object = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'categories': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return SearchCategoryPreview.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return SearchCategoryPreview.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (SearchCategoryPreview.propInfo[prop] != null &&
                        SearchCategoryPreview.propInfo[prop].default != null &&
                        SearchCategoryPreview.propInfo[prop].default != '');
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
        if (values && values['categories'] != null) {
            this['categories'] = values['categories'];
        } else if (fillDefaults && SearchCategoryPreview.hasDefaultValue('categories')) {
            this['categories'] = SearchCategoryPreview.propInfo['categories'].default;
        } else {
            this['categories'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'categories': CustomFormControl(new FormControl(this['categories']), SearchCategoryPreview.propInfo['categories'].description),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['categories'].setValue(this['categories']);
        }
    }
}

