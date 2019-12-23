/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { NetworkBGPNeighbor_enable_address_families,  } from './enums';

export interface INetworkBGPNeighbor {
    'shutdown'?: boolean;
    'ip-address': string;
    'remote-as'?: number;
    'multi-hop': number;
    'enable-address-families': Array<NetworkBGPNeighbor_enable_address_families>;
}


export class NetworkBGPNeighbor extends BaseModel implements INetworkBGPNeighbor {
    /** Shutdown this neighbor session. */
    'shutdown': boolean = null;
    /** Neighbor IP Address. */
    'ip-address': string = null;
    /** ASN the neighbor belongs to. */
    'remote-as': number = null;
    /** BGP Multihop configuration, 0 disables multihop. */
    'multi-hop': number = null;
    /** Address families to enable on the neighbor. */
    'enable-address-families': Array<NetworkBGPNeighbor_enable_address_families> = null;
    public static propInfo: { [prop in keyof INetworkBGPNeighbor]: PropInfoItem } = {
        'shutdown': {
            description:  `Shutdown this neighbor session.`,
            required: false,
            type: 'boolean'
        },
        'ip-address': {
            description:  `Neighbor IP Address.`,
            hint:  '10.1.1.1, ff02::5 ',
            required: true,
            type: 'string'
        },
        'remote-as': {
            description:  `ASN the neighbor belongs to.`,
            required: false,
            type: 'number'
        },
        'multi-hop': {
            description:  `BGP Multihop configuration, 0 disables multihop.`,
            required: true,
            type: 'number'
        },
        'enable-address-families': {
            enum: NetworkBGPNeighbor_enable_address_families,
            default: 'ipv4-unicast',
            description:  `Address families to enable on the neighbor.`,
            required: true,
            type: 'Array<string>'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return NetworkBGPNeighbor.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return NetworkBGPNeighbor.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (NetworkBGPNeighbor.propInfo[prop] != null &&
                        NetworkBGPNeighbor.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['enable-address-families'] = new Array<NetworkBGPNeighbor_enable_address_families>();
        this._inputValue = values;
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['shutdown'] != null) {
            this['shutdown'] = values['shutdown'];
        } else if (fillDefaults && NetworkBGPNeighbor.hasDefaultValue('shutdown')) {
            this['shutdown'] = NetworkBGPNeighbor.propInfo['shutdown'].default;
        } else {
            this['shutdown'] = null
        }
        if (values && values['ip-address'] != null) {
            this['ip-address'] = values['ip-address'];
        } else if (fillDefaults && NetworkBGPNeighbor.hasDefaultValue('ip-address')) {
            this['ip-address'] = NetworkBGPNeighbor.propInfo['ip-address'].default;
        } else {
            this['ip-address'] = null
        }
        if (values && values['remote-as'] != null) {
            this['remote-as'] = values['remote-as'];
        } else if (fillDefaults && NetworkBGPNeighbor.hasDefaultValue('remote-as')) {
            this['remote-as'] = NetworkBGPNeighbor.propInfo['remote-as'].default;
        } else {
            this['remote-as'] = null
        }
        if (values && values['multi-hop'] != null) {
            this['multi-hop'] = values['multi-hop'];
        } else if (fillDefaults && NetworkBGPNeighbor.hasDefaultValue('multi-hop')) {
            this['multi-hop'] = NetworkBGPNeighbor.propInfo['multi-hop'].default;
        } else {
            this['multi-hop'] = null
        }
        if (values && values['enable-address-families'] != null) {
            this['enable-address-families'] = values['enable-address-families'];
        } else if (fillDefaults && NetworkBGPNeighbor.hasDefaultValue('enable-address-families')) {
            this['enable-address-families'] = [ NetworkBGPNeighbor.propInfo['enable-address-families'].default];
        } else {
            this['enable-address-families'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'shutdown': CustomFormControl(new FormControl(this['shutdown']), NetworkBGPNeighbor.propInfo['shutdown']),
                'ip-address': CustomFormControl(new FormControl(this['ip-address'], [required, ]), NetworkBGPNeighbor.propInfo['ip-address']),
                'remote-as': CustomFormControl(new FormControl(this['remote-as']), NetworkBGPNeighbor.propInfo['remote-as']),
                'multi-hop': CustomFormControl(new FormControl(this['multi-hop'], [required, maxValueValidator(256), ]), NetworkBGPNeighbor.propInfo['multi-hop']),
                'enable-address-families': CustomFormControl(new FormControl(this['enable-address-families']), NetworkBGPNeighbor.propInfo['enable-address-families']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['shutdown'].setValue(this['shutdown']);
            this._formGroup.controls['ip-address'].setValue(this['ip-address']);
            this._formGroup.controls['remote-as'].setValue(this['remote-as']);
            this._formGroup.controls['multi-hop'].setValue(this['multi-hop']);
            this._formGroup.controls['enable-address-families'].setValue(this['enable-address-families']);
        }
    }
}

