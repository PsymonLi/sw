/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { ApiObjectMeta, IApiObjectMeta } from './api-object-meta.model';
import { NetworkVirtualRouterPeeringGroupSpec, INetworkVirtualRouterPeeringGroupSpec } from './network-virtual-router-peering-group-spec.model';
import { NetworkVirtualRouterPeeringGroupStatus, INetworkVirtualRouterPeeringGroupStatus } from './network-virtual-router-peering-group-status.model';

export interface INetworkVirtualRouterPeeringGroup {
    'kind'?: string;
    'api-version'?: string;
    'meta'?: IApiObjectMeta;
    'spec'?: INetworkVirtualRouterPeeringGroupSpec;
    'status'?: INetworkVirtualRouterPeeringGroupStatus;
    '_ui'?: any;
}


export class NetworkVirtualRouterPeeringGroup extends BaseModel implements INetworkVirtualRouterPeeringGroup {
    /** Field for holding arbitrary ui state */
    '_ui': any = {};
    'kind': string = null;
    'api-version': string = null;
    'meta': ApiObjectMeta = null;
    'spec': NetworkVirtualRouterPeeringGroupSpec = null;
    'status': NetworkVirtualRouterPeeringGroupStatus = null;
    public static propInfo: { [prop in keyof INetworkVirtualRouterPeeringGroup]: PropInfoItem } = {
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
            required: false,
            type: 'object'
        },
        'status': {
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return NetworkVirtualRouterPeeringGroup.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return NetworkVirtualRouterPeeringGroup.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (NetworkVirtualRouterPeeringGroup.propInfo[prop] != null &&
                        NetworkVirtualRouterPeeringGroup.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['meta'] = new ApiObjectMeta();
        this['spec'] = new NetworkVirtualRouterPeeringGroupSpec();
        this['status'] = new NetworkVirtualRouterPeeringGroupStatus();
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
        if (values && values['kind'] != null) {
            this['kind'] = values['kind'];
        } else if (fillDefaults && NetworkVirtualRouterPeeringGroup.hasDefaultValue('kind')) {
            this['kind'] = NetworkVirtualRouterPeeringGroup.propInfo['kind'].default;
        } else {
            this['kind'] = null
        }
        if (values && values['api-version'] != null) {
            this['api-version'] = values['api-version'];
        } else if (fillDefaults && NetworkVirtualRouterPeeringGroup.hasDefaultValue('api-version')) {
            this['api-version'] = NetworkVirtualRouterPeeringGroup.propInfo['api-version'].default;
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
                'kind': CustomFormControl(new FormControl(this['kind']), NetworkVirtualRouterPeeringGroup.propInfo['kind']),
                'api-version': CustomFormControl(new FormControl(this['api-version']), NetworkVirtualRouterPeeringGroup.propInfo['api-version']),
                'meta': CustomFormGroup(this['meta'].$formGroup, NetworkVirtualRouterPeeringGroup.propInfo['meta'].required),
                'spec': CustomFormGroup(this['spec'].$formGroup, NetworkVirtualRouterPeeringGroup.propInfo['spec'].required),
                'status': CustomFormGroup(this['status'].$formGroup, NetworkVirtualRouterPeeringGroup.propInfo['status'].required),
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

