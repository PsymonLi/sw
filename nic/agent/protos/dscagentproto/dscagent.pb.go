// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: dscagent.proto

/*
	Package dscagentproto is a generated protocol buffer package.

	It is generated from these files:
		dscagent.proto

	It has these top-level messages:
		DSCAgentStatus
*/
package dscagentproto

import proto "github.com/gogo/protobuf/proto"
import fmt "fmt"
import math "math"
import _ "google.golang.org/genproto/googleapis/api/annotations"
import api "github.com/pensando/sw/api"
import _ "github.com/pensando/sw/venice/utils/apigen/annotations"
import _ "github.com/gogo/protobuf/gogoproto"
import cluster2 "github.com/pensando/sw/api/generated/cluster"

import (
	context "golang.org/x/net/context"
	grpc "google.golang.org/grpc"
)

import io "io"

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

// This is a compile-time assertion to ensure that this generated file
// is compatible with the proto package it is being compiled against.
// A compilation error at this line likely means your copy of the
// proto package needs to be updated.
const _ = proto.GoGoProtoPackageIsVersion2 // please upgrade the proto package

type DSCAgentStatus struct {
	// control plane status
	ControlPlaneStatus  *cluster2.DSCControlPlaneStatus `protobuf:"bytes,1,opt,name=ControlPlaneStatus" json:"control-plane-status,omitempty"`
	IsConnectedToVenice bool                            `protobuf:"varint,2,opt,name=IsConnectedToVenice,proto3" json:"is-connected-to-venice,omitempty"`
	UnhealthyServices   []string                        `protobuf:"bytes,3,rep,name=UnhealthyServices" json:"unhealthy-services,omitempty"`
}

func (m *DSCAgentStatus) Reset()                    { *m = DSCAgentStatus{} }
func (m *DSCAgentStatus) String() string            { return proto.CompactTextString(m) }
func (*DSCAgentStatus) ProtoMessage()               {}
func (*DSCAgentStatus) Descriptor() ([]byte, []int) { return fileDescriptorDscagent, []int{0} }

func (m *DSCAgentStatus) GetControlPlaneStatus() *cluster2.DSCControlPlaneStatus {
	if m != nil {
		return m.ControlPlaneStatus
	}
	return nil
}

func (m *DSCAgentStatus) GetIsConnectedToVenice() bool {
	if m != nil {
		return m.IsConnectedToVenice
	}
	return false
}

func (m *DSCAgentStatus) GetUnhealthyServices() []string {
	if m != nil {
		return m.UnhealthyServices
	}
	return nil
}

func init() {
	proto.RegisterType((*DSCAgentStatus)(nil), "dscagentproto.DSCAgentStatus")
}

// Reference imports to suppress errors if they are not otherwise used.
var _ context.Context
var _ grpc.ClientConn

// This is a compile-time assertion to ensure that this generated file
// is compatible with the grpc package it is being compiled against.
const _ = grpc.SupportPackageIsVersion4

// Client API for DSCAgentAPI service

type DSCAgentAPIClient interface {
	// Gets the status of the agent
	GetAgentStatus(ctx context.Context, in *api.Empty, opts ...grpc.CallOption) (*DSCAgentStatus, error)
}

type dSCAgentAPIClient struct {
	cc *grpc.ClientConn
}

func NewDSCAgentAPIClient(cc *grpc.ClientConn) DSCAgentAPIClient {
	return &dSCAgentAPIClient{cc}
}

func (c *dSCAgentAPIClient) GetAgentStatus(ctx context.Context, in *api.Empty, opts ...grpc.CallOption) (*DSCAgentStatus, error) {
	out := new(DSCAgentStatus)
	err := grpc.Invoke(ctx, "/dscagentproto.DSCAgentAPI/GetAgentStatus", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

// Server API for DSCAgentAPI service

type DSCAgentAPIServer interface {
	// Gets the status of the agent
	GetAgentStatus(context.Context, *api.Empty) (*DSCAgentStatus, error)
}

func RegisterDSCAgentAPIServer(s *grpc.Server, srv DSCAgentAPIServer) {
	s.RegisterService(&_DSCAgentAPI_serviceDesc, srv)
}

func _DSCAgentAPI_GetAgentStatus_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(api.Empty)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(DSCAgentAPIServer).GetAgentStatus(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/dscagentproto.DSCAgentAPI/GetAgentStatus",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(DSCAgentAPIServer).GetAgentStatus(ctx, req.(*api.Empty))
	}
	return interceptor(ctx, in, info, handler)
}

var _DSCAgentAPI_serviceDesc = grpc.ServiceDesc{
	ServiceName: "dscagentproto.DSCAgentAPI",
	HandlerType: (*DSCAgentAPIServer)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "GetAgentStatus",
			Handler:    _DSCAgentAPI_GetAgentStatus_Handler,
		},
	},
	Streams:  []grpc.StreamDesc{},
	Metadata: "dscagent.proto",
}

func (m *DSCAgentStatus) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *DSCAgentStatus) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if m.ControlPlaneStatus != nil {
		dAtA[i] = 0xa
		i++
		i = encodeVarintDscagent(dAtA, i, uint64(m.ControlPlaneStatus.Size()))
		n1, err := m.ControlPlaneStatus.MarshalTo(dAtA[i:])
		if err != nil {
			return 0, err
		}
		i += n1
	}
	if m.IsConnectedToVenice {
		dAtA[i] = 0x10
		i++
		if m.IsConnectedToVenice {
			dAtA[i] = 1
		} else {
			dAtA[i] = 0
		}
		i++
	}
	if len(m.UnhealthyServices) > 0 {
		for _, s := range m.UnhealthyServices {
			dAtA[i] = 0x1a
			i++
			l = len(s)
			for l >= 1<<7 {
				dAtA[i] = uint8(uint64(l)&0x7f | 0x80)
				l >>= 7
				i++
			}
			dAtA[i] = uint8(l)
			i++
			i += copy(dAtA[i:], s)
		}
	}
	return i, nil
}

func encodeVarintDscagent(dAtA []byte, offset int, v uint64) int {
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return offset + 1
}
func (m *DSCAgentStatus) Size() (n int) {
	var l int
	_ = l
	if m.ControlPlaneStatus != nil {
		l = m.ControlPlaneStatus.Size()
		n += 1 + l + sovDscagent(uint64(l))
	}
	if m.IsConnectedToVenice {
		n += 2
	}
	if len(m.UnhealthyServices) > 0 {
		for _, s := range m.UnhealthyServices {
			l = len(s)
			n += 1 + l + sovDscagent(uint64(l))
		}
	}
	return n
}

func sovDscagent(x uint64) (n int) {
	for {
		n++
		x >>= 7
		if x == 0 {
			break
		}
	}
	return n
}
func sozDscagent(x uint64) (n int) {
	return sovDscagent(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *DSCAgentStatus) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowDscagent
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
			return fmt.Errorf("proto: DSCAgentStatus: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: DSCAgentStatus: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field ControlPlaneStatus", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowDscagent
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
				return ErrInvalidLengthDscagent
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if m.ControlPlaneStatus == nil {
				m.ControlPlaneStatus = &cluster2.DSCControlPlaneStatus{}
			}
			if err := m.ControlPlaneStatus.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		case 2:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field IsConnectedToVenice", wireType)
			}
			var v int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowDscagent
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				v |= (int(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			m.IsConnectedToVenice = bool(v != 0)
		case 3:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field UnhealthyServices", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowDscagent
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
				return ErrInvalidLengthDscagent
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.UnhealthyServices = append(m.UnhealthyServices, string(dAtA[iNdEx:postIndex]))
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipDscagent(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthDscagent
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
func skipDscagent(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowDscagent
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
					return 0, ErrIntOverflowDscagent
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
					return 0, ErrIntOverflowDscagent
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
				return 0, ErrInvalidLengthDscagent
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowDscagent
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
				next, err := skipDscagent(dAtA[start:])
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
	ErrInvalidLengthDscagent = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowDscagent   = fmt.Errorf("proto: integer overflow")
)

func init() { proto.RegisterFile("dscagent.proto", fileDescriptorDscagent) }

var fileDescriptorDscagent = []byte{
	// 393 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x7c, 0x92, 0xd1, 0x8a, 0xd3, 0x40,
	0x14, 0x86, 0x4d, 0x17, 0x44, 0x67, 0xb5, 0x60, 0xbc, 0x29, 0x65, 0x4d, 0x43, 0x11, 0xec, 0x85,
	0x99, 0x91, 0xf5, 0xda, 0x8b, 0x6d, 0x77, 0x91, 0x05, 0x91, 0x62, 0x75, 0xef, 0xa7, 0x93, 0x63,
	0x3a, 0x90, 0x9c, 0x13, 0x32, 0x27, 0x2b, 0xfb, 0x4e, 0x3e, 0x88, 0x97, 0x3e, 0x41, 0x91, 0x5e,
	0xf6, 0x29, 0x24, 0x93, 0x06, 0xb2, 0x6c, 0xf5, 0x2e, 0xe7, 0xfc, 0xdf, 0xff, 0x9f, 0xf0, 0x33,
	0x62, 0x98, 0x3a, 0xa3, 0x33, 0x40, 0x96, 0x65, 0x45, 0x4c, 0xe1, 0xf3, 0x6e, 0xf6, 0xe3, 0xf8,
	0x2c, 0x23, 0xca, 0x72, 0x50, 0xba, 0xb4, 0x4a, 0x23, 0x12, 0x6b, 0xb6, 0x84, 0xae, 0x85, 0xc7,
	0x6f, 0x32, 0xcb, 0x9b, 0x7a, 0x2d, 0x0d, 0x15, 0xaa, 0x04, 0x74, 0x1a, 0x53, 0x52, 0xee, 0x87,
	0xa7, 0x0d, 0x15, 0x05, 0xe1, 0x01, 0xbc, 0xfa, 0x07, 0x78, 0x0b, 0x68, 0x0d, 0xa8, 0x9a, 0x6d,
	0xee, 0x1a, 0x57, 0x06, 0xd8, 0x3f, 0xa3, 0x2c, 0x9a, 0xbc, 0x4e, 0xa1, 0xbb, 0x97, 0xf4, 0x62,
	0x32, 0xca, 0x48, 0xf9, 0xf5, 0xba, 0xfe, 0xee, 0x27, 0x3f, 0xf8, 0xaf, 0x03, 0xfe, 0xee, 0x3f,
	0xbf, 0xe7, 0x09, 0xa7, 0x5c, 0xa1, 0x2b, 0x46, 0x6b, 0x5a, 0xc7, 0xf4, 0xe7, 0x40, 0x0c, 0x2f,
	0x57, 0x8b, 0x8b, 0xa6, 0x80, 0x15, 0x6b, 0xae, 0x5d, 0x58, 0x89, 0x70, 0x41, 0xc8, 0x15, 0xe5,
	0xcb, 0x5c, 0x23, 0xb4, 0xdb, 0x51, 0x10, 0x07, 0xb3, 0xd3, 0xf3, 0x48, 0x9a, 0xbc, 0x76, 0x0c,
	0x95, 0xbc, 0x5c, 0x2d, 0x1e, 0x52, 0xf3, 0xe9, 0x7e, 0x3b, 0x89, 0x4c, 0xbb, 0x4f, 0xca, 0x46,
	0x48, 0x9c, 0x57, 0xde, 0x52, 0x61, 0x19, 0x8a, 0x92, 0xef, 0xbe, 0x1c, 0x49, 0x0f, 0x6f, 0xc4,
	0xcb, 0x6b, 0xb7, 0x20, 0x44, 0x30, 0x0c, 0xe9, 0x57, 0xba, 0xf1, 0x35, 0x8d, 0x06, 0x71, 0x30,
	0x7b, 0x32, 0x7f, 0xbd, 0xdf, 0x4e, 0x62, 0xeb, 0x12, 0xd3, 0xe9, 0x09, 0x53, 0xd2, 0x16, 0xd9,
	0x8b, 0x3d, 0x16, 0x10, 0x7e, 0x16, 0x2f, 0xbe, 0xe1, 0x06, 0x74, 0xce, 0x9b, 0xbb, 0x15, 0x54,
	0xb7, 0xd6, 0x80, 0x1b, 0x9d, 0xc4, 0x27, 0xb3, 0xa7, 0xf3, 0x78, 0xbf, 0x9d, 0x9c, 0xd5, 0x9d,
	0x98, 0xb8, 0x83, 0xda, 0x4b, 0x7c, 0x68, 0x3d, 0xff, 0x24, 0x4e, 0xbb, 0xb6, 0x2e, 0x96, 0xd7,
	0xe1, 0x07, 0x31, 0xfc, 0x08, 0xdc, 0x2f, 0x4f, 0x48, 0x5d, 0x5a, 0x79, 0xd5, 0xf8, 0xc7, 0xaf,
	0xe4, 0xbd, 0xa7, 0x25, 0xef, 0xf7, 0x3c, 0x7d, 0x34, 0x7f, 0xf6, 0x6b, 0x17, 0x05, 0xbf, 0x77,
	0x51, 0xf0, 0x67, 0x17, 0x05, 0xcb, 0xc1, 0xfa, 0xb1, 0xe7, 0xde, 0xff, 0x0d, 0x00, 0x00, 0xff,
	0xff, 0x22, 0x22, 0xb3, 0x63, 0xa3, 0x02, 0x00, 0x00,
}
