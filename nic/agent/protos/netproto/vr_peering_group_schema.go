// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package netprotoApiServer is a auto generated package.
Input file: vr_peering_group.proto
*/
package netproto

import (
	"reflect"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/venice/utils/runtime"
)

var typesMapVr_peering_group = map[string]*api.Struct{

	"netproto.VRPeeringRoute": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(VRPeeringRoute{}) },
		Fields: map[string]api.Field{
			"IPv4Prefix": api.Field{Name: "IPv4Prefix", CLITag: api.CLIInfo{ID: "ipv4-prefix", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "ipv4-prefix", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"DestVirtualRouter": api.Field{Name: "DestVirtualRouter", CLITag: api.CLIInfo{ID: "dest-virtual-router", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "dest-virtual-router", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"netproto.VRPeeringRouteTable": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(VRPeeringRouteTable{}) },
		Fields: map[string]api.Field{
			"VRPeeringRoutes": api.Field{Name: "VRPeeringRoutes", CLITag: api.CLIInfo{ID: "vr-peering-routes", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "vr-peering-routes", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "netproto.VRPeeringRoute"},
		},
	},
	"netproto.VirtualRouterPeeringGroup": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(VirtualRouterPeeringGroup{}) },
		Fields: map[string]api.Field{
			"TypeMeta": api.Field{Name: "TypeMeta", CLITag: api.CLIInfo{ID: "TypeMeta", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: true, FromInline: false, KeyType: "", Type: "api.TypeMeta"},

			"ObjectMeta": api.Field{Name: "ObjectMeta", CLITag: api.CLIInfo{ID: "meta", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "meta", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.ObjectMeta"},

			"Spec": api.Field{Name: "Spec", CLITag: api.CLIInfo{ID: "spec", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "spec", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "netproto.VirtualRouterPeeringGroupSpec"},

			"Status": api.Field{Name: "Status", CLITag: api.CLIInfo{ID: "status", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "status", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "netproto.VirtualRouterPeeringGroupStatus"},

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
			"api-version":         api.CLIInfo{Path: "APIVersion", Skip: false, Insert: "", Help: ""},
			"dest-virtual-router": api.CLIInfo{Path: "Spec.VRPeeringRouteTables[].VRPeeringRoutes[].DestVirtualRouter", Skip: false, Insert: "", Help: ""},
			"generation-id":       api.CLIInfo{Path: "GenerationID", Skip: false, Insert: "", Help: ""},
			"ipv4-prefix":         api.CLIInfo{Path: "Spec.VRPeeringRouteTables[].VRPeeringRoutes[].IPv4Prefix", Skip: false, Insert: "", Help: ""},
			"kind":                api.CLIInfo{Path: "Kind", Skip: false, Insert: "", Help: ""},
			"labels":              api.CLIInfo{Path: "Labels", Skip: false, Insert: "", Help: ""},
			"name":                api.CLIInfo{Path: "Name", Skip: false, Insert: "", Help: ""},
			"namespace":           api.CLIInfo{Path: "Namespace", Skip: false, Insert: "", Help: ""},
			"resource-version":    api.CLIInfo{Path: "ResourceVersion", Skip: false, Insert: "", Help: ""},
			"self-link":           api.CLIInfo{Path: "SelfLink", Skip: false, Insert: "", Help: ""},
			"tenant":              api.CLIInfo{Path: "Tenant", Skip: false, Insert: "", Help: ""},
			"uuid":                api.CLIInfo{Path: "UUID", Skip: false, Insert: "", Help: ""},
		},
	},
	"netproto.VirtualRouterPeeringGroupEvent": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(VirtualRouterPeeringGroupEvent{}) },
		Fields: map[string]api.Field{
			"EventType": api.Field{Name: "EventType", CLITag: api.CLIInfo{ID: "event-type", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "event-type", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_ENUM"},

			"VirtualRouterPeeringGroup": api.Field{Name: "VirtualRouterPeeringGroup", CLITag: api.CLIInfo{ID: "vr-peering-group", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "vr-peering-group", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "netproto.VirtualRouterPeeringGroup"},
		},
	},
	"netproto.VirtualRouterPeeringGroupEventList": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(VirtualRouterPeeringGroupEventList{}) },
		Fields: map[string]api.Field{
			"virtualRouterPeeringGroupEvents": api.Field{Name: "virtualRouterPeeringGroupEvents", CLITag: api.CLIInfo{ID: "virtualRouterPeeringGroupEvents", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: true, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "netproto.VirtualRouterPeeringGroupEvent"},
		},
	},
	"netproto.VirtualRouterPeeringGroupList": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(VirtualRouterPeeringGroupList{}) },
		Fields: map[string]api.Field{
			"virtualRouterPeeringGroups": api.Field{Name: "virtualRouterPeeringGroups", CLITag: api.CLIInfo{ID: "virtualRouterPeeringGroups", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: true, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "netproto.VirtualRouterPeeringGroup"},
		},
	},
	"netproto.VirtualRouterPeeringGroupSpec": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(VirtualRouterPeeringGroupSpec{}) },
		Fields: map[string]api.Field{
			"VRPeeringRouteTables": api.Field{Name: "VRPeeringRouteTables", CLITag: api.CLIInfo{ID: "vr-peering-route-tables", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "vr-peering-route-tables", Pointer: true, Slice: false, Mutable: true, Map: true, Inline: false, FromInline: false, KeyType: "TYPE_STRING", Type: "netproto.VRPeeringRouteTable"},
		},
	},
	"netproto.VirtualRouterPeeringGroupSpec.VRPeeringRouteTablesEntry": &api.Struct{
		Fields: map[string]api.Field{
			"key": api.Field{Name: "key", CLITag: api.CLIInfo{ID: "key", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"value": api.Field{Name: "value", CLITag: api.CLIInfo{ID: "value", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "netproto.VRPeeringRouteTable"},
		},
	},
	"netproto.VirtualRouterPeeringGroupStatus": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(VirtualRouterPeeringGroupStatus{}) },
		Fields: map[string]api.Field{
			"VRPeeringRouteTables": api.Field{Name: "VRPeeringRouteTables", CLITag: api.CLIInfo{ID: "vr-peering-route-tables", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "vr-peering-route-tables", Pointer: true, Slice: false, Mutable: true, Map: true, Inline: false, FromInline: false, KeyType: "TYPE_STRING", Type: "netproto.VRPeeringRouteTable"},
		},
	},
	"netproto.VirtualRouterPeeringGroupStatus.VRPeeringRouteTablesEntry": &api.Struct{
		Fields: map[string]api.Field{
			"key": api.Field{Name: "key", CLITag: api.CLIInfo{ID: "key", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"value": api.Field{Name: "value", CLITag: api.CLIInfo{ID: "value", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "netproto.VRPeeringRouteTable"},
		},
	},
}

var keyMapVr_peering_group = map[string][]api.PathsMap{}

func init() {
	schema := runtime.GetDefaultScheme()
	schema.AddSchema(typesMapVr_peering_group)
	schema.AddPaths(keyMapVr_peering_group)
}
