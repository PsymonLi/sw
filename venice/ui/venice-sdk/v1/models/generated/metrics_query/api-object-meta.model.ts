/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator } from './validators';
import { BaseModel, PropInfoItem } from './base-model';


export interface IApiObjectMeta {
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
}


export class ApiObjectMeta extends BaseModel implements IApiObjectMeta {
    'name': string = null;
    'tenant': string = null;
    'namespace': string = null;
    /** GenerationID is the generation Id for the object. This is incremented anytime there
     is an update to the user intent, including Spec update and any update to ObjectMeta.
     System generated and updated, not updatable by user. */
    'generation-id': string = null;
    /** Resource version in the object store. This is updated anytime there is any change to the object.
     System generated and updated, not updatable by user. */
    'resource-version': string = null;
    /** UUID is the unique identifier for the object. This is generated on creation of the object.
    System generated, not updatable by user. */
    'uuid': string = null;
    /** Labels are arbitrary (key,value) pairs associated with any object. */
    'labels': object = null;
    /** CreationTime is the creation time of the object
     System generated and updated, not updatable by user. */
    'creation-time': Date = null;
    /** ModTime is the Last Modification time of the object
     System generated and updated, not updatable by user. */
    'mod-time': Date = null;
    /** SelfLink is a link for accessing this object. When the object is served from the API-GW it is the
     URI path. Example:
       - "/v1/tenants/tenants/tenant2" 
     System generated and updated, not updatable by user. */
    'self-link': string = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'name': {
            type: 'string'
        },
        'tenant': {
            type: 'string'
        },
        'namespace': {
            type: 'string'
        },
        'generation-id': {
            description:  'GenerationID is the generation Id for the object. This is incremented anytime there  is an update to the user intent, including Spec update and any update to ObjectMeta.  System generated and updated, not updatable by user.',
            type: 'string'
        },
        'resource-version': {
            description:  'Resource version in the object store. This is updated anytime there is any change to the object.  System generated and updated, not updatable by user.',
            type: 'string'
        },
        'uuid': {
            description:  'UUID is the unique identifier for the object. This is generated on creation of the object. System generated, not updatable by user.',
            type: 'string'
        },
        'labels': {
            description:  'Labels are arbitrary (key,value) pairs associated with any object.',
            type: 'object'
        },
        'creation-time': {
            description:  'CreationTime is the creation time of the object  System generated and updated, not updatable by user.',
            type: 'Date'
        },
        'mod-time': {
            description:  'ModTime is the Last Modification time of the object  System generated and updated, not updatable by user.',
            type: 'Date'
        },
        'self-link': {
            description:  'SelfLink is a link for accessing this object. When the object is served from the API-GW it is the  URI path. Example:    - &quot;/v1/tenants/tenants/tenant2&quot;   System generated and updated, not updatable by user.',
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return ApiObjectMeta.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return ApiObjectMeta.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (ApiObjectMeta.propInfo[prop] != null &&
                        ApiObjectMeta.propInfo[prop].default != null &&
                        ApiObjectMeta.propInfo[prop].default != '');
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
        if (values && values['name'] != null) {
            this['name'] = values['name'];
        } else if (fillDefaults && ApiObjectMeta.hasDefaultValue('name')) {
            this['name'] = ApiObjectMeta.propInfo['name'].default;
        }
        if (values && values['tenant'] != null) {
            this['tenant'] = values['tenant'];
        } else if (fillDefaults && ApiObjectMeta.hasDefaultValue('tenant')) {
            this['tenant'] = ApiObjectMeta.propInfo['tenant'].default;
        }
        if (values && values['namespace'] != null) {
            this['namespace'] = values['namespace'];
        } else if (fillDefaults && ApiObjectMeta.hasDefaultValue('namespace')) {
            this['namespace'] = ApiObjectMeta.propInfo['namespace'].default;
        }
        if (values && values['generation-id'] != null) {
            this['generation-id'] = values['generation-id'];
        } else if (fillDefaults && ApiObjectMeta.hasDefaultValue('generation-id')) {
            this['generation-id'] = ApiObjectMeta.propInfo['generation-id'].default;
        }
        if (values && values['resource-version'] != null) {
            this['resource-version'] = values['resource-version'];
        } else if (fillDefaults && ApiObjectMeta.hasDefaultValue('resource-version')) {
            this['resource-version'] = ApiObjectMeta.propInfo['resource-version'].default;
        }
        if (values && values['uuid'] != null) {
            this['uuid'] = values['uuid'];
        } else if (fillDefaults && ApiObjectMeta.hasDefaultValue('uuid')) {
            this['uuid'] = ApiObjectMeta.propInfo['uuid'].default;
        }
        if (values && values['labels'] != null) {
            this['labels'] = values['labels'];
        } else if (fillDefaults && ApiObjectMeta.hasDefaultValue('labels')) {
            this['labels'] = ApiObjectMeta.propInfo['labels'].default;
        }
        if (values && values['creation-time'] != null) {
            this['creation-time'] = values['creation-time'];
        } else if (fillDefaults && ApiObjectMeta.hasDefaultValue('creation-time')) {
            this['creation-time'] = ApiObjectMeta.propInfo['creation-time'].default;
        }
        if (values && values['mod-time'] != null) {
            this['mod-time'] = values['mod-time'];
        } else if (fillDefaults && ApiObjectMeta.hasDefaultValue('mod-time')) {
            this['mod-time'] = ApiObjectMeta.propInfo['mod-time'].default;
        }
        if (values && values['self-link'] != null) {
            this['self-link'] = values['self-link'];
        } else if (fillDefaults && ApiObjectMeta.hasDefaultValue('self-link')) {
            this['self-link'] = ApiObjectMeta.propInfo['self-link'].default;
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'name': new FormControl(this['name']),
                'tenant': new FormControl(this['tenant']),
                'namespace': new FormControl(this['namespace']),
                'generation-id': new FormControl(this['generation-id']),
                'resource-version': new FormControl(this['resource-version']),
                'uuid': new FormControl(this['uuid']),
                'labels': new FormControl(this['labels']),
                'creation-time': new FormControl(this['creation-time']),
                'mod-time': new FormControl(this['mod-time']),
                'self-link': new FormControl(this['self-link']),
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
        }
    }
}

