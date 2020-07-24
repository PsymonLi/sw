/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';


export interface ISecurityDns {
    'drop-multi-question-packets'?: boolean;
    'drop-large-domain-name-packets'?: boolean;
    'drop-long-label-packets'?: boolean;
    'max-message-length'?: number;
    'query-response-timeout': string;
    '_ui'?: any;
}


export class SecurityDns extends BaseModel implements ISecurityDns {
    /** Field for holding arbitrary ui state */
    '_ui': any = {};
    /** Drop packet if number of questions is more than one. */
    'drop-multi-question-packets': boolean = null;
    /** Drop if domain name size is > 255 bytes. */
    'drop-large-domain-name-packets': boolean = null;
    /** Drop if label length is 64 bytes or higher. */
    'drop-long-label-packets': boolean = null;
    /** Maximum message length, default value is 512, maximum specified user value is 8129. */
    'max-message-length': number = null;
    /** Timeout for DNS Query, default 60s. Should be a valid time duration. */
    'query-response-timeout': string = null;
    public static propInfo: { [prop in keyof ISecurityDns]: PropInfoItem } = {
        'drop-multi-question-packets': {
            description:  `Drop packet if number of questions is more than one.`,
            required: false,
            type: 'boolean'
        },
        'drop-large-domain-name-packets': {
            description:  `Drop if domain name size is > 255 bytes.`,
            required: false,
            type: 'boolean'
        },
        'drop-long-label-packets': {
            description:  `Drop if label length is 64 bytes or higher.`,
            required: false,
            type: 'boolean'
        },
        'max-message-length': {
            description:  `Maximum message length, default value is 512, maximum specified user value is 8129.`,
            required: false,
            type: 'number'
        },
        'query-response-timeout': {
            default: '60s',
            description:  `Timeout for DNS Query, default 60s. Should be a valid time duration.`,
            hint:  '60s',
            required: true,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return SecurityDns.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return SecurityDns.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (SecurityDns.propInfo[prop] != null &&
                        SecurityDns.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
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
        if (values && values['drop-multi-question-packets'] != null) {
            this['drop-multi-question-packets'] = values['drop-multi-question-packets'];
        } else if (fillDefaults && SecurityDns.hasDefaultValue('drop-multi-question-packets')) {
            this['drop-multi-question-packets'] = SecurityDns.propInfo['drop-multi-question-packets'].default;
        } else {
            this['drop-multi-question-packets'] = null
        }
        if (values && values['drop-large-domain-name-packets'] != null) {
            this['drop-large-domain-name-packets'] = values['drop-large-domain-name-packets'];
        } else if (fillDefaults && SecurityDns.hasDefaultValue('drop-large-domain-name-packets')) {
            this['drop-large-domain-name-packets'] = SecurityDns.propInfo['drop-large-domain-name-packets'].default;
        } else {
            this['drop-large-domain-name-packets'] = null
        }
        if (values && values['drop-long-label-packets'] != null) {
            this['drop-long-label-packets'] = values['drop-long-label-packets'];
        } else if (fillDefaults && SecurityDns.hasDefaultValue('drop-long-label-packets')) {
            this['drop-long-label-packets'] = SecurityDns.propInfo['drop-long-label-packets'].default;
        } else {
            this['drop-long-label-packets'] = null
        }
        if (values && values['max-message-length'] != null) {
            this['max-message-length'] = values['max-message-length'];
        } else if (fillDefaults && SecurityDns.hasDefaultValue('max-message-length')) {
            this['max-message-length'] = SecurityDns.propInfo['max-message-length'].default;
        } else {
            this['max-message-length'] = null
        }
        if (values && values['query-response-timeout'] != null) {
            this['query-response-timeout'] = values['query-response-timeout'];
        } else if (fillDefaults && SecurityDns.hasDefaultValue('query-response-timeout')) {
            this['query-response-timeout'] = SecurityDns.propInfo['query-response-timeout'].default;
        } else {
            this['query-response-timeout'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'drop-multi-question-packets': CustomFormControl(new FormControl(this['drop-multi-question-packets']), SecurityDns.propInfo['drop-multi-question-packets']),
                'drop-large-domain-name-packets': CustomFormControl(new FormControl(this['drop-large-domain-name-packets']), SecurityDns.propInfo['drop-large-domain-name-packets']),
                'drop-long-label-packets': CustomFormControl(new FormControl(this['drop-long-label-packets']), SecurityDns.propInfo['drop-long-label-packets']),
                'max-message-length': CustomFormControl(new FormControl(this['max-message-length']), SecurityDns.propInfo['max-message-length']),
                'query-response-timeout': CustomFormControl(new FormControl(this['query-response-timeout'], [required, ]), SecurityDns.propInfo['query-response-timeout']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['drop-multi-question-packets'].setValue(this['drop-multi-question-packets']);
            this._formGroup.controls['drop-large-domain-name-packets'].setValue(this['drop-large-domain-name-packets']);
            this._formGroup.controls['drop-long-label-packets'].setValue(this['drop-long-label-packets']);
            this._formGroup.controls['max-message-length'].setValue(this['max-message-length']);
            this._formGroup.controls['query-response-timeout'].setValue(this['query-response-timeout']);
        }
    }
}

