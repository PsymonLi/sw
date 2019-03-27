package auth

import (
	"context"
	"fmt"
	"strings"
	"testing"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/auth"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/staging"
	"github.com/pensando/sw/api/login"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/authz"

	. "github.com/pensando/sw/test/utils"
	. "github.com/pensando/sw/venice/utils/authn/testutils"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestAuthorization(t *testing.T) {
	userCred := &auth.PasswordCredential{
		Username: testUser,
		Password: testPassword,
		Tenant:   testTenant,
	}
	// create tenant and admin user
	if err := SetupAuth(tinfo.apiServerAddr, true, &auth.Ldap{Enabled: false}, &auth.Radius{Enabled: false}, userCred, tinfo.l); err != nil {
		t.Fatalf("auth setup failed")
	}
	defer CleanupAuth(tinfo.apiServerAddr, true, false, userCred, tinfo.l)

	ctx, err := NewLoggedInContext(context.Background(), tinfo.apiGwAddr, userCred)
	AssertOk(t, err, "error creating logged in context")
	// retrieve auth policy
	AssertConsistently(t, func() (bool, interface{}) {
		_, err = tinfo.restcl.AuthV1().AuthenticationPolicy().Get(ctx, &api.ObjectMeta{Name: "AuthenticationPolicy"})
		return err != nil, nil
	}, "authorization error expected while retrieving authentication policy", "100ms", "1s")
	// retrieve tenant
	AssertConsistently(t, func() (bool, interface{}) {
		_, err := tinfo.restcl.ClusterV1().Tenant().Get(ctx, &api.ObjectMeta{Name: testTenant})
		return err != nil, nil
	}, "authorization error expected while retrieving tenant", "100ms", "1s")
	// retrieve role
	var role *auth.Role
	AssertEventually(t, func() (bool, interface{}) {
		role, err = tinfo.restcl.AuthV1().Role().Get(ctx, &api.ObjectMeta{Name: globals.AdminRole, Tenant: testTenant})
		return err == nil, nil
	}, fmt.Sprintf("error while retrieving role: Err: %v", err))
	Assert(t, role.Name == globals.AdminRole && role.Tenant == testTenant, fmt.Sprintf("incorrect role retrieved [%#v]", role))
	const testTenant2 = "testtenant2"

	// create testtenant2 and admin user
	MustCreateTenant(tinfo.apicl, testTenant2)
	defer MustDeleteTenant(tinfo.apicl, testTenant2)
	MustCreateTestUser(tinfo.apicl, testUser, testPassword, testTenant2)
	defer MustDeleteUser(tinfo.apicl, testUser, testTenant2)
	MustUpdateRoleBinding(tinfo.apicl, globals.AdminRoleBinding, testTenant2, globals.AdminRole, []string{testUser}, nil)
	defer MustUpdateRoleBinding(tinfo.apicl, globals.AdminRoleBinding, testTenant2, globals.AdminRole, nil, nil)
	ctx, err = NewLoggedInContext(context.Background(), tinfo.apiGwAddr, &auth.PasswordCredential{Username: testUser, Password: testPassword, Tenant: testTenant2})
	AssertOk(t, err, "error creating logged in context for testtenant2 admin user")
	// tenant boundaries should be respected; retrieving testtenant AdminRole should fail
	AssertConsistently(t, func() (bool, interface{}) {
		_, err := tinfo.restcl.AuthV1().Role().Get(ctx, &api.ObjectMeta{Name: globals.AdminRole, Tenant: testTenant})
		return err != nil, nil
	}, "authorization error expected while retrieve other tenant's AdminRole", "100ms", "1s")
	AssertConsistently(t, func() (bool, interface{}) {
		_, err := tinfo.restcl.AuthV1().User().Get(ctx, &api.ObjectMeta{Name: testUser, Tenant: testTenant})
		return err != nil, nil
	}, "authorization error expected while retrieve other tenant's admin user", "100ms", "1s")
}

func TestAdminRole(t *testing.T) {
	adminCred := &auth.PasswordCredential{
		Username: testUser,
		Password: testPassword,
		Tenant:   globals.DefaultTenant,
	}
	// create default tenant and global admin user
	if err := SetupAuth(tinfo.apiServerAddr, true, &auth.Ldap{Enabled: false}, &auth.Radius{Enabled: false}, adminCred, tinfo.l); err != nil {
		t.Fatalf("auth setup failed")
	}
	defer CleanupAuth(tinfo.apiServerAddr, true, false, adminCred, tinfo.l)

	superAdminCtx, err := NewLoggedInContext(context.Background(), tinfo.apiGwAddr, adminCred)
	AssertOk(t, err, "error creating logged in context")

	AssertConsistently(t, func() (bool, interface{}) {
		_, err := tinfo.restcl.AuthV1().Role().Delete(superAdminCtx, &api.ObjectMeta{Name: globals.AdminRole, Tenant: globals.DefaultTenant})
		tinfo.l.Infof("admin role delete error: %v", err)
		return err != nil, err
	}, "admin role shouldn't be deleted", "100ms", "1s")
	AssertConsistently(t, func() (bool, interface{}) {
		_, err := tinfo.restcl.AuthV1().Role().Create(superAdminCtx, login.NewRole(globals.AdminRole, globals.DefaultTenant, login.NewPermission(
			authz.ResourceTenantAll,
			authz.ResourceGroupAll,
			authz.ResourceKindAll,
			authz.ResourceNamespaceAll,
			"",
			auth.Permission_AllActions.String())))
		tinfo.l.Infof("admin role create error: %v", err)
		return err != nil, err
	}, "admin role shouldn't be created", "100ms", "1s")
	AssertConsistently(t, func() (bool, interface{}) {
		_, err := tinfo.restcl.AuthV1().Role().Update(superAdminCtx, login.NewRole(globals.AdminRole, globals.DefaultTenant, login.NewPermission(
			authz.ResourceTenantAll,
			authz.ResourceGroupAll,
			authz.ResourceKindAll,
			authz.ResourceNamespaceAll,
			"",
			auth.Permission_AllActions.String())))
		tinfo.l.Infof("admin role update error: %v", err)
		return err != nil, err
	}, "admin role shouldn't be updated", "100ms", "1s")
	currTime := time.Now()
	MustCreateTenant(tinfo.apicl, testTenant)
	var adminRole *auth.Role
	AssertEventually(t, func() (bool, interface{}) {
		adminRole, err = tinfo.restcl.AuthV1().Role().Get(superAdminCtx, &api.ObjectMeta{Name: globals.AdminRole, Tenant: testTenant})
		return err == nil, err
	}, "admin role should be created when a tenant is created")
	creationTime, err := adminRole.CreationTime.Time()
	AssertOk(t, err, "error getting role creation time")
	Assert(t, creationTime.After(currTime), "admin role creation time is not set")
	Assert(t, adminRole.UUID != "", "admin role UUID is not set")
	MustDeleteTenant(tinfo.apicl, testTenant)
	AssertEventually(t, func() (bool, interface{}) {
		_, err := tinfo.restcl.AuthV1().Role().Get(superAdminCtx, &api.ObjectMeta{Name: globals.AdminRole, Tenant: testTenant})
		return err != nil, err
	}, "admin role should be deleted when a tenant is deleted")
}

func TestAdminRoleBinding(t *testing.T) {
	adminCred := &auth.PasswordCredential{
		Username: testUser,
		Password: testPassword,
		Tenant:   globals.DefaultTenant,
	}
	// create default tenant and global admin user
	if err := SetupAuth(tinfo.apiServerAddr, true, &auth.Ldap{Enabled: false}, &auth.Radius{Enabled: false}, adminCred, tinfo.l); err != nil {
		t.Fatalf("auth setup failed")
	}
	defer CleanupAuth(tinfo.apiServerAddr, true, false, adminCred, tinfo.l)

	superAdminCtx, err := NewLoggedInContext(context.Background(), tinfo.apiGwAddr, adminCred)
	AssertOk(t, err, "error creating logged in context")
	// Deleting admin role binding should fail
	_, err = tinfo.restcl.AuthV1().RoleBinding().Delete(superAdminCtx, &api.ObjectMeta{Name: globals.AdminRoleBinding, Tenant: globals.DefaultTenant})
	Assert(t, err != nil, "expected error while deleting admin role binding")
	// test admin role binding creation and deletion
	currTime := time.Now()
	MustCreateTenant(tinfo.apicl, testTenant)
	var adminRoleBinding *auth.RoleBinding
	AssertEventually(t, func() (bool, interface{}) {
		adminRoleBinding, err = tinfo.restcl.AuthV1().RoleBinding().Get(superAdminCtx, &api.ObjectMeta{Name: globals.AdminRoleBinding, Tenant: testTenant})
		return err == nil, err
	}, "admin role binding should be created when a tenant is created")
	creationTime, err := adminRoleBinding.CreationTime.Time()
	AssertOk(t, err, "error getting role binding creation time")
	Assert(t, creationTime.After(currTime), "admin role binding creation time is not set")
	Assert(t, adminRoleBinding.UUID != "", "admin role binding UUID is not set")
	// updating AdminRoleBinding to refer to a non-admin role should fail
	MustCreateRole(tinfo.apicl, "NetworkAdminRole", testTenant, login.NewPermission(
		testTenant,
		string(apiclient.GroupNetwork),
		string(network.KindNetwork),
		authz.ResourceNamespaceAll,
		"",
		auth.Permission_AllActions.String()),
		login.NewPermission(
			testTenant,
			string(apiclient.GroupAuth),
			authz.ResourceKindAll,
			authz.ResourceNamespaceAll,
			"",
			auth.Permission_AllActions.String()))
	adminRoleBinding.Spec.Role = "NetworkAdminRole"
	_, err = tinfo.restcl.AuthV1().RoleBinding().Update(superAdminCtx, adminRoleBinding)
	// cleanup objs in tenant before deleting
	MustDeleteRole(tinfo.apicl, "NetworkAdminRole", testTenant)
	Assert(t, err != nil, "expected error while updating AdminRoleBinding to have non-admin role")
	Assert(t, strings.Contains(err.Error(), "AdminRoleBinding can bind to only AdminRole"), fmt.Sprintf("unexpected error: %v", err))
	// deleting tenant should delete AdminRoleBinding
	MustDeleteTenant(tinfo.apicl, testTenant)
	AssertEventually(t, func() (bool, interface{}) {
		_, err := tinfo.restcl.AuthV1().RoleBinding().Get(superAdminCtx, &api.ObjectMeta{Name: globals.AdminRoleBinding, Tenant: testTenant})
		return err != nil, err
	}, "admin role binding should be deleted when a tenant is deleted")
}

func TestPrivilegeEscalation(t *testing.T) {
	userCred := &auth.PasswordCredential{
		Username: testUser,
		Password: testPassword,
		Tenant:   testTenant,
	}
	// create tenant and admin user
	if err := SetupAuth(tinfo.apiServerAddr, true, &auth.Ldap{Enabled: false}, &auth.Radius{Enabled: false}, userCred, tinfo.l); err != nil {
		t.Fatalf("auth setup failed")
	}
	defer CleanupAuth(tinfo.apiServerAddr, true, false, userCred, tinfo.l)

	ctx, err := NewLoggedInContext(context.Background(), tinfo.apiGwAddr, userCred)
	AssertOk(t, err, "error creating logged in context")

	AssertConsistently(t, func() (bool, interface{}) {
		_, err := tinfo.restcl.AuthV1().Role().Update(ctx, login.NewRole(globals.AdminRole, testTenant, login.NewPermission(
			authz.ResourceTenantAll,
			authz.ResourceGroupAll,
			authz.ResourceKindAll,
			authz.ResourceNamespaceAll,
			"",
			auth.Permission_AllActions.String())))
		tinfo.l.Infof("global admin role update error: %v", err)
		return err != nil, err
	}, "tenant admin should not be able to update tenant admin role to global admin role", "100ms", "1s")

	// create testUser2 as network admin with permissions to create RBAC objects also
	MustCreateTestUser(tinfo.apicl, "testUser2", testPassword, testTenant)
	defer MustDeleteUser(tinfo.apicl, "testUser2", testTenant)
	MustCreateRole(tinfo.apicl, "NetworkAdminRole", testTenant, login.NewPermission(
		testTenant,
		string(apiclient.GroupNetwork),
		string(network.KindNetwork),
		authz.ResourceNamespaceAll,
		"",
		auth.Permission_AllActions.String()),
		login.NewPermission(
			testTenant,
			string(apiclient.GroupAuth),
			authz.ResourceKindAll,
			authz.ResourceNamespaceAll,
			"",
			auth.Permission_AllActions.String()))
	defer MustDeleteRole(tinfo.apicl, "NetworkAdminRole", testTenant)
	roleBinding := MustCreateRoleBinding(tinfo.apicl, "NetworkAdminRoleBinding", testTenant, "NetworkAdminRole", []string{"testUser2"}, nil)
	defer MustDeleteRoleBinding(tinfo.apicl, "NetworkAdminRoleBinding", testTenant)
	// login as network admin
	ctx, err = NewLoggedInContext(context.Background(), tinfo.apiGwAddr, &auth.PasswordCredential{
		Username: "testUser2",
		Password: testPassword,
		Tenant:   testTenant,
	})
	AssertOk(t, err, "error creating logged in context")
	// assign tenant admin role
	roleBinding.Spec.Role = globals.AdminRole
	AssertConsistently(t, func() (bool, interface{}) {
		_, err := tinfo.restcl.AuthV1().RoleBinding().Update(ctx, roleBinding)
		tinfo.l.Infof("network admin role binding update error: %v", err)
		return err != nil, err
	}, "network admin should not be able to update its role binding to assign himself tenant admin role", "100ms", "1s")
	AssertConsistently(t, func() (bool, interface{}) {
		_, err := tinfo.restcl.AuthV1().Role().Create(ctx, login.NewRole("NetworkGroupAdminRole", testTenant, login.NewPermission(
			testTenant,
			string(apiclient.GroupNetwork),
			authz.ResourceKindAll,
			authz.ResourceNamespaceAll,
			"",
			auth.Permission_AllActions.String())))
		tinfo.l.Infof("network group admin role create error: %v", err)
		return err != nil, err
	}, "network admin should not be able to create role with privileges to all kinds within network group", "100ms", "1s")
	AssertEventually(t, func() (bool, interface{}) {
		_, err := tinfo.restcl.AuthV1().Role().Create(ctx, login.NewRole("LimitedNetworkRole", testTenant, login.NewPermission(
			testTenant,
			string(apiclient.GroupNetwork),
			string(network.KindNetwork),
			authz.ResourceNamespaceAll,
			"testnetwork",
			auth.Permission_Read.String())))
		if err == nil {
			MustDeleteRole(tinfo.apicl, "LimitedNetworkRole", testTenant)
		}
		return err == nil, err
	}, "network admin should be able to create role with lesser privileges")
}

func TestBootstrapFlag(t *testing.T) {
	// create cluster object
	clusterObj := MustCreateCluster(tinfo.apicl)
	defer MustDeleteCluster(tinfo.apicl)
	// create tenant
	MustCreateTenant(tinfo.restcl, globals.DefaultTenant)
	defer MustDeleteTenant(tinfo.apicl, globals.DefaultTenant)
	AssertConsistently(t, func() (bool, interface{}) {
		clusterObj, err := tinfo.restcl.ClusterV1().Cluster().AuthBootstrapComplete(context.TODO(), &cluster.ClusterAuthBootstrapRequest{})
		tinfo.l.Infof("set bootstrap flag error: %v", err)
		return err != nil, clusterObj
	}, "bootstrap flag shouldn't be set till auth policy is created", "100ms", "1s")
	// create auth policy
	MustCreateAuthenticationPolicy(tinfo.restcl, &auth.Local{Enabled: true}, &auth.Ldap{Enabled: false}, &auth.Radius{Enabled: false})
	defer MustDeleteAuthenticationPolicy(tinfo.apicl)
	AssertConsistently(t, func() (bool, interface{}) {
		clusterObj, err := tinfo.restcl.ClusterV1().Cluster().AuthBootstrapComplete(context.TODO(), &cluster.ClusterAuthBootstrapRequest{})
		tinfo.l.Infof("set bootstrap flag error: %v", err)
		return err != nil, clusterObj
	}, "bootstrap flag shouldn't be set till admin role binding is updated with an user", "100ms", "1s")
	MustCreateTestUser(tinfo.restcl, testUser, testPassword, globals.DefaultTenant)
	defer MustDeleteUser(tinfo.apicl, testUser, globals.DefaultTenant)
	MustUpdateRoleBinding(tinfo.restcl, globals.AdminRoleBinding, globals.DefaultTenant, globals.AdminRole, []string{testUser}, nil)
	defer MustUpdateRoleBinding(tinfo.apicl, globals.AdminRoleBinding, globals.DefaultTenant, globals.AdminRole, nil, nil)
	// set bootstrap flag
	AssertEventually(t, func() (bool, interface{}) {
		var err error
		clusterObj, err = tinfo.restcl.ClusterV1().Cluster().AuthBootstrapComplete(context.TODO(), &cluster.ClusterAuthBootstrapRequest{})
		return err == nil, err
	}, "error setting bootstrap flag")
	Assert(t, clusterObj.Status.AuthBootstrapped, "bootstrap flag should be set to true")
	// once set, setting bootstrap flag will now need proper authorization
	AssertEventually(t, func() (bool, interface{}) {
		clusterObj, err := tinfo.restcl.ClusterV1().Cluster().AuthBootstrapComplete(context.TODO(), &cluster.ClusterAuthBootstrapRequest{})
		tinfo.l.Infof("set bootstrap flag error: %v", err)
		return err != nil, clusterObj
	}, "unauthenticated request to set bootstrap flag should fail")
	adminCred := &auth.PasswordCredential{
		Username: testUser,
		Password: testPassword,
		Tenant:   globals.DefaultTenant,
	}
	// login in as global admin
	superAdminCtx, err := NewLoggedInContext(context.TODO(), tinfo.apiGwAddr, adminCred)
	AssertOk(t, err, "error creating logged in context")
	AssertEventually(t, func() (bool, interface{}) {
		clusterObj, err = tinfo.restcl.ClusterV1().Cluster().AuthBootstrapComplete(superAdminCtx, &cluster.ClusterAuthBootstrapRequest{})
		return err == nil, err
	}, "error setting bootstrap flag")
	Assert(t, clusterObj.Status.AuthBootstrapped, "bootstrap flag should be set to true")
	// try to unset bootstrap flag by updating cluster obj through API server
	clusterObj.Status.AuthBootstrapped = false
	updatedClusterObj, err := tinfo.apicl.ClusterV1().Cluster().Update(context.TODO(), clusterObj)
	AssertOk(t, err, "error updating cluster obj")
	Assert(t, updatedClusterObj.Status.AuthBootstrapped, "bootstrap flag should be un-settable once set to true")
}

func TestAPIGroupAuthorization(t *testing.T) {
	adminCred := &auth.PasswordCredential{
		Username: testUser,
		Password: testPassword,
		Tenant:   globals.DefaultTenant,
	}
	// create default tenant and global admin user
	if err := SetupAuth(tinfo.apiServerAddr, true, &auth.Ldap{Enabled: false}, &auth.Radius{Enabled: false}, adminCred, tinfo.l); err != nil {
		t.Fatalf("auth setup failed")
	}
	defer CleanupAuth(tinfo.apiServerAddr, true, false, adminCred, tinfo.l)

	// create testUser2 as user admin with permissions to create RBAC objects
	MustCreateTestUser(tinfo.apicl, "testUser2", testPassword, globals.DefaultTenant)
	defer MustDeleteUser(tinfo.apicl, "testUser2", globals.DefaultTenant)
	MustCreateRole(tinfo.apicl, "UserAdminRole", globals.DefaultTenant,
		login.NewPermission(
			globals.DefaultTenant,
			string(apiclient.GroupAuth),
			authz.ResourceKindAll,
			authz.ResourceNamespaceAll,
			"",
			auth.Permission_AllActions.String()))
	defer MustDeleteRole(tinfo.apicl, "UserAdminRole", globals.DefaultTenant)
	// login as user admin
	ctx, err := NewLoggedInContext(context.Background(), tinfo.apiGwAddr, &auth.PasswordCredential{
		Username: "testUser2",
		Password: testPassword,
		Tenant:   globals.DefaultTenant,
	})
	AssertOk(t, err, "error creating logged in context")
	// retrieving role should result in authorization error at this point
	AssertConsistently(t, func() (bool, interface{}) {
		_, err := tinfo.restcl.AuthV1().Role().Get(ctx, &api.ObjectMeta{Name: "UserAdminRole", Tenant: globals.DefaultTenant})
		return err != nil, nil
	}, "authorization error expected while retrieving role", "100ms", "1s")
	MustCreateRoleBinding(tinfo.apicl, "UserAdminRoleBinding", globals.DefaultTenant, "UserAdminRole", []string{"testUser2"}, nil)
	defer MustDeleteRoleBinding(tinfo.apicl, "UserAdminRoleBinding", globals.DefaultTenant)
	// retrieving role should succeed now
	AssertEventually(t, func() (bool, interface{}) {
		_, err = tinfo.restcl.AuthV1().Role().Get(ctx, &api.ObjectMeta{Name: "UserAdminRole", Tenant: globals.DefaultTenant})
		return err == nil, nil
	}, fmt.Sprintf("error while retrieving role: %v", err))
	AssertEventually(t, func() (bool, interface{}) {
		_, err = tinfo.restcl.AuthV1().RoleBinding().Get(ctx, &api.ObjectMeta{Name: "UserAdminRoleBinding", Tenant: globals.DefaultTenant})
		return err == nil, nil
	}, fmt.Sprintf("error while retrieving role binding: %v", err))
	// retrieving tenant should fail due to authorization error
	AssertConsistently(t, func() (bool, interface{}) {
		_, err := tinfo.restcl.ClusterV1().Tenant().Get(ctx, &api.ObjectMeta{Name: globals.DefaultTenant})
		return err != nil, nil
	}, "authorization error expected while retrieving tenant", "100ms", "1s")
}

func TestUserSelfOperations(t *testing.T) {
	userCred := &auth.PasswordCredential{
		Username: testUser,
		Password: testPassword,
		Tenant:   testTenant,
	}
	// create tenant and admin user
	if err := SetupAuth(tinfo.apiServerAddr, true, &auth.Ldap{Enabled: false}, &auth.Radius{Enabled: false}, userCred, tinfo.l); err != nil {
		t.Fatalf("auth setup failed")
	}
	defer CleanupAuth(tinfo.apiServerAddr, true, false, userCred, tinfo.l)

	MustCreateTestUser(tinfo.apicl, "testUser2", testPassword, testTenant)
	defer MustDeleteUser(tinfo.apicl, "testUser2", testTenant)
	// login as testUser2 who has no roles assigned
	ctx, err := NewLoggedInContext(context.TODO(), tinfo.apiGwAddr, &auth.PasswordCredential{Username: "testUser2", Password: testPassword, Tenant: testTenant})
	AssertOk(t, err, "error creating logged in context")

	// test get user
	var user *auth.User
	AssertEventually(t, func() (bool, interface{}) {
		user, err = tinfo.restcl.AuthV1().User().Get(ctx, &api.ObjectMeta{Name: "testUser2", Tenant: testTenant})
		return err == nil, err
	}, "unable to get user")
	Assert(t, user.Name == "testUser2" && user.Tenant == testTenant, fmt.Sprintf("user get failed: %#v", *user))
	// test update user
	AssertEventually(t, func() (bool, interface{}) {
		user, err = tinfo.restcl.AuthV1().User().Update(ctx, &auth.User{
			TypeMeta: api.TypeMeta{Kind: string(auth.KindUser)},
			ObjectMeta: api.ObjectMeta{
				Tenant: testTenant,
				Name:   "testUser2",
			},
			Spec: auth.UserSpec{
				Fullname: "Test User2",
				Email:    "testuser@pensandio.io",
				Type:     auth.UserSpec_Local.String(),
			},
		})
		return err == nil, err
	}, "unable to update user")
	Assert(t, user.Spec.Fullname == "Test User2", fmt.Sprintf("user update failed: %#v", *user))
	// test change password
	const newPassword = "Newpassword1#"
	AssertEventually(t, func() (bool, interface{}) {
		_, err := tinfo.restcl.AuthV1().User().PasswordChange(ctx, &auth.PasswordChangeRequest{
			TypeMeta: api.TypeMeta{
				Kind: string(auth.KindUser),
			},
			ObjectMeta: api.ObjectMeta{
				Name:   "testUser2",
				Tenant: testTenant,
			},
			OldPassword: testPassword,
			NewPassword: newPassword,
		})
		return err == nil, err
	}, "unable to change password")
	ctx, err = NewLoggedInContext(context.TODO(), tinfo.apiGwAddr, &auth.PasswordCredential{Username: "testUser2", Password: newPassword, Tenant: testTenant})
	AssertOk(t, err, "unable to get logged in context with new password")
	// test reset password
	AssertEventually(t, func() (bool, interface{}) {
		user, err = tinfo.restcl.AuthV1().User().PasswordReset(ctx, &auth.PasswordResetRequest{
			TypeMeta: api.TypeMeta{
				Kind: string(auth.KindUser),
			},
			ObjectMeta: api.ObjectMeta{
				Name:   "testUser2",
				Tenant: testTenant,
			},
		})
		return err == nil, err
	}, "unable to reset password")
	ctx, err = NewLoggedInContext(context.TODO(), tinfo.apiGwAddr, &auth.PasswordCredential{Username: "testUser2", Password: user.Spec.Password, Tenant: testTenant})
	AssertOk(t, err, "unable to get logged in context with new reset password")
}

func TestUserDelete(t *testing.T) {
	userCred := &auth.PasswordCredential{
		Username: testUser,
		Password: testPassword,
		Tenant:   testTenant,
	}
	// create tenant and admin user
	if err := SetupAuth(tinfo.apiServerAddr, true, &auth.Ldap{Enabled: false}, &auth.Radius{Enabled: false}, userCred, tinfo.l); err != nil {
		t.Fatalf("auth setup failed")
	}
	defer CleanupAuth(tinfo.apiServerAddr, true, false, userCred, tinfo.l)

	// user should not be able to delete itself
	adminCtx, err := NewLoggedInContext(context.TODO(), tinfo.apiGwAddr, userCred)
	AssertOk(t, err, "unable to get admin logged in context")
	_, err = tinfo.restcl.AuthV1().User().Delete(adminCtx, &api.ObjectMeta{Name: userCred.Username, Tenant: userCred.Tenant})
	Assert(t, err != nil, "user should not be able to delete himself")
	Assert(t, strings.Contains(err.Error(), "self-deletion of user is not allowed"), fmt.Sprintf("unexpected error: %v", err))
	// should be able to delete another admin user
	MustCreateTestUser(tinfo.apicl, "admin", testPassword, testTenant)
	MustCreateRoleBinding(tinfo.apicl, "AdminRoleBinding2", testTenant, globals.AdminRole, []string{"admin"}, nil)
	MustDeleteRoleBinding(tinfo.apicl, "AdminRoleBinding2", testTenant)
	AssertEventually(t, func() (bool, interface{}) {
		_, err := tinfo.restcl.AuthV1().User().Delete(adminCtx, &api.ObjectMeta{Name: "admin", Tenant: testTenant})
		return err == nil, err
	}, "unable to delete another admin user")
}

func TestCommitBuffer(t *testing.T) {
	// setup auth
	// create staging buffer
	// stage role
	// stage role binding
	// commit buffer
	// get role, rolebinding
	userCred := &auth.PasswordCredential{
		Username: testUser,
		Password: testPassword,
		Tenant:   globals.DefaultTenant,
	}
	// create tenant and admin user
	if err := SetupAuth(tinfo.apiServerAddr, true, &auth.Ldap{Enabled: false}, &auth.Radius{Enabled: false}, userCred, tinfo.l); err != nil {
		t.Fatalf("auth setup failed")
	}
	defer CleanupAuth(tinfo.apiServerAddr, true, false, userCred, tinfo.l)

	ctx, err := NewLoggedInContext(context.Background(), tinfo.apiGwAddr, userCred)
	AssertOk(t, err, "error creating logged in context")
	AssertEventually(t, func() (bool, interface{}) {
		_, err = tinfo.restcl.StagingV1().Buffer().Create(ctx, &staging.Buffer{ObjectMeta: api.ObjectMeta{Name: "TestBuffer", Tenant: globals.DefaultTenant}})
		return err == nil, nil
	}, fmt.Sprintf("unable to create staging buffer, err: %v", err))
	defer tinfo.restcl.StagingV1().Buffer().Delete(ctx, &api.ObjectMeta{Name: "TestBuffer", Tenant: globals.DefaultTenant})
	stagecl, err := apiclient.NewStagedRestAPIClient(tinfo.apiGwAddr, "TestBuffer")
	AssertOk(t, err, "error creating staging client")
	defer stagecl.Close()
	MustCreateRoleWithCtx(ctx, stagecl, "NetworkAdminRole", globals.DefaultTenant, login.NewPermission(
		globals.DefaultTenant,
		string(apiclient.GroupNetwork),
		string(network.KindNetwork),
		authz.ResourceNamespaceAll,
		"",
		auth.Permission_AllActions.String()),
		login.NewPermission(
			globals.DefaultTenant,
			string(apiclient.GroupAuth),
			authz.ResourceKindAll,
			authz.ResourceNamespaceAll,
			"",
			auth.Permission_AllActions.String()))
	MustCreateRoleBindingWithCtx(ctx, stagecl, "NetworkAdminRoleBinding", globals.DefaultTenant, "NetworkAdminRole", []string{testUser}, nil)
	ca := staging.CommitAction{}
	ca.Name = "TestBuffer"
	ca.Tenant = globals.DefaultTenant
	_, err = tinfo.restcl.StagingV1().Buffer().Commit(ctx, &ca)
	AssertOk(t, err, "unable to commit staging buffer")
	AssertEventually(t, func() (bool, interface{}) {
		_, err = tinfo.restcl.AuthV1().Role().Get(ctx, &api.ObjectMeta{Name: "NetworkAdminRole", Tenant: globals.DefaultTenant})
		return err == nil, nil
	}, fmt.Sprintf("error retrieving role: %v", err))
	AssertEventually(t, func() (bool, interface{}) {
		_, err = tinfo.restcl.AuthV1().RoleBinding().Get(ctx, &api.ObjectMeta{Name: "NetworkAdminRoleBinding", Tenant: globals.DefaultTenant})
		return err == nil, nil
	}, fmt.Sprintf("error retrieving role binding: %v", err))
	MustDeleteRoleBinding(tinfo.apicl, "NetworkAdminRoleBinding", globals.DefaultTenant)
	MustDeleteRole(tinfo.apicl, "NetworkAdminRole", globals.DefaultTenant)
}
