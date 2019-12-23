/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';


export interface IMonitoringAppProtoSelector {
    'proto-ports'?: Array<string>;
    'applications'?: Array<string>;
}


export class MonitoringAppProtoSelector extends BaseModel implements IMonitoringAppProtoSelector {
    /** ports - Includes protocol name and port Eg ["tcp/1234", "udp"]should be a valid layer3 or layer 4 protocol and port/type */
    'proto-ports': Array<string> = null;
    /** Apps - E.g. ["Redis"] */
    'applications': Array<string> = null;
    public static propInfo: { [prop in keyof IMonitoringAppProtoSelector]: PropInfoItem } = {
        'proto-ports': {
            description:  `Ports - Includes protocol name and port Eg ["tcp/1234", "udp"]should be a valid layer3 or layer 4 protocol and port/type`,
            hint:  'tcp/1234, arp',
            required: false,
            type: 'Array<string>'
        },
        'applications': {
            description:  `Apps - E.g. ["Redis"]`,
            required: false,
            type: 'Array<string>'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return MonitoringAppProtoSelector.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return MonitoringAppProtoSelector.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (MonitoringAppProtoSelector.propInfo[prop] != null &&
                        MonitoringAppProtoSelector.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['proto-ports'] = new Array<string>();
        this['applications'] = new Array<string>();
        this._inputValue = values;
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['proto-ports'] != null) {
            this['proto-ports'] = values['proto-ports'];
        } else if (fillDefaults && MonitoringAppProtoSelector.hasDefaultValue('proto-ports')) {
            this['proto-ports'] = [ MonitoringAppProtoSelector.propInfo['proto-ports'].default];
        } else {
            this['proto-ports'] = [];
        }
        if (values && values['applications'] != null) {
            this['applications'] = values['applications'];
        } else if (fillDefaults && MonitoringAppProtoSelector.hasDefaultValue('applications')) {
            this['applications'] = [ MonitoringAppProtoSelector.propInfo['applications'].default];
        } else {
            this['applications'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'proto-ports': CustomFormControl(new FormControl(this['proto-ports']), MonitoringAppProtoSelector.propInfo['proto-ports']),
                'applications': CustomFormControl(new FormControl(this['applications']), MonitoringAppProtoSelector.propInfo['applications']),
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['proto-ports'].setValue(this['proto-ports']);
            this._formGroup.controls['applications'].setValue(this['applications']);
        }
    }
}

