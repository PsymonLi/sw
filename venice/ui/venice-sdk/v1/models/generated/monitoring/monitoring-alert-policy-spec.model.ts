/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { MonitoringAlertPolicySpec_severity,  MonitoringAlertPolicySpec_severity_uihint  } from './enums';
import { FieldsRequirement, IFieldsRequirement } from './fields-requirement.model';

export interface IMonitoringAlertPolicySpec {
    'resource'?: string;
    'severity': MonitoringAlertPolicySpec_severity;
    'message'?: string;
    'requirements'?: Array<IFieldsRequirement>;
    'persistence-duration'?: string;
    'clear-duration'?: string;
    'enable'?: boolean;
    'auto-resolve'?: boolean;
    'destinations'?: Array<string>;
}


export class MonitoringAlertPolicySpec extends BaseModel implements IMonitoringAlertPolicySpec {
    /** Resource type - target resource to run this policy.
    e.g. Network, Endpoint - object based alert policy
         Event - event based alert policy
         EndpointMetrics - metric based alert policy
    based on the resource type, the policy gets interpreted. */
    'resource': string = null;
    'severity': MonitoringAlertPolicySpec_severity = null;
    /** Message to be used while generating the alert
    XXX: Event based alerts should not carry a message. It will be derived from the event. */
    'message': string = null;
    'requirements': Array<FieldsRequirement> = null;
    'persistence-duration': string = null;
    'clear-duration': string = null;
    /** User can disable the policy by setting this field.
    Disabled policies will not generate any more alerts but the outstanding ones will remain as is. */
    'enable': boolean = null;
    'auto-resolve': boolean = null;
    /** name of the alert destinations to be used to send out notification when an alert
    gets generated. */
    'destinations': Array<string> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'resource': {
            description:  'Resource type - target resource to run this policy. e.g. Network, Endpoint - object based alert policy      Event - event based alert policy      EndpointMetrics - metric based alert policy based on the resource type, the policy gets interpreted.',
            type: 'string'
        },
        'severity': {
            enum: MonitoringAlertPolicySpec_severity_uihint,
            default: 'INFO',
            type: 'string'
        },
        'message': {
            description:  'Message to be used while generating the alert XXX: Event based alerts should not carry a message. It will be derived from the event.',
            type: 'string'
        },
        'requirements': {
            type: 'object'
        },
        'persistence-duration': {
            type: 'string'
        },
        'clear-duration': {
            type: 'string'
        },
        'enable': {
            default: 'true',
            description:  'User can disable the policy by setting this field. Disabled policies will not generate any more alerts but the outstanding ones will remain as is.',
            type: 'boolean'
        },
        'auto-resolve': {
            type: 'boolean'
        },
        'destinations': {
            description:  'name of the alert destinations to be used to send out notification when an alert gets generated.',
            type: 'Array<string>'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return MonitoringAlertPolicySpec.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return MonitoringAlertPolicySpec.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (MonitoringAlertPolicySpec.propInfo[prop] != null &&
                        MonitoringAlertPolicySpec.propInfo[prop].default != null &&
                        MonitoringAlertPolicySpec.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['requirements'] = new Array<FieldsRequirement>();
        this['destinations'] = new Array<string>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['resource'] != null) {
            this['resource'] = values['resource'];
        } else if (fillDefaults && MonitoringAlertPolicySpec.hasDefaultValue('resource')) {
            this['resource'] = MonitoringAlertPolicySpec.propInfo['resource'].default;
        } else {
            this['resource'] = null
        }
        if (values && values['severity'] != null) {
            this['severity'] = values['severity'];
        } else if (fillDefaults && MonitoringAlertPolicySpec.hasDefaultValue('severity')) {
            this['severity'] = <MonitoringAlertPolicySpec_severity>  MonitoringAlertPolicySpec.propInfo['severity'].default;
        } else {
            this['severity'] = null
        }
        if (values && values['message'] != null) {
            this['message'] = values['message'];
        } else if (fillDefaults && MonitoringAlertPolicySpec.hasDefaultValue('message')) {
            this['message'] = MonitoringAlertPolicySpec.propInfo['message'].default;
        } else {
            this['message'] = null
        }
        if (values) {
            this.fillModelArray<FieldsRequirement>(this, 'requirements', values['requirements'], FieldsRequirement);
        } else {
            this['requirements'] = [];
        }
        if (values && values['persistence-duration'] != null) {
            this['persistence-duration'] = values['persistence-duration'];
        } else if (fillDefaults && MonitoringAlertPolicySpec.hasDefaultValue('persistence-duration')) {
            this['persistence-duration'] = MonitoringAlertPolicySpec.propInfo['persistence-duration'].default;
        } else {
            this['persistence-duration'] = null
        }
        if (values && values['clear-duration'] != null) {
            this['clear-duration'] = values['clear-duration'];
        } else if (fillDefaults && MonitoringAlertPolicySpec.hasDefaultValue('clear-duration')) {
            this['clear-duration'] = MonitoringAlertPolicySpec.propInfo['clear-duration'].default;
        } else {
            this['clear-duration'] = null
        }
        if (values && values['enable'] != null) {
            this['enable'] = values['enable'];
        } else if (fillDefaults && MonitoringAlertPolicySpec.hasDefaultValue('enable')) {
            this['enable'] = MonitoringAlertPolicySpec.propInfo['enable'].default;
        } else {
            this['enable'] = null
        }
        if (values && values['auto-resolve'] != null) {
            this['auto-resolve'] = values['auto-resolve'];
        } else if (fillDefaults && MonitoringAlertPolicySpec.hasDefaultValue('auto-resolve')) {
            this['auto-resolve'] = MonitoringAlertPolicySpec.propInfo['auto-resolve'].default;
        } else {
            this['auto-resolve'] = null
        }
        if (values && values['destinations'] != null) {
            this['destinations'] = values['destinations'];
        } else if (fillDefaults && MonitoringAlertPolicySpec.hasDefaultValue('destinations')) {
            this['destinations'] = [ MonitoringAlertPolicySpec.propInfo['destinations'].default];
        } else {
            this['destinations'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'resource': CustomFormControl(new FormControl(this['resource']), MonitoringAlertPolicySpec.propInfo['resource'].description),
                'severity': CustomFormControl(new FormControl(this['severity'], [required, enumValidator(MonitoringAlertPolicySpec_severity), ]), MonitoringAlertPolicySpec.propInfo['severity'].description),
                'message': CustomFormControl(new FormControl(this['message']), MonitoringAlertPolicySpec.propInfo['message'].description),
                'requirements': new FormArray([]),
                'persistence-duration': CustomFormControl(new FormControl(this['persistence-duration']), MonitoringAlertPolicySpec.propInfo['persistence-duration'].description),
                'clear-duration': CustomFormControl(new FormControl(this['clear-duration']), MonitoringAlertPolicySpec.propInfo['clear-duration'].description),
                'enable': CustomFormControl(new FormControl(this['enable']), MonitoringAlertPolicySpec.propInfo['enable'].description),
                'auto-resolve': CustomFormControl(new FormControl(this['auto-resolve']), MonitoringAlertPolicySpec.propInfo['auto-resolve'].description),
                'destinations': CustomFormControl(new FormControl(this['destinations']), MonitoringAlertPolicySpec.propInfo['destinations'].description),
            });
            // generate FormArray control elements
            this.fillFormArray<FieldsRequirement>('requirements', this['requirements'], FieldsRequirement);
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['resource'].setValue(this['resource']);
            this._formGroup.controls['severity'].setValue(this['severity']);
            this._formGroup.controls['message'].setValue(this['message']);
            this.fillModelArray<FieldsRequirement>(this, 'requirements', this['requirements'], FieldsRequirement);
            this._formGroup.controls['persistence-duration'].setValue(this['persistence-duration']);
            this._formGroup.controls['clear-duration'].setValue(this['clear-duration']);
            this._formGroup.controls['enable'].setValue(this['enable']);
            this._formGroup.controls['auto-resolve'].setValue(this['auto-resolve']);
            this._formGroup.controls['destinations'].setValue(this['destinations']);
        }
    }
}

