// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package clusterApiServer is a auto generated package.
Input file: smartnic.proto
*/
package cluster

import (
	"reflect"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/venice/utils/runtime"
)

var typesMapSmartnic = map[string]*api.Struct{

	"cluster.BiosInfo": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(BiosInfo{}) },
		Fields: map[string]api.Field{
			"Vendor": api.Field{Name: "Vendor", CLITag: api.CLIInfo{ID: "vendor", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "vendor", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Version": api.Field{Name: "Version", CLITag: api.CLIInfo{ID: "version", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "version", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"FwMajorVersion": api.Field{Name: "FwMajorVersion", CLITag: api.CLIInfo{ID: "fw-major-ver", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "fw-major-ver", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"FwMinorVersion": api.Field{Name: "FwMinorVersion", CLITag: api.CLIInfo{ID: "fw-minor-ver", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "fw-minor-ver", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"cluster.DSCCondition": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(DSCCondition{}) },
		Fields: map[string]api.Field{
			"Type": api.Field{Name: "Type", CLITag: api.CLIInfo{ID: "type", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "type", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Status": api.Field{Name: "Status", CLITag: api.CLIInfo{ID: "status", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "status", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"LastTransitionTime": api.Field{Name: "LastTransitionTime", CLITag: api.CLIInfo{ID: "last-transition-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "last-transition-time", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Reason": api.Field{Name: "Reason", CLITag: api.CLIInfo{ID: "reason", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "reason", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Message": api.Field{Name: "Message", CLITag: api.CLIInfo{ID: "message", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "message", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"cluster.DSCControlPlaneStatus": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(DSCControlPlaneStatus{}) },
		Fields: map[string]api.Field{
			"BGPStatus": api.Field{Name: "BGPStatus", CLITag: api.CLIInfo{ID: "bgp-status", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "bgp-status", Pointer: true, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "cluster.PeerStatus"},
		},
	},
	"cluster.DSCInfo": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(DSCInfo{}) },
		Fields: map[string]api.Field{
			"BiosInfo": api.Field{Name: "BiosInfo", CLITag: api.CLIInfo{ID: "bios-info", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "bios-info", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "cluster.BiosInfo"},

			"OsInfo": api.Field{Name: "OsInfo", CLITag: api.CLIInfo{ID: "os-info", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "os-info", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "cluster.OsInfo"},

			"CpuInfo": api.Field{Name: "CpuInfo", CLITag: api.CLIInfo{ID: "cpu-info", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "cpu-info", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "cluster.CPUInfo"},

			"MemoryInfo": api.Field{Name: "MemoryInfo", CLITag: api.CLIInfo{ID: "memory-info", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "memory-info", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "cluster.MemInfo"},

			"StorageInfo": api.Field{Name: "StorageInfo", CLITag: api.CLIInfo{ID: "storage-info", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "storage-info", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "cluster.StorageInfo"},
		},
	},
	"cluster.DistributedServiceCard": &api.Struct{
		Kind: "DistributedServiceCard", APIGroup: "cluster", Scopes: []string{"Cluster"}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(DistributedServiceCard{}) },
		Fields: map[string]api.Field{
			"TypeMeta": api.Field{Name: "TypeMeta", CLITag: api.CLIInfo{ID: "T", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: true, FromInline: false, KeyType: "", Type: "api.TypeMeta"},

			"ObjectMeta": api.Field{Name: "ObjectMeta", CLITag: api.CLIInfo{ID: "meta", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "meta", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.ObjectMeta"},

			"Spec": api.Field{Name: "Spec", CLITag: api.CLIInfo{ID: "spec", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "spec", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "cluster.DistributedServiceCardSpec"},

			"Status": api.Field{Name: "Status", CLITag: api.CLIInfo{ID: "status", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "status", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "cluster.DistributedServiceCardStatus"},

			"Kind": api.Field{Name: "Kind", CLITag: api.CLIInfo{ID: "kind", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "kind", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"APIVersion": api.Field{Name: "APIVersion", CLITag: api.CLIInfo{ID: "api-version", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "api-version", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Name": api.Field{Name: "Name", CLITag: api.CLIInfo{ID: "name", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "name", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Tenant": api.Field{Name: "Tenant", CLITag: api.CLIInfo{ID: "tenant", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "tenant", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Namespace": api.Field{Name: "Namespace", CLITag: api.CLIInfo{ID: "namespace", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "namespace", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"GenerationID": api.Field{Name: "GenerationID", CLITag: api.CLIInfo{ID: "generation-id", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "generation-id", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"ResourceVersion": api.Field{Name: "ResourceVersion", CLITag: api.CLIInfo{ID: "resource-version", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "resource-version", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"UUID": api.Field{Name: "UUID", CLITag: api.CLIInfo{ID: "uuid", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "uuid", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Labels": api.Field{Name: "Labels", CLITag: api.CLIInfo{ID: "labels", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "labels", Pointer: true, Slice: false, Mutable: true, Map: true, Inline: false, FromInline: true, KeyType: "TYPE_STRING", Type: "TYPE_STRING"},

			"CreationTime": api.Field{Name: "CreationTime", CLITag: api.CLIInfo{ID: "creation-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "creation-time", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "api.Timestamp"},

			"ModTime": api.Field{Name: "ModTime", CLITag: api.CLIInfo{ID: "mod-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "mod-time", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "api.Timestamp"},

			"SelfLink": api.Field{Name: "SelfLink", CLITag: api.CLIInfo{ID: "self-link", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "self-link", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},
		},

		CLITags: map[string]api.CLIInfo{
			"DSCSku":                 api.CLIInfo{Path: "Status.DSCSku", Skip: false, Insert: "", Help: ""},
			"DSCVersion":             api.CLIInfo{Path: "Status.DSCVersion", Skip: false, Insert: "", Help: ""},
			"address-families":       api.CLIInfo{Path: "Status.ControlPlaneStatus.BGPStatus[].AddressFamilies", Skip: false, Insert: "", Help: ""},
			"adm-phase-reason":       api.CLIInfo{Path: "Status.AdmissionPhaseReason", Skip: false, Insert: "", Help: ""},
			"admission-phase":        api.CLIInfo{Path: "Status.AdmissionPhase", Skip: false, Insert: "", Help: ""},
			"admit":                  api.CLIInfo{Path: "Spec.Admit", Skip: false, Insert: "", Help: ""},
			"api-version":            api.CLIInfo{Path: "APIVersion", Skip: false, Insert: "", Help: ""},
			"controllers":            api.CLIInfo{Path: "Spec.Controllers", Skip: false, Insert: "", Help: ""},
			"default-gw":             api.CLIInfo{Path: "Spec.IPConfig.DefaultGW", Skip: false, Insert: "", Help: ""},
			"dns-servers":            api.CLIInfo{Path: "Spec.IPConfig.DNSServers", Skip: false, Insert: "", Help: ""},
			"dscprofile":             api.CLIInfo{Path: "Spec.DSCProfile", Skip: false, Insert: "", Help: ""},
			"fw-major-ver":           api.CLIInfo{Path: "Status.SystemInfo.BiosInfo.FwMajorVersion", Skip: false, Insert: "", Help: ""},
			"fw-minor-ver":           api.CLIInfo{Path: "Status.SystemInfo.BiosInfo.FwMinorVersion", Skip: false, Insert: "", Help: ""},
			"generation-id":          api.CLIInfo{Path: "GenerationID", Skip: false, Insert: "", Help: ""},
			"host":                   api.CLIInfo{Path: "Status.Host", Skip: false, Insert: "", Help: ""},
			"id":                     api.CLIInfo{Path: "Spec.ID", Skip: false, Insert: "", Help: ""},
			"interfaces":             api.CLIInfo{Path: "Status.Interfaces", Skip: false, Insert: "", Help: ""},
			"ip-address":             api.CLIInfo{Path: "Spec.IPConfig.IPAddress", Skip: false, Insert: "", Help: ""},
			"is-connected-to-venice": api.CLIInfo{Path: "Status.IsConnectedToVenice", Skip: false, Insert: "", Help: ""},
			"kind":                   api.CLIInfo{Path: "Kind", Skip: false, Insert: "", Help: ""},
			"labels":                 api.CLIInfo{Path: "Labels", Skip: false, Insert: "", Help: ""},
			"last-transition-time":   api.CLIInfo{Path: "Status.Conditions[].LastTransitionTime", Skip: false, Insert: "", Help: ""},
			"message":                api.CLIInfo{Path: "Status.Conditions[].Message", Skip: false, Insert: "", Help: ""},
			"mgmt-mode":              api.CLIInfo{Path: "Spec.MgmtMode", Skip: false, Insert: "", Help: ""},
			"mgmt-vlan":              api.CLIInfo{Path: "Spec.MgmtVlan", Skip: false, Insert: "", Help: ""},
			"name":                   api.CLIInfo{Path: "Name", Skip: false, Insert: "", Help: ""},
			"namespace":              api.CLIInfo{Path: "Namespace", Skip: false, Insert: "", Help: ""},
			"network-mode":           api.CLIInfo{Path: "Spec.NetworkMode", Skip: false, Insert: "", Help: ""},
			"peer-address":           api.CLIInfo{Path: "Status.ControlPlaneStatus.BGPStatus[].PeerAddress", Skip: false, Insert: "", Help: ""},
			"primary-mac":            api.CLIInfo{Path: "Status.PrimaryMAC", Skip: false, Insert: "", Help: ""},
			"reason":                 api.CLIInfo{Path: "Status.Conditions[].Reason", Skip: false, Insert: "", Help: ""},
			"remote-asn":             api.CLIInfo{Path: "Status.ControlPlaneStatus.BGPStatus[].RemoteASN", Skip: false, Insert: "", Help: ""},
			"resource-version":       api.CLIInfo{Path: "ResourceVersion", Skip: false, Insert: "", Help: ""},
			"routing-config":         api.CLIInfo{Path: "Spec.RoutingConfig", Skip: false, Insert: "", Help: ""},
			"self-link":              api.CLIInfo{Path: "SelfLink", Skip: false, Insert: "", Help: ""},
			"serial-num":             api.CLIInfo{Path: "Status.SerialNum", Skip: false, Insert: "", Help: ""},
			"state":                  api.CLIInfo{Path: "Status.ControlPlaneStatus.BGPStatus[].State", Skip: false, Insert: "", Help: ""},
			"status":                 api.CLIInfo{Path: "Status.Conditions[].Status", Skip: false, Insert: "", Help: ""},
			"tenant":                 api.CLIInfo{Path: "Tenant", Skip: false, Insert: "", Help: ""},
			"type":                   api.CLIInfo{Path: "Status.Conditions[].Type", Skip: false, Insert: "", Help: ""},
			"unhealthy-services":     api.CLIInfo{Path: "Status.UnhealthyServices", Skip: false, Insert: "", Help: ""},
			"uuid":                   api.CLIInfo{Path: "UUID", Skip: false, Insert: "", Help: ""},
			"vendor":                 api.CLIInfo{Path: "Status.SystemInfo.BiosInfo.Vendor", Skip: false, Insert: "", Help: ""},
			"version":                api.CLIInfo{Path: "Status.SystemInfo.BiosInfo.Version", Skip: false, Insert: "", Help: ""},
			"version-mismatch":       api.CLIInfo{Path: "Status.VersionMismatch", Skip: false, Insert: "", Help: ""},
		},
	},
	"cluster.DistributedServiceCardSpec": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(DistributedServiceCardSpec{}) },
		Fields: map[string]api.Field{
			"Admit": api.Field{Name: "Admit", CLITag: api.CLIInfo{ID: "admit", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "admit", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_BOOL"},

			"ID": api.Field{Name: "ID", CLITag: api.CLIInfo{ID: "id", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "id", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"IPConfig": api.Field{Name: "IPConfig", CLITag: api.CLIInfo{ID: "ip-config", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "ip-config", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "cluster.IPConfig"},

			"MgmtMode": api.Field{Name: "MgmtMode", CLITag: api.CLIInfo{ID: "mgmt-mode", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "mgmt-mode", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"NetworkMode": api.Field{Name: "NetworkMode", CLITag: api.CLIInfo{ID: "network-mode", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "network-mode", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"MgmtVlan": api.Field{Name: "MgmtVlan", CLITag: api.CLIInfo{ID: "mgmt-vlan", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "mgmt-vlan", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_UINT32"},

			"Controllers": api.Field{Name: "Controllers", CLITag: api.CLIInfo{ID: "controllers", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "controllers", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"RoutingConfig": api.Field{Name: "RoutingConfig", CLITag: api.CLIInfo{ID: "routing-config", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "routing-config", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"DSCProfile": api.Field{Name: "DSCProfile", CLITag: api.CLIInfo{ID: "dscprofile", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "dscprofile", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"cluster.DistributedServiceCardStatus": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(DistributedServiceCardStatus{}) },
		Fields: map[string]api.Field{
			"AdmissionPhase": api.Field{Name: "AdmissionPhase", CLITag: api.CLIInfo{ID: "admission-phase", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "admission-phase", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Conditions": api.Field{Name: "Conditions", CLITag: api.CLIInfo{ID: "conditions", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "conditions", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "cluster.DSCCondition"},

			"SerialNum": api.Field{Name: "SerialNum", CLITag: api.CLIInfo{ID: "serial-num", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "serial-num", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"PrimaryMAC": api.Field{Name: "PrimaryMAC", CLITag: api.CLIInfo{ID: "primary-mac", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "primary-mac", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"IPConfig": api.Field{Name: "IPConfig", CLITag: api.CLIInfo{ID: "ip-config", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "ip-config", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "cluster.IPConfig"},

			"SystemInfo": api.Field{Name: "SystemInfo", CLITag: api.CLIInfo{ID: "system-info", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "system-info", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "cluster.DSCInfo"},

			"Interfaces": api.Field{Name: "Interfaces", CLITag: api.CLIInfo{ID: "interfaces", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "interfaces", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"DSCVersion": api.Field{Name: "DSCVersion", CLITag: api.CLIInfo{ID: "DSCVersion", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "DSCVersion", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"DSCSku": api.Field{Name: "DSCSku", CLITag: api.CLIInfo{ID: "DSCSku", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "DSCSku", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Host": api.Field{Name: "Host", CLITag: api.CLIInfo{ID: "host", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "host", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"AdmissionPhaseReason": api.Field{Name: "AdmissionPhaseReason", CLITag: api.CLIInfo{ID: "adm-phase-reason", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "adm-phase-reason", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"VersionMismatch": api.Field{Name: "VersionMismatch", CLITag: api.CLIInfo{ID: "version-mismatch", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "version-mismatch", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_BOOL"},

			"ControlPlaneStatus": api.Field{Name: "ControlPlaneStatus", CLITag: api.CLIInfo{ID: "control-plane-status", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "control-plane-status", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "cluster.DSCControlPlaneStatus"},

			"IsConnectedToVenice": api.Field{Name: "IsConnectedToVenice", CLITag: api.CLIInfo{ID: "is-connected-to-venice", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "is-connected-to-venice", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_BOOL"},

			"UnhealthyServices": api.Field{Name: "UnhealthyServices", CLITag: api.CLIInfo{ID: "unhealthy-services", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "unhealthy-services", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"cluster.IPConfig": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(IPConfig{}) },
		Fields: map[string]api.Field{
			"IPAddress": api.Field{Name: "IPAddress", CLITag: api.CLIInfo{ID: "ip-address", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "ip-address", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"DefaultGW": api.Field{Name: "DefaultGW", CLITag: api.CLIInfo{ID: "default-gw", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "default-gw", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"DNSServers": api.Field{Name: "DNSServers", CLITag: api.CLIInfo{ID: "dns-servers", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "dns-servers", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"cluster.MacRange": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(MacRange{}) },
		Fields: map[string]api.Field{
			"Start": api.Field{Name: "Start", CLITag: api.CLIInfo{ID: "mac-start", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "mac-start", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"End": api.Field{Name: "End", CLITag: api.CLIInfo{ID: "mac-end", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "mac-end", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"cluster.PeerStatus": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(PeerStatus{}) },
		Fields: map[string]api.Field{
			"PeerAddress": api.Field{Name: "PeerAddress", CLITag: api.CLIInfo{ID: "peer-address", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "peer-address", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"State": api.Field{Name: "State", CLITag: api.CLIInfo{ID: "state", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "state", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"RemoteASN": api.Field{Name: "RemoteASN", CLITag: api.CLIInfo{ID: "remote-asn", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "remote-asn", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_UINT32"},

			"AddressFamilies": api.Field{Name: "AddressFamilies", CLITag: api.CLIInfo{ID: "address-families", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "address-families", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
}

var keyMapSmartnic = map[string][]api.PathsMap{}

func init() {
	schema := runtime.GetDefaultScheme()
	schema.AddSchema(typesMapSmartnic)
	schema.AddPaths(keyMapSmartnic)
}
