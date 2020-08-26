
import { AfterViewInit, Component, Input, OnDestroy, OnInit, ViewEncapsulation, ChangeDetectionStrategy, ChangeDetectorRef } from '@angular/core';
import { ValidatorFn } from '@angular/forms';
import { Animations } from '@app/animations';
import { Utility } from '@app/common/Utility';
import { ControllerService } from '@app/services/controller.service';
import { ClusterService } from '@app/services/generated/cluster.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { ClusterDSCProfile, IClusterDSCProfile, ClusterDSCProfileSpec_deployment_target, ClusterDSCProfileSpec_feature_set } from '@sdk/v1/models/generated/cluster';
import { SelectItem } from 'primeng';
import { CreationForm } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';

@Component({
  selector: 'app-newdscprofile',
  templateUrl: './newdscprofile.component.html',
  styleUrls: ['./newdscprofile.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush
})

export class NewdscprofileComponent extends CreationForm<IClusterDSCProfile, ClusterDSCProfile> implements OnInit, AfterViewInit, OnDestroy {
  @Input() existingObjects: ClusterDSCProfile[] = [];

  selectedFwdMode: SelectItem;
  selectedPolicyMode: SelectItem;
  validationErrorMessage: string;

  depolymentTargetOptions: SelectItem[] = Utility.convertEnumToSelectItem(ClusterDSCProfileSpec_deployment_target);
  featureSets: SelectItem[] = Utility.convertEnumToSelectItem(ClusterDSCProfileSpec_feature_set);
  hostFeatureSets: SelectItem[] = [];
  virtualizedFeatureSets: SelectItem[] = [];

  constructor(protected _controllerService: ControllerService,
    private clusterService: ClusterService,
    protected uiconfigsService: UIConfigsService,
    protected cdr: ChangeDetectorRef
  ) {
    super(_controllerService, uiconfigsService, cdr, ClusterDSCProfile);
  }

  postNgInit(): void {
    // populate form options depending on ui-config
    if (this.uiconfigsService.isFeatureEnabled('enterprise')) {
      this.featureSets = Utility.convertEnumToSelectItem(ClusterDSCProfileSpec_feature_set, ['sdn']);
      this.hostFeatureSets = this.featureSets.slice(0, 2);
      this.virtualizedFeatureSets = this.featureSets.slice(2);
    } else {
      // for cloud, only show [host - sdn] combination
      this.depolymentTargetOptions = Utility.convertEnumToSelectItem(ClusterDSCProfileSpec_deployment_target, ['virtualized']);
      this.featureSets = Utility.convertEnumToSelectItem(ClusterDSCProfileSpec_feature_set, ['smartnic', 'flowaware', 'flowaware_firewall']);
      this.hostFeatureSets = this.featureSets;
    }

    if (!this.isInline) {
      // for a new dsc profile clear default value for deployment target and featureset
      this.newObject.$formGroup.get(['spec', 'deployment-target']).setValue(null);
      this.newObject.$formGroup.get(['spec', 'feature-set']).setValue(null);
    }
  }

  setCustomValidation() {
    this.newObject.$formGroup.get(['meta', 'name']).setValidators([
      this.newObject.$formGroup.get(['meta', 'name']).validator,
      this.isDSCProfileNameValid(this.existingObjects)]);
  }

  isDSCProfileNameValid(existingTechSupportRequest: ClusterDSCProfile[]): ValidatorFn {
    return Utility.isModelNameUniqueValidator(existingTechSupportRequest, 'DSC-profile-name');
  }

  setToolbar() {
    this.setCreationButtonsToolbar('SAVE FIREWALL PROFILE',
      UIRolePermissions.clusterdscprofile_update);
  }

  getFeatureSetOptions(): SelectItem[] {
    const deploymentTarget = this.newObject.$formGroup.get(['spec', 'deployment-target']).value;
    if (deploymentTarget === 'host') {
      return this.hostFeatureSets;
    }
    if (deploymentTarget === 'virtualized') {
      return this.virtualizedFeatureSets;
    }
    return [];
  }

  onDeploymentTargetChange() {
    this.newObject.$formGroup.get(['spec', 'feature-set']).setValue(null);
  }

  createObject(object: IClusterDSCProfile) {
    return this.clusterService.AddDSCProfile(object, '', true, false);
  }

  updateObject(newObject: IClusterDSCProfile, oldObject: IClusterDSCProfile) {
    return this.clusterService.UpdateDSCProfile(oldObject.meta.name, newObject, null, oldObject, true, false);
  }

  generateCreateSuccessMsg(object: IClusterDSCProfile) {
    return 'Created DSC Profile ' + object.meta.name;
  }

  generateUpdateSuccessMsg(object: IClusterDSCProfile) {
    return 'Updated DSC Profile ' + object.meta.name;
  }


  isFormValid(): boolean {
    if (Utility.isEmpty(this.newObject.$formGroup.get(['meta', 'name']).value)) {
      this.submitButtonTooltip = 'Error: Name field is empty.';
      return false;
    }
    if (this.newObject.$formGroup.get(['meta', 'name']).invalid) {
      this.submitButtonTooltip = 'Error: Name field is not valid.';
      return false;
    }
    if (this.isFieldEmpty(this.newObject.$formGroup.get(['spec', 'deployment-target']))) {
      this.submitButtonTooltip = 'Error: Please select a Deployment Target';
      return false;
    }
    if (this.isFieldEmpty(this.newObject.$formGroup.get(['spec', 'feature-set']))) {
      this.submitButtonTooltip = 'Error: Please select a Feature Set';
      return false;
    }

    this.submitButtonTooltip = 'Ready to submit';
    return true;
  }

   /**
    * Override super API
    * In case update fails, restore original value;
    * @param isCreate
    */
   onSaveFailure(isCreate: boolean) {
     if (! isCreate) { // it is an update operation failure
      this._controllerService.invokeInfoToaster('Info', 'Restore original value to DSC Profile.');
      this.newObject.setFormGroupValuesToBeModelValues();
     }
   }
}
