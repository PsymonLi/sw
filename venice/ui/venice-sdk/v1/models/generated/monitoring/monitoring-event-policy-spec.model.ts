/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { MonitoringEventPolicySpec_format,  MonitoringEventPolicySpec_format_uihint  } from './enums';
import { FieldsSelector, IFieldsSelector } from './fields-selector.model';
import { MonitoringExportConfig, IMonitoringExportConfig } from './monitoring-export-config.model';
import { MonitoringSyslogExportConfig, IMonitoringSyslogExportConfig } from './monitoring-syslog-export-config.model';

export interface IMonitoringEventPolicySpec {
    'format': MonitoringEventPolicySpec_format;
    'selector'?: IFieldsSelector;
    'targets'?: Array<IMonitoringExportConfig>;
    'config'?: IMonitoringSyslogExportConfig;
}


export class MonitoringEventPolicySpec extends BaseModel implements IMonitoringEventPolicySpec {
    'format': MonitoringEventPolicySpec_format = null;
    'selector': FieldsSelector = null;
    'targets': Array<MonitoringExportConfig> = null;
    'config': MonitoringSyslogExportConfig = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'format': {
            enum: MonitoringEventPolicySpec_format_uihint,
            default: 'SYSLOG_BSD',
            type: 'string'
        },
        'selector': {
            type: 'object'
        },
        'targets': {
            type: 'object'
        },
        'config': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return MonitoringEventPolicySpec.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return MonitoringEventPolicySpec.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (MonitoringEventPolicySpec.propInfo[prop] != null &&
                        MonitoringEventPolicySpec.propInfo[prop].default != null &&
                        MonitoringEventPolicySpec.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['selector'] = new FieldsSelector();
        this['targets'] = new Array<MonitoringExportConfig>();
        this['config'] = new MonitoringSyslogExportConfig();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['format'] != null) {
            this['format'] = values['format'];
        } else if (fillDefaults && MonitoringEventPolicySpec.hasDefaultValue('format')) {
            this['format'] = <MonitoringEventPolicySpec_format>  MonitoringEventPolicySpec.propInfo['format'].default;
        } else {
            this['format'] = null
        }
        if (values) {
            this['selector'].setValues(values['selector'], fillDefaults);
        } else {
            this['selector'].setValues(null, fillDefaults);
        }
        if (values) {
            this.fillModelArray<MonitoringExportConfig>(this, 'targets', values['targets'], MonitoringExportConfig);
        } else {
            this['targets'] = [];
        }
        if (values) {
            this['config'].setValues(values['config'], fillDefaults);
        } else {
            this['config'].setValues(null, fillDefaults);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'format': CustomFormControl(new FormControl(this['format'], [required, enumValidator(MonitoringEventPolicySpec_format), ]), MonitoringEventPolicySpec.propInfo['format'].description),
                'selector': this['selector'].$formGroup,
                'targets': new FormArray([]),
                'config': this['config'].$formGroup,
            });
            // generate FormArray control elements
            this.fillFormArray<MonitoringExportConfig>('targets', this['targets'], MonitoringExportConfig);
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['format'].setValue(this['format']);
            this['selector'].setFormGroupValuesToBeModelValues();
            this.fillModelArray<MonitoringExportConfig>(this, 'targets', this['targets'], MonitoringExportConfig);
            this['config'].setFormGroupValuesToBeModelValues();
        }
    }
}

