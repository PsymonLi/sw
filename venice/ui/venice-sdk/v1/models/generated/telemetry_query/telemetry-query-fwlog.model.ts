/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { Telemetry_queryFwlog_action,  Telemetry_queryFwlog_action_uihint  } from './enums';
import { Telemetry_queryFwlog_direction,  Telemetry_queryFwlog_direction_uihint  } from './enums';

export interface ITelemetry_queryFwlog {
    'tenant'?: string;
    'source'?: string;
    'destination'?: string;
    'source-port'?: number;
    'destination-port'?: number;
    'protocol'?: string;
    'action': Telemetry_queryFwlog_action;
    'direction': Telemetry_queryFwlog_direction;
    'rule-id'?: string;
    'policy-name'?: string;
    'timestamp'?: Date;
}


export class Telemetry_queryFwlog extends BaseModel implements ITelemetry_queryFwlog {
    'tenant': string = null;
    'source': string = null;
    'destination': string = null;
    'source-port': number = null;
    'destination-port': number = null;
    'protocol': string = null;
    'action': Telemetry_queryFwlog_action = null;
    'direction': Telemetry_queryFwlog_direction = null;
    'rule-id': string = null;
    'policy-name': string = null;
    'timestamp': Date = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'tenant': {
            required: false,
            type: 'string'
        },
        'source': {
            required: false,
            type: 'string'
        },
        'destination': {
            required: false,
            type: 'string'
        },
        'source-port': {
            required: false,
            type: 'number'
        },
        'destination-port': {
            required: false,
            type: 'number'
        },
        'protocol': {
            required: false,
            type: 'string'
        },
        'action': {
            enum: Telemetry_queryFwlog_action_uihint,
            default: 'ALLOW',
            required: true,
            type: 'string'
        },
        'direction': {
            enum: Telemetry_queryFwlog_direction_uihint,
            default: 'FROM_HOST',
            required: true,
            type: 'string'
        },
        'rule-id': {
            required: false,
            type: 'string'
        },
        'policy-name': {
            required: false,
            type: 'string'
        },
        'timestamp': {
            required: false,
            type: 'Date'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return Telemetry_queryFwlog.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return Telemetry_queryFwlog.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (Telemetry_queryFwlog.propInfo[prop] != null &&
                        Telemetry_queryFwlog.propInfo[prop].default != null);
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
        if (values && values['tenant'] != null) {
            this['tenant'] = values['tenant'];
        } else if (fillDefaults && Telemetry_queryFwlog.hasDefaultValue('tenant')) {
            this['tenant'] = Telemetry_queryFwlog.propInfo['tenant'].default;
        } else {
            this['tenant'] = null
        }
        if (values && values['source'] != null) {
            this['source'] = values['source'];
        } else if (fillDefaults && Telemetry_queryFwlog.hasDefaultValue('source')) {
            this['source'] = Telemetry_queryFwlog.propInfo['source'].default;
        } else {
            this['source'] = null
        }
        if (values && values['destination'] != null) {
            this['destination'] = values['destination'];
        } else if (fillDefaults && Telemetry_queryFwlog.hasDefaultValue('destination')) {
            this['destination'] = Telemetry_queryFwlog.propInfo['destination'].default;
        } else {
            this['destination'] = null
        }
        if (values && values['source-port'] != null) {
            this['source-port'] = values['source-port'];
        } else if (fillDefaults && Telemetry_queryFwlog.hasDefaultValue('source-port')) {
            this['source-port'] = Telemetry_queryFwlog.propInfo['source-port'].default;
        } else {
            this['source-port'] = null
        }
        if (values && values['destination-port'] != null) {
            this['destination-port'] = values['destination-port'];
        } else if (fillDefaults && Telemetry_queryFwlog.hasDefaultValue('destination-port')) {
            this['destination-port'] = Telemetry_queryFwlog.propInfo['destination-port'].default;
        } else {
            this['destination-port'] = null
        }
        if (values && values['protocol'] != null) {
            this['protocol'] = values['protocol'];
        } else if (fillDefaults && Telemetry_queryFwlog.hasDefaultValue('protocol')) {
            this['protocol'] = Telemetry_queryFwlog.propInfo['protocol'].default;
        } else {
            this['protocol'] = null
        }
        if (values && values['action'] != null) {
            this['action'] = values['action'];
        } else if (fillDefaults && Telemetry_queryFwlog.hasDefaultValue('action')) {
            this['action'] = <Telemetry_queryFwlog_action>  Telemetry_queryFwlog.propInfo['action'].default;
        } else {
            this['action'] = null
        }
        if (values && values['direction'] != null) {
            this['direction'] = values['direction'];
        } else if (fillDefaults && Telemetry_queryFwlog.hasDefaultValue('direction')) {
            this['direction'] = <Telemetry_queryFwlog_direction>  Telemetry_queryFwlog.propInfo['direction'].default;
        } else {
            this['direction'] = null
        }
        if (values && values['rule-id'] != null) {
            this['rule-id'] = values['rule-id'];
        } else if (fillDefaults && Telemetry_queryFwlog.hasDefaultValue('rule-id')) {
            this['rule-id'] = Telemetry_queryFwlog.propInfo['rule-id'].default;
        } else {
            this['rule-id'] = null
        }
        if (values && values['policy-name'] != null) {
            this['policy-name'] = values['policy-name'];
        } else if (fillDefaults && Telemetry_queryFwlog.hasDefaultValue('policy-name')) {
            this['policy-name'] = Telemetry_queryFwlog.propInfo['policy-name'].default;
        } else {
            this['policy-name'] = null
        }
        if (values && values['timestamp'] != null) {
            this['timestamp'] = values['timestamp'];
        } else if (fillDefaults && Telemetry_queryFwlog.hasDefaultValue('timestamp')) {
            this['timestamp'] = Telemetry_queryFwlog.propInfo['timestamp'].default;
        } else {
            this['timestamp'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'tenant': CustomFormControl(new FormControl(this['tenant']), Telemetry_queryFwlog.propInfo['tenant']),
                'source': CustomFormControl(new FormControl(this['source']), Telemetry_queryFwlog.propInfo['source']),
                'destination': CustomFormControl(new FormControl(this['destination']), Telemetry_queryFwlog.propInfo['destination']),
                'source-port': CustomFormControl(new FormControl(this['source-port']), Telemetry_queryFwlog.propInfo['source-port']),
                'destination-port': CustomFormControl(new FormControl(this['destination-port']), Telemetry_queryFwlog.propInfo['destination-port']),
                'protocol': CustomFormControl(new FormControl(this['protocol']), Telemetry_queryFwlog.propInfo['protocol']),
                'action': CustomFormControl(new FormControl(this['action'], [required, enumValidator(Telemetry_queryFwlog_action), ]), Telemetry_queryFwlog.propInfo['action']),
                'direction': CustomFormControl(new FormControl(this['direction'], [required, enumValidator(Telemetry_queryFwlog_direction), ]), Telemetry_queryFwlog.propInfo['direction']),
                'rule-id': CustomFormControl(new FormControl(this['rule-id']), Telemetry_queryFwlog.propInfo['rule-id']),
                'policy-name': CustomFormControl(new FormControl(this['policy-name']), Telemetry_queryFwlog.propInfo['policy-name']),
                'timestamp': CustomFormControl(new FormControl(this['timestamp']), Telemetry_queryFwlog.propInfo['timestamp']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['tenant'].setValue(this['tenant']);
            this._formGroup.controls['source'].setValue(this['source']);
            this._formGroup.controls['destination'].setValue(this['destination']);
            this._formGroup.controls['source-port'].setValue(this['source-port']);
            this._formGroup.controls['destination-port'].setValue(this['destination-port']);
            this._formGroup.controls['protocol'].setValue(this['protocol']);
            this._formGroup.controls['action'].setValue(this['action']);
            this._formGroup.controls['direction'].setValue(this['direction']);
            this._formGroup.controls['rule-id'].setValue(this['rule-id']);
            this._formGroup.controls['policy-name'].setValue(this['policy-name']);
            this._formGroup.controls['timestamp'].setValue(this['timestamp']);
        }
    }
}

