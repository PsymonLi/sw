/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { ApiObjectMeta, IApiObjectMeta } from './api-object-meta.model';
import { MonitoringMirrorSessionSpec, IMonitoringMirrorSessionSpec } from './monitoring-mirror-session-spec.model';
import { MonitoringMirrorSessionStatus, IMonitoringMirrorSessionStatus } from './monitoring-mirror-session-status.model';

export interface IMonitoringMirrorSession {
    'kind'?: string;
    'api-version'?: string;
    'meta'?: IApiObjectMeta;
    'mirror-session-spec'?: IMonitoringMirrorSessionSpec;
    'status'?: IMonitoringMirrorSessionStatus;
}


export class MonitoringMirrorSession extends BaseModel implements IMonitoringMirrorSession {
    'kind': string = null;
    'api-version': string = null;
    'meta': ApiObjectMeta = null;
    'mirror-session-spec': MonitoringMirrorSessionSpec = null;
    'status': MonitoringMirrorSessionStatus = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
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
        'mirror-session-spec': {
            required: false,
            type: 'object'
        },
        'status': {
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return MonitoringMirrorSession.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return MonitoringMirrorSession.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (MonitoringMirrorSession.propInfo[prop] != null &&
                        MonitoringMirrorSession.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['meta'] = new ApiObjectMeta();
        this['mirror-session-spec'] = new MonitoringMirrorSessionSpec();
        this['status'] = new MonitoringMirrorSessionStatus();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['kind'] != null) {
            this['kind'] = values['kind'];
        } else if (fillDefaults && MonitoringMirrorSession.hasDefaultValue('kind')) {
            this['kind'] = MonitoringMirrorSession.propInfo['kind'].default;
        } else {
            this['kind'] = null
        }
        if (values && values['api-version'] != null) {
            this['api-version'] = values['api-version'];
        } else if (fillDefaults && MonitoringMirrorSession.hasDefaultValue('api-version')) {
            this['api-version'] = MonitoringMirrorSession.propInfo['api-version'].default;
        } else {
            this['api-version'] = null
        }
        if (values) {
            this['meta'].setValues(values['meta'], fillDefaults);
        } else {
            this['meta'].setValues(null, fillDefaults);
        }
        if (values) {
            this['mirror-session-spec'].setValues(values['mirror-session-spec'], fillDefaults);
        } else {
            this['mirror-session-spec'].setValues(null, fillDefaults);
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
                'kind': CustomFormControl(new FormControl(this['kind']), MonitoringMirrorSession.propInfo['kind']),
                'api-version': CustomFormControl(new FormControl(this['api-version']), MonitoringMirrorSession.propInfo['api-version']),
                'meta': CustomFormGroup(this['meta'].$formGroup, MonitoringMirrorSession.propInfo['meta'].required),
                'mirror-session-spec': CustomFormGroup(this['mirror-session-spec'].$formGroup, MonitoringMirrorSession.propInfo['mirror-session-spec'].required),
                'status': CustomFormGroup(this['status'].$formGroup, MonitoringMirrorSession.propInfo['status'].required),
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('meta') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('meta').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('mirror-session-spec') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('mirror-session-spec').get(field);
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
            this['mirror-session-spec'].setFormGroupValuesToBeModelValues();
            this['status'].setFormGroupValuesToBeModelValues();
        }
    }
}

