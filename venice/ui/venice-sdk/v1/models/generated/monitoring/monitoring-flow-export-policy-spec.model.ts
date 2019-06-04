/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { MonitoringFlowExportPolicySpec_format,  } from './enums';
import { MonitoringMatchRule, IMonitoringMatchRule } from './monitoring-match-rule.model';
import { MonitoringExportConfig, IMonitoringExportConfig } from './monitoring-export-config.model';

export interface IMonitoringFlowExportPolicySpec {
    'vrf-name'?: string;
    'interval': string;
    'template_interval': string;
    'format': MonitoringFlowExportPolicySpec_format;
    'match-rules'?: Array<IMonitoringMatchRule>;
    'exports'?: Array<IMonitoringExportConfig>;
}


export class MonitoringFlowExportPolicySpec extends BaseModel implements IMonitoringFlowExportPolicySpec {
    'vrf-name': string = null;
    /** should be a valid time duration between 1s and 24h0m0s */
    'interval': string = null;
    /** should be a valid time duration between 3m0s and 24h0m0s */
    'template_interval': string = null;
    'format': MonitoringFlowExportPolicySpec_format = null;
    'match-rules': Array<MonitoringMatchRule> = null;
    /** Export contains export parameters. */
    'exports': Array<MonitoringExportConfig> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'vrf-name': {
            required: false,
            type: 'string'
        },
        'interval': {
            default: '10s',
            description:  'should be a valid time duration between 1s and 24h0m0s',
            hint:  '2h',
            required: true,
            type: 'string'
        },
        'template_interval': {
            default: '3m',
            description:  'should be a valid time duration between 3m0s and 24h0m0s',
            hint:  '2h',
            required: true,
            type: 'string'
        },
        'format': {
            enum: MonitoringFlowExportPolicySpec_format,
            default: 'Ipfix',
            required: true,
            type: 'string'
        },
        'match-rules': {
            required: false,
            type: 'object'
        },
        'exports': {
            description:  'Export contains export parameters.',
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
        if (values && values['template_interval'] != null) {
            this['template_interval'] = values['template_interval'];
        } else if (fillDefaults && MonitoringFlowExportPolicySpec.hasDefaultValue('template_interval')) {
            this['template_interval'] = MonitoringFlowExportPolicySpec.propInfo['template_interval'].default;
        } else {
            this['template_interval'] = null
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
                'template_interval': CustomFormControl(new FormControl(this['template_interval'], [required, ]), MonitoringFlowExportPolicySpec.propInfo['template_interval']),
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
            this._formGroup.controls['template_interval'].setValue(this['template_interval']);
            this._formGroup.controls['format'].setValue(this['format']);
            this.fillModelArray<MonitoringMatchRule>(this, 'match-rules', this['match-rules'], MonitoringMatchRule);
            this.fillModelArray<MonitoringExportConfig>(this, 'exports', this['exports'], MonitoringExportConfig);
        }
    }
}

