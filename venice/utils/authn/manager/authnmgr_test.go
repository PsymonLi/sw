package manager

import (
	"fmt"
	"os"
	"testing"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/auth"
	"github.com/pensando/sw/venice/apiserver"
	"github.com/pensando/sw/venice/apiserver/pkg"
	"github.com/pensando/sw/venice/utils/authn"
	. "github.com/pensando/sw/venice/utils/authn/testutils"
	"github.com/pensando/sw/venice/utils/kvstore/store"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/runtime"
	. "github.com/pensando/sw/venice/utils/testutils"

	_ "github.com/pensando/sw/api/generated/exports/apiserver"
	_ "github.com/pensando/sw/api/hooks/apiserver"
)

const (
	apisrvURL    = "localhost:0"
	testUser     = "test"
	testPassword = "pensandoo0"
	tenant       = "default"
	expiration   = 600
)

var apicl apiclient.Services
var apiSrv apiserver.Server
var apiSrvAddr string
var secret []byte
var authnmgr *AuthenticationManager

// testJWTToken for benchmarking
var testHS512JWTToken string

func TestMain(m *testing.M) {
	setup()
	code := m.Run()
	shutdown()
	os.Exit(code)
}

func setup() {
	// api server
	apiSrv = createAPIServer(apisrvURL)
	if apiSrv == nil {
		panic("Unable to create API Server")
	}
	var err error
	apiSrvAddr, err = apiSrv.GetAddr()
	if err != nil {
		panic("Unable to get API Server address")
	}
	// api server client
	logger := log.WithContext("Pkg", "authnmgr_test")
	apicl, err = apiclient.NewGrpcAPIClient("authnmgr_test", apiSrvAddr, logger)
	if err != nil {
		panic("Error creating api client")
	}

	// create test user
	MustCreateTestUser(apicl, testUser, testPassword, "default")

	// create secret for jwt tests
	secret, err = authn.CreateSecret(128)
	if err != nil {
		panic(fmt.Sprintf("Error generating secret: Err: %v", err))
	}
	testHS512JWTToken = createHeadlessToken(signatureAlgorithm, secret, time.Duration(expiration)*time.Second, issuerClaimValue)

	// create authentication manager
	authnmgr, err = NewAuthenticationManager("authnmgr_test", apiSrvAddr, nil, time.Duration(expiration))
	if err != nil {
		panic("Error creating authentication manager")
	}
}

func shutdown() {
	// stop api server
	apiSrv.Stop()
	// un-initialize authentication manager
	authnmgr.Uninitialize()
}

func createAPIServer(url string) apiserver.Server {
	logger := log.WithContext("Pkg", "authnmgr_test")

	// api server config
	sch := runtime.GetDefaultScheme()
	apisrvConfig := apiserver.Config{
		GrpcServerPort: url,
		Logger:         logger,
		Version:        "v1",
		Scheme:         sch,
		Kvstore: store.Config{
			Type:    store.KVStoreTypeMemkv,
			Servers: []string{""},
			Codec:   runtime.NewJSONCodec(sch),
		},
	}
	// create api server
	apiSrv := apisrvpkg.MustGetAPIServer()
	go apiSrv.Run(apisrvConfig)
	apiSrv.WaitRunning()

	return apiSrv
}

// authenticationPoliciesData returns policies configured with Local and LDAP authenticators in different order
func authenticationPoliciesData() map[string]*auth.AuthenticationPolicy {
	policydata := make(map[string]*auth.AuthenticationPolicy)

	policydata["LDAP enabled, Local enabled"] = &auth.AuthenticationPolicy{
		TypeMeta: api.TypeMeta{Kind: "AuthenticationPolicy"},
		ObjectMeta: api.ObjectMeta{
			Name: "AuthenticationPolicy",
		},
		Spec: auth.AuthenticationPolicySpec{
			Authenticators: auth.Authenticators{
				Ldap: &auth.Ldap{
					Enabled: true,
				},
				Local: &auth.Local{
					Enabled: true,
				},
				AuthenticatorOrder: []string{auth.Authenticators_LDAP.String(), auth.Authenticators_LOCAL.String()},
			},
		},
	}
	policydata["LDAP disabled, Local enabled"] = &auth.AuthenticationPolicy{
		TypeMeta: api.TypeMeta{Kind: "AuthenticationPolicy"},
		ObjectMeta: api.ObjectMeta{
			Name: "AuthenticationPolicy",
		},
		Spec: auth.AuthenticationPolicySpec{
			Authenticators: auth.Authenticators{
				Ldap: &auth.Ldap{
					Enabled: false,
				},
				Local: &auth.Local{
					Enabled: true,
				},
				AuthenticatorOrder: []string{auth.Authenticators_LDAP.String(), auth.Authenticators_LOCAL.String()},
			},
		},
	}
	policydata["Local enabled, LDAP enabled"] = &auth.AuthenticationPolicy{
		TypeMeta: api.TypeMeta{Kind: "AuthenticationPolicy"},
		ObjectMeta: api.ObjectMeta{
			Name: "AuthenticationPolicy",
		},
		Spec: auth.AuthenticationPolicySpec{
			Authenticators: auth.Authenticators{
				Ldap: &auth.Ldap{
					Enabled: true,
				},
				Local: &auth.Local{
					Enabled: true,
				},
				AuthenticatorOrder: []string{auth.Authenticators_LOCAL.String(), auth.Authenticators_LDAP.String()},
			},
		},
	}
	policydata["Local enabled, LDAP disabled"] = &auth.AuthenticationPolicy{
		TypeMeta: api.TypeMeta{Kind: "AuthenticationPolicy"},
		ObjectMeta: api.ObjectMeta{
			Name: "AuthenticationPolicy",
		},
		Spec: auth.AuthenticationPolicySpec{
			Authenticators: auth.Authenticators{
				Ldap: &auth.Ldap{
					Enabled: false,
				},
				Local: &auth.Local{
					Enabled: true,
				},
				AuthenticatorOrder: []string{auth.Authenticators_LOCAL.String(), auth.Authenticators_LDAP.String()},
			},
		},
	}

	return policydata
}

func createAuthenticationPolicy(t *testing.T, policy *auth.AuthenticationPolicy) *auth.AuthenticationPolicy {
	authGetter := GetAuthGetter("AuthGetterTest", apiSrvAddr, nil, 600)
	AssertEventually(t, func() (bool, interface{}) {
		_, err := authGetter.GetAuthenticationPolicy()
		return err != nil, nil
	}, "failed to create AuthenticationPolicy")

	ret, err := CreateAuthenticationPolicyWithOrder(apicl,
		policy.Spec.Authenticators.Local,
		policy.Spec.Authenticators.Ldap,
		policy.Spec.Authenticators.Radius,
		policy.Spec.Authenticators.AuthenticatorOrder)
	if err != nil {
		panic(fmt.Sprintf("CreateAuthenticationPolicyWithOrder failed with err %s", err))
	}

	AssertEventually(t, func() (bool, interface{}) {
		_, err := authGetter.GetAuthenticationPolicy()
		return err == nil, nil
	}, "Authentication policy not found after creation")
	return ret
}

func deleteAuthenticationPolicy(t *testing.T) {
	authGetter := GetAuthGetter("AuthGetterTest", apiSrvAddr, nil, 600)
	AssertEventually(t, func() (bool, interface{}) {
		_, err := authGetter.GetAuthenticationPolicy()
		return err == nil, nil
	}, "did not find AuthenticationPolicy to delete")

	DeleteAuthenticationPolicy(apicl)
	AssertEventually(t, func() (bool, interface{}) {
		_, err := authGetter.GetAuthenticationPolicy()
		return err != nil, nil
	}, "found AuthenticationPolicy after delete")
}

// TestAuthenticate tests successful authentication for various authentication policies with LDAP and Local Authenticator configured.
// This tests authentication for different order of the authenticators and if they are enabled or disabled.
func TestAuthenticate(t *testing.T) {
	for testtype, policy := range authenticationPoliciesData() {
		createAuthenticationPolicy(t, policy)

		// authenticate
		var autheduser *auth.User
		var ok bool
		var err error
		AssertEventually(t, func() (bool, interface{}) {
			autheduser, ok, err = authnmgr.Authenticate(&auth.PasswordCredential{Username: testUser, Tenant: "default", Password: testPassword})
			return ok, nil
		}, fmt.Sprintf("[%v] Unsuccessful local user authentication", testtype))

		Assert(t, autheduser.Name == testUser, fmt.Sprintf("[%v] User returned by authentication manager didn't match user being authenticated", testtype))
		Assert(t, autheduser.Spec.GetType() == auth.UserSpec_LOCAL.String(), fmt.Sprintf("[%v] User returned is not of type LOCAL", testtype))
		AssertOk(t, err, fmt.Sprintf("[%v] Error authenticating user", testtype))

		deleteAuthenticationPolicy(t)
	}
}

// TestIncorrectPasswordAuthentication tests failed authentication by all authenticators
func TestIncorrectPasswordAuthentication(t *testing.T) {
	for testtype, policy := range authenticationPoliciesData() {
		createAuthenticationPolicy(t, policy)

		// authenticate
		var autheduser *auth.User
		var ok bool
		var err error
		AssertConsistently(t, func() (bool, interface{}) {
			autheduser, ok, err = authnmgr.Authenticate(&auth.PasswordCredential{Username: testUser, Password: "wrongpassword"})
			return !ok, nil
		}, fmt.Sprintf("[%v] Successful local user authentication", testtype))

		Assert(t, autheduser == nil, fmt.Sprintf("[%v] User returned while authenticating with wrong password", testtype))
		Assert(t, err != nil, fmt.Sprintf("[%v] No error returned while authenticating with wrong password", testtype))

		deleteAuthenticationPolicy(t)
	}
}

// TestNotYetImplementedAuthenticator tests un-implemented authenticator
func TestNotYetImplementedAuthenticator(t *testing.T) {
	policy := &auth.AuthenticationPolicy{
		TypeMeta: api.TypeMeta{Kind: "AuthenticationPolicy"},
		ObjectMeta: api.ObjectMeta{
			Name: "AuthenticationPolicy",
		},
		Spec: auth.AuthenticationPolicySpec{
			Authenticators: auth.Authenticators{
				Ldap: &auth.Ldap{
					Enabled: false,
				},
				Local: &auth.Local{
					Enabled: true,
				},
				Radius:             &auth.Radius{},
				AuthenticatorOrder: []string{auth.Authenticators_LOCAL.String(), auth.Authenticators_LDAP.String(), auth.Authenticators_RADIUS.String()},
			},
		},
	}
	createAuthenticationPolicy(t, policy)
	defer deleteAuthenticationPolicy(t)

	var user *auth.User
	var ok bool
	var err error
	AssertConsistently(t, func() (bool, interface{}) {
		user, ok, err = authnmgr.Authenticate(&auth.PasswordCredential{Username: testUser, Tenant: "default", Password: testPassword})
		return !ok, nil
	}, "User authenticated when unimplemented authenticator is configured")

	Assert(t, err != nil, "No error returned for un-implemented authenticator")
	Assert(t, user == nil, "User returned when unimplemented authenticator is configured")
}

// disabledLocalAuthenticatorPolicyData returns policy data where both LDAP and Local authenticators are disabled
func disabledLocalAuthenticatorPolicyData() map[string](*auth.AuthenticationPolicy) {
	policydata := make(map[string]*auth.AuthenticationPolicy)

	policydata["LDAP explicitly disabled, Local explicitly disabled"] = &auth.AuthenticationPolicy{
		TypeMeta: api.TypeMeta{Kind: "AuthenticationPolicy"},
		ObjectMeta: api.ObjectMeta{
			Name: "AuthenticationPolicy",
		},
		Spec: auth.AuthenticationPolicySpec{
			Authenticators: auth.Authenticators{
				Ldap: &auth.Ldap{
					Enabled: false,
				},
				Local: &auth.Local{
					Enabled: false,
				},
				AuthenticatorOrder: []string{auth.Authenticators_LDAP.String(), auth.Authenticators_LOCAL.String()},
			},
		},
	}
	policydata["LDAP implicitly disabled, Local implicitly disabled"] = &auth.AuthenticationPolicy{
		TypeMeta: api.TypeMeta{Kind: "AuthenticationPolicy"},
		ObjectMeta: api.ObjectMeta{
			Name: "AuthenticationPolicy",
		},
		Spec: auth.AuthenticationPolicySpec{
			Authenticators: auth.Authenticators{
				Ldap:               &auth.Ldap{},
				Local:              &auth.Local{},
				AuthenticatorOrder: []string{auth.Authenticators_LDAP.String(), auth.Authenticators_LOCAL.String()},
			},
		},
	}

	return policydata
}

// TestAuthenticateWithDisabledAuthenticators test authentication for local user when all authenticators are disabled
func TestAuthenticateWithDisabledAuthenticators(t *testing.T) {
	for testtype, policy := range disabledLocalAuthenticatorPolicyData() {
		createAuthenticationPolicy(t, policy)

		// authenticate
		var autheduser *auth.User
		var ok bool
		var err error
		AssertConsistently(t, func() (bool, interface{}) {
			autheduser, ok, err = authnmgr.Authenticate(&auth.PasswordCredential{Username: testUser, Password: testPassword})
			return !ok, nil
		}, fmt.Sprintf("[%v] local user authentication should fail", testtype))

		Assert(t, autheduser == nil, fmt.Sprintf("[%v] User returned with disabled authenticators", testtype))
		AssertOk(t, err, fmt.Sprintf("[%v] No error should be returned with disabled authenticators", testtype))
		deleteAuthenticationPolicy(t)
	}
}

func TestAuthnMgrValidateToken(t *testing.T) {
	policy := &auth.AuthenticationPolicy{
		TypeMeta: api.TypeMeta{Kind: "AuthenticationPolicy"},
		ObjectMeta: api.ObjectMeta{
			Name: "AuthenticationPolicy",
		},
		Spec: auth.AuthenticationPolicySpec{
			Authenticators: auth.Authenticators{
				Ldap: &auth.Ldap{
					Enabled: true,
				},
				Local: &auth.Local{
					Enabled: true,
				},
				AuthenticatorOrder: []string{auth.Authenticators_LDAP.String(), auth.Authenticators_LOCAL.String()},
			},
		},
	}
	createdPolicy := createAuthenticationPolicy(t, policy)
	defer deleteAuthenticationPolicy(t)

	// create JWT token
	jwtTok := createHeadlessToken(signatureAlgorithm, createdPolicy.Spec.Secret, time.Duration(expiration)*time.Second, issuerClaimValue)

	var user *auth.User
	var ok bool
	var csrfTok string
	var err error
	AssertEventually(t, func() (bool, interface{}) {
		user, ok, csrfTok, err = authnmgr.ValidateToken(jwtTok)
		return ok, nil
	}, "token validation failed")
	AssertOk(t, err, "error validating token")
	Assert(t, csrfTok == testCsrfToken, "incorrect csrf token")
	Assert(t, user != nil && user.Name == testUser && user.Tenant == tenant, "incorrect user returned")
}

func TestValidateTokenErrors(t *testing.T) {
	mockAuthnmgr := NewMockAuthenticationManager()
	tests := []struct {
		name     string
		token    string
		expected error
	}{
		{
			name:     "invalid user claim",
			token:    invalidUserClaim,
			expected: ErrUserNotFound,
		},
		{
			name:     "invalid tenant claim",
			token:    invalidTenantClaim,
			expected: ErrUserNotFound,
		},
		{
			name:     "non existent user",
			token:    nonExistentUserClaim,
			expected: ErrUserNotFound,
		},
	}
	for _, test := range tests {
		user, ok, csrfTok, err := mockAuthnmgr.ValidateToken(test.token)
		Assert(t, err == test.expected, fmt.Sprintf("[%v]  test failed", test.name))
		Assert(t, !ok, fmt.Sprintf("[%v]  test failed", test.name))
		Assert(t, user == nil, fmt.Sprintf("[%v]  test failed", test.name))
		Assert(t, csrfTok == "", fmt.Sprintf("[%v]  test failed", test.name))
	}
}
