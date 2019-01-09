/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, enumValidator } from './validators';
import { BaseModel, PropInfoItem } from './base-model';

import { ClusterIPConfig, IClusterIPConfig } from './cluster-ip-config.model';
import { ClusterSmartNICSpec_mgmt_mode,  ClusterSmartNICSpec_mgmt_mode_uihint  } from './enums';
import { ClusterSmartNICSpec_network_mode,  ClusterSmartNICSpec_network_mode_uihint  } from './enums';

export interface IClusterSmartNICSpec {
    'admit'?: boolean;
    'hostname'?: string;
    'ip-config'?: IClusterIPConfig;
    'mgmt-mode'?: ClusterSmartNICSpec_mgmt_mode;
    'network-mode'?: ClusterSmartNICSpec_network_mode;
    'mgmt-vlan'?: number;
    'controllers'?: Array<string>;
}


export class ClusterSmartNICSpec extends BaseModel implements IClusterSmartNICSpec {
    'admit': boolean = null;
    'hostname': string = null;
    'ip-config': ClusterIPConfig = null;
    'mgmt-mode': ClusterSmartNICSpec_mgmt_mode = null;
    'network-mode': ClusterSmartNICSpec_network_mode = null;
    /** value should be between 0 and 4095
     */
    'mgmt-vlan': number = null;
    'controllers': Array<string> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'admit': {
            type: 'boolean'
        },
        'hostname': {
            type: 'string'
        },
        'ip-config': {
            type: 'object'
        },
        'mgmt-mode': {
            enum: ClusterSmartNICSpec_mgmt_mode_uihint,
            default: 'HOST',
            type: 'string'
        },
        'network-mode': {
            enum: ClusterSmartNICSpec_network_mode_uihint,
            default: 'OOB',
            type: 'string'
        },
        'mgmt-vlan': {
            description:  'value should be between 0 and 4095 ',
            type: 'number'
        },
        'controllers': {
            type: 'Array<string>'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return ClusterSmartNICSpec.propInfo[propName];
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (ClusterSmartNICSpec.propInfo[prop] != null &&
                        ClusterSmartNICSpec.propInfo[prop].default != null &&
                        ClusterSmartNICSpec.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any) {
        super();
        this['ip-config'] = new ClusterIPConfig();
        this['controllers'] = new Array<string>();
        this.setValues(values);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['admit'] != null) {
            this['admit'] = values['admit'];
        } else if (fillDefaults && ClusterSmartNICSpec.hasDefaultValue('admit')) {
            this['admit'] = ClusterSmartNICSpec.propInfo['admit'].default;
        }
        if (values && values['hostname'] != null) {
            this['hostname'] = values['hostname'];
        } else if (fillDefaults && ClusterSmartNICSpec.hasDefaultValue('hostname')) {
            this['hostname'] = ClusterSmartNICSpec.propInfo['hostname'].default;
        }
        if (values) {
            this['ip-config'].setValues(values['ip-config']);
        }
        if (values && values['mgmt-mode'] != null) {
            this['mgmt-mode'] = values['mgmt-mode'];
        } else if (fillDefaults && ClusterSmartNICSpec.hasDefaultValue('mgmt-mode')) {
            this['mgmt-mode'] = <ClusterSmartNICSpec_mgmt_mode>  ClusterSmartNICSpec.propInfo['mgmt-mode'].default;
        }
        if (values && values['network-mode'] != null) {
            this['network-mode'] = values['network-mode'];
        } else if (fillDefaults && ClusterSmartNICSpec.hasDefaultValue('network-mode')) {
            this['network-mode'] = <ClusterSmartNICSpec_network_mode>  ClusterSmartNICSpec.propInfo['network-mode'].default;
        }
        if (values && values['mgmt-vlan'] != null) {
            this['mgmt-vlan'] = values['mgmt-vlan'];
        } else if (fillDefaults && ClusterSmartNICSpec.hasDefaultValue('mgmt-vlan')) {
            this['mgmt-vlan'] = ClusterSmartNICSpec.propInfo['mgmt-vlan'].default;
        }
        if (values && values['controllers'] != null) {
            this['controllers'] = values['controllers'];
        } else if (fillDefaults && ClusterSmartNICSpec.hasDefaultValue('controllers')) {
            this['controllers'] = [ ClusterSmartNICSpec.propInfo['controllers'].default];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'admit': new FormControl(this['admit']),
                'hostname': new FormControl(this['hostname']),
                'ip-config': this['ip-config'].$formGroup,
                'mgmt-mode': new FormControl(this['mgmt-mode'], [enumValidator(ClusterSmartNICSpec_mgmt_mode), ]),
                'network-mode': new FormControl(this['network-mode'], [enumValidator(ClusterSmartNICSpec_network_mode), ]),
                'mgmt-vlan': new FormControl(this['mgmt-vlan'], [Validators.max(4095), ]),
                'controllers': new FormControl(this['controllers']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['admit'].setValue(this['admit']);
            this._formGroup.controls['hostname'].setValue(this['hostname']);
            this['ip-config'].setFormGroupValuesToBeModelValues();
            this._formGroup.controls['mgmt-mode'].setValue(this['mgmt-mode']);
            this._formGroup.controls['network-mode'].setValue(this['network-mode']);
            this._formGroup.controls['mgmt-vlan'].setValue(this['mgmt-vlan']);
            this._formGroup.controls['controllers'].setValue(this['controllers']);
        }
    }
}

