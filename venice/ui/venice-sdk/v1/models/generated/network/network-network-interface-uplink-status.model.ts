/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { NetworkTransceiverStatus, INetworkTransceiverStatus } from './network-transceiver-status.model';

export interface INetworkNetworkInterfaceUplinkStatus {
    'link-speed'?: string;
    'transceiver-status'?: INetworkTransceiverStatus;
}


export class NetworkNetworkInterfaceUplinkStatus extends BaseModel implements INetworkNetworkInterfaceUplinkStatus {
    /** LinkSpeed auto-negotiated */
    'link-speed': string = null;
    'transceiver-status': NetworkTransceiverStatus = null;
    public static propInfo: { [prop in keyof INetworkNetworkInterfaceUplinkStatus]: PropInfoItem } = {
        'link-speed': {
            description:  `LinkSpeed auto-negotiated`,
            required: false,
            type: 'string'
        },
        'transceiver-status': {
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return NetworkNetworkInterfaceUplinkStatus.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return NetworkNetworkInterfaceUplinkStatus.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (NetworkNetworkInterfaceUplinkStatus.propInfo[prop] != null &&
                        NetworkNetworkInterfaceUplinkStatus.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['transceiver-status'] = new NetworkTransceiverStatus();
        this._inputValue = values;
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['link-speed'] != null) {
            this['link-speed'] = values['link-speed'];
        } else if (fillDefaults && NetworkNetworkInterfaceUplinkStatus.hasDefaultValue('link-speed')) {
            this['link-speed'] = NetworkNetworkInterfaceUplinkStatus.propInfo['link-speed'].default;
        } else {
            this['link-speed'] = null
        }
        if (values) {
            this['transceiver-status'].setValues(values['transceiver-status'], fillDefaults);
        } else {
            this['transceiver-status'].setValues(null, fillDefaults);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'link-speed': CustomFormControl(new FormControl(this['link-speed']), NetworkNetworkInterfaceUplinkStatus.propInfo['link-speed']),
                'transceiver-status': CustomFormGroup(this['transceiver-status'].$formGroup, NetworkNetworkInterfaceUplinkStatus.propInfo['transceiver-status'].required),
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('transceiver-status') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('transceiver-status').get(field);
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
            this._formGroup.controls['link-speed'].setValue(this['link-speed']);
            this['transceiver-status'].setFormGroupValuesToBeModelValues();
        }
    }
}

