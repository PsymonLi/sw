/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { Telemetry_queryFwlogsQuerySpec_actions,  Telemetry_queryFwlogsQuerySpec_actions_uihint  } from './enums';
import { Telemetry_queryFwlogsQuerySpec_directions,  Telemetry_queryFwlogsQuerySpec_directions_uihint  } from './enums';
import { Telemetry_queryPaginationSpec, ITelemetry_queryPaginationSpec } from './telemetry-query-pagination-spec.model';

export interface ITelemetry_queryFwlogsQuerySpec {
    'source-ips': Array<string>;
    'dest-ips': Array<string>;
    'source-ports': Array<number>;
    'dest-ports': Array<number>;
    'protocols'?: Array<string>;
    'actions': Array<Telemetry_queryFwlogsQuerySpec_actions>;
    'directions': Array<Telemetry_queryFwlogsQuerySpec_directions>;
    'rule-ids'?: Array<string>;
    'policy-names'?: Array<string>;
    'start-time'?: Date;
    'end-time'?: Date;
    'pagination'?: ITelemetry_queryPaginationSpec;
}


export class Telemetry_queryFwlogsQuerySpec extends BaseModel implements ITelemetry_queryFwlogsQuerySpec {
    /** should be a valid v4 or v6 IP address
     */
    'source-ips': Array<string> = null;
    /** should be a valid v4 or v6 IP address
     */
    'dest-ips': Array<string> = null;
    /** value should be between 0 and 65535
     */
    'source-ports': Array<number> = null;
    /** value should be between 0 and 65535
     */
    'dest-ports': Array<number> = null;
    'protocols': Array<string> = null;
    'actions': Array<Telemetry_queryFwlogsQuerySpec_actions> = null;
    'directions': Array<Telemetry_queryFwlogsQuerySpec_directions> = null;
    'rule-ids': Array<string> = null;
    'policy-names': Array<string> = null;
    'start-time': Date = null;
    'end-time': Date = null;
    'pagination': Telemetry_queryPaginationSpec = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'source-ips': {
            description:  'should be a valid v4 or v6 IP address ',
            hint:  '10.1.1.1, ff02::5 ',
            required: true,
            type: 'Array<string>'
        },
        'dest-ips': {
            description:  'should be a valid v4 or v6 IP address ',
            hint:  '10.1.1.1, ff02::5 ',
            required: true,
            type: 'Array<string>'
        },
        'source-ports': {
            description:  'value should be between 0 and 65535 ',
            required: true,
            type: 'Array<number>'
        },
        'dest-ports': {
            description:  'value should be between 0 and 65535 ',
            required: true,
            type: 'Array<number>'
        },
        'protocols': {
            required: false,
            type: 'Array<string>'
        },
        'actions': {
            enum: Telemetry_queryFwlogsQuerySpec_actions_uihint,
            default: 'ALL',
            required: true,
            type: 'Array<string>'
        },
        'directions': {
            enum: Telemetry_queryFwlogsQuerySpec_directions_uihint,
            default: 'DIRECTION_ALL',
            required: true,
            type: 'Array<string>'
        },
        'rule-ids': {
            required: false,
            type: 'Array<string>'
        },
        'policy-names': {
            required: false,
            type: 'Array<string>'
        },
        'start-time': {
            required: false,
            type: 'Date'
        },
        'end-time': {
            required: false,
            type: 'Date'
        },
        'pagination': {
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return Telemetry_queryFwlogsQuerySpec.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return Telemetry_queryFwlogsQuerySpec.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (Telemetry_queryFwlogsQuerySpec.propInfo[prop] != null &&
                        Telemetry_queryFwlogsQuerySpec.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['source-ips'] = new Array<string>();
        this['dest-ips'] = new Array<string>();
        this['source-ports'] = new Array<number>();
        this['dest-ports'] = new Array<number>();
        this['protocols'] = new Array<string>();
        this['actions'] = new Array<Telemetry_queryFwlogsQuerySpec_actions>();
        this['directions'] = new Array<Telemetry_queryFwlogsQuerySpec_directions>();
        this['rule-ids'] = new Array<string>();
        this['policy-names'] = new Array<string>();
        this['pagination'] = new Telemetry_queryPaginationSpec();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['source-ips'] != null) {
            this['source-ips'] = values['source-ips'];
        } else if (fillDefaults && Telemetry_queryFwlogsQuerySpec.hasDefaultValue('source-ips')) {
            this['source-ips'] = [ Telemetry_queryFwlogsQuerySpec.propInfo['source-ips'].default];
        } else {
            this['source-ips'] = [];
        }
        if (values && values['dest-ips'] != null) {
            this['dest-ips'] = values['dest-ips'];
        } else if (fillDefaults && Telemetry_queryFwlogsQuerySpec.hasDefaultValue('dest-ips')) {
            this['dest-ips'] = [ Telemetry_queryFwlogsQuerySpec.propInfo['dest-ips'].default];
        } else {
            this['dest-ips'] = [];
        }
        if (values && values['source-ports'] != null) {
            this['source-ports'] = values['source-ports'];
        } else if (fillDefaults && Telemetry_queryFwlogsQuerySpec.hasDefaultValue('source-ports')) {
            this['source-ports'] = [ Telemetry_queryFwlogsQuerySpec.propInfo['source-ports'].default];
        } else {
            this['source-ports'] = [];
        }
        if (values && values['dest-ports'] != null) {
            this['dest-ports'] = values['dest-ports'];
        } else if (fillDefaults && Telemetry_queryFwlogsQuerySpec.hasDefaultValue('dest-ports')) {
            this['dest-ports'] = [ Telemetry_queryFwlogsQuerySpec.propInfo['dest-ports'].default];
        } else {
            this['dest-ports'] = [];
        }
        if (values && values['protocols'] != null) {
            this['protocols'] = values['protocols'];
        } else if (fillDefaults && Telemetry_queryFwlogsQuerySpec.hasDefaultValue('protocols')) {
            this['protocols'] = [ Telemetry_queryFwlogsQuerySpec.propInfo['protocols'].default];
        } else {
            this['protocols'] = [];
        }
        if (values && values['actions'] != null) {
            this['actions'] = values['actions'];
        } else if (fillDefaults && Telemetry_queryFwlogsQuerySpec.hasDefaultValue('actions')) {
            this['actions'] = [ Telemetry_queryFwlogsQuerySpec.propInfo['actions'].default];
        } else {
            this['actions'] = [];
        }
        if (values && values['directions'] != null) {
            this['directions'] = values['directions'];
        } else if (fillDefaults && Telemetry_queryFwlogsQuerySpec.hasDefaultValue('directions')) {
            this['directions'] = [ Telemetry_queryFwlogsQuerySpec.propInfo['directions'].default];
        } else {
            this['directions'] = [];
        }
        if (values && values['rule-ids'] != null) {
            this['rule-ids'] = values['rule-ids'];
        } else if (fillDefaults && Telemetry_queryFwlogsQuerySpec.hasDefaultValue('rule-ids')) {
            this['rule-ids'] = [ Telemetry_queryFwlogsQuerySpec.propInfo['rule-ids'].default];
        } else {
            this['rule-ids'] = [];
        }
        if (values && values['policy-names'] != null) {
            this['policy-names'] = values['policy-names'];
        } else if (fillDefaults && Telemetry_queryFwlogsQuerySpec.hasDefaultValue('policy-names')) {
            this['policy-names'] = [ Telemetry_queryFwlogsQuerySpec.propInfo['policy-names'].default];
        } else {
            this['policy-names'] = [];
        }
        if (values && values['start-time'] != null) {
            this['start-time'] = values['start-time'];
        } else if (fillDefaults && Telemetry_queryFwlogsQuerySpec.hasDefaultValue('start-time')) {
            this['start-time'] = Telemetry_queryFwlogsQuerySpec.propInfo['start-time'].default;
        } else {
            this['start-time'] = null
        }
        if (values && values['end-time'] != null) {
            this['end-time'] = values['end-time'];
        } else if (fillDefaults && Telemetry_queryFwlogsQuerySpec.hasDefaultValue('end-time')) {
            this['end-time'] = Telemetry_queryFwlogsQuerySpec.propInfo['end-time'].default;
        } else {
            this['end-time'] = null
        }
        if (values) {
            this['pagination'].setValues(values['pagination'], fillDefaults);
        } else {
            this['pagination'].setValues(null, fillDefaults);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'source-ips': CustomFormControl(new FormControl(this['source-ips']), Telemetry_queryFwlogsQuerySpec.propInfo['source-ips']),
                'dest-ips': CustomFormControl(new FormControl(this['dest-ips']), Telemetry_queryFwlogsQuerySpec.propInfo['dest-ips']),
                'source-ports': CustomFormControl(new FormControl(this['source-ports']), Telemetry_queryFwlogsQuerySpec.propInfo['source-ports']),
                'dest-ports': CustomFormControl(new FormControl(this['dest-ports']), Telemetry_queryFwlogsQuerySpec.propInfo['dest-ports']),
                'protocols': CustomFormControl(new FormControl(this['protocols']), Telemetry_queryFwlogsQuerySpec.propInfo['protocols']),
                'actions': CustomFormControl(new FormControl(this['actions']), Telemetry_queryFwlogsQuerySpec.propInfo['actions']),
                'directions': CustomFormControl(new FormControl(this['directions']), Telemetry_queryFwlogsQuerySpec.propInfo['directions']),
                'rule-ids': CustomFormControl(new FormControl(this['rule-ids']), Telemetry_queryFwlogsQuerySpec.propInfo['rule-ids']),
                'policy-names': CustomFormControl(new FormControl(this['policy-names']), Telemetry_queryFwlogsQuerySpec.propInfo['policy-names']),
                'start-time': CustomFormControl(new FormControl(this['start-time']), Telemetry_queryFwlogsQuerySpec.propInfo['start-time']),
                'end-time': CustomFormControl(new FormControl(this['end-time']), Telemetry_queryFwlogsQuerySpec.propInfo['end-time']),
                'pagination': CustomFormGroup(this['pagination'].$formGroup, Telemetry_queryFwlogsQuerySpec.propInfo['pagination'].required),
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('pagination') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('pagination').get(field);
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
            this._formGroup.controls['source-ips'].setValue(this['source-ips']);
            this._formGroup.controls['dest-ips'].setValue(this['dest-ips']);
            this._formGroup.controls['source-ports'].setValue(this['source-ports']);
            this._formGroup.controls['dest-ports'].setValue(this['dest-ports']);
            this._formGroup.controls['protocols'].setValue(this['protocols']);
            this._formGroup.controls['actions'].setValue(this['actions']);
            this._formGroup.controls['directions'].setValue(this['directions']);
            this._formGroup.controls['rule-ids'].setValue(this['rule-ids']);
            this._formGroup.controls['policy-names'].setValue(this['policy-names']);
            this._formGroup.controls['start-time'].setValue(this['start-time']);
            this._formGroup.controls['end-time'].setValue(this['end-time']);
            this['pagination'].setFormGroupValuesToBeModelValues();
        }
    }
}

