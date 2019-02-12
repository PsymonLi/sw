/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { FieldsSelector, IFieldsSelector } from './fields-selector.model';
import { MonitoringEmailExport, IMonitoringEmailExport } from './monitoring-email-export.model';
import { MonitoringSNMPExport, IMonitoringSNMPExport } from './monitoring-snmp-export.model';
import { MonitoringSyslogExport, IMonitoringSyslogExport } from './monitoring-syslog-export.model';

export interface IMonitoringAlertDestinationSpec {
    'selector'?: IFieldsSelector;
    'email-export'?: IMonitoringEmailExport;
    'snmp-export'?: IMonitoringSNMPExport;
    'syslog-export'?: IMonitoringSyslogExport;
}


export class MonitoringAlertDestinationSpec extends BaseModel implements IMonitoringAlertDestinationSpec {
    'selector': FieldsSelector = null;
    'email-export': MonitoringEmailExport = null;
    'snmp-export': MonitoringSNMPExport = null;
    'syslog-export': MonitoringSyslogExport = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'selector': {
            type: 'object'
        },
        'email-export': {
            type: 'object'
        },
        'snmp-export': {
            type: 'object'
        },
        'syslog-export': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return MonitoringAlertDestinationSpec.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return MonitoringAlertDestinationSpec.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (MonitoringAlertDestinationSpec.propInfo[prop] != null &&
                        MonitoringAlertDestinationSpec.propInfo[prop].default != null &&
                        MonitoringAlertDestinationSpec.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['selector'] = new FieldsSelector();
        this['email-export'] = new MonitoringEmailExport();
        this['snmp-export'] = new MonitoringSNMPExport();
        this['syslog-export'] = new MonitoringSyslogExport();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values) {
            this['selector'].setValues(values['selector'], fillDefaults);
        } else {
            this['selector'].setValues(null, fillDefaults);
        }
        if (values) {
            this['email-export'].setValues(values['email-export'], fillDefaults);
        } else {
            this['email-export'].setValues(null, fillDefaults);
        }
        if (values) {
            this['snmp-export'].setValues(values['snmp-export'], fillDefaults);
        } else {
            this['snmp-export'].setValues(null, fillDefaults);
        }
        if (values) {
            this['syslog-export'].setValues(values['syslog-export'], fillDefaults);
        } else {
            this['syslog-export'].setValues(null, fillDefaults);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'selector': this['selector'].$formGroup,
                'email-export': this['email-export'].$formGroup,
                'snmp-export': this['snmp-export'].$formGroup,
                'syslog-export': this['syslog-export'].$formGroup,
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this['selector'].setFormGroupValuesToBeModelValues();
            this['email-export'].setFormGroupValuesToBeModelValues();
            this['snmp-export'].setFormGroupValuesToBeModelValues();
            this['syslog-export'].setFormGroupValuesToBeModelValues();
        }
    }
}

