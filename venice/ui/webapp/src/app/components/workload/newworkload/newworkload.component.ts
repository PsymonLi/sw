import { AfterViewInit, ChangeDetectionStrategy, Component, EventEmitter, Input, OnInit, Output, ViewEncapsulation, OnChanges, SimpleChanges, ChangeDetectorRef } from '@angular/core';
import { ControllerService } from '@app/services/controller.service';
import { Animations } from '@app/animations';
import { IWorkloadWorkload, WorkloadWorkload, WorkloadWorkloadIntfSpec } from '@sdk/v1/models/generated/workload';
import { WorkloadService } from '@app/services/generated/workload.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { FormArray, ValidatorFn, AbstractControl, ValidationErrors, FormGroup } from '@angular/forms';
import { SelectItem } from 'primeng/api';
import { IPUtility } from '@app/common/IPUtility';
import { Utility } from '@app/common/Utility';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { CreationPushForm } from '@app/components/shared/pentable/penpushtable.component';

@Component({
  selector: 'app-newworkload',
  templateUrl: './newworkload.component.html',
  styleUrls: ['./newworkload.component.scss'],
  encapsulation: ViewEncapsulation.None,
  animations: Animations,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class NewworkloadComponent extends CreationPushForm<IWorkloadWorkload, WorkloadWorkload> implements OnInit, OnChanges, AfterViewInit {

  // Let workload.component pass in hostOptions
  @Input() hostOptions: SelectItem[] = [];
  @Input() existingObjects: IWorkloadWorkload[] = [];

  IPS_LABEL: string = 'IP Addresses';
  IPS_ERRORMSG: string = 'Invalid IP addresses';
  IPS_TOOLTIP: string = 'Type in a single or multiple IP addresses separated by commas.';
  MACS_LABEL: string = 'MAC Addresses';
  MACS_ERRORMSG: string = 'Invalid MAC addresses. It should be aaaa.bbbb.cccc format.';

  constructor(protected _controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected workloadService: WorkloadService,
    protected cdr: ChangeDetectorRef
  ) {
    super(_controllerService, uiconfigsService, cdr, WorkloadWorkload);
  }


  // Empty Hook
  postNgInit() {
    // Add one interface if it doesn't already have one
    const interfaces = this.newObject.$formGroup.get(['spec', 'interfaces']) as FormArray;

    if (interfaces.length === 0) {
      this.addInterface();
    } else {
      interfaces.controls.forEach((form: FormGroup) => {
        if (!form.get(['external-vlan']).value) {
          form.get(['external-vlan']).setValue(0);
        }
        form.get(['ip-addresses']).setValidators([
          this.isValidIpAddresses()
        ]);
        form.get(['mac-address']).setValidators([
          this.isValidMacAddress()
        ]);
      });
    }

    this.newObject.$formGroup.get(['meta', 'name']).setValidators([
      this.newObject.$formGroup.get(['meta', 'name']).validator,
      this.isNewPolicyNameValid(this.existingObjects)]);
  }

  isNewPolicyNameValid(existingObjects: IWorkloadWorkload[]): ValidatorFn {
    // checks if name field is valid
    return Utility.isModelNameUniqueValidator(existingObjects, 'newWorkload-name');
  }

  addInterface() {
    const interfaces = this.newObject.$formGroup.get(['spec', 'interfaces']) as FormArray;
    const newInterface = new WorkloadWorkloadIntfSpec().$formGroup;

    newInterface.get('mac-address').setValidators([
      this.isValidMacAddress()
    ]);
    newInterface.get('ip-addresses').setValidators([
      this.isValidIpAddresses()
    ]);
    interfaces.push(newInterface);
  }

  removeInterface(index: number) {
    const interfaces = this.newObject.$formGroup.get(['spec', 'interfaces']) as FormArray;
    if (interfaces.length > 1) {
      interfaces.removeAt(index);
    }
  }

  isValidIpAddresses(): ValidatorFn {
    return (control: AbstractControl): ValidationErrors | null => {
      const value: string[] = control.value;
      if (Utility.isValueOrArrayEmpty(value)) {
        return null;
      }
      for (let i = 0; i < value.length; i++) {
        const ip: string = value[i].trim();
        if (!IPUtility.isValidIPWithOptionalMask(ip)) {
          return {
            objectName: {
              required: true,
              message: this.IPS_ERRORMSG
            }
          };
        }
      }
      return null;
    };
  }

  isValidMacAddress(): ValidatorFn {
    return (control: AbstractControl): ValidationErrors | null => {
      const macAddress = control.value;
      if (!macAddress) {
        return null;
      }
      if (!Utility.MACADDRESS_REGEX.test(macAddress)) {
        return {
          objectName: {
            required: true,
            message: this.MACS_ERRORMSG
          }
        };
      }
      return null;
    };
  }

  // Empty Hook
  isFormValid() {
    if (Utility.isEmpty(this.newObject.$formGroup.get(['meta', 'name']).value)) {
      this.submitButtonTooltip = 'Error: Name field is empty.';
      return false;
    }
    if (this.newObject.$formGroup.get(['meta', 'name']).invalid)  {
      this.submitButtonTooltip = 'Error: Name field is invalid.';
      return false;
    }
    if (Utility.isEmpty(this.newObject.$formGroup.get(['spec', 'host-name']).value)) {
      this.submitButtonTooltip = 'Error: Host is required.';
      return false;
    }
    const arr: FormArray = this.newObject.$formGroup.get(['spec', 'interfaces']) as FormArray;
    for (let i = 0; i < arr.length; i++) {
      const fieldValue: FormGroup = arr['controls'][i] as FormGroup;
      if ((fieldValue.controls['mac-address']).value !== null) {
        if (!(fieldValue.controls['mac-address']).valid) {
          this.submitButtonTooltip =
            'Error: Interface ' + (i + 1) + ' source MAC address is invalid.';
          return false;
        }
      } else {
        this.submitButtonTooltip =
          'Error: Interface ' + (i + 1) + ' source MAC address is empty.';
        return false;
      }
      if ((fieldValue.controls['ip-addresses']).value.length) {
        if (!(fieldValue.controls['ip-addresses']).valid) {
          this.submitButtonTooltip =
            'Error: Interface ' + (i + 1) + ' destination IP addresses are invalid.';
          return false;
        }
      }
      if ((fieldValue.controls['external-vlan']).value !== null) {
        if (!(fieldValue.controls['external-vlan']).valid) {
          this.submitButtonTooltip =
            'Error: Interface ' + (i + 1) + ' external-vlan is invalid.';
          return false;
        }
      } else {
        this.submitButtonTooltip =
          'Error: Interface ' + (i + 1) + ' external-vlan is empty.';
        return false;
      }
      if ((fieldValue.controls['micro-seg-vlan']).value !== null) {
        if (!(fieldValue.controls['micro-seg-vlan']).valid) {
          this.submitButtonTooltip =
            'Error: Interface ' + (i + 1) + ' micro-seg-vlan is invalid.';
          return false;
        }
      } else {
        this.submitButtonTooltip =
          'Error: Interface ' + (i + 1) + '  micro-seg-vlan is empty.';
        return false;
      }
    }
    if (Utility.isValueOrArrayEmpty(arr)) {
      this.submitButtonTooltip = 'At least one interface has to be added.';
      return false;
    }
    if (!this.newObject.$formGroup.valid) {
      this.submitButtonTooltip = 'Error: form is not valid';
      return false;
    }
    return true;
  }

  oneInterfaceIsValid(): boolean {
    const array: FormArray = this.newObject.$formGroup.get(['spec', 'interfaces']) as FormArray;
    for (let i = 0; i < array.controls.length; i++) {
      if (this.isInterfaceValid(array.controls[i] as FormGroup)) {
        return true;
      }
    }
    return false;
  }

  isInterfaceValid(form: FormGroup) {
    const fields = [
      form.get(['mac-address']),
      form.get(['external-vlan']),
      form.get(['micro-seg-vlan'])
    ];
    for (let i = 0; i < fields.length; i++) {
      if (Utility.isEmpty(fields[i].value) || !fields[i].valid) {
        return false;
      }
    }
    const field = form.get(['ip-addresses']);
    return field.valid;
  }

  setToolbar() {
    this.setCreationButtonsToolbar('CREATE WORKLOAD',
        UIRolePermissions.workloadworkload_create);
  }

  createObject(object: IWorkloadWorkload) {
    return this.workloadService.AddWorkload(object);
  }

  updateObject(newObject: IWorkloadWorkload, oldObject: IWorkloadWorkload) {
    return this.workloadService.UpdateWorkload(oldObject.meta.name, newObject, null, oldObject);
  }

  generateCreateSuccessMsg(object: IWorkloadWorkload) {
    return 'Created workload ' + object.meta.name;
  }

  generateUpdateSuccessMsg(object: IWorkloadWorkload) {
    return 'Updated workload ' + object.meta.name;
  }
}
