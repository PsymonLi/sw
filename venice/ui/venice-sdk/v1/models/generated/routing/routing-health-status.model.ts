/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { HealthStatusPeeringStatus, IHealthStatusPeeringStatus } from './health-status-peering-status.model';

export interface IRoutingHealthStatus {
    'router-id'?: string;
    'internal-peers'?: IHealthStatusPeeringStatus;
    'external-peers'?: IHealthStatusPeeringStatus;
    'unexpected-peers'?: number;
    '_ui'?: any;
}


export class RoutingHealthStatus extends BaseModel implements IRoutingHealthStatus {
    /** Field for holding arbitrary ui state */
    '_ui': any = {};
    'router-id': string = null;
    'internal-peers': HealthStatusPeeringStatus = null;
    'external-peers': HealthStatusPeeringStatus = null;
    'unexpected-peers': number = null;
    public static propInfo: { [prop in keyof IRoutingHealthStatus]: PropInfoItem } = {
        'router-id': {
            required: false,
            type: 'string'
        },
        'internal-peers': {
            required: false,
            type: 'object'
        },
        'external-peers': {
            required: false,
            type: 'object'
        },
        'unexpected-peers': {
            required: false,
            type: 'number'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return RoutingHealthStatus.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return RoutingHealthStatus.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (RoutingHealthStatus.propInfo[prop] != null &&
                        RoutingHealthStatus.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['internal-peers'] = new HealthStatusPeeringStatus();
        this['external-peers'] = new HealthStatusPeeringStatus();
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
        if (values && values['router-id'] != null) {
            this['router-id'] = values['router-id'];
        } else if (fillDefaults && RoutingHealthStatus.hasDefaultValue('router-id')) {
            this['router-id'] = RoutingHealthStatus.propInfo['router-id'].default;
        } else {
            this['router-id'] = null
        }
        if (values) {
            this['internal-peers'].setValues(values['internal-peers'], fillDefaults);
        } else {
            this['internal-peers'].setValues(null, fillDefaults);
        }
        if (values) {
            this['external-peers'].setValues(values['external-peers'], fillDefaults);
        } else {
            this['external-peers'].setValues(null, fillDefaults);
        }
        if (values && values['unexpected-peers'] != null) {
            this['unexpected-peers'] = values['unexpected-peers'];
        } else if (fillDefaults && RoutingHealthStatus.hasDefaultValue('unexpected-peers')) {
            this['unexpected-peers'] = RoutingHealthStatus.propInfo['unexpected-peers'].default;
        } else {
            this['unexpected-peers'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'router-id': CustomFormControl(new FormControl(this['router-id']), RoutingHealthStatus.propInfo['router-id']),
                'internal-peers': CustomFormGroup(this['internal-peers'].$formGroup, RoutingHealthStatus.propInfo['internal-peers'].required),
                'external-peers': CustomFormGroup(this['external-peers'].$formGroup, RoutingHealthStatus.propInfo['external-peers'].required),
                'unexpected-peers': CustomFormControl(new FormControl(this['unexpected-peers']), RoutingHealthStatus.propInfo['unexpected-peers']),
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('internal-peers') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('internal-peers').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('external-peers') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('external-peers').get(field);
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
            this._formGroup.controls['router-id'].setValue(this['router-id']);
            this['internal-peers'].setFormGroupValuesToBeModelValues();
            this['external-peers'].setFormGroupValuesToBeModelValues();
            this._formGroup.controls['unexpected-peers'].setValue(this['unexpected-peers']);
        }
    }
}

