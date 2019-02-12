/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { SearchEntry, ISearchEntry } from './search-entry.model';

export interface ISearchEntryList {
    'entries'?: Array<ISearchEntry>;
}


export class SearchEntryList extends BaseModel implements ISearchEntryList {
    'entries': Array<SearchEntry> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'entries': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return SearchEntryList.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return SearchEntryList.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (SearchEntryList.propInfo[prop] != null &&
                        SearchEntryList.propInfo[prop].default != null &&
                        SearchEntryList.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['entries'] = new Array<SearchEntry>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values) {
            this.fillModelArray<SearchEntry>(this, 'entries', values['entries'], SearchEntry);
        } else {
            this['entries'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'entries': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<SearchEntry>('entries', this['entries'], SearchEntry);
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this.fillModelArray<SearchEntry>(this, 'entries', this['entries'], SearchEntry);
        }
    }
}

