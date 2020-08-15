import { AfterViewInit, ChangeDetectionStrategy, Component, OnDestroy, OnInit, ViewEncapsulation, ChangeDetectorRef } from '@angular/core';
import { ControllerService } from '@app/services/controller.service';
import { SecurityService } from '@app/services/generated/security.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { ISecurityFirewallProfile, SecurityFirewallProfile } from '@sdk/v1/models/generated/security';
import { minValueValidator } from '@sdk/v1/utils/validators';
import { CreationForm } from '@app/components/shared/tableviewedit/tableviewedit.component';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';

/**
 * As FirewallProfile object is a singleton object within Venice.
 * We make the create-form to perform object update.
 *
 * These API and properties tweak system to perform object update.
 *
 * isSingleton: boolean = true;
 * postNgInit()
 */
@Component({
  selector: 'app-newfirewallprofile',
  templateUrl: './newfirewallprofile.component.html',
  styleUrls: ['./newfirewallprofile.component.scss'],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class NewfirewallprofileComponent extends CreationForm<SecurityFirewallProfile, SecurityFirewallProfile> implements OnInit, AfterViewInit, OnDestroy {

  isSingleton: boolean = true;
  saveButtonTooltip: string = 'There is no change';

  constructor(protected _controllerService: ControllerService,
    protected uiconfigsService: UIConfigsService,
    protected securityService: SecurityService,
    protected cdr: ChangeDetectorRef
  ) {
    super(_controllerService, uiconfigsService, cdr, SecurityFirewallProfile);
  }

  postNgInit() {
    // use selectedFirewallProfile to populate this.newObject
    this.newObject = new SecurityFirewallProfile(this.objectData);
    this.newObject.$formGroup.get(['meta', 'name']).disable();
    this.addFieldValidator(this.newObject.$formGroup.get(['spec', 'session-idle-timeout']), this.isTimeoutValid('session-idle-timeout'));
    this.addFieldValidator(this.newObject.$formGroup.get(['spec', 'tcp-connection-setup-timeout']), this.isTimeoutValid('tcp-connection-setup-timeout'));
    this.addFieldValidator(this.newObject.$formGroup.get(['spec', 'tcp-close-timeout']), this.isTimeoutValid('tcp-close-timeout'));
    this.addFieldValidator(this.newObject.$formGroup.get(['spec', 'tcp-half-closed-timeout']), this.isTimeoutValid('tcp-half-closed-timeout'));
    this.addFieldValidator(this.newObject.$formGroup.get(['spec', 'tcp-drop-timeout']), this.isTimeoutValid('tcp-drop-timeout'));
    this.addFieldValidator(this.newObject.$formGroup.get(['spec', 'udp-drop-timeout']), this.isTimeoutValid('udp-drop-timeout'));
    this.addFieldValidator(this.newObject.$formGroup.get(['spec', 'icmp-drop-timeout']), this.isTimeoutValid('icmp-drop-timeout'));
    this.addFieldValidator(this.newObject.$formGroup.get(['spec', 'drop-timeout']), this.isTimeoutValid('drop-timeout'));
    this.addFieldValidator(this.newObject.$formGroup.get(['spec', 'tcp-timeout']), this.isTimeoutValid('tcp-timeout'));
    this.addFieldValidator(this.newObject.$formGroup.get(['spec', 'udp-timeout']), this.isTimeoutValid('udp-timeout'));
    this.addFieldValidator(this.newObject.$formGroup.get(['spec', 'icmp-timeout']), this.isTimeoutValid('icmp-timeout'));
  }

  setToolbar(): void {
    this.setCreationButtonsToolbar('SAVE FIREWALL PROFILE',
      UIRolePermissions.securityfirewallprofile_update);
  }

  isFormValid(): boolean {
    if (this.newObject.$formGroup.get(['spec', 'session-idle-timeout']).invalid) {
      this.submitButtonTooltip = 'Error: Session Idle Timeout is invalid.';
      return false;
    }
    if (this.newObject.$formGroup.get(['spec', 'tcp-connection-setup-timeout']).invalid) {
      this.submitButtonTooltip = 'Error: Tcp Connection Timeout is invalid.';
      return false;
    }
    if (this.newObject.$formGroup.get(['spec', 'tcp-close-timeout']).invalid) {
      this.submitButtonTooltip = 'Error: TCP Close Timeout is invalid.';
      return false;
    }
    if (this.newObject.$formGroup.get(['spec', 'tcp-half-closed-timeout']).invalid) {
      this.submitButtonTooltip = 'Error: TCP Half Closed Timeout is invalid.';
      return false;
    }
    if (this.newObject.$formGroup.get(['spec', 'tcp-drop-timeout']).invalid) {
      this.submitButtonTooltip = 'Error: TCP Drop Timeout is invalid.';
      return false;
    }
    if (this.newObject.$formGroup.get(['spec', 'udp-drop-timeout']).invalid) {
      this.submitButtonTooltip = 'Error: UDP Drop Timeout is invalid.';
      return false;
    }
    if (this.newObject.$formGroup.get(['spec', 'icmp-drop-timeout']).invalid) {
      this.submitButtonTooltip = 'Error: ICMP Drop Timeout is invalid.';
      return false;
    }
    if (this.newObject.$formGroup.get(['spec', 'drop-timeout']).invalid) {
      this.submitButtonTooltip = 'Error: Drop Timeout is invalid.';
      return false;
    }
    if (this.newObject.$formGroup.get(['spec', 'tcp-timeout']).invalid) {
      this.submitButtonTooltip = 'Error: TCP Timeout is invalid.';
      return false;
    }
    if (this.newObject.$formGroup.get(['spec', 'udp-timeout']).invalid) {
      this.submitButtonTooltip = 'Error: UDP Timeout is invalid.';
      return false;
    }
    if (this.newObject.$formGroup.get(['spec', 'icmp-timeout']).invalid) {
      this.submitButtonTooltip = 'Error: ICMP Timeout is invalid.';
      return false;
    }if (this.newObject.$formGroup.get(['spec', 'tcp-half-open-session-limit']).invalid) {
      this.submitButtonTooltip = 'Error: TCP Half Open Session Limit is invalid.';
      return false;
    }if (this.newObject.$formGroup.get(['spec', 'udp-active-session-limit']).invalid) {
      this.submitButtonTooltip = 'Error: UDP Active Session Limit is invalid.';
      return false;
    }
    if (this.newObject.$formGroup.get(['spec', 'icmp-active-session-limit']).invalid) {
      this.submitButtonTooltip = 'Error: ICMP Active Session Limit is invalid.';
      return false;
    }
    this.submitButtonTooltip = 'Ready to submit';
    return true;
  }

  createObject(object: SecurityFirewallProfile) {
    return null;
  }

  // tcp,icmp,udp, any + 1< x < 255
  updateObject(newObject: ISecurityFirewallProfile, oldObject: ISecurityFirewallProfile) {
    return this.securityService.UpdateFirewallProfile(oldObject.meta.name, newObject, null, oldObject, true, false);
  }

  generateUpdateSuccessMsg(object: SecurityFirewallProfile): string {
    return 'Updated firewall profile ' + object.meta.name;
  }

  generateCreateSuccessMsg(object: SecurityFirewallProfile): string {
    throw new Error('Firewall Profile is a singleton object. Create method is not supported');
  }

}
