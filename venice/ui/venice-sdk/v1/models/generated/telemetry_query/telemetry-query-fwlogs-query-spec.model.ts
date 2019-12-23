/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { Telemetry_queryFwlogsQuerySpec_actions,  Telemetry_queryFwlogsQuerySpec_actions_uihint  } from './enums';
import { Telemetry_queryFwlogsQuerySpec_directions,  Telemetry_queryFwlogsQuerySpec_directions_uihint  } from './enums';
import { Telemetry_queryPaginationSpec, ITelemetry_queryPaginationSpec } from './telemetry-query-pagination-spec.model';
import { Telemetry_queryFwlogsQuerySpec_sort_order,  } from './enums';

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
    'reporter-ids'?: Array<string>;
    'start-time'?: Date;
    'end-time'?: Date;
    'pagination'?: ITelemetry_queryPaginationSpec;
    'sort-order': Telemetry_queryFwlogsQuerySpec_sort_order;
}


export class Telemetry_queryFwlogsQuerySpec extends BaseModel implements ITelemetry_queryFwlogsQuerySpec {
    /** OR of sources IPs to be matchedshould be a valid v4 or v6 IP address */
    'source-ips': Array<string> = null;
    /** OR of dest IPs to be matchedshould be a valid v4 or v6 IP address */
    'dest-ips': Array<string> = null;
    /** OR of source ports to be matchedvalue should be between 0 and 65535 */
    'source-ports': Array<number> = null;
    /** OR of dest ports to be matchedvalue should be between 0 and 65535 */
    'dest-ports': Array<number> = null;
    /** OR of protocols to be matched */
    'protocols': Array<string> = null;
    /** OR of actions to be matched */
    'actions': Array<Telemetry_queryFwlogsQuerySpec_actions> = null;
    /** OR of directions to be matched */
    'directions': Array<Telemetry_queryFwlogsQuerySpec_directions> = null;
    /** OR of ruleID to be matched */
    'rule-ids': Array<string> = null;
    /** OR of policy names to be matched */
    'policy-names': Array<string> = null;
    /** OR of reporter names to be matched */
    'reporter-ids': Array<string> = null;
    /** StartTime selects all logs with timestamp greater than the StartTime, example 2018-10-18T00:12:00Z */
    'start-time': Date = null;
    /** EndTime selects all logs with timestamp less than the EndTime, example 2018-09-18T00:12:00Z */
    'end-time': Date = null;
    /** PaginationSpec specifies the number of series to include */
    'pagination': Telemetry_queryPaginationSpec = null;
    /** SortOrder specifies time ordering of results */
    'sort-order': Telemetry_queryFwlogsQuerySpec_sort_order = null;
    public static propInfo: { [prop in keyof ITelemetry_queryFwlogsQuerySpec]: PropInfoItem } = {
        'source-ips': {
            description:  `OR of sources IPs to be matchedshould be a valid v4 or v6 IP address`,
            hint:  '10.1.1.1, ff02::5 ',
            required: true,
            type: 'Array<string>'
        },
        'dest-ips': {
            description:  `OR of dest IPs to be matchedshould be a valid v4 or v6 IP address`,
            hint:  '10.1.1.1, ff02::5 ',
            required: true,
            type: 'Array<string>'
        },
        'source-ports': {
            description:  `OR of source ports to be matchedvalue should be between 0 and 65535`,
            required: true,
            type: 'Array<number>'
        },
        'dest-ports': {
            description:  `OR of dest ports to be matchedvalue should be between 0 and 65535`,
            required: true,
            type: 'Array<number>'
        },
        'protocols': {
            description:  `OR of protocols to be matched`,
            required: false,
            type: 'Array<string>'
        },
        'actions': {
            enum: Telemetry_queryFwlogsQuerySpec_actions_uihint,
            default: 'allow',
            description:  `OR of actions to be matched`,
            required: true,
            type: 'Array<string>'
        },
        'directions': {
            enum: Telemetry_queryFwlogsQuerySpec_directions_uihint,
            default: 'from-host',
            description:  `OR of directions to be matched`,
            required: true,
            type: 'Array<string>'
        },
        'rule-ids': {
            description:  `OR of ruleID to be matched`,
            required: false,
            type: 'Array<string>'
        },
        'policy-names': {
            description:  `OR of policy names to be matched`,
            required: false,
            type: 'Array<string>'
        },
        'reporter-ids': {
            description:  `OR of reporter names to be matched`,
            required: false,
            type: 'Array<string>'
        },
        'start-time': {
            description:  `StartTime selects all logs with timestamp greater than the StartTime, example 2018-10-18T00:12:00Z`,
            required: false,
            type: 'Date'
        },
        'end-time': {
            description:  `EndTime selects all logs with timestamp less than the EndTime, example 2018-09-18T00:12:00Z`,
            required: false,
            type: 'Date'
        },
        'pagination': {
            description:  `PaginationSpec specifies the number of series to include`,
            required: false,
            type: 'object'
        },
        'sort-order': {
            enum: Telemetry_queryFwlogsQuerySpec_sort_order,
            default: 'descending',
            description:  `SortOrder specifies time ordering of results`,
            required: true,
            type: 'string'
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
        this['reporter-ids'] = new Array<string>();
        this['pagination'] = new Telemetry_queryPaginationSpec();
        this._inputValue = values;
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
        if (values && values['reporter-ids'] != null) {
            this['reporter-ids'] = values['reporter-ids'];
        } else if (fillDefaults && Telemetry_queryFwlogsQuerySpec.hasDefaultValue('reporter-ids')) {
            this['reporter-ids'] = [ Telemetry_queryFwlogsQuerySpec.propInfo['reporter-ids'].default];
        } else {
            this['reporter-ids'] = [];
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
        if (values && values['sort-order'] != null) {
            this['sort-order'] = values['sort-order'];
        } else if (fillDefaults && Telemetry_queryFwlogsQuerySpec.hasDefaultValue('sort-order')) {
            this['sort-order'] = <Telemetry_queryFwlogsQuerySpec_sort_order>  Telemetry_queryFwlogsQuerySpec.propInfo['sort-order'].default;
        } else {
            this['sort-order'] = null
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
                'reporter-ids': CustomFormControl(new FormControl(this['reporter-ids']), Telemetry_queryFwlogsQuerySpec.propInfo['reporter-ids']),
                'start-time': CustomFormControl(new FormControl(this['start-time']), Telemetry_queryFwlogsQuerySpec.propInfo['start-time']),
                'end-time': CustomFormControl(new FormControl(this['end-time']), Telemetry_queryFwlogsQuerySpec.propInfo['end-time']),
                'pagination': CustomFormGroup(this['pagination'].$formGroup, Telemetry_queryFwlogsQuerySpec.propInfo['pagination'].required),
                'sort-order': CustomFormControl(new FormControl(this['sort-order'], [required, enumValidator(Telemetry_queryFwlogsQuerySpec_sort_order), ]), Telemetry_queryFwlogsQuerySpec.propInfo['sort-order']),
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
            this._formGroup.controls['reporter-ids'].setValue(this['reporter-ids']);
            this._formGroup.controls['start-time'].setValue(this['start-time']);
            this._formGroup.controls['end-time'].setValue(this['end-time']);
            this['pagination'].setFormGroupValuesToBeModelValues();
            this._formGroup.controls['sort-order'].setValue(this['sort-order']);
        }
    }
}

