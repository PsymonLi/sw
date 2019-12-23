/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { RolloutRolloutActionStatus_state,  } from './enums';

export interface IRolloutRolloutActionStatus {
    'state': RolloutRolloutActionStatus_state;
    'completion-percent'?: number;
    'start-time'?: Date;
    'end-time'?: Date;
    'prev-version'?: string;
}


export class RolloutRolloutActionStatus extends BaseModel implements IRolloutRolloutActionStatus {
    'state': RolloutRolloutActionStatus_state = null;
    /** Heuristic value of percentage completion of the rollout */
    'completion-percent': number = null;
    /** Start time of Rollout */
    'start-time': Date = null;
    /** End time of Rollout */
    'end-time': Date = null;
    /** Version of the cluster before the start of rollout */
    'prev-version': string = null;
    public static propInfo: { [prop in keyof IRolloutRolloutActionStatus]: PropInfoItem } = {
        'state': {
            enum: RolloutRolloutActionStatus_state,
            default: 'progressing',
            required: true,
            type: 'string'
        },
        'completion-percent': {
            description:  `Heuristic value of percentage completion of the rollout`,
            required: false,
            type: 'number'
        },
        'start-time': {
            description:  `Start time of Rollout`,
            required: false,
            type: 'Date'
        },
        'end-time': {
            description:  `End time of Rollout`,
            required: false,
            type: 'Date'
        },
        'prev-version': {
            description:  `Version of the cluster before the start of rollout`,
            required: false,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return RolloutRolloutActionStatus.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return RolloutRolloutActionStatus.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (RolloutRolloutActionStatus.propInfo[prop] != null &&
                        RolloutRolloutActionStatus.propInfo[prop].default != null);
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
        if (values && values['state'] != null) {
            this['state'] = values['state'];
        } else if (fillDefaults && RolloutRolloutActionStatus.hasDefaultValue('state')) {
            this['state'] = <RolloutRolloutActionStatus_state>  RolloutRolloutActionStatus.propInfo['state'].default;
        } else {
            this['state'] = null
        }
        if (values && values['completion-percent'] != null) {
            this['completion-percent'] = values['completion-percent'];
        } else if (fillDefaults && RolloutRolloutActionStatus.hasDefaultValue('completion-percent')) {
            this['completion-percent'] = RolloutRolloutActionStatus.propInfo['completion-percent'].default;
        } else {
            this['completion-percent'] = null
        }
        if (values && values['start-time'] != null) {
            this['start-time'] = values['start-time'];
        } else if (fillDefaults && RolloutRolloutActionStatus.hasDefaultValue('start-time')) {
            this['start-time'] = RolloutRolloutActionStatus.propInfo['start-time'].default;
        } else {
            this['start-time'] = null
        }
        if (values && values['end-time'] != null) {
            this['end-time'] = values['end-time'];
        } else if (fillDefaults && RolloutRolloutActionStatus.hasDefaultValue('end-time')) {
            this['end-time'] = RolloutRolloutActionStatus.propInfo['end-time'].default;
        } else {
            this['end-time'] = null
        }
        if (values && values['prev-version'] != null) {
            this['prev-version'] = values['prev-version'];
        } else if (fillDefaults && RolloutRolloutActionStatus.hasDefaultValue('prev-version')) {
            this['prev-version'] = RolloutRolloutActionStatus.propInfo['prev-version'].default;
        } else {
            this['prev-version'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'state': CustomFormControl(new FormControl(this['state'], [required, enumValidator(RolloutRolloutActionStatus_state), ]), RolloutRolloutActionStatus.propInfo['state']),
                'completion-percent': CustomFormControl(new FormControl(this['completion-percent']), RolloutRolloutActionStatus.propInfo['completion-percent']),
                'start-time': CustomFormControl(new FormControl(this['start-time']), RolloutRolloutActionStatus.propInfo['start-time']),
                'end-time': CustomFormControl(new FormControl(this['end-time']), RolloutRolloutActionStatus.propInfo['end-time']),
                'prev-version': CustomFormControl(new FormControl(this['prev-version']), RolloutRolloutActionStatus.propInfo['prev-version']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['state'].setValue(this['state']);
            this._formGroup.controls['completion-percent'].setValue(this['completion-percent']);
            this._formGroup.controls['start-time'].setValue(this['start-time']);
            this._formGroup.controls['end-time'].setValue(this['end-time']);
            this._formGroup.controls['prev-version'].setValue(this['prev-version']);
        }
    }
}

