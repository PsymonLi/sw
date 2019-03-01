/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { ApiListMeta, IApiListMeta } from './api-list-meta.model';
import { MonitoringStatsPolicy, IMonitoringStatsPolicy } from './monitoring-stats-policy.model';

export interface IMonitoringStatsPolicyList {
    'kind'?: string;
    'api-version'?: string;
    'list-meta'?: IApiListMeta;
    'items'?: Array<IMonitoringStatsPolicy>;
}


export class MonitoringStatsPolicyList extends BaseModel implements IMonitoringStatsPolicyList {
    'kind': string = null;
    'api-version': string = null;
    'list-meta': ApiListMeta = null;
    'items': Array<MonitoringStatsPolicy> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'kind': {
            required: false,
            type: 'string'
        },
        'api-version': {
            required: false,
            type: 'string'
        },
        'list-meta': {
            required: false,
            type: 'object'
        },
        'items': {
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return MonitoringStatsPolicyList.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return MonitoringStatsPolicyList.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (MonitoringStatsPolicyList.propInfo[prop] != null &&
                        MonitoringStatsPolicyList.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['list-meta'] = new ApiListMeta();
        this['items'] = new Array<MonitoringStatsPolicy>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['kind'] != null) {
            this['kind'] = values['kind'];
        } else if (fillDefaults && MonitoringStatsPolicyList.hasDefaultValue('kind')) {
            this['kind'] = MonitoringStatsPolicyList.propInfo['kind'].default;
        } else {
            this['kind'] = null
        }
        if (values && values['api-version'] != null) {
            this['api-version'] = values['api-version'];
        } else if (fillDefaults && MonitoringStatsPolicyList.hasDefaultValue('api-version')) {
            this['api-version'] = MonitoringStatsPolicyList.propInfo['api-version'].default;
        } else {
            this['api-version'] = null
        }
        if (values) {
            this['list-meta'].setValues(values['list-meta'], fillDefaults);
        } else {
            this['list-meta'].setValues(null, fillDefaults);
        }
        if (values) {
            this.fillModelArray<MonitoringStatsPolicy>(this, 'items', values['items'], MonitoringStatsPolicy);
        } else {
            this['items'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'kind': CustomFormControl(new FormControl(this['kind']), MonitoringStatsPolicyList.propInfo['kind']),
                'api-version': CustomFormControl(new FormControl(this['api-version']), MonitoringStatsPolicyList.propInfo['api-version']),
                'list-meta': CustomFormGroup(this['list-meta'].$formGroup, MonitoringStatsPolicyList.propInfo['list-meta'].required),
                'items': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<MonitoringStatsPolicy>('items', this['items'], MonitoringStatsPolicy);
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('list-meta') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('list-meta').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('items') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('items').get(field);
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
            this._formGroup.controls['kind'].setValue(this['kind']);
            this._formGroup.controls['api-version'].setValue(this['api-version']);
            this['list-meta'].setFormGroupValuesToBeModelValues();
            this.fillModelArray<MonitoringStatsPolicy>(this, 'items', this['items'], MonitoringStatsPolicy);
        }
    }
}

