import { AfterViewInit, Component, Input, OnInit, ViewEncapsulation, ViewChild, OnDestroy, ChangeDetectionStrategy, ChangeDetectorRef } from '@angular/core';
import { FormArray, ValidatorFn, FormGroup, FormControl } from '@angular/forms';
import { Animations } from '@app/animations';
import { SelectItem } from 'primeng/primeng';

import { Utility } from '@app/common/Utility';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { IMonitoringTechSupportRequest, MonitoringTechSupportRequest } from '@sdk/v1/models/generated/monitoring';
import { RepeaterComponent, RepeaterData, ValueType } from 'web-app-framework';
import { CreationForm } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { UIConfigsService } from '@app/services/uiconfigs.service';

@Component({
  selector: 'app-newtechsupport',
  templateUrl: './newtechsupport.component.html',
  styleUrls: ['./newtechsupport.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class NewtechsupportComponent extends CreationForm<IMonitoringTechSupportRequest, MonitoringTechSupportRequest> implements OnInit {
  @ViewChild('cslabelRepeater') cslabelRepeater: RepeaterComponent;
  @ViewChild('nmlabelRepeater') nmlabelRepeater: RepeaterComponent;

  @Input() existingTechSupportRequest: MonitoringTechSupportRequest[] = [];
  @Input() naplesOptions: SelectItem[] = [];
  @Input() nodesOptions: SelectItem[] = [];

  constructor(protected _controllerService: ControllerService,
    protected _monitoringService: MonitoringService,
    protected uiconfigsService: UIConfigsService,
    protected cdr: ChangeDetectorRef,
  ) {
    super(_controllerService, uiconfigsService, cdr, MonitoringTechSupportRequest);
  }

  postNgInit() {
    this.newObject.$formGroup.get(['meta', 'name']).setValidators([
      this.newObject.$formGroup.get(['meta', 'name']).validator,
      this.isTechSupportRequestNameValid(this.existingTechSupportRequest)]);
    const formGroup = this.newObject.$formGroup.get(['spec', 'node-selector']) as FormGroup;
    formGroup.addControl('dscNames', new FormControl());
  }

  isTechSupportRequestNameValid(existingTechSupportRequest: MonitoringTechSupportRequest[]): ValidatorFn {
    return Utility.isModelNameUniqueValidator(existingTechSupportRequest, 'tech-support-request-name');
  }

  isRequired(): boolean {
    return this.isFieldEmpty(this.newObject.$formGroup.get(['spec', 'node-selector', 'names'])) &&
        this.isFieldEmpty(this.newObject.$formGroup.get(['spec', 'node-selector', 'dscNames']));
  }

  isFormValid(): boolean {
    if (Utility.isEmpty(this.newObject.$formGroup.get(['meta', 'name']).value)) {
      this.submitButtonTooltip = 'Error: Name field is empty.';
      return false;
    }
    if (this.newObject.$formGroup.get(['meta', 'name']).invalid)  {
      this.submitButtonTooltip = 'Error: Name field is invalid.';
      return false;
    }
    if (this.isFieldEmpty(this.newObject.$formGroup.get(['spec', 'node-selector', 'names'])) &&
      this.isFieldEmpty(this.newObject.$formGroup.get(['spec', 'node-selector', 'dscNames']))) {
      this.submitButtonTooltip = 'Error: No Nodes or DSCs are selected.';
      return false;
    }
    this.submitButtonTooltip = 'Ready to submit';
    return true;
  }

  getObjectValues(): IMonitoringTechSupportRequest {
    const techsupport: IMonitoringTechSupportRequest = this.newObject.getFormGroupValues();
    const nodeNames = techsupport.spec['node-selector'].names || [];
    const dscNames = (techsupport.spec['node-selector'] as any).dscNames || [];
    techsupport.spec['node-selector'].names = [...nodeNames, ...dscNames];
    delete (techsupport.spec['node-selector'] as any).dscNames;
    return techsupport;
  }

  setToolbar() {
    this.setCreationButtonsToolbar('CREATE TECH SUPPORT REQUEST', null);
  }

  createObject(techsupport: IMonitoringTechSupportRequest) {
    return this._monitoringService.AddTechSupportRequest(techsupport);
  }

  updateObject(newObject: IMonitoringTechSupportRequest, oldObject: IMonitoringTechSupportRequest) {
    // unimplemented
    return null;
  }

  generateCreateSuccessMsg(object: IMonitoringTechSupportRequest) {
    return 'Created tech support request ' + object.meta.name;
  }

  generateUpdateSuccessMsg(object: IMonitoringTechSupportRequest) {
    return 'Updated tech support request ' + object.meta.name;
  }
}
