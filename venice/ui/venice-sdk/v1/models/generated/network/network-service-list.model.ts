/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator } from './validators';
import { BaseModel, PropInfoItem } from './base-model';

import { ApiListMeta, IApiListMeta } from './api-list-meta.model';
import { NetworkService, INetworkService } from './network-service.model';

export interface INetworkServiceList {
    'kind'?: string;
    'api-version'?: string;
    'list-meta'?: IApiListMeta;
    'items'?: Array<INetworkService>;
}


export class NetworkServiceList extends BaseModel implements INetworkServiceList {
    'kind': string = null;
    'api-version': string = null;
    'list-meta': ApiListMeta = null;
    'items': Array<NetworkService> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'kind': {
            type: 'string'
        },
        'api-version': {
            type: 'string'
        },
        'list-meta': {
            type: 'object'
        },
        'items': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return NetworkServiceList.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return NetworkServiceList.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (NetworkServiceList.propInfo[prop] != null &&
                        NetworkServiceList.propInfo[prop].default != null &&
                        NetworkServiceList.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['list-meta'] = new ApiListMeta();
        this['items'] = new Array<NetworkService>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['kind'] != null) {
            this['kind'] = values['kind'];
        } else if (fillDefaults && NetworkServiceList.hasDefaultValue('kind')) {
            this['kind'] = NetworkServiceList.propInfo['kind'].default;
        }
        if (values && values['api-version'] != null) {
            this['api-version'] = values['api-version'];
        } else if (fillDefaults && NetworkServiceList.hasDefaultValue('api-version')) {
            this['api-version'] = NetworkServiceList.propInfo['api-version'].default;
        }
        if (values) {
            this['list-meta'].setValues(values['list-meta']);
        }
        if (values) {
            this.fillModelArray<NetworkService>(this, 'items', values['items'], NetworkService);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'kind': new FormControl(this['kind']),
                'api-version': new FormControl(this['api-version']),
                'list-meta': this['list-meta'].$formGroup,
                'items': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<NetworkService>('items', this['items'], NetworkService);
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['kind'].setValue(this['kind']);
            this._formGroup.controls['api-version'].setValue(this['api-version']);
            this['list-meta'].setFormGroupValuesToBeModelValues();
            this.fillModelArray<NetworkService>(this, 'items', this['items'], NetworkService);
        }
    }
}

