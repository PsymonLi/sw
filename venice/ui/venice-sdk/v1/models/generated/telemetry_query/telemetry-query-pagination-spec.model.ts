/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';


export interface ITelemetry_queryPaginationSpec {
    'offset'?: number;
    'count'?: number;
}


export class Telemetry_queryPaginationSpec extends BaseModel implements ITelemetry_queryPaginationSpec {
    'offset': number = null;
    'count': number = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'offset': {
            type: 'number'
        },
        'count': {
            type: 'number'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return Telemetry_queryPaginationSpec.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return Telemetry_queryPaginationSpec.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (Telemetry_queryPaginationSpec.propInfo[prop] != null &&
                        Telemetry_queryPaginationSpec.propInfo[prop].default != null &&
                        Telemetry_queryPaginationSpec.propInfo[prop].default != '');
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
        if (values && values['offset'] != null) {
            this['offset'] = values['offset'];
        } else if (fillDefaults && Telemetry_queryPaginationSpec.hasDefaultValue('offset')) {
            this['offset'] = Telemetry_queryPaginationSpec.propInfo['offset'].default;
        } else {
            this['offset'] = null
        }
        if (values && values['count'] != null) {
            this['count'] = values['count'];
        } else if (fillDefaults && Telemetry_queryPaginationSpec.hasDefaultValue('count')) {
            this['count'] = Telemetry_queryPaginationSpec.propInfo['count'].default;
        } else {
            this['count'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'offset': CustomFormControl(new FormControl(this['offset']), Telemetry_queryPaginationSpec.propInfo['offset'].description),
                'count': CustomFormControl(new FormControl(this['count']), Telemetry_queryPaginationSpec.propInfo['count'].description),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['offset'].setValue(this['offset']);
            this._formGroup.controls['count'].setValue(this['count']);
        }
    }
}

