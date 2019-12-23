/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { ApiListWatchOptions_sort_order,  ApiListWatchOptions_sort_order_uihint  } from './enums';

export interface IApiListWatchOptions {
    'name'?: string;
    'tenant'?: string;
    'namespace'?: string;
    'generation-id'?: string;
    'resource-version'?: string;
    'uuid'?: string;
    'labels'?: object;
    'creation-time'?: Date;
    'mod-time'?: Date;
    'self-link'?: string;
    'label-selector'?: string;
    'field-selector'?: string;
    'field-change-selector'?: Array<string>;
    'from'?: number;
    'max-results'?: number;
    'sort-order': ApiListWatchOptions_sort_order;
}


export class ApiListWatchOptions extends BaseModel implements IApiListWatchOptions {
    /** must start and end with alpha numeric and can have alphanumeric, -, _, .length of string should be between 2 and 64 */
    'name': string = null;
    /** must be alpha-numericslength of string should be between 1 and 48 */
    'tenant': string = null;
    /** must start and end with alpha numeric and can have alphanumeric, -, _, .length of string should be between 2 and 64 */
    'namespace': string = null;
    'generation-id': string = null;
    'resource-version': string = null;
    'uuid': string = null;
    'labels': object = null;
    'creation-time': Date = null;
    'mod-time': Date = null;
    'self-link': string = null;
    /** LabelSelector to select on labels in list or watch results. */
    'label-selector': string = null;
    /** FieldSelector to select on field values in list or watch results */
    'field-selector': string = null;
    /** FieldChangeSelector specifies to generate a watch notification on change in field(s) specified. */
    'field-change-selector': Array<string> = null;
    /** from represents the start offset (zero based), used for pagination.
results returned would be in the range [from ... from+max-results-1] */
    'from': number = null;
    /** max. number of events to be fetched for the request. */
    'max-results': number = null;
    /** order to sort List results in. */
    'sort-order': ApiListWatchOptions_sort_order = null;
    public static propInfo: { [prop in keyof IApiListWatchOptions]: PropInfoItem } = {
        'name': {
            description:  `Must start and end with alpha numeric and can have alphanumeric, -, _, .length of string should be between 2 and 64`,
            required: false,
            type: 'string'
        },
        'tenant': {
            description:  `Must be alpha-numericslength of string should be between 1 and 48`,
            required: false,
            type: 'string'
        },
        'namespace': {
            description:  `Must start and end with alpha numeric and can have alphanumeric, -, _, .length of string should be between 2 and 64`,
            required: false,
            type: 'string'
        },
        'generation-id': {
            required: false,
            type: 'string'
        },
        'resource-version': {
            required: false,
            type: 'string'
        },
        'uuid': {
            required: false,
            type: 'string'
        },
        'labels': {
            required: false,
            type: 'object'
        },
        'creation-time': {
            required: false,
            type: 'Date'
        },
        'mod-time': {
            required: false,
            type: 'Date'
        },
        'self-link': {
            required: false,
            type: 'string'
        },
        'label-selector': {
            description:  `LabelSelector to select on labels in list or watch results.`,
            required: false,
            type: 'string'
        },
        'field-selector': {
            description:  `FieldSelector to select on field values in list or watch results`,
            required: false,
            type: 'string'
        },
        'field-change-selector': {
            description:  `FieldChangeSelector specifies to generate a watch notification on change in field(s) specified.`,
            required: false,
            type: 'Array<string>'
        },
        'from': {
            description:  `From represents the start offset (zero based), used for pagination. results returned would be in the range [from ... from+max-results-1]`,
            required: false,
            type: 'number'
        },
        'max-results': {
            description:  `Max. number of events to be fetched for the request.`,
            required: false,
            type: 'number'
        },
        'sort-order': {
            enum: ApiListWatchOptions_sort_order_uihint,
            default: 'none',
            description:  `Order to sort List results in.`,
            required: true,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return ApiListWatchOptions.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return ApiListWatchOptions.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (ApiListWatchOptions.propInfo[prop] != null &&
                        ApiListWatchOptions.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['field-change-selector'] = new Array<string>();
        this._inputValue = values;
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['name'] != null) {
            this['name'] = values['name'];
        } else if (fillDefaults && ApiListWatchOptions.hasDefaultValue('name')) {
            this['name'] = ApiListWatchOptions.propInfo['name'].default;
        } else {
            this['name'] = null
        }
        if (values && values['tenant'] != null) {
            this['tenant'] = values['tenant'];
        } else if (fillDefaults && ApiListWatchOptions.hasDefaultValue('tenant')) {
            this['tenant'] = ApiListWatchOptions.propInfo['tenant'].default;
        } else {
            this['tenant'] = null
        }
        if (values && values['namespace'] != null) {
            this['namespace'] = values['namespace'];
        } else if (fillDefaults && ApiListWatchOptions.hasDefaultValue('namespace')) {
            this['namespace'] = ApiListWatchOptions.propInfo['namespace'].default;
        } else {
            this['namespace'] = null
        }
        if (values && values['generation-id'] != null) {
            this['generation-id'] = values['generation-id'];
        } else if (fillDefaults && ApiListWatchOptions.hasDefaultValue('generation-id')) {
            this['generation-id'] = ApiListWatchOptions.propInfo['generation-id'].default;
        } else {
            this['generation-id'] = null
        }
        if (values && values['resource-version'] != null) {
            this['resource-version'] = values['resource-version'];
        } else if (fillDefaults && ApiListWatchOptions.hasDefaultValue('resource-version')) {
            this['resource-version'] = ApiListWatchOptions.propInfo['resource-version'].default;
        } else {
            this['resource-version'] = null
        }
        if (values && values['uuid'] != null) {
            this['uuid'] = values['uuid'];
        } else if (fillDefaults && ApiListWatchOptions.hasDefaultValue('uuid')) {
            this['uuid'] = ApiListWatchOptions.propInfo['uuid'].default;
        } else {
            this['uuid'] = null
        }
        if (values && values['labels'] != null) {
            this['labels'] = values['labels'];
        } else if (fillDefaults && ApiListWatchOptions.hasDefaultValue('labels')) {
            this['labels'] = ApiListWatchOptions.propInfo['labels'].default;
        } else {
            this['labels'] = null
        }
        if (values && values['creation-time'] != null) {
            this['creation-time'] = values['creation-time'];
        } else if (fillDefaults && ApiListWatchOptions.hasDefaultValue('creation-time')) {
            this['creation-time'] = ApiListWatchOptions.propInfo['creation-time'].default;
        } else {
            this['creation-time'] = null
        }
        if (values && values['mod-time'] != null) {
            this['mod-time'] = values['mod-time'];
        } else if (fillDefaults && ApiListWatchOptions.hasDefaultValue('mod-time')) {
            this['mod-time'] = ApiListWatchOptions.propInfo['mod-time'].default;
        } else {
            this['mod-time'] = null
        }
        if (values && values['self-link'] != null) {
            this['self-link'] = values['self-link'];
        } else if (fillDefaults && ApiListWatchOptions.hasDefaultValue('self-link')) {
            this['self-link'] = ApiListWatchOptions.propInfo['self-link'].default;
        } else {
            this['self-link'] = null
        }
        if (values && values['label-selector'] != null) {
            this['label-selector'] = values['label-selector'];
        } else if (fillDefaults && ApiListWatchOptions.hasDefaultValue('label-selector')) {
            this['label-selector'] = ApiListWatchOptions.propInfo['label-selector'].default;
        } else {
            this['label-selector'] = null
        }
        if (values && values['field-selector'] != null) {
            this['field-selector'] = values['field-selector'];
        } else if (fillDefaults && ApiListWatchOptions.hasDefaultValue('field-selector')) {
            this['field-selector'] = ApiListWatchOptions.propInfo['field-selector'].default;
        } else {
            this['field-selector'] = null
        }
        if (values && values['field-change-selector'] != null) {
            this['field-change-selector'] = values['field-change-selector'];
        } else if (fillDefaults && ApiListWatchOptions.hasDefaultValue('field-change-selector')) {
            this['field-change-selector'] = [ ApiListWatchOptions.propInfo['field-change-selector'].default];
        } else {
            this['field-change-selector'] = [];
        }
        if (values && values['from'] != null) {
            this['from'] = values['from'];
        } else if (fillDefaults && ApiListWatchOptions.hasDefaultValue('from')) {
            this['from'] = ApiListWatchOptions.propInfo['from'].default;
        } else {
            this['from'] = null
        }
        if (values && values['max-results'] != null) {
            this['max-results'] = values['max-results'];
        } else if (fillDefaults && ApiListWatchOptions.hasDefaultValue('max-results')) {
            this['max-results'] = ApiListWatchOptions.propInfo['max-results'].default;
        } else {
            this['max-results'] = null
        }
        if (values && values['sort-order'] != null) {
            this['sort-order'] = values['sort-order'];
        } else if (fillDefaults && ApiListWatchOptions.hasDefaultValue('sort-order')) {
            this['sort-order'] = <ApiListWatchOptions_sort_order>  ApiListWatchOptions.propInfo['sort-order'].default;
        } else {
            this['sort-order'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'name': CustomFormControl(new FormControl(this['name'], [minLengthValidator(2), maxLengthValidator(64), patternValidator('^[a-zA-Z0-9][\\w\\-\\.]*[a-zA-Z0-9]$', 'must start and end with alpha numeric and can have alphanumeric, -, _, .length of string should be between 2 and 64'), ]), ApiListWatchOptions.propInfo['name']),
                'tenant': CustomFormControl(new FormControl(this['tenant'], [minLengthValidator(1), maxLengthValidator(48), patternValidator('^[a-zA-Z0-9]+$', 'must be alpha-numericslength of string should be between 1 and 48'), ]), ApiListWatchOptions.propInfo['tenant']),
                'namespace': CustomFormControl(new FormControl(this['namespace'], [minLengthValidator(2), maxLengthValidator(64), patternValidator('^[a-zA-Z0-9][\\w\\-\\.]*[a-zA-Z0-9]$', 'must start and end with alpha numeric and can have alphanumeric, -, _, .length of string should be between 2 and 64'), ]), ApiListWatchOptions.propInfo['namespace']),
                'generation-id': CustomFormControl(new FormControl(this['generation-id']), ApiListWatchOptions.propInfo['generation-id']),
                'resource-version': CustomFormControl(new FormControl(this['resource-version']), ApiListWatchOptions.propInfo['resource-version']),
                'uuid': CustomFormControl(new FormControl(this['uuid']), ApiListWatchOptions.propInfo['uuid']),
                'labels': CustomFormControl(new FormControl(this['labels']), ApiListWatchOptions.propInfo['labels']),
                'creation-time': CustomFormControl(new FormControl(this['creation-time']), ApiListWatchOptions.propInfo['creation-time']),
                'mod-time': CustomFormControl(new FormControl(this['mod-time']), ApiListWatchOptions.propInfo['mod-time']),
                'self-link': CustomFormControl(new FormControl(this['self-link']), ApiListWatchOptions.propInfo['self-link']),
                'label-selector': CustomFormControl(new FormControl(this['label-selector']), ApiListWatchOptions.propInfo['label-selector']),
                'field-selector': CustomFormControl(new FormControl(this['field-selector']), ApiListWatchOptions.propInfo['field-selector']),
                'field-change-selector': CustomFormControl(new FormControl(this['field-change-selector']), ApiListWatchOptions.propInfo['field-change-selector']),
                'from': CustomFormControl(new FormControl(this['from']), ApiListWatchOptions.propInfo['from']),
                'max-results': CustomFormControl(new FormControl(this['max-results']), ApiListWatchOptions.propInfo['max-results']),
                'sort-order': CustomFormControl(new FormControl(this['sort-order'], [required, enumValidator(ApiListWatchOptions_sort_order), ]), ApiListWatchOptions.propInfo['sort-order']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['name'].setValue(this['name']);
            this._formGroup.controls['tenant'].setValue(this['tenant']);
            this._formGroup.controls['namespace'].setValue(this['namespace']);
            this._formGroup.controls['generation-id'].setValue(this['generation-id']);
            this._formGroup.controls['resource-version'].setValue(this['resource-version']);
            this._formGroup.controls['uuid'].setValue(this['uuid']);
            this._formGroup.controls['labels'].setValue(this['labels']);
            this._formGroup.controls['creation-time'].setValue(this['creation-time']);
            this._formGroup.controls['mod-time'].setValue(this['mod-time']);
            this._formGroup.controls['self-link'].setValue(this['self-link']);
            this._formGroup.controls['label-selector'].setValue(this['label-selector']);
            this._formGroup.controls['field-selector'].setValue(this['field-selector']);
            this._formGroup.controls['field-change-selector'].setValue(this['field-change-selector']);
            this._formGroup.controls['from'].setValue(this['from']);
            this._formGroup.controls['max-results'].setValue(this['max-results']);
            this._formGroup.controls['sort-order'].setValue(this['sort-order']);
        }
    }
}

