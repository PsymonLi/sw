// (c) Pensando Systems, Inc.
// This is a generated file, please do not hand edit !!

package pregen

import (
	api2 "github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/security"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/api/labels"
	"github.com/pensando/sw/venice/cli/api"
)

// GetObjSpec is
func GetObjSpec(objName string) interface{} {
	switch objName {

	case "SGPolicy":
		return security.SGPolicySpec{}

	case "cluster":
		return cluster.ClusterSpec{}

	case "endpoint":
		return workload.EndpointSpec{}

	case "lbPolicy":
		return network.LbPolicySpec{}

	case "network":
		return network.NetworkSpec{}

	case "node":
		return cluster.NodeSpec{}

	case "permission":
		return api.PermissionSpec{}

	case "role":
		return api.RoleSpec{}

	case "securityGroup":
		return security.SecurityGroupSpec{}

	case "service":
		return network.ServiceSpec{}

	case "smartNIC":
		return cluster.SmartNICSpec{}

	case "tenant":
		return cluster.TenantSpec{}

	case "user":
		return api.UserSpec{}

	}
	return nil
}

// GetSubObj is
func GetSubObj(kind string) interface{} {
	switch kind {

	case "SGRule":
		var v security.SGRule
		return &v

	case "Timestamp":
		var v api2.Timestamp
		return &v

	case "HealthCheckSpec":
		var v network.HealthCheckSpec
		return &v

	case "NodeCondition":
		var v cluster.NodeCondition
		return &v

	case "PortCondition":
		var v cluster.PortCondition
		return &v

	case "ConditionStatus":
		var v cluster.ConditionStatus
		return &v

	case "OsInfo":
		var v cluster.OsInfo
		return &v

	case "DockerInfo":
		var v cluster.DockerInfo
		return &v

	case "CPUInfo":
		var v cluster.CPUInfo
		return &v

	case "MemInfo":
		var v cluster.MemInfo
		return &v

	case "StorageInfo":
		var v cluster.StorageInfo
		return &v

	case "StorageDeviceInfo":
		var v cluster.StorageDeviceInfo
		return &v

	case "NetworkInfo":
		var v cluster.NetworkInfo
		return &v

	case "Selector":
		var v labels.Selector
		return &v

	case "Requirement":
		var v labels.Requirement
		return &v

	case "TLSServerPolicySpec":
		var v network.TLSServerPolicySpec
		return &v

	case "TLSClientPolicySpec":
		var v network.TLSClientPolicySpec
		return &v

	case "SmartNICCondition":
		var v cluster.SmartNICCondition
		return &v

	case "SmartNICInfo":
		var v cluster.SmartNICInfo
		return &v

	case "UplinkStatus":
		var v cluster.UplinkStatus
		return &v

	case "PFStatus":
		var v cluster.PFStatus
		return &v

	case "IPConfig":
		var v cluster.IPConfig
		return &v

	case "BiosInfo":
		var v cluster.BiosInfo
		return &v

	case "UserAuditLog":
		var v api.UserAuditLog
		return &v

	}
	return nil
}

// GetObjStatus is
func GetObjStatus(objName string) interface{} {
	switch objName {

	case "SGPolicy":
		o := security.SGPolicy{}
		return o.Status

	case "cluster":
		o := cluster.Cluster{}
		return o.Status

	case "endpoint":
		o := workload.Endpoint{}
		return o.Status

	case "lbPolicy":
		o := network.LbPolicy{}
		return o.Status

	case "network":
		o := network.Network{}
		return o.Status

	case "node":
		o := cluster.Node{}
		return o.Status

	case "permission":
		o := api.Permission{}
		return o.Status

	case "role":
		o := api.Role{}
		return o.Status

	case "securityGroup":
		o := security.SecurityGroup{}
		return o.Status

	case "service":
		o := network.Service{}
		return o.Status

	case "smartNIC":
		o := cluster.SmartNIC{}
		return o.Status

	case "tenant":
		o := cluster.Tenant{}
		return o.Status

	case "user":
		o := api.User{}
		return o.Status

	}
	return nil
}
