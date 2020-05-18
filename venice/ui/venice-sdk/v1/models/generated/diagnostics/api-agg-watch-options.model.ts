/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { ApiObjectMeta, IApiObjectMeta } from './api-object-meta.model';
import { ApiKindWatchOptions, IApiKindWatchOptions } from './api-kind-watch-options.model';

export interface IApiAggWatchOptions {
    'meta'?: IApiObjectMeta;
    'watch-options'?: Array<IApiKindWatchOptions>;
    '_ui'?: any;
}


export class ApiAggWatchOptions extends BaseModel implements IApiAggWatchOptions {
    /** Field for holding arbitrary ui state */
    '_ui': any = {};
    'meta': ApiObjectMeta = null;
    'watch-options': Array<ApiKindWatchOptions> = null;
    public static propInfo: { [prop in keyof IApiAggWatchOptions]: PropInfoItem } = {
        'meta': {
            required: false,
            type: 'object'
        },
        'watch-options': {
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return ApiAggWatchOptions.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return ApiAggWatchOptions.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (ApiAggWatchOptions.propInfo[prop] != null &&
                        ApiAggWatchOptions.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['meta'] = new ApiObjectMeta();
        this['watch-options'] = new Array<ApiKindWatchOptions>();
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
        if (values) {
            this['meta'].setValues(values['meta'], fillDefaults);
        } else {
            this['meta'].setValues(null, fillDefaults);
        }
        if (values) {
            this.fillModelArray<ApiKindWatchOptions>(this, 'watch-options', values['watch-options'], ApiKindWatchOptions);
        } else {
            this['watch-options'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'meta': CustomFormGroup(this['meta'].$formGroup, ApiAggWatchOptions.propInfo['meta'].required),
                'watch-options': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<ApiKindWatchOptions>('watch-options', this['watch-options'], ApiKindWatchOptions);
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('meta') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('meta').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('watch-options') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('watch-options').get(field);
                control.updateValueAndValidity();
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this['meta'].setFormGroupValuesToBeModelValues();
            this.fillModelArray<ApiKindWatchOptions>(this, 'watch-options', this['watch-options'], ApiKindWatchOptions);
        }
    }
}

