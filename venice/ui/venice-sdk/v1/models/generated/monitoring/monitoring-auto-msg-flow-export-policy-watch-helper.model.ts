/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { MonitoringAutoMsgFlowExportPolicyWatchHelperWatchEvent, IMonitoringAutoMsgFlowExportPolicyWatchHelperWatchEvent } from './monitoring-auto-msg-flow-export-policy-watch-helper-watch-event.model';

export interface IMonitoringAutoMsgFlowExportPolicyWatchHelper {
    'events'?: Array<IMonitoringAutoMsgFlowExportPolicyWatchHelperWatchEvent>;
}


export class MonitoringAutoMsgFlowExportPolicyWatchHelper extends BaseModel implements IMonitoringAutoMsgFlowExportPolicyWatchHelper {
    'events': Array<MonitoringAutoMsgFlowExportPolicyWatchHelperWatchEvent> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'events': {
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return MonitoringAutoMsgFlowExportPolicyWatchHelper.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return MonitoringAutoMsgFlowExportPolicyWatchHelper.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (MonitoringAutoMsgFlowExportPolicyWatchHelper.propInfo[prop] != null &&
                        MonitoringAutoMsgFlowExportPolicyWatchHelper.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['events'] = new Array<MonitoringAutoMsgFlowExportPolicyWatchHelperWatchEvent>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values) {
            this.fillModelArray<MonitoringAutoMsgFlowExportPolicyWatchHelperWatchEvent>(this, 'events', values['events'], MonitoringAutoMsgFlowExportPolicyWatchHelperWatchEvent);
        } else {
            this['events'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'events': new FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray<MonitoringAutoMsgFlowExportPolicyWatchHelperWatchEvent>('events', this['events'], MonitoringAutoMsgFlowExportPolicyWatchHelperWatchEvent);
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('events') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('events').get(field);
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
            this.fillModelArray<MonitoringAutoMsgFlowExportPolicyWatchHelperWatchEvent>(this, 'events', this['events'], MonitoringAutoMsgFlowExportPolicyWatchHelperWatchEvent);
        }
    }
}

