/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { MonitoringFlowExportPolicySpec_format,  } from './enums';
import { MonitoringMatchRule, IMonitoringMatchRule } from './monitoring-match-rule.model';
import { MonitoringExportConfig, IMonitoringExportConfig } from './monitoring-export-config.model';

export interface IMonitoringFlowExportPolicySpec {
    'vrf-name'?: string;
    'interval': string;
    'template-interval': string;
    'format': MonitoringFlowExportPolicySpec_format;
    'match-rules'?: Array<IMonitoringMatchRule>;
    'exports'?: Array<IMonitoringExportConfig>;
}


export class MonitoringFlowExportPolicySpec extends BaseModel implements IMonitoringFlowExportPolicySpec {
    /** VrfName specifies the name of the VRF that the current flow export Policy belongs to */
    'vrf-name': string = null;
    /** Interval defines how often to push the records to an external collector
The value is specified as a string format, '10s', '20m'should be a valid time duration between 1s and 24h0m0s */
    'interval': string = null;
    /** TemplateInterval defines how often to send ipfix templates to an external collector
The value is specified as a string format, '1m', '10m'should be a valid time duration between 1m0s and 30m0s */
    'template-interval': string = null;
    'format': MonitoringFlowExportPolicySpec_format = null;
    'match-rules': Array<MonitoringMatchRule> = null;
    /** Export contains export parameters. */
    'exports': Array<MonitoringExportConfig> = null;
    public static propInfo: { [prop in keyof IMonitoringFlowExportPolicySpec]: PropInfoItem } = {
        'vrf-name': {
            description:  `VrfName specifies the name of the VRF that the current flow export Policy belongs to`,
            required: false,
            type: 'string'
        },
        'interval': {
            default: '10s',
            description:  `Interval defines how often to push the records to an external collector The value is specified as a string format, '10s', '20m'should be a valid time duration between 1s and 24h0m0s`,
            hint:  '2h',
            required: true,
            type: 'string'
        },
        'template-interval': {
            default: '5m',
            description:  `TemplateInterval defines how often to send ipfix templates to an external collector The value is specified as a string format, '1m', '10m'should be a valid time duration between 1m0s and 30m0s`,
            hint:  '2h',
            required: true,
            type: 'string'
        },
        'format': {
            enum: MonitoringFlowExportPolicySpec_format,
            default: 'ipfix',
            required: true,
            type: 'string'
        },
        'match-rules': {
            required: false,
            type: 'object'
        },
        'exports': {
            description:  `Export contains export parameters.`,
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return MonitoringFlowExportPolicySpec.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return MonitoringFlowExportPolicySpec.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (MonitoringFlowExportPolicySpec.propInfo[prop] != null &&
                        MonitoringFlowExportPolicySpec.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['match-rules'] = new Array<MonitoringMatchRule>();
        this['exports'] = new Array<MonitoringExportConfig>();
        this._inputValue = values;
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['vrf-name'] != null) {
            this['vrf-name'] = values['vrf-name'];
        } else if (fillDefaults && MonitoringFlowExportPolicySpec.hasDefaultValue('vrf-name')) {
            this['vrf-name'] = MonitoringFlowExportPolicySpec.propInfo['vrf-name'].default;
        } else {
            this['vrf-name'] = null
        }
        if (values && values['interval'] != null) {
            this['interval'] = values['interval'];
        } else if (fillDefaults && MonitoringFlowExportPolicySpec.hasDefaultValue('interval')) {
            this['interval'] = MonitoringFlowExportPolicySpec.propInfo['interval'].default;
        } else {
            this['interval'] = null
        }
        if (values && values['template-interval'] != null) {
            this['template-interval'] = values['template-interval'];
        } else if (fillDefaults && MonitoringFlowExportPolicySpec.hasDefaultValue('template-interval')) {
            this['template-interval'] = MonitoringFlowExportPolicySpec.propInfo['template-interval'].default;
        } else {
            this['template-interval'] = null
        }
        if (values && values['format'] != null) {
            this['format'] = values['format'];
        } else if (fillDefaults && MonitoringFlowExportPolicySpec.hasDefaultValue('format')) {
            this['format'] = <MonitoringFlowExportPolicySpec_format>  MonitoringFlowExportPolicySpec.propInfo['format'].default;
        } else {
            this['format'] = null
        }
        if (values) {
            this.fillModelArray<MonitoringMatchRule>(this, 'match-rules', values['match-rules'], MonitoringMatchRule);
        } else {
            this['match-rules'] = [];
        }
        if (values) {
            this.fillModelArray<MonitoringExportConfig>(this, 'exports', values['exports'], MonitoringExportConfig);
        } else {
            this['exports'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'vrf-name': CustomFormControl(new FormControl(this['vrf-name']), MonitoringFlowExportPolicySpec.propInfo['vrf-name']),
                'interval': CustomFormControl(new FormControl(this['interval'], [required, ]), MonitoringFlowExportPolicySpec.propInfo['interval']),
                'template-interval': CustomFormControl(new FormControl(this['template-interval'], [required, ]), MonitoringFlowExportPolicySpec.propInfo['template-interval']),
                'format': CustomFormControl(new FormControl(this['format'], [required, enumValidator(MonitoringFlowExportPolicySpec_format), ]), MonitoringFlowExportPolicySpec.propInfo['format']),
                'match-rules': new FormArray([]),
                'exports': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<MonitoringMatchRule>('match-rules', this['match-rules'], MonitoringMatchRule);
            // generate FormArray control elements
            this.fillFormArray<MonitoringExportConfig>('exports', this['exports'], MonitoringExportConfig);
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('match-rules') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('match-rules').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('exports') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('exports').get(field);
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
            this._formGroup.controls['vrf-name'].setValue(this['vrf-name']);
            this._formGroup.controls['interval'].setValue(this['interval']);
            this._formGroup.controls['template-interval'].setValue(this['template-interval']);
            this._formGroup.controls['format'].setValue(this['format']);
            this.fillModelArray<MonitoringMatchRule>(this, 'match-rules', this['match-rules'], MonitoringMatchRule);
            this.fillModelArray<MonitoringExportConfig>(this, 'exports', this['exports'], MonitoringExportConfig);
        }
    }
}

