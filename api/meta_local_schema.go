// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package apiApiServer is a auto generated package.
Input file: meta.proto
*/
package api

import "reflect"

var typesMapMeta = map[string]*Struct{

	"api.Any": &Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(Any{}) },
		Fields: map[string]Field{
			"Any": Field{Name: "Any", CLITag: CLIInfo{ID: "Object", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "google.protobuf.Any"},

			"type_url": Field{Name: "type_url", CLITag: CLIInfo{ID: "type_url", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"value": Field{Name: "value", CLITag: CLIInfo{ID: "value", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_BYTES"},
		},
	},
	"api.Interface": &Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(Interface{}) },
		Fields: map[string]Field{
			"Str": Field{Name: "Str", CLITag: CLIInfo{ID: "Str", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Int64": Field{Name: "Int64", CLITag: CLIInfo{ID: "Int64", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_INT64"},

			"Bool": Field{Name: "Bool", CLITag: CLIInfo{ID: "Bool", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_BOOL"},

			"Float": Field{Name: "Float", CLITag: CLIInfo{ID: "Float", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_DOUBLE"},

			"Interfaces": Field{Name: "Interfaces", CLITag: CLIInfo{ID: "Interfaces", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.InterfaceSlice"},
		},
	},
	"api.InterfaceSlice": &Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(InterfaceSlice{}) },
		Fields: map[string]Field{
			"Values": Field{Name: "Values", CLITag: CLIInfo{ID: "Values", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: true, Slice: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.Interface"},
		},
	},
	"api.ListMeta": &Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(ListMeta{}) },
		Fields: map[string]Field{
			"ResourceVersion": Field{Name: "ResourceVersion", CLITag: CLIInfo{ID: "resource-version", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "resource-version", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"api.ListWatchOptions": &Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(ListWatchOptions{}) },
		Fields: map[string]Field{
			"ObjectMeta": Field{Name: "ObjectMeta", CLITag: CLIInfo{ID: "O", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.ObjectMeta"},

			"LabelSelector": Field{Name: "LabelSelector", CLITag: CLIInfo{ID: "label-selector", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "label-selector", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"FieldSelector": Field{Name: "FieldSelector", CLITag: CLIInfo{ID: "field-selector", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "field-selector", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"FieldChangeSelector": Field{Name: "FieldChangeSelector", CLITag: CLIInfo{ID: "field-change-selector", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "field-change-selector", Pointer: false, Slice: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"From": Field{Name: "From", CLITag: CLIInfo{ID: "from", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "from", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_INT32"},

			"MaxResults": Field{Name: "MaxResults", CLITag: CLIInfo{ID: "max-results", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "max-results", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_INT32"},

			"Name": Field{Name: "Name", CLITag: CLIInfo{ID: "name", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "name", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Tenant": Field{Name: "Tenant", CLITag: CLIInfo{ID: "tenant", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "tenant", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Namespace": Field{Name: "Namespace", CLITag: CLIInfo{ID: "namespace", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "namespace", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"GenerationID": Field{Name: "GenerationID", CLITag: CLIInfo{ID: "generation-id", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "generation-id", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"ResourceVersion": Field{Name: "ResourceVersion", CLITag: CLIInfo{ID: "resource-version", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "resource-version", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"UUID": Field{Name: "UUID", CLITag: CLIInfo{ID: "uuid", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "uuid", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Labels": Field{Name: "Labels", CLITag: CLIInfo{ID: "labels", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "labels", Pointer: true, Slice: false, Map: true, Inline: false, FromInline: true, KeyType: "TYPE_STRING", Type: "TYPE_STRING"},

			"CreationTime": Field{Name: "CreationTime", CLITag: CLIInfo{ID: "creation-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "creation-time", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "api.Timestamp"},

			"ModTime": Field{Name: "ModTime", CLITag: CLIInfo{ID: "mod-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "mod-time", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "api.Timestamp"},

			"SelfLink": Field{Name: "SelfLink", CLITag: CLIInfo{ID: "self-link", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "self-link", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"api.ObjectMeta": &Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(ObjectMeta{}) },
		Fields: map[string]Field{
			"Name": Field{Name: "Name", CLITag: CLIInfo{ID: "name", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "name", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Tenant": Field{Name: "Tenant", CLITag: CLIInfo{ID: "tenant", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "tenant", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Namespace": Field{Name: "Namespace", CLITag: CLIInfo{ID: "namespace", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "namespace", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"GenerationID": Field{Name: "GenerationID", CLITag: CLIInfo{ID: "generation-id", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "generation-id", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"ResourceVersion": Field{Name: "ResourceVersion", CLITag: CLIInfo{ID: "resource-version", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "resource-version", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"UUID": Field{Name: "UUID", CLITag: CLIInfo{ID: "uuid", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "uuid", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Labels": Field{Name: "Labels", CLITag: CLIInfo{ID: "labels", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "labels", Pointer: true, Slice: false, Map: true, Inline: false, FromInline: false, KeyType: "TYPE_STRING", Type: "TYPE_STRING"},

			"CreationTime": Field{Name: "CreationTime", CLITag: CLIInfo{ID: "creation-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "creation-time", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.Timestamp"},

			"ModTime": Field{Name: "ModTime", CLITag: CLIInfo{ID: "mod-time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "mod-time", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.Timestamp"},

			"SelfLink": Field{Name: "SelfLink", CLITag: CLIInfo{ID: "self-link", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "self-link", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"api.ObjectMeta.LabelsEntry": &Struct{
		Fields: map[string]Field{
			"key": Field{Name: "key", CLITag: CLIInfo{ID: "key", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"value": Field{Name: "value", CLITag: CLIInfo{ID: "value", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"api.ObjectRef": &Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(ObjectRef{}) },
		Fields: map[string]Field{
			"Tenant": Field{Name: "Tenant", CLITag: CLIInfo{ID: "tenant", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "tenant", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Namespace": Field{Name: "Namespace", CLITag: CLIInfo{ID: "namespace", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "namespace", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Kind": Field{Name: "Kind", CLITag: CLIInfo{ID: "kind", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "kind", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Name": Field{Name: "Name", CLITag: CLIInfo{ID: "name", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "name", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"URI": Field{Name: "URI", CLITag: CLIInfo{ID: "uri", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "uri", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"api.Status": &Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(Status{}) },
		Fields: map[string]Field{
			"TypeMeta": Field{Name: "TypeMeta", CLITag: CLIInfo{ID: "T", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.TypeMeta"},

			"Result": Field{Name: "Result", CLITag: CLIInfo{ID: "result", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "result", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.StatusResult"},

			"Message": Field{Name: "Message", CLITag: CLIInfo{ID: "message", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "message", Pointer: false, Slice: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Code": Field{Name: "Code", CLITag: CLIInfo{ID: "code", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "code", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_INT32"},

			"Ref": Field{Name: "Ref", CLITag: CLIInfo{ID: "object-ref", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "object-ref", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.ObjectRef"},

			"Kind": Field{Name: "Kind", CLITag: CLIInfo{ID: "kind", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "kind", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"APIVersion": Field{Name: "APIVersion", CLITag: CLIInfo{ID: "api-version", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "api-version", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"api.StatusResult": &Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(StatusResult{}) },
		Fields: map[string]Field{
			"Str": Field{Name: "Str", CLITag: CLIInfo{ID: "Str", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"api.Timestamp": &Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(Timestamp{}) },
		Fields: map[string]Field{
			"Timestamp": Field{Name: "Timestamp", CLITag: CLIInfo{ID: "Time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "google.protobuf.Timestamp"},

			"seconds": Field{Name: "seconds", CLITag: CLIInfo{ID: "seconds", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_INT64"},

			"nanos": Field{Name: "nanos", CLITag: CLIInfo{ID: "nanos", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_INT32"},
		},
	},
	"api.TypeMeta": &Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(TypeMeta{}) },
		Fields: map[string]Field{
			"Kind": Field{Name: "Kind", CLITag: CLIInfo{ID: "kind", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "kind", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"APIVersion": Field{Name: "APIVersion", CLITag: CLIInfo{ID: "api-version", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "api-version", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"api.WatchEvent": &Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(WatchEvent{}) },
		Fields: map[string]Field{
			"Type": Field{Name: "Type", CLITag: CLIInfo{ID: "type", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "type", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Object": Field{Name: "Object", CLITag: CLIInfo{ID: "object", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "object", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "google.protobuf.Any"},
		},
	},
	"api.WatchEventList": &Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(WatchEventList{}) },
		Fields: map[string]Field{
			"Events": Field{Name: "Events", CLITag: CLIInfo{ID: "events", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "events", Pointer: true, Slice: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.WatchEvent"},
		},
	},
}

func init() {
	schema_init_once.Do(init_schema_map)
	for k, v := range typesMapMeta {
		local_schema[k] = v
	}
}
