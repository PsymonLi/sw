/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, enumValidator } from './validators';
import { BaseModel } from './base-model';
import { SearchEntryList } from './search-entry-list.model';


export interface ISearchKindAggregation {
    'kinds'?: { [key: string]: SearchEntryList };
}


export class SearchKindAggregation extends BaseModel implements ISearchKindAggregation {
    'kinds': { [key: string]: SearchEntryList };
    public static enumProperties = {
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any) {
        super();
        if (values) {
            this.setValues(values);
        }
    }

    /**
     * set the values.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any): void {
        if (values) {
            this['kinds'] = values['kinds'];
        }
    }

    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'kinds': new FormControl(this['kinds']),
            });
        }
        return this._formGroup;
    }

    setFormGroupValues() {
        if (this._formGroup) {
            this._formGroup.controls['kinds'].setValue(this['kinds']);
        }
    }
}

