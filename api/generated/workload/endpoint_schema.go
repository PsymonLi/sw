// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package workloadApiServer is a auto generated package.
Input file: endpoint.proto
*/
package workload

import (
	"reflect"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/venice/utils/runtime"
)

var typesMapEndpoint = map[string]*api.Struct{

	"workload.Endpoint": &api.Struct{
		Kind: "Endpoint", APIGroup: "workload", Scopes: []string{"Tenant"}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(Endpoint{}) },
		Fields: map[string]api.Field{
			"TypeMeta": api.Field{Name: "TypeMeta", CLITag: api.CLIInfo{ID: "T", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: true, FromInline: false, KeyType: "", Type: "api.TypeMeta"},

			"ObjectMeta": api.Field{Name: "ObjectMeta", CLITag: api.CLIInfo{ID: "meta", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "meta", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.ObjectMeta"},

			"Spec": api.Field{Name: "Spec", CLITag: api.CLIInfo{ID: "spec", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "spec", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "workload.EndpointSpec"},

			"Status": api.Field{Name: "Status", CLITag: api.CLIInfo{ID: "status", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "status", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "workload.EndpointStatus"},

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
			"EndpointState":       api.CLIInfo{Path: "Status.EndpointState", Skip: false, Insert: "", Help: ""},
			"SecurityGroups":      api.CLIInfo{Path: "Status.SecurityGroups", Skip: false, Insert: "", Help: ""},
			"api-version":         api.CLIInfo{Path: "APIVersion", Skip: false, Insert: "", Help: ""},
			"generation-id":       api.CLIInfo{Path: "GenerationID", Skip: false, Insert: "", Help: ""},
			"homing-host-addr":    api.CLIInfo{Path: "Status.HomingHostAddr", Skip: false, Insert: "", Help: ""},
			"homing-host-name":    api.CLIInfo{Path: "Status.HomingHostName", Skip: false, Insert: "", Help: ""},
			"ipv4-address":        api.CLIInfo{Path: "Status.IPv4Address", Skip: false, Insert: "", Help: ""},
			"ipv4-gateway":        api.CLIInfo{Path: "Status.IPv4Gateway", Skip: false, Insert: "", Help: ""},
			"ipv6-address":        api.CLIInfo{Path: "Status.IPv6Address", Skip: false, Insert: "", Help: ""},
			"ipv6-gateway":        api.CLIInfo{Path: "Status.IPv6Gateway", Skip: false, Insert: "", Help: ""},
			"kind":                api.CLIInfo{Path: "Kind", Skip: false, Insert: "", Help: ""},
			"labels":              api.CLIInfo{Path: "Labels", Skip: false, Insert: "", Help: ""},
			"mac-address":         api.CLIInfo{Path: "Status.MacAddress", Skip: false, Insert: "", Help: ""},
			"micro-segment-vlan":  api.CLIInfo{Path: "Status.MicroSegmentVlan", Skip: false, Insert: "", Help: ""},
			"name":                api.CLIInfo{Path: "Name", Skip: false, Insert: "", Help: ""},
			"namespace":           api.CLIInfo{Path: "Namespace", Skip: false, Insert: "", Help: ""},
			"network":             api.CLIInfo{Path: "Status.Network", Skip: false, Insert: "", Help: ""},
			"node-uuid":           api.CLIInfo{Path: "Status.NodeUUID", Skip: false, Insert: "", Help: ""},
			"resource-version":    api.CLIInfo{Path: "ResourceVersion", Skip: false, Insert: "", Help: ""},
			"self-link":           api.CLIInfo{Path: "SelfLink", Skip: false, Insert: "", Help: ""},
			"status":              api.CLIInfo{Path: "Status.Migration.Status", Skip: false, Insert: "", Help: ""},
			"tenant":              api.CLIInfo{Path: "Tenant", Skip: false, Insert: "", Help: ""},
			"uuid":                api.CLIInfo{Path: "UUID", Skip: false, Insert: "", Help: ""},
			"workload-attributes": api.CLIInfo{Path: "Status.WorkloadAttributes", Skip: false, Insert: "", Help: ""},
			"workload-name":       api.CLIInfo{Path: "Status.WorkloadName", Skip: false, Insert: "", Help: ""},
		},
	},
	"workload.EndpointMigrationStatus": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(EndpointMigrationStatus{}) },
		Fields: map[string]api.Field{
			"Status": api.Field{Name: "Status", CLITag: api.CLIInfo{ID: "status", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "status", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"workload.EndpointSpec": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(EndpointSpec{}) },
		Fields: map[string]api.Field{
			"NodeUUID": api.Field{Name: "NodeUUID", CLITag: api.CLIInfo{ID: "node-uuid", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "node-uuid", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"HomingHostAddr": api.Field{Name: "HomingHostAddr", CLITag: api.CLIInfo{ID: "homing-host-addr", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "homing-host-addr", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"workload.EndpointStatus": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(EndpointStatus{}) },
		Fields: map[string]api.Field{
			"WorkloadName": api.Field{Name: "WorkloadName", CLITag: api.CLIInfo{ID: "workload-name", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "workload-name", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Network": api.Field{Name: "Network", CLITag: api.CLIInfo{ID: "network", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "network", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"HomingHostAddr": api.Field{Name: "HomingHostAddr", CLITag: api.CLIInfo{ID: "homing-host-addr", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "homing-host-addr", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"HomingHostName": api.Field{Name: "HomingHostName", CLITag: api.CLIInfo{ID: "homing-host-name", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "homing-host-name", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"IPv4Address": api.Field{Name: "IPv4Address", CLITag: api.CLIInfo{ID: "ipv4-address", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "ipv4-address", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"IPv4Gateway": api.Field{Name: "IPv4Gateway", CLITag: api.CLIInfo{ID: "ipv4-gateway", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "ipv4-gateway", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"IPv6Address": api.Field{Name: "IPv6Address", CLITag: api.CLIInfo{ID: "ipv6-address", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "ipv6-address", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"IPv6Gateway": api.Field{Name: "IPv6Gateway", CLITag: api.CLIInfo{ID: "ipv6-gateway", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "ipv6-gateway", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"MacAddress": api.Field{Name: "MacAddress", CLITag: api.CLIInfo{ID: "mac-address", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "mac-address", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"NodeUUID": api.Field{Name: "NodeUUID", CLITag: api.CLIInfo{ID: "node-uuid", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "node-uuid", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"EndpointState": api.Field{Name: "EndpointState", CLITag: api.CLIInfo{ID: "EndpointState", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"SecurityGroups": api.Field{Name: "SecurityGroups", CLITag: api.CLIInfo{ID: "SecurityGroups", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"MicroSegmentVlan": api.Field{Name: "MicroSegmentVlan", CLITag: api.CLIInfo{ID: "micro-segment-vlan", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "micro-segment-vlan", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_UINT32"},

			"WorkloadAttributes": api.Field{Name: "WorkloadAttributes", CLITag: api.CLIInfo{ID: "workload-attributes", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "workload-attributes", Pointer: true, Slice: false, Mutable: true, Map: true, Inline: false, FromInline: false, KeyType: "TYPE_STRING", Type: "TYPE_STRING"},

			"Migration": api.Field{Name: "Migration", CLITag: api.CLIInfo{ID: "migration", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "migration", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "workload.EndpointMigrationStatus"},
		},
	},
	"workload.EndpointStatus.WorkloadAttributesEntry": &api.Struct{
		Fields: map[string]api.Field{
			"key": api.Field{Name: "key", CLITag: api.CLIInfo{ID: "key", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"value": api.Field{Name: "value", CLITag: api.CLIInfo{ID: "value", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
}

var keyMapEndpoint = map[string][]api.PathsMap{}

func init() {
	schema := runtime.GetDefaultScheme()
	schema.AddSchema(typesMapEndpoint)
	schema.AddPaths(keyMapEndpoint)
}
