// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: security.proto

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

// security group object
type SecurityGroup struct {
	api.TypeMeta   `protobuf:"bytes,1,opt,name=TypeMeta,embedded=TypeMeta" json:",inline"`
	api.ObjectMeta `protobuf:"bytes,2,opt,name=ObjectMeta,embedded=ObjectMeta" json:"meta,omitempty"`
	Spec           SecurityGroupSpec   `protobuf:"bytes,3,opt,name=Spec" json:"spec,omitempty"`
	Status         SecurityGroupStatus `protobuf:"bytes,4,opt,name=Status" json:"status,omitempty"`
}

func (m *SecurityGroup) Reset()                    { *m = SecurityGroup{} }
func (m *SecurityGroup) String() string            { return proto.CompactTextString(m) }
func (*SecurityGroup) ProtoMessage()               {}
func (*SecurityGroup) Descriptor() ([]byte, []int) { return fileDescriptorSecurity, []int{0} }

func (m *SecurityGroup) GetSpec() SecurityGroupSpec {
	if m != nil {
		return m.Spec
	}
	return SecurityGroupSpec{}
}

func (m *SecurityGroup) GetStatus() SecurityGroupStatus {
	if m != nil {
		return m.Status
	}
	return SecurityGroupStatus{}
}

// security group spec
type SecurityGroupSpec struct {
	SecurityProfile string `protobuf:"bytes,1,opt,name=SecurityProfile,proto3" json:"SecurityProfile,omitempty"`
}

func (m *SecurityGroupSpec) Reset()                    { *m = SecurityGroupSpec{} }
func (m *SecurityGroupSpec) String() string            { return proto.CompactTextString(m) }
func (*SecurityGroupSpec) ProtoMessage()               {}
func (*SecurityGroupSpec) Descriptor() ([]byte, []int) { return fileDescriptorSecurity, []int{1} }

func (m *SecurityGroupSpec) GetSecurityProfile() string {
	if m != nil {
		return m.SecurityProfile
	}
	return ""
}

// security group status
type SecurityGroupStatus struct {
	SecurityGroupID uint64 `protobuf:"varint,1,opt,name=SecurityGroupID,proto3" json:"SecurityGroupID,omitempty"`
}

func (m *SecurityGroupStatus) Reset()                    { *m = SecurityGroupStatus{} }
func (m *SecurityGroupStatus) String() string            { return proto.CompactTextString(m) }
func (*SecurityGroupStatus) ProtoMessage()               {}
func (*SecurityGroupStatus) Descriptor() ([]byte, []int) { return fileDescriptorSecurity, []int{2} }

func (m *SecurityGroupStatus) GetSecurityGroupID() uint64 {
	if m != nil {
		return m.SecurityGroupID
	}
	return 0
}

// list of security groups
type SecurityGroupList struct {
	SecurityGroups []*SecurityGroup `protobuf:"bytes,1,rep,name=SecurityGroups" json:"SecurityGroups,omitempty"`
}

func (m *SecurityGroupList) Reset()                    { *m = SecurityGroupList{} }
func (m *SecurityGroupList) String() string            { return proto.CompactTextString(m) }
func (*SecurityGroupList) ProtoMessage()               {}
func (*SecurityGroupList) Descriptor() ([]byte, []int) { return fileDescriptorSecurity, []int{3} }

func (m *SecurityGroupList) GetSecurityGroups() []*SecurityGroup {
	if m != nil {
		return m.SecurityGroups
	}
	return nil
}

// security group watch event
type SecurityGroupEvent struct {
	EventType     api.EventType `protobuf:"varint,1,opt,name=EventType,proto3,enum=api.EventType" json:"event-type,omitempty"`
	SecurityGroup SecurityGroup `protobuf:"bytes,2,opt,name=SecurityGroup" json:"security-group,omitempty"`
}

func (m *SecurityGroupEvent) Reset()                    { *m = SecurityGroupEvent{} }
func (m *SecurityGroupEvent) String() string            { return proto.CompactTextString(m) }
func (*SecurityGroupEvent) ProtoMessage()               {}
func (*SecurityGroupEvent) Descriptor() ([]byte, []int) { return fileDescriptorSecurity, []int{4} }

func (m *SecurityGroupEvent) GetEventType() api.EventType {
	if m != nil {
		return m.EventType
	}
	return api.EventType_CreateEvent
}

func (m *SecurityGroupEvent) GetSecurityGroup() SecurityGroup {
	if m != nil {
		return m.SecurityGroup
	}
	return SecurityGroup{}
}

// security group watch events batched
type SecurityGroupEventList struct {
	SecurityGroupEvents []*SecurityGroupEvent `protobuf:"bytes,1,rep,name=SecurityGroupEvents" json:"SecurityGroupEvents,omitempty"`
}

func (m *SecurityGroupEventList) Reset()                    { *m = SecurityGroupEventList{} }
func (m *SecurityGroupEventList) String() string            { return proto.CompactTextString(m) }
func (*SecurityGroupEventList) ProtoMessage()               {}
func (*SecurityGroupEventList) Descriptor() ([]byte, []int) { return fileDescriptorSecurity, []int{5} }

func (m *SecurityGroupEventList) GetSecurityGroupEvents() []*SecurityGroupEvent {
	if m != nil {
		return m.SecurityGroupEvents
	}
	return nil
}

func init() {
	proto.RegisterType((*SecurityGroup)(nil), "netproto.SecurityGroup")
	proto.RegisterType((*SecurityGroupSpec)(nil), "netproto.SecurityGroupSpec")
	proto.RegisterType((*SecurityGroupStatus)(nil), "netproto.SecurityGroupStatus")
	proto.RegisterType((*SecurityGroupList)(nil), "netproto.SecurityGroupList")
	proto.RegisterType((*SecurityGroupEvent)(nil), "netproto.SecurityGroupEvent")
	proto.RegisterType((*SecurityGroupEventList)(nil), "netproto.SecurityGroupEventList")
}

// Reference imports to suppress errors if they are not otherwise used.
var _ context.Context
var _ grpc.ClientConn

// This is a compile-time assertion to ensure that this generated file
// is compatible with the grpc package it is being compiled against.
const _ = grpc.SupportPackageIsVersion4

// Client API for SecurityGroupApi service

type SecurityGroupApiClient interface {
	GetSecurityGroup(ctx context.Context, in *api.ObjectMeta, opts ...grpc.CallOption) (*SecurityGroup, error)
	ListSecurityGroups(ctx context.Context, in *api.ListWatchOptions, opts ...grpc.CallOption) (*SecurityGroupList, error)
	WatchSecurityGroups(ctx context.Context, in *api.ListWatchOptions, opts ...grpc.CallOption) (SecurityGroupApi_WatchSecurityGroupsClient, error)
	UpdateSecurityGroup(ctx context.Context, in *SecurityGroup, opts ...grpc.CallOption) (*SecurityGroup, error)
	SecurityGroupOperUpdate(ctx context.Context, opts ...grpc.CallOption) (SecurityGroupApi_SecurityGroupOperUpdateClient, error)
}

type securityGroupApiClient struct {
	cc *grpc.ClientConn
}

func NewSecurityGroupApiClient(cc *grpc.ClientConn) SecurityGroupApiClient {
	return &securityGroupApiClient{cc}
}

func (c *securityGroupApiClient) GetSecurityGroup(ctx context.Context, in *api.ObjectMeta, opts ...grpc.CallOption) (*SecurityGroup, error) {
	out := new(SecurityGroup)
	err := grpc.Invoke(ctx, "/netproto.SecurityGroupApi/GetSecurityGroup", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *securityGroupApiClient) ListSecurityGroups(ctx context.Context, in *api.ListWatchOptions, opts ...grpc.CallOption) (*SecurityGroupList, error) {
	out := new(SecurityGroupList)
	err := grpc.Invoke(ctx, "/netproto.SecurityGroupApi/ListSecurityGroups", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *securityGroupApiClient) WatchSecurityGroups(ctx context.Context, in *api.ListWatchOptions, opts ...grpc.CallOption) (SecurityGroupApi_WatchSecurityGroupsClient, error) {
	stream, err := grpc.NewClientStream(ctx, &_SecurityGroupApi_serviceDesc.Streams[0], c.cc, "/netproto.SecurityGroupApi/WatchSecurityGroups", opts...)
	if err != nil {
		return nil, err
	}
	x := &securityGroupApiWatchSecurityGroupsClient{stream}
	if err := x.ClientStream.SendMsg(in); err != nil {
		return nil, err
	}
	if err := x.ClientStream.CloseSend(); err != nil {
		return nil, err
	}
	return x, nil
}

type SecurityGroupApi_WatchSecurityGroupsClient interface {
	Recv() (*SecurityGroupEventList, error)
	grpc.ClientStream
}

type securityGroupApiWatchSecurityGroupsClient struct {
	grpc.ClientStream
}

func (x *securityGroupApiWatchSecurityGroupsClient) Recv() (*SecurityGroupEventList, error) {
	m := new(SecurityGroupEventList)
	if err := x.ClientStream.RecvMsg(m); err != nil {
		return nil, err
	}
	return m, nil
}

func (c *securityGroupApiClient) UpdateSecurityGroup(ctx context.Context, in *SecurityGroup, opts ...grpc.CallOption) (*SecurityGroup, error) {
	out := new(SecurityGroup)
	err := grpc.Invoke(ctx, "/netproto.SecurityGroupApi/UpdateSecurityGroup", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *securityGroupApiClient) SecurityGroupOperUpdate(ctx context.Context, opts ...grpc.CallOption) (SecurityGroupApi_SecurityGroupOperUpdateClient, error) {
	stream, err := grpc.NewClientStream(ctx, &_SecurityGroupApi_serviceDesc.Streams[1], c.cc, "/netproto.SecurityGroupApi/SecurityGroupOperUpdate", opts...)
	if err != nil {
		return nil, err
	}
	x := &securityGroupApiSecurityGroupOperUpdateClient{stream}
	return x, nil
}

type SecurityGroupApi_SecurityGroupOperUpdateClient interface {
	Send(*SecurityGroupEvent) error
	CloseAndRecv() (*api.TypeMeta, error)
	grpc.ClientStream
}

type securityGroupApiSecurityGroupOperUpdateClient struct {
	grpc.ClientStream
}

func (x *securityGroupApiSecurityGroupOperUpdateClient) Send(m *SecurityGroupEvent) error {
	return x.ClientStream.SendMsg(m)
}

func (x *securityGroupApiSecurityGroupOperUpdateClient) CloseAndRecv() (*api.TypeMeta, error) {
	if err := x.ClientStream.CloseSend(); err != nil {
		return nil, err
	}
	m := new(api.TypeMeta)
	if err := x.ClientStream.RecvMsg(m); err != nil {
		return nil, err
	}
	return m, nil
}

// Server API for SecurityGroupApi service

type SecurityGroupApiServer interface {
	GetSecurityGroup(context.Context, *api.ObjectMeta) (*SecurityGroup, error)
	ListSecurityGroups(context.Context, *api.ListWatchOptions) (*SecurityGroupList, error)
	WatchSecurityGroups(*api.ListWatchOptions, SecurityGroupApi_WatchSecurityGroupsServer) error
	UpdateSecurityGroup(context.Context, *SecurityGroup) (*SecurityGroup, error)
	SecurityGroupOperUpdate(SecurityGroupApi_SecurityGroupOperUpdateServer) error
}

func RegisterSecurityGroupApiServer(s *grpc.Server, srv SecurityGroupApiServer) {
	s.RegisterService(&_SecurityGroupApi_serviceDesc, srv)
}

func _SecurityGroupApi_GetSecurityGroup_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(api.ObjectMeta)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(SecurityGroupApiServer).GetSecurityGroup(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/netproto.SecurityGroupApi/GetSecurityGroup",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(SecurityGroupApiServer).GetSecurityGroup(ctx, req.(*api.ObjectMeta))
	}
	return interceptor(ctx, in, info, handler)
}

func _SecurityGroupApi_ListSecurityGroups_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(api.ListWatchOptions)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(SecurityGroupApiServer).ListSecurityGroups(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/netproto.SecurityGroupApi/ListSecurityGroups",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(SecurityGroupApiServer).ListSecurityGroups(ctx, req.(*api.ListWatchOptions))
	}
	return interceptor(ctx, in, info, handler)
}

func _SecurityGroupApi_WatchSecurityGroups_Handler(srv interface{}, stream grpc.ServerStream) error {
	m := new(api.ListWatchOptions)
	if err := stream.RecvMsg(m); err != nil {
		return err
	}
	return srv.(SecurityGroupApiServer).WatchSecurityGroups(m, &securityGroupApiWatchSecurityGroupsServer{stream})
}

type SecurityGroupApi_WatchSecurityGroupsServer interface {
	Send(*SecurityGroupEventList) error
	grpc.ServerStream
}

type securityGroupApiWatchSecurityGroupsServer struct {
	grpc.ServerStream
}

func (x *securityGroupApiWatchSecurityGroupsServer) Send(m *SecurityGroupEventList) error {
	return x.ServerStream.SendMsg(m)
}

func _SecurityGroupApi_UpdateSecurityGroup_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(SecurityGroup)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(SecurityGroupApiServer).UpdateSecurityGroup(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/netproto.SecurityGroupApi/UpdateSecurityGroup",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(SecurityGroupApiServer).UpdateSecurityGroup(ctx, req.(*SecurityGroup))
	}
	return interceptor(ctx, in, info, handler)
}

func _SecurityGroupApi_SecurityGroupOperUpdate_Handler(srv interface{}, stream grpc.ServerStream) error {
	return srv.(SecurityGroupApiServer).SecurityGroupOperUpdate(&securityGroupApiSecurityGroupOperUpdateServer{stream})
}

type SecurityGroupApi_SecurityGroupOperUpdateServer interface {
	SendAndClose(*api.TypeMeta) error
	Recv() (*SecurityGroupEvent, error)
	grpc.ServerStream
}

type securityGroupApiSecurityGroupOperUpdateServer struct {
	grpc.ServerStream
}

func (x *securityGroupApiSecurityGroupOperUpdateServer) SendAndClose(m *api.TypeMeta) error {
	return x.ServerStream.SendMsg(m)
}

func (x *securityGroupApiSecurityGroupOperUpdateServer) Recv() (*SecurityGroupEvent, error) {
	m := new(SecurityGroupEvent)
	if err := x.ServerStream.RecvMsg(m); err != nil {
		return nil, err
	}
	return m, nil
}

var _SecurityGroupApi_serviceDesc = grpc.ServiceDesc{
	ServiceName: "netproto.SecurityGroupApi",
	HandlerType: (*SecurityGroupApiServer)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "GetSecurityGroup",
			Handler:    _SecurityGroupApi_GetSecurityGroup_Handler,
		},
		{
			MethodName: "ListSecurityGroups",
			Handler:    _SecurityGroupApi_ListSecurityGroups_Handler,
		},
		{
			MethodName: "UpdateSecurityGroup",
			Handler:    _SecurityGroupApi_UpdateSecurityGroup_Handler,
		},
	},
	Streams: []grpc.StreamDesc{
		{
			StreamName:    "WatchSecurityGroups",
			Handler:       _SecurityGroupApi_WatchSecurityGroups_Handler,
			ServerStreams: true,
		},
		{
			StreamName:    "SecurityGroupOperUpdate",
			Handler:       _SecurityGroupApi_SecurityGroupOperUpdate_Handler,
			ClientStreams: true,
		},
	},
	Metadata: "security.proto",
}

func (m *SecurityGroup) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *SecurityGroup) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	dAtA[i] = 0xa
	i++
	i = encodeVarintSecurity(dAtA, i, uint64(m.TypeMeta.Size()))
	n1, err := m.TypeMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n1
	dAtA[i] = 0x12
	i++
	i = encodeVarintSecurity(dAtA, i, uint64(m.ObjectMeta.Size()))
	n2, err := m.ObjectMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n2
	dAtA[i] = 0x1a
	i++
	i = encodeVarintSecurity(dAtA, i, uint64(m.Spec.Size()))
	n3, err := m.Spec.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n3
	dAtA[i] = 0x22
	i++
	i = encodeVarintSecurity(dAtA, i, uint64(m.Status.Size()))
	n4, err := m.Status.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n4
	return i, nil
}

func (m *SecurityGroupSpec) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *SecurityGroupSpec) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.SecurityProfile) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintSecurity(dAtA, i, uint64(len(m.SecurityProfile)))
		i += copy(dAtA[i:], m.SecurityProfile)
	}
	return i, nil
}

func (m *SecurityGroupStatus) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *SecurityGroupStatus) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if m.SecurityGroupID != 0 {
		dAtA[i] = 0x8
		i++
		i = encodeVarintSecurity(dAtA, i, uint64(m.SecurityGroupID))
	}
	return i, nil
}

func (m *SecurityGroupList) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *SecurityGroupList) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.SecurityGroups) > 0 {
		for _, msg := range m.SecurityGroups {
			dAtA[i] = 0xa
			i++
			i = encodeVarintSecurity(dAtA, i, uint64(msg.Size()))
			n, err := msg.MarshalTo(dAtA[i:])
			if err != nil {
				return 0, err
			}
			i += n
		}
	}
	return i, nil
}

func (m *SecurityGroupEvent) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *SecurityGroupEvent) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if m.EventType != 0 {
		dAtA[i] = 0x8
		i++
		i = encodeVarintSecurity(dAtA, i, uint64(m.EventType))
	}
	dAtA[i] = 0x12
	i++
	i = encodeVarintSecurity(dAtA, i, uint64(m.SecurityGroup.Size()))
	n5, err := m.SecurityGroup.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n5
	return i, nil
}

func (m *SecurityGroupEventList) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *SecurityGroupEventList) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.SecurityGroupEvents) > 0 {
		for _, msg := range m.SecurityGroupEvents {
			dAtA[i] = 0xa
			i++
			i = encodeVarintSecurity(dAtA, i, uint64(msg.Size()))
			n, err := msg.MarshalTo(dAtA[i:])
			if err != nil {
				return 0, err
			}
			i += n
		}
	}
	return i, nil
}

func encodeVarintSecurity(dAtA []byte, offset int, v uint64) int {
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return offset + 1
}
func (m *SecurityGroup) Size() (n int) {
	var l int
	_ = l
	l = m.TypeMeta.Size()
	n += 1 + l + sovSecurity(uint64(l))
	l = m.ObjectMeta.Size()
	n += 1 + l + sovSecurity(uint64(l))
	l = m.Spec.Size()
	n += 1 + l + sovSecurity(uint64(l))
	l = m.Status.Size()
	n += 1 + l + sovSecurity(uint64(l))
	return n
}

func (m *SecurityGroupSpec) Size() (n int) {
	var l int
	_ = l
	l = len(m.SecurityProfile)
	if l > 0 {
		n += 1 + l + sovSecurity(uint64(l))
	}
	return n
}

func (m *SecurityGroupStatus) Size() (n int) {
	var l int
	_ = l
	if m.SecurityGroupID != 0 {
		n += 1 + sovSecurity(uint64(m.SecurityGroupID))
	}
	return n
}

func (m *SecurityGroupList) Size() (n int) {
	var l int
	_ = l
	if len(m.SecurityGroups) > 0 {
		for _, e := range m.SecurityGroups {
			l = e.Size()
			n += 1 + l + sovSecurity(uint64(l))
		}
	}
	return n
}

func (m *SecurityGroupEvent) Size() (n int) {
	var l int
	_ = l
	if m.EventType != 0 {
		n += 1 + sovSecurity(uint64(m.EventType))
	}
	l = m.SecurityGroup.Size()
	n += 1 + l + sovSecurity(uint64(l))
	return n
}

func (m *SecurityGroupEventList) Size() (n int) {
	var l int
	_ = l
	if len(m.SecurityGroupEvents) > 0 {
		for _, e := range m.SecurityGroupEvents {
			l = e.Size()
			n += 1 + l + sovSecurity(uint64(l))
		}
	}
	return n
}

func sovSecurity(x uint64) (n int) {
	for {
		n++
		x >>= 7
		if x == 0 {
			break
		}
	}
	return n
}
func sozSecurity(x uint64) (n int) {
	return sovSecurity(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *SecurityGroup) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowSecurity
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
			return fmt.Errorf("proto: SecurityGroup: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: SecurityGroup: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field TypeMeta", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowSecurity
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
				return ErrInvalidLengthSecurity
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
					return ErrIntOverflowSecurity
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
				return ErrInvalidLengthSecurity
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
					return ErrIntOverflowSecurity
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
				return ErrInvalidLengthSecurity
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
					return ErrIntOverflowSecurity
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
				return ErrInvalidLengthSecurity
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
			skippy, err := skipSecurity(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthSecurity
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
func (m *SecurityGroupSpec) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowSecurity
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
			return fmt.Errorf("proto: SecurityGroupSpec: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: SecurityGroupSpec: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field SecurityProfile", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowSecurity
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
				return ErrInvalidLengthSecurity
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.SecurityProfile = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipSecurity(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthSecurity
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
func (m *SecurityGroupStatus) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowSecurity
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
			return fmt.Errorf("proto: SecurityGroupStatus: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: SecurityGroupStatus: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field SecurityGroupID", wireType)
			}
			m.SecurityGroupID = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowSecurity
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.SecurityGroupID |= (uint64(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		default:
			iNdEx = preIndex
			skippy, err := skipSecurity(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthSecurity
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
func (m *SecurityGroupList) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowSecurity
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
			return fmt.Errorf("proto: SecurityGroupList: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: SecurityGroupList: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field SecurityGroups", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowSecurity
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
				return ErrInvalidLengthSecurity
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.SecurityGroups = append(m.SecurityGroups, &SecurityGroup{})
			if err := m.SecurityGroups[len(m.SecurityGroups)-1].Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipSecurity(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthSecurity
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
func (m *SecurityGroupEvent) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowSecurity
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
			return fmt.Errorf("proto: SecurityGroupEvent: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: SecurityGroupEvent: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field EventType", wireType)
			}
			m.EventType = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowSecurity
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
				return fmt.Errorf("proto: wrong wireType = %d for field SecurityGroup", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowSecurity
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
				return ErrInvalidLengthSecurity
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if err := m.SecurityGroup.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipSecurity(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthSecurity
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
func (m *SecurityGroupEventList) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowSecurity
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
			return fmt.Errorf("proto: SecurityGroupEventList: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: SecurityGroupEventList: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field SecurityGroupEvents", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowSecurity
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
				return ErrInvalidLengthSecurity
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.SecurityGroupEvents = append(m.SecurityGroupEvents, &SecurityGroupEvent{})
			if err := m.SecurityGroupEvents[len(m.SecurityGroupEvents)-1].Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipSecurity(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthSecurity
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
func skipSecurity(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowSecurity
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
					return 0, ErrIntOverflowSecurity
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
					return 0, ErrIntOverflowSecurity
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
				return 0, ErrInvalidLengthSecurity
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowSecurity
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
				next, err := skipSecurity(dAtA[start:])
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
	ErrInvalidLengthSecurity = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowSecurity   = fmt.Errorf("proto: integer overflow")
)

func init() { proto.RegisterFile("security.proto", fileDescriptorSecurity) }

var fileDescriptorSecurity = []byte{
	// 656 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x8c, 0x94, 0x4d, 0x4f, 0xdb, 0x4c,
	0x10, 0xc7, 0x63, 0x12, 0xf1, 0xb2, 0x3c, 0x04, 0x9e, 0x4d, 0x0b, 0xae, 0x4b, 0x49, 0x64, 0xa9,
	0x52, 0x0e, 0x60, 0x57, 0xe9, 0x19, 0x50, 0xad, 0x52, 0x44, 0x5f, 0x08, 0x32, 0x54, 0x3d, 0x56,
	0x8e, 0x33, 0x98, 0xad, 0x9c, 0xdd, 0x55, 0x76, 0x4d, 0x15, 0x55, 0x9c, 0xfa, 0x8d, 0x7a, 0xe8,
	0xad, 0x77, 0x8e, 0x7c, 0x82, 0xa8, 0xe2, 0xc8, 0x97, 0x68, 0xb5, 0x9b, 0x04, 0x6c, 0x83, 0xa3,
	0xde, 0xbc, 0x7f, 0xcf, 0xfc, 0xf6, 0x3f, 0x33, 0xbb, 0x8b, 0xaa, 0x02, 0xc2, 0xa4, 0x4f, 0xe4,
	0xc0, 0xe1, 0x7d, 0x26, 0x19, 0x9e, 0xa7, 0x20, 0xf5, 0x97, 0xb5, 0x1e, 0x31, 0x16, 0xc5, 0xe0,
	0x06, 0x9c, 0xb8, 0x01, 0xa5, 0x4c, 0x06, 0x92, 0x30, 0x2a, 0x46, 0x71, 0xd6, 0x5e, 0x44, 0xe4,
	0x59, 0xd2, 0x71, 0x42, 0xd6, 0x73, 0x39, 0x50, 0x11, 0xd0, 0x2e, 0x73, 0xc5, 0x57, 0xf7, 0x1c,
	0x28, 0x09, 0xc1, 0x4d, 0x24, 0x89, 0x85, 0x4a, 0x8d, 0x80, 0xa6, 0xb3, 0x5d, 0x42, 0xc3, 0x38,
	0xe9, 0xc2, 0x04, 0xb3, 0x95, 0xc2, 0x44, 0x2c, 0x62, 0xae, 0x96, 0x3b, 0xc9, 0xa9, 0x5e, 0xe9,
	0x85, 0xfe, 0x1a, 0x87, 0x3f, 0x2f, 0xd8, 0x55, 0x79, 0xec, 0x81, 0x0c, 0x46, 0x61, 0xf6, 0xcf,
	0x19, 0xb4, 0x74, 0x3c, 0xae, 0x6b, 0xbf, 0xcf, 0x12, 0x8e, 0x77, 0xd0, 0xfc, 0xc9, 0x80, 0xc3,
	0x07, 0x90, 0x81, 0x69, 0x34, 0x8c, 0xe6, 0x62, 0x6b, 0xc9, 0x09, 0x38, 0x71, 0x26, 0xa2, 0x57,
	0xbb, 0x1c, 0xd6, 0x4b, 0x57, 0xc3, 0xba, 0x71, 0x33, 0xac, 0xcf, 0x6d, 0x12, 0x1a, 0x13, 0x0a,
	0xfe, 0x6d, 0x0e, 0x7e, 0x87, 0x50, 0xbb, 0xf3, 0x05, 0x42, 0xa9, 0x09, 0x33, 0x9a, 0xb0, 0xac,
	0x09, 0x77, 0xb2, 0x67, 0xa5, 0x18, 0x55, 0x65, 0x67, 0x93, 0xf5, 0x88, 0x84, 0x1e, 0x97, 0x03,
	0x3f, 0x95, 0x8e, 0xf7, 0x51, 0xe5, 0x98, 0x43, 0x68, 0x96, 0x35, 0xe6, 0xa9, 0x33, 0x69, 0xb9,
	0x93, 0xf1, 0xac, 0x42, 0xbc, 0x55, 0x85, 0x54, 0x38, 0xc1, 0x21, 0x4c, 0xe1, 0x34, 0x00, 0xb7,
	0xd1, 0xec, 0xb1, 0x0c, 0x64, 0x22, 0xcc, 0x8a, 0x46, 0x3d, 0x2b, 0x42, 0xe9, 0x20, 0xcf, 0x1c,
	0xc3, 0x56, 0x84, 0x5e, 0xa7, 0x70, 0x63, 0x8c, 0xbd, 0x8d, 0xfe, 0xbf, 0xe7, 0x01, 0x37, 0xd1,
	0xf2, 0x44, 0x3c, 0xea, 0xb3, 0x53, 0x12, 0x83, 0x6e, 0xe1, 0x82, 0x9f, 0x97, 0xed, 0x5d, 0x54,
	0x7b, 0x60, 0xdf, 0x34, 0x40, 0xcb, 0x07, 0xaf, 0x35, 0xa0, 0xe2, 0xe7, 0x65, 0xfb, 0x24, 0xb7,
	0xff, 0x7b, 0x22, 0x24, 0xde, 0x45, 0xd5, 0x8c, 0x28, 0x4c, 0xa3, 0x51, 0x6e, 0x2e, 0xb6, 0xd6,
	0x0a, 0xaa, 0xf5, 0x73, 0xe1, 0xf6, 0x2f, 0x03, 0xe1, 0x8c, 0xb4, 0x77, 0x0e, 0x54, 0xe2, 0x37,
	0x68, 0x41, 0x7f, 0xa8, 0x21, 0x6b, 0x43, 0xd5, 0x56, 0x55, 0x8f, 0xf4, 0x56, 0xf5, 0xcc, 0x9b,
	0x61, 0xfd, 0x11, 0xa8, 0xe5, 0x96, 0x1c, 0x70, 0x48, 0x75, 0xec, 0x2e, 0x15, 0x7f, 0xce, 0x1d,
	0xb6, 0xf1, 0xf1, 0x28, 0xb2, 0xe7, 0x35, 0xc6, 0x63, 0x30, 0x27, 0x57, 0x6f, 0x2b, 0x52, 0x7a,
	0x0a, 0x9e, 0xe5, 0xd9, 0x67, 0x68, 0xf5, 0xbe, 0x7d, 0xdd, 0x9a, 0xc3, 0x5c, 0xc3, 0xf5, 0x9f,
	0x49, 0x7f, 0xd6, 0x0b, 0x0c, 0xe8, 0x20, 0xff, 0xa1, 0xc4, 0xd6, 0x9f, 0x32, 0x5a, 0xc9, 0xe8,
	0xaf, 0x38, 0xc1, 0x3b, 0x68, 0x65, 0x1f, 0x64, 0xf6, 0x3e, 0xe5, 0xcf, 0xbe, 0x55, 0x54, 0xad,
	0x5d, 0xc2, 0x6f, 0x11, 0x56, 0x66, 0xb3, 0x43, 0xc1, 0x8f, 0x35, 0x41, 0xfd, 0xf8, 0x14, 0xc8,
	0xf0, 0xac, 0xcd, 0xf5, 0xfb, 0x60, 0x15, 0xdd, 0x06, 0x15, 0x68, 0x97, 0xb0, 0x8f, 0x6a, 0x3a,
	0xfc, 0xdf, 0x60, 0x8d, 0x69, 0x1d, 0x18, 0x11, 0x5f, 0x18, 0xf8, 0x00, 0xd5, 0x3e, 0xf2, 0x6e,
	0x20, 0x21, 0x5b, 0x62, 0x51, 0x45, 0xd3, 0x4b, 0x5d, 0xcb, 0x48, 0x6d, 0x0e, 0xfd, 0x11, 0x1b,
	0x4f, 0x9d, 0x86, 0x95, 0x7d, 0x8d, 0xec, 0x52, 0xd3, 0xb0, 0xe4, 0x8f, 0xef, 0x4f, 0x78, 0xfe,
	0x1d, 0xab, 0xc4, 0x6a, 0xec, 0x15, 0xce, 0x84, 0xc4, 0x65, 0x9e, 0x48, 0x3c, 0xdb, 0x85, 0x18,
	0x24, 0x58, 0xdb, 0xee, 0xb7, 0xbb, 0x69, 0x38, 0x27, 0x40, 0x03, 0x2a, 0x2f, 0x32, 0xda, 0x61,
	0xd0, 0x03, 0xc1, 0x83, 0x10, 0xee, 0xcb, 0x17, 0xf6, 0x9c, 0x7a, 0x41, 0x45, 0x24, 0xbc, 0xff,
	0x2e, 0xaf, 0x37, 0x8c, 0xab, 0xeb, 0x0d, 0xe3, 0xf7, 0xf5, 0x86, 0x71, 0x64, 0x74, 0x66, 0xb5,
	0xe5, 0x97, 0x7f, 0x03, 0x00, 0x00, 0xff, 0xff, 0xc2, 0x2b, 0x11, 0x7c, 0x28, 0x06, 0x00, 0x00,
}
