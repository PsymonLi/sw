/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { ClusterAutoMsgClusterWatchHelperWatchEvent, IClusterAutoMsgClusterWatchHelperWatchEvent } from './cluster-auto-msg-cluster-watch-helper-watch-event.model';

export interface IClusterAutoMsgClusterWatchHelper {
    'events'?: Array<IClusterAutoMsgClusterWatchHelperWatchEvent>;
}


export class ClusterAutoMsgClusterWatchHelper extends BaseModel implements IClusterAutoMsgClusterWatchHelper {
    'events': Array<ClusterAutoMsgClusterWatchHelperWatchEvent> = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'events': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return ClusterAutoMsgClusterWatchHelper.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return ClusterAutoMsgClusterWatchHelper.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (ClusterAutoMsgClusterWatchHelper.propInfo[prop] != null &&
                        ClusterAutoMsgClusterWatchHelper.propInfo[prop].default != null &&
                        ClusterAutoMsgClusterWatchHelper.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['events'] = new Array<ClusterAutoMsgClusterWatchHelperWatchEvent>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values) {
            this.fillModelArray<ClusterAutoMsgClusterWatchHelperWatchEvent>(this, 'events', values['events'], ClusterAutoMsgClusterWatchHelperWatchEvent);
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
            this.fillFormArray<ClusterAutoMsgClusterWatchHelperWatchEvent>('events', this['events'], ClusterAutoMsgClusterWatchHelperWatchEvent);
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this.fillModelArray<ClusterAutoMsgClusterWatchHelperWatchEvent>(this, 'events', this['events'], ClusterAutoMsgClusterWatchHelperWatchEvent);
        }
    }
}

