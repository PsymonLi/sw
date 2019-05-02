package impl

import (
	"context"
	"errors"
	"fmt"
	"reflect"
	"testing"

	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/auth"
	apigwpkg "github.com/pensando/sw/venice/apigw/pkg"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/authz"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/venice/apigw/pkg/mocks"
	"github.com/pensando/sw/venice/utils/bootstrapper"
	"github.com/pensando/sw/venice/utils/log"

	. "github.com/pensando/sw/venice/utils/authz/testutils"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestClusterHooksRegistration(t *testing.T) {
	logConfig := log.GetDefaultConfig("TestAPIGwClusterHooks")
	l := log.GetNewLogger(logConfig)
	svc := mocks.NewFakeAPIGwService(l, false)
	r := &clusterHooks{}
	r.logger = l
	err := registerClusterHooks(svc, l)
	AssertOk(t, err, "apigw cluster hook registration failed")
	prof, err := svc.GetCrudServiceProfile("Tenant", "create")
	AssertOk(t, err, "error getting service profile for Tenant create")
	Assert(t, len(prof.PreAuthNHooks()) == 1, fmt.Sprintf("unexpected number of pre authn hooks [%d] for Tenant create profile", len(prof.PreAuthNHooks())))

	// test error
	svc = mocks.NewFakeAPIGwService(l, true)
	err = registerClusterHooks(svc, l)
	Assert(t, err != nil, "expected error in cluster hook registration")
}

func TestAuthBootstrapForCluster(t *testing.T) {
	tests := []struct {
		name         string
		in           interface{}
		bootstrapper bootstrapper.Bootstrapper
		skipAuth     bool
		err          error
	}{
		{
			name: "non default tenant",
			in: &cluster.Tenant{
				TypeMeta: api.TypeMeta{Kind: string(cluster.KindTenant)},
				ObjectMeta: api.ObjectMeta{
					Name: "testTenant",
				},
			},
			bootstrapper: bootstrapper.NewMockBootstrapper(false),
			skipAuth:     false,
			err:          nil,
		},
		{
			name:         "invalid input type",
			in:           struct{ name string }{"testing"},
			bootstrapper: bootstrapper.NewMockBootstrapper(false),
			skipAuth:     false,
			err:          errors.New("invalid input type"),
		},
		{
			name:         "set bootstrap flag",
			in:           &cluster.ClusterAuthBootstrapRequest{},
			bootstrapper: bootstrapper.NewMockBootstrapper(false),
			skipAuth:     false,
			err:          nil,
		},
	}
	logConfig := log.GetDefaultConfig("TestAPIGwAuthHooks")
	l := log.GetNewLogger(logConfig)
	r := &clusterHooks{}
	r.logger = l
	for _, test := range tests {
		r.bootstrapper = test.bootstrapper
		_, _, skipAuth, err := r.authBootstrap(context.TODO(), test.in)
		Assert(t, skipAuth == test.skipAuth, fmt.Sprintf("[%s] test failed, expected skipAuth [%v], got [%v]", test.name, test.skipAuth, skipAuth))
		Assert(t, reflect.DeepEqual(err, test.err), fmt.Sprintf("[%s] test failed, expected err [%v], got [%v]", test.name, test.err, err))
	}
}

func TestSetAuthBootstrapFlag(t *testing.T) {
	tests := []struct {
		name         string
		bootstrapper bootstrapper.Bootstrapper
		skipCall     bool
		err          error
	}{
		{
			name:         "error in bootstrapper",
			bootstrapper: bootstrapper.NewMockBootstrapper(true),
			skipCall:     true,
			err:          bootstrapper.ErrFeatureNotFound,
		},
		{
			name:         "feature is bootstrapped",
			bootstrapper: bootstrapper.NewMockBootstrapper(false),
			skipCall:     false,
			err:          nil,
		},
	}
	logConfig := log.GetDefaultConfig("TestAPIGwAuthHooks")
	l := log.GetNewLogger(logConfig)
	r := &clusterHooks{}
	r.logger = l
	for _, test := range tests {
		r.bootstrapper = test.bootstrapper
		_, _, skipCall, err := r.setAuthBootstrapFlag(context.TODO(), nil)
		Assert(t, skipCall == test.skipCall, fmt.Sprintf("[%s] test failed, expected skipCall [%v], got [%v]", test.name, test.skipCall, skipCall))
		Assert(t, reflect.DeepEqual(err, test.err), fmt.Sprintf("[%s] test failed, expected err [%v], got [%v]", test.name, test.err, err))
	}
}

func TestAddOwnerForVersionGetAndWatch(t *testing.T) {
	user := &auth.User{
		ObjectMeta: api.ObjectMeta{Name: "test", Tenant: globals.DefaultTenant},
	}
	tests := []struct {
		name               string
		in                 interface{}
		operations         []authz.Operation
		expectedOperations []authz.Operation
		out                interface{}
		err                bool
	}{
		{
			name: "should grant user access",
			in:   cluster.Version{},
			operations: []authz.Operation{
				authz.NewOperation(authz.NewResource(globals.DefaultTenant,
					string(apiclient.GroupAuth), string(auth.KindUser),
					"", "test"),
					auth.Permission_Create.String()),
			},
			expectedOperations: []authz.Operation{
				authz.NewOperation(authz.NewResourceWithOwner(globals.DefaultTenant,
					string(apiclient.GroupAuth), string(auth.KindUser),
					"", "test",
					user),
					auth.Permission_Create.String()),
			},
			out: cluster.Version{},
			err: false,
		},
	}
	logConfig := log.GetDefaultConfig("TestAPIGwAuthHooks")
	l := log.GetNewLogger(logConfig)
	r := &clusterHooks{}
	r.logger = l
	for _, test := range tests {
		ctx := apigwpkg.NewContextWithOperations(context.TODO(), test.operations...)
		ctx = apigwpkg.NewContextWithUser(ctx, user)
		nctx, out, err := r.addOwner(ctx, test.in)
		Assert(t, test.err == (err != nil), fmt.Sprintf("got error [%v], [%s] test failed", err, test.name))
		operations, _ := apigwpkg.OperationsFromContext(nctx)
		Assert(t, AreOperationsEqual(test.expectedOperations, operations),
			fmt.Sprintf("unexpected operations, [%s] test failed", test.name))
		Assert(t, reflect.DeepEqual(test.out, out),
			fmt.Sprintf("expected returned object [%v], got [%v], [%s] test failed", test.out, out, test.name))
	}
}
