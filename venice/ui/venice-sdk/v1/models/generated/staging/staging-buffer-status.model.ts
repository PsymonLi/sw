/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { StagingBufferStatus_validation_result,  StagingBufferStatus_validation_result_uihint  } from './enums';
import { StagingValidationError, IStagingValidationError } from './staging-validation-error.model';
import { StagingItem, IStagingItem } from './staging-item.model';

export interface IStagingBufferStatus {
    'validation-result': StagingBufferStatus_validation_result;
    'errors'?: Array<IStagingValidationError>;
    'items'?: Array<IStagingItem>;
}


export class StagingBufferStatus extends BaseModel implements IStagingBufferStatus {
    'validation-result': StagingBufferStatus_validation_result = null;
    'errors': Array<StagingValidationError> = null;
    'items': Array<StagingItem> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'validation-result': {
            enum: StagingBufferStatus_validation_result_uihint,
            default: 'SUCCESS',
            required: true,
            type: 'string'
        },
        'errors': {
            required: false,
            type: 'object'
        },
        'items': {
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return StagingBufferStatus.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return StagingBufferStatus.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (StagingBufferStatus.propInfo[prop] != null &&
                        StagingBufferStatus.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['errors'] = new Array<StagingValidationError>();
        this['items'] = new Array<StagingItem>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['validation-result'] != null) {
            this['validation-result'] = values['validation-result'];
        } else if (fillDefaults && StagingBufferStatus.hasDefaultValue('validation-result')) {
            this['validation-result'] = <StagingBufferStatus_validation_result>  StagingBufferStatus.propInfo['validation-result'].default;
        } else {
            this['validation-result'] = null
        }
        if (values) {
            this.fillModelArray<StagingValidationError>(this, 'errors', values['errors'], StagingValidationError);
        } else {
            this['errors'] = [];
        }
        if (values) {
            this.fillModelArray<StagingItem>(this, 'items', values['items'], StagingItem);
        } else {
            this['items'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'validation-result': CustomFormControl(new FormControl(this['validation-result'], [required, enumValidator(StagingBufferStatus_validation_result), ]), StagingBufferStatus.propInfo['validation-result']),
                'errors': new FormArray([]),
                'items': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<StagingValidationError>('errors', this['errors'], StagingValidationError);
            // generate FormArray control elements
            this.fillFormArray<StagingItem>('items', this['items'], StagingItem);
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('errors') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('errors').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('items') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('items').get(field);
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
            this._formGroup.controls['validation-result'].setValue(this['validation-result']);
            this.fillModelArray<StagingValidationError>(this, 'errors', this['errors'], StagingValidationError);
            this.fillModelArray<StagingItem>(this, 'items', this['items'], StagingItem);
        }
    }
}

