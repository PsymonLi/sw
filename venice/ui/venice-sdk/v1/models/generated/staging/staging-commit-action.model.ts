/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, enumValidator } from './validators';
import { BaseModel, PropInfoItem } from './base-model';

import { ApiObjectMeta, IApiObjectMeta } from './api-object-meta.model';
import { StagingCommitActionSpec, IStagingCommitActionSpec } from './staging-commit-action-spec.model';
import { StagingCommitActionStatus, IStagingCommitActionStatus } from './staging-commit-action-status.model';

export interface IStagingCommitAction {
    'kind'?: string;
    'api-version'?: string;
    'meta'?: IApiObjectMeta;
    'spec'?: IStagingCommitActionSpec;
    'status'?: IStagingCommitActionStatus;
}


export class StagingCommitAction extends BaseModel implements IStagingCommitAction {
    'kind': string = null;
    'api-version': string = null;
    'meta': ApiObjectMeta = null;
    'spec': StagingCommitActionSpec = null;
    'status': StagingCommitActionStatus = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'kind': {
            type: 'string'
        },
        'api-version': {
            type: 'string'
        },
        'meta': {
            type: 'object'
        },
        'spec': {
            type: 'object'
        },
        'status': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return StagingCommitAction.propInfo[propName];
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (StagingCommitAction.propInfo[prop] != null &&
                        StagingCommitAction.propInfo[prop].default != null &&
                        StagingCommitAction.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any) {
        super();
        this['meta'] = new ApiObjectMeta();
        this['spec'] = new StagingCommitActionSpec();
        this['status'] = new StagingCommitActionStatus();
        this.setValues(values);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['kind'] != null) {
            this['kind'] = values['kind'];
        } else if (fillDefaults && StagingCommitAction.hasDefaultValue('kind')) {
            this['kind'] = StagingCommitAction.propInfo['kind'].default;
        }
        if (values && values['api-version'] != null) {
            this['api-version'] = values['api-version'];
        } else if (fillDefaults && StagingCommitAction.hasDefaultValue('api-version')) {
            this['api-version'] = StagingCommitAction.propInfo['api-version'].default;
        }
        if (values) {
            this['meta'].setValues(values['meta']);
        }
        if (values) {
            this['spec'].setValues(values['spec']);
        }
        if (values) {
            this['status'].setValues(values['status']);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'kind': new FormControl(this['kind']),
                'api-version': new FormControl(this['api-version']),
                'meta': this['meta'].$formGroup,
                'spec': this['spec'].$formGroup,
                'status': this['status'].$formGroup,
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['kind'].setValue(this['kind']);
            this._formGroup.controls['api-version'].setValue(this['api-version']);
            this['meta'].setFormGroupValuesToBeModelValues();
            this['spec'].setFormGroupValuesToBeModelValues();
            this['status'].setFormGroupValuesToBeModelValues();
        }
    }
}

