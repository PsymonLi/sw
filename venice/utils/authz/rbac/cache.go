package rbac

import (
	"sync"

	"github.com/pensando/sw/api/generated/auth"
	"github.com/pensando/sw/venice/utils/log"
)

// userPermissionsCache is a cache of roles and role bindings. It implements permissionCache interface.
type userPermissionsCache struct {
	// mutex to synchronize access to roles and role bindings cache
	sync.RWMutex
	// cache roles with tenant as the key
	roles map[string]map[string]*auth.Role
	// cache role bindings with tenant as the key
	roleBindings map[string]map[string]*auth.RoleBinding
}

func (c *userPermissionsCache) reset() {
	defer c.Unlock()
	c.Lock()
	// reset cache
	c.roles = make(map[string]map[string]*auth.Role)
	c.roleBindings = make(map[string]map[string]*auth.RoleBinding)
	log.Info("RBAC cache reset")
}

func (c *userPermissionsCache) getPermissions(user *auth.User) []auth.Permission {
	var permissions []auth.Permission
	roles := c.getRolesForUser(user)
	for _, role := range roles {
		permissions = append(permissions, role.Spec.GetPermissions()...)
	}
	return permissions
}

func (c *userPermissionsCache) getRolesForUser(user *auth.User) []auth.Role {
	var roles []auth.Role

	defer c.RUnlock()
	c.RLock()
	if user == nil {
		return nil
	}
	tenant := user.GetTenant()
	roleBindings := c.roleBindings[tenant]
	// check which role bindings apply to user and accumulate roles
	for _, roleBinding := range roleBindings {
		if has(roleBinding.Spec.GetUsers(), user.GetName()) ||
			hasAny(roleBinding.Spec.GetUserGroups(), user.Status.GetUserGroups()) {
			role, ok := c.roles[tenant][getKey(tenant, roleBinding.Spec.GetRole())]
			if !ok {
				log.Errorf("role [%s] specified in role binding [%v] doesn't exist in tenant [%s]", roleBinding.Spec.GetRole(), roleBinding, tenant)
				continue
			}
			// accumulate role
			roles = append(roles, *role)
		}
	}
	return roles
}

func (c *userPermissionsCache) getRoles(tenant string) []auth.Role {
	var roles []auth.Role

	defer c.RUnlock()
	c.RLock()
	if rolemap, ok := c.roles[tenant]; ok {
		for _, role := range rolemap {
			roles = append(roles, *role)
		}
	}
	return roles
}

func (c *userPermissionsCache) addRole(role *auth.Role) {
	defer c.Unlock()
	c.Lock()
	// create role cache for tenant if it doesn't exist
	_, ok := c.roles[role.GetTenant()]
	if !ok {
		c.roles[role.GetTenant()] = make(map[string]*auth.Role)
		log.Debugf("initialized role cache for tenant [%s] while adding role [%v]", role.GetTenant(), role)
	}
	c.roles[role.GetTenant()][getKey(role.GetTenant(), role.GetName())] = role
}

func (c *userPermissionsCache) deleteRole(role *auth.Role) {
	defer c.Unlock()
	c.Lock()
	delete(c.roles[role.GetTenant()], getKey(role.GetTenant(), role.GetName()))
}

func (c *userPermissionsCache) getRole(name, tenant string) (auth.Role, bool) {
	defer c.RUnlock()
	c.RLock()
	_, ok := c.roles[tenant]
	if !ok {
		log.Debugf("no role cache found for tenant [%s]", tenant)
		return auth.Role{}, false
	}
	role, ok := c.roles[tenant][getKey(tenant, name)]
	if !ok {
		return auth.Role{}, ok
	}
	return *role, ok
}

func (c *userPermissionsCache) getRoleBindings(tenant string) []auth.RoleBinding {
	var roleBindings []auth.RoleBinding

	defer c.RUnlock()
	c.RLock()
	if roleBindingMap, ok := c.roleBindings[tenant]; ok {
		for _, roleBinding := range roleBindingMap {
			roleBindings = append(roleBindings, *roleBinding)
		}
	}
	return roleBindings
}

func (c *userPermissionsCache) addRoleBinding(roleBinding *auth.RoleBinding) {
	defer c.Unlock()
	c.Lock()
	// create role binding cache for tenant if it doesn't exist
	_, ok := c.roleBindings[roleBinding.GetTenant()]
	if !ok {
		c.roleBindings[roleBinding.GetTenant()] = make(map[string]*auth.RoleBinding)
		log.Debugf("initialized role binding cache for tenant [%s] while adding role binding [%v]", roleBinding.GetTenant(), roleBinding)
	}
	c.roleBindings[roleBinding.GetTenant()][getKey(roleBinding.GetTenant(), roleBinding.GetName())] = roleBinding
}

func (c *userPermissionsCache) deleteRoleBinding(roleBinding *auth.RoleBinding) {
	defer c.Unlock()
	c.Lock()
	delete(c.roleBindings[roleBinding.GetTenant()], getKey(roleBinding.GetTenant(), roleBinding.GetName()))
}

func (c *userPermissionsCache) getRoleBinding(name, tenant string) (auth.RoleBinding, bool) {
	defer c.RUnlock()
	c.RLock()

	_, ok := c.roleBindings[tenant]
	if !ok {
		log.Debugf("no role binding cache found for tenant [%s]", tenant)
		return auth.RoleBinding{}, false
	}
	roleBinding, ok := c.roleBindings[tenant][getKey(tenant, name)]
	if !ok {
		return auth.RoleBinding{}, ok
	}
	return *roleBinding, ok
}

func (c *userPermissionsCache) initializeCacheForTenant(tenant string) {
	defer c.Unlock()
	c.Lock()
	// create role and role binding cache for tenant if they don't exist. It is possible to receive role/role binding create events before tenant create events
	_, ok := c.roles[tenant]
	if !ok {
		c.roles[tenant] = make(map[string]*auth.Role)
		log.Debugf("initialized role cache for tenant [%s]", tenant)
	}
	_, ok = c.roleBindings[tenant]
	if !ok {
		c.roleBindings[tenant] = make(map[string]*auth.RoleBinding)
		log.Debugf("initialized role binding cache for tenant [%s]", tenant)
	}
}

func (c *userPermissionsCache) deleteCacheForTenant(tenant string) {
	defer c.Unlock()
	c.Lock()
	// delete role and role binding cache for tenant
	delete(c.roles, tenant)
	delete(c.roleBindings, tenant)
}

// for testing
func (c *userPermissionsCache) getRoleCache() map[string]map[string]*auth.Role {
	return c.roles
}

// for testing
func (c *userPermissionsCache) getRoleBindingCache() map[string]map[string]*auth.RoleBinding {
	return c.roleBindings
}

// newUserPermissionsCache returns an instance of userPermissionsCache
func newUserPermissionsCache() *userPermissionsCache {
	return &userPermissionsCache{
		roles:        make(map[string]map[string]*auth.Role),
		roleBindings: make(map[string]map[string]*auth.RoleBinding),
	}
}

func getKey(tenant string, name string) string {
	return tenant + "/" + name
}

func has(set []string, ele string) bool {
	// do not match empty string
	if ele == "" {
		return false
	}
	for _, s := range set {
		if s == ele {
			return true
		}
	}
	return false
}

func hasAny(set, contains []string) bool {
	owning := make(map[string]struct{}, len(set))
	for _, ele := range set {
		// do not add empty string
		if ele != "" {
			owning[ele] = struct{}{}
		}
	}
	for _, ele := range contains {
		if _, ok := owning[ele]; ok {
			return true
		}
	}
	return false
}
