/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { ApiListMeta, IApiListMeta } from './api-list-meta.model';
import { MonitoringFlowExportPolicy, IMonitoringFlowExportPolicy } from './monitoring-flow-export-policy.model';

export interface IMonitoringFlowExportPolicyList {
    'kind'?: string;
    'api-version'?: string;
    'list-meta'?: IApiListMeta;
    'items'?: Array<IMonitoringFlowExportPolicy>;
}


export class MonitoringFlowExportPolicyList extends BaseModel implements IMonitoringFlowExportPolicyList {
    'kind': string = null;
    'api-version': string = null;
    'list-meta': ApiListMeta = null;
    'items': Array<MonitoringFlowExportPolicy> = null;
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
        return MonitoringFlowExportPolicyList.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return MonitoringFlowExportPolicyList.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (MonitoringFlowExportPolicyList.propInfo[prop] != null &&
                        MonitoringFlowExportPolicyList.propInfo[prop].default != null &&
                        MonitoringFlowExportPolicyList.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['list-meta'] = new ApiListMeta();
        this['items'] = new Array<MonitoringFlowExportPolicy>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['kind'] != null) {
            this['kind'] = values['kind'];
        } else if (fillDefaults && MonitoringFlowExportPolicyList.hasDefaultValue('kind')) {
            this['kind'] = MonitoringFlowExportPolicyList.propInfo['kind'].default;
        } else {
            this['kind'] = null
        }
        if (values && values['api-version'] != null) {
            this['api-version'] = values['api-version'];
        } else if (fillDefaults && MonitoringFlowExportPolicyList.hasDefaultValue('api-version')) {
            this['api-version'] = MonitoringFlowExportPolicyList.propInfo['api-version'].default;
        } else {
            this['api-version'] = null
        }
        if (values) {
            this['list-meta'].setValues(values['list-meta'], fillDefaults);
        } else {
            this['list-meta'].setValues(null, fillDefaults);
        }
        if (values) {
            this.fillModelArray<MonitoringFlowExportPolicy>(this, 'items', values['items'], MonitoringFlowExportPolicy);
        } else {
            this['items'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'kind': CustomFormControl(new FormControl(this['kind']), MonitoringFlowExportPolicyList.propInfo['kind'].description),
                'api-version': CustomFormControl(new FormControl(this['api-version']), MonitoringFlowExportPolicyList.propInfo['api-version'].description),
                'list-meta': this['list-meta'].$formGroup,
                'items': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<MonitoringFlowExportPolicy>('items', this['items'], MonitoringFlowExportPolicy);
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
            this.fillModelArray<MonitoringFlowExportPolicy>(this, 'items', this['items'], MonitoringFlowExportPolicy);
        }
    }
}

