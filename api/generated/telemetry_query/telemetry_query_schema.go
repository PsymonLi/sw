// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package telemetry_queryApiServer is a auto generated package.
Input file: telemetry_query.proto
*/
package telemetry_query

import (
	"reflect"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/venice/utils/runtime"
)

var typesMapTelemetry_query = map[string]*api.Struct{

	"telemetry_query.Fwlog": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(Fwlog{}) },
		Fields: map[string]api.Field{
			"Tenant": api.Field{Name: "Tenant", CLITag: api.CLIInfo{ID: "tenant", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "tenant", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Src": api.Field{Name: "Src", CLITag: api.CLIInfo{ID: "source", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "source", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Dest": api.Field{Name: "Dest", CLITag: api.CLIInfo{ID: "destination", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "destination", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"SrcPort": api.Field{Name: "SrcPort", CLITag: api.CLIInfo{ID: "source-port", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "source-port", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_UINT32"},

			"DestPort": api.Field{Name: "DestPort", CLITag: api.CLIInfo{ID: "destination-port", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "destination-port", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_UINT32"},

			"Protocol": api.Field{Name: "Protocol", CLITag: api.CLIInfo{ID: "protocol", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "protocol", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Action": api.Field{Name: "Action", CLITag: api.CLIInfo{ID: "action", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "action", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Direction": api.Field{Name: "Direction", CLITag: api.CLIInfo{ID: "direction", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "direction", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"RuleID": api.Field{Name: "RuleID", CLITag: api.CLIInfo{ID: "rule-id", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "rule-id", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"PolicyName": api.Field{Name: "PolicyName", CLITag: api.CLIInfo{ID: "policy-name", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "policy-name", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Timestamp": api.Field{Name: "Timestamp", CLITag: api.CLIInfo{ID: "timestamp", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "timestamp", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.Timestamp"},
		},
	},
	"telemetry_query.FwlogsQueryList": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(FwlogsQueryList{}) },
		Fields: map[string]api.Field{
			"Tenant": api.Field{Name: "Tenant", CLITag: api.CLIInfo{ID: "tenant", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "tenant", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Namespace": api.Field{Name: "Namespace", CLITag: api.CLIInfo{ID: "namespace", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "namespace", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Queries": api.Field{Name: "Queries", CLITag: api.CLIInfo{ID: "queries", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "queries", Pointer: true, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "telemetry_query.FwlogsQuerySpec"},
		},
	},
	"telemetry_query.FwlogsQueryResponse": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(FwlogsQueryResponse{}) },
		Fields: map[string]api.Field{
			"Tenant": api.Field{Name: "Tenant", CLITag: api.CLIInfo{ID: "tenant", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "tenant", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Namespace": api.Field{Name: "Namespace", CLITag: api.CLIInfo{ID: "namespace", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "namespace", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Results": api.Field{Name: "Results", CLITag: api.CLIInfo{ID: "results", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "results", Pointer: true, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "telemetry_query.FwlogsQueryResult"},
		},
	},
	"telemetry_query.FwlogsQueryResult": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(FwlogsQueryResult{}) },
		Fields: map[string]api.Field{
			"StatementID": api.Field{Name: "StatementID", CLITag: api.CLIInfo{ID: "statement_id", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "statement_id", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_INT32"},

			"Logs": api.Field{Name: "Logs", CLITag: api.CLIInfo{ID: "logs", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "logs", Pointer: true, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "telemetry_query.Fwlog"},
		},
	},
	"telemetry_query.FwlogsQuerySpec": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(FwlogsQuerySpec{}) },
		Fields: map[string]api.Field{
			"SourceIPs": api.Field{Name: "SourceIPs", CLITag: api.CLIInfo{ID: "source-ips", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "source-ips", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"DestIPs": api.Field{Name: "DestIPs", CLITag: api.CLIInfo{ID: "dest-ips", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "dest-ips", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"SourcePorts": api.Field{Name: "SourcePorts", CLITag: api.CLIInfo{ID: "source-ports", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "source-ports", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_UINT32"},

			"DestPorts": api.Field{Name: "DestPorts", CLITag: api.CLIInfo{ID: "dest-ports", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "dest-ports", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_UINT32"},

			"Protocols": api.Field{Name: "Protocols", CLITag: api.CLIInfo{ID: "protocols", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "protocols", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Actions": api.Field{Name: "Actions", CLITag: api.CLIInfo{ID: "actions", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "actions", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Directions": api.Field{Name: "Directions", CLITag: api.CLIInfo{ID: "directions", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "directions", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"RuleIDs": api.Field{Name: "RuleIDs", CLITag: api.CLIInfo{ID: "rule-ids", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "rule-ids", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"PolicyNames": api.Field{Name: "PolicyNames", CLITag: api.CLIInfo{ID: "policy-names", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "policy-names", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"StartTime": api.Field{Name: "StartTime", CLITag: api.CLIInfo{ID: "start-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "start-time", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.Timestamp"},

			"EndTime": api.Field{Name: "EndTime", CLITag: api.CLIInfo{ID: "end-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "end-time", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.Timestamp"},

			"Pagination": api.Field{Name: "Pagination", CLITag: api.CLIInfo{ID: "pagination", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "pagination", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "telemetry_query.PaginationSpec"},
		},
	},
	"telemetry_query.MetricsQueryList": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(MetricsQueryList{}) },
		Fields: map[string]api.Field{
			"Tenant": api.Field{Name: "Tenant", CLITag: api.CLIInfo{ID: "tenant", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "tenant", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Namespace": api.Field{Name: "Namespace", CLITag: api.CLIInfo{ID: "namespace", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "namespace", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Queries": api.Field{Name: "Queries", CLITag: api.CLIInfo{ID: "queries", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "queries", Pointer: true, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "telemetry_query.MetricsQuerySpec"},
		},
	},
	"telemetry_query.MetricsQueryResponse": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(MetricsQueryResponse{}) },
		Fields: map[string]api.Field{
			"Tenant": api.Field{Name: "Tenant", CLITag: api.CLIInfo{ID: "tenant", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "tenant", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Namespace": api.Field{Name: "Namespace", CLITag: api.CLIInfo{ID: "namespace", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "namespace", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Results": api.Field{Name: "Results", CLITag: api.CLIInfo{ID: "results", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "results", Pointer: true, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "telemetry_query.MetricsQueryResult"},
		},
	},
	"telemetry_query.MetricsQueryResult": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(MetricsQueryResult{}) },
		Fields: map[string]api.Field{
			"StatementID": api.Field{Name: "StatementID", CLITag: api.CLIInfo{ID: "statement_id", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "statement_id", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_INT32"},

			"Series": api.Field{Name: "Series", CLITag: api.CLIInfo{ID: "series", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "series", Pointer: true, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "telemetry_query.ResultSeries"},
		},
	},
	"telemetry_query.MetricsQuerySpec": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(MetricsQuerySpec{}) },
		Fields: map[string]api.Field{
			"TypeMeta": api.Field{Name: "TypeMeta", CLITag: api.CLIInfo{ID: "T", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: true, FromInline: false, KeyType: "", Type: "api.TypeMeta"},

			"Name": api.Field{Name: "Name", CLITag: api.CLIInfo{ID: "name", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "name", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Selector": api.Field{Name: "Selector", CLITag: api.CLIInfo{ID: "selector", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "selector", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "labels.Selector"},

			"Fields": api.Field{Name: "Fields", CLITag: api.CLIInfo{ID: "fields", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "fields", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Function": api.Field{Name: "Function", CLITag: api.CLIInfo{ID: "function", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "function", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"StartTime": api.Field{Name: "StartTime", CLITag: api.CLIInfo{ID: "start-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "start-time", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.Timestamp"},

			"EndTime": api.Field{Name: "EndTime", CLITag: api.CLIInfo{ID: "end-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "end-time", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.Timestamp"},

			"GroupbyTime": api.Field{Name: "GroupbyTime", CLITag: api.CLIInfo{ID: "group-by-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "group-by-time", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"GroupbyField": api.Field{Name: "GroupbyField", CLITag: api.CLIInfo{ID: "group-by-field", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "group-by-field", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Pagination": api.Field{Name: "Pagination", CLITag: api.CLIInfo{ID: "pagination", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "pagination", Pointer: true, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "telemetry_query.PaginationSpec"},

			"Kind": api.Field{Name: "Kind", CLITag: api.CLIInfo{ID: "kind", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "kind", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"APIVersion": api.Field{Name: "APIVersion", CLITag: api.CLIInfo{ID: "api-version", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "api-version", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"telemetry_query.PaginationSpec": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(PaginationSpec{}) },
		Fields: map[string]api.Field{
			"Offset": api.Field{Name: "Offset", CLITag: api.CLIInfo{ID: "offset", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "offset", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_INT32"},

			"Count": api.Field{Name: "Count", CLITag: api.CLIInfo{ID: "count", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "count", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_INT32"},
		},
	},
	"telemetry_query.ResultSeries": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(ResultSeries{}) },
		Fields: map[string]api.Field{
			"Name": api.Field{Name: "Name", CLITag: api.CLIInfo{ID: "name", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "name", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Tags": api.Field{Name: "Tags", CLITag: api.CLIInfo{ID: "tags", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "tags", Pointer: true, Slice: false, Mutable: true, Map: true, Inline: false, FromInline: false, KeyType: "TYPE_STRING", Type: "TYPE_STRING"},

			"Columns": api.Field{Name: "Columns", CLITag: api.CLIInfo{ID: "columns", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "columns", Pointer: false, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Values": api.Field{Name: "Values", CLITag: api.CLIInfo{ID: "values", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "values", Pointer: true, Slice: true, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.InterfaceSlice"},
		},
	},
	"telemetry_query.ResultSeries.TagsEntry": &api.Struct{
		Fields: map[string]api.Field{
			"key": api.Field{Name: "key", CLITag: api.CLIInfo{ID: "key", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"value": api.Field{Name: "value", CLITag: api.CLIInfo{ID: "value", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Mutable: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
}

var keyMapTelemetry_query = map[string][]api.PathsMap{}

func init() {
	schema := runtime.GetDefaultScheme()
	schema.AddSchema(typesMapTelemetry_query)
	schema.AddPaths(keyMapTelemetry_query)
}
