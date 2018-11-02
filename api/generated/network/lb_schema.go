// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package networkApiServer is a auto generated package.
Input file: lb.proto
*/
package network

import (
	"reflect"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/venice/utils/runtime"
)

var typesMapLb = map[string]*api.Struct{

	"network.HealthCheckSpec": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(HealthCheckSpec{}) },
		Fields: map[string]api.Field{
			"Interval": api.Field{Name: "Interval", CLITag: api.CLIInfo{ID: "interval", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "interval", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_UINT32"},

			"ProbesPerInterval": api.Field{Name: "ProbesPerInterval", CLITag: api.CLIInfo{ID: "probes-per-interval", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "probes-per-interval", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_UINT32"},

			"ProbePortOrUrl": api.Field{Name: "ProbePortOrUrl", CLITag: api.CLIInfo{ID: "probe-port-or-url", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "probe-port-or-url", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"MaxTimeouts": api.Field{Name: "MaxTimeouts", CLITag: api.CLIInfo{ID: "max-timeouts", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "max-timeouts", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_UINT32"},

			"DeclareHealthyCount": api.Field{Name: "DeclareHealthyCount", CLITag: api.CLIInfo{ID: "declare-healthy-count", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "declare-healthy-count", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_UINT32"},
		},
	},
	"network.LbPolicy": &api.Struct{
		Kind: "LbPolicy", APIGroup: "network", Scopes: []string{"Tenant"}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(LbPolicy{}) },
		Fields: map[string]api.Field{
			"TypeMeta": api.Field{Name: "TypeMeta", CLITag: api.CLIInfo{ID: "T", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: true, FromInline: false, KeyType: "", Type: "api.TypeMeta"},

			"ObjectMeta": api.Field{Name: "ObjectMeta", CLITag: api.CLIInfo{ID: "meta", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "meta", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.ObjectMeta"},

			"Spec": api.Field{Name: "Spec", CLITag: api.CLIInfo{ID: "spec", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "spec", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "network.LbPolicySpec"},

			"Status": api.Field{Name: "Status", CLITag: api.CLIInfo{ID: "status", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "status", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "network.LbPolicyStatus"},

			"Kind": api.Field{Name: "Kind", CLITag: api.CLIInfo{ID: "kind", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "kind", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"APIVersion": api.Field{Name: "APIVersion", CLITag: api.CLIInfo{ID: "api-version", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "api-version", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Name": api.Field{Name: "Name", CLITag: api.CLIInfo{ID: "name", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "name", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Tenant": api.Field{Name: "Tenant", CLITag: api.CLIInfo{ID: "tenant", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "tenant", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Namespace": api.Field{Name: "Namespace", CLITag: api.CLIInfo{ID: "namespace", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "namespace", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"GenerationID": api.Field{Name: "GenerationID", CLITag: api.CLIInfo{ID: "generation-id", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "generation-id", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"ResourceVersion": api.Field{Name: "ResourceVersion", CLITag: api.CLIInfo{ID: "resource-version", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "resource-version", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"UUID": api.Field{Name: "UUID", CLITag: api.CLIInfo{ID: "uuid", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "uuid", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Labels": api.Field{Name: "Labels", CLITag: api.CLIInfo{ID: "labels", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "labels", Pointer: true, Slice: false, Map: true, Inline: false, FromInline: true, KeyType: "TYPE_STRING", Type: "TYPE_STRING"},

			"CreationTime": api.Field{Name: "CreationTime", CLITag: api.CLIInfo{ID: "creation-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "creation-time", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "api.Timestamp"},

			"ModTime": api.Field{Name: "ModTime", CLITag: api.CLIInfo{ID: "mod-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "mod-time", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "api.Timestamp"},

			"SelfLink": api.Field{Name: "SelfLink", CLITag: api.CLIInfo{ID: "self-link", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "self-link", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},
		},

		CLITags: map[string]api.CLIInfo{
			"algorithm":             api.CLIInfo{Path: "Spec.Algorithm", Skip: false, Insert: "", Help: ""},
			"api-version":           api.CLIInfo{Path: "APIVersion", Skip: false, Insert: "", Help: ""},
			"declare-healthy-count": api.CLIInfo{Path: "Spec.HealthCheck.DeclareHealthyCount", Skip: false, Insert: "", Help: ""},
			"generation-id":         api.CLIInfo{Path: "GenerationID", Skip: false, Insert: "", Help: ""},
			"interval":              api.CLIInfo{Path: "Spec.HealthCheck.Interval", Skip: false, Insert: "", Help: ""},
			"kind":                  api.CLIInfo{Path: "Kind", Skip: false, Insert: "", Help: ""},
			"labels":                api.CLIInfo{Path: "Labels", Skip: false, Insert: "", Help: ""},
			"max-timeouts":          api.CLIInfo{Path: "Spec.HealthCheck.MaxTimeouts", Skip: false, Insert: "", Help: ""},
			"name":                  api.CLIInfo{Path: "Name", Skip: false, Insert: "", Help: ""},
			"namespace":             api.CLIInfo{Path: "Namespace", Skip: false, Insert: "", Help: ""},
			"probe-port-or-url":     api.CLIInfo{Path: "Spec.HealthCheck.ProbePortOrUrl", Skip: false, Insert: "", Help: ""},
			"probes-per-interval":   api.CLIInfo{Path: "Spec.HealthCheck.ProbesPerInterval", Skip: false, Insert: "", Help: ""},
			"resource-version":      api.CLIInfo{Path: "ResourceVersion", Skip: false, Insert: "", Help: ""},
			"self-link":             api.CLIInfo{Path: "SelfLink", Skip: false, Insert: "", Help: ""},
			"session-affinity":      api.CLIInfo{Path: "Spec.SessionAffinity", Skip: false, Insert: "", Help: ""},
			"tenant":                api.CLIInfo{Path: "Tenant", Skip: false, Insert: "", Help: ""},
			"type":                  api.CLIInfo{Path: "Status.Services", Skip: false, Insert: "", Help: ""},
			"uuid":                  api.CLIInfo{Path: "UUID", Skip: false, Insert: "", Help: ""},
		},
	},
	"network.LbPolicySpec": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(LbPolicySpec{}) },
		Fields: map[string]api.Field{
			"Type": api.Field{Name: "Type", CLITag: api.CLIInfo{ID: "type", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "type", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Algorithm": api.Field{Name: "Algorithm", CLITag: api.CLIInfo{ID: "algorithm", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "algorithm", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"SessionAffinity": api.Field{Name: "SessionAffinity", CLITag: api.CLIInfo{ID: "session-affinity", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "session-affinity", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"HealthCheck": api.Field{Name: "HealthCheck", CLITag: api.CLIInfo{ID: "health-check", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "health-check", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "network.HealthCheckSpec"},
		},
	},
	"network.LbPolicyStatus": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(LbPolicyStatus{}) },
		Fields: map[string]api.Field{
			"Services": api.Field{Name: "Services", CLITag: api.CLIInfo{ID: "type", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "type", Pointer: true, Slice: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
}

func init() {
	schema := runtime.GetDefaultScheme()
	schema.AddSchema(typesMapLb)
}
