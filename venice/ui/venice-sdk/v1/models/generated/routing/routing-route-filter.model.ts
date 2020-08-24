/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { ApiObjectMeta, IApiObjectMeta } from './api-object-meta.model';

export interface IRoutingRouteFilter {
    'kind'?: string;
    'api-version'?: string;
    'meta'?: IApiObjectMeta;
    'instance'?: string;
    'route'?: string;
    'type'?: string;
    'extcomm'?: string;
    'vnid'?: string;
    'rtype'?: string;
    'nhop'?: string;
    '_ui'?: any;
}


export class RoutingRouteFilter extends BaseModel implements IRoutingRouteFilter {
    /** Field for holding arbitrary ui state */
    '_ui': any = {};
    'kind': string = null;
    'api-version': string = null;
    'meta': ApiObjectMeta = null;
    'instance': string = null;
    'route': string = null;
    'type': string = null;
    'extcomm': string = null;
    'vnid': string = null;
    'rtype': string = null;
    'nhop': string = null;
    public static propInfo: { [prop in keyof IRoutingRouteFilter]: PropInfoItem } = {
        'kind': {
            required: false,
            type: 'string'
        },
        'api-version': {
            required: false,
            type: 'string'
        },
        'meta': {
            required: false,
            type: 'object'
        },
        'instance': {
            required: false,
            type: 'string'
        },
        'route': {
            required: false,
            type: 'string'
        },
        'type': {
            required: false,
            type: 'string'
        },
        'extcomm': {
            required: false,
            type: 'string'
        },
        'vnid': {
            required: false,
            type: 'string'
        },
        'rtype': {
            required: false,
            type: 'string'
        },
        'nhop': {
            required: false,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return RoutingRouteFilter.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return RoutingRouteFilter.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (RoutingRouteFilter.propInfo[prop] != null &&
                        RoutingRouteFilter.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['meta'] = new ApiObjectMeta();
        this._inputValue = values;
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['_ui']) {
            this['_ui'] = values['_ui']
        }
        if (values && values['kind'] != null) {
            this['kind'] = values['kind'];
        } else if (fillDefaults && RoutingRouteFilter.hasDefaultValue('kind')) {
            this['kind'] = RoutingRouteFilter.propInfo['kind'].default;
        } else {
            this['kind'] = null
        }
        if (values && values['api-version'] != null) {
            this['api-version'] = values['api-version'];
        } else if (fillDefaults && RoutingRouteFilter.hasDefaultValue('api-version')) {
            this['api-version'] = RoutingRouteFilter.propInfo['api-version'].default;
        } else {
            this['api-version'] = null
        }
        if (values) {
            this['meta'].setValues(values['meta'], fillDefaults);
        } else {
            this['meta'].setValues(null, fillDefaults);
        }
        if (values && values['instance'] != null) {
            this['instance'] = values['instance'];
        } else if (fillDefaults && RoutingRouteFilter.hasDefaultValue('instance')) {
            this['instance'] = RoutingRouteFilter.propInfo['instance'].default;
        } else {
            this['instance'] = null
        }
        if (values && values['route'] != null) {
            this['route'] = values['route'];
        } else if (fillDefaults && RoutingRouteFilter.hasDefaultValue('route')) {
            this['route'] = RoutingRouteFilter.propInfo['route'].default;
        } else {
            this['route'] = null
        }
        if (values && values['type'] != null) {
            this['type'] = values['type'];
        } else if (fillDefaults && RoutingRouteFilter.hasDefaultValue('type')) {
            this['type'] = RoutingRouteFilter.propInfo['type'].default;
        } else {
            this['type'] = null
        }
        if (values && values['extcomm'] != null) {
            this['extcomm'] = values['extcomm'];
        } else if (fillDefaults && RoutingRouteFilter.hasDefaultValue('extcomm')) {
            this['extcomm'] = RoutingRouteFilter.propInfo['extcomm'].default;
        } else {
            this['extcomm'] = null
        }
        if (values && values['vnid'] != null) {
            this['vnid'] = values['vnid'];
        } else if (fillDefaults && RoutingRouteFilter.hasDefaultValue('vnid')) {
            this['vnid'] = RoutingRouteFilter.propInfo['vnid'].default;
        } else {
            this['vnid'] = null
        }
        if (values && values['rtype'] != null) {
            this['rtype'] = values['rtype'];
        } else if (fillDefaults && RoutingRouteFilter.hasDefaultValue('rtype')) {
            this['rtype'] = RoutingRouteFilter.propInfo['rtype'].default;
        } else {
            this['rtype'] = null
        }
        if (values && values['nhop'] != null) {
            this['nhop'] = values['nhop'];
        } else if (fillDefaults && RoutingRouteFilter.hasDefaultValue('nhop')) {
            this['nhop'] = RoutingRouteFilter.propInfo['nhop'].default;
        } else {
            this['nhop'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'kind': CustomFormControl(new FormControl(this['kind']), RoutingRouteFilter.propInfo['kind']),
                'api-version': CustomFormControl(new FormControl(this['api-version']), RoutingRouteFilter.propInfo['api-version']),
                'meta': CustomFormGroup(this['meta'].$formGroup, RoutingRouteFilter.propInfo['meta'].required),
                'instance': CustomFormControl(new FormControl(this['instance']), RoutingRouteFilter.propInfo['instance']),
                'route': CustomFormControl(new FormControl(this['route']), RoutingRouteFilter.propInfo['route']),
                'type': CustomFormControl(new FormControl(this['type']), RoutingRouteFilter.propInfo['type']),
                'extcomm': CustomFormControl(new FormControl(this['extcomm']), RoutingRouteFilter.propInfo['extcomm']),
                'vnid': CustomFormControl(new FormControl(this['vnid']), RoutingRouteFilter.propInfo['vnid']),
                'rtype': CustomFormControl(new FormControl(this['rtype']), RoutingRouteFilter.propInfo['rtype']),
                'nhop': CustomFormControl(new FormControl(this['nhop']), RoutingRouteFilter.propInfo['nhop']),
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('meta') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('meta').get(field);
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
            this['meta'].setFormGroupValuesToBeModelValues();
            this._formGroup.controls['instance'].setValue(this['instance']);
            this._formGroup.controls['route'].setValue(this['route']);
            this._formGroup.controls['type'].setValue(this['type']);
            this._formGroup.controls['extcomm'].setValue(this['extcomm']);
            this._formGroup.controls['vnid'].setValue(this['vnid']);
            this._formGroup.controls['rtype'].setValue(this['rtype']);
            this._formGroup.controls['nhop'].setValue(this['nhop']);
        }
    }
}

