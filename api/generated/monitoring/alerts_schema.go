// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package monitoringApiServer is a auto generated package.
Input file: alerts.proto
*/
package monitoring

import (
	"reflect"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/venice/utils/runtime"
)

var typesMapAlerts = map[string]*api.Struct{

	"monitoring.Alert": &api.Struct{
		Kind: "Alert", APIGroup: "monitoring", Scopes: []string{"Tenant"}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(Alert{}) },
		Fields: map[string]api.Field{
			"TypeMeta": api.Field{Name: "TypeMeta", CLITag: api.CLIInfo{ID: "T", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: true, FromInline: false, KeyType: "", Type: "api.TypeMeta"},

			"ObjectMeta": api.Field{Name: "ObjectMeta", CLITag: api.CLIInfo{ID: "meta", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "meta", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.ObjectMeta"},

			"Spec": api.Field{Name: "Spec", CLITag: api.CLIInfo{ID: "spec", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "spec", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.AlertSpec"},

			"Status": api.Field{Name: "Status", CLITag: api.CLIInfo{ID: "status", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "status", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.AlertStatus"},

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
			"alert-policy-id":  api.CLIInfo{Path: "Status.Reason.PolicyID", Skip: false, Insert: "", Help: ""},
			"api-version":      api.CLIInfo{Path: "APIVersion", Skip: false, Insert: "", Help: ""},
			"component":        api.CLIInfo{Path: "Status.Source.Component", Skip: false, Insert: "", Help: ""},
			"event-uri":        api.CLIInfo{Path: "Status.EventURI", Skip: false, Insert: "", Help: ""},
			"generation-id":    api.CLIInfo{Path: "GenerationID", Skip: false, Insert: "", Help: ""},
			"key":              api.CLIInfo{Path: "Status.Reason.MatchedRequirements[].Key", Skip: false, Insert: "", Help: ""},
			"kind":             api.CLIInfo{Path: "Kind", Skip: false, Insert: "", Help: ""},
			"labels":           api.CLIInfo{Path: "Labels", Skip: false, Insert: "", Help: ""},
			"message":          api.CLIInfo{Path: "Status.Message", Skip: false, Insert: "", Help: ""},
			"name":             api.CLIInfo{Path: "Name", Skip: false, Insert: "", Help: ""},
			"namespace":        api.CLIInfo{Path: "Namespace", Skip: false, Insert: "", Help: ""},
			"node-name":        api.CLIInfo{Path: "Status.Source.NodeName", Skip: false, Insert: "", Help: ""},
			"observed-value":   api.CLIInfo{Path: "Status.Reason.MatchedRequirements[].ObservedValue", Skip: false, Insert: "", Help: ""},
			"operator":         api.CLIInfo{Path: "Status.Reason.MatchedRequirements[].Operator", Skip: false, Insert: "", Help: ""},
			"resource-version": api.CLIInfo{Path: "ResourceVersion", Skip: false, Insert: "", Help: ""},
			"self-link":        api.CLIInfo{Path: "SelfLink", Skip: false, Insert: "", Help: ""},
			"severity":         api.CLIInfo{Path: "Status.Severity", Skip: false, Insert: "", Help: ""},
			"state":            api.CLIInfo{Path: "Spec.State", Skip: false, Insert: "", Help: ""},
			"tenant":           api.CLIInfo{Path: "Tenant", Skip: false, Insert: "", Help: ""},
			"user":             api.CLIInfo{Path: "Status.Resolved.User", Skip: false, Insert: "", Help: ""},
			"uuid":             api.CLIInfo{Path: "UUID", Skip: false, Insert: "", Help: ""},
			"values":           api.CLIInfo{Path: "Status.Reason.MatchedRequirements[].Values", Skip: false, Insert: "", Help: ""},
		},
	},
	"monitoring.AlertDestination": &api.Struct{
		Kind: "AlertDestination", APIGroup: "monitoring", Scopes: []string{"Tenant"}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(AlertDestination{}) },
		Fields: map[string]api.Field{
			"TypeMeta": api.Field{Name: "TypeMeta", CLITag: api.CLIInfo{ID: "T", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: true, FromInline: false, KeyType: "", Type: "api.TypeMeta"},

			"ObjectMeta": api.Field{Name: "ObjectMeta", CLITag: api.CLIInfo{ID: "meta", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "meta", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.ObjectMeta"},

			"Spec": api.Field{Name: "Spec", CLITag: api.CLIInfo{ID: "spec", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "spec", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.AlertDestinationSpec"},

			"Status": api.Field{Name: "Status", CLITag: api.CLIInfo{ID: "status", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "status", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.AlertDestinationStatus"},

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
			"algo":                     api.CLIInfo{Path: "Spec.SNMPTrapServers[].PrivacyConfig.Algo", Skip: false, Insert: "", Help: ""},
			"api-version":              api.CLIInfo{Path: "APIVersion", Skip: false, Insert: "", Help: ""},
			"community-or-user":        api.CLIInfo{Path: "Spec.SNMPTrapServers[].CommunityOrUser", Skip: false, Insert: "", Help: ""},
			"email-list":               api.CLIInfo{Path: "Spec.EmailList", Skip: false, Insert: "", Help: ""},
			"format":                   api.CLIInfo{Path: "Spec.SyslogServers[].Format", Skip: false, Insert: "", Help: ""},
			"generation-id":            api.CLIInfo{Path: "GenerationID", Skip: false, Insert: "", Help: ""},
			"host":                     api.CLIInfo{Path: "Spec.SNMPTrapServers[].Host", Skip: false, Insert: "", Help: ""},
			"kind":                     api.CLIInfo{Path: "Kind", Skip: false, Insert: "", Help: ""},
			"labels":                   api.CLIInfo{Path: "Labels", Skip: false, Insert: "", Help: ""},
			"name":                     api.CLIInfo{Path: "Name", Skip: false, Insert: "", Help: ""},
			"namespace":                api.CLIInfo{Path: "Namespace", Skip: false, Insert: "", Help: ""},
			"password":                 api.CLIInfo{Path: "Spec.SNMPTrapServers[].PrivacyConfig.Password", Skip: false, Insert: "", Help: ""},
			"port":                     api.CLIInfo{Path: "Spec.SNMPTrapServers[].Port", Skip: false, Insert: "", Help: ""},
			"resource-version":         api.CLIInfo{Path: "ResourceVersion", Skip: false, Insert: "", Help: ""},
			"self-link":                api.CLIInfo{Path: "SelfLink", Skip: false, Insert: "", Help: ""},
			"tenant":                   api.CLIInfo{Path: "Tenant", Skip: false, Insert: "", Help: ""},
			"total-notifications-sent": api.CLIInfo{Path: "Status.totalNotificationsSent", Skip: false, Insert: "", Help: ""},
			"uuid":                     api.CLIInfo{Path: "UUID", Skip: false, Insert: "", Help: ""},
			"version":                  api.CLIInfo{Path: "Spec.SNMPTrapServers[].Version", Skip: false, Insert: "", Help: ""},
		},
	},
	"monitoring.AlertDestinationSpec": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(AlertDestinationSpec{}) },
		Fields: map[string]api.Field{
			"EmailList": api.Field{Name: "EmailList", CLITag: api.CLIInfo{ID: "email-list", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "email-list", Pointer: true, Slice: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"SNMPTrapServers": api.Field{Name: "SNMPTrapServers", CLITag: api.CLIInfo{ID: "snmp-trap-servers", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "snmp-trap-servers", Pointer: true, Slice: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.SNMPTrapServer"},

			"SyslogServers": api.Field{Name: "SyslogServers", CLITag: api.CLIInfo{ID: "syslog-servers", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "syslog-servers", Pointer: true, Slice: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.SyslogExport"},
		},
	},
	"monitoring.AlertDestinationStatus": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(AlertDestinationStatus{}) },
		Fields: map[string]api.Field{
			"totalNotificationsSent": api.Field{Name: "totalNotificationsSent", CLITag: api.CLIInfo{ID: "total-notifications-sent", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "total-notifications-sent", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_INT32"},
		},
	},
	"monitoring.AlertPolicy": &api.Struct{
		Kind: "AlertPolicy", APIGroup: "monitoring", Scopes: []string{"Tenant"}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(AlertPolicy{}) },
		Fields: map[string]api.Field{
			"TypeMeta": api.Field{Name: "TypeMeta", CLITag: api.CLIInfo{ID: "T", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: false, Slice: false, Map: false, Inline: true, FromInline: false, KeyType: "", Type: "api.TypeMeta"},

			"ObjectMeta": api.Field{Name: "ObjectMeta", CLITag: api.CLIInfo{ID: "meta", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "meta", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.ObjectMeta"},

			"Spec": api.Field{Name: "Spec", CLITag: api.CLIInfo{ID: "spec", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "spec", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.AlertPolicySpec"},

			"Status": api.Field{Name: "Status", CLITag: api.CLIInfo{ID: "status", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "status", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.AlertPolicyStatus"},

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
			"acknowledged-alerts":  api.CLIInfo{Path: "Status.AcknowledgedAlerts", Skip: false, Insert: "", Help: ""},
			"api-version":          api.CLIInfo{Path: "APIVersion", Skip: false, Insert: "", Help: ""},
			"auto-resolve":         api.CLIInfo{Path: "Spec.AutoResolve", Skip: false, Insert: "", Help: ""},
			"clear-duration":       api.CLIInfo{Path: "Spec.ClearDuration", Skip: false, Insert: "", Help: ""},
			"destinations":         api.CLIInfo{Path: "Spec.Destinations", Skip: false, Insert: "", Help: ""},
			"enable":               api.CLIInfo{Path: "Spec.Enable", Skip: false, Insert: "", Help: ""},
			"generation-id":        api.CLIInfo{Path: "GenerationID", Skip: false, Insert: "", Help: ""},
			"kind":                 api.CLIInfo{Path: "Kind", Skip: false, Insert: "", Help: ""},
			"labels":               api.CLIInfo{Path: "Labels", Skip: false, Insert: "", Help: ""},
			"message":              api.CLIInfo{Path: "Spec.Message", Skip: false, Insert: "", Help: ""},
			"name":                 api.CLIInfo{Path: "Name", Skip: false, Insert: "", Help: ""},
			"namespace":            api.CLIInfo{Path: "Namespace", Skip: false, Insert: "", Help: ""},
			"open-alerts":          api.CLIInfo{Path: "Status.OpenAlerts", Skip: false, Insert: "", Help: ""},
			"persistence-duration": api.CLIInfo{Path: "Spec.PersistenceDuration", Skip: false, Insert: "", Help: ""},
			"resource":             api.CLIInfo{Path: "Spec.Resource", Skip: false, Insert: "", Help: ""},
			"resource-version":     api.CLIInfo{Path: "ResourceVersion", Skip: false, Insert: "", Help: ""},
			"self-link":            api.CLIInfo{Path: "SelfLink", Skip: false, Insert: "", Help: ""},
			"severity":             api.CLIInfo{Path: "Spec.Severity", Skip: false, Insert: "", Help: ""},
			"tenant":               api.CLIInfo{Path: "Tenant", Skip: false, Insert: "", Help: ""},
			"total-hits":           api.CLIInfo{Path: "Status.TotalHits", Skip: false, Insert: "", Help: ""},
			"uuid":                 api.CLIInfo{Path: "UUID", Skip: false, Insert: "", Help: ""},
		},
	},
	"monitoring.AlertPolicySpec": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(AlertPolicySpec{}) },
		Fields: map[string]api.Field{
			"Resource": api.Field{Name: "Resource", CLITag: api.CLIInfo{ID: "resource", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "resource", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Severity": api.Field{Name: "Severity", CLITag: api.CLIInfo{ID: "severity", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "severity", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Message": api.Field{Name: "Message", CLITag: api.CLIInfo{ID: "message", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "message", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Requirements": api.Field{Name: "Requirements", CLITag: api.CLIInfo{ID: "requirements", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "requirements", Pointer: true, Slice: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "fields.Requirement"},

			"PersistenceDuration": api.Field{Name: "PersistenceDuration", CLITag: api.CLIInfo{ID: "persistence-duration", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "persistence-duration", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"ClearDuration": api.Field{Name: "ClearDuration", CLITag: api.CLIInfo{ID: "clear-duration", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "clear-duration", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Enable": api.Field{Name: "Enable", CLITag: api.CLIInfo{ID: "enable", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "enable", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_BOOL"},

			"AutoResolve": api.Field{Name: "AutoResolve", CLITag: api.CLIInfo{ID: "auto-resolve", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "auto-resolve", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_BOOL"},

			"Destinations": api.Field{Name: "Destinations", CLITag: api.CLIInfo{ID: "destinations", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "destinations", Pointer: false, Slice: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"monitoring.AlertPolicyStatus": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(AlertPolicyStatus{}) },
		Fields: map[string]api.Field{
			"TotalHits": api.Field{Name: "TotalHits", CLITag: api.CLIInfo{ID: "total-hits", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "total-hits", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_INT32"},

			"OpenAlerts": api.Field{Name: "OpenAlerts", CLITag: api.CLIInfo{ID: "open-alerts", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "open-alerts", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_INT32"},

			"AcknowledgedAlerts": api.Field{Name: "AcknowledgedAlerts", CLITag: api.CLIInfo{ID: "acknowledged-alerts", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "acknowledged-alerts", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_INT32"},
		},
	},
	"monitoring.AlertReason": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(AlertReason{}) },
		Fields: map[string]api.Field{
			"MatchedRequirements": api.Field{Name: "MatchedRequirements", CLITag: api.CLIInfo{ID: "matched-requirements", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "matched-requirements", Pointer: true, Slice: true, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.MatchedRequirement"},

			"PolicyID": api.Field{Name: "PolicyID", CLITag: api.CLIInfo{ID: "alert-policy-id", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "alert-policy-id", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"monitoring.AlertSource": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(AlertSource{}) },
		Fields: map[string]api.Field{
			"Component": api.Field{Name: "Component", CLITag: api.CLIInfo{ID: "component", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "component", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"NodeName": api.Field{Name: "NodeName", CLITag: api.CLIInfo{ID: "node-name", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "node-name", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"monitoring.AlertSpec": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(AlertSpec{}) },
		Fields: map[string]api.Field{
			"State": api.Field{Name: "State", CLITag: api.CLIInfo{ID: "state", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "state", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"monitoring.AlertStatus": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(AlertStatus{}) },
		Fields: map[string]api.Field{
			"Severity": api.Field{Name: "Severity", CLITag: api.CLIInfo{ID: "severity", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "severity", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Source": api.Field{Name: "Source", CLITag: api.CLIInfo{ID: "source", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "source", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.AlertSource"},

			"EventURI": api.Field{Name: "EventURI", CLITag: api.CLIInfo{ID: "event-uri", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "event-uri", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"ObjectRef": api.Field{Name: "ObjectRef", CLITag: api.CLIInfo{ID: "object-ref", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "object-ref", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.ObjectRef"},

			"Message": api.Field{Name: "Message", CLITag: api.CLIInfo{ID: "message", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "message", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Reason": api.Field{Name: "Reason", CLITag: api.CLIInfo{ID: "reason", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "reason", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.AlertReason"},

			"Acknowledged": api.Field{Name: "Acknowledged", CLITag: api.CLIInfo{ID: "acknowledged", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "acknowledged", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.AuditInfo"},

			"Resolved": api.Field{Name: "Resolved", CLITag: api.CLIInfo{ID: "resolved", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "resolved", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.AuditInfo"},
		},
	},
	"monitoring.AuditInfo": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(AuditInfo{}) },
		Fields: map[string]api.Field{
			"User": api.Field{Name: "User", CLITag: api.CLIInfo{ID: "user", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "user", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Time": api.Field{Name: "Time", CLITag: api.CLIInfo{ID: "time", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "time", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "api.Timestamp"},
		},
	},
	"monitoring.AuthConfig": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(AuthConfig{}) },
		Fields: map[string]api.Field{
			"Algo": api.Field{Name: "Algo", CLITag: api.CLIInfo{ID: "algo", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "algo", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Password": api.Field{Name: "Password", CLITag: api.CLIInfo{ID: "password", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "password", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"monitoring.MatchedRequirement": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(MatchedRequirement{}) },
		Fields: map[string]api.Field{
			"Requirement": api.Field{Name: "Requirement", CLITag: api.CLIInfo{ID: "Requirement", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "", Pointer: true, Slice: false, Map: false, Inline: true, FromInline: false, KeyType: "", Type: "fields.Requirement"},

			"ObservedValue": api.Field{Name: "ObservedValue", CLITag: api.CLIInfo{ID: "observed-value", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "observed-value", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Key": api.Field{Name: "Key", CLITag: api.CLIInfo{ID: "key", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "key", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Operator": api.Field{Name: "Operator", CLITag: api.CLIInfo{ID: "operator", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "operator", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},

			"Values": api.Field{Name: "Values", CLITag: api.CLIInfo{ID: "values", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "values", Pointer: false, Slice: true, Map: false, Inline: false, FromInline: true, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"monitoring.PrivacyConfig": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(PrivacyConfig{}) },
		Fields: map[string]api.Field{
			"Algo": api.Field{Name: "Algo", CLITag: api.CLIInfo{ID: "algo", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "algo", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Password": api.Field{Name: "Password", CLITag: api.CLIInfo{ID: "password", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "password", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},
		},
	},
	"monitoring.SNMPTrapServer": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(SNMPTrapServer{}) },
		Fields: map[string]api.Field{
			"Host": api.Field{Name: "Host", CLITag: api.CLIInfo{ID: "host", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "host", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Port": api.Field{Name: "Port", CLITag: api.CLIInfo{ID: "port", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "port", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Version": api.Field{Name: "Version", CLITag: api.CLIInfo{ID: "version", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "version", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"CommunityOrUser": api.Field{Name: "CommunityOrUser", CLITag: api.CLIInfo{ID: "community-or-user", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "community-or-user", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"AuthConfig": api.Field{Name: "AuthConfig", CLITag: api.CLIInfo{ID: "auth-config", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "auth-config", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.AuthConfig"},

			"PrivacyConfig": api.Field{Name: "PrivacyConfig", CLITag: api.CLIInfo{ID: "privacy-config", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "privacy-config", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.PrivacyConfig"},
		},
	},
	"monitoring.SyslogExport": &api.Struct{
		Kind: "", APIGroup: "", Scopes: []string{}, GetTypeFn: func() reflect.Type { return reflect.TypeOf(SyslogExport{}) },
		Fields: map[string]api.Field{
			"Format": api.Field{Name: "Format", CLITag: api.CLIInfo{ID: "format", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "format", Pointer: false, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "TYPE_STRING"},

			"Target": api.Field{Name: "Target", CLITag: api.CLIInfo{ID: "target", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "target", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.ExportConfig"},

			"Config": api.Field{Name: "Config", CLITag: api.CLIInfo{ID: "config", Path: "", Skip: false, Insert: "", Help: ""}, JSONTag: "config", Pointer: true, Slice: false, Map: false, Inline: false, FromInline: false, KeyType: "", Type: "monitoring.SyslogExportConfig"},
		},
	},
}

func init() {
	schema := runtime.GetDefaultScheme()
	schema.AddSchema(typesMapAlerts)
}
