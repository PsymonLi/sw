import { Component, OnDestroy, OnInit, ViewEncapsulation } from '@angular/core';
import { Animations } from '@app/animations';
import { Observable, forkJoin, throwError } from 'rxjs';

import { map, switchMap, tap, catchError, buffer } from 'rxjs/operators';

import { SelectItem } from 'primeng/primeng';
import { ErrorStateMatcher } from '@angular/material';

import { BaseComponent } from '@app/components/base/base.component';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { ControllerService } from '@app/services/controller.service';
import { Utility } from '@app/common/Utility';
import { UserDataReadyMap } from './';

import { AuthService } from '@app/services/generated/auth.service';
import { StagingService } from '@app/services/generated/staging.service';
import {
  ApiStatus, IAuthRoleBindingList, AuthRoleBindingList,
  AuthRoleBinding, IAuthRoleList, AuthRole, IAuthUserList,
  AuthUserList, AuthUser, AuthRoleList, AuthPasswordChangeRequest
} from '@sdk/v1/models/generated/auth';
import { StagingBuffer, StagingCommitAction } from '@sdk/v1/models/generated/staging';

export enum ACTIONTYPE {
  CREATE = 'Create',
  UPDATE = 'Update',
  DELETE = 'Delete'
}

export interface AuthPolicyObject {
  meta: {
    name: string;
  };
}

/**
 * User-component allow end-user manage users, roles and role-binding.
 * Venice user can pick USER/ROLE/ROLE-BINDING from toolbar menu
 *
 * I
 * User
 * Adding new-user is in newuser.component.ts.   User is corresponding to AuthUser object. AuthUser.status should contain role information which is readonly.
 *
 * AuthUser, AuthRole, AuthRoleBinding (objects) are inter-related.  If assigning a role to user, AuthRoleBinding must update user-name to AuthRoleBinding.spec.users [..]
 * For example, adding "test" to admin-role,  AdminRole (AuthRoleBinding)..spec.users[ 'test' .. ]
 *
 * II
 * Role
 * users.component.html has Role list UI.  Included NewroleComponent handles role creation or update.
 *
 * III
 * Role-binding
 *
 *
 */

@Component({
  selector: 'app-users',
  templateUrl: './users.component.html',
  styleUrls: ['./users.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None,
})
export class UsersComponent extends BaseComponent implements OnInit, OnDestroy {

  public static UI_PANEL_USER = 'user';
  public static UI_PANEL_ROLE = 'role';
  public static UI_PANEL_ROLEBINDING = 'rolebinding';

  public static USER_ACTION_UPDATE = 'update';
  public static USER_ACTION_CHANGEPWD = 'changepassword';
  authPasswordChangeRequest: AuthPasswordChangeRequest = null; // This is for changing user password

  authusers: AuthUser[] = [];
  authRoles: AuthRole[] = [];
  authRoleBindings: AuthRoleBinding[] = [];

  selectedAuthUser: AuthUser = null;
  selectedAuthRole: AuthRole = null;
  selectedAuthRolebinding: AuthRoleBinding = null;

  rolebindingOptions: SelectItem[] = [];
  selectedRolebindingsForUsers: AuthRoleBinding[] = [];
  rolebindingUpdateMap = {};

  whichPanel = UsersComponent.UI_PANEL_USER;
  selectedDropdown: any = UsersComponent.UI_PANEL_USER;

  dataReadyMap: UserDataReadyMap = {
    users: false,
    roles: false,
    rolebindings: false
  };

  isToShowAddUserPanel: boolean = false;
  isToShowAddRolePanel: boolean = false;
  isToShowAddRolebindingPanel: boolean = false;



  usericon: any = {
    margin: {
      top: '9px',
      left: '8px'
    },
    url: '/assets/images/icons/auth/ico-authorizations.svg'
  };


  userEditAction: string = UsersComponent.USER_ACTION_UPDATE;

  constructor(protected _controllerService: ControllerService,
    protected _authService: AuthService,
    protected stagingService: StagingService
  ) {
    super(_controllerService, null);  // we don't want to use messageService.
  }

  ngOnInit() {
    this.getData();
    this.setToolbarItems();
  }

  protected setToolbarItems() {
    this._controllerService.setToolbarData({
      buttons: [
        {
          cssClass: 'global-button-primary users-toolbar-button users-toolbar-button-refresh',
          text: 'Refresh',
          callback: () => { this.getData(); },
        }
      ],
      breadcrumb: [{ label: 'RBAC Management', url: '' }],
      dropdowns: [
        {
          callback: (event, sbutton) => {
            this.onLayoutDropDownChange(sbutton);
          },
          options: [
            { label: UsersComponent.UI_PANEL_USER, value: UsersComponent.UI_PANEL_USER },
            { label: UsersComponent.UI_PANEL_ROLE, value: UsersComponent.UI_PANEL_ROLE },
            { label: UsersComponent.UI_PANEL_ROLEBINDING, value: UsersComponent.UI_PANEL_ROLEBINDING }
          ],
          model: this.selectedDropdown,
          placeholder: 'Select layout'
        }
      ]
    });
  }

  onLayoutDropDownChange(sbutton: any) {
    this.whichPanel = sbutton.model;
  }
  /**
   * Component is about to exit
   */
  ngOnDestroy() {
    // publish event that AppComponent is about to exist
    this._controllerService.publish(Eventtypes.COMPONENT_DESTROY, { 'component': 'UsersComponent', 'state': Eventtypes.COMPONENT_DESTROY });
  }

  getClassName(): string {
    return this.constructor.name;
  }

  setDataReadyMap(key: string, value: boolean = true) {
    this.dataReadyMap[key] = value;
    const keys = Object.keys(this.dataReadyMap);
    let allset = true;
    keys.forEach((item) => {
      if (this.dataReadyMap[item] === false) {
        allset = false;
      }
    });
    if (allset) {
      this.combineData();
    }
  }

  getData() {
    this.getUsers();
    this.getAuthRoles();
    this.getRolebindings();
  }

  combineData() {
    this.populateRolebindingOptions();
  }

  protected populateRolebindingOptions() {
    if (this.authRoleBindings) {
      this.rolebindingOptions.length = 0;
      this.authRoleBindings.forEach((rolebinding) => {
        const obj: SelectItem = {
          label: rolebinding.meta.name, // rolebinding.meta.name, rolebinding.spec.role
          value: rolebinding
        };
        this.rolebindingOptions.push(obj);
      });
    }
  }

  getUsers() {
    this._authService.ListUser().subscribe(
      (data) => {
        const authUserList: AuthUserList = new AuthUserList(<IAuthUserList>data.body);
        if (authUserList.items.length > 0) {
          this.authusers.length = 0;
          this.authusers = authUserList.items;
          this.setDataReadyMap('users', true);
        }
      },
      this._controllerService.restErrorHandler('Failed to get Users')
    );
  }

  getAuthRoles() {
    this._authService.ListRole().subscribe(
      (data) => {
        const authRoleList: AuthRoleList = new AuthRoleList(<IAuthRoleList>data.body);
        if (authRoleList.items.length > 0) {
          this.authRoles.length = 0;
          this.authRoles = authRoleList.items;
          this.setDataReadyMap('roles', true);
        }
      },
      this._controllerService.restErrorHandler('Failed to get Roles')
    );
  }

  getRolebindings() {
    this._authService.ListRoleBinding().subscribe(
      (data) => {
        const authRoleBindings: AuthRoleBindingList = new AuthRoleBindingList(<IAuthRoleBindingList>data.body);
        if (authRoleBindings.items.length > 0) {
          this.authRoleBindings.length = 0;
          this.authRoleBindings = authRoleBindings.items;
          this.setDataReadyMap('rolebindings', true);
        }
      },
      this._controllerService.restErrorHandler('Failed to get Role Bindings')
    );
  }

  /**
   * The API serves HTML template
   * Find unique role list
   */
  getRoles(): string[] {
    const roles: string[] = [];
    const unique = {};

    for (let i = 0; i < this.authusers.length; i++) {
      const user = this.authusers[i];
      const userRoleValues = this.getUserRoleName(user);
      const v = (userRoleValues && userRoleValues.length > 0) ? userRoleValues[0] : 'default';
      if (typeof (unique[v]) === 'undefined') {
        roles.push(v);
      }
      const count = (unique[v]) ? unique[v] : 0;
      unique[v] = count + 1;
    }
    return roles;
  }

  getUserRoleName(user: AuthUser): any {
    const _ = Utility.getLodash();
    return _.result(user, 'status.roles');
  }

  /**
   * This API serves html template
   * Find all users of a role.
   * @param role
   */
  getRoleUsers(role: string): AuthUser[] {
    const _ = Utility.getLodash();
    const users: AuthUser[] = [];
    for (let i = 0; i < this.authusers.length; i++) {
      const user = this.authusers[i];
      const value: any = _.result(user, 'status.roles');
      const v = value.length > 0 ? value[0] : 'default';
      if (role === v) {
        users.push(user);
      }
    }
    return users;
  }

  /**
   * This API serves html template
   * @param $event
   */
  onAddUser($event) {
    this.isToShowAddUserPanel = true;
  }
  /**
   * This API serves html template
   * @param $event
   */
  onAddRole($event) {
    this.isToShowAddRolePanel = true;
    this.selectedAuthRole = null;
  }

  /**
   * This API serves html template
   * @param $event
   */
  onAddRoleBinding($event) {
    this.isToShowAddRolebindingPanel = true;
  }

  /**
   *
   * @param $event
   * @param user : AuthUser
   */
  onDeleteUser($event, user: AuthUser) {
    this.onDeleteUser_with_staging($event, user);
  }

  /**
   * Using staging buffer to delete user
   * @param $event
   * @param deletedUser
   */
  onDeleteUser_with_staging($event, deletedUser: AuthUser) {
    this._controllerService.invokeConfirm({
      header: Utility.generateDeleteConfirmMsg('User', deletedUser.meta.name),
      message: 'This action cannot be reversed',
      acceptLabel: 'Delete',
      accept: () => {
        this.deleteUser_with_staging(deletedUser);
      }
    });
  }

  /**
   * Delete user
   * @param deletedUser
   *
   * (A) create buffer -> (B) update all those role-bindings referenced with will-be-deleted user -> (C) delete user -> (D) commit buffer
   *
   */
  deleteUser_with_staging(deletedUser: AuthUser) {
    const msgFailedToDeleteUser = 'Failed to delete user';
    let createdBuffer: StagingBuffer = null;  // responseBuffer.body as StagingBuffer;
    let buffername = null; // createdBuffer.meta.name;
    const observables: Observable<any>[] = [];
    this.createStagingBuffer().pipe(   // (A) create buffer
      switchMap(responseBuffer => {
        createdBuffer = responseBuffer.body as StagingBuffer;
        buffername = createdBuffer.meta.name;
        const username = deletedUser.meta.name;
        this.authRoleBindings.forEach((rolebinding) => {
          this.rolebindingUpdateMap[rolebinding.meta.name] = false;
          const observable = this.removeUserFromRolebing(rolebinding, username, buffername);
          if (observable != null) {
            observables.push(observable);
          }
        });
        if (observables.length === 0) {
          // If there is no observables, delete user directly
          return this._authService.DeleteUser(deletedUser.meta.name, buffername).pipe(  // (C) delete user
            switchMap(() => {
              return this.commitStagingBuffer(buffername);  // (D) commit buffer
            })
          );
        } else {
          return forkJoin(observables).pipe(  // (B) update all referenced role-bindings
            switchMap(results => {
              const isAllOK = Utility.isForkjoinResultAllOK(results);
              if (isAllOK) {
                return this._authService.DeleteUser(deletedUser.meta.name, buffername).pipe(  // (C) delete user
                  switchMap(() => {
                    return this.commitStagingBuffer(buffername);  // (D) commit buffer
                  })
                );
              } else {
                const error = Utility.joinErrors(results);
                return throwError(error);
              }
            })
          );
        }
      })
    ).subscribe(
      (responseCommitBuffer) => {
        this._controllerService.invokeSuccessToaster(Utility.DELETE_SUCCESS_SUMMARY, ACTIONTYPE.DELETE + ' User ' + deletedUser.meta.name);
        this.getData(); // refresh data after deleting object
      },
      (error) => {
        this._controllerService.invokeRESTErrorToaster(Utility.DELETE_FAILED_SUMMARY, error);
        this.deleteStagingBuffer(buffername, msgFailedToDeleteUser, false);
      }
    );
  }


  deleteStagingBuffer(buffername: string, reason: string, isToshowToaster: boolean = true) {
    if (buffername == null) {
      return;
    }

    this.stagingService.GetBuffer(buffername).subscribe( (rest) => {
      console.log(rest);
    });
    this.stagingService.DeleteBuffer(buffername).subscribe(
      response => {
        if (isToshowToaster) {
          this._controllerService.invokeSuccessToaster('Successfully deleted buffer', 'Deleted Buffer ' + buffername + '\n' + reason);
        }
      },
      this._controllerService.restErrorHandler('Delete Staging Buffer Failed')
    );
  }

  commitStagingBuffer(buffername: string): Observable<any> {
    const commitBufferBody: StagingCommitAction = Utility.buildCommitBufferCommit(buffername);
    return this.stagingService.Commit(buffername, commitBufferBody);
  }

  createStagingBuffer(): Observable<any> {
    const stagingBuffer: StagingBuffer = Utility.buildCommitBuffer();
    return this.stagingService.AddBuffer(stagingBuffer);
  }

  /**
   * This API serves html template
   * @param $event
   * @param role
   */
  onDeleteRole($event, role: AuthRole) {
    this._controllerService.invokeConfirm({
      header: Utility.generateDeleteConfirmMsg('Role', role.meta.name),
      message: 'This action cannot be reversed',
      acceptLabel: 'Delete',
      accept: () => {
        this.deleteRole_with_staging(role);
      }
    });
  }

  /**
   * Delete a row.  Say deleting roleA, we will also delete roleA-rolebinding
   *
   * delete a role (say roleA)
	 *	 create buffer
   *       delete roleA
   *        delete the roleA-rolebinding
	 *			if (success) {
   *            commit buffer
   *       else
   *            delete buffer
   *
   *  @param deletedRole
   *
   */
  deleteRole_with_staging(deletedRole: AuthRole) {
    let createdBuffer: StagingBuffer = null;  // responseBuffer.body as StagingBuffer;
    let buffername = null; // createdBuffer.meta.name;
    const observables: Observable<any>[] = [];
    this.createStagingBuffer() // invoke REST call (A)
      .pipe(
        switchMap(responseBuffer => {  // get server response from (A)
          createdBuffer = responseBuffer.body as StagingBuffer;
          buffername = createdBuffer.meta.name;
          this.authRoleBindings.forEach((rolebinding: AuthRoleBinding) => {
            if (rolebinding.spec.role === deletedRole.meta.name) {
              const observable = this._authService.DeleteRoleBinding(rolebinding.meta.name, buffername);
              observables.push(observable);
            }
          });
          // Delete role after delete role-bindings.
          observables.push(this._authService.DeleteRole(deletedRole.meta.name, buffername));
          if (observables.length > 0) {
            return this.invokeForkJoin(observables, buffername); // (B-C)
          } else {
            // it is possible that user delete role-binding manually first, observables[] is empty
            return this.commitStagingBuffer(buffername);
          }
        })
      )
      .subscribe(
        // We are getting response for (C)
        (responseCommitBuffer) => {
          this._controllerService.invokeSuccessToaster(Utility.DELETE_SUCCESS_SUMMARY, ACTIONTYPE.DELETE + ' Role ' + deletedRole.meta.name);
          this.getData(); // refresh data after deleting object
        },
        (error) => {
          // any error in (A) (B) or (C), error will land here
          this._controllerService.invokeRESTErrorToaster(Utility.DELETE_FAILED_SUMMARY, error);
          this.deleteStagingBuffer(buffername, 'Failed to delete', false);
        }
      );
  }

  invokeForkJoin(observables: Observable<any>[], buffername: any) {
    return forkJoin(observables) // (B)
      .pipe(switchMap((results) => {
        const isAllOK = Utility.isForkjoinResultAllOK(results);
        if (isAllOK) {
          return this.commitStagingBuffer(buffername); // (C)
        } else {
          const error = Utility.joinErrors(results);
          return throwError(error);
        }
      }));
  }

  /**
   * Serves HTML template. User clicks Role-binding delete button
   * @param $event
   * @param rolebinding
   */
  onDeleteRoleBinding($event, rolebinding: AuthRoleBinding) {
    this._controllerService.invokeConfirm({
      header: Utility.generateDeleteConfirmMsg('Rolebinding', rolebinding.meta.name),
      message: 'This action cannot be reversed',
      acceptLabel: 'Delete',
      accept: () => {
        this.deleteRoleBinding(rolebinding);
      }
    });
  }

  deleteRoleBinding(rolebinding: AuthRoleBinding) {
    this._authService.DeleteRoleBinding(rolebinding.meta.name).subscribe(
      (data) => {
        // refresh roles list
        this._controllerService.invokeSuccessToaster(Utility.DELETE_SUCCESS_SUMMARY, 'Deleted rolebinding ' + rolebinding.meta.name);
        this.getData();
      },
      this._controllerService.restErrorHandler(Utility.DELETE_FAILED_SUMMARY)
    );
  }

  /**
   * This API serves html template
   * @param $event
   * @param user
   */
  onEditUser(event, user: AuthUser) {
    this.selectedAuthUser = user;
    this.userEditAction = UsersComponent.USER_ACTION_UPDATE;
    this.selectedRolebindingsForUsers.length = 0;
    this.rolebindingOptions.forEach((selectItem) => {
      for (let i = 0; user.status && user.status.roles && i < user.status.roles.length; i++) {
        const rbName = user.status.roles[i];
        if (rbName === selectItem.value.spec.role) {
          this.selectedRolebindingsForUsers.push(selectItem.value);
        }
      }
    });
    this.selectedAuthUser.$formGroup.get(['meta', 'name']).disable();
  }

  /**
   * This API serves html template
   * @param $event
   * @param role
   */
  onEditRole(event, role: AuthRole) {
    this.selectedAuthRole = role;
    this.isToShowAddRolePanel = true;
  }

  /**
   * This API serves html template
   * @param $event
   * @param rolebinding
   */
  onEditRoleBinding($event, rolebinding: AuthRoleBinding) {
    this.selectedAuthRolebinding = rolebinding;
    this.isToShowAddRolebindingPanel = true;
  }
  /**
   * This API serves html template
   */
  showEditUserButton(user: AuthUser): boolean {
    const _ = Utility.getLodash();
    const value: any = _.result(user, 'status.roles');
    return (value && value.length > 0) || true;
  }

  /**
   * This API serves html template
   */
  showEditRoleButton(role: AuthRole): boolean {
    return true;
  }

  /**
   * This API serves html template
   */
  showEditBindingRoleButton(rolebinding: AuthRoleBinding): boolean {
    return true;
  }

  /**
   * This API serves html template
   */
  showDeleteUserButton(user: AuthUser): boolean {
    const _ = Utility.getLodash();
    const value: any = _.result(user, 'status.roles');
    const loginname = Utility.getInstance().getLoginName();
    return ((value && value.length > 0) || true) && (user.meta.name !== loginname);
  }

  /**
   * This API serves html template
   */
  showDeleteRoleBindingButton(rolebinding: AuthRoleBinding): boolean {
    return (this.authRoleBindings && this.authRoleBindings.length > 1);
  }

  /**
   * This API serves html template
   */
  showDeleteRoleButton(role: AuthRole): boolean {
    return (this.authRoles && this.authRoles.length > 1);
  }
  /**
   * This API serves html template.
   * If it is a save-user operation, refresh data
   */
  creationUserFormClose(isSaveData: boolean) {
    if (isSaveData) {
      this.getData();
    }
    this.isToShowAddUserPanel = false;
  }

  /**
  * This API serves html template.
  * If it is a save-user operation, refresh data
  */
  creationRoleFormClose(isSaveData: boolean) {
    if (isSaveData) {
      this.getData();
    }
    this.selectedAuthRole = null;
    this.isToShowAddRolePanel = false;
  }

  /**
  * This API serves html template.
  * If it is a save-user operation, refresh data
  */
  creationRoleBindingFormClose(isSaveData: boolean) {
    if (isSaveData) {
      this.getData();
    }
    this.selectedAuthRolebinding = null;
    this.isToShowAddRolebindingPanel = false;
  }
  /**
   * This API serves html template.
   * Check if user is in edit-mode
   */
  isUserEditMode(user: AuthUser): boolean {
    return (this.selectedAuthUser === user);
  }

  isRoleEditMode(role: AuthRole): boolean {
    return (this.selectedAuthRole === role);
  }

  isRoleBindingEditMode(rolebinding: AuthRoleBinding): boolean {
    return (this.selectedAuthRolebinding === rolebinding);
  }

  onCancelEditUser(event, user) {
    this.selectedAuthUser = null;
  }

  onCancelEditRole(event, role: AuthRole) {
    this.selectedAuthRole = null;
  }

  onSaveEditUser($event, user) {
    if (!this.isSelectedAuthUserInputValid()) {
      this._controllerService.invokeErrorToaster(Utility.UPDATE_FAILED_SUMMARY, 'There are invalid inputs.');
      return;
    }
    if (this.userEditAction === UsersComponent.USER_ACTION_UPDATE) {
      this.updateUser_with_staging();
    } else if (this.userEditAction === UsersComponent.USER_ACTION_CHANGEPWD) {
      this.changeUserPassword();
    }
  }
  /**
   * This API serves HTML template
   */
  isSelectedAuthUserInputValid() {
    let hasFormGroupError = null;
    if (this.userEditAction === UsersComponent.USER_ACTION_UPDATE) {
      hasFormGroupError = Utility.getAllFormgroupErrors(this.selectedAuthUser.$formGroup);
    } else if (this.userEditAction === UsersComponent.USER_ACTION_CHANGEPWD) {
      hasFormGroupError = Utility.getAllFormgroupErrors(this.authPasswordChangeRequest.$formGroup);
    }
    return (hasFormGroupError === null);
  }

  /**
   * Invoke REST API to change user password
   */
  changeUserPassword() {
    this._authService.PasswordChange(this.selectedAuthUser.meta.name, this.authPasswordChangeRequest.getFormGroupValues()).subscribe(
      response => {
        this._controllerService.invokeSuccessToaster('Change password Successful', 'Change Password ' + this.selectedAuthUser.meta.name);
        const updatedAuthUser: AuthUser = response.body as AuthUser;
        this.selectedAuthUser = updatedAuthUser;
        this.userEditAction = null;
      },
      this._controllerService.restErrorHandler(Utility.UPDATE_FAILED_SUMMARY)
    );
  }

  onSaveEditRole(event, role) {
    this.updateRole();
  }

  onRolebindingChange($event) {
    this.selectedRolebindingsForUsers = $event.value;
  }

  /**
   * Venice has role-bindings [rb1, rb2, rb3, rb4, rb5]
   * User initially binds to [rb1, rb2, rb3]
   * Update user to bind to [rb1, rb3, rb4]
   * We will remove user from rb1, add user to rb4
   */
  updateUser_with_staging() {
    const updateUser = this.selectedAuthUser.getFormGroupValues();
    updateUser.meta.name = this.selectedAuthUser.meta.name;  // sine we don't let change login name, we have to patch the meta.name

    let buffername;
    delete updateUser.status;  // remove status property
    this.createStagingBuffer().pipe(
      switchMap(responseBuffer => {
        const createdBuffer: StagingBuffer = responseBuffer.body as StagingBuffer;
        buffername = createdBuffer.meta.name;
        const observables: Observable<any>[] = [];
        const username = updateUser.meta.name;
        observables.push(this._authService.UpdateUser(this.selectedAuthUser.meta.name, updateUser, buffername));
        this.authRoleBindings.forEach((rb) => {
          const inRBlist = this.isUserAlreadyInRoleBinding(rb, username);
          if (inRBlist) {
            let inSelectedRBList = false;
            for (let i = 0; i < this.selectedRolebindingsForUsers.length; i++) {
              if (rb.meta.name === this.selectedRolebindingsForUsers[i].meta.name) {
                inSelectedRBList = true;
                break;
              }
            }
            if (!inSelectedRBList) {
              const observable = this.removeUserFromRolebing(rb, username, buffername);
              if (observable) {
                observables.push(observable);
              }
            }
          }
        });
        this.selectedRolebindingsForUsers.forEach((rolebinding) => {
          this.rolebindingUpdateMap[rolebinding.meta.name] = false;
          const observabe = this.addUserToRolebindings(rolebinding, username, buffername);
          if (observabe) {
            observables.push(observabe);
          }
        });
        if (observables.length > 0) {
          return this.invokeForkJoin(observables, buffername);
        } else {
          return this.commitStagingBuffer(buffername);
        }
      })
    ).subscribe(
      (responseCommitBuffer) => {
        this._controllerService.invokeSuccessToaster(Utility.UPDATE_SUCCESS_SUMMARY, ACTIONTYPE.UPDATE + ' User ' + updateUser.meta.name);
        this.creationRoleFormClose(true);
        this.getData();
      },
      (error) => {
        // any error in (A) (B) or (C), error will land here
        this._controllerService.invokeRESTErrorToaster(Utility.UPDATE_FAILED_SUMMARY, error);
        this.deleteStagingBuffer(buffername, 'Failed to delete', false);
      }
    );
  }

  /**
   * Invoke server REST API to update user information without having to include password
   */
  updateUser_without_staging() {
    this._authService.UpdateUser(this.selectedAuthUser.meta.name, this.selectedAuthUser.getFormGroupValues()).subscribe(
      response => {
        this._controllerService.invokeSuccessToaster(Utility.UPDATE_SUCCESS_SUMMARY, 'Updated user ' + this.selectedAuthUser.meta.name);
        const updatedAuthUser: AuthUser = response.body as AuthUser;
        this.userEditAction = null;
      },
      this._controllerService.restErrorHandler(Utility.UPDATE_FAILED_SUMMARY)
    );
  }

  /**
   * Invoke server REST API to update auth-role
   */
  updateRole() {
    // Updating role does not need commit-buffer.
    this._authService.UpdateRole(this.selectedAuthRole.meta.name, this.selectedAuthRole.getFormGroupValues()).subscribe(
      response => {
        this._controllerService.invokeSuccessToaster(Utility.UPDATE_SUCCESS_SUMMARY, 'Updated Role ' + this.selectedAuthUser.meta.name);
        this.getData();
      },
      this._controllerService.restErrorHandler(Utility.UPDATE_FAILED_SUMMARY)
    );
  }



  /**
   * Check if newUserName already exist.
   * We have to pass in authUsers as newUser.component is using this API.
   * @param newUsername
   * @param authUsers
   */
  isUserAlreadyExist(newUsername: string, authUsers: AuthUser[]): boolean {
    return this.isAuthPolicyObjectAlreadyExist(newUsername, authUsers);
  }

  /**
   * heck if newRolename already exist.
   * @param newRolename
   * @param authRoles
   */
  isRoleAlreadyExist(newRolename: string, authRoles: AuthRole[]): boolean {
    return this.isAuthPolicyObjectAlreadyExist(newRolename, authRoles);
  }

  /**
   * Check if newRolebindignname already exist.
   * @param newRolebindignname
   * @param authRolebindings
   */
  isRoleBindingAlreadyExist(newRolebindignname: string, authRolebindings: AuthRoleBinding[]): boolean {
    return this.isAuthPolicyObjectAlreadyExist(newRolebindignname, authRolebindings);
  }

  /**
   * Helper function to check if want-to-new auth-policy object (user/role/role-binding) has unique name
   * @param name
   * @param authpolicyObjects
   */
  isAuthPolicyObjectAlreadyExist(name: string, authpolicyObjects: AuthPolicyObject[]): boolean {
    const isIn = false;
    for (let i = 0; i < authpolicyObjects.length; i++) {
      const authObject = authpolicyObjects[i];
      if (authObject.meta.name === name) {
        return true;
      }
    }
    return false;
  }

  isUserAlreadyInRoleBinding(rolebinding: AuthRoleBinding, username: string): boolean {
    return (rolebinding.getFormGroupValues().spec.users.indexOf(username) >= 0);
  }

  addUserToRolebindings(rolebinding: AuthRoleBinding, username: string, buffername: string = ''): Observable<any> {
    if (this.isUserAlreadyInRoleBinding(rolebinding, username)) {
      return null; // Since username is already in rolebinding.users list, we will do nothing;
    }
    rolebinding.getFormGroupValues().spec.users.push(username);
    return this._authService.UpdateRoleBinding(rolebinding.meta.name, rolebinding.getFormGroupValues(), buffername);
  }

  removeUserFromRolebing(rolebinding: AuthRoleBinding, username: string, buffername: string = ''): Observable<any> {
    if (this.isUserAlreadyInRoleBinding(rolebinding, username)) {
      const users = rolebinding.getFormGroupValues().spec.users;
      const newuserList = [];
      users.forEach((uname, i) => {
        if (uname !== username) {
          newuserList.push(uname);
        }
      });
      rolebinding.getFormGroupValues().spec.users = newuserList;
      return this._authService.UpdateRoleBinding(rolebinding.meta.name, rolebinding.getFormGroupValues(), buffername);
    } else {
      return null;
    }
  }

  /**
   * This API serves html template
   */
  showEditButton(user: AuthUser) {
    const _ = Utility.getLodash();
    const value: any = _.result(user, 'status.roles');
    return (value && value.length > 0) || true;
  }

  /**
   * Server HTML template. Venice-user/admin wants to change password
   * @param $event
   * @param user
   */
  onChangePassword($event, user: AuthUser) {
    this.selectedAuthUser = user;
    this.userEditAction = UsersComponent.USER_ACTION_CHANGEPWD;
    this.authPasswordChangeRequest = new AuthPasswordChangeRequest();
    const selectedUserData = this.selectedAuthUser.getFormGroupValues();
    this.authPasswordChangeRequest.setValues(selectedUserData); // this will populate data.
    // note: auth.proto set new/odd password validation rules.
  }

}
