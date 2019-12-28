// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: svc_orchestration.proto

package orchestration

import proto "github.com/gogo/protobuf/proto"
import fmt "fmt"
import math "math"
import _ "github.com/pensando/grpc-gateway/third_party/googleapis/google/api"
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

// AutoMsgOrchestratorWatchHelper is a wrapper object for watch events for Orchestrator objects
type AutoMsgOrchestratorWatchHelper struct {
	Events []*AutoMsgOrchestratorWatchHelper_WatchEvent `protobuf:"bytes,1,rep,name=Events,json=events" json:"events"`
}

func (m *AutoMsgOrchestratorWatchHelper) Reset()         { *m = AutoMsgOrchestratorWatchHelper{} }
func (m *AutoMsgOrchestratorWatchHelper) String() string { return proto.CompactTextString(m) }
func (*AutoMsgOrchestratorWatchHelper) ProtoMessage()    {}
func (*AutoMsgOrchestratorWatchHelper) Descriptor() ([]byte, []int) {
	return fileDescriptorSvcOrchestration, []int{0}
}

func (m *AutoMsgOrchestratorWatchHelper) GetEvents() []*AutoMsgOrchestratorWatchHelper_WatchEvent {
	if m != nil {
		return m.Events
	}
	return nil
}

type AutoMsgOrchestratorWatchHelper_WatchEvent struct {
	Type   string        `protobuf:"bytes,1,opt,name=Type,proto3" json:"type,omitempty"`
	Object *Orchestrator `protobuf:"bytes,2,opt,name=Object" json:"object,omitempty"`
}

func (m *AutoMsgOrchestratorWatchHelper_WatchEvent) Reset() {
	*m = AutoMsgOrchestratorWatchHelper_WatchEvent{}
}
func (m *AutoMsgOrchestratorWatchHelper_WatchEvent) String() string { return proto.CompactTextString(m) }
func (*AutoMsgOrchestratorWatchHelper_WatchEvent) ProtoMessage()    {}
func (*AutoMsgOrchestratorWatchHelper_WatchEvent) Descriptor() ([]byte, []int) {
	return fileDescriptorSvcOrchestration, []int{0, 0}
}

func (m *AutoMsgOrchestratorWatchHelper_WatchEvent) GetType() string {
	if m != nil {
		return m.Type
	}
	return ""
}

func (m *AutoMsgOrchestratorWatchHelper_WatchEvent) GetObject() *Orchestrator {
	if m != nil {
		return m.Object
	}
	return nil
}

// OrchestratorList is a container object for list of Orchestrator objects
type OrchestratorList struct {
	api.TypeMeta `protobuf:"bytes,2,opt,name=T,json=,inline,embedded=T" json:",inline"`
	api.ListMeta `protobuf:"bytes,3,opt,name=ListMeta,json=list-meta,inline,embedded=ListMeta" json:"list-meta,inline"`
	// List of Orchestrator objects
	Items []*Orchestrator `protobuf:"bytes,4,rep,name=Items,json=items" json:"items"`
}

func (m *OrchestratorList) Reset()                    { *m = OrchestratorList{} }
func (m *OrchestratorList) String() string            { return proto.CompactTextString(m) }
func (*OrchestratorList) ProtoMessage()               {}
func (*OrchestratorList) Descriptor() ([]byte, []int) { return fileDescriptorSvcOrchestration, []int{1} }

func (m *OrchestratorList) GetItems() []*Orchestrator {
	if m != nil {
		return m.Items
	}
	return nil
}

func init() {
	proto.RegisterType((*AutoMsgOrchestratorWatchHelper)(nil), "orchestration.AutoMsgOrchestratorWatchHelper")
	proto.RegisterType((*AutoMsgOrchestratorWatchHelper_WatchEvent)(nil), "orchestration.AutoMsgOrchestratorWatchHelper.WatchEvent")
	proto.RegisterType((*OrchestratorList)(nil), "orchestration.OrchestratorList")
}

// Reference imports to suppress errors if they are not otherwise used.
var _ context.Context
var _ grpc.ClientConn

// This is a compile-time assertion to ensure that this generated file
// is compatible with the grpc package it is being compiled against.
const _ = grpc.SupportPackageIsVersion4

// Client API for OrchestratorV1 service

type OrchestratorV1Client interface {
	// Create Orchestrator object
	AutoAddOrchestrator(ctx context.Context, in *Orchestrator, opts ...grpc.CallOption) (*Orchestrator, error)
	// Delete Orchestrator object
	AutoDeleteOrchestrator(ctx context.Context, in *Orchestrator, opts ...grpc.CallOption) (*Orchestrator, error)
	// Get Orchestrator object
	AutoGetOrchestrator(ctx context.Context, in *Orchestrator, opts ...grpc.CallOption) (*Orchestrator, error)
	// List Orchestrator objects
	AutoListOrchestrator(ctx context.Context, in *api.ListWatchOptions, opts ...grpc.CallOption) (*OrchestratorList, error)
	// Update Orchestrator object
	AutoUpdateOrchestrator(ctx context.Context, in *Orchestrator, opts ...grpc.CallOption) (*Orchestrator, error)
	// Watch Orchestrator objects. Supports WebSockets or HTTP long poll
	AutoWatchOrchestrator(ctx context.Context, in *api.ListWatchOptions, opts ...grpc.CallOption) (OrchestratorV1_AutoWatchOrchestratorClient, error)
	AutoWatchSvcOrchestratorV1(ctx context.Context, in *api.ListWatchOptions, opts ...grpc.CallOption) (OrchestratorV1_AutoWatchSvcOrchestratorV1Client, error)
}

type orchestratorV1Client struct {
	cc *grpc.ClientConn
}

func NewOrchestratorV1Client(cc *grpc.ClientConn) OrchestratorV1Client {
	return &orchestratorV1Client{cc}
}

func (c *orchestratorV1Client) AutoAddOrchestrator(ctx context.Context, in *Orchestrator, opts ...grpc.CallOption) (*Orchestrator, error) {
	out := new(Orchestrator)
	err := grpc.Invoke(ctx, "/orchestration.OrchestratorV1/AutoAddOrchestrator", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *orchestratorV1Client) AutoDeleteOrchestrator(ctx context.Context, in *Orchestrator, opts ...grpc.CallOption) (*Orchestrator, error) {
	out := new(Orchestrator)
	err := grpc.Invoke(ctx, "/orchestration.OrchestratorV1/AutoDeleteOrchestrator", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *orchestratorV1Client) AutoGetOrchestrator(ctx context.Context, in *Orchestrator, opts ...grpc.CallOption) (*Orchestrator, error) {
	out := new(Orchestrator)
	err := grpc.Invoke(ctx, "/orchestration.OrchestratorV1/AutoGetOrchestrator", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *orchestratorV1Client) AutoListOrchestrator(ctx context.Context, in *api.ListWatchOptions, opts ...grpc.CallOption) (*OrchestratorList, error) {
	out := new(OrchestratorList)
	err := grpc.Invoke(ctx, "/orchestration.OrchestratorV1/AutoListOrchestrator", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *orchestratorV1Client) AutoUpdateOrchestrator(ctx context.Context, in *Orchestrator, opts ...grpc.CallOption) (*Orchestrator, error) {
	out := new(Orchestrator)
	err := grpc.Invoke(ctx, "/orchestration.OrchestratorV1/AutoUpdateOrchestrator", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *orchestratorV1Client) AutoWatchOrchestrator(ctx context.Context, in *api.ListWatchOptions, opts ...grpc.CallOption) (OrchestratorV1_AutoWatchOrchestratorClient, error) {
	stream, err := grpc.NewClientStream(ctx, &_OrchestratorV1_serviceDesc.Streams[0], c.cc, "/orchestration.OrchestratorV1/AutoWatchOrchestrator", opts...)
	if err != nil {
		return nil, err
	}
	x := &orchestratorV1AutoWatchOrchestratorClient{stream}
	if err := x.ClientStream.SendMsg(in); err != nil {
		return nil, err
	}
	if err := x.ClientStream.CloseSend(); err != nil {
		return nil, err
	}
	return x, nil
}

type OrchestratorV1_AutoWatchOrchestratorClient interface {
	Recv() (*AutoMsgOrchestratorWatchHelper, error)
	grpc.ClientStream
}

type orchestratorV1AutoWatchOrchestratorClient struct {
	grpc.ClientStream
}

func (x *orchestratorV1AutoWatchOrchestratorClient) Recv() (*AutoMsgOrchestratorWatchHelper, error) {
	m := new(AutoMsgOrchestratorWatchHelper)
	if err := x.ClientStream.RecvMsg(m); err != nil {
		return nil, err
	}
	return m, nil
}

func (c *orchestratorV1Client) AutoWatchSvcOrchestratorV1(ctx context.Context, in *api.ListWatchOptions, opts ...grpc.CallOption) (OrchestratorV1_AutoWatchSvcOrchestratorV1Client, error) {
	stream, err := grpc.NewClientStream(ctx, &_OrchestratorV1_serviceDesc.Streams[1], c.cc, "/orchestration.OrchestratorV1/AutoWatchSvcOrchestratorV1", opts...)
	if err != nil {
		return nil, err
	}
	x := &orchestratorV1AutoWatchSvcOrchestratorV1Client{stream}
	if err := x.ClientStream.SendMsg(in); err != nil {
		return nil, err
	}
	if err := x.ClientStream.CloseSend(); err != nil {
		return nil, err
	}
	return x, nil
}

type OrchestratorV1_AutoWatchSvcOrchestratorV1Client interface {
	Recv() (*api.WatchEventList, error)
	grpc.ClientStream
}

type orchestratorV1AutoWatchSvcOrchestratorV1Client struct {
	grpc.ClientStream
}

func (x *orchestratorV1AutoWatchSvcOrchestratorV1Client) Recv() (*api.WatchEventList, error) {
	m := new(api.WatchEventList)
	if err := x.ClientStream.RecvMsg(m); err != nil {
		return nil, err
	}
	return m, nil
}

// Server API for OrchestratorV1 service

type OrchestratorV1Server interface {
	// Create Orchestrator object
	AutoAddOrchestrator(context.Context, *Orchestrator) (*Orchestrator, error)
	// Delete Orchestrator object
	AutoDeleteOrchestrator(context.Context, *Orchestrator) (*Orchestrator, error)
	// Get Orchestrator object
	AutoGetOrchestrator(context.Context, *Orchestrator) (*Orchestrator, error)
	// List Orchestrator objects
	AutoListOrchestrator(context.Context, *api.ListWatchOptions) (*OrchestratorList, error)
	// Update Orchestrator object
	AutoUpdateOrchestrator(context.Context, *Orchestrator) (*Orchestrator, error)
	// Watch Orchestrator objects. Supports WebSockets or HTTP long poll
	AutoWatchOrchestrator(*api.ListWatchOptions, OrchestratorV1_AutoWatchOrchestratorServer) error
	AutoWatchSvcOrchestratorV1(*api.ListWatchOptions, OrchestratorV1_AutoWatchSvcOrchestratorV1Server) error
}

func RegisterOrchestratorV1Server(s *grpc.Server, srv OrchestratorV1Server) {
	s.RegisterService(&_OrchestratorV1_serviceDesc, srv)
}

func _OrchestratorV1_AutoAddOrchestrator_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(Orchestrator)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(OrchestratorV1Server).AutoAddOrchestrator(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/orchestration.OrchestratorV1/AutoAddOrchestrator",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(OrchestratorV1Server).AutoAddOrchestrator(ctx, req.(*Orchestrator))
	}
	return interceptor(ctx, in, info, handler)
}

func _OrchestratorV1_AutoDeleteOrchestrator_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(Orchestrator)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(OrchestratorV1Server).AutoDeleteOrchestrator(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/orchestration.OrchestratorV1/AutoDeleteOrchestrator",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(OrchestratorV1Server).AutoDeleteOrchestrator(ctx, req.(*Orchestrator))
	}
	return interceptor(ctx, in, info, handler)
}

func _OrchestratorV1_AutoGetOrchestrator_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(Orchestrator)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(OrchestratorV1Server).AutoGetOrchestrator(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/orchestration.OrchestratorV1/AutoGetOrchestrator",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(OrchestratorV1Server).AutoGetOrchestrator(ctx, req.(*Orchestrator))
	}
	return interceptor(ctx, in, info, handler)
}

func _OrchestratorV1_AutoListOrchestrator_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(api.ListWatchOptions)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(OrchestratorV1Server).AutoListOrchestrator(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/orchestration.OrchestratorV1/AutoListOrchestrator",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(OrchestratorV1Server).AutoListOrchestrator(ctx, req.(*api.ListWatchOptions))
	}
	return interceptor(ctx, in, info, handler)
}

func _OrchestratorV1_AutoUpdateOrchestrator_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(Orchestrator)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(OrchestratorV1Server).AutoUpdateOrchestrator(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/orchestration.OrchestratorV1/AutoUpdateOrchestrator",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(OrchestratorV1Server).AutoUpdateOrchestrator(ctx, req.(*Orchestrator))
	}
	return interceptor(ctx, in, info, handler)
}

func _OrchestratorV1_AutoWatchOrchestrator_Handler(srv interface{}, stream grpc.ServerStream) error {
	m := new(api.ListWatchOptions)
	if err := stream.RecvMsg(m); err != nil {
		return err
	}
	return srv.(OrchestratorV1Server).AutoWatchOrchestrator(m, &orchestratorV1AutoWatchOrchestratorServer{stream})
}

type OrchestratorV1_AutoWatchOrchestratorServer interface {
	Send(*AutoMsgOrchestratorWatchHelper) error
	grpc.ServerStream
}

type orchestratorV1AutoWatchOrchestratorServer struct {
	grpc.ServerStream
}

func (x *orchestratorV1AutoWatchOrchestratorServer) Send(m *AutoMsgOrchestratorWatchHelper) error {
	return x.ServerStream.SendMsg(m)
}

func _OrchestratorV1_AutoWatchSvcOrchestratorV1_Handler(srv interface{}, stream grpc.ServerStream) error {
	m := new(api.ListWatchOptions)
	if err := stream.RecvMsg(m); err != nil {
		return err
	}
	return srv.(OrchestratorV1Server).AutoWatchSvcOrchestratorV1(m, &orchestratorV1AutoWatchSvcOrchestratorV1Server{stream})
}

type OrchestratorV1_AutoWatchSvcOrchestratorV1Server interface {
	Send(*api.WatchEventList) error
	grpc.ServerStream
}

type orchestratorV1AutoWatchSvcOrchestratorV1Server struct {
	grpc.ServerStream
}

func (x *orchestratorV1AutoWatchSvcOrchestratorV1Server) Send(m *api.WatchEventList) error {
	return x.ServerStream.SendMsg(m)
}

var _OrchestratorV1_serviceDesc = grpc.ServiceDesc{
	ServiceName: "orchestration.OrchestratorV1",
	HandlerType: (*OrchestratorV1Server)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "AutoAddOrchestrator",
			Handler:    _OrchestratorV1_AutoAddOrchestrator_Handler,
		},
		{
			MethodName: "AutoDeleteOrchestrator",
			Handler:    _OrchestratorV1_AutoDeleteOrchestrator_Handler,
		},
		{
			MethodName: "AutoGetOrchestrator",
			Handler:    _OrchestratorV1_AutoGetOrchestrator_Handler,
		},
		{
			MethodName: "AutoListOrchestrator",
			Handler:    _OrchestratorV1_AutoListOrchestrator_Handler,
		},
		{
			MethodName: "AutoUpdateOrchestrator",
			Handler:    _OrchestratorV1_AutoUpdateOrchestrator_Handler,
		},
	},
	Streams: []grpc.StreamDesc{
		{
			StreamName:    "AutoWatchOrchestrator",
			Handler:       _OrchestratorV1_AutoWatchOrchestrator_Handler,
			ServerStreams: true,
		},
		{
			StreamName:    "AutoWatchSvcOrchestratorV1",
			Handler:       _OrchestratorV1_AutoWatchSvcOrchestratorV1_Handler,
			ServerStreams: true,
		},
	},
	Metadata: "svc_orchestration.proto",
}

func (m *AutoMsgOrchestratorWatchHelper) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *AutoMsgOrchestratorWatchHelper) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Events) > 0 {
		for _, msg := range m.Events {
			dAtA[i] = 0xa
			i++
			i = encodeVarintSvcOrchestration(dAtA, i, uint64(msg.Size()))
			n, err := msg.MarshalTo(dAtA[i:])
			if err != nil {
				return 0, err
			}
			i += n
		}
	}
	return i, nil
}

func (m *AutoMsgOrchestratorWatchHelper_WatchEvent) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *AutoMsgOrchestratorWatchHelper_WatchEvent) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Type) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintSvcOrchestration(dAtA, i, uint64(len(m.Type)))
		i += copy(dAtA[i:], m.Type)
	}
	if m.Object != nil {
		dAtA[i] = 0x12
		i++
		i = encodeVarintSvcOrchestration(dAtA, i, uint64(m.Object.Size()))
		n1, err := m.Object.MarshalTo(dAtA[i:])
		if err != nil {
			return 0, err
		}
		i += n1
	}
	return i, nil
}

func (m *OrchestratorList) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *OrchestratorList) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	dAtA[i] = 0x12
	i++
	i = encodeVarintSvcOrchestration(dAtA, i, uint64(m.TypeMeta.Size()))
	n2, err := m.TypeMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n2
	dAtA[i] = 0x1a
	i++
	i = encodeVarintSvcOrchestration(dAtA, i, uint64(m.ListMeta.Size()))
	n3, err := m.ListMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n3
	if len(m.Items) > 0 {
		for _, msg := range m.Items {
			dAtA[i] = 0x22
			i++
			i = encodeVarintSvcOrchestration(dAtA, i, uint64(msg.Size()))
			n, err := msg.MarshalTo(dAtA[i:])
			if err != nil {
				return 0, err
			}
			i += n
		}
	}
	return i, nil
}

func encodeVarintSvcOrchestration(dAtA []byte, offset int, v uint64) int {
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return offset + 1
}
func (m *AutoMsgOrchestratorWatchHelper) Size() (n int) {
	var l int
	_ = l
	if len(m.Events) > 0 {
		for _, e := range m.Events {
			l = e.Size()
			n += 1 + l + sovSvcOrchestration(uint64(l))
		}
	}
	return n
}

func (m *AutoMsgOrchestratorWatchHelper_WatchEvent) Size() (n int) {
	var l int
	_ = l
	l = len(m.Type)
	if l > 0 {
		n += 1 + l + sovSvcOrchestration(uint64(l))
	}
	if m.Object != nil {
		l = m.Object.Size()
		n += 1 + l + sovSvcOrchestration(uint64(l))
	}
	return n
}

func (m *OrchestratorList) Size() (n int) {
	var l int
	_ = l
	l = m.TypeMeta.Size()
	n += 1 + l + sovSvcOrchestration(uint64(l))
	l = m.ListMeta.Size()
	n += 1 + l + sovSvcOrchestration(uint64(l))
	if len(m.Items) > 0 {
		for _, e := range m.Items {
			l = e.Size()
			n += 1 + l + sovSvcOrchestration(uint64(l))
		}
	}
	return n
}

func sovSvcOrchestration(x uint64) (n int) {
	for {
		n++
		x >>= 7
		if x == 0 {
			break
		}
	}
	return n
}
func sozSvcOrchestration(x uint64) (n int) {
	return sovSvcOrchestration(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *AutoMsgOrchestratorWatchHelper) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowSvcOrchestration
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
			return fmt.Errorf("proto: AutoMsgOrchestratorWatchHelper: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: AutoMsgOrchestratorWatchHelper: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Events", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowSvcOrchestration
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
				return ErrInvalidLengthSvcOrchestration
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Events = append(m.Events, &AutoMsgOrchestratorWatchHelper_WatchEvent{})
			if err := m.Events[len(m.Events)-1].Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipSvcOrchestration(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthSvcOrchestration
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
func (m *AutoMsgOrchestratorWatchHelper_WatchEvent) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowSvcOrchestration
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
			return fmt.Errorf("proto: WatchEvent: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: WatchEvent: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Type", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowSvcOrchestration
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
				return ErrInvalidLengthSvcOrchestration
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Type = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Object", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowSvcOrchestration
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
				return ErrInvalidLengthSvcOrchestration
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if m.Object == nil {
				m.Object = &Orchestrator{}
			}
			if err := m.Object.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipSvcOrchestration(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthSvcOrchestration
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
func (m *OrchestratorList) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowSvcOrchestration
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
			return fmt.Errorf("proto: OrchestratorList: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: OrchestratorList: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field TypeMeta", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowSvcOrchestration
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
				return ErrInvalidLengthSvcOrchestration
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if err := m.TypeMeta.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		case 3:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field ListMeta", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowSvcOrchestration
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
				return ErrInvalidLengthSvcOrchestration
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if err := m.ListMeta.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		case 4:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Items", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowSvcOrchestration
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
				return ErrInvalidLengthSvcOrchestration
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Items = append(m.Items, &Orchestrator{})
			if err := m.Items[len(m.Items)-1].Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipSvcOrchestration(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthSvcOrchestration
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
func skipSvcOrchestration(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowSvcOrchestration
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
					return 0, ErrIntOverflowSvcOrchestration
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
					return 0, ErrIntOverflowSvcOrchestration
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
				return 0, ErrInvalidLengthSvcOrchestration
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowSvcOrchestration
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
				next, err := skipSvcOrchestration(dAtA[start:])
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
	ErrInvalidLengthSvcOrchestration = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowSvcOrchestration   = fmt.Errorf("proto: integer overflow")
)

func init() { proto.RegisterFile("svc_orchestration.proto", fileDescriptorSvcOrchestration) }

var fileDescriptorSvcOrchestration = []byte{
	// 761 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0xac, 0x95, 0xcf, 0x6f, 0xd3, 0x4a,
	0x10, 0xc7, 0xb3, 0x49, 0x9a, 0xbe, 0x6e, 0x5f, 0xfb, 0xf2, 0x36, 0x6d, 0x5f, 0xec, 0x57, 0xe2,
	0x28, 0x12, 0x52, 0xa9, 0x5a, 0xbb, 0x3f, 0x00, 0xa1, 0x88, 0x4b, 0x23, 0x2a, 0x40, 0xa2, 0x04,
	0x95, 0x02, 0x17, 0x7e, 0xc8, 0x71, 0x16, 0xc7, 0xc8, 0xf1, 0x9a, 0x78, 0x9d, 0x2a, 0x42, 0xbd,
	0x34, 0xa9, 0x04, 0x47, 0xe0, 0xc6, 0x91, 0x23, 0xc7, 0x9c, 0x38, 0x72, 0xec, 0xb1, 0x12, 0xb7,
	0x1e, 0x2c, 0x14, 0x71, 0x80, 0xfc, 0x07, 0xdc, 0xd0, 0xae, 0x93, 0xd6, 0x6e, 0x49, 0x04, 0x52,
	0x4f, 0x5e, 0xcf, 0xce, 0xcc, 0xf7, 0xb3, 0x33, 0xe3, 0x35, 0xfc, 0xcf, 0xa9, 0x6b, 0x4f, 0x48,
	0x4d, 0xab, 0x60, 0x87, 0xd6, 0x54, 0x6a, 0x10, 0x4b, 0xb6, 0x6b, 0x84, 0x12, 0x34, 0x11, 0x32,
	0x8a, 0xb3, 0x3a, 0x21, 0xba, 0x89, 0x15, 0xd5, 0x36, 0x14, 0xd5, 0xb2, 0x08, 0xe5, 0x66, 0xc7,
	0x77, 0x16, 0xd7, 0x75, 0x83, 0x56, 0xdc, 0x92, 0xac, 0x91, 0xaa, 0x62, 0x63, 0xcb, 0x51, 0xad,
	0x32, 0x51, 0x9c, 0x6d, 0xa5, 0x8e, 0x2d, 0x43, 0xc3, 0x8a, 0x4b, 0x0d, 0xd3, 0x61, 0xa1, 0x3a,
	0xb6, 0x82, 0xd1, 0x8a, 0x61, 0x69, 0xa6, 0x5b, 0xc6, 0xfd, 0x34, 0x8b, 0x81, 0x34, 0x3a, 0xd1,
	0x89, 0xc2, 0xcd, 0x25, 0xf7, 0x29, 0x7f, 0xe3, 0x2f, 0x7c, 0xd5, 0x73, 0x4f, 0xfd, 0x82, 0x5b,
	0x3c, 0x3f, 0x00, 0x85, 0x81, 0x57, 0x31, 0x55, 0x7d, 0xb7, 0xdc, 0x6e, 0x14, 0x66, 0xd6, 0x5c,
	0x4a, 0x36, 0x1c, 0xbd, 0x78, 0x94, 0x85, 0xd4, 0x1e, 0xa8, 0x54, 0xab, 0xdc, 0xc0, 0xa6, 0x8d,
	0x6b, 0xe8, 0x21, 0x4c, 0xac, 0xd7, 0xb1, 0x45, 0x9d, 0x34, 0xc8, 0xc6, 0xe6, 0xc6, 0x57, 0xae,
	0xc8, 0x61, 0xbd, 0xe1, 0xe1, 0x32, 0x5f, 0xf3, 0x04, 0x05, 0xd8, 0xf5, 0xa4, 0x04, 0xe6, 0xb9,
	0x36, 0x7b, 0x4f, 0xf1, 0x39, 0x84, 0xc7, 0x1e, 0x28, 0x0b, 0xe3, 0x5b, 0x0d, 0x1b, 0xa7, 0x41,
	0x16, 0xcc, 0x8d, 0x15, 0x50, 0xd7, 0x93, 0x26, 0x69, 0xc3, 0xc6, 0x0b, 0xa4, 0x6a, 0x50, 0x5c,
	0xb5, 0x69, 0x03, 0xad, 0xc1, 0x44, 0xb1, 0xf4, 0x0c, 0x6b, 0x34, 0x1d, 0xcd, 0x82, 0xb9, 0xf1,
	0x95, 0xff, 0x4f, 0xd0, 0x04, 0x31, 0x0a, 0x53, 0x5d, 0x4f, 0x4a, 0x12, 0xee, 0x7e, 0x9c, 0x22,
	0xff, 0xcf, 0xe1, 0x9e, 0x30, 0xbe, 0xcd, 0x44, 0x2b, 0x1c, 0x31, 0xf7, 0x1d, 0xc0, 0x64, 0x30,
	0xee, 0x96, 0xe1, 0x50, 0x74, 0x19, 0x82, 0xad, 0x9e, 0xc6, 0x84, 0xac, 0xda, 0x86, 0xcc, 0xc0,
	0x36, 0x30, 0x55, 0x0b, 0xa9, 0x7d, 0x4f, 0x8a, 0x1c, 0x78, 0x12, 0xe8, 0x7a, 0xd2, 0xe8, 0x82,
	0x61, 0x99, 0x86, 0x85, 0x37, 0xfb, 0x0b, 0x54, 0x84, 0x7f, 0xb1, 0x78, 0xe6, 0x99, 0x8e, 0x05,
	0xc2, 0xfb, 0xc6, 0xc2, 0x6c, 0x20, 0x3c, 0x69, 0x1a, 0x0e, 0x5d, 0x64, 0xfd, 0xe8, 0xe7, 0x39,
	0x65, 0x41, 0x57, 0xe1, 0xc8, 0x4d, 0x8a, 0xab, 0x4e, 0x3a, 0xce, 0xcb, 0x3f, 0xf4, 0xc0, 0x63,
	0x5d, 0x4f, 0x1a, 0x61, 0xe7, 0x74, 0x36, 0xfd, 0x47, 0x7e, 0xf2, 0x70, 0x4f, 0x80, 0x2c, 0xa7,
	0x7f, 0xd6, 0x95, 0x1f, 0xa3, 0x70, 0x32, 0x18, 0x72, 0x7f, 0x19, 0xed, 0xc0, 0x14, 0xeb, 0xe1,
	0x5a, 0xb9, 0x1c, 0xdc, 0x40, 0xc3, 0x84, 0xc4, 0x61, 0x9b, 0xb9, 0x85, 0x76, 0x4b, 0x48, 0x68,
	0x35, 0xac, 0x52, 0xfc, 0xb1, 0x25, 0x80, 0x4f, 0x2d, 0x21, 0xb2, 0xfb, 0xf9, 0xeb, 0xdb, 0xe8,
	0x14, 0x8c, 0xe4, 0xc1, 0x7c, 0x6e, 0x42, 0x21, 0x41, 0x9d, 0x97, 0x00, 0xce, 0x30, 0xfd, 0x6b,
	0xd8, 0xc4, 0x14, 0x9f, 0x11, 0xc2, 0x45, 0x86, 0x50, 0xe6, 0x19, 0x43, 0x08, 0xb3, 0x30, 0x92,
	0x8f, 0xcc, 0xcf, 0x84, 0x08, 0x94, 0x17, 0x45, 0xf9, 0xb6, 0x5a, 0xc5, 0x3b, 0x68, 0x17, 0xf8,
	0xa5, 0xb8, 0x8e, 0xe9, 0x19, 0x71, 0x2c, 0xb7, 0x5b, 0x42, 0x4c, 0xc7, 0xf4, 0x34, 0x04, 0x1a,
	0x04, 0xd1, 0x80, 0x53, 0x8c, 0x81, 0xcd, 0x4b, 0x08, 0x62, 0xfa, 0x68, 0x8c, 0xf8, 0x07, 0x53,
	0xb4, 0xf9, 0xdd, 0x21, 0x4a, 0x43, 0xe4, 0x99, 0x73, 0xee, 0x42, 0xbb, 0x25, 0xc4, 0xd9, 0x04,
	0x84, 0x18, 0x52, 0x9c, 0xe1, 0x44, 0x2b, 0x5e, 0xf5, 0x5a, 0x71, 0xcf, 0x2e, 0xab, 0x67, 0xd6,
	0x8a, 0x4b, 0xac, 0x15, 0x2e, 0xcf, 0x18, 0x22, 0x38, 0xc7, 0xa7, 0x41, 0x1c, 0x54, 0x86, 0xd7,
	0x00, 0x4e, 0x33, 0x16, 0xff, 0xb0, 0xbf, 0x51, 0x88, 0xc5, 0x3f, 0xba, 0x97, 0x72, 0x4b, 0xed,
	0x96, 0x30, 0xc2, 0x6f, 0x81, 0x10, 0x95, 0xc0, 0xeb, 0x92, 0x52, 0xf8, 0x4e, 0x08, 0x6d, 0x09,
	0xa0, 0x47, 0x50, 0x3c, 0x42, 0xba, 0x5b, 0xd7, 0x4e, 0x7c, 0x47, 0x03, 0xb8, 0x52, 0xdc, 0x7c,
	0x7c, 0xc9, 0xf1, 0xa6, 0xfc, 0x7b, 0x4a, 0x7d, 0x09, 0x88, 0x8f, 0xdf, 0x34, 0x85, 0x68, 0x7d,
	0xf9, 0x5d, 0x53, 0x08, 0xff, 0x75, 0xde, 0x37, 0x85, 0xbf, 0x83, 0x5a, 0x1f, 0x9a, 0xc2, 0x2a,
	0x0c, 0x59, 0x50, 0xcc, 0x76, 0x29, 0x62, 0x83, 0x86, 0x7a, 0x53, 0x8f, 0xe2, 0x36, 0x71, 0x28,
	0xe2, 0x8d, 0x47, 0xbe, 0x52, 0x21, 0xb9, 0xdf, 0xc9, 0x80, 0x83, 0x4e, 0x06, 0x7c, 0xe9, 0x64,
	0xc0, 0xb7, 0x4e, 0x26, 0x72, 0x07, 0x94, 0x12, 0xfc, 0x3f, 0xb0, 0xfa, 0x33, 0x00, 0x00, 0xff,
	0xff, 0xa5, 0x5d, 0x0e, 0x66, 0x01, 0x07, 0x00, 0x00,
}
