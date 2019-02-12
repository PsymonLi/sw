/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { LabelsSelector, ILabelsSelector } from './labels-selector.model';

export interface ITechSupportRequestSpecNodeSelectorSpec {
    'names'?: Array<string>;
    'labels'?: ILabelsSelector;
}


export class TechSupportRequestSpecNodeSelectorSpec extends BaseModel implements ITechSupportRequestSpecNodeSelectorSpec {
    'names': Array<string> = null;
    'labels': LabelsSelector = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'names': {
            type: 'Array<string>'
        },
        'labels': {
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return TechSupportRequestSpecNodeSelectorSpec.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return TechSupportRequestSpecNodeSelectorSpec.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (TechSupportRequestSpecNodeSelectorSpec.propInfo[prop] != null &&
                        TechSupportRequestSpecNodeSelectorSpec.propInfo[prop].default != null &&
                        TechSupportRequestSpecNodeSelectorSpec.propInfo[prop].default != '');
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['names'] = new Array<string>();
        this['labels'] = new LabelsSelector();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['names'] != null) {
            this['names'] = values['names'];
        } else if (fillDefaults && TechSupportRequestSpecNodeSelectorSpec.hasDefaultValue('names')) {
            this['names'] = [ TechSupportRequestSpecNodeSelectorSpec.propInfo['names'].default];
        } else {
            this['names'] = [];
        }
        if (values) {
            this['labels'].setValues(values['labels'], fillDefaults);
        } else {
            this['labels'].setValues(null, fillDefaults);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'names': CustomFormControl(new FormControl(this['names']), TechSupportRequestSpecNodeSelectorSpec.propInfo['names'].description),
                'labels': this['labels'].$formGroup,
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['names'].setValue(this['names']);
            this['labels'].setFormGroupValuesToBeModelValues();
        }
    }
}

