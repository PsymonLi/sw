// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

package auth

import (
	"context"

	api "github.com/pensando/sw/api"
	"github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/venice/utils/kvstore"
)

// Dummy vars to suppress unused imports message
var _ context.Context
var _ api.ObjectMeta
var _ kvstore.Interface

// AuthV1UserInterface exposes the CRUD methods for User
type AuthV1UserInterface interface {
	Create(ctx context.Context, in *User) (*User, error)
	Update(ctx context.Context, in *User) (*User, error)
	UpdateStatus(ctx context.Context, in *User) (*User, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*User, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*User, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*User, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
	PasswordChange(ctx context.Context, in *PasswordChangeRequest) (*User, error)
	PasswordReset(ctx context.Context, in *PasswordResetRequest) (*User, error)
	IsAuthorized(ctx context.Context, in *SubjectAccessReviewRequest) (*User, error)
}

// AuthV1AuthenticationPolicyInterface exposes the CRUD methods for AuthenticationPolicy
type AuthV1AuthenticationPolicyInterface interface {
	Create(ctx context.Context, in *AuthenticationPolicy) (*AuthenticationPolicy, error)
	Update(ctx context.Context, in *AuthenticationPolicy) (*AuthenticationPolicy, error)
	UpdateStatus(ctx context.Context, in *AuthenticationPolicy) (*AuthenticationPolicy, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*AuthenticationPolicy, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*AuthenticationPolicy, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*AuthenticationPolicy, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
	LdapConnectionCheck(ctx context.Context, in *AuthenticationPolicy) (*AuthenticationPolicy, error)
	LdapBindCheck(ctx context.Context, in *AuthenticationPolicy) (*AuthenticationPolicy, error)
}

// AuthV1RoleInterface exposes the CRUD methods for Role
type AuthV1RoleInterface interface {
	Create(ctx context.Context, in *Role) (*Role, error)
	Update(ctx context.Context, in *Role) (*Role, error)
	UpdateStatus(ctx context.Context, in *Role) (*Role, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*Role, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*Role, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*Role, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// AuthV1RoleBindingInterface exposes the CRUD methods for RoleBinding
type AuthV1RoleBindingInterface interface {
	Create(ctx context.Context, in *RoleBinding) (*RoleBinding, error)
	Update(ctx context.Context, in *RoleBinding) (*RoleBinding, error)
	UpdateStatus(ctx context.Context, in *RoleBinding) (*RoleBinding, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*RoleBinding, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*RoleBinding, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*RoleBinding, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// AuthV1Interface exposes objects with CRUD operations allowed by the service
type AuthV1Interface interface {
	User() AuthV1UserInterface
	AuthenticationPolicy() AuthV1AuthenticationPolicyInterface
	Role() AuthV1RoleInterface
	RoleBinding() AuthV1RoleBindingInterface
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
}
