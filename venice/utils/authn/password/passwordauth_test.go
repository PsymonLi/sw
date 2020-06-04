package password_test

import (
	"context"
	"fmt"
	"os"
	"testing"
	"time"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/auth"
	"github.com/pensando/sw/venice/apiserver"
	. "github.com/pensando/sw/venice/utils/authn/password"
	. "github.com/pensando/sw/venice/utils/authn/testutils"
	"github.com/pensando/sw/venice/utils/events/recorder"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
	"github.com/pensando/sw/venice/utils/log"
	. "github.com/pensando/sw/venice/utils/testutils"
	"github.com/pensando/sw/venice/utils/testutils/serviceutils"

	_ "github.com/pensando/sw/api/generated/exports/apiserver"
	_ "github.com/pensando/sw/api/hooks/apiserver"
)

const (
	apisrvURL    = "localhost:0"
	testUser     = "test"
	testPassword = "Pensandoo0$"
)

// test user
var (
	user             *auth.User
	policy           *auth.AuthenticationPolicy
	apicl            apiclient.Services
	apiSrv           apiserver.Server
	apiSrvAddr       string
	testPasswordHash string // password hash for testPassword

	logger = log.WithContext("Pkg", "password_test")

	// create mock events recorder
	_ = recorder.Override(mockevtsrecorder.NewRecorder("passwordauth_test", logger))
)

func TestMain(m *testing.M) {
	setup()
	code := m.Run()
	shutdown()
	os.Exit(code)
}

func setup() {
	var err error
	// api server
	apiSrv, apiSrvAddr, err = serviceutils.StartAPIServer(apisrvURL, "password_test", logger, []string{})
	if err != nil {
		panic("Unable to start API Server")
	}

	// api server client
	apicl, err = apiclient.NewGrpcAPIClient("password_test", apiSrvAddr, logger)
	if err != nil {
		panic("Error creating api client")
	}

	// for tests in hasher_test.go
	cachePasswordHash()
}

func shutdown() {
	if apicl != nil {
		apicl.Close()
	}
	//stop api server
	apiSrv.Stop()
}

func cachePasswordHash() {
	// get password hasher
	hasher := GetPasswordHasher()
	// generate hash
	passwdhash, err := hasher.GetPasswordHash(testPassword)
	if err != nil {
		panic("Error creating password hash")
	}
	//cache password for tests in passwordhasher_test.go
	testPasswordHash = passwdhash
}

func setupAuth() {
	// Create Tenant
	MustCreateTenant(apicl, "default")
	//create test user
	user = MustCreateTestUser(apicl, testUser, testPassword, "default")
	// create auth policy
	policy = MustCreateAuthenticationPolicy(apicl, &auth.Local{}, nil, nil)
}

func cleanupAuth() {
	MustDeleteAuthenticationPolicy(apicl)
	MustDeleteUser(apicl, testUser, "default")
	MustDeleteTenant(apicl, "default")
}

func TestAuthenticate(t *testing.T) {
	setupAuth()
	defer cleanupAuth()

	ch := make(chan error)
	// create password authenticator
	authenticator := NewPasswordAuthenticator(apicl, ch, 30*time.Second, policy.Spec.Authenticators.GetLocal(), logger)

	// authenticate
	autheduser, ok, err := authenticator.Authenticate(&auth.PasswordCredential{Username: testUser, Tenant: "default", Password: testPassword})

	Assert(t, ok, "Unsuccessful local user authentication")
	Assert(t, (autheduser.Name == user.Name && autheduser.Tenant == user.Tenant), "User returned by password auth module didn't match user being authenticated")
	AssertOk(t, err, "Error authenticating user")
}

func TestIncorrectPasswordAuthentication(t *testing.T) {
	setupAuth()
	defer cleanupAuth()

	ch := make(chan error)
	// create password authenticator
	authenticator := NewPasswordAuthenticator(apicl, ch, 30*time.Second, policy.Spec.Authenticators.GetLocal(), logger)

	// authenticate
	autheduser, ok, err := authenticator.Authenticate(&auth.PasswordCredential{Username: testUser, Tenant: "default", Password: "wrongpassword"})

	Assert(t, !ok, "Successful local user authentication")
	Assert(t, (autheduser == nil), "User returned while authenticating with wrong password")
	Assert(t, err != nil, "No error returned for incorrect password")
}

func TestIncorrectUserAuthentication(t *testing.T) {
	setupAuth()
	defer cleanupAuth()

	ch := make(chan error)
	// create password authenticator
	authenticator := NewPasswordAuthenticator(apicl, ch, 30*time.Second, policy.Spec.Authenticators.GetLocal(), logger)

	// authenticate
	autheduser, ok, err := authenticator.Authenticate(&auth.PasswordCredential{Username: "test1", Tenant: "default", Password: "password"})

	Assert(t, !ok, "Successful local user authentication")
	Assert(t, autheduser == nil, "User returned while authenticating with incorrect username")
	Assert(t, err != nil, "No error returned for incorrect username")
	Assert(t, err == ErrInvalidCredential, "Incorrect error type returned")
}

func TestAPIServerDown(t *testing.T) {
	setupAuth()
	defer cleanupAuth()
	ch := make(chan error)
	// create password authenticator
	authenticator := NewPasswordAuthenticator(nil, ch, 30*time.Second, policy.Spec.Authenticators.GetLocal(), logger)

	// authenticate
	autheduser, ok, err := authenticator.Authenticate(&auth.PasswordCredential{Username: testUser, Tenant: "default", Password: testPassword})

	Assert(t, !ok, "Successful local user authentication")
	Assert(t, autheduser == nil, "User returned while authenticating with un-initialized API client")
	Assert(t, err == ErrAPIServerClientNotInitialized, "Incorrect error type returned")

	// test context timeout
	ch = make(chan error)
	authenticator = NewPasswordAuthenticator(apicl, ch, time.Nanosecond, policy.Spec.Authenticators.GetLocal(), logger)

	go readInitClientChanel(t, ch, status.Error(codes.DeadlineExceeded, context.DeadlineExceeded.Error()), 5*time.Second)

	// authenticate
	autheduser, ok, err = authenticator.Authenticate(&auth.PasswordCredential{Username: testUser, Tenant: "default", Password: testPassword})

	Assert(t, !ok, "Successful local user authentication")
	Assert(t, autheduser == nil, "User returned when expecting context to timeout")
	Assert(t, err == ErrAPIServerUnavailable, "Incorrect error type returned")

	// stop API server
	apiSrv.Stop()
	defer setup()

	ch = make(chan error)
	// create password authenticator
	authenticator = NewPasswordAuthenticator(apicl, ch, 30*time.Second, policy.Spec.Authenticators.GetLocal(), logger)

	go readInitClientChanel(t, ch, status.Error(codes.Unavailable, "grpc: the connection is unavailable"), 45*time.Second)
	// authenticate
	autheduser, ok, err = authenticator.Authenticate(&auth.PasswordCredential{Username: testUser, Tenant: "default", Password: testPassword})

	Assert(t, !ok, "Successful local user authentication")
	Assert(t, autheduser == nil, "User returned while authenticating when API server is down")
	Assert(t, err == ErrAPIServerUnavailable, "Incorrect error type returned")
}

func readInitClientChanel(t *testing.T, ch <-chan error, expectedErr error, timeout time.Duration) {
	for {
		select {
		case <-time.After(timeout):
			if expectedErr != nil {
				Assert(t, false, fmt.Sprintf("expected error: %v, got none", expectedErr))
			}
		case err := <-ch:
			Assert(t, err.Error() == expectedErr.Error(), fmt.Sprintf("expected error: %v, got: %v", expectedErr, err))
		}
		return
	}
}
