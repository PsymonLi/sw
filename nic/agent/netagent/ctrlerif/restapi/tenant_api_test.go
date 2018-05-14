// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package netproto is a auto generated package.
Input file: tenant.proto
*/
package restapi

import (
	"testing"

	api "github.com/pensando/sw/api"
	"github.com/pensando/sw/venice/ctrler/npm/rpcserver/netproto"
	"github.com/pensando/sw/venice/utils/netutils"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestTenantList(t *testing.T) {
	t.Parallel()
	var ok bool
	var tenantList []*netproto.Tenant

	err := netutils.HTTPGet("http://"+agentRestURL+"/api/tenants/", &tenantList)

	AssertOk(t, err, "Error getting tenants from the REST Server")
	for _, o := range tenantList {
		if o.Name == "preCreatedTenant" {
			ok = true
			break
		}
	}
	if !ok {
		t.Errorf("Could not find preCreatedTenant in Response: %v", tenantList)
	}

}

func TestTenantPost(t *testing.T) {
	t.Parallel()
	var resp Response
	var ok bool
	var tenantList []*netproto.Tenant

	postData := netproto.Tenant{
		TypeMeta: api.TypeMeta{Kind: "Tenant"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "testPostTenant",
			Name:      "testPostTenant",
			Namespace: "testPostTenant",
		},
	}

	err := netutils.HTTPPost("http://"+agentRestURL+"/api/tenants/", &postData, &resp)
	getErr := netutils.HTTPGet("http://"+agentRestURL+"/api/tenants/", &tenantList)

	AssertOk(t, err, "Error posting tenant to REST Server")
	AssertOk(t, getErr, "Error getting tenants from the REST Server")
	for _, o := range tenantList {
		if o.Name == "testPostTenant" {
			ok = true
			break
		}
	}
	if !ok {
		t.Errorf("Could not find testPostTenant in Response: %v", tenantList)
	}

}

func TestTenantDelete(t *testing.T) {
	t.Parallel()
	var resp Response
	var found bool
	var tenantList []*netproto.Tenant

	deleteData := netproto.Tenant{
		TypeMeta: api.TypeMeta{Kind: "Tenant"},
		ObjectMeta: api.ObjectMeta{
			Tenant: "testDeleteTenant",
			Name:   "testDeleteTenant",
		},
	}
	postErr := netutils.HTTPPost("http://"+agentRestURL+"/api/tenants/", &deleteData, &resp)
	err := netutils.HTTPDelete("http://"+agentRestURL+"/api/tenants/testDeleteTenant", &deleteData, &resp)
	getErr := netutils.HTTPGet("http://"+agentRestURL+"/api/tenants/", &tenantList)

	AssertOk(t, postErr, "Error posting tenant to REST Server")
	AssertOk(t, err, "Error deleting tenant from REST Server")
	AssertOk(t, getErr, "Error getting tenants from the REST Server")
	for _, o := range tenantList {
		if o.Name == "testDeleteTenant" {
			found = true
			break
		}
	}
	if found {
		t.Errorf("Found testDeleteTenant in Response after deleting: %v", tenantList)
	}

}

func TestTenantUpdate(t *testing.T) {
	t.Parallel()
	var resp Response
	var tenantList []*netproto.Tenant

	var actualTenantSpec netproto.TenantSpec
	updatedTenantSpec := netproto.TenantSpec{
		Meta: &api.ObjectMeta{
			Namespace: "testNamespace",
		},
	}
	putData := netproto.Tenant{
		TypeMeta: api.TypeMeta{Kind: "Tenant"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "preCreatedTenant",
			Namespace: "preCreatedTenant",
			Name:      "preCreatedTenant",
		},
		Spec: updatedTenantSpec,
	}
	err := netutils.HTTPPut("http://"+agentRestURL+"/api/tenants/preCreatedTenant", &putData, &resp)
	AssertOk(t, err, "Error updating tenant to REST Server")

	getErr := netutils.HTTPGet("http://"+agentRestURL+"/api/tenants/", &tenantList)
	AssertOk(t, getErr, "Error getting tenants from the REST Server")

	for _, o := range tenantList {
		if o.Name == "preCreatedTenant" {
			actualTenantSpec = o.Spec
			break
		}
	}
	AssertEquals(t, updatedTenantSpec, actualTenantSpec, "Could not validate updated spec.")

}

func TestTenantCreateErr(t *testing.T) {
	t.Parallel()
	var resp Response
	badPostData := netproto.Tenant{
		TypeMeta: api.TypeMeta{Kind: "Tenant"},
		ObjectMeta: api.ObjectMeta{
			Name: "",
		},
	}

	err := netutils.HTTPPost("http://"+agentRestURL+"/api/tenants/", &badPostData, &resp)

	Assert(t, err != nil, "Expected test to error out with 500. It passed instead")
}

func TestTenantDeleteErr(t *testing.T) {
	t.Parallel()
	var resp Response
	badDelData := netproto.Tenant{
		TypeMeta: api.TypeMeta{Kind: "Tenant"},
		ObjectMeta: api.ObjectMeta{Tenant: "default",
			Namespace: "default",
			Name:      "badObject"},
	}

	err := netutils.HTTPDelete("http://"+agentRestURL+"/api/tenants/badObject", &badDelData, &resp)

	Assert(t, err != nil, "Expected test to error out with 500. It passed instead")
}

func TestTenantUpdateErr(t *testing.T) {
	t.Parallel()
	var resp Response
	badDelData := netproto.Tenant{
		TypeMeta: api.TypeMeta{Kind: "Tenant"},
		ObjectMeta: api.ObjectMeta{Tenant: "default",
			Namespace: "default",
			Name:      "badObject"},
	}

	err := netutils.HTTPPut("http://"+agentRestURL+"/api/tenants/badObject", &badDelData, &resp)

	Assert(t, err != nil, "Expected test to error out with 500. It passed instead")
}
