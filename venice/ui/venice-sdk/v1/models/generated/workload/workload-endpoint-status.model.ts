/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { WorkloadEndpointMigrationStatus, IWorkloadEndpointMigrationStatus } from './workload-endpoint-migration-status.model';

export interface IWorkloadEndpointStatus {
    'workload-name'?: string;
    'network'?: string;
    'homing-host-addr'?: string;
    'homing-host-name'?: string;
    'ipv4-address'?: string;
    'ipv4-gateway'?: string;
    'ipv6-address'?: string;
    'ipv6-gateway'?: string;
    'mac-address'?: string;
    'node-uuid'?: string;
    'EndpointState'?: string;
    'SecurityGroups'?: Array<string>;
    'micro-segment-vlan'?: number;
    'workload-attributes'?: object;
    'migration'?: IWorkloadEndpointMigrationStatus;
}


export class WorkloadEndpointStatus extends BaseModel implements IWorkloadEndpointStatus {
    /** VM or container name */
    'workload-name': string = null;
    /** network this endpoint belogs to */
    'network': string = null;
    /** host address of the host where this endpoint exists */
    'homing-host-addr': string = null;
    /** host name of the host where this endpoint exists */
    'homing-host-name': string = null;
    /** IPv4 address of the endpoint */
    'ipv4-address': string = null;
    /** IPv4 gateway for the endpoint */
    'ipv4-gateway': string = null;
    /** IPv6 address for the endpoint */
    'ipv6-address': string = null;
    /** IPv6 gateway */
    'ipv6-gateway': string = null;
    /** Mac address of the endpointshould be a valid MAC address */
    'mac-address': string = null;
    /** homing host's UUID */
    'node-uuid': string = null;
    /** endpoint FSM state */
    'EndpointState': string = null;
    /** security groups */
    'SecurityGroups': Array<string> = null;
    /** micro-segment VLAN */
    'micro-segment-vlan': number = null;
    /** VM or container attribute/labels */
    'workload-attributes': object = null;
    /** Used to store state if the endpoint is migrating */
    'migration': WorkloadEndpointMigrationStatus = null;
    public static propInfo: { [prop in keyof IWorkloadEndpointStatus]: PropInfoItem } = {
        'workload-name': {
            description:  'VM or container name',
            required: false,
            type: 'string'
        },
        'network': {
            description:  'Network this endpoint belogs to',
            required: false,
            type: 'string'
        },
        'homing-host-addr': {
            description:  'Host address of the host where this endpoint exists',
            required: false,
            type: 'string'
        },
        'homing-host-name': {
            description:  'Host name of the host where this endpoint exists',
            required: false,
            type: 'string'
        },
        'ipv4-address': {
            description:  'IPv4 address of the endpoint',
            required: false,
            type: 'string'
        },
        'ipv4-gateway': {
            description:  'IPv4 gateway for the endpoint',
            required: false,
            type: 'string'
        },
        'ipv6-address': {
            description:  'IPv6 address for the endpoint',
            required: false,
            type: 'string'
        },
        'ipv6-gateway': {
            description:  'IPv6 gateway',
            required: false,
            type: 'string'
        },
        'mac-address': {
            description:  'Mac address of the endpointshould be a valid MAC address',
            hint:  'aabb.ccdd.0000, aabb.ccdd.0000, aabb.ccdd.0000',
            required: false,
            type: 'string'
        },
        'node-uuid': {
            description:  'Homing host&#x27;s UUID',
            required: false,
            type: 'string'
        },
        'EndpointState': {
            description:  'Endpoint FSM state',
            required: false,
            type: 'string'
        },
        'SecurityGroups': {
            description:  'Security groups',
            required: false,
            type: 'Array<string>'
        },
        'micro-segment-vlan': {
            description:  'Micro-segment VLAN',
            required: false,
            type: 'number'
        },
        'workload-attributes': {
            description:  'VM or container attribute/labels',
            required: false,
            type: 'object'
        },
        'migration': {
            description:  'Used to store state if the endpoint is migrating',
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return WorkloadEndpointStatus.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return WorkloadEndpointStatus.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (WorkloadEndpointStatus.propInfo[prop] != null &&
                        WorkloadEndpointStatus.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['SecurityGroups'] = new Array<string>();
        this['migration'] = new WorkloadEndpointMigrationStatus();
        this._inputValue = values;
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['workload-name'] != null) {
            this['workload-name'] = values['workload-name'];
        } else if (fillDefaults && WorkloadEndpointStatus.hasDefaultValue('workload-name')) {
            this['workload-name'] = WorkloadEndpointStatus.propInfo['workload-name'].default;
        } else {
            this['workload-name'] = null
        }
        if (values && values['network'] != null) {
            this['network'] = values['network'];
        } else if (fillDefaults && WorkloadEndpointStatus.hasDefaultValue('network')) {
            this['network'] = WorkloadEndpointStatus.propInfo['network'].default;
        } else {
            this['network'] = null
        }
        if (values && values['homing-host-addr'] != null) {
            this['homing-host-addr'] = values['homing-host-addr'];
        } else if (fillDefaults && WorkloadEndpointStatus.hasDefaultValue('homing-host-addr')) {
            this['homing-host-addr'] = WorkloadEndpointStatus.propInfo['homing-host-addr'].default;
        } else {
            this['homing-host-addr'] = null
        }
        if (values && values['homing-host-name'] != null) {
            this['homing-host-name'] = values['homing-host-name'];
        } else if (fillDefaults && WorkloadEndpointStatus.hasDefaultValue('homing-host-name')) {
            this['homing-host-name'] = WorkloadEndpointStatus.propInfo['homing-host-name'].default;
        } else {
            this['homing-host-name'] = null
        }
        if (values && values['ipv4-address'] != null) {
            this['ipv4-address'] = values['ipv4-address'];
        } else if (fillDefaults && WorkloadEndpointStatus.hasDefaultValue('ipv4-address')) {
            this['ipv4-address'] = WorkloadEndpointStatus.propInfo['ipv4-address'].default;
        } else {
            this['ipv4-address'] = null
        }
        if (values && values['ipv4-gateway'] != null) {
            this['ipv4-gateway'] = values['ipv4-gateway'];
        } else if (fillDefaults && WorkloadEndpointStatus.hasDefaultValue('ipv4-gateway')) {
            this['ipv4-gateway'] = WorkloadEndpointStatus.propInfo['ipv4-gateway'].default;
        } else {
            this['ipv4-gateway'] = null
        }
        if (values && values['ipv6-address'] != null) {
            this['ipv6-address'] = values['ipv6-address'];
        } else if (fillDefaults && WorkloadEndpointStatus.hasDefaultValue('ipv6-address')) {
            this['ipv6-address'] = WorkloadEndpointStatus.propInfo['ipv6-address'].default;
        } else {
            this['ipv6-address'] = null
        }
        if (values && values['ipv6-gateway'] != null) {
            this['ipv6-gateway'] = values['ipv6-gateway'];
        } else if (fillDefaults && WorkloadEndpointStatus.hasDefaultValue('ipv6-gateway')) {
            this['ipv6-gateway'] = WorkloadEndpointStatus.propInfo['ipv6-gateway'].default;
        } else {
            this['ipv6-gateway'] = null
        }
        if (values && values['mac-address'] != null) {
            this['mac-address'] = values['mac-address'];
        } else if (fillDefaults && WorkloadEndpointStatus.hasDefaultValue('mac-address')) {
            this['mac-address'] = WorkloadEndpointStatus.propInfo['mac-address'].default;
        } else {
            this['mac-address'] = null
        }
        if (values && values['node-uuid'] != null) {
            this['node-uuid'] = values['node-uuid'];
        } else if (fillDefaults && WorkloadEndpointStatus.hasDefaultValue('node-uuid')) {
            this['node-uuid'] = WorkloadEndpointStatus.propInfo['node-uuid'].default;
        } else {
            this['node-uuid'] = null
        }
        if (values && values['EndpointState'] != null) {
            this['EndpointState'] = values['EndpointState'];
        } else if (fillDefaults && WorkloadEndpointStatus.hasDefaultValue('EndpointState')) {
            this['EndpointState'] = WorkloadEndpointStatus.propInfo['EndpointState'].default;
        } else {
            this['EndpointState'] = null
        }
        if (values && values['SecurityGroups'] != null) {
            this['SecurityGroups'] = values['SecurityGroups'];
        } else if (fillDefaults && WorkloadEndpointStatus.hasDefaultValue('SecurityGroups')) {
            this['SecurityGroups'] = [ WorkloadEndpointStatus.propInfo['SecurityGroups'].default];
        } else {
            this['SecurityGroups'] = [];
        }
        if (values && values['micro-segment-vlan'] != null) {
            this['micro-segment-vlan'] = values['micro-segment-vlan'];
        } else if (fillDefaults && WorkloadEndpointStatus.hasDefaultValue('micro-segment-vlan')) {
            this['micro-segment-vlan'] = WorkloadEndpointStatus.propInfo['micro-segment-vlan'].default;
        } else {
            this['micro-segment-vlan'] = null
        }
        if (values && values['workload-attributes'] != null) {
            this['workload-attributes'] = values['workload-attributes'];
        } else if (fillDefaults && WorkloadEndpointStatus.hasDefaultValue('workload-attributes')) {
            this['workload-attributes'] = WorkloadEndpointStatus.propInfo['workload-attributes'].default;
        } else {
            this['workload-attributes'] = null
        }
        if (values) {
            this['migration'].setValues(values['migration'], fillDefaults);
        } else {
            this['migration'].setValues(null, fillDefaults);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'workload-name': CustomFormControl(new FormControl(this['workload-name']), WorkloadEndpointStatus.propInfo['workload-name']),
                'network': CustomFormControl(new FormControl(this['network']), WorkloadEndpointStatus.propInfo['network']),
                'homing-host-addr': CustomFormControl(new FormControl(this['homing-host-addr']), WorkloadEndpointStatus.propInfo['homing-host-addr']),
                'homing-host-name': CustomFormControl(new FormControl(this['homing-host-name']), WorkloadEndpointStatus.propInfo['homing-host-name']),
                'ipv4-address': CustomFormControl(new FormControl(this['ipv4-address']), WorkloadEndpointStatus.propInfo['ipv4-address']),
                'ipv4-gateway': CustomFormControl(new FormControl(this['ipv4-gateway']), WorkloadEndpointStatus.propInfo['ipv4-gateway']),
                'ipv6-address': CustomFormControl(new FormControl(this['ipv6-address']), WorkloadEndpointStatus.propInfo['ipv6-address']),
                'ipv6-gateway': CustomFormControl(new FormControl(this['ipv6-gateway']), WorkloadEndpointStatus.propInfo['ipv6-gateway']),
                'mac-address': CustomFormControl(new FormControl(this['mac-address']), WorkloadEndpointStatus.propInfo['mac-address']),
                'node-uuid': CustomFormControl(new FormControl(this['node-uuid']), WorkloadEndpointStatus.propInfo['node-uuid']),
                'EndpointState': CustomFormControl(new FormControl(this['EndpointState']), WorkloadEndpointStatus.propInfo['EndpointState']),
                'SecurityGroups': CustomFormControl(new FormControl(this['SecurityGroups']), WorkloadEndpointStatus.propInfo['SecurityGroups']),
                'micro-segment-vlan': CustomFormControl(new FormControl(this['micro-segment-vlan']), WorkloadEndpointStatus.propInfo['micro-segment-vlan']),
                'workload-attributes': CustomFormControl(new FormControl(this['workload-attributes']), WorkloadEndpointStatus.propInfo['workload-attributes']),
                'migration': CustomFormGroup(this['migration'].$formGroup, WorkloadEndpointStatus.propInfo['migration'].required),
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('migration') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('migration').get(field);
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
            this._formGroup.controls['workload-name'].setValue(this['workload-name']);
            this._formGroup.controls['network'].setValue(this['network']);
            this._formGroup.controls['homing-host-addr'].setValue(this['homing-host-addr']);
            this._formGroup.controls['homing-host-name'].setValue(this['homing-host-name']);
            this._formGroup.controls['ipv4-address'].setValue(this['ipv4-address']);
            this._formGroup.controls['ipv4-gateway'].setValue(this['ipv4-gateway']);
            this._formGroup.controls['ipv6-address'].setValue(this['ipv6-address']);
            this._formGroup.controls['ipv6-gateway'].setValue(this['ipv6-gateway']);
            this._formGroup.controls['mac-address'].setValue(this['mac-address']);
            this._formGroup.controls['node-uuid'].setValue(this['node-uuid']);
            this._formGroup.controls['EndpointState'].setValue(this['EndpointState']);
            this._formGroup.controls['SecurityGroups'].setValue(this['SecurityGroups']);
            this._formGroup.controls['micro-segment-vlan'].setValue(this['micro-segment-vlan']);
            this._formGroup.controls['workload-attributes'].setValue(this['workload-attributes']);
            this['migration'].setFormGroupValuesToBeModelValues();
        }
    }
}

