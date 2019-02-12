/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { MonitoringAutoMsgMirrorSessionWatchHelperWatchEvent, IMonitoringAutoMsgMirrorSessionWatchHelperWatchEvent } from './monitoring-auto-msg-mirror-session-watch-helper-watch-event.model';

export interface IMonitoringAutoMsgMirrorSessionWatchHelper {
    'events'?: Array<IMonitoringAutoMsgMirrorSessionWatchHelperWatchEvent>;
}


export class MonitoringAutoMsgMirrorSessionWatchHelper extends BaseModel implements IMonitoringAutoMsgMirrorSessionWatchHelper {
    'events': Array<MonitoringAutoMsgMirrorSessionWatchHelperWatchEvent> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'events': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return MonitoringAutoMsgMirrorSessionWatchHelper.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return MonitoringAutoMsgMirrorSessionWatchHelper.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (MonitoringAutoMsgMirrorSessionWatchHelper.propInfo[prop] != null &&
                        MonitoringAutoMsgMirrorSessionWatchHelper.propInfo[prop].default != null &&
                        MonitoringAutoMsgMirrorSessionWatchHelper.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['events'] = new Array<MonitoringAutoMsgMirrorSessionWatchHelperWatchEvent>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values) {
            this.fillModelArray<MonitoringAutoMsgMirrorSessionWatchHelperWatchEvent>(this, 'events', values['events'], MonitoringAutoMsgMirrorSessionWatchHelperWatchEvent);
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
            this.fillFormArray<MonitoringAutoMsgMirrorSessionWatchHelperWatchEvent>('events', this['events'], MonitoringAutoMsgMirrorSessionWatchHelperWatchEvent);
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this.fillModelArray<MonitoringAutoMsgMirrorSessionWatchHelperWatchEvent>(this, 'events', this['events'], MonitoringAutoMsgMirrorSessionWatchHelperWatchEvent);
        }
    }
}

