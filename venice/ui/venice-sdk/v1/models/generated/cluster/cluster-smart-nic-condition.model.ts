/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { ClusterSmartNICCondition_type,  ClusterSmartNICCondition_type_uihint  } from './enums';
import { ClusterSmartNICCondition_status,  ClusterSmartNICCondition_status_uihint  } from './enums';

export interface IClusterSmartNICCondition {
    'type': ClusterSmartNICCondition_type;
    'status': ClusterSmartNICCondition_status;
    'last-transition-time'?: string;
    'reason'?: string;
    'message'?: string;
}


export class ClusterSmartNICCondition extends BaseModel implements IClusterSmartNICCondition {
    'type': ClusterSmartNICCondition_type = null;
    'status': ClusterSmartNICCondition_status = null;
    'last-transition-time': string = null;
    'reason': string = null;
    /** A detailed message indicating details about the transition. */
    'message': string = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'type': {
            enum: ClusterSmartNICCondition_type_uihint,
            default: 'HEALTHY',
            required: true,
            type: 'string'
        },
        'status': {
            enum: ClusterSmartNICCondition_status_uihint,
            default: 'UNKNOWN',
            required: true,
            type: 'string'
        },
        'last-transition-time': {
            required: false,
            type: 'string'
        },
        'reason': {
            required: false,
            type: 'string'
        },
        'message': {
            description:  'A detailed message indicating details about the transition.',
            required: false,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return ClusterSmartNICCondition.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return ClusterSmartNICCondition.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (ClusterSmartNICCondition.propInfo[prop] != null &&
                        ClusterSmartNICCondition.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['type'] != null) {
            this['type'] = values['type'];
        } else if (fillDefaults && ClusterSmartNICCondition.hasDefaultValue('type')) {
            this['type'] = <ClusterSmartNICCondition_type>  ClusterSmartNICCondition.propInfo['type'].default;
        } else {
            this['type'] = null
        }
        if (values && values['status'] != null) {
            this['status'] = values['status'];
        } else if (fillDefaults && ClusterSmartNICCondition.hasDefaultValue('status')) {
            this['status'] = <ClusterSmartNICCondition_status>  ClusterSmartNICCondition.propInfo['status'].default;
        } else {
            this['status'] = null
        }
        if (values && values['last-transition-time'] != null) {
            this['last-transition-time'] = values['last-transition-time'];
        } else if (fillDefaults && ClusterSmartNICCondition.hasDefaultValue('last-transition-time')) {
            this['last-transition-time'] = ClusterSmartNICCondition.propInfo['last-transition-time'].default;
        } else {
            this['last-transition-time'] = null
        }
        if (values && values['reason'] != null) {
            this['reason'] = values['reason'];
        } else if (fillDefaults && ClusterSmartNICCondition.hasDefaultValue('reason')) {
            this['reason'] = ClusterSmartNICCondition.propInfo['reason'].default;
        } else {
            this['reason'] = null
        }
        if (values && values['message'] != null) {
            this['message'] = values['message'];
        } else if (fillDefaults && ClusterSmartNICCondition.hasDefaultValue('message')) {
            this['message'] = ClusterSmartNICCondition.propInfo['message'].default;
        } else {
            this['message'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'type': CustomFormControl(new FormControl(this['type'], [required, enumValidator(ClusterSmartNICCondition_type), ]), ClusterSmartNICCondition.propInfo['type']),
                'status': CustomFormControl(new FormControl(this['status'], [required, enumValidator(ClusterSmartNICCondition_status), ]), ClusterSmartNICCondition.propInfo['status']),
                'last-transition-time': CustomFormControl(new FormControl(this['last-transition-time']), ClusterSmartNICCondition.propInfo['last-transition-time']),
                'reason': CustomFormControl(new FormControl(this['reason']), ClusterSmartNICCondition.propInfo['reason']),
                'message': CustomFormControl(new FormControl(this['message']), ClusterSmartNICCondition.propInfo['message']),
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
            this._formGroup.controls['status'].setValue(this['status']);
            this._formGroup.controls['last-transition-time'].setValue(this['last-transition-time']);
            this._formGroup.controls['reason'].setValue(this['reason']);
            this._formGroup.controls['message'].setValue(this['message']);
        }
    }
}

