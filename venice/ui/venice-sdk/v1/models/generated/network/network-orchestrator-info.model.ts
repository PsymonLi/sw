/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';


export interface INetworkOrchestratorInfo {
    'orchestrator-name'?: string;
    'namespace'?: string;
}


export class NetworkOrchestratorInfo extends BaseModel implements INetworkOrchestratorInfo {
    /** Name of Orchestrator object to which this network should be applied to */
    'orchestrator-name': string = null;
    /** Namespace in the orchestrator in which this network should be created in. */
    'namespace': string = null;
    public static propInfo: { [prop in keyof INetworkOrchestratorInfo]: PropInfoItem } = {
        'orchestrator-name': {
            description:  `Name of Orchestrator object to which this network should be applied to`,
            required: false,
            type: 'string'
        },
        'namespace': {
            description:  `Namespace in the orchestrator in which this network should be created in.`,
            required: false,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return NetworkOrchestratorInfo.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return NetworkOrchestratorInfo.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (NetworkOrchestratorInfo.propInfo[prop] != null &&
                        NetworkOrchestratorInfo.propInfo[prop].default != null);
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
        if (values && values['orchestrator-name'] != null) {
            this['orchestrator-name'] = values['orchestrator-name'];
        } else if (fillDefaults && NetworkOrchestratorInfo.hasDefaultValue('orchestrator-name')) {
            this['orchestrator-name'] = NetworkOrchestratorInfo.propInfo['orchestrator-name'].default;
        } else {
            this['orchestrator-name'] = null
        }
        if (values && values['namespace'] != null) {
            this['namespace'] = values['namespace'];
        } else if (fillDefaults && NetworkOrchestratorInfo.hasDefaultValue('namespace')) {
            this['namespace'] = NetworkOrchestratorInfo.propInfo['namespace'].default;
        } else {
            this['namespace'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'orchestrator-name': CustomFormControl(new FormControl(this['orchestrator-name']), NetworkOrchestratorInfo.propInfo['orchestrator-name']),
                'namespace': CustomFormControl(new FormControl(this['namespace']), NetworkOrchestratorInfo.propInfo['namespace']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['orchestrator-name'].setValue(this['orchestrator-name']);
            this._formGroup.controls['namespace'].setValue(this['namespace']);
        }
    }
}

