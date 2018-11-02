// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package monitoringApiServer is a auto generated package.
Input file: export.proto
*/
package monitoring

import (
	"reflect"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/venice/utils/runtime"
)

var typesMapExport = map[string]*api.Struct{

	"monitoring.ExportConfig": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(ExportConfig{}) },
		Fields: map[string]api.Field{
			"Destination": api.Field{Name: "Destination", CLITag: api.CLIInfo{ID: "destination", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "destination", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Transport": api.Field{Name: "Transport", CLITag: api.CLIInfo{ID: "transport", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "transport", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Credentials": api.Field{Name: "Credentials", CLITag: api.CLIInfo{ID: "credentials", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "credentials", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.ExternalCred"},
		},
	},
	"monitoring.ExternalCred": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(ExternalCred{}) },
		Fields: map[string]api.Field{
			"AuthType": api.Field{Name: "AuthType", CLITag: api.CLIInfo{ID: "auth-type", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "auth-type", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"UserName": api.Field{Name: "UserName", CLITag: api.CLIInfo{ID: "username", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "username", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Password": api.Field{Name: "Password", CLITag: api.CLIInfo{ID: "password", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "password", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"BearerToken": api.Field{Name: "BearerToken", CLITag: api.CLIInfo{ID: "bearer-token", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "bearer-token", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"CertData": api.Field{Name: "CertData", CLITag: api.CLIInfo{ID: "cert-data", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "cert-data", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_BYTES"},

			"KeyData": api.Field{Name: "KeyData", CLITag: api.CLIInfo{ID: "key-data", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "key-data", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_BYTES"},

			"CaData": api.Field{Name: "CaData", CLITag: api.CLIInfo{ID: "ca-data", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "ca-data", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_BYTES"},
		},
	},
	"monitoring.SyslogExportConfig": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(SyslogExportConfig{}) },
		Fields: map[string]api.Field{
			"FacilityOverride": api.Field{Name: "FacilityOverride", CLITag: api.CLIInfo{ID: "facility-override", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "facility-override", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Prefix": api.Field{Name: "Prefix", CLITag: api.CLIInfo{ID: "prefix", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "prefix", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
}

func init() {
	schema := runtime.GetDefaultScheme()
	schema.AddSchema(typesMapExport)
}
