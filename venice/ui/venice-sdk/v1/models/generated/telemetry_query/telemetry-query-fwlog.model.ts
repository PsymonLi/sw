/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { Telemetry_queryFwlog_action,  Telemetry_queryFwlog_action_uihint  } from './enums';
import { Telemetry_queryFwlog_direction,  Telemetry_queryFwlog_direction_uihint  } from './enums';

export interface ITelemetry_queryFwlog {
    'tenant'?: string;
    'src'?: string;
    'dest'?: string;
    'src-port'?: number;
    'dest-port'?: number;
    'protocol'?: string;
    'action': Telemetry_queryFwlog_action;
    'direction': Telemetry_queryFwlog_direction;
    'rule-id'?: string;
    'policy-name'?: string;
    'timestamp'?: Date;
}


export class Telemetry_queryFwlog extends BaseModel implements ITelemetry_queryFwlog {
    'tenant': string = null;
    'src': string = null;
    'dest': string = null;
    'src-port': number = null;
    'dest-port': number = null;
    'protocol': string = null;
    'action': Telemetry_queryFwlog_action = null;
    'direction': Telemetry_queryFwlog_direction = null;
    'rule-id': string = null;
    'policy-name': string = null;
    'timestamp': Date = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'tenant': {
            type: 'string'
        },
        'src': {
            type: 'string'
        },
        'dest': {
            type: 'string'
        },
        'src-port': {
            type: 'number'
        },
        'dest-port': {
            type: 'number'
        },
        'protocol': {
            type: 'string'
        },
        'action': {
            enum: Telemetry_queryFwlog_action_uihint,
            default: 'ALLOW',
            type: 'string'
        },
        'direction': {
            enum: Telemetry_queryFwlog_direction_uihint,
            default: 'FROM_HOST',
            type: 'string'
        },
        'rule-id': {
            type: 'string'
        },
        'policy-name': {
            type: 'string'
        },
        'timestamp': {
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
                        Telemetry_queryFwlog.propInfo[prop].default != null &&
                        Telemetry_queryFwlog.propInfo[prop].default != '');
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
        if (values && values['src'] != null) {
            this['src'] = values['src'];
        } else if (fillDefaults && Telemetry_queryFwlog.hasDefaultValue('src')) {
            this['src'] = Telemetry_queryFwlog.propInfo['src'].default;
        } else {
            this['src'] = null
        }
        if (values && values['dest'] != null) {
            this['dest'] = values['dest'];
        } else if (fillDefaults && Telemetry_queryFwlog.hasDefaultValue('dest')) {
            this['dest'] = Telemetry_queryFwlog.propInfo['dest'].default;
        } else {
            this['dest'] = null
        }
        if (values && values['src-port'] != null) {
            this['src-port'] = values['src-port'];
        } else if (fillDefaults && Telemetry_queryFwlog.hasDefaultValue('src-port')) {
            this['src-port'] = Telemetry_queryFwlog.propInfo['src-port'].default;
        } else {
            this['src-port'] = null
        }
        if (values && values['dest-port'] != null) {
            this['dest-port'] = values['dest-port'];
        } else if (fillDefaults && Telemetry_queryFwlog.hasDefaultValue('dest-port')) {
            this['dest-port'] = Telemetry_queryFwlog.propInfo['dest-port'].default;
        } else {
            this['dest-port'] = null
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
                'tenant': CustomFormControl(new FormControl(this['tenant']), Telemetry_queryFwlog.propInfo['tenant'].description),
                'src': CustomFormControl(new FormControl(this['src']), Telemetry_queryFwlog.propInfo['src'].description),
                'dest': CustomFormControl(new FormControl(this['dest']), Telemetry_queryFwlog.propInfo['dest'].description),
                'src-port': CustomFormControl(new FormControl(this['src-port']), Telemetry_queryFwlog.propInfo['src-port'].description),
                'dest-port': CustomFormControl(new FormControl(this['dest-port']), Telemetry_queryFwlog.propInfo['dest-port'].description),
                'protocol': CustomFormControl(new FormControl(this['protocol']), Telemetry_queryFwlog.propInfo['protocol'].description),
                'action': CustomFormControl(new FormControl(this['action'], [required, enumValidator(Telemetry_queryFwlog_action), ]), Telemetry_queryFwlog.propInfo['action'].description),
                'direction': CustomFormControl(new FormControl(this['direction'], [required, enumValidator(Telemetry_queryFwlog_direction), ]), Telemetry_queryFwlog.propInfo['direction'].description),
                'rule-id': CustomFormControl(new FormControl(this['rule-id']), Telemetry_queryFwlog.propInfo['rule-id'].description),
                'policy-name': CustomFormControl(new FormControl(this['policy-name']), Telemetry_queryFwlog.propInfo['policy-name'].description),
                'timestamp': CustomFormControl(new FormControl(this['timestamp']), Telemetry_queryFwlog.propInfo['timestamp'].description),
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
            this._formGroup.controls['src'].setValue(this['src']);
            this._formGroup.controls['dest'].setValue(this['dest']);
            this._formGroup.controls['src-port'].setValue(this['src-port']);
            this._formGroup.controls['dest-port'].setValue(this['dest-port']);
            this._formGroup.controls['protocol'].setValue(this['protocol']);
            this._formGroup.controls['action'].setValue(this['action']);
            this._formGroup.controls['direction'].setValue(this['direction']);
            this._formGroup.controls['rule-id'].setValue(this['rule-id']);
            this._formGroup.controls['policy-name'].setValue(this['policy-name']);
            this._formGroup.controls['timestamp'].setValue(this['timestamp']);
        }
    }
}

