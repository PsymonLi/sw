/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';


export interface ISearchError {
    'type'?: string;
    'reason'?: string;
}


export class SearchError extends BaseModel implements ISearchError {
    /** Type of error */
    'type': string = null;
    /** Reason or description of the failure */
    'reason': string = null;
    public static propInfo: { [prop in keyof ISearchError]: PropInfoItem } = {
        'type': {
            description:  `Type of error`,
            required: false,
            type: 'string'
        },
        'reason': {
            description:  `Reason or description of the failure`,
            required: false,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return SearchError.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return SearchError.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (SearchError.propInfo[prop] != null &&
                        SearchError.propInfo[prop].default != null);
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
        } else if (fillDefaults && SearchError.hasDefaultValue('type')) {
            this['type'] = SearchError.propInfo['type'].default;
        } else {
            this['type'] = null
        }
        if (values && values['reason'] != null) {
            this['reason'] = values['reason'];
        } else if (fillDefaults && SearchError.hasDefaultValue('reason')) {
            this['reason'] = SearchError.propInfo['reason'].default;
        } else {
            this['reason'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'type': CustomFormControl(new FormControl(this['type']), SearchError.propInfo['type']),
                'reason': CustomFormControl(new FormControl(this['reason']), SearchError.propInfo['reason']),
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
            this._formGroup.controls['reason'].setValue(this['reason']);
        }
    }
}

