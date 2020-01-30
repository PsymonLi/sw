// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: eventtypes.proto

/*
Package eventtypes is a generated protocol buffer package.

It is generated from these files:
	eventtypes.proto

It has these top-level messages:
*/
package eventtypes

import proto "github.com/gogo/protobuf/proto"
import fmt "fmt"
import math "math"
import gogoproto "github.com/gogo/protobuf/gogoproto"
import google_protobuf "github.com/gogo/protobuf/protoc-gen-gogo/descriptor"
import eventattrs "github.com/pensando/sw/events/generated/eventattrs"

import strconv "strconv"

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

// This is a compile-time assertion to ensure that this generated file
// is compatible with the proto package it is being compiled against.
// A compilation error at this line likely means your copy of the
// proto package needs to be updated.
const _ = proto.GoGoProtoPackageIsVersion2 // please upgrade the proto package

// goproto_enum_prefix from public import gogo.proto
var E_GoprotoEnumPrefix = gogoproto.E_GoprotoEnumPrefix

// goproto_enum_stringer from public import gogo.proto
var E_GoprotoEnumStringer = gogoproto.E_GoprotoEnumStringer

// enum_stringer from public import gogo.proto
var E_EnumStringer = gogoproto.E_EnumStringer

// enum_customname from public import gogo.proto
var E_EnumCustomname = gogoproto.E_EnumCustomname

// enumdecl from public import gogo.proto
var E_Enumdecl = gogoproto.E_Enumdecl

// enumvalue_customname from public import gogo.proto
var E_EnumvalueCustomname = gogoproto.E_EnumvalueCustomname

// goproto_getters_all from public import gogo.proto
var E_GoprotoGettersAll = gogoproto.E_GoprotoGettersAll

// goproto_enum_prefix_all from public import gogo.proto
var E_GoprotoEnumPrefixAll = gogoproto.E_GoprotoEnumPrefixAll

// goproto_stringer_all from public import gogo.proto
var E_GoprotoStringerAll = gogoproto.E_GoprotoStringerAll

// verbose_equal_all from public import gogo.proto
var E_VerboseEqualAll = gogoproto.E_VerboseEqualAll

// face_all from public import gogo.proto
var E_FaceAll = gogoproto.E_FaceAll

// gostring_all from public import gogo.proto
var E_GostringAll = gogoproto.E_GostringAll

// populate_all from public import gogo.proto
var E_PopulateAll = gogoproto.E_PopulateAll

// stringer_all from public import gogo.proto
var E_StringerAll = gogoproto.E_StringerAll

// onlyone_all from public import gogo.proto
var E_OnlyoneAll = gogoproto.E_OnlyoneAll

// equal_all from public import gogo.proto
var E_EqualAll = gogoproto.E_EqualAll

// description_all from public import gogo.proto
var E_DescriptionAll = gogoproto.E_DescriptionAll

// testgen_all from public import gogo.proto
var E_TestgenAll = gogoproto.E_TestgenAll

// benchgen_all from public import gogo.proto
var E_BenchgenAll = gogoproto.E_BenchgenAll

// marshaler_all from public import gogo.proto
var E_MarshalerAll = gogoproto.E_MarshalerAll

// unmarshaler_all from public import gogo.proto
var E_UnmarshalerAll = gogoproto.E_UnmarshalerAll

// stable_marshaler_all from public import gogo.proto
var E_StableMarshalerAll = gogoproto.E_StableMarshalerAll

// sizer_all from public import gogo.proto
var E_SizerAll = gogoproto.E_SizerAll

// goproto_enum_stringer_all from public import gogo.proto
var E_GoprotoEnumStringerAll = gogoproto.E_GoprotoEnumStringerAll

// enum_stringer_all from public import gogo.proto
var E_EnumStringerAll = gogoproto.E_EnumStringerAll

// unsafe_marshaler_all from public import gogo.proto
var E_UnsafeMarshalerAll = gogoproto.E_UnsafeMarshalerAll

// unsafe_unmarshaler_all from public import gogo.proto
var E_UnsafeUnmarshalerAll = gogoproto.E_UnsafeUnmarshalerAll

// goproto_extensions_map_all from public import gogo.proto
var E_GoprotoExtensionsMapAll = gogoproto.E_GoprotoExtensionsMapAll

// goproto_unrecognized_all from public import gogo.proto
var E_GoprotoUnrecognizedAll = gogoproto.E_GoprotoUnrecognizedAll

// gogoproto_import from public import gogo.proto
var E_GogoprotoImport = gogoproto.E_GogoprotoImport

// protosizer_all from public import gogo.proto
var E_ProtosizerAll = gogoproto.E_ProtosizerAll

// compare_all from public import gogo.proto
var E_CompareAll = gogoproto.E_CompareAll

// typedecl_all from public import gogo.proto
var E_TypedeclAll = gogoproto.E_TypedeclAll

// enumdecl_all from public import gogo.proto
var E_EnumdeclAll = gogoproto.E_EnumdeclAll

// goproto_registration from public import gogo.proto
var E_GoprotoRegistration = gogoproto.E_GoprotoRegistration

// goproto_getters from public import gogo.proto
var E_GoprotoGetters = gogoproto.E_GoprotoGetters

// goproto_stringer from public import gogo.proto
var E_GoprotoStringer = gogoproto.E_GoprotoStringer

// verbose_equal from public import gogo.proto
var E_VerboseEqual = gogoproto.E_VerboseEqual

// face from public import gogo.proto
var E_Face = gogoproto.E_Face

// gostring from public import gogo.proto
var E_Gostring = gogoproto.E_Gostring

// populate from public import gogo.proto
var E_Populate = gogoproto.E_Populate

// stringer from public import gogo.proto
var E_Stringer = gogoproto.E_Stringer

// onlyone from public import gogo.proto
var E_Onlyone = gogoproto.E_Onlyone

// equal from public import gogo.proto
var E_Equal = gogoproto.E_Equal

// description from public import gogo.proto
var E_Description = gogoproto.E_Description

// testgen from public import gogo.proto
var E_Testgen = gogoproto.E_Testgen

// benchgen from public import gogo.proto
var E_Benchgen = gogoproto.E_Benchgen

// marshaler from public import gogo.proto
var E_Marshaler = gogoproto.E_Marshaler

// unmarshaler from public import gogo.proto
var E_Unmarshaler = gogoproto.E_Unmarshaler

// stable_marshaler from public import gogo.proto
var E_StableMarshaler = gogoproto.E_StableMarshaler

// sizer from public import gogo.proto
var E_Sizer = gogoproto.E_Sizer

// unsafe_marshaler from public import gogo.proto
var E_UnsafeMarshaler = gogoproto.E_UnsafeMarshaler

// unsafe_unmarshaler from public import gogo.proto
var E_UnsafeUnmarshaler = gogoproto.E_UnsafeUnmarshaler

// goproto_extensions_map from public import gogo.proto
var E_GoprotoExtensionsMap = gogoproto.E_GoprotoExtensionsMap

// goproto_unrecognized from public import gogo.proto
var E_GoprotoUnrecognized = gogoproto.E_GoprotoUnrecognized

// protosizer from public import gogo.proto
var E_Protosizer = gogoproto.E_Protosizer

// compare from public import gogo.proto
var E_Compare = gogoproto.E_Compare

// typedecl from public import gogo.proto
var E_Typedecl = gogoproto.E_Typedecl

// nullable from public import gogo.proto
var E_Nullable = gogoproto.E_Nullable

// embed from public import gogo.proto
var E_Embed = gogoproto.E_Embed

// customtype from public import gogo.proto
var E_Customtype = gogoproto.E_Customtype

// customname from public import gogo.proto
var E_Customname = gogoproto.E_Customname

// jsontag from public import gogo.proto
var E_Jsontag = gogoproto.E_Jsontag

// moretags from public import gogo.proto
var E_Moretags = gogoproto.E_Moretags

// casttype from public import gogo.proto
var E_Casttype = gogoproto.E_Casttype

// castkey from public import gogo.proto
var E_Castkey = gogoproto.E_Castkey

// castvalue from public import gogo.proto
var E_Castvalue = gogoproto.E_Castvalue

// stdtime from public import gogo.proto
var E_Stdtime = gogoproto.E_Stdtime

// stdduration from public import gogo.proto
var E_Stdduration = gogoproto.E_Stdduration

//
type EventType int32

const (
	// ----------------------------- Cluster events -------------------------- //
	ELECTION_STARTED EventType = 0
	//
	ELECTION_CANCELLED EventType = 1
	//
	ELECTION_NOTIFICATION_FAILED EventType = 2
	//
	ELECTION_STOPPED EventType = 3
	//
	LEADER_ELECTED EventType = 4
	//
	LEADER_LOST EventType = 5
	//
	LEADER_CHANGED EventType = 6
	// ------------------------------- Node events ----------------------------- //
	NODE_JOINED EventType = 7
	//
	NODE_DISJOINED EventType = 8
	//
	NODE_HEALTHY EventType = 9
	//
	NODE_UNREACHABLE EventType = 10
	//
	CLOCK_SYNC_FAILED EventType = 11
	// ------------------------------- Quorum events ----------------------------- //
	QUORUM_MEMBER_ADD EventType = 15
	//
	QUORUM_MEMBER_REMOVE EventType = 16
	//
	QUORUM_MEMBER_HEALTHY EventType = 17
	//
	QUORUM_MEMBER_UNHEALTHY EventType = 18
	//
	UNSUPPORTED_QUORUM_SIZE EventType = 19
	//
	QUORUM_UNHEALTHY EventType = 20
	// ------------------------------- Diagnostic events ----------------------------- //
	MODULE_CREATION_FAILED EventType = 24
	// ------------------------ Config Snapshot/Restore events ------------------------ //
	CONFIG_RESTORED EventType = 30
	//
	CONFIG_RESTORE_ABORTED EventType = 31
	//
	CONFIG_RESTORE_FAILED EventType = 32
	// -------------------------------- Host/DSC events -------------------------- //
	DSC_ADMITTED EventType = 100
	//
	DSC_REJECTED EventType = 101
	//
	DSC_UNREACHABLE EventType = 102
	//
	DSC_HEALTHY EventType = 103
	//
	DSC_UNHEALTHY EventType = 104
	//
	HOST_DSC_SPEC_CONFLICT EventType = 105
	//
	DSC_DEADMITTED EventType = 106
	//
	DSC_DECOMMISSIONED EventType = 107
	// ----------------------------- API Gateway events ---------------------- //
	AUTO_GENERATED_TLS_CERT EventType = 200
	// --------------------------- Auth/Audit events ------------------------- //
	LOGIN_FAILED EventType = 300
	//
	AUDITING_FAILED EventType = 301
	//
	PASSWORD_CHANGED EventType = 302
	//
	PASSWORD_RESET EventType = 303
	// --------------------------- HAL/Link events --------------------------- //
	LINK_UP EventType = 400
	//
	LINK_DOWN EventType = 401
	// ------------------------------ System events -------------------------- //
	SERVICE_STARTED EventType = 500
	//
	SERVICE_STOPPED EventType = 501
	//
	NAPLES_SERVICE_STOPPED EventType = 502
	//
	SERVICE_PENDING EventType = 503
	//
	SERVICE_RUNNING EventType = 504
	//
	SERVICE_UNRESPONSIVE EventType = 505
	//
	SYSTEM_COLDBOOT EventType = 506
	//
	SYSTEM_RESOURCE_USAGE EventType = 507
	//
	NAPLES_FATAL_INTERRUPT EventType = 508
	//
	NAPLES_CATTRIP_INTERRUPT EventType = 509
	//
	NAPLES_OVER_TEMP EventType = 510
	//
	NAPLES_OVER_TEMP_EXIT EventType = 511
	//
	NAPLES_PANIC_EVENT EventType = 512
	//
	NAPLES_POST_DIAG_FAILURE_EVENT EventType = 513
	// ------------------------------ Resource events -------------------------- //
	DISK_THRESHOLD_EXCEEDED EventType = 601
	// ------------------------------ Rollout events -------------------------- //
	ROLLOUT_STARTED EventType = 701
	//
	ROLLOUT_SUCCESS EventType = 702
	//
	ROLLOUT_FAILED EventType = 703
	//
	ROLLOUT_SUSPENDED EventType = 704
	// ------------------------------ Config events -------------------------- //
	CONFIG_FAIL EventType = 801
)

var EventType_name = map[int32]string{
	0:   "ELECTION_STARTED",
	1:   "ELECTION_CANCELLED",
	2:   "ELECTION_NOTIFICATION_FAILED",
	3:   "ELECTION_STOPPED",
	4:   "LEADER_ELECTED",
	5:   "LEADER_LOST",
	6:   "LEADER_CHANGED",
	7:   "NODE_JOINED",
	8:   "NODE_DISJOINED",
	9:   "NODE_HEALTHY",
	10:  "NODE_UNREACHABLE",
	11:  "CLOCK_SYNC_FAILED",
	15:  "QUORUM_MEMBER_ADD",
	16:  "QUORUM_MEMBER_REMOVE",
	17:  "QUORUM_MEMBER_HEALTHY",
	18:  "QUORUM_MEMBER_UNHEALTHY",
	19:  "UNSUPPORTED_QUORUM_SIZE",
	20:  "QUORUM_UNHEALTHY",
	24:  "MODULE_CREATION_FAILED",
	30:  "CONFIG_RESTORED",
	31:  "CONFIG_RESTORE_ABORTED",
	32:  "CONFIG_RESTORE_FAILED",
	100: "DSC_ADMITTED",
	101: "DSC_REJECTED",
	102: "DSC_UNREACHABLE",
	103: "DSC_HEALTHY",
	104: "DSC_UNHEALTHY",
	105: "HOST_DSC_SPEC_CONFLICT",
	106: "DSC_DEADMITTED",
	107: "DSC_DECOMMISSIONED",
	200: "AUTO_GENERATED_TLS_CERT",
	300: "LOGIN_FAILED",
	301: "AUDITING_FAILED",
	302: "PASSWORD_CHANGED",
	303: "PASSWORD_RESET",
	400: "LINK_UP",
	401: "LINK_DOWN",
	500: "SERVICE_STARTED",
	501: "SERVICE_STOPPED",
	502: "NAPLES_SERVICE_STOPPED",
	503: "SERVICE_PENDING",
	504: "SERVICE_RUNNING",
	505: "SERVICE_UNRESPONSIVE",
	506: "SYSTEM_COLDBOOT",
	507: "SYSTEM_RESOURCE_USAGE",
	508: "NAPLES_FATAL_INTERRUPT",
	509: "NAPLES_CATTRIP_INTERRUPT",
	510: "NAPLES_OVER_TEMP",
	511: "NAPLES_OVER_TEMP_EXIT",
	512: "NAPLES_PANIC_EVENT",
	513: "NAPLES_POST_DIAG_FAILURE_EVENT",
	601: "DISK_THRESHOLD_EXCEEDED",
	701: "ROLLOUT_STARTED",
	702: "ROLLOUT_SUCCESS",
	703: "ROLLOUT_FAILED",
	704: "ROLLOUT_SUSPENDED",
	801: "CONFIG_FAIL",
}
var EventType_value = map[string]int32{
	"ELECTION_STARTED":               0,
	"ELECTION_CANCELLED":             1,
	"ELECTION_NOTIFICATION_FAILED":   2,
	"ELECTION_STOPPED":               3,
	"LEADER_ELECTED":                 4,
	"LEADER_LOST":                    5,
	"LEADER_CHANGED":                 6,
	"NODE_JOINED":                    7,
	"NODE_DISJOINED":                 8,
	"NODE_HEALTHY":                   9,
	"NODE_UNREACHABLE":               10,
	"CLOCK_SYNC_FAILED":              11,
	"QUORUM_MEMBER_ADD":              15,
	"QUORUM_MEMBER_REMOVE":           16,
	"QUORUM_MEMBER_HEALTHY":          17,
	"QUORUM_MEMBER_UNHEALTHY":        18,
	"UNSUPPORTED_QUORUM_SIZE":        19,
	"QUORUM_UNHEALTHY":               20,
	"MODULE_CREATION_FAILED":         24,
	"CONFIG_RESTORED":                30,
	"CONFIG_RESTORE_ABORTED":         31,
	"CONFIG_RESTORE_FAILED":          32,
	"DSC_ADMITTED":                   100,
	"DSC_REJECTED":                   101,
	"DSC_UNREACHABLE":                102,
	"DSC_HEALTHY":                    103,
	"DSC_UNHEALTHY":                  104,
	"HOST_DSC_SPEC_CONFLICT":         105,
	"DSC_DEADMITTED":                 106,
	"DSC_DECOMMISSIONED":             107,
	"AUTO_GENERATED_TLS_CERT":        200,
	"LOGIN_FAILED":                   300,
	"AUDITING_FAILED":                301,
	"PASSWORD_CHANGED":               302,
	"PASSWORD_RESET":                 303,
	"LINK_UP":                        400,
	"LINK_DOWN":                      401,
	"SERVICE_STARTED":                500,
	"SERVICE_STOPPED":                501,
	"NAPLES_SERVICE_STOPPED":         502,
	"SERVICE_PENDING":                503,
	"SERVICE_RUNNING":                504,
	"SERVICE_UNRESPONSIVE":           505,
	"SYSTEM_COLDBOOT":                506,
	"SYSTEM_RESOURCE_USAGE":          507,
	"NAPLES_FATAL_INTERRUPT":         508,
	"NAPLES_CATTRIP_INTERRUPT":       509,
	"NAPLES_OVER_TEMP":               510,
	"NAPLES_OVER_TEMP_EXIT":          511,
	"NAPLES_PANIC_EVENT":             512,
	"NAPLES_POST_DIAG_FAILURE_EVENT": 513,
	"DISK_THRESHOLD_EXCEEDED":        601,
	"ROLLOUT_STARTED":                701,
	"ROLLOUT_SUCCESS":                702,
	"ROLLOUT_FAILED":                 703,
	"ROLLOUT_SUSPENDED":              704,
	"CONFIG_FAIL":                    801,
}

func (EventType) EnumDescriptor() ([]byte, []int) { return fileDescriptorEventtypes, []int{0} }

var E_Severity = &proto.ExtensionDesc{
	ExtendedType:  (*google_protobuf.EnumValueOptions)(nil),
	ExtensionType: (*eventattrs.Severity)(nil),
	Field:         10005,
	Name:          "eventtypes.severity",
	Tag:           "varint,10005,opt,name=severity,enum=eventattrs.Severity",
	Filename:      "eventtypes.proto",
}

var E_Category = &proto.ExtensionDesc{
	ExtendedType:  (*google_protobuf.EnumValueOptions)(nil),
	ExtensionType: (*eventattrs.Category)(nil),
	Field:         10006,
	Name:          "eventtypes.category",
	Tag:           "varint,10006,opt,name=category,enum=eventattrs.Category",
	Filename:      "eventtypes.proto",
}

var E_Desc = &proto.ExtensionDesc{
	ExtendedType:  (*google_protobuf.EnumValueOptions)(nil),
	ExtensionType: (*string)(nil),
	Field:         10007,
	Name:          "eventtypes.desc",
	Tag:           "bytes,10007,opt,name=desc",
	Filename:      "eventtypes.proto",
}

var E_SuppressMm = &proto.ExtensionDesc{
	ExtendedType:  (*google_protobuf.EnumValueOptions)(nil),
	ExtensionType: (*bool)(nil),
	Field:         10008,
	Name:          "eventtypes.suppress_mm",
	Tag:           "varint,10008,opt,name=suppress_mm,json=suppressMm",
	Filename:      "eventtypes.proto",
}

func init() {
	proto.RegisterEnum("eventtypes.EventType", EventType_name, EventType_value)
	proto.RegisterExtension(E_Severity)
	proto.RegisterExtension(E_Category)
	proto.RegisterExtension(E_Desc)
	proto.RegisterExtension(E_SuppressMm)
}
func (x EventType) String() string {
	s, ok := EventType_name[int32(x)]
	if ok {
		return s
	}
	return strconv.Itoa(int(x))
}

func init() { proto.RegisterFile("eventtypes.proto", fileDescriptorEventtypes) }

var fileDescriptorEventtypes = []byte{
	// 2111 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x8c, 0x97, 0x4d, 0x6c, 0xdc, 0xc6,
	0x15, 0xc7, 0xb5, 0xda, 0x75, 0x12, 0x8f, 0x5c, 0x89, 0x66, 0x6c, 0xc9, 0x61, 0x9c, 0xf5, 0x38,
	0x01, 0x5c, 0x37, 0x6e, 0xa4, 0xc4, 0x6e, 0x12, 0xd7, 0x6d, 0x50, 0x53, 0xe4, 0x68, 0xc5, 0x98,
	0x4b, 0xae, 0x49, 0xae, 0x1c, 0x37, 0x28, 0x08, 0x2e, 0x39, 0xbb, 0x4b, 0x9b, 0xcb, 0xd9, 0x70,
	0x48, 0xa9, 0xea, 0xa9, 0x45, 0x2f, 0x3d, 0xb6, 0x87, 0x7e, 0x5c, 0x8b, 0xfa, 0xe0, 0x43, 0x3f,
	0x7c, 0x36, 0xd0, 0x36, 0xc7, 0x1c, 0x7b, 0xed, 0xad, 0x30, 0x50, 0xa0, 0x57, 0x01, 0xfd, 0xfe,
	0xc6, 0x0c, 0x87, 0x2b, 0xae, 0xac, 0xb4, 0x3e, 0x4a, 0xd8, 0xff, 0x6f, 0xde, 0x7b, 0xf3, 0xe6,
	0xff, 0x1e, 0x81, 0x84, 0x77, 0x71, 0x9a, 0xe7, 0xfb, 0x53, 0x4c, 0xd7, 0xa7, 0x19, 0xc9, 0x89,
	0x0c, 0x0e, 0xff, 0xa3, 0x80, 0x11, 0x19, 0x91, 0xf2, 0xff, 0x0a, 0x1c, 0x11, 0x32, 0x4a, 0xf0,
	0x06, 0xff, 0x6b, 0x50, 0x0c, 0x37, 0x22, 0x4c, 0xc3, 0x2c, 0x9e, 0xe6, 0x24, 0x13, 0xbf, 0x90,
	0x82, 0x3c, 0xcf, 0xe2, 0x41, 0x91, 0x57, 0xac, 0xd7, 0x7f, 0xdf, 0x06, 0x00, 0x31, 0x9c, 0xc7,
	0x70, 0xb2, 0x09, 0x24, 0x64, 0x22, 0xcd, 0x33, 0x6c, 0xcb, 0x77, 0x3d, 0xd5, 0xf1, 0x90, 0x2e,
	0x2d, 0x28, 0xef, 0x3c, 0x3c, 0x68, 0x2d, 0x3c, 0x3a, 0x68, 0x2d, 0x3c, 0x3e, 0x68, 0x5d, 0x32,
	0x71, 0x10, 0xe1, 0x0c, 0xe2, 0x04, 0x87, 0x79, 0x4c, 0x52, 0x48, 0xf3, 0x20, 0xcb, 0x71, 0x04,
	0xe3, 0x14, 0xe6, 0x63, 0x0c, 0xc3, 0xa4, 0xa0, 0x39, 0xce, 0x3e, 0x3e, 0x68, 0x35, 0x64, 0x15,
	0xc8, 0x33, 0x9a, 0xa6, 0x5a, 0x1a, 0x32, 0x4d, 0xa4, 0x4b, 0x0d, 0xe5, 0x73, 0x0f, 0x0f, 0x5a,
	0x0d, 0xc1, 0x7b, 0xe9, 0x28, 0x2f, 0x0c, 0xd2, 0x10, 0x27, 0x09, 0x8e, 0x38, 0xe2, 0x6b, 0xe0,
	0xfc, 0x0c, 0x61, 0xd9, 0x9e, 0xb1, 0x65, 0x68, 0x2a, 0xff, 0x63, 0x4b, 0x35, 0x18, 0x6c, 0x51,
	0xf9, 0x52, 0x0d, 0x76, 0x65, 0x2b, 0x88, 0x13, 0x1c, 0xc1, 0x9c, 0x40, 0x8a, 0xd3, 0x08, 0x26,
	0x47, 0xd8, 0x29, 0xc9, 0xe3, 0x61, 0x1c, 0x06, 0xec, 0x0f, 0x8e, 0x7f, 0x6f, 0x2e, 0x5f, 0xbb,
	0xd7, 0x43, 0xba, 0xd4, 0x54, 0x3e, 0x5b, 0x43, 0xae, 0x3d, 0x9d, 0x2f, 0x99, 0x4e, 0x45, 0x74,
	0x9b, 0x60, 0xd9, 0x44, 0xaa, 0x8e, 0x1c, 0x9f, 0x53, 0x90, 0x2e, 0xb5, 0x94, 0xf5, 0x5a, 0xb1,
	0xda, 0x75, 0x31, 0x8e, 0xe0, 0x90, 0x64, 0x4f, 0x15, 0xc9, 0x00, 0x4b, 0x82, 0x61, 0xda, 0xae,
	0x27, 0x9d, 0x50, 0xae, 0xd7, 0x00, 0x97, 0x2d, 0x12, 0x61, 0x98, 0x10, 0x9a, 0x8b, 0x5c, 0xe8,
	0x38, 0x9e, 0xc2, 0xa8, 0xc8, 0xe2, 0x74, 0xc4, 0x49, 0x55, 0x58, 0x47, 0xc2, 0xd1, 0xb6, 0x55,
	0xab, 0x83, 0x74, 0xe9, 0xb9, 0x63, 0xc3, 0x09, 0xc7, 0x41, 0x3a, 0x3a, 0xbc, 0xb2, 0x39, 0xc6,
	0x75, 0xb0, 0x64, 0xd9, 0x3a, 0xf2, 0xdf, 0xb7, 0x0d, 0x0b, 0xe9, 0xd2, 0xf3, 0xbc, 0x18, 0x15,
	0x60, 0x8d, 0x87, 0x73, 0x8f, 0xc4, 0x29, 0x2b, 0xf2, 0x91, 0x44, 0xbe, 0x02, 0x96, 0xb9, 0x52,
	0x37, 0x5c, 0x21, 0x7e, 0x41, 0xb9, 0x52, 0xab, 0xe4, 0x05, 0x2e, 0x8e, 0x62, 0x2a, 0xf4, 0xc3,
	0x8c, 0x4c, 0xea, 0x10, 0xf9, 0x2d, 0x70, 0x8a, 0x03, 0xb6, 0x91, 0x6a, 0x7a, 0xdb, 0x77, 0xa5,
	0x93, 0xca, 0x85, 0xda, 0xd9, 0x2b, 0x5c, 0x1e, 0x53, 0x38, 0xc6, 0x41, 0x92, 0x8f, 0xf7, 0xf9,
	0x99, 0x5f, 0x04, 0x12, 0x97, 0xf4, 0x2d, 0x07, 0xa9, 0xda, 0xb6, 0xba, 0x69, 0x22, 0x09, 0x28,
	0xaf, 0x3d, 0x3c, 0x68, 0x2d, 0x0a, 0xd9, 0x8b, 0x95, 0xac, 0x48, 0x33, 0x1c, 0x84, 0xe3, 0x60,
	0x90, 0x60, 0x2e, 0xed, 0x80, 0xd3, 0x9a, 0x69, 0x6b, 0xb7, 0x7c, 0xf7, 0xae, 0xa5, 0x55, 0xed,
	0xb4, 0xa4, 0xbc, 0x59, 0x8b, 0x18, 0x72, 0xed, 0xf0, 0xb0, 0xa7, 0xf6, 0xd3, 0x70, 0x9c, 0x91,
	0x34, 0xfe, 0x06, 0x0b, 0x9a, 0x84, 0xf7, 0x45, 0x0f, 0x9d, 0xbe, 0xdd, 0xb7, 0x9d, 0x7e, 0xd7,
	0xef, 0xa2, 0xee, 0x26, 0x72, 0x7c, 0x55, 0xd7, 0xa5, 0x15, 0xe5, 0x52, 0x2d, 0xf6, 0x55, 0x35,
	0x8a, 0x70, 0x04, 0x27, 0x78, 0x32, 0xc0, 0x19, 0x23, 0x7d, 0x54, 0x90, 0xac, 0x98, 0x70, 0xb9,
	0x0e, 0xce, 0xcc, 0xcb, 0x1d, 0xd4, 0xb5, 0x77, 0x90, 0x24, 0x29, 0xaf, 0xd7, 0x08, 0x8a, 0x83,
	0x27, 0x64, 0xf7, 0x90, 0xc1, 0x8b, 0x57, 0xa3, 0x74, 0xc0, 0xd9, 0x79, 0x4a, 0x55, 0xc4, 0xd3,
	0xca, 0xe7, 0x6b, 0x98, 0xf3, 0xb7, 0xb9, 0xa4, 0xa2, 0xc4, 0x14, 0xa6, 0x64, 0x6f, 0xae, 0xa2,
	0xb7, 0xc0, 0xda, 0x3c, 0xa8, 0x6f, 0x55, 0x28, 0x99, 0x37, 0x53, 0x55, 0xd8, 0xf6, 0xb1, 0xa8,
	0x22, 0xad, 0xc3, 0x5c, 0xb0, 0xd6, 0xb7, 0xdc, 0x7e, 0xaf, 0x67, 0x33, 0x27, 0xf1, 0x05, 0xd8,
	0x35, 0xbe, 0x8a, 0xa4, 0x17, 0xb9, 0xab, 0x54, 0xb0, 0x4b, 0x02, 0x46, 0x59, 0x69, 0x63, 0x0a,
	0x07, 0x38, 0x21, 0x7b, 0x90, 0x16, 0xd3, 0x29, 0xe1, 0xe6, 0x32, 0x89, 0xd3, 0x78, 0x22, 0x52,
	0xbd, 0x0d, 0x24, 0x01, 0x3a, 0x0c, 0xed, 0x0c, 0xb7, 0x81, 0x8a, 0x76, 0x45, 0xd0, 0x22, 0x82,
	0x59, 0x54, 0x39, 0x1c, 0x07, 0xbb, 0x18, 0xe2, 0x94, 0x14, 0xa3, 0x71, 0x95, 0xac, 0x88, 0x9b,
	0x72, 0xa4, 0x03, 0x56, 0xbb, 0xb6, 0xde, 0x37, 0x91, 0xaf, 0x39, 0x68, 0xce, 0x5f, 0xce, 0xf1,
	0x30, 0xab, 0x86, 0xb8, 0xd4, 0x25, 0x51, 0x91, 0x60, 0x18, 0x66, 0x98, 0x7b, 0x08, 0x7f, 0xd0,
	0x51, 0x1c, 0x8c, 0x52, 0x42, 0xf3, 0x38, 0xa4, 0xa2, 0x57, 0xc4, 0x73, 0x58, 0xd1, 0x6c, 0x6b,
	0xcb, 0xe8, 0xf8, 0x0e, 0x72, 0x3d, 0xdb, 0x41, 0xba, 0xd4, 0x9e, 0xbf, 0x52, 0x8d, 0xa4, 0xc3,
	0x78, 0x54, 0x64, 0x25, 0x6a, 0x2f, 0xa0, 0x30, 0xc3, 0x34, 0x27, 0x99, 0x00, 0xdc, 0x01, 0xab,
	0xf3, 0x00, 0x5f, 0xdd, 0xe4, 0x75, 0x94, 0x2e, 0xcc, 0x9b, 0xde, 0x3c, 0x47, 0x30, 0x20, 0x99,
	0xe2, 0x1a, 0x39, 0x18, 0xf0, 0x42, 0x8a, 0x02, 0x9e, 0x3d, 0x02, 0x16, 0xc9, 0xc2, 0xf9, 0x3b,
	0xf9, 0x7f, 0xdc, 0x5a, 0xb2, 0xef, 0x81, 0x53, 0xba, 0xab, 0xf9, 0xaa, 0xde, 0x35, 0x3c, 0x16,
	0x61, 0xc4, 0x5f, 0x7e, 0x95, 0xe9, 0xcb, 0xba, 0xab, 0xc1, 0x20, 0x9a, 0xc4, 0x79, 0x5e, 0x3e,
	0xa4, 0xa3, 0xd6, 0x71, 0xb3, 0x94, 0x3b, 0xe8, 0xfd, 0xd2, 0x45, 0x31, 0xef, 0xb4, 0x2a, 0xc1,
	0x57, 0x2b, 0x39, 0xa5, 0x65, 0x20, 0x1f, 0x15, 0x98, 0xe6, 0xa2, 0x60, 0xf7, 0xb8, 0xb3, 0xca,
	0xef, 0x82, 0x15, 0x46, 0xa8, 0xfb, 0xc0, 0x50, 0x79, 0xb5, 0x96, 0x8d, 0xcc, 0x20, 0xc7, 0xd8,
	0xc0, 0x06, 0x58, 0x62, 0xc2, 0xaa, 0x91, 0x46, 0x4a, 0xbb, 0x16, 0xf8, 0xb2, 0x10, 0xd5, 0x7b,
	0xfa, 0x1a, 0xf8, 0x4c, 0x79, 0x52, 0x25, 0x19, 0x2b, 0xb0, 0x76, 0x8e, 0x34, 0x3b, 0xa7, 0x2e,
	0xfa, 0x76, 0x03, 0xac, 0x6e, 0xdb, 0xae, 0xe7, 0x33, 0xa9, 0xdb, 0x43, 0x9a, 0xcf, 0x6e, 0xc0,
	0x34, 0x34, 0x4f, 0x8a, 0x95, 0x51, 0x2d, 0xd7, 0x0f, 0xbd, 0x31, 0x86, 0x74, 0x9f, 0xe6, 0x78,
	0x02, 0xc7, 0x01, 0x85, 0x11, 0xce, 0xcb, 0xd1, 0x11, 0xc0, 0x90, 0xa4, 0xc3, 0x24, 0x0e, 0x73,
	0x38, 0xc0, 0xf9, 0x1e, 0xc6, 0xa5, 0x77, 0xb3, 0xd3, 0xe8, 0x14, 0x87, 0xb3, 0xb1, 0x46, 0x21,
	0x19, 0xc2, 0x28, 0x1e, 0x0e, 0x71, 0x86, 0xd3, 0x1c, 0x6e, 0xb3, 0xd1, 0x41, 0x06, 0xac, 0x4a,
	0x54, 0xbe, 0x09, 0x58, 0x42, 0xbe, 0x8e, 0x66, 0xf7, 0x74, 0x6f, 0xce, 0x1d, 0x38, 0x2d, 0xc2,
	0x6f, 0xcc, 0xae, 0xea, 0x29, 0x8b, 0xee, 0x00, 0xb9, 0x24, 0x68, 0x76, 0xb7, 0x6b, 0xb8, 0xae,
	0x61, 0x33, 0x9f, 0xbf, 0xaf, 0x6c, 0xd4, 0x28, 0xaf, 0x95, 0x94, 0x90, 0x4c, 0xc4, 0x8d, 0x1d,
	0x07, 0xba, 0x07, 0xd6, 0xd4, 0xbe, 0x67, 0xfb, 0x1d, 0x64, 0x21, 0x47, 0x65, 0xe6, 0xe0, 0x99,
	0xae, 0xaf, 0x21, 0xc7, 0x93, 0x3e, 0x69, 0x28, 0x66, 0xad, 0x22, 0x5f, 0x56, 0x8b, 0x9c, 0xc0,
	0x11, 0x4e, 0x59, 0xdb, 0xe1, 0x08, 0x86, 0x38, 0x13, 0x43, 0x5c, 0x38, 0x05, 0x1b, 0x85, 0x05,
	0x15, 0xd3, 0x55, 0xed, 0x19, 0xb0, 0x13, 0xe4, 0x78, 0x2f, 0xd8, 0x87, 0x9e, 0xe9, 0xf2, 0xe2,
	0xbf, 0x09, 0x4e, 0x99, 0x76, 0xc7, 0x98, 0xbd, 0xe9, 0x9f, 0x2e, 0x2a, 0xaf, 0xd4, 0xe2, 0x3d,
	0xdd, 0xa7, 0x38, 0x83, 0x09, 0x19, 0xc5, 0x55, 0x4b, 0xcb, 0x9b, 0x60, 0x45, 0xed, 0xeb, 0x86,
	0x67, 0x58, 0x9d, 0x4a, 0xf4, 0xb3, 0x45, 0x5e, 0xaa, 0xea, 0x9a, 0xcf, 0xdf, 0xc9, 0xe2, 0x9c,
	0x9d, 0x4c, 0x86, 0x50, 0x2d, 0xa2, 0x38, 0xe7, 0x4b, 0x54, 0xfd, 0x49, 0x5c, 0x03, 0x52, 0x4f,
	0x75, 0xdd, 0x3b, 0xb6, 0xa3, 0xcf, 0xc6, 0xf1, 0xcf, 0x17, 0x95, 0xf3, 0xb5, 0xd4, 0xa4, 0x5e,
	0x40, 0xe9, 0x1e, 0xc9, 0xa2, 0x6a, 0x22, 0xcb, 0x1b, 0x60, 0x79, 0x26, 0x72, 0x90, 0x8b, 0x3c,
	0xe9, 0x17, 0x8b, 0x8a, 0x52, 0x93, 0x2c, 0xcf, 0x24, 0x19, 0xa6, 0x38, 0x97, 0x37, 0xc0, 0xf3,
	0xa6, 0x61, 0xdd, 0xf2, 0xfb, 0x3d, 0xe9, 0xbb, 0x4d, 0xe5, 0xa2, 0x48, 0xab, 0xc1, 0xd2, 0xea,
	0x91, 0x2c, 0x67, 0x15, 0x4a, 0xe2, 0xf4, 0x3e, 0x8e, 0x60, 0x31, 0x15, 0xf3, 0xfd, 0x24, 0x17,
	0xe8, 0xf6, 0x1d, 0x4b, 0xfa, 0x5e, 0x53, 0xb9, 0x2c, 0xe0, 0x4c, 0x72, 0x8e, 0x4b, 0xd8, 0xef,
	0xd9, 0x56, 0x97, 0x17, 0x94, 0xc9, 0x23, 0xb2, 0x57, 0x6e, 0x06, 0x6f, 0x83, 0x15, 0x17, 0x39,
	0x3b, 0x86, 0x86, 0x66, 0xab, 0xe1, 0x1f, 0x9b, 0x7c, 0x44, 0x37, 0x1f, 0x1d, 0xb4, 0x16, 0xd9,
	0x88, 0x76, 0x71, 0xb6, 0x1b, 0x87, 0xb8, 0xda, 0x09, 0x9f, 0x96, 0x95, 0x1b, 0xd6, 0x9f, 0x4a,
	0x59, 0xe3, 0x69, 0xd9, 0xe1, 0x6a, 0xa5, 0x81, 0x55, 0x4b, 0xed, 0x99, 0xc8, 0xf5, 0x8f, 0xaa,
	0xff, 0xdc, 0xe4, 0xb3, 0x75, 0x51, 0xa8, 0x57, 0xad, 0x60, 0x9a, 0x60, 0x0a, 0xe9, 0x31, 0x90,
	0xb7, 0x0e, 0xcf, 0xee, 0x21, 0x4b, 0x37, 0xac, 0x8e, 0xf4, 0x97, 0xa6, 0xf2, 0xf2, 0x71, 0x21,
	0x4f, 0x71, 0x1a, 0xc5, 0xe9, 0xa8, 0x1e, 0xae, 0xd3, 0xb7, 0x2c, 0x26, 0xf9, 0xeb, 0xa7, 0x64,
	0x99, 0x15, 0x69, 0x1a, 0xa7, 0x23, 0x7e, 0xd2, 0x87, 0xe0, 0x4c, 0x25, 0x63, 0x1e, 0xe4, 0xf6,
	0x6c, 0xcb, 0x35, 0x76, 0x90, 0xf4, 0xb7, 0xa6, 0x72, 0xb3, 0x16, 0xec, 0x17, 0x2a, 0x2d, 0xb3,
	0x21, 0x3a, 0x25, 0x29, 0x8d, 0x77, 0x31, 0x8c, 0x0a, 0xcc, 0x5c, 0x31, 0x09, 0xc2, 0xfb, 0xac,
	0x9f, 0xc4, 0xf3, 0xcf, 0x30, 0x25, 0x45, 0x16, 0x62, 0x2a, 0x5f, 0x07, 0x2b, 0xee, 0x5d, 0xd7,
	0x43, 0x5d, 0x5f, 0xb3, 0x4d, 0x7d, 0xd3, 0xb6, 0x3d, 0xe9, 0xef, 0x4d, 0xee, 0x6e, 0x55, 0x09,
	0x65, 0xb7, 0xd4, 0x84, 0x24, 0x89, 0xe0, 0x80, 0x90, 0xaa, 0xf8, 0x08, 0x9c, 0x15, 0x4a, 0x07,
	0xb9, 0x76, 0xdf, 0x61, 0xe1, 0xb9, 0x6a, 0x07, 0x49, 0xff, 0x68, 0xce, 0xb6, 0x70, 0xa6, 0x7f,
	0xc5, 0x9d, 0x3f, 0x13, 0x16, 0x34, 0x18, 0x95, 0xdb, 0x56, 0x3c, 0x1a, 0xcb, 0x68, 0x76, 0x19,
	0x5b, 0xaa, 0xa7, 0x9a, 0xbe, 0x61, 0x79, 0xc8, 0x71, 0xfa, 0x3d, 0x4f, 0xfa, 0x67, 0xd9, 0x41,
	0x55, 0x7e, 0xe7, 0xc5, 0x65, 0x30, 0xeb, 0x0a, 0xe0, 0x30, 0xc8, 0x83, 0x04, 0xc6, 0x69, 0x8e,
	0xb3, 0xac, 0x98, 0xe6, 0xf2, 0x0e, 0x38, 0x27, 0x30, 0x9a, 0xea, 0x79, 0x8e, 0xd1, 0xab, 0x81,
	0xfe, 0xd5, 0x54, 0xde, 0xad, 0x81, 0xae, 0x88, 0x80, 0x70, 0x1a, 0x92, 0x82, 0x01, 0xd8, 0xcb,
	0xe7, 0x5f, 0x2f, 0xd3, 0xb2, 0xf3, 0xf9, 0xe3, 0x2b, 0x2b, 0x25, 0x1b, 0x40, 0x12, 0x5c, 0x7b,
	0x07, 0x39, 0xbe, 0x87, 0xba, 0x3d, 0xe9, 0xdf, 0x4d, 0xe5, 0x6a, 0x8d, 0x77, 0x49, 0xf0, 0x72,
	0x3c, 0xe1, 0xf3, 0xab, 0xc8, 0x78, 0x76, 0xc1, 0x80, 0xec, 0x62, 0x98, 0x8f, 0x33, 0x4c, 0xc7,
	0x24, 0x89, 0xd6, 0x65, 0x0b, 0x9c, 0x3d, 0x8a, 0xf2, 0xd1, 0x07, 0x86, 0x27, 0xfd, 0xa7, 0xe4,
	0x2d, 0xfc, 0x6f, 0x5e, 0xb9, 0xb7, 0xd4, 0x78, 0x5b, 0x40, 0x16, 0xbc, 0x9e, 0x6a, 0x19, 0x9a,
	0x8f, 0x76, 0x90, 0xe5, 0x49, 0xdf, 0x6c, 0x29, 0x6f, 0xd4, 0x82, 0xbb, 0x28, 0x60, 0xd3, 0x20,
	0x8d, 0x43, 0x48, 0x4a, 0x67, 0x9f, 0x66, 0x78, 0x37, 0x26, 0x05, 0xe5, 0xd7, 0x29, 0x77, 0x41,
	0xbb, 0xe2, 0xf0, 0x31, 0x62, 0xa8, 0xa5, 0x33, 0xf5, 0x1d, 0x24, 0x98, 0xdf, 0x6a, 0xcd, 0xde,
	0x32, 0xbf, 0x89, 0x8a, 0xc9, 0x06, 0x00, 0x5b, 0x51, 0x60, 0xce, 0x86, 0xa6, 0x30, 0x38, 0x15,
	0xac, 0xe9, 0x86, 0x7b, 0xcb, 0xf7, 0xb6, 0x1d, 0xe4, 0x6e, 0xdb, 0xa6, 0xee, 0xa3, 0x0f, 0x34,
	0x84, 0x74, 0xa4, 0x4b, 0xbf, 0x6d, 0xcd, 0xf6, 0xe7, 0x13, 0x6c, 0xe5, 0xd7, 0x63, 0x7a, 0xff,
	0x30, 0x23, 0x88, 0xbf, 0x1e, 0x62, 0x1c, 0xe1, 0x48, 0xbe, 0x06, 0x56, 0x1c, 0xdb, 0x34, 0xed,
	0xbe, 0x37, 0xb3, 0x83, 0x5f, 0x9e, 0x98, 0x19, 0x6b, 0x93, 0x39, 0x90, 0x43, 0x92, 0x84, 0x14,
	0x39, 0x8c, 0xd3, 0x38, 0x8f, 0x99, 0x79, 0xcb, 0x37, 0x6b, 0xa2, 0xbe, 0xa6, 0x21, 0xd7, 0x95,
	0x7e, 0x75, 0x62, 0xb6, 0x15, 0x31, 0x51, 0xbb, 0x12, 0xd1, 0x22, 0x0c, 0x31, 0xa5, 0xc3, 0x22,
	0x49, 0xf6, 0x61, 0x48, 0x26, 0xd3, 0x04, 0xe7, 0xa5, 0x43, 0x56, 0x04, 0xe1, 0xcc, 0xbf, 0x3e,
	0xc1, 0x1d, 0x72, 0x51, 0x00, 0x96, 0x2b, 0x80, 0x48, 0xf5, 0x6d, 0x70, 0xfa, 0xf0, 0x48, 0x97,
	0xd9, 0x00, 0xd2, 0xa5, 0x8f, 0x3f, 0x25, 0x52, 0x5a, 0x50, 0x66, 0x04, 0x38, 0x92, 0xaf, 0x82,
	0x25, 0xb1, 0x24, 0xb1, 0x63, 0xa4, 0x1f, 0x3f, 0xc7, 0xa7, 0x3c, 0xab, 0x6e, 0xeb, 0xf1, 0x41,
	0xeb, 0xcc, 0xfc, 0x6e, 0x54, 0x1e, 0xa5, 0xbc, 0xf4, 0x9d, 0x9f, 0xb4, 0x17, 0x1e, 0x3e, 0x68,
	0x2f, 0x3c, 0x7a, 0xd0, 0x6e, 0x3c, 0x7e, 0xd0, 0x3e, 0x39, 0xfb, 0xb2, 0xbe, 0xe1, 0x81, 0x17,
	0x28, 0xde, 0xc5, 0x59, 0x9c, 0xef, 0xcb, 0x17, 0xd7, 0xcb, 0x0f, 0xf5, 0xf5, 0xea, 0x43, 0x7d,
	0x1d, 0xa5, 0xc5, 0x64, 0x27, 0x48, 0x0a, 0x6c, 0x4f, 0xf9, 0x0c, 0x3f, 0xf7, 0x7d, 0x0b, 0x36,
	0x2e, 0x2f, 0x5f, 0x3d, 0xb3, 0xce, 0x3f, 0xf5, 0x59, 0xdf, 0xd3, 0x75, 0x57, 0xe8, 0x9d, 0x19,
	0x89, 0x51, 0xd9, 0x1c, 0x1c, 0x91, 0xec, 0x99, 0xa8, 0x3f, 0x38, 0x86, 0xaa, 0x09, 0xbd, 0x33,
	0x23, 0xdd, 0x78, 0x07, 0xb4, 0x22, 0x4c, 0xc3, 0x67, 0x21, 0xfe, 0x90, 0x11, 0x4f, 0x3a, 0xfc,
	0xf7, 0x37, 0x34, 0xb0, 0xc4, 0x36, 0xf6, 0x0c, 0x53, 0xea, 0x4f, 0x26, 0xcf, 0x22, 0xff, 0x11,
	0x93, 0xbf, 0xe0, 0x80, 0x4a, 0xd6, 0x9d, 0x6c, 0x4a, 0x9f, 0x3c, 0x69, 0x37, 0x7e, 0xf3, 0xa4,
	0xdd, 0xf8, 0xdd, 0x93, 0x76, 0xe3, 0x0f, 0x4f, 0xda, 0x0b, 0xbd, 0x85, 0xc1, 0x73, 0x9c, 0x70,
	0xed, 0xbf, 0x01, 0x00, 0x00, 0xff, 0xff, 0x18, 0x5f, 0x2a, 0x9b, 0x0b, 0x11, 0x00, 0x00,
}
