/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from './base-model';

import { ClusterSmartNICStatus_admission_phase,  ClusterSmartNICStatus_admission_phase_uihint  } from './enums';
import { ClusterSmartNICCondition, IClusterSmartNICCondition } from './cluster-smart-nic-condition.model';
import { ClusterIPConfig, IClusterIPConfig } from './cluster-ip-config.model';
import { ClusterSmartNICInfo, IClusterSmartNICInfo } from './cluster-smart-nic-info.model';

export interface IClusterSmartNICStatus {
    'admission-phase': ClusterSmartNICStatus_admission_phase;
    'conditions'?: Array<IClusterSmartNICCondition>;
    'serial-num'?: string;
    'primary-mac'?: string;
    'ip-config'?: IClusterIPConfig;
    'system-info'?: IClusterSmartNICInfo;
    'interfaces'?: Array<string>;
    'smartNicVersion'?: string;
    'smartNicSku'?: string;
}


export class ClusterSmartNICStatus extends BaseModel implements IClusterSmartNICStatus {
    /** Current admission phase of the SmartNIC.
    When auto-admission is enabled, AdmissionPhase will be set to NIC_ADMITTED
    by CMD for validated NICs.
    When auto-admission is not enabled, AdmissionPhase will be set to NIC_PENDING
    by CMD for validated NICs since it requires manual approval.
    To admit the NIC as a part of manual admission, user is expected to
    set Spec.Admit to true for the NICs that are in NIC_PENDING
    state. Note : Whitelist mode is not supported yet. */
    'admission-phase': ClusterSmartNICStatus_admission_phase = null;
    'conditions': Array<ClusterSmartNICCondition> = null;
    'serial-num': string = null;
    'primary-mac': string = null;
    'ip-config': ClusterIPConfig = null;
    'system-info': ClusterSmartNICInfo = null;
    'interfaces': Array<string> = null;
    'smartNicVersion': string = null;
    'smartNicSku': string = null;
    public static propInfo: { [prop: string]: PropInfoItem } = {
        'admission-phase': {
            enum: ClusterSmartNICStatus_admission_phase_uihint,
            default: 'UNKNOWN',
            description:  'Current admission phase of the SmartNIC. When auto-admission is enabled, AdmissionPhase will be set to NIC_ADMITTED by CMD for validated NICs. When auto-admission is not enabled, AdmissionPhase will be set to NIC_PENDING by CMD for validated NICs since it requires manual approval. To admit the NIC as a part of manual admission, user is expected to set Spec.Admit to true for the NICs that are in NIC_PENDING state. Note : Whitelist mode is not supported yet.',
            required: true,
            type: 'string'
        },
        'conditions': {
            required: false,
            type: 'object'
        },
        'serial-num': {
            required: false,
            type: 'string'
        },
        'primary-mac': {
            required: false,
            type: 'string'
        },
        'ip-config': {
            required: false,
            type: 'object'
        },
        'system-info': {
            required: false,
            type: 'object'
        },
        'interfaces': {
            required: false,
            type: 'Array<string>'
        },
        'smartNicVersion': {
            required: false,
            type: 'string'
        },
        'smartNicSku': {
            required: false,
            type: 'string'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return ClusterSmartNICStatus.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return ClusterSmartNICStatus.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (ClusterSmartNICStatus.propInfo[prop] != null &&
                        ClusterSmartNICStatus.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['conditions'] = new Array<ClusterSmartNICCondition>();
        this['ip-config'] = new ClusterIPConfig();
        this['system-info'] = new ClusterSmartNICInfo();
        this['interfaces'] = new Array<string>();
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['admission-phase'] != null) {
            this['admission-phase'] = values['admission-phase'];
        } else if (fillDefaults && ClusterSmartNICStatus.hasDefaultValue('admission-phase')) {
            this['admission-phase'] = <ClusterSmartNICStatus_admission_phase>  ClusterSmartNICStatus.propInfo['admission-phase'].default;
        } else {
            this['admission-phase'] = null
        }
        if (values) {
            this.fillModelArray<ClusterSmartNICCondition>(this, 'conditions', values['conditions'], ClusterSmartNICCondition);
        } else {
            this['conditions'] = [];
        }
        if (values && values['serial-num'] != null) {
            this['serial-num'] = values['serial-num'];
        } else if (fillDefaults && ClusterSmartNICStatus.hasDefaultValue('serial-num')) {
            this['serial-num'] = ClusterSmartNICStatus.propInfo['serial-num'].default;
        } else {
            this['serial-num'] = null
        }
        if (values && values['primary-mac'] != null) {
            this['primary-mac'] = values['primary-mac'];
        } else if (fillDefaults && ClusterSmartNICStatus.hasDefaultValue('primary-mac')) {
            this['primary-mac'] = ClusterSmartNICStatus.propInfo['primary-mac'].default;
        } else {
            this['primary-mac'] = null
        }
        if (values) {
            this['ip-config'].setValues(values['ip-config'], fillDefaults);
        } else {
            this['ip-config'].setValues(null, fillDefaults);
        }
        if (values) {
            this['system-info'].setValues(values['system-info'], fillDefaults);
        } else {
            this['system-info'].setValues(null, fillDefaults);
        }
        if (values && values['interfaces'] != null) {
            this['interfaces'] = values['interfaces'];
        } else if (fillDefaults && ClusterSmartNICStatus.hasDefaultValue('interfaces')) {
            this['interfaces'] = [ ClusterSmartNICStatus.propInfo['interfaces'].default];
        } else {
            this['interfaces'] = [];
        }
        if (values && values['smartNicVersion'] != null) {
            this['smartNicVersion'] = values['smartNicVersion'];
        } else if (fillDefaults && ClusterSmartNICStatus.hasDefaultValue('smartNicVersion')) {
            this['smartNicVersion'] = ClusterSmartNICStatus.propInfo['smartNicVersion'].default;
        } else {
            this['smartNicVersion'] = null
        }
        if (values && values['smartNicSku'] != null) {
            this['smartNicSku'] = values['smartNicSku'];
        } else if (fillDefaults && ClusterSmartNICStatus.hasDefaultValue('smartNicSku')) {
            this['smartNicSku'] = ClusterSmartNICStatus.propInfo['smartNicSku'].default;
        } else {
            this['smartNicSku'] = null
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'admission-phase': CustomFormControl(new FormControl(this['admission-phase'], [required, enumValidator(ClusterSmartNICStatus_admission_phase), ]), ClusterSmartNICStatus.propInfo['admission-phase']),
                'conditions': new FormArray([]),
                'serial-num': CustomFormControl(new FormControl(this['serial-num']), ClusterSmartNICStatus.propInfo['serial-num']),
                'primary-mac': CustomFormControl(new FormControl(this['primary-mac']), ClusterSmartNICStatus.propInfo['primary-mac']),
                'ip-config': CustomFormGroup(this['ip-config'].$formGroup, ClusterSmartNICStatus.propInfo['ip-config'].required),
                'system-info': CustomFormGroup(this['system-info'].$formGroup, ClusterSmartNICStatus.propInfo['system-info'].required),
                'interfaces': CustomFormControl(new FormControl(this['interfaces']), ClusterSmartNICStatus.propInfo['interfaces']),
                'smartNicVersion': CustomFormControl(new FormControl(this['smartNicVersion']), ClusterSmartNICStatus.propInfo['smartNicVersion']),
                'smartNicSku': CustomFormControl(new FormControl(this['smartNicSku']), ClusterSmartNICStatus.propInfo['smartNicSku']),
            });
            // generate FormArray control elements
            this.fillFormArray<ClusterSmartNICCondition>('conditions', this['conditions'], ClusterSmartNICCondition);
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('conditions') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('conditions').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('ip-config') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('ip-config').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('system-info') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('system-info').get(field);
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
            this._formGroup.controls['admission-phase'].setValue(this['admission-phase']);
            this.fillModelArray<ClusterSmartNICCondition>(this, 'conditions', this['conditions'], ClusterSmartNICCondition);
            this._formGroup.controls['serial-num'].setValue(this['serial-num']);
            this._formGroup.controls['primary-mac'].setValue(this['primary-mac']);
            this['ip-config'].setFormGroupValuesToBeModelValues();
            this['system-info'].setFormGroupValuesToBeModelValues();
            this._formGroup.controls['interfaces'].setValue(this['interfaces']);
            this._formGroup.controls['smartNicVersion'].setValue(this['smartNicVersion']);
            this._formGroup.controls['smartNicSku'].setValue(this['smartNicSku']);
        }
    }
}

