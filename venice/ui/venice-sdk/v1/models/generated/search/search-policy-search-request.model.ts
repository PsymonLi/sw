/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';


export interface ISearchPolicySearchRequest {
    'tenant'?: string;
    'namespace'?: string;
    'sg-policy'?: string;
    'app'?: string;
    'protocol'?: string;
    'port'?: string;
    'from-ip-address'?: string;
    'to-ip-address'?: string;
    'from-security-group'?: string;
    'to-security-group'?: string;
}


export class SearchPolicySearchRequest extends BaseModel implements ISearchPolicySearchRequest {
    /** Tenant Name, to perform query within a Tenant's scope.
The default tenant is "default". In the backend
this field gets auto-filled & validated by apigw-hook
based on user login context. */
    'tenant': string = null;
    /** Namespace is optional. If provided policy-search will
be limited to the specified namespace. */
    'namespace': string = null;
    /** NetworkSecurityPolicy name is optional. If provided policy-search will
be limited to the specified SGpolicy object name. */
    'sg-policy': string = null;
    /** App specification,  predefined apps and alg config */
    'app': string = null;
    /** Protocol eg: tcp, udp, icmp */
    'protocol': string = null;
    /** TCP or UDP Port number */
    'port': string = null;
    /** Inbound ip-address, use any to refer to all ipaddresses
eg: 10.1.1.1, any */
    'from-ip-address': string = null;
    /** Outbound ip-address, use any to refer to all ipaddresses
eg: 20.1.1.1, any */
    'to-ip-address': string = null;
    /** Inbound security group */
    'from-security-group': string = null;
    /** Outbound security group */
    'to-security-group': string = null;
    public static propInfo: { [prop in keyof ISearchPolicySearchRequest]: PropInfoItem } = {
        'tenant': {
            default: 'default',
            description:  `Tenant Name, to perform query within a Tenant's scope. The default tenant is "default". In the backend this field gets auto-filled & validated by apigw-hook based on user login context.`,
            required: false,
            type: 'string'
        },
        'namespace': {
            default: 'default',
            description:  `Namespace is optional. If provided policy-search will be limited to the specified namespace.`,
            required: false,
            type: 'string'
        },
        'sg-policy': {
            description:  `NetworkSecurityPolicy name is optional. If provided policy-search will be limited to the specified SGpolicy object name.`,
            required: false,
            type: 'string'
        },
        'app': {
            description:  `App specification,  predefined apps and alg config`,
            required: false,
            type: 'string'
        },
        'protocol': {
            description:  `Protocol eg: tcp, udp, icmp`,
            required: false,
            type: 'string'
        },
        'port': {
            description:  `TCP or UDP Port number`,
            required: false,
            type: 'string'
        },
        'from-ip-address': {
            description:  `Inbound ip-address, use any to refer to all ipaddresses eg: 10.1.1.1, any`,
            required: false,
            type: 'string'
        },
        'to-ip-address': {
            description:  `Outbound ip-address, use any to refer to all ipaddresses eg: 20.1.1.1, any`,
            required: false,
            type: 'string'
        },
        'from-security-group': {
            description:  `Inbound security group`,
            required: false,
            type: 'string'
        },
        'to-security-group': {
            description:  `Outbound security group`,
            required: false,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return SearchPolicySearchRequest.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return SearchPolicySearchRequest.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (SearchPolicySearchRequest.propInfo[prop] != null &&
                        SearchPolicySearchRequest.propInfo[prop].default != null);
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
        if (values && values['tenant'] != null) {
            this['tenant'] = values['tenant'];
        } else if (fillDefaults && SearchPolicySearchRequest.hasDefaultValue('tenant')) {
            this['tenant'] = SearchPolicySearchRequest.propInfo['tenant'].default;
        } else {
            this['tenant'] = null
        }
        if (values && values['namespace'] != null) {
            this['namespace'] = values['namespace'];
        } else if (fillDefaults && SearchPolicySearchRequest.hasDefaultValue('namespace')) {
            this['namespace'] = SearchPolicySearchRequest.propInfo['namespace'].default;
        } else {
            this['namespace'] = null
        }
        if (values && values['sg-policy'] != null) {
            this['sg-policy'] = values['sg-policy'];
        } else if (fillDefaults && SearchPolicySearchRequest.hasDefaultValue('sg-policy')) {
            this['sg-policy'] = SearchPolicySearchRequest.propInfo['sg-policy'].default;
        } else {
            this['sg-policy'] = null
        }
        if (values && values['app'] != null) {
            this['app'] = values['app'];
        } else if (fillDefaults && SearchPolicySearchRequest.hasDefaultValue('app')) {
            this['app'] = SearchPolicySearchRequest.propInfo['app'].default;
        } else {
            this['app'] = null
        }
        if (values && values['protocol'] != null) {
            this['protocol'] = values['protocol'];
        } else if (fillDefaults && SearchPolicySearchRequest.hasDefaultValue('protocol')) {
            this['protocol'] = SearchPolicySearchRequest.propInfo['protocol'].default;
        } else {
            this['protocol'] = null
        }
        if (values && values['port'] != null) {
            this['port'] = values['port'];
        } else if (fillDefaults && SearchPolicySearchRequest.hasDefaultValue('port')) {
            this['port'] = SearchPolicySearchRequest.propInfo['port'].default;
        } else {
            this['port'] = null
        }
        if (values && values['from-ip-address'] != null) {
            this['from-ip-address'] = values['from-ip-address'];
        } else if (fillDefaults && SearchPolicySearchRequest.hasDefaultValue('from-ip-address')) {
            this['from-ip-address'] = SearchPolicySearchRequest.propInfo['from-ip-address'].default;
        } else {
            this['from-ip-address'] = null
        }
        if (values && values['to-ip-address'] != null) {
            this['to-ip-address'] = values['to-ip-address'];
        } else if (fillDefaults && SearchPolicySearchRequest.hasDefaultValue('to-ip-address')) {
            this['to-ip-address'] = SearchPolicySearchRequest.propInfo['to-ip-address'].default;
        } else {
            this['to-ip-address'] = null
        }
        if (values && values['from-security-group'] != null) {
            this['from-security-group'] = values['from-security-group'];
        } else if (fillDefaults && SearchPolicySearchRequest.hasDefaultValue('from-security-group')) {
            this['from-security-group'] = SearchPolicySearchRequest.propInfo['from-security-group'].default;
        } else {
            this['from-security-group'] = null
        }
        if (values && values['to-security-group'] != null) {
            this['to-security-group'] = values['to-security-group'];
        } else if (fillDefaults && SearchPolicySearchRequest.hasDefaultValue('to-security-group')) {
            this['to-security-group'] = SearchPolicySearchRequest.propInfo['to-security-group'].default;
        } else {
            this['to-security-group'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'tenant': CustomFormControl(new FormControl(this['tenant']), SearchPolicySearchRequest.propInfo['tenant']),
                'namespace': CustomFormControl(new FormControl(this['namespace']), SearchPolicySearchRequest.propInfo['namespace']),
                'sg-policy': CustomFormControl(new FormControl(this['sg-policy']), SearchPolicySearchRequest.propInfo['sg-policy']),
                'app': CustomFormControl(new FormControl(this['app']), SearchPolicySearchRequest.propInfo['app']),
                'protocol': CustomFormControl(new FormControl(this['protocol']), SearchPolicySearchRequest.propInfo['protocol']),
                'port': CustomFormControl(new FormControl(this['port']), SearchPolicySearchRequest.propInfo['port']),
                'from-ip-address': CustomFormControl(new FormControl(this['from-ip-address']), SearchPolicySearchRequest.propInfo['from-ip-address']),
                'to-ip-address': CustomFormControl(new FormControl(this['to-ip-address']), SearchPolicySearchRequest.propInfo['to-ip-address']),
                'from-security-group': CustomFormControl(new FormControl(this['from-security-group']), SearchPolicySearchRequest.propInfo['from-security-group']),
                'to-security-group': CustomFormControl(new FormControl(this['to-security-group']), SearchPolicySearchRequest.propInfo['to-security-group']),
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
            this._formGroup.controls['namespace'].setValue(this['namespace']);
            this._formGroup.controls['sg-policy'].setValue(this['sg-policy']);
            this._formGroup.controls['app'].setValue(this['app']);
            this._formGroup.controls['protocol'].setValue(this['protocol']);
            this._formGroup.controls['port'].setValue(this['port']);
            this._formGroup.controls['from-ip-address'].setValue(this['from-ip-address']);
            this._formGroup.controls['to-ip-address'].setValue(this['to-ip-address']);
            this._formGroup.controls['from-security-group'].setValue(this['from-security-group']);
            this._formGroup.controls['to-security-group'].setValue(this['to-security-group']);
        }
    }
}

