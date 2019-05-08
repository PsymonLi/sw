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
	// -------------------------------- Host/NIC events -------------------------- //
	NIC_ADMITTED EventType = 100
	//
	NIC_HEALTH_UNKNOWN EventType = 101
	//
	NIC_HEALTHY EventType = 102
	//
	NIC_UNHEALTHY EventType = 103
	//
	NIC_REJECTED EventType = 104
	//
	HOST_SMART_NIC_SPEC_CONFLICT EventType = 105
	// ----------------------------- API Gateway events ---------------------- //
	AUTO_GENERATED_TLS_CERT EventType = 200
	// --------------------------- Auth/Audit events ------------------------- //
	LOGIN_FAILED EventType = 300
	//
	AUDITING_FAILED EventType = 301
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
	100: "NIC_ADMITTED",
	101: "NIC_HEALTH_UNKNOWN",
	102: "NIC_HEALTHY",
	103: "NIC_UNHEALTHY",
	104: "NIC_REJECTED",
	105: "HOST_SMART_NIC_SPEC_CONFLICT",
	200: "AUTO_GENERATED_TLS_CERT",
	300: "LOGIN_FAILED",
	301: "AUDITING_FAILED",
	400: "LINK_UP",
	401: "LINK_DOWN",
	500: "SERVICE_STARTED",
	501: "SERVICE_STOPPED",
	502: "NAPLES_SERVICE_STOPPED",
	503: "SERVICE_PENDING",
	504: "SERVICE_RUNNING",
	505: "SERVICE_UNRESPONSIVE",
	506: "SYSTEM_COLDBOOT",
}
var EventType_value = map[string]int32{
	"ELECTION_STARTED":             0,
	"ELECTION_CANCELLED":           1,
	"ELECTION_NOTIFICATION_FAILED": 2,
	"ELECTION_STOPPED":             3,
	"LEADER_ELECTED":               4,
	"LEADER_LOST":                  5,
	"LEADER_CHANGED":               6,
	"NODE_JOINED":                  7,
	"NODE_DISJOINED":               8,
	"NIC_ADMITTED":                 100,
	"NIC_HEALTH_UNKNOWN":           101,
	"NIC_HEALTHY":                  102,
	"NIC_UNHEALTHY":                103,
	"NIC_REJECTED":                 104,
	"HOST_SMART_NIC_SPEC_CONFLICT": 105,
	"AUTO_GENERATED_TLS_CERT":      200,
	"LOGIN_FAILED":                 300,
	"AUDITING_FAILED":              301,
	"LINK_UP":                      400,
	"LINK_DOWN":                    401,
	"SERVICE_STARTED":              500,
	"SERVICE_STOPPED":              501,
	"NAPLES_SERVICE_STOPPED":       502,
	"SERVICE_PENDING":              503,
	"SERVICE_RUNNING":              504,
	"SERVICE_UNRESPONSIVE":         505,
	"SYSTEM_COLDBOOT":              506,
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

func init() {
	proto.RegisterEnum("eventtypes.EventType", EventType_name, EventType_value)
	proto.RegisterExtension(E_Severity)
	proto.RegisterExtension(E_Category)
	proto.RegisterExtension(E_Desc)
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
	// 1198 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x8c, 0x96, 0xcb, 0x92, 0xd3, 0xc6,
	0x1a, 0xc7, 0xc7, 0x63, 0x1f, 0x2e, 0x0d, 0x67, 0x46, 0x47, 0x45, 0x71, 0x11, 0x1c, 0xa7, 0x21,
	0x24, 0x43, 0xa0, 0x32, 0x10, 0x32, 0x49, 0x0a, 0x2a, 0x81, 0x08, 0xa9, 0xc7, 0x16, 0x08, 0xc9,
	0x65, 0xc9, 0x50, 0x54, 0x16, 0x8a, 0x2c, 0x7d, 0xb6, 0x05, 0x1a, 0xb5, 0xd3, 0xdd, 0x9a, 0xa9,
	0x79, 0x83, 0xac, 0x52, 0xc9, 0x22, 0xc9, 0x03, 0x84, 0xc5, 0x2c, 0x92, 0x2a, 0xd6, 0x6c, 0xb3,
	0x61, 0x99, 0x47, 0x48, 0xf1, 0x06, 0xae, 0xca, 0x7d, 0x95, 0x6a, 0x59, 0x36, 0x9a, 0xe1, 0x52,
	0x2c, 0xe5, 0xea, 0xdf, 0xaf, 0xff, 0xfd, 0xf5, 0xd7, 0xdd, 0x46, 0x0a, 0x6c, 0x42, 0x26, 0xc4,
	0xf6, 0x18, 0xf8, 0xea, 0x98, 0x51, 0x41, 0x55, 0xf4, 0xec, 0x17, 0x0d, 0x0d, 0xe9, 0x90, 0x4e,
	0x7f, 0xd7, 0xf0, 0x90, 0xd2, 0x61, 0x0a, 0x17, 0x8b, 0xaf, 0x7e, 0x3e, 0xb8, 0x18, 0x03, 0x8f,
	0x58, 0x32, 0x16, 0x94, 0x95, 0x23, 0x94, 0x50, 0x08, 0x96, 0xf4, 0x73, 0x31, 0x73, 0x9d, 0xff,
	0x79, 0x09, 0x21, 0x22, 0x75, 0xbe, 0xd4, 0xa9, 0x6d, 0xa4, 0x10, 0x9b, 0x18, 0xbe, 0xe5, 0x3a,
	0x81, 0xe7, 0xeb, 0x5d, 0x9f, 0x98, 0xca, 0x82, 0x76, 0x79, 0x67, 0xd2, 0x58, 0x78, 0x34, 0x69,
	0x2c, 0x3c, 0x9e, 0x34, 0xde, 0xb6, 0x21, 0x8c, 0x81, 0x61, 0x48, 0x21, 0x12, 0x09, 0xcd, 0x30,
	0x17, 0x21, 0x13, 0x10, 0xe3, 0x24, 0xc3, 0x62, 0x04, 0x38, 0x4a, 0x73, 0x2e, 0x80, 0xa9, 0x9f,
	0x20, 0x75, 0x6e, 0x32, 0x74, 0xc7, 0x20, 0xb6, 0x4d, 0x4c, 0xa5, 0xa6, 0xbd, 0xb5, 0x33, 0x69,
	0xd4, 0x4a, 0xd7, 0x89, 0xbd, 0xae, 0x28, 0xcc, 0x22, 0x48, 0x53, 0x88, 0xd5, 0x7b, 0xe8, 0xd4,
	0x1c, 0x77, 0x5c, 0xdf, 0x5a, 0xb7, 0x0c, 0xbd, 0xf8, 0x58, 0xd7, 0x2d, 0x29, 0x5a, 0xd4, 0x3e,
	0xaa, 0x88, 0x2e, 0xac, 0x87, 0x49, 0x0a, 0x31, 0x16, 0x14, 0x73, 0xc8, 0x62, 0x9c, 0xee, 0xf1,
	0x66, 0x54, 0x24, 0x83, 0x24, 0x0a, 0xe5, 0x87, 0x7a, 0x65, 0xd7, 0x1a, 0xdd, 0x4e, 0x87, 0x98,
	0x4a, 0x5d, 0x7b, 0xb3, 0xa2, 0x3b, 0xf6, 0xfc, 0x1a, 0xe9, 0x78, 0x0c, 0xb1, 0x7a, 0x0d, 0x2d,
	0xd9, 0x44, 0x37, 0x49, 0x37, 0x28, 0x0c, 0xc4, 0x54, 0x1a, 0xda, 0xf9, 0x4a, 0x71, 0x9a, 0x55,
	0x10, 0x62, 0x3c, 0xa0, 0x6c, 0x57, 0x51, 0x08, 0x3a, 0x54, 0xf2, 0xb6, 0xeb, 0xf9, 0xca, 0x7f,
	0xb4, 0xb5, 0x0a, 0x7c, 0xce, 0xa1, 0x31, 0xe0, 0x94, 0x72, 0x51, 0xe6, 0xe7, 0xa3, 0x64, 0x8c,
	0xe3, 0x9c, 0x25, 0xd9, 0xb0, 0xb0, 0xcc, 0xe2, 0x54, 0x62, 0x18, 0x6d, 0xdd, 0x69, 0x11, 0x53,
	0xd9, 0xf7, 0xc2, 0x18, 0xd1, 0x28, 0xcc, 0x86, 0xcf, 0xb6, 0x66, 0xce, 0xaf, 0xa1, 0x43, 0x8e,
	0x6b, 0x92, 0xe0, 0xa6, 0x6b, 0x39, 0xc4, 0x54, 0xf6, 0x17, 0x8b, 0x9f, 0xc1, 0xc7, 0x8a, 0x18,
	0xf7, 0x69, 0x92, 0xc9, 0x82, 0x56, 0xc2, 0x5f, 0x47, 0x4b, 0x05, 0x65, 0x5a, 0x5e, 0x09, 0x1e,
	0xd0, 0x2e, 0x54, 0xaa, 0xf6, 0x46, 0x01, 0xc6, 0x09, 0x2f, 0xd9, 0x01, 0xa3, 0x1b, 0xbb, 0x04,
	0x57, 0xd0, 0x61, 0xc7, 0x32, 0x02, 0xdd, 0xbc, 0x6d, 0xf9, 0xb2, 0x76, 0xb1, 0xb6, 0x52, 0x99,
	0xf7, 0xa4, 0x63, 0x19, 0x38, 0x8c, 0x37, 0x12, 0x21, 0xa6, 0x3b, 0x59, 0x45, 0xdb, 0x48, 0x95,
	0x68, 0x9b, 0xe8, 0xb6, 0xdf, 0x0e, 0x7a, 0xce, 0x2d, 0xc7, 0xbd, 0xeb, 0x28, 0xa0, 0x5d, 0xaa,
	0xcc, 0x7f, 0x56, 0x0a, 0x46, 0x10, 0xa6, 0x62, 0x84, 0x05, 0x0b, 0x33, 0x9e, 0xc8, 0xc5, 0x4e,
	0x5d, 0x2b, 0x79, 0xf6, 0x20, 0xa3, 0x5b, 0xd9, 0x8a, 0x7a, 0x0d, 0x1d, 0x7a, 0x66, 0xba, 0xa7,
	0x0c, 0xb4, 0x77, 0x2b, 0x19, 0x4e, 0xbf, 0x4a, 0x21, 0x58, 0x0e, 0x2b, 0xaa, 0x8e, 0xfe, 0x2b,
	0xf9, 0x9e, 0x33, 0x33, 0x0c, 0xb5, 0xd5, 0x4a, 0x88, 0x33, 0xaf, 0x32, 0x0c, 0xc2, 0x94, 0xc3,
	0x8a, 0xda, 0x9a, 0xd6, 0xa1, 0x4b, 0x6e, 0x4e, 0x7b, 0x68, 0xa4, 0x7d, 0x50, 0x31, 0xbc, 0xa3,
	0x63, 0x6f, 0x23, 0x64, 0x62, 0x56, 0x0e, 0xce, 0x65, 0x07, 0x32, 0xf8, 0x22, 0x07, 0x2e, 0xf0,
	0x56, 0xc8, 0x31, 0x83, 0xfb, 0x45, 0x7b, 0xa9, 0x5f, 0xd5, 0xd0, 0xa9, 0xb6, 0xeb, 0xf9, 0x81,
	0x77, 0x5b, 0xef, 0xfa, 0x81, 0x94, 0x7a, 0x1d, 0x62, 0x04, 0x86, 0xeb, 0xac, 0xdb, 0x96, 0xe1,
	0x2b, 0x89, 0x96, 0x56, 0xcc, 0x9f, 0xfb, 0x23, 0xc0, 0x7c, 0x9b, 0x0b, 0xd8, 0xc0, 0xa3, 0x90,
	0xe3, 0x18, 0xc4, 0xb4, 0x4d, 0x43, 0x1c, 0xd1, 0x6c, 0x90, 0x26, 0x91, 0xc0, 0x7d, 0x10, 0x5b,
	0x00, 0xd3, 0x7e, 0x99, 0xc7, 0xe0, 0x63, 0x88, 0xe6, 0xe7, 0x87, 0x63, 0x3a, 0xc0, 0x71, 0x32,
	0x18, 0x00, 0x83, 0x4c, 0xe0, 0xb6, 0xec, 0x57, 0xda, 0x97, 0x89, 0xb8, 0x3a, 0x40, 0xc7, 0xf4,
	0x9e, 0xef, 0x06, 0x2d, 0xe2, 0x90, 0xae, 0xee, 0x13, 0x33, 0xf0, 0x6d, 0x2f, 0x30, 0x48, 0xd7,
	0x57, 0x9e, 0xd4, 0xb4, 0x76, 0x25, 0xcb, 0xc7, 0x7a, 0x2e, 0x28, 0x1e, 0x42, 0x06, 0x2c, 0x94,
	0x11, 0x22, 0x60, 0xe5, 0x11, 0x05, 0x9c, 0x70, 0xdc, 0x07, 0xd9, 0xf4, 0x39, 0x2f, 0xcf, 0x90,
	0xde, 0xb1, 0x70, 0x2b, 0x14, 0xb0, 0x15, 0x6e, 0x63, 0xdf, 0xf6, 0xd4, 0x4b, 0xe8, 0xb0, 0xed,
	0xb6, 0xac, 0xf9, 0x6d, 0xf0, 0xe3, 0xa2, 0xf6, 0xff, 0xca, 0x36, 0xfe, 0xaf, 0xc7, 0x81, 0xe1,
	0x94, 0x0e, 0x93, 0x0c, 0x0f, 0x8a, 0x9b, 0x41, 0xbd, 0x86, 0x96, 0xf5, 0x9e, 0x69, 0xf9, 0x96,
	0xd3, 0x9a, 0x41, 0x3f, 0x2d, 0x6a, 0xe7, 0x76, 0x26, 0x8d, 0xc5, 0x12, 0x3a, 0x75, 0x97, 0x25,
	0x42, 0xce, 0x4a, 0x07, 0x58, 0xcf, 0xe3, 0x44, 0x14, 0xd7, 0xe2, 0x8c, 0xbf, 0x80, 0xf6, 0xdb,
	0x96, 0x73, 0x2b, 0xe8, 0x75, 0x94, 0xaf, 0xeb, 0xf3, 0xc9, 0x6a, 0x72, 0xb2, 0x0e, 0x65, 0x42,
	0x66, 0x4e, 0x93, 0xec, 0x01, 0xc4, 0x38, 0x1f, 0xab, 0x6b, 0xe8, 0x60, 0x31, 0xd8, 0x94, 0x4d,
	0xfa, 0x4d, 0x5d, 0x3b, 0x5b, 0x2e, 0x5c, 0x0e, 0x3f, 0x5e, 0x0c, 0x97, 0x63, 0xe5, 0xcd, 0x29,
	0x72, 0x2e, 0xd1, 0x98, 0x6e, 0x65, 0xea, 0x7b, 0x68, 0xd9, 0x23, 0xdd, 0x3b, 0x96, 0x41, 0xe6,
	0x57, 0xef, 0x6f, 0x75, 0xed, 0xe4, 0xce, 0xa4, 0x51, 0x7f, 0x34, 0x69, 0x2c, 0x3e, 0x9e, 0x34,
	0x96, 0x3d, 0x60, 0x9b, 0x49, 0x04, 0xb3, 0x3b, 0x77, 0x37, 0x32, 0xbd, 0xc9, 0x7e, 0x9f, 0x22,
	0xb5, 0xe7, 0x91, 0xe9, 0x15, 0x76, 0x1d, 0x1d, 0x75, 0xf4, 0x8e, 0x4d, 0xbc, 0x60, 0x2f, 0xf9,
	0x47, 0x5d, 0x3b, 0x53, 0xd6, 0x43, 0x92, 0x47, 0x9d, 0x70, 0x9c, 0x02, 0xc7, 0x7c, 0x8f, 0xa0,
	0x32, 0x67, 0x87, 0x38, 0xa6, 0xe5, 0xb4, 0x94, 0x3f, 0x5f, 0x12, 0x73, 0x0c, 0x59, 0x9c, 0x64,
	0xc3, 0x2a, 0xd2, 0xed, 0x39, 0x8e, 0x44, 0xfe, 0x7a, 0x09, 0xc2, 0xf2, 0x2c, 0x93, 0xc8, 0x67,
	0xe8, 0xc8, 0x0c, 0xe9, 0x39, 0x5d, 0xe2, 0x75, 0x5c, 0xc7, 0xb3, 0xee, 0x10, 0xe5, 0xef, 0xba,
	0xf6, 0x69, 0x25, 0xe4, 0xda, 0x8c, 0xcb, 0x33, 0x06, 0x7c, 0x4c, 0x33, 0x9e, 0x6c, 0x02, 0x8e,
	0x73, 0x90, 0xe7, 0x2d, 0x0d, 0xa3, 0x07, 0x72, 0x37, 0xcb, 0x96, 0x67, 0xc0, 0x69, 0xce, 0x22,
	0xe0, 0xea, 0x1a, 0x5a, 0xf6, 0xee, 0x79, 0x3e, 0xb9, 0x1d, 0x18, 0xae, 0x6d, 0xde, 0x70, 0x5d,
	0x5f, 0xf9, 0xa7, 0xae, 0x35, 0x2b, 0x65, 0x53, 0xbd, 0x29, 0x13, 0xd1, 0x34, 0xc6, 0x7d, 0x4a,
	0x05, 0xc4, 0xda, 0x89, 0x2f, 0x7f, 0x68, 0x2e, 0xec, 0x3c, 0x6c, 0x2e, 0x3c, 0x7a, 0xd8, 0xac,
	0x3d, 0x7e, 0xd8, 0x3c, 0x38, 0x7f, 0x37, 0xaf, 0xfa, 0xe8, 0x00, 0x87, 0x4d, 0x60, 0x89, 0xd8,
	0x56, 0x4f, 0xaf, 0x4e, 0x9f, 0xe1, 0xd5, 0xd9, 0x33, 0xbc, 0x4a, 0xb2, 0x7c, 0xe3, 0x4e, 0x98,
	0xe6, 0xe0, 0x8e, 0x8b, 0x03, 0x74, 0xfc, 0x5b, 0x07, 0xd7, 0xce, 0x2d, 0x5d, 0x3e, 0xb2, 0x5a,
	0x3c, 0xe4, 0xf2, 0x4d, 0xe6, 0xab, 0x5e, 0xc9, 0x77, 0xe7, 0x26, 0x69, 0x95, 0xe7, 0x61, 0x48,
	0xd9, 0x6b, 0x59, 0xbf, 0x7b, 0x81, 0xd5, 0x28, 0xf9, 0xee, 0xdc, 0x74, 0xf5, 0x43, 0xd4, 0x90,
	0xff, 0x0b, 0x5e, 0xc7, 0xf8, 0xbd, 0x34, 0x1e, 0xec, 0x16, 0xe3, 0x6f, 0x1c, 0x7e, 0xf2, 0xb4,
	0x59, 0xfb, 0xe5, 0x69, 0xb3, 0xf6, 0xeb, 0xd3, 0x66, 0xad, 0xb3, 0xd0, 0xdf, 0x57, 0x70, 0xef,
	0xff, 0x1b, 0x00, 0x00, 0xff, 0xff, 0x65, 0x6c, 0xd2, 0xf1, 0xa0, 0x08, 0x00, 0x00,
}
