// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package metrics_queryApiServer is a auto generated package.
Input file: metrics_query.proto
*/
package metrics_query

import (
	"reflect"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/venice/utils/runtime"
)

var typesMapMetrics_query = map[string]*api.Struct{

	"metrics_query.ObjectSelector": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(ObjectSelector{}) },
		Fields: map[string]api.Field{
			"Name": api.Field{Name: "Name", CLITag: api.CLIInfo{ID: "name", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "name", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Tenant": api.Field{Name: "Tenant", CLITag: api.CLIInfo{ID: "tenant", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "tenant", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Namespace": api.Field{Name: "Namespace", CLITag: api.CLIInfo{ID: "namespace", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "namespace", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Selector": api.Field{Name: "Selector", CLITag: api.CLIInfo{ID: "selector", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "selector", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "labels.Selector"},
		},
	},
	"metrics_query.PaginationSpec": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(PaginationSpec{}) },
		Fields: map[string]api.Field{
			"Offset": api.Field{Name: "Offset", CLITag: api.CLIInfo{ID: "offset", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "offset", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_INT32"},

			"Count": api.Field{Name: "Count", CLITag: api.CLIInfo{ID: "count", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "count", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_INT32"},
		},
	},
	"metrics_query.QueryResponse": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(QueryResponse{}) },
		Fields: map[string]api.Field{
			"Results": api.Field{Name: "Results", CLITag: api.CLIInfo{ID: "results", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "results", Pointer: true, Slice: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "metrics_query.QueryResult"},
		},
	},
	"metrics_query.QueryResult": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(QueryResult{}) },
		Fields: map[string]api.Field{
			"StatementId": api.Field{Name: "StatementId", CLITag: api.CLIInfo{ID: "statement_id", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "statement_id", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_INT32"},

			"Series": api.Field{Name: "Series", CLITag: api.CLIInfo{ID: "series", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "series", Pointer: true, Slice: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "metrics_query.ResultSeries"},
		},
	},
	"metrics_query.QuerySpec": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(QuerySpec{}) },
		Fields: map[string]api.Field{
			"TypeMeta": api.Field{Name: "TypeMeta", CLITag: api.CLIInfo{ID: "T", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: true, FromInline: false, KeyType: "", Type: "api.TypeMeta"},

			"ObjectSelector": api.Field{Name: "ObjectSelector", CLITag: api.CLIInfo{ID: "meta", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "meta", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "metrics_query.ObjectSelector"},

			"Fields": api.Field{Name: "Fields", CLITag: api.CLIInfo{ID: "fields", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "fields", Pointer: false, Slice: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Function": api.Field{Name: "Function", CLITag: api.CLIInfo{ID: "function", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "function", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"StartTime": api.Field{Name: "StartTime", CLITag: api.CLIInfo{ID: "start-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "start-time", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.Timestamp"},

			"EndTime": api.Field{Name: "EndTime", CLITag: api.CLIInfo{ID: "end-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "end-time", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.Timestamp"},

			"GroupbyTime": api.Field{Name: "GroupbyTime", CLITag: api.CLIInfo{ID: "group-by-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "group-by-time", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"GroupbyField": api.Field{Name: "GroupbyField", CLITag: api.CLIInfo{ID: "group-by-field", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "group-by-field", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Pagination": api.Field{Name: "Pagination", CLITag: api.CLIInfo{ID: "pagination", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "pagination", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "metrics_query.PaginationSpec"},

			"Kind": api.Field{Name: "Kind", CLITag: api.CLIInfo{ID: "kind", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "kind", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"APIVersion": api.Field{Name: "APIVersion", CLITag: api.CLIInfo{ID: "api-version", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "api-version", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Name": api.Field{Name: "Name", CLITag: api.CLIInfo{ID: "name", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "name", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Tenant": api.Field{Name: "Tenant", CLITag: api.CLIInfo{ID: "tenant", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "tenant", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Namespace": api.Field{Name: "Namespace", CLITag: api.CLIInfo{ID: "namespace", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "namespace", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Selector": api.Field{Name: "Selector", CLITag: api.CLIInfo{ID: "selector", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "selector", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "labels.Selector"},
		},
	},
	"metrics_query.ResultSeries": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(ResultSeries{}) },
		Fields: map[string]api.Field{
			"Name": api.Field{Name: "Name", CLITag: api.CLIInfo{ID: "name", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "name", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Tags": api.Field{Name: "Tags", CLITag: api.CLIInfo{ID: "tags", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "tags", Pointer: true, Slice: false, Map: true, Inline: false, FromInline: false, KeyType: "TYPE_STRING", Type: "TYPE_STRING"},

			"Columns": api.Field{Name: "Columns", CLITag: api.CLIInfo{ID: "columns", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "columns", Pointer: false, Slice: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Values": api.Field{Name: "Values", CLITag: api.CLIInfo{ID: "values", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "values", Pointer: true, Slice: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.InterfaceSlice"},
		},
	},
	"metrics_query.ResultSeries.TagsEntry": &api.Struct{
		Fields: map[string]api.Field{
			"key": api.Field{Name: "key", CLITag: api.CLIInfo{ID: "key", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"value": api.Field{Name: "value", CLITag: api.CLIInfo{ID: "value", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
}

func init() {
	schema := runtime.GetDefaultScheme()
	schema.AddSchema(typesMapMetrics_query)
}
