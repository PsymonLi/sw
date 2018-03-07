// Code generated by protoc-gen-go. DO NOT EDIT.
// source: metric.proto

/*
Package metric is a generated protocol buffer package.

It is generated from these files:
	metric.proto

It has these top-level messages:
	Field
	MetricPoint
	MetricBundle
*/
package metric

import proto "github.com/golang/protobuf/proto"
import fmt "fmt"
import math "math"
import _ "google.golang.org/genproto/googleapis/api/annotations"
import _ "github.com/pensando/sw/venice/utils/apigen/annotations"
import api "github.com/pensando/sw/api"
import api1 "github.com/pensando/sw/api"
import _ "github.com/gogo/protobuf/gogoproto"

import (
	context "golang.org/x/net/context"
	grpc "google.golang.org/grpc"
)

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

// This is a compile-time assertion to ensure that this generated file
// is compatible with the proto package it is being compiled against.
// A compilation error at this line likely means your copy of the
// proto package needs to be updated.
const _ = proto.ProtoPackageIsVersion2 // please upgrade the proto package

// Field is one of (uint64, float64, string, bool)
type Field struct {
	// Types that are valid to be assigned to F:
	//	*Field_Int64
	//	*Field_Float64
	//	*Field_String_
	//	*Field_Bool
	F isField_F `protobuf_oneof:"f"`
}

func (m *Field) Reset()                    { *m = Field{} }
func (m *Field) String() string            { return proto.CompactTextString(m) }
func (*Field) ProtoMessage()               {}
func (*Field) Descriptor() ([]byte, []int) { return fileDescriptor0, []int{0} }

type isField_F interface{ isField_F() }

type Field_Int64 struct {
	Int64 int64 `protobuf:"varint,1,opt,name=Int64,oneof"`
}
type Field_Float64 struct {
	Float64 float64 `protobuf:"fixed64,2,opt,name=Float64,oneof"`
}
type Field_String_ struct {
	String_ string `protobuf:"bytes,3,opt,name=String,oneof"`
}
type Field_Bool struct {
	Bool bool `protobuf:"varint,4,opt,name=Bool,oneof"`
}

func (*Field_Int64) isField_F()   {}
func (*Field_Float64) isField_F() {}
func (*Field_String_) isField_F() {}
func (*Field_Bool) isField_F()    {}

func (m *Field) GetF() isField_F {
	if m != nil {
		return m.F
	}
	return nil
}

func (m *Field) GetInt64() int64 {
	if x, ok := m.GetF().(*Field_Int64); ok {
		return x.Int64
	}
	return 0
}

func (m *Field) GetFloat64() float64 {
	if x, ok := m.GetF().(*Field_Float64); ok {
		return x.Float64
	}
	return 0
}

func (m *Field) GetString_() string {
	if x, ok := m.GetF().(*Field_String_); ok {
		return x.String_
	}
	return ""
}

func (m *Field) GetBool() bool {
	if x, ok := m.GetF().(*Field_Bool); ok {
		return x.Bool
	}
	return false
}

// XXX_OneofFuncs is for the internal use of the proto package.
func (*Field) XXX_OneofFuncs() (func(msg proto.Message, b *proto.Buffer) error, func(msg proto.Message, tag, wire int, b *proto.Buffer) (bool, error), func(msg proto.Message) (n int), []interface{}) {
	return _Field_OneofMarshaler, _Field_OneofUnmarshaler, _Field_OneofSizer, []interface{}{
		(*Field_Int64)(nil),
		(*Field_Float64)(nil),
		(*Field_String_)(nil),
		(*Field_Bool)(nil),
	}
}

func _Field_OneofMarshaler(msg proto.Message, b *proto.Buffer) error {
	m := msg.(*Field)
	// f
	switch x := m.F.(type) {
	case *Field_Int64:
		b.EncodeVarint(1<<3 | proto.WireVarint)
		b.EncodeVarint(uint64(x.Int64))
	case *Field_Float64:
		b.EncodeVarint(2<<3 | proto.WireFixed64)
		b.EncodeFixed64(math.Float64bits(x.Float64))
	case *Field_String_:
		b.EncodeVarint(3<<3 | proto.WireBytes)
		b.EncodeStringBytes(x.String_)
	case *Field_Bool:
		t := uint64(0)
		if x.Bool {
			t = 1
		}
		b.EncodeVarint(4<<3 | proto.WireVarint)
		b.EncodeVarint(t)
	case nil:
	default:
		return fmt.Errorf("Field.F has unexpected type %T", x)
	}
	return nil
}

func _Field_OneofUnmarshaler(msg proto.Message, tag, wire int, b *proto.Buffer) (bool, error) {
	m := msg.(*Field)
	switch tag {
	case 1: // f.Int64
		if wire != proto.WireVarint {
			return true, proto.ErrInternalBadWireType
		}
		x, err := b.DecodeVarint()
		m.F = &Field_Int64{int64(x)}
		return true, err
	case 2: // f.Float64
		if wire != proto.WireFixed64 {
			return true, proto.ErrInternalBadWireType
		}
		x, err := b.DecodeFixed64()
		m.F = &Field_Float64{math.Float64frombits(x)}
		return true, err
	case 3: // f.String
		if wire != proto.WireBytes {
			return true, proto.ErrInternalBadWireType
		}
		x, err := b.DecodeStringBytes()
		m.F = &Field_String_{x}
		return true, err
	case 4: // f.Bool
		if wire != proto.WireVarint {
			return true, proto.ErrInternalBadWireType
		}
		x, err := b.DecodeVarint()
		m.F = &Field_Bool{x != 0}
		return true, err
	default:
		return false, nil
	}
}

func _Field_OneofSizer(msg proto.Message) (n int) {
	m := msg.(*Field)
	// f
	switch x := m.F.(type) {
	case *Field_Int64:
		n += proto.SizeVarint(1<<3 | proto.WireVarint)
		n += proto.SizeVarint(uint64(x.Int64))
	case *Field_Float64:
		n += proto.SizeVarint(2<<3 | proto.WireFixed64)
		n += 8
	case *Field_String_:
		n += proto.SizeVarint(3<<3 | proto.WireBytes)
		n += proto.SizeVarint(uint64(len(x.String_)))
		n += len(x.String_)
	case *Field_Bool:
		n += proto.SizeVarint(4<<3 | proto.WireVarint)
		n += 1
	case nil:
	default:
		panic(fmt.Sprintf("proto: unexpected type %T in oneof", x))
	}
	return n
}

// MetricPoint contains a set of tags and fields, and a timestamp
type MetricPoint struct {
	Name   string            `protobuf:"bytes,1,opt,name=name" json:"name,omitempty"`
	Tags   map[string]string `protobuf:"bytes,2,rep,name=tags" json:"tags,omitempty" protobuf_key:"bytes,1,opt,name=key" protobuf_val:"bytes,2,opt,name=value"`
	Fields map[string]*Field `protobuf:"bytes,3,rep,name=fields" json:"fields,omitempty" protobuf_key:"bytes,1,opt,name=key" protobuf_val:"bytes,2,opt,name=value"`
	When   *api1.Timestamp   `protobuf:"bytes,4,opt,name=when" json:"when,omitempty"`
}

func (m *MetricPoint) Reset()                    { *m = MetricPoint{} }
func (m *MetricPoint) String() string            { return proto.CompactTextString(m) }
func (*MetricPoint) ProtoMessage()               {}
func (*MetricPoint) Descriptor() ([]byte, []int) { return fileDescriptor0, []int{1} }

func (m *MetricPoint) GetName() string {
	if m != nil {
		return m.Name
	}
	return ""
}

func (m *MetricPoint) GetTags() map[string]string {
	if m != nil {
		return m.Tags
	}
	return nil
}

func (m *MetricPoint) GetFields() map[string]*Field {
	if m != nil {
		return m.Fields
	}
	return nil
}

func (m *MetricPoint) GetWhen() *api1.Timestamp {
	if m != nil {
		return m.When
	}
	return nil
}

// MetricBundle is a set of metric points to be written to the same db.
type MetricBundle struct {
	DbName   string         `protobuf:"bytes,1,opt,name=dbName" json:"dbName,omitempty"`
	Reporter string         `protobuf:"bytes,2,opt,name=reporter" json:"reporter,omitempty"`
	Metrics  []*MetricPoint `protobuf:"bytes,3,rep,name=metrics" json:"metrics,omitempty"`
}

func (m *MetricBundle) Reset()                    { *m = MetricBundle{} }
func (m *MetricBundle) String() string            { return proto.CompactTextString(m) }
func (*MetricBundle) ProtoMessage()               {}
func (*MetricBundle) Descriptor() ([]byte, []int) { return fileDescriptor0, []int{2} }

func (m *MetricBundle) GetDbName() string {
	if m != nil {
		return m.DbName
	}
	return ""
}

func (m *MetricBundle) GetReporter() string {
	if m != nil {
		return m.Reporter
	}
	return ""
}

func (m *MetricBundle) GetMetrics() []*MetricPoint {
	if m != nil {
		return m.Metrics
	}
	return nil
}

func init() {
	proto.RegisterType((*Field)(nil), "metric.Field")
	proto.RegisterType((*MetricPoint)(nil), "metric.MetricPoint")
	proto.RegisterType((*MetricBundle)(nil), "metric.MetricBundle")
}

// Reference imports to suppress errors if they are not otherwise used.
var _ context.Context
var _ grpc.ClientConn

// This is a compile-time assertion to ensure that this generated file
// is compatible with the grpc package it is being compiled against.
const _ = grpc.SupportPackageIsVersion4

// Client API for MetricApi service

type MetricApiClient interface {
	WriteMetrics(ctx context.Context, in *MetricBundle, opts ...grpc.CallOption) (*api.Empty, error)
}

type metricApiClient struct {
	cc *grpc.ClientConn
}

func NewMetricApiClient(cc *grpc.ClientConn) MetricApiClient {
	return &metricApiClient{cc}
}

func (c *metricApiClient) WriteMetrics(ctx context.Context, in *MetricBundle, opts ...grpc.CallOption) (*api.Empty, error) {
	out := new(api.Empty)
	err := grpc.Invoke(ctx, "/metric.MetricApi/WriteMetrics", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

// Server API for MetricApi service

type MetricApiServer interface {
	WriteMetrics(context.Context, *MetricBundle) (*api.Empty, error)
}

func RegisterMetricApiServer(s *grpc.Server, srv MetricApiServer) {
	s.RegisterService(&_MetricApi_serviceDesc, srv)
}

func _MetricApi_WriteMetrics_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(MetricBundle)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetricApiServer).WriteMetrics(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/metric.MetricApi/WriteMetrics",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetricApiServer).WriteMetrics(ctx, req.(*MetricBundle))
	}
	return interceptor(ctx, in, info, handler)
}

var _MetricApi_serviceDesc = grpc.ServiceDesc{
	ServiceName: "metric.MetricApi",
	HandlerType: (*MetricApiServer)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "WriteMetrics",
			Handler:    _MetricApi_WriteMetrics_Handler,
		},
	},
	Streams:  []grpc.StreamDesc{},
	Metadata: "metric.proto",
}

func init() { proto.RegisterFile("metric.proto", fileDescriptor0) }

var fileDescriptor0 = []byte{
	// 476 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x7c, 0x52, 0xd1, 0x6e, 0x94, 0x40,
	0x14, 0x85, 0x85, 0xa5, 0xe5, 0xb2, 0x1a, 0x33, 0x6e, 0x1a, 0x42, 0x34, 0x12, 0x8c, 0x91, 0x97,
	0x42, 0x44, 0x63, 0x8d, 0x2f, 0xc6, 0x4d, 0xb6, 0xc1, 0x07, 0x4d, 0x83, 0x4d, 0x7c, 0x9e, 0x85,
	0x59, 0x3a, 0x11, 0x66, 0x10, 0x86, 0x36, 0xfb, 0x5f, 0x7e, 0xa0, 0x61, 0x06, 0x2a, 0x26, 0x6b,
	0xdf, 0xee, 0xb9, 0x73, 0xce, 0xbd, 0xe7, 0x1e, 0x80, 0x55, 0x4d, 0x44, 0x4b, 0xf3, 0xa8, 0x69,
	0xb9, 0xe0, 0xc8, 0x52, 0xc8, 0x7b, 0x56, 0x72, 0x5e, 0x56, 0x24, 0xc6, 0x0d, 0x8d, 0x31, 0x63,
	0x5c, 0x60, 0x41, 0x39, 0xeb, 0x14, 0xcb, 0xdb, 0x96, 0x54, 0xdc, 0xf4, 0xbb, 0x28, 0xe7, 0x75,
	0xdc, 0x10, 0xd6, 0x61, 0x56, 0xf0, 0xb8, 0xbb, 0x8b, 0x6f, 0x09, 0xa3, 0x39, 0x89, 0x7b, 0x41,
	0xab, 0x6e, 0x90, 0x96, 0x84, 0xcd, 0xd5, 0x31, 0x65, 0x79, 0xd5, 0x17, 0x64, 0x1a, 0xf3, 0xfa,
	0x3f, 0x63, 0x86, 0xa5, 0x39, 0xaf, 0x6b, 0xce, 0x46, 0xe2, 0xab, 0x07, 0x88, 0x35, 0x11, 0x78,
	0xa4, 0x9d, 0xcf, 0x68, 0x25, 0x2f, 0x79, 0x2c, 0xdb, 0xbb, 0x7e, 0x2f, 0x91, 0x04, 0xb2, 0x52,
	0xf4, 0xa0, 0x81, 0xe5, 0x25, 0x25, 0x55, 0x81, 0xce, 0x60, 0xf9, 0x85, 0x89, 0xf7, 0xef, 0x5c,
	0xdd, 0xd7, 0x43, 0x23, 0xd5, 0x32, 0x05, 0x91, 0x07, 0x27, 0x97, 0x15, 0xc7, 0xc3, 0xcb, 0xc2,
	0xd7, 0x43, 0x3d, 0xd5, 0xb2, 0xa9, 0x81, 0x5c, 0xb0, 0xbe, 0x8b, 0x96, 0xb2, 0xd2, 0x35, 0x7c,
	0x3d, 0xb4, 0x53, 0x2d, 0x1b, 0x31, 0x5a, 0x83, 0xb9, 0xe1, 0xbc, 0x72, 0x4d, 0x5f, 0x0f, 0x4f,
	0x53, 0x2d, 0x93, 0x68, 0x63, 0x80, 0xbe, 0x0f, 0x7e, 0x2f, 0xc0, 0xf9, 0x2a, 0x03, 0xbe, 0xe2,
	0x94, 0x09, 0x84, 0xc0, 0x64, 0xb8, 0x26, 0x72, 0xaf, 0x9d, 0xc9, 0x1a, 0xbd, 0x01, 0x53, 0xe0,
	0xb2, 0x73, 0x17, 0xbe, 0x11, 0x3a, 0xc9, 0xf3, 0x68, 0xfc, 0x3c, 0x33, 0x59, 0x74, 0x8d, 0xcb,
	0x6e, 0xcb, 0x44, 0x7b, 0xc8, 0x24, 0x15, 0x5d, 0x80, 0xb5, 0x1f, 0x0e, 0xe9, 0x5c, 0x43, 0x8a,
	0x5e, 0x1c, 0x13, 0xc9, 0x53, 0x47, 0xd9, 0x48, 0x47, 0x01, 0x98, 0x77, 0x37, 0x84, 0x49, 0xab,
	0x4e, 0xf2, 0x38, 0xc2, 0x0d, 0x8d, 0xae, 0x69, 0x4d, 0x3a, 0x81, 0xeb, 0x26, 0x93, 0x6f, 0xde,
	0x05, 0xd8, 0xf7, 0xfb, 0xd0, 0x13, 0x30, 0x7e, 0x92, 0xc3, 0xe8, 0x77, 0x28, 0xd1, 0x1a, 0x96,
	0xb7, 0xb8, 0xea, 0x89, 0x4c, 0xc8, 0xce, 0x14, 0xf8, 0xb8, 0xf8, 0xa0, 0x7b, 0x29, 0x38, 0xb3,
	0x9d, 0x47, 0xa4, 0x2f, 0xe7, 0x52, 0x27, 0x79, 0x34, 0xb9, 0x96, 0xaa, 0xd9, 0xa4, 0xe0, 0x17,
	0xac, 0xd4, 0x25, 0x9b, 0x9e, 0x15, 0x15, 0x41, 0x67, 0x60, 0x15, 0xbb, 0x6f, 0x7f, 0x83, 0x1b,
	0x11, 0xf2, 0xe0, 0xb4, 0x25, 0x0d, 0x6f, 0x05, 0x69, 0x47, 0x3b, 0xf7, 0x18, 0x9d, 0xc3, 0x89,
	0x1a, 0x3f, 0x85, 0xf4, 0xf4, 0x48, 0x48, 0xd9, 0xc4, 0x49, 0x3e, 0x81, 0xad, 0xfa, 0x9f, 0x1b,
	0x8a, 0x12, 0x58, 0xfd, 0x68, 0xa9, 0x20, 0xaa, 0xd3, 0xa1, 0xf5, 0xbf, 0x52, 0xe5, 0xca, 0x03,
	0x19, 0xdf, 0xb6, 0x6e, 0xc4, 0x21, 0xd0, 0xae, 0xf4, 0x9d, 0x25, 0xff, 0xb2, 0xb7, 0x7f, 0x02,
	0x00, 0x00, 0xff, 0xff, 0x70, 0x9d, 0x7b, 0xfc, 0x61, 0x03, 0x00, 0x00,
}
