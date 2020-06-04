package password

import (
	"context"
	"errors"
	"fmt"
	"time"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/auth"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/authn"
	"github.com/pensando/sw/venice/utils/log"
)

var (
	// ErrInvalidCredential error is returned when user doesn't exist
	ErrInvalidCredential = errors.New("incorrect credential")
	// ErrAPIServerClientNotInitialized error is returned when API server client hasn't been initialized
	ErrAPIServerClientNotInitialized = errors.New("API server client not initialized")
	// ErrAPIServerUnavailable error is returned when API server is not reachable
	ErrAPIServerUnavailable = errors.New("API server not available")
)

// authenticator is used for authenticating local user. It implements authn.Authenticator interface.
type authenticator struct {
	authConfig   *auth.Local
	logger       log.Logger
	client       apiclient.Services
	initClientCh chan<- error
	apiTimeout   time.Duration
}

// NewPasswordAuthenticator returns an instance of Authenticator
func NewPasswordAuthenticator(client apiclient.Services, initClientCh chan<- error, apiTimeout time.Duration, config *auth.Local, logger log.Logger) authn.Authenticator {
	return &authenticator{
		authConfig:   config,
		logger:       logger,
		client:       client,
		initClientCh: initClientCh,
		apiTimeout:   apiTimeout,
	}
}

// Authenticate authenticates local user
// Returns
//   *auth.User upon successful authentication
//   bool true upon successful authentication
//   error authn.ErrInvalidCredentialType if invalid credential type is passed, ErrInvalidCredential if user doesn't exist,
//         errors returned by hash comparison function
func (a *authenticator) Authenticate(credential authn.Credential) (*auth.User, bool, error) {
	passwdcred, found := credential.(*auth.PasswordCredential)
	if !found {
		a.logger.ErrorLog("method", "Authenticate", "msg", fmt.Sprintf("Incorrect credential type: expected '*authn.PasswordCredential', got [%T]", credential))
		return nil, false, authn.ErrInvalidCredentialType
	}
	apicl := a.client
	if apicl == nil {
		a.logger.ErrorLog("method", "Authenticate", "msg", "API server client not initialized")
		return nil, false, ErrAPIServerClientNotInitialized
	}
	// fetch user
	objMeta := &api.ObjectMeta{
		Name:      passwdcred.Username,
		Tenant:    passwdcred.Tenant,
		Namespace: globals.DefaultNamespace,
	}
	ctx, cancel := context.WithTimeout(context.Background(), a.apiTimeout)
	defer cancel()
	user, err := apicl.AuthV1().User().Get(ctx, objMeta)
	if err != nil {
		a.logger.ErrorLog("method", "Authenticate", "msg", fmt.Sprintf("Error fetching user [%s]", passwdcred.Username), "error", err)
		grpcStatus, ok := status.FromError(err)
		if ok && (grpcStatus.Code() == codes.Unavailable || grpcStatus.Code() == codes.DeadlineExceeded) {
			a.initClientCh <- err
			return nil, false, ErrAPIServerUnavailable
		}
		return nil, false, ErrInvalidCredential
	}
	if user.Spec.Type == auth.UserSpec_External.String() {
		a.logger.InfoLog("method", "Authenticate", "msg", fmt.Sprintf("skipping local auth for external user [%s|%s]", passwdcred.Tenant, passwdcred.Username))
		return nil, false, ErrInvalidCredential
	}
	hasher := GetPasswordHasher()
	ok, err := hasher.CompareHashAndPassword(user.GetSpec().Password, passwdcred.Password)
	if err != nil {
		return nil, false, err
	}

	user.Status.Authenticators = []string{auth.Authenticators_LOCAL.String()}
	a.logger.DebugLog("method", "Authenticate", "msg", fmt.Sprintf("Successfully authenticated user [%s]", passwdcred.Username))

	return user, ok, nil
}
