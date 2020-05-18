// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: svc_search.proto

package search

import proto "github.com/gogo/protobuf/proto"
import fmt "fmt"
import math "math"
import _ "github.com/pensando/grpc-gateway/third_party/googleapis/google/api"
import _ "github.com/pensando/sw/venice/utils/apigen/annotations"
import _ "github.com/gogo/protobuf/gogoproto"
import _ "github.com/pensando/sw/api/generated/cluster"
import api "github.com/pensando/sw/api"

import (
	context "golang.org/x/net/context"
	grpc "google.golang.org/grpc"
)

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

// Reference imports to suppress errors if they are not otherwise used.
var _ context.Context
var _ grpc.ClientConn

// This is a compile-time assertion to ensure that this generated file
// is compatible with the grpc package it is being compiled against.
const _ = grpc.SupportPackageIsVersion4

// Client API for SearchV1 service

type SearchV1Client interface {
	AutoWatchSvcSearchV1(ctx context.Context, in *api.AggWatchOptions, opts ...grpc.CallOption) (SearchV1_AutoWatchSvcSearchV1Client, error)
	// Security Policy Query
	PolicyQuery(ctx context.Context, in *PolicySearchRequest, opts ...grpc.CallOption) (*PolicySearchResponse, error)
	// Structured or free-form search
	Query(ctx context.Context, in *SearchRequest, opts ...grpc.CallOption) (*SearchResponse, error)
}

type searchV1Client struct {
	cc *grpc.ClientConn
}

func NewSearchV1Client(cc *grpc.ClientConn) SearchV1Client {
	return &searchV1Client{cc}
}

func (c *searchV1Client) AutoWatchSvcSearchV1(ctx context.Context, in *api.AggWatchOptions, opts ...grpc.CallOption) (SearchV1_AutoWatchSvcSearchV1Client, error) {
	stream, err := grpc.NewClientStream(ctx, &_SearchV1_serviceDesc.Streams[0], c.cc, "/search.SearchV1/AutoWatchSvcSearchV1", opts...)
	if err != nil {
		return nil, err
	}
	x := &searchV1AutoWatchSvcSearchV1Client{stream}
	if err := x.ClientStream.SendMsg(in); err != nil {
		return nil, err
	}
	if err := x.ClientStream.CloseSend(); err != nil {
		return nil, err
	}
	return x, nil
}

type SearchV1_AutoWatchSvcSearchV1Client interface {
	Recv() (*api.WatchEventList, error)
	grpc.ClientStream
}

type searchV1AutoWatchSvcSearchV1Client struct {
	grpc.ClientStream
}

func (x *searchV1AutoWatchSvcSearchV1Client) Recv() (*api.WatchEventList, error) {
	m := new(api.WatchEventList)
	if err := x.ClientStream.RecvMsg(m); err != nil {
		return nil, err
	}
	return m, nil
}

func (c *searchV1Client) PolicyQuery(ctx context.Context, in *PolicySearchRequest, opts ...grpc.CallOption) (*PolicySearchResponse, error) {
	out := new(PolicySearchResponse)
	err := grpc.Invoke(ctx, "/search.SearchV1/PolicyQuery", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *searchV1Client) Query(ctx context.Context, in *SearchRequest, opts ...grpc.CallOption) (*SearchResponse, error) {
	out := new(SearchResponse)
	err := grpc.Invoke(ctx, "/search.SearchV1/Query", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

// Server API for SearchV1 service

type SearchV1Server interface {
	AutoWatchSvcSearchV1(*api.AggWatchOptions, SearchV1_AutoWatchSvcSearchV1Server) error
	// Security Policy Query
	PolicyQuery(context.Context, *PolicySearchRequest) (*PolicySearchResponse, error)
	// Structured or free-form search
	Query(context.Context, *SearchRequest) (*SearchResponse, error)
}

func RegisterSearchV1Server(s *grpc.Server, srv SearchV1Server) {
	s.RegisterService(&_SearchV1_serviceDesc, srv)
}

func _SearchV1_AutoWatchSvcSearchV1_Handler(srv interface{}, stream grpc.ServerStream) error {
	m := new(api.AggWatchOptions)
	if err := stream.RecvMsg(m); err != nil {
		return err
	}
	return srv.(SearchV1Server).AutoWatchSvcSearchV1(m, &searchV1AutoWatchSvcSearchV1Server{stream})
}

type SearchV1_AutoWatchSvcSearchV1Server interface {
	Send(*api.WatchEventList) error
	grpc.ServerStream
}

type searchV1AutoWatchSvcSearchV1Server struct {
	grpc.ServerStream
}

func (x *searchV1AutoWatchSvcSearchV1Server) Send(m *api.WatchEventList) error {
	return x.ServerStream.SendMsg(m)
}

func _SearchV1_PolicyQuery_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(PolicySearchRequest)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(SearchV1Server).PolicyQuery(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/search.SearchV1/PolicyQuery",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(SearchV1Server).PolicyQuery(ctx, req.(*PolicySearchRequest))
	}
	return interceptor(ctx, in, info, handler)
}

func _SearchV1_Query_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(SearchRequest)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(SearchV1Server).Query(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/search.SearchV1/Query",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(SearchV1Server).Query(ctx, req.(*SearchRequest))
	}
	return interceptor(ctx, in, info, handler)
}

var _SearchV1_serviceDesc = grpc.ServiceDesc{
	ServiceName: "search.SearchV1",
	HandlerType: (*SearchV1Server)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "PolicyQuery",
			Handler:    _SearchV1_PolicyQuery_Handler,
		},
		{
			MethodName: "Query",
			Handler:    _SearchV1_Query_Handler,
		},
	},
	Streams: []grpc.StreamDesc{
		{
			StreamName:    "AutoWatchSvcSearchV1",
			Handler:       _SearchV1_AutoWatchSvcSearchV1_Handler,
			ServerStreams: true,
		},
	},
	Metadata: "svc_search.proto",
}

func init() { proto.RegisterFile("svc_search.proto", fileDescriptorSvcSearch) }

var fileDescriptorSvcSearch = []byte{
	// 402 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x74, 0x92, 0x41, 0x8f, 0xd2, 0x40,
	0x1c, 0xc5, 0x5b, 0x12, 0x08, 0xa9, 0x18, 0xb5, 0x82, 0xc9, 0x14, 0xd2, 0x43, 0x8d, 0x07, 0x4d,
	0xe8, 0x88, 0xde, 0xbc, 0x61, 0xc2, 0xcd, 0x44, 0x94, 0x04, 0x13, 0x2f, 0x66, 0x18, 0xc6, 0x61,
	0x92, 0x32, 0x33, 0x30, 0xd3, 0x12, 0xae, 0x85, 0x4f, 0xa0, 0x37, 0x4e, 0x9e, 0x3d, 0x7a, 0xe2,
	0xe8, 0xd1, 0xa3, 0xc9, 0x7e, 0x81, 0x4d, 0xb3, 0x87, 0xfd, 0x18, 0x9b, 0x4e, 0x0b, 0x81, 0xcd,
	0xee, 0xad, 0xef, 0xf7, 0xff, 0xff, 0xdf, 0x7b, 0x69, 0xc6, 0x79, 0xac, 0x12, 0xfc, 0x4d, 0x11,
	0xb4, 0xc4, 0xb3, 0x50, 0x2e, 0x85, 0x16, 0x6e, 0xad, 0x50, 0x5e, 0x87, 0x0a, 0x41, 0x23, 0x02,
	0x91, 0x64, 0x10, 0x71, 0x2e, 0x34, 0xd2, 0x4c, 0x70, 0x55, 0x6c, 0x79, 0x03, 0xca, 0xf4, 0x2c,
	0x9e, 0x84, 0x58, 0xcc, 0xa1, 0x24, 0x5c, 0x21, 0x3e, 0x15, 0x50, 0xad, 0x60, 0x42, 0x38, 0xc3,
	0x04, 0xc6, 0x9a, 0x45, 0x2a, 0x3f, 0xa5, 0x84, 0x9f, 0x5e, 0x43, 0xc6, 0x71, 0x14, 0x4f, 0xc9,
	0xc1, 0xa6, 0x7b, 0x62, 0x43, 0x05, 0x15, 0xd0, 0xe0, 0x49, 0xfc, 0xdd, 0x28, 0x23, 0xcc, 0x57,
	0xb9, 0xde, 0xd0, 0x84, 0x23, 0xae, 0x0f, 0xea, 0xb4, 0xb7, 0xf7, 0xe2, 0x9e, 0x46, 0x79, 0xff,
	0x39, 0xd1, 0xa8, 0x58, 0x7b, 0xb3, 0xaf, 0x38, 0xf5, 0x91, 0xb9, 0x1b, 0xf7, 0xdc, 0xb1, 0xd3,
	0xec, 0xc7, 0x5a, 0x7c, 0x41, 0x1a, 0xcf, 0x46, 0x09, 0x3e, 0xf2, 0x66, 0x88, 0x24, 0x0b, 0xfb,
	0x94, 0x9a, 0xc9, 0x47, 0x69, 0xba, 0x7b, 0x4f, 0x0d, 0x35, 0x68, 0x90, 0x10, 0xae, 0x3f, 0x30,
	0xa5, 0x83, 0x27, 0x7f, 0xb6, 0xa0, 0xba, 0xca, 0xd9, 0x7e, 0x0b, 0xec, 0xbf, 0x5b, 0x60, 0xbd,
	0xb6, 0x5d, 0xe5, 0x3c, 0x18, 0x8a, 0x88, 0xe1, 0xf5, 0xa7, 0x98, 0x2c, 0xd7, 0x6e, 0x3b, 0x2c,
	0x9b, 0x16, 0xb0, 0x88, 0xf9, 0x4c, 0x16, 0x31, 0x51, 0xda, 0xeb, 0xdc, 0x3d, 0x54, 0x52, 0x70,
	0x45, 0x82, 0x97, 0xe9, 0xc5, 0xd5, 0xcf, 0xca, 0xf3, 0xe0, 0x21, 0x94, 0x66, 0xdc, 0x5d, 0xe4,
	0x8e, 0xef, 0xec, 0x57, 0x5f, 0x1f, 0xb9, 0xe7, 0xcc, 0x1d, 0x39, 0xd5, 0x22, 0xae, 0x75, 0x70,
	0x3c, 0x0f, 0x7a, 0x76, 0x1b, 0x97, 0x11, 0x6d, 0x13, 0xd1, 0x0a, 0x6a, 0xf0, 0xe8, 0x5d, 0x77,
	0x4b, 0xe1, 0x39, 0x3f, 0x36, 0xa0, 0x92, 0xf4, 0x76, 0x1b, 0x60, 0xbd, 0x0f, 0x77, 0x29, 0x68,
	0x48, 0xc2, 0xbb, 0x4a, 0xae, 0x69, 0x84, 0x94, 0xfa, 0x95, 0x02, 0xeb, 0x77, 0x0a, 0xca, 0x17,
	0xf3, 0x2f, 0xf3, 0xed, 0xff, 0x99, 0x6f, 0x5f, 0x66, 0xbe, 0x7d, 0x9d, 0xf9, 0xd6, 0xd0, 0x9e,
	0xd4, 0xcc, 0x3f, 0x7f, 0x7b, 0x13, 0x00, 0x00, 0xff, 0xff, 0x4c, 0xb7, 0x11, 0x59, 0x66, 0x02,
	0x00, 0x00,
}
