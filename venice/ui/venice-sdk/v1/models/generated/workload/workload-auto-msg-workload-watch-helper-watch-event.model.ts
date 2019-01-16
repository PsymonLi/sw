/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator } from './validators';
import { BaseModel, PropInfoItem } from './base-model';

import { WorkloadWorkload, IWorkloadWorkload } from './workload-workload.model';

export interface IWorkloadAutoMsgWorkloadWatchHelperWatchEvent {
    'type'?: string;
    'object'?: IWorkloadWorkload;
}


export class WorkloadAutoMsgWorkloadWatchHelperWatchEvent extends BaseModel implements IWorkloadAutoMsgWorkloadWatchHelperWatchEvent {
    'type': string = null;
    'object': WorkloadWorkload = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'type': {
            type: 'string'
        },
        'object': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return WorkloadAutoMsgWorkloadWatchHelperWatchEvent.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return WorkloadAutoMsgWorkloadWatchHelperWatchEvent.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (WorkloadAutoMsgWorkloadWatchHelperWatchEvent.propInfo[prop] != null &&
                        WorkloadAutoMsgWorkloadWatchHelperWatchEvent.propInfo[prop].default != null &&
                        WorkloadAutoMsgWorkloadWatchHelperWatchEvent.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['object'] = new WorkloadWorkload();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['type'] != null) {
            this['type'] = values['type'];
        } else if (fillDefaults && WorkloadAutoMsgWorkloadWatchHelperWatchEvent.hasDefaultValue('type')) {
            this['type'] = WorkloadAutoMsgWorkloadWatchHelperWatchEvent.propInfo['type'].default;
        }
        if (values) {
            this['object'].setValues(values['object']);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'type': new FormControl(this['type']),
                'object': this['object'].$formGroup,
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['type'].setValue(this['type']);
            this['object'].setFormGroupValuesToBeModelValues();
        }
    }
}

