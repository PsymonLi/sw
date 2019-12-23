/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';


export interface IGoogleprotobufAny {
    'type_url'?: string;
    'value'?: string;
}


export class GoogleprotobufAny extends BaseModel implements IGoogleprotobufAny {
    /** A URL/resource name whose content describes the type of the
serialized protocol buffer message.

For URLs which use the scheme `http`, `https`, or no scheme, the
following restrictions and interpretations apply:

* If no scheme is provided, `https` is assumed.
* The last segment of the URL's path must represent the fully
  qualified name of the type (as in `path/google.protobuf.Duration`).
  The name should be in a canonical form (e.g., leading "." is
  not accepted).
* An HTTP GET on the URL must yield a [google.protobuf.Type][]
  value in binary format, or produce an error.
* Applications are allowed to cache lookup results based on the
  URL, or have them precompiled into a binary to avoid any
  lookup. Therefore, binary compatibility needs to be preserved
  on changes to types. (Use versioned type names to manage
  breaking changes.)

Schemes other than `http`, `https` (or the empty scheme) might be
used with implementation specific semantics. */
    'type_url': string = null;
    /** Must be a valid serialized protocol buffer of the above specified type. */
    'value': string = null;
    public static propInfo: { [prop in keyof IGoogleprotobufAny]: PropInfoItem } = {
        'type_url': {
            description:  `A URL/resource name whose content describes the type of the serialized protocol buffer message.  For URLs which use the scheme 'http', 'https', or no scheme, the following restrictions and interpretations apply:  * If no scheme is provided, 'https' is assumed. * The last segment of the URL's path must represent the fully   qualified name of the type (as in 'path/google.protobuf.Duration').   The name should be in a canonical form (e.g., leading "." is   not accepted). * An HTTP GET on the URL must yield a [google.protobuf.Type][]   value in binary format, or produce an error. * Applications are allowed to cache lookup results based on the   URL, or have them precompiled into a binary to avoid any   lookup. Therefore, binary compatibility needs to be preserved   on changes to types. (Use versioned type names to manage   breaking changes.)  Schemes other than 'http', 'https' (or the empty scheme) might be used with implementation specific semantics.`,
            required: false,
            type: 'string'
        },
        'value': {
            description:  `Must be a valid serialized protocol buffer of the above specified type.`,
            required: false,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return GoogleprotobufAny.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return GoogleprotobufAny.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (GoogleprotobufAny.propInfo[prop] != null &&
                        GoogleprotobufAny.propInfo[prop].default != null);
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
        if (values && values['type_url'] != null) {
            this['type_url'] = values['type_url'];
        } else if (fillDefaults && GoogleprotobufAny.hasDefaultValue('type_url')) {
            this['type_url'] = GoogleprotobufAny.propInfo['type_url'].default;
        } else {
            this['type_url'] = null
        }
        if (values && values['value'] != null) {
            this['value'] = values['value'];
        } else if (fillDefaults && GoogleprotobufAny.hasDefaultValue('value')) {
            this['value'] = GoogleprotobufAny.propInfo['value'].default;
        } else {
            this['value'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'type_url': CustomFormControl(new FormControl(this['type_url']), GoogleprotobufAny.propInfo['type_url']),
                'value': CustomFormControl(new FormControl(this['value']), GoogleprotobufAny.propInfo['value']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['type_url'].setValue(this['type_url']);
            this._formGroup.controls['value'].setValue(this['value']);
        }
    }
}

