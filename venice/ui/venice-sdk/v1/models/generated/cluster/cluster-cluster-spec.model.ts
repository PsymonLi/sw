/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';


export interface IClusterClusterSpec {
    'quorum-nodes'?: Array<string>;
    'virtual-ip'?: string;
    'ntp-servers'?: Array<string>;
    'auto-admit-nics'?: boolean;
    'certs'?: string;
    'key'?: string;
}


export class ClusterClusterSpec extends BaseModel implements IClusterClusterSpec {
    /** QuorumNodes contains the list of hostnames for nodes configured to be quorum
    nodes in the cluster. */
    'quorum-nodes': Array<string> = null;
    /** VirtualIP is the IP address for managing the cluster. It will be hosted by
    the winner of election between quorum nodes. */
    'virtual-ip': string = null;
    /** NTPServers contains the list of NTP servers for the cluster. */
    'ntp-servers': Array<string> = null;
    /** AutoAdmitNICs when enabled auto-admits NICs that are validated
    into Venice Cluster. When it is disabled, NICs validated by CMD are
    set to Pending state and it requires Manual approval to be admitted
    into the cluster. */
    'auto-admit-nics': boolean = null;
    'certs': string = null;
    'key': string = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'quorum-nodes': {
            description:  'QuorumNodes contains the list of hostnames for nodes configured to be quorum nodes in the cluster.',
            type: 'Array<string>'
        },
        'virtual-ip': {
            description:  'VirtualIP is the IP address for managing the cluster. It will be hosted by the winner of election between quorum nodes.',
            type: 'string'
        },
        'ntp-servers': {
            description:  'NTPServers contains the list of NTP servers for the cluster.',
            type: 'Array<string>'
        },
        'auto-admit-nics': {
            description:  'AutoAdmitNICs when enabled auto-admits NICs that are validated into Venice Cluster. When it is disabled, NICs validated by CMD are set to Pending state and it requires Manual approval to be admitted into the cluster.',
            type: 'boolean'
        },
        'certs': {
            type: 'string'
        },
        'key': {
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return ClusterClusterSpec.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return ClusterClusterSpec.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (ClusterClusterSpec.propInfo[prop] != null &&
                        ClusterClusterSpec.propInfo[prop].default != null &&
                        ClusterClusterSpec.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['quorum-nodes'] = new Array<string>();
        this['ntp-servers'] = new Array<string>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['quorum-nodes'] != null) {
            this['quorum-nodes'] = values['quorum-nodes'];
        } else if (fillDefaults && ClusterClusterSpec.hasDefaultValue('quorum-nodes')) {
            this['quorum-nodes'] = [ ClusterClusterSpec.propInfo['quorum-nodes'].default];
        } else {
            this['quorum-nodes'] = [];
        }
        if (values && values['virtual-ip'] != null) {
            this['virtual-ip'] = values['virtual-ip'];
        } else if (fillDefaults && ClusterClusterSpec.hasDefaultValue('virtual-ip')) {
            this['virtual-ip'] = ClusterClusterSpec.propInfo['virtual-ip'].default;
        } else {
            this['virtual-ip'] = null
        }
        if (values && values['ntp-servers'] != null) {
            this['ntp-servers'] = values['ntp-servers'];
        } else if (fillDefaults && ClusterClusterSpec.hasDefaultValue('ntp-servers')) {
            this['ntp-servers'] = [ ClusterClusterSpec.propInfo['ntp-servers'].default];
        } else {
            this['ntp-servers'] = [];
        }
        if (values && values['auto-admit-nics'] != null) {
            this['auto-admit-nics'] = values['auto-admit-nics'];
        } else if (fillDefaults && ClusterClusterSpec.hasDefaultValue('auto-admit-nics')) {
            this['auto-admit-nics'] = ClusterClusterSpec.propInfo['auto-admit-nics'].default;
        } else {
            this['auto-admit-nics'] = null
        }
        if (values && values['certs'] != null) {
            this['certs'] = values['certs'];
        } else if (fillDefaults && ClusterClusterSpec.hasDefaultValue('certs')) {
            this['certs'] = ClusterClusterSpec.propInfo['certs'].default;
        } else {
            this['certs'] = null
        }
        if (values && values['key'] != null) {
            this['key'] = values['key'];
        } else if (fillDefaults && ClusterClusterSpec.hasDefaultValue('key')) {
            this['key'] = ClusterClusterSpec.propInfo['key'].default;
        } else {
            this['key'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'quorum-nodes': CustomFormControl(new FormControl(this['quorum-nodes']), ClusterClusterSpec.propInfo['quorum-nodes'].description),
                'virtual-ip': CustomFormControl(new FormControl(this['virtual-ip']), ClusterClusterSpec.propInfo['virtual-ip'].description),
                'ntp-servers': CustomFormControl(new FormControl(this['ntp-servers']), ClusterClusterSpec.propInfo['ntp-servers'].description),
                'auto-admit-nics': CustomFormControl(new FormControl(this['auto-admit-nics']), ClusterClusterSpec.propInfo['auto-admit-nics'].description),
                'certs': CustomFormControl(new FormControl(this['certs']), ClusterClusterSpec.propInfo['certs'].description),
                'key': CustomFormControl(new FormControl(this['key']), ClusterClusterSpec.propInfo['key'].description),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['quorum-nodes'].setValue(this['quorum-nodes']);
            this._formGroup.controls['virtual-ip'].setValue(this['virtual-ip']);
            this._formGroup.controls['ntp-servers'].setValue(this['ntp-servers']);
            this._formGroup.controls['auto-admit-nics'].setValue(this['auto-admit-nics']);
            this._formGroup.controls['certs'].setValue(this['certs']);
            this._formGroup.controls['key'].setValue(this['key']);
        }
    }
}

