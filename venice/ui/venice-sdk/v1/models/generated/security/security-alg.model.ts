/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, enumValidator } from './validators';
import { BaseModel, EnumDef } from './base-model';

import { SecurityDNS, ISecurityDNS } from './security-dns.model';
import { SecuritySIP, ISecuritySIP } from './security-sip.model';
import { SecuritySunRPC, ISecuritySunRPC } from './security-sun-rpc.model';
import { SecurityFTP, ISecurityFTP } from './security-ftp.model';
import { SecurityMSRPC, ISecurityMSRPC } from './security-msrpc.model';
import { SecurityTFTP, ISecurityTFTP } from './security-tftp.model';
import { SecurityRSTP, ISecurityRSTP } from './security-rstp.model';

export interface ISecurityALG {
    'dns'?: ISecurityDNS;
    'sip'?: ISecuritySIP;
    'sunrpc'?: ISecuritySunRPC;
    'ftp'?: ISecurityFTP;
    'msrpc'?: ISecurityMSRPC;
    'tftp'?: ISecurityTFTP;
    'rstp'?: ISecurityRSTP;
}


export class SecurityALG extends BaseModel implements ISecurityALG {
    'dns': SecurityDNS;
    'sip': SecuritySIP;
    'sunrpc': SecuritySunRPC;
    'ftp': SecurityFTP;
    'msrpc': SecurityMSRPC;
    'tftp': SecurityTFTP;
    'rstp': SecurityRSTP;
    public static enumProperties: { [key: string] : EnumDef } = {
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any) {
        super();
        this['dns'] = new SecurityDNS();
        this['sip'] = new SecuritySIP();
        this['sunrpc'] = new SecuritySunRPC();
        this['ftp'] = new SecurityFTP();
        this['msrpc'] = new SecurityMSRPC();
        this['tftp'] = new SecurityTFTP();
        this['rstp'] = new SecurityRSTP();
        if (values) {
            this.setValues(values);
        }
    }

    /**
     * set the values.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any): void {
        if (values) {
            this['dns'].setValues(values['dns']);
            this['sip'].setValues(values['sip']);
            this['sunrpc'].setValues(values['sunrpc']);
            this['ftp'].setValues(values['ftp']);
            this['msrpc'].setValues(values['msrpc']);
            this['tftp'].setValues(values['tftp']);
            this['rstp'].setValues(values['rstp']);
        }
    }

    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'dns': this['dns'].$formGroup,
                'sip': this['sip'].$formGroup,
                'sunrpc': this['sunrpc'].$formGroup,
                'ftp': this['ftp'].$formGroup,
                'msrpc': this['msrpc'].$formGroup,
                'tftp': this['tftp'].$formGroup,
                'rstp': this['rstp'].$formGroup,
            });
        }
        return this._formGroup;
    }

    setFormGroupValues() {
        if (this._formGroup) {
            this['dns'].setFormGroupValues();
            this['sip'].setFormGroupValues();
            this['sunrpc'].setFormGroupValues();
            this['ftp'].setFormGroupValues();
            this['msrpc'].setFormGroupValues();
            this['tftp'].setFormGroupValues();
            this['rstp'].setFormGroupValues();
        }
    }
}

