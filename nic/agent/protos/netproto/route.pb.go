// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: route.proto

package netproto

import proto "github.com/gogo/protobuf/proto"
import fmt "fmt"
import math "math"
import _ "google.golang.org/genproto/googleapis/api/annotations"
import _ "github.com/pensando/sw/venice/utils/apigen/annotations"
import _ "github.com/gogo/protobuf/gogoproto"
import api "github.com/pensando/sw/api"

import (
	context "golang.org/x/net/context"
	grpc "google.golang.org/grpc"
)

import io "io"

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

// Route object
type Route struct {
	api.TypeMeta   `protobuf:"bytes,1,opt,name=TypeMeta,embedded=TypeMeta" json:",inline"`
	api.ObjectMeta `protobuf:"bytes,2,opt,name=ObjectMeta,embedded=ObjectMeta" json:"meta,omitempty"`
	Spec           RouteSpec   `protobuf:"bytes,3,opt,name=Spec" json:"spec,omitempty"`
	Status         RouteStatus `protobuf:"bytes,4,opt,name=Status" json:"status,omitempty"`
}

func (m *Route) Reset()                    { *m = Route{} }
func (m *Route) String() string            { return proto.CompactTextString(m) }
func (*Route) ProtoMessage()               {}
func (*Route) Descriptor() ([]byte, []int) { return fileDescriptorRoute, []int{0} }

func (m *Route) GetSpec() RouteSpec {
	if m != nil {
		return m.Spec
	}
	return RouteSpec{}
}

func (m *Route) GetStatus() RouteStatus {
	if m != nil {
		return m.Status
	}
	return RouteStatus{}
}

// RouteSpec captures all the route configuration
type RouteSpec struct {
	// VrfName specifies the name of the VRF that the current Route belongs to
	VrfName string `protobuf:"bytes,1,opt,name=VrfName,proto3" json:"vrf-name,omitempty"`
	// CIDR based ip prefix.
	IPPrefix string `protobuf:"bytes,2,opt,name=IPPrefix,proto3" json:"ip-prefix,omitempty"`
	// Next Hop interface
	Interface string `protobuf:"bytes,3,opt,name=Interface,proto3" json:"interface,omitempty"`
	// Next Hop gateway IP. This should resolve to a valid endpoint. Required
	GatewayIP string `protobuf:"bytes,4,opt,name=GatewayIP,proto3" json:"gateway-ip,omitempty"`
}

func (m *RouteSpec) Reset()                    { *m = RouteSpec{} }
func (m *RouteSpec) String() string            { return proto.CompactTextString(m) }
func (*RouteSpec) ProtoMessage()               {}
func (*RouteSpec) Descriptor() ([]byte, []int) { return fileDescriptorRoute, []int{1} }

func (m *RouteSpec) GetVrfName() string {
	if m != nil {
		return m.VrfName
	}
	return ""
}

func (m *RouteSpec) GetIPPrefix() string {
	if m != nil {
		return m.IPPrefix
	}
	return ""
}

func (m *RouteSpec) GetInterface() string {
	if m != nil {
		return m.Interface
	}
	return ""
}

func (m *RouteSpec) GetGatewayIP() string {
	if m != nil {
		return m.GatewayIP
	}
	return ""
}

// RouteStatus captures the route status
type RouteStatus struct {
	// Route ID in the datapath
	RouteID uint64 `protobuf:"varint,1,opt,name=RouteID,proto3" json:"id,omitempty"`
}

func (m *RouteStatus) Reset()                    { *m = RouteStatus{} }
func (m *RouteStatus) String() string            { return proto.CompactTextString(m) }
func (*RouteStatus) ProtoMessage()               {}
func (*RouteStatus) Descriptor() ([]byte, []int) { return fileDescriptorRoute, []int{2} }

func (m *RouteStatus) GetRouteID() uint64 {
	if m != nil {
		return m.RouteID
	}
	return 0
}

type RouteList struct {
	Routes []*Route `protobuf:"bytes,1,rep,name=routes" json:"routes,omitempty"`
}

func (m *RouteList) Reset()                    { *m = RouteList{} }
func (m *RouteList) String() string            { return proto.CompactTextString(m) }
func (*RouteList) ProtoMessage()               {}
func (*RouteList) Descriptor() ([]byte, []int) { return fileDescriptorRoute, []int{3} }

func (m *RouteList) GetRoutes() []*Route {
	if m != nil {
		return m.Routes
	}
	return nil
}

// route watch event
type RouteEvent struct {
	EventType api.EventType `protobuf:"varint,1,opt,name=EventType,proto3,enum=api.EventType" json:"event-type,omitempty"`
	Route     Route         `protobuf:"bytes,2,opt,name=Route" json:"route,omitempty"`
}

func (m *RouteEvent) Reset()                    { *m = RouteEvent{} }
func (m *RouteEvent) String() string            { return proto.CompactTextString(m) }
func (*RouteEvent) ProtoMessage()               {}
func (*RouteEvent) Descriptor() ([]byte, []int) { return fileDescriptorRoute, []int{4} }

func (m *RouteEvent) GetEventType() api.EventType {
	if m != nil {
		return m.EventType
	}
	return api.EventType_CreateEvent
}

func (m *RouteEvent) GetRoute() Route {
	if m != nil {
		return m.Route
	}
	return Route{}
}

// route watch events batched
type RouteEventList struct {
	RouteEvents []*RouteEvent `protobuf:"bytes,1,rep,name=RouteEvents" json:"RouteEvents,omitempty"`
}

func (m *RouteEventList) Reset()                    { *m = RouteEventList{} }
func (m *RouteEventList) String() string            { return proto.CompactTextString(m) }
func (*RouteEventList) ProtoMessage()               {}
func (*RouteEventList) Descriptor() ([]byte, []int) { return fileDescriptorRoute, []int{5} }

func (m *RouteEventList) GetRouteEvents() []*RouteEvent {
	if m != nil {
		return m.RouteEvents
	}
	return nil
}

func init() {
	proto.RegisterType((*Route)(nil), "netproto.Route")
	proto.RegisterType((*RouteSpec)(nil), "netproto.RouteSpec")
	proto.RegisterType((*RouteStatus)(nil), "netproto.RouteStatus")
	proto.RegisterType((*RouteList)(nil), "netproto.RouteList")
	proto.RegisterType((*RouteEvent)(nil), "netproto.RouteEvent")
	proto.RegisterType((*RouteEventList)(nil), "netproto.RouteEventList")
}

// Reference imports to suppress errors if they are not otherwise used.
var _ context.Context
var _ grpc.ClientConn

// This is a compile-time assertion to ensure that this generated file
// is compatible with the grpc package it is being compiled against.
const _ = grpc.SupportPackageIsVersion4

// Client API for RouteApi service

type RouteApiClient interface {
	GetRoute(ctx context.Context, in *api.ObjectMeta, opts ...grpc.CallOption) (*Route, error)
	ListRoutes(ctx context.Context, in *api.ListWatchOptions, opts ...grpc.CallOption) (*RouteList, error)
	WatchRoutes(ctx context.Context, in *api.ListWatchOptions, opts ...grpc.CallOption) (RouteApi_WatchRoutesClient, error)
}

type routeApiClient struct {
	cc *grpc.ClientConn
}

func NewRouteApiClient(cc *grpc.ClientConn) RouteApiClient {
	return &routeApiClient{cc}
}

func (c *routeApiClient) GetRoute(ctx context.Context, in *api.ObjectMeta, opts ...grpc.CallOption) (*Route, error) {
	out := new(Route)
	err := grpc.Invoke(ctx, "/netproto.RouteApi/GetRoute", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *routeApiClient) ListRoutes(ctx context.Context, in *api.ListWatchOptions, opts ...grpc.CallOption) (*RouteList, error) {
	out := new(RouteList)
	err := grpc.Invoke(ctx, "/netproto.RouteApi/ListRoutes", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *routeApiClient) WatchRoutes(ctx context.Context, in *api.ListWatchOptions, opts ...grpc.CallOption) (RouteApi_WatchRoutesClient, error) {
	stream, err := grpc.NewClientStream(ctx, &_RouteApi_serviceDesc.Streams[0], c.cc, "/netproto.RouteApi/WatchRoutes", opts...)
	if err != nil {
		return nil, err
	}
	x := &routeApiWatchRoutesClient{stream}
	if err := x.ClientStream.SendMsg(in); err != nil {
		return nil, err
	}
	if err := x.ClientStream.CloseSend(); err != nil {
		return nil, err
	}
	return x, nil
}

type RouteApi_WatchRoutesClient interface {
	Recv() (*RouteEventList, error)
	grpc.ClientStream
}

type routeApiWatchRoutesClient struct {
	grpc.ClientStream
}

func (x *routeApiWatchRoutesClient) Recv() (*RouteEventList, error) {
	m := new(RouteEventList)
	if err := x.ClientStream.RecvMsg(m); err != nil {
		return nil, err
	}
	return m, nil
}

// Server API for RouteApi service

type RouteApiServer interface {
	GetRoute(context.Context, *api.ObjectMeta) (*Route, error)
	ListRoutes(context.Context, *api.ListWatchOptions) (*RouteList, error)
	WatchRoutes(*api.ListWatchOptions, RouteApi_WatchRoutesServer) error
}

func RegisterRouteApiServer(s *grpc.Server, srv RouteApiServer) {
	s.RegisterService(&_RouteApi_serviceDesc, srv)
}

func _RouteApi_GetRoute_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(api.ObjectMeta)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(RouteApiServer).GetRoute(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/netproto.RouteApi/GetRoute",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(RouteApiServer).GetRoute(ctx, req.(*api.ObjectMeta))
	}
	return interceptor(ctx, in, info, handler)
}

func _RouteApi_ListRoutes_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(api.ListWatchOptions)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(RouteApiServer).ListRoutes(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/netproto.RouteApi/ListRoutes",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(RouteApiServer).ListRoutes(ctx, req.(*api.ListWatchOptions))
	}
	return interceptor(ctx, in, info, handler)
}

func _RouteApi_WatchRoutes_Handler(srv interface{}, stream grpc.ServerStream) error {
	m := new(api.ListWatchOptions)
	if err := stream.RecvMsg(m); err != nil {
		return err
	}
	return srv.(RouteApiServer).WatchRoutes(m, &routeApiWatchRoutesServer{stream})
}

type RouteApi_WatchRoutesServer interface {
	Send(*RouteEventList) error
	grpc.ServerStream
}

type routeApiWatchRoutesServer struct {
	grpc.ServerStream
}

func (x *routeApiWatchRoutesServer) Send(m *RouteEventList) error {
	return x.ServerStream.SendMsg(m)
}

var _RouteApi_serviceDesc = grpc.ServiceDesc{
	ServiceName: "netproto.RouteApi",
	HandlerType: (*RouteApiServer)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "GetRoute",
			Handler:    _RouteApi_GetRoute_Handler,
		},
		{
			MethodName: "ListRoutes",
			Handler:    _RouteApi_ListRoutes_Handler,
		},
	},
	Streams: []grpc.StreamDesc{
		{
			StreamName:    "WatchRoutes",
			Handler:       _RouteApi_WatchRoutes_Handler,
			ServerStreams: true,
		},
	},
	Metadata: "route.proto",
}

func (m *Route) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *Route) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	dAtA[i] = 0xa
	i++
	i = encodeVarintRoute(dAtA, i, uint64(m.TypeMeta.Size()))
	n1, err := m.TypeMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n1
	dAtA[i] = 0x12
	i++
	i = encodeVarintRoute(dAtA, i, uint64(m.ObjectMeta.Size()))
	n2, err := m.ObjectMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n2
	dAtA[i] = 0x1a
	i++
	i = encodeVarintRoute(dAtA, i, uint64(m.Spec.Size()))
	n3, err := m.Spec.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n3
	dAtA[i] = 0x22
	i++
	i = encodeVarintRoute(dAtA, i, uint64(m.Status.Size()))
	n4, err := m.Status.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n4
	return i, nil
}

func (m *RouteSpec) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *RouteSpec) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.VrfName) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintRoute(dAtA, i, uint64(len(m.VrfName)))
		i += copy(dAtA[i:], m.VrfName)
	}
	if len(m.IPPrefix) > 0 {
		dAtA[i] = 0x12
		i++
		i = encodeVarintRoute(dAtA, i, uint64(len(m.IPPrefix)))
		i += copy(dAtA[i:], m.IPPrefix)
	}
	if len(m.Interface) > 0 {
		dAtA[i] = 0x1a
		i++
		i = encodeVarintRoute(dAtA, i, uint64(len(m.Interface)))
		i += copy(dAtA[i:], m.Interface)
	}
	if len(m.GatewayIP) > 0 {
		dAtA[i] = 0x22
		i++
		i = encodeVarintRoute(dAtA, i, uint64(len(m.GatewayIP)))
		i += copy(dAtA[i:], m.GatewayIP)
	}
	return i, nil
}

func (m *RouteStatus) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *RouteStatus) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if m.RouteID != 0 {
		dAtA[i] = 0x8
		i++
		i = encodeVarintRoute(dAtA, i, uint64(m.RouteID))
	}
	return i, nil
}

func (m *RouteList) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *RouteList) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Routes) > 0 {
		for _, msg := range m.Routes {
			dAtA[i] = 0xa
			i++
			i = encodeVarintRoute(dAtA, i, uint64(msg.Size()))
			n, err := msg.MarshalTo(dAtA[i:])
			if err != nil {
				return 0, err
			}
			i += n
		}
	}
	return i, nil
}

func (m *RouteEvent) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *RouteEvent) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if m.EventType != 0 {
		dAtA[i] = 0x8
		i++
		i = encodeVarintRoute(dAtA, i, uint64(m.EventType))
	}
	dAtA[i] = 0x12
	i++
	i = encodeVarintRoute(dAtA, i, uint64(m.Route.Size()))
	n5, err := m.Route.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n5
	return i, nil
}

func (m *RouteEventList) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *RouteEventList) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.RouteEvents) > 0 {
		for _, msg := range m.RouteEvents {
			dAtA[i] = 0xa
			i++
			i = encodeVarintRoute(dAtA, i, uint64(msg.Size()))
			n, err := msg.MarshalTo(dAtA[i:])
			if err != nil {
				return 0, err
			}
			i += n
		}
	}
	return i, nil
}

func encodeVarintRoute(dAtA []byte, offset int, v uint64) int {
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return offset + 1
}
func (m *Route) Size() (n int) {
	var l int
	_ = l
	l = m.TypeMeta.Size()
	n += 1 + l + sovRoute(uint64(l))
	l = m.ObjectMeta.Size()
	n += 1 + l + sovRoute(uint64(l))
	l = m.Spec.Size()
	n += 1 + l + sovRoute(uint64(l))
	l = m.Status.Size()
	n += 1 + l + sovRoute(uint64(l))
	return n
}

func (m *RouteSpec) Size() (n int) {
	var l int
	_ = l
	l = len(m.VrfName)
	if l > 0 {
		n += 1 + l + sovRoute(uint64(l))
	}
	l = len(m.IPPrefix)
	if l > 0 {
		n += 1 + l + sovRoute(uint64(l))
	}
	l = len(m.Interface)
	if l > 0 {
		n += 1 + l + sovRoute(uint64(l))
	}
	l = len(m.GatewayIP)
	if l > 0 {
		n += 1 + l + sovRoute(uint64(l))
	}
	return n
}

func (m *RouteStatus) Size() (n int) {
	var l int
	_ = l
	if m.RouteID != 0 {
		n += 1 + sovRoute(uint64(m.RouteID))
	}
	return n
}

func (m *RouteList) Size() (n int) {
	var l int
	_ = l
	if len(m.Routes) > 0 {
		for _, e := range m.Routes {
			l = e.Size()
			n += 1 + l + sovRoute(uint64(l))
		}
	}
	return n
}

func (m *RouteEvent) Size() (n int) {
	var l int
	_ = l
	if m.EventType != 0 {
		n += 1 + sovRoute(uint64(m.EventType))
	}
	l = m.Route.Size()
	n += 1 + l + sovRoute(uint64(l))
	return n
}

func (m *RouteEventList) Size() (n int) {
	var l int
	_ = l
	if len(m.RouteEvents) > 0 {
		for _, e := range m.RouteEvents {
			l = e.Size()
			n += 1 + l + sovRoute(uint64(l))
		}
	}
	return n
}

func sovRoute(x uint64) (n int) {
	for {
		n++
		x >>= 7
		if x == 0 {
			break
		}
	}
	return n
}
func sozRoute(x uint64) (n int) {
	return sovRoute(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *Route) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowRoute
			}
			if iNdEx >= l {
				return io.ErrUnexpectedEOF
			}
			b := dAtA[iNdEx]
			iNdEx++
			wire |= (uint64(b) & 0x7F) << shift
			if b < 0x80 {
				break
			}
		}
		fieldNum := int32(wire >> 3)
		wireType := int(wire & 0x7)
		if wireType == 4 {
			return fmt.Errorf("proto: Route: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: Route: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field TypeMeta", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowRoute
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				msglen |= (int(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			if msglen < 0 {
				return ErrInvalidLengthRoute
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if err := m.TypeMeta.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field ObjectMeta", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowRoute
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				msglen |= (int(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			if msglen < 0 {
				return ErrInvalidLengthRoute
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if err := m.ObjectMeta.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		case 3:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Spec", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowRoute
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				msglen |= (int(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			if msglen < 0 {
				return ErrInvalidLengthRoute
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if err := m.Spec.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		case 4:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Status", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowRoute
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				msglen |= (int(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			if msglen < 0 {
				return ErrInvalidLengthRoute
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if err := m.Status.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipRoute(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthRoute
			}
			if (iNdEx + skippy) > l {
				return io.ErrUnexpectedEOF
			}
			iNdEx += skippy
		}
	}

	if iNdEx > l {
		return io.ErrUnexpectedEOF
	}
	return nil
}
func (m *RouteSpec) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowRoute
			}
			if iNdEx >= l {
				return io.ErrUnexpectedEOF
			}
			b := dAtA[iNdEx]
			iNdEx++
			wire |= (uint64(b) & 0x7F) << shift
			if b < 0x80 {
				break
			}
		}
		fieldNum := int32(wire >> 3)
		wireType := int(wire & 0x7)
		if wireType == 4 {
			return fmt.Errorf("proto: RouteSpec: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: RouteSpec: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field VrfName", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowRoute
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				stringLen |= (uint64(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			intStringLen := int(stringLen)
			if intStringLen < 0 {
				return ErrInvalidLengthRoute
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.VrfName = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field IPPrefix", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowRoute
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				stringLen |= (uint64(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			intStringLen := int(stringLen)
			if intStringLen < 0 {
				return ErrInvalidLengthRoute
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.IPPrefix = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 3:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Interface", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowRoute
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				stringLen |= (uint64(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			intStringLen := int(stringLen)
			if intStringLen < 0 {
				return ErrInvalidLengthRoute
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Interface = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 4:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field GatewayIP", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowRoute
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				stringLen |= (uint64(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			intStringLen := int(stringLen)
			if intStringLen < 0 {
				return ErrInvalidLengthRoute
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.GatewayIP = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipRoute(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthRoute
			}
			if (iNdEx + skippy) > l {
				return io.ErrUnexpectedEOF
			}
			iNdEx += skippy
		}
	}

	if iNdEx > l {
		return io.ErrUnexpectedEOF
	}
	return nil
}
func (m *RouteStatus) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowRoute
			}
			if iNdEx >= l {
				return io.ErrUnexpectedEOF
			}
			b := dAtA[iNdEx]
			iNdEx++
			wire |= (uint64(b) & 0x7F) << shift
			if b < 0x80 {
				break
			}
		}
		fieldNum := int32(wire >> 3)
		wireType := int(wire & 0x7)
		if wireType == 4 {
			return fmt.Errorf("proto: RouteStatus: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: RouteStatus: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field RouteID", wireType)
			}
			m.RouteID = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowRoute
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.RouteID |= (uint64(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		default:
			iNdEx = preIndex
			skippy, err := skipRoute(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthRoute
			}
			if (iNdEx + skippy) > l {
				return io.ErrUnexpectedEOF
			}
			iNdEx += skippy
		}
	}

	if iNdEx > l {
		return io.ErrUnexpectedEOF
	}
	return nil
}
func (m *RouteList) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowRoute
			}
			if iNdEx >= l {
				return io.ErrUnexpectedEOF
			}
			b := dAtA[iNdEx]
			iNdEx++
			wire |= (uint64(b) & 0x7F) << shift
			if b < 0x80 {
				break
			}
		}
		fieldNum := int32(wire >> 3)
		wireType := int(wire & 0x7)
		if wireType == 4 {
			return fmt.Errorf("proto: RouteList: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: RouteList: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Routes", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowRoute
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				msglen |= (int(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			if msglen < 0 {
				return ErrInvalidLengthRoute
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Routes = append(m.Routes, &Route{})
			if err := m.Routes[len(m.Routes)-1].Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipRoute(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthRoute
			}
			if (iNdEx + skippy) > l {
				return io.ErrUnexpectedEOF
			}
			iNdEx += skippy
		}
	}

	if iNdEx > l {
		return io.ErrUnexpectedEOF
	}
	return nil
}
func (m *RouteEvent) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowRoute
			}
			if iNdEx >= l {
				return io.ErrUnexpectedEOF
			}
			b := dAtA[iNdEx]
			iNdEx++
			wire |= (uint64(b) & 0x7F) << shift
			if b < 0x80 {
				break
			}
		}
		fieldNum := int32(wire >> 3)
		wireType := int(wire & 0x7)
		if wireType == 4 {
			return fmt.Errorf("proto: RouteEvent: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: RouteEvent: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field EventType", wireType)
			}
			m.EventType = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowRoute
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.EventType |= (api.EventType(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Route", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowRoute
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				msglen |= (int(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			if msglen < 0 {
				return ErrInvalidLengthRoute
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if err := m.Route.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipRoute(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthRoute
			}
			if (iNdEx + skippy) > l {
				return io.ErrUnexpectedEOF
			}
			iNdEx += skippy
		}
	}

	if iNdEx > l {
		return io.ErrUnexpectedEOF
	}
	return nil
}
func (m *RouteEventList) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowRoute
			}
			if iNdEx >= l {
				return io.ErrUnexpectedEOF
			}
			b := dAtA[iNdEx]
			iNdEx++
			wire |= (uint64(b) & 0x7F) << shift
			if b < 0x80 {
				break
			}
		}
		fieldNum := int32(wire >> 3)
		wireType := int(wire & 0x7)
		if wireType == 4 {
			return fmt.Errorf("proto: RouteEventList: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: RouteEventList: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field RouteEvents", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowRoute
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				msglen |= (int(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			if msglen < 0 {
				return ErrInvalidLengthRoute
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.RouteEvents = append(m.RouteEvents, &RouteEvent{})
			if err := m.RouteEvents[len(m.RouteEvents)-1].Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipRoute(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthRoute
			}
			if (iNdEx + skippy) > l {
				return io.ErrUnexpectedEOF
			}
			iNdEx += skippy
		}
	}

	if iNdEx > l {
		return io.ErrUnexpectedEOF
	}
	return nil
}
func skipRoute(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowRoute
			}
			if iNdEx >= l {
				return 0, io.ErrUnexpectedEOF
			}
			b := dAtA[iNdEx]
			iNdEx++
			wire |= (uint64(b) & 0x7F) << shift
			if b < 0x80 {
				break
			}
		}
		wireType := int(wire & 0x7)
		switch wireType {
		case 0:
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return 0, ErrIntOverflowRoute
				}
				if iNdEx >= l {
					return 0, io.ErrUnexpectedEOF
				}
				iNdEx++
				if dAtA[iNdEx-1] < 0x80 {
					break
				}
			}
			return iNdEx, nil
		case 1:
			iNdEx += 8
			return iNdEx, nil
		case 2:
			var length int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return 0, ErrIntOverflowRoute
				}
				if iNdEx >= l {
					return 0, io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				length |= (int(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			iNdEx += length
			if length < 0 {
				return 0, ErrInvalidLengthRoute
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowRoute
					}
					if iNdEx >= l {
						return 0, io.ErrUnexpectedEOF
					}
					b := dAtA[iNdEx]
					iNdEx++
					innerWire |= (uint64(b) & 0x7F) << shift
					if b < 0x80 {
						break
					}
				}
				innerWireType := int(innerWire & 0x7)
				if innerWireType == 4 {
					break
				}
				next, err := skipRoute(dAtA[start:])
				if err != nil {
					return 0, err
				}
				iNdEx = start + next
			}
			return iNdEx, nil
		case 4:
			return iNdEx, nil
		case 5:
			iNdEx += 4
			return iNdEx, nil
		default:
			return 0, fmt.Errorf("proto: illegal wireType %d", wireType)
		}
	}
	panic("unreachable")
}

var (
	ErrInvalidLengthRoute = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowRoute   = fmt.Errorf("proto: integer overflow")
)

func init() { proto.RegisterFile("route.proto", fileDescriptorRoute) }

var fileDescriptorRoute = []byte{
	// 697 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x7c, 0x54, 0xdd, 0x4e, 0x13, 0x41,
	0x14, 0x66, 0xa1, 0x96, 0x76, 0x8a, 0x85, 0x0c, 0x7f, 0xb5, 0x31, 0x94, 0x6c, 0x62, 0x34, 0x86,
	0xee, 0x12, 0x50, 0x12, 0x4d, 0xc4, 0xb8, 0x11, 0xb1, 0xf1, 0x87, 0xa6, 0x10, 0xbd, 0x9e, 0x6e,
	0x4f, 0xcb, 0x68, 0x3b, 0x3b, 0xe9, 0x4e, 0xc1, 0xc6, 0x70, 0xe5, 0x2b, 0x78, 0xed, 0x0b, 0x78,
	0xe7, 0x53, 0x70, 0xc9, 0x13, 0x34, 0xca, 0x65, 0x9f, 0xc2, 0xcc, 0xd9, 0xdd, 0x76, 0x5a, 0x7f,
	0xee, 0xf6, 0x7c, 0xf3, 0x7d, 0xdf, 0xf9, 0xd9, 0x33, 0x43, 0x72, 0xdd, 0xa0, 0xa7, 0xc0, 0x91,
	0xdd, 0x40, 0x05, 0x34, 0x23, 0x40, 0xe1, 0x57, 0xf1, 0x76, 0x2b, 0x08, 0x5a, 0x6d, 0x70, 0x99,
	0xe4, 0x2e, 0x13, 0x22, 0x50, 0x4c, 0xf1, 0x40, 0x84, 0x11, 0xaf, 0x78, 0xd0, 0xe2, 0xea, 0xb4,
	0x57, 0x77, 0xfc, 0xa0, 0xe3, 0x4a, 0x10, 0x21, 0x13, 0x8d, 0xc0, 0x0d, 0xcf, 0xdd, 0x33, 0x10,
	0xdc, 0x07, 0xb7, 0xa7, 0x78, 0x3b, 0xd4, 0xd2, 0x16, 0x08, 0x53, 0xed, 0x72, 0xe1, 0xb7, 0x7b,
	0x0d, 0x48, 0x6c, 0xca, 0x86, 0x4d, 0x2b, 0x68, 0x05, 0x2e, 0xc2, 0xf5, 0x5e, 0x13, 0x23, 0x0c,
	0xf0, 0x2b, 0xa6, 0xdf, 0xf9, 0x47, 0x56, 0x5d, 0x63, 0x07, 0x14, 0x8b, 0x68, 0xf6, 0xb7, 0x59,
	0x72, 0xa3, 0xa6, 0x9b, 0xa2, 0xfb, 0x24, 0x73, 0xd2, 0x97, 0xf0, 0x06, 0x14, 0x2b, 0x58, 0x9b,
	0xd6, 0xbd, 0xdc, 0xce, 0x4d, 0x87, 0x49, 0xee, 0x24, 0xa0, 0xb7, 0x7c, 0x39, 0x28, 0xcd, 0x5c,
	0x0d, 0x4a, 0xd6, 0x70, 0x50, 0x9a, 0xdf, 0xe2, 0xa2, 0xcd, 0x05, 0xd4, 0x46, 0x1a, 0xfa, 0x8a,
	0x90, 0xa3, 0xfa, 0x07, 0xf0, 0x15, 0x3a, 0xcc, 0xa2, 0xc3, 0x22, 0x3a, 0x8c, 0x61, 0xaf, 0x68,
	0x78, 0xe4, 0x75, 0x19, 0x5b, 0x41, 0x87, 0x2b, 0xe8, 0x48, 0xd5, 0xaf, 0x19, 0x72, 0xfa, 0x94,
	0xa4, 0x8e, 0x25, 0xf8, 0x85, 0x39, 0xb4, 0x59, 0x76, 0x92, 0x51, 0x3b, 0x58, 0xab, 0x3e, 0xf2,
	0xd6, 0xb4, 0x95, 0xb6, 0x09, 0x25, 0xf8, 0x86, 0x0d, 0x0a, 0xe9, 0x21, 0x49, 0x1f, 0x2b, 0xa6,
	0x7a, 0x61, 0x21, 0x85, 0x16, 0xab, 0xd3, 0x16, 0x78, 0xe8, 0x15, 0x62, 0x93, 0xa5, 0x10, 0x63,
	0xc3, 0x26, 0x96, 0xdb, 0xbf, 0x2c, 0x92, 0x1d, 0x25, 0xa5, 0xdb, 0x64, 0xfe, 0x5d, 0xb7, 0xf9,
	0x96, 0x75, 0x00, 0x67, 0x94, 0xf5, 0xd6, 0x86, 0x83, 0x12, 0x3d, 0xeb, 0x36, 0xcb, 0x82, 0x75,
	0xc0, 0x90, 0x27, 0x34, 0xba, 0x4b, 0x32, 0x95, 0x6a, 0xb5, 0x0b, 0x4d, 0xfe, 0x09, 0x87, 0x92,
	0xf5, 0xd6, 0x87, 0x83, 0xd2, 0x32, 0x97, 0x65, 0x89, 0xa0, 0xa1, 0x19, 0x11, 0xe9, 0x43, 0x92,
	0xad, 0x08, 0x05, 0xdd, 0x26, 0xf3, 0x01, 0x67, 0x90, 0xa8, 0x12, 0xd0, 0x50, 0x8d, 0x99, 0x74,
	0x8f, 0x64, 0x0f, 0x99, 0x82, 0x73, 0xd6, 0xaf, 0x54, 0xb1, 0xef, 0xac, 0x57, 0x18, 0x0e, 0x4a,
	0x2b, 0xad, 0x08, 0x2c, 0x73, 0x69, 0xea, 0x46, 0x54, 0xfb, 0x11, 0xc9, 0x19, 0x43, 0xa1, 0xf7,
	0xc9, 0x3c, 0x86, 0x95, 0xe7, 0xd8, 0x64, 0xca, 0x5b, 0x1a, 0x0e, 0x4a, 0x0b, 0xbc, 0x61, 0xb6,
	0x17, 0x13, 0xec, 0x07, 0xf1, 0x74, 0x5e, 0xf3, 0x50, 0xd1, 0xbb, 0x24, 0x8d, 0x17, 0x24, 0x2c,
	0x58, 0x9b, 0x73, 0xf8, 0xfb, 0x27, 0x87, 0x5e, 0x8b, 0x8f, 0xed, 0xaf, 0x16, 0x21, 0x88, 0x1c,
	0x9c, 0x81, 0x50, 0xf4, 0x05, 0xc9, 0xe2, 0x87, 0xde, 0x25, 0x4c, 0x99, 0xdf, 0xc9, 0xe3, 0xe6,
	0x8c, 0xd0, 0xa8, 0x0f, 0xd0, 0x61, 0x59, 0xf5, 0xe5, 0x44, 0xff, 0x23, 0x12, 0xdd, 0x8f, 0x77,
	0x79, 0xb4, 0x7d, 0x93, 0xe9, 0xbd, 0xf5, 0xf8, 0x6f, 0x2f, 0x62, 0x19, 0x86, 0x47, 0x24, 0xb3,
	0x5f, 0x92, 0xfc, 0xb8, 0x2a, 0xec, 0x68, 0x2f, 0x9e, 0x0c, 0x22, 0x49, 0x5b, 0x2b, 0x53, 0xbe,
	0x78, 0x58, 0x33, 0x89, 0x3b, 0xdf, 0x67, 0x49, 0x06, 0xe3, 0x67, 0x92, 0x53, 0x87, 0x64, 0x0e,
	0x41, 0x45, 0xb7, 0x6c, 0xfa, 0x46, 0x14, 0xa7, 0x8b, 0xb4, 0x67, 0xe8, 0x63, 0x42, 0x74, 0x72,
	0x0c, 0x43, 0xba, 0x8a, 0x0a, 0x0d, 0xbc, 0x67, 0xca, 0x3f, 0x3d, 0x92, 0xf8, 0x3a, 0x14, 0xa7,
	0xef, 0x84, 0x26, 0xd8, 0x33, 0xd4, 0x23, 0x39, 0xa4, 0xfd, 0x5f, 0x5c, 0xf8, 0x5b, 0x07, 0x91,
	0xc3, 0xb6, 0x55, 0x0c, 0x7e, 0x7c, 0xb9, 0xf5, 0x31, 0x79, 0x16, 0x52, 0x6d, 0x3d, 0x87, 0x94,
	0x0c, 0x42, 0x45, 0xd3, 0x0d, 0x68, 0x83, 0x02, 0x3a, 0x27, 0x7b, 0xaa, 0xf8, 0xc4, 0xfd, 0x3c,
	0x6e, 0xc3, 0x39, 0x01, 0xc1, 0x84, 0xba, 0x98, 0xc0, 0xf4, 0xfe, 0x87, 0x92, 0xf9, 0xf0, 0x27,
	0x7c, 0x61, 0x13, 0xfd, 0x10, 0x45, 0xeb, 0xe0, 0x2d, 0x5c, 0x5e, 0x6f, 0x58, 0x57, 0xd7, 0x1b,
	0xd6, 0xcf, 0xeb, 0x0d, 0xab, 0x6a, 0xd5, 0xd3, 0x58, 0xda, 0xee, 0xef, 0x00, 0x00, 0x00, 0xff,
	0xff, 0x24, 0x56, 0x1d, 0xee, 0x6f, 0x05, 0x00, 0x00,
}
