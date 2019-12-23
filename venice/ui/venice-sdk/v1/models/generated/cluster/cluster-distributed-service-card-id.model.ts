/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';


export interface IClusterDistributedServiceCardID {
    'id'?: string;
    'mac-address'?: string;
}


export class ClusterDistributedServiceCardID extends BaseModel implements IClusterDistributedServiceCardID {
    /** Name contains the name of the DistributedServiceCard on a host */
    'id': string = null;
    /** MACAddress contains the primary MAC address of a DistributedServiceCardshould be a valid MAC address */
    'mac-address': string = null;
    public static propInfo: { [prop in keyof IClusterDistributedServiceCardID]: PropInfoItem } = {
        'id': {
            description:  `Name contains the name of the DistributedServiceCard on a host`,
            required: false,
            type: 'string'
        },
        'mac-address': {
            description:  `MACAddress contains the primary MAC address of a DistributedServiceCardshould be a valid MAC address`,
            hint:  'aabb.ccdd.0000, aabb.ccdd.0000, aabb.ccdd.0000',
            required: false,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return ClusterDistributedServiceCardID.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return ClusterDistributedServiceCardID.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (ClusterDistributedServiceCardID.propInfo[prop] != null &&
                        ClusterDistributedServiceCardID.propInfo[prop].default != null);
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
        if (values && values['id'] != null) {
            this['id'] = values['id'];
        } else if (fillDefaults && ClusterDistributedServiceCardID.hasDefaultValue('id')) {
            this['id'] = ClusterDistributedServiceCardID.propInfo['id'].default;
        } else {
            this['id'] = null
        }
        if (values && values['mac-address'] != null) {
            this['mac-address'] = values['mac-address'];
        } else if (fillDefaults && ClusterDistributedServiceCardID.hasDefaultValue('mac-address')) {
            this['mac-address'] = ClusterDistributedServiceCardID.propInfo['mac-address'].default;
        } else {
            this['mac-address'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'id': CustomFormControl(new FormControl(this['id']), ClusterDistributedServiceCardID.propInfo['id']),
                'mac-address': CustomFormControl(new FormControl(this['mac-address']), ClusterDistributedServiceCardID.propInfo['mac-address']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['id'].setValue(this['id']);
            this._formGroup.controls['mac-address'].setValue(this['mac-address']);
        }
    }
}

