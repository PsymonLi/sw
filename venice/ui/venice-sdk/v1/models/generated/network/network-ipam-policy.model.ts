/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { ApiObjectMeta, IApiObjectMeta } from './api-object-meta.model';
import { NetworkIPAMPolicySpec, INetworkIPAMPolicySpec } from './network-ipam-policy-spec.model';
import { NetworkIPAMPolicyStatus, INetworkIPAMPolicyStatus } from './network-ipam-policy-status.model';

export interface INetworkIPAMPolicy {
    'kind'?: string;
    'api-version'?: string;
    'meta'?: IApiObjectMeta;
    'spec'?: INetworkIPAMPolicySpec;
    'status'?: INetworkIPAMPolicyStatus;
}


export class NetworkIPAMPolicy extends BaseModel implements INetworkIPAMPolicy {
    'kind': string = null;
    'api-version': string = null;
    'meta': ApiObjectMeta = null;
    /** Spec contains the configuration for the IPAM service. */
    'spec': NetworkIPAMPolicySpec = null;
    /** Status contains the current state of the IPAM service. */
    'status': NetworkIPAMPolicyStatus = null;
    public static propInfo: { [prop in keyof INetworkIPAMPolicy]: PropInfoItem } = {
        'kind': {
            required: false,
            type: 'string'
        },
        'api-version': {
            required: false,
            type: 'string'
        },
        'meta': {
            required: false,
            type: 'object'
        },
        'spec': {
            description:  'Spec contains the configuration for the IPAM service.',
            required: false,
            type: 'object'
        },
        'status': {
            description:  'Status contains the current state of the IPAM service.',
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return NetworkIPAMPolicy.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return NetworkIPAMPolicy.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (NetworkIPAMPolicy.propInfo[prop] != null &&
                        NetworkIPAMPolicy.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['meta'] = new ApiObjectMeta();
        this['spec'] = new NetworkIPAMPolicySpec();
        this['status'] = new NetworkIPAMPolicyStatus();
        this._inputValue = values;
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['kind'] != null) {
            this['kind'] = values['kind'];
        } else if (fillDefaults && NetworkIPAMPolicy.hasDefaultValue('kind')) {
            this['kind'] = NetworkIPAMPolicy.propInfo['kind'].default;
        } else {
            this['kind'] = null
        }
        if (values && values['api-version'] != null) {
            this['api-version'] = values['api-version'];
        } else if (fillDefaults && NetworkIPAMPolicy.hasDefaultValue('api-version')) {
            this['api-version'] = NetworkIPAMPolicy.propInfo['api-version'].default;
        } else {
            this['api-version'] = null
        }
        if (values) {
            this['meta'].setValues(values['meta'], fillDefaults);
        } else {
            this['meta'].setValues(null, fillDefaults);
        }
        if (values) {
            this['spec'].setValues(values['spec'], fillDefaults);
        } else {
            this['spec'].setValues(null, fillDefaults);
        }
        if (values) {
            this['status'].setValues(values['status'], fillDefaults);
        } else {
            this['status'].setValues(null, fillDefaults);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'kind': CustomFormControl(new FormControl(this['kind']), NetworkIPAMPolicy.propInfo['kind']),
                'api-version': CustomFormControl(new FormControl(this['api-version']), NetworkIPAMPolicy.propInfo['api-version']),
                'meta': CustomFormGroup(this['meta'].$formGroup, NetworkIPAMPolicy.propInfo['meta'].required),
                'spec': CustomFormGroup(this['spec'].$formGroup, NetworkIPAMPolicy.propInfo['spec'].required),
                'status': CustomFormGroup(this['status'].$formGroup, NetworkIPAMPolicy.propInfo['status'].required),
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('meta') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('meta').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('spec') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('spec').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('status') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('status').get(field);
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
            this._formGroup.controls['kind'].setValue(this['kind']);
            this._formGroup.controls['api-version'].setValue(this['api-version']);
            this['meta'].setFormGroupValuesToBeModelValues();
            this['spec'].setFormGroupValuesToBeModelValues();
            this['status'].setFormGroupValuesToBeModelValues();
        }
    }
}

