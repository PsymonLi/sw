// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: routing.proto

/*
	Package routing is a generated protocol buffer package.

	It is generated from these files:
		routing.proto
		svc_routing.proto

	It has these top-level messages:
		Neighbor
		NeighborStatus
		AutoMsgNeighborWatchHelper
		NeighborList
*/
package routing

import proto "github.com/gogo/protobuf/proto"
import fmt "fmt"
import math "math"
import _ "github.com/pensando/sw/venice/utils/apigen/annotations"
import api "github.com/pensando/sw/api"
import network "github.com/pensando/sw/api/generated/network"

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

//
type NeighborStatus_State int32

const (
	//
	NeighborStatus_Idle NeighborStatus_State = 0
	//
	NeighborStatus_Connect NeighborStatus_State = 1
	//
	NeighborStatus_Active NeighborStatus_State = 2
	//
	NeighborStatus_OpenSent NeighborStatus_State = 3
	//
	NeighborStatus_OpenConfirmed NeighborStatus_State = 4
	//
	NeighborStatus_Established NeighborStatus_State = 5
)

var NeighborStatus_State_name = map[int32]string{
	0: "Idle",
	1: "Connect",
	2: "Active",
	3: "OpenSent",
	4: "OpenConfirmed",
	5: "Established",
}
var NeighborStatus_State_value = map[string]int32{
	"Idle":          0,
	"Connect":       1,
	"Active":        2,
	"OpenSent":      3,
	"OpenConfirmed": 4,
	"Established":   5,
}

func (NeighborStatus_State) EnumDescriptor() ([]byte, []int) {
	return fileDescriptorRouting, []int{1, 0}
}

//
type Neighbor struct {
	//
	api.TypeMeta `protobuf:"bytes,1,opt,name=T,json=,inline,embedded=T" json:",inline"`
	//
	api.ObjectMeta `protobuf:"bytes,2,opt,name=O,json=meta,inline,embedded=O" json:"meta,inline"`
	//
	Spec network.BGPNeighbor `protobuf:"bytes,3,opt,name=Spec,json=spec" json:"spec"`
	//
	Status NeighborStatus `protobuf:"bytes,4,opt,name=Status,json=status" json:"status"`
}

func (m *Neighbor) Reset()                    { *m = Neighbor{} }
func (m *Neighbor) String() string            { return proto.CompactTextString(m) }
func (*Neighbor) ProtoMessage()               {}
func (*Neighbor) Descriptor() ([]byte, []int) { return fileDescriptorRouting, []int{0} }

func (m *Neighbor) GetSpec() network.BGPNeighbor {
	if m != nil {
		return m.Spec
	}
	return network.BGPNeighbor{}
}

func (m *Neighbor) GetStatus() NeighborStatus {
	if m != nil {
		return m.Status
	}
	return NeighborStatus{}
}

//
type NeighborStatus struct {
	//
	Status string `protobuf:"bytes,1,opt,name=Status,json=status,proto3" json:"status"`
}

func (m *NeighborStatus) Reset()                    { *m = NeighborStatus{} }
func (m *NeighborStatus) String() string            { return proto.CompactTextString(m) }
func (*NeighborStatus) ProtoMessage()               {}
func (*NeighborStatus) Descriptor() ([]byte, []int) { return fileDescriptorRouting, []int{1} }

func (m *NeighborStatus) GetStatus() string {
	if m != nil {
		return m.Status
	}
	return ""
}

func init() {
	proto.RegisterType((*Neighbor)(nil), "routing.Neighbor")
	proto.RegisterType((*NeighborStatus)(nil), "routing.NeighborStatus")
	proto.RegisterEnum("routing.NeighborStatus_State", NeighborStatus_State_name, NeighborStatus_State_value)
}
func (m *Neighbor) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *Neighbor) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	dAtA[i] = 0xa
	i++
	i = encodeVarintRouting(dAtA, i, uint64(m.TypeMeta.Size()))
	n1, err := m.TypeMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n1
	dAtA[i] = 0x12
	i++
	i = encodeVarintRouting(dAtA, i, uint64(m.ObjectMeta.Size()))
	n2, err := m.ObjectMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n2
	dAtA[i] = 0x1a
	i++
	i = encodeVarintRouting(dAtA, i, uint64(m.Spec.Size()))
	n3, err := m.Spec.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n3
	dAtA[i] = 0x22
	i++
	i = encodeVarintRouting(dAtA, i, uint64(m.Status.Size()))
	n4, err := m.Status.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n4
	return i, nil
}

func (m *NeighborStatus) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *NeighborStatus) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Status) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintRouting(dAtA, i, uint64(len(m.Status)))
		i += copy(dAtA[i:], m.Status)
	}
	return i, nil
}

func encodeVarintRouting(dAtA []byte, offset int, v uint64) int {
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return offset + 1
}
func (m *Neighbor) Size() (n int) {
	var l int
	_ = l
	l = m.TypeMeta.Size()
	n += 1 + l + sovRouting(uint64(l))
	l = m.ObjectMeta.Size()
	n += 1 + l + sovRouting(uint64(l))
	l = m.Spec.Size()
	n += 1 + l + sovRouting(uint64(l))
	l = m.Status.Size()
	n += 1 + l + sovRouting(uint64(l))
	return n
}

func (m *NeighborStatus) Size() (n int) {
	var l int
	_ = l
	l = len(m.Status)
	if l > 0 {
		n += 1 + l + sovRouting(uint64(l))
	}
	return n
}

func sovRouting(x uint64) (n int) {
	for {
		n++
		x >>= 7
		if x == 0 {
			break
		}
	}
	return n
}
func sozRouting(x uint64) (n int) {
	return sovRouting(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *Neighbor) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowRouting
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
			return fmt.Errorf("proto: Neighbor: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: Neighbor: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field TypeMeta", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowRouting
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
				return ErrInvalidLengthRouting
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
					return ErrIntOverflowRouting
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
				return ErrInvalidLengthRouting
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
					return ErrIntOverflowRouting
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
				return ErrInvalidLengthRouting
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
					return ErrIntOverflowRouting
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
				return ErrInvalidLengthRouting
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
			skippy, err := skipRouting(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthRouting
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
func (m *NeighborStatus) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowRouting
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
			return fmt.Errorf("proto: NeighborStatus: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: NeighborStatus: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Status", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowRouting
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
				return ErrInvalidLengthRouting
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Status = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipRouting(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthRouting
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
func skipRouting(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowRouting
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
					return 0, ErrIntOverflowRouting
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
					return 0, ErrIntOverflowRouting
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
				return 0, ErrInvalidLengthRouting
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowRouting
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
				next, err := skipRouting(dAtA[start:])
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
	ErrInvalidLengthRouting = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowRouting   = fmt.Errorf("proto: integer overflow")
)

func init() { proto.RegisterFile("routing.proto", fileDescriptorRouting) }

var fileDescriptorRouting = []byte{
	// 442 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x74, 0x91, 0xc1, 0x6e, 0xd3, 0x40,
	0x10, 0x86, 0xb3, 0xa9, 0x9b, 0xa4, 0xeb, 0xa6, 0x31, 0x0b, 0x52, 0xd3, 0x4a, 0x24, 0x28, 0x12,
	0x52, 0x11, 0xc8, 0x96, 0x40, 0xea, 0x81, 0x0b, 0xe0, 0x28, 0x42, 0x1c, 0x20, 0x55, 0xd2, 0x23,
	0x97, 0xb5, 0x3d, 0x24, 0x0b, 0xce, 0xec, 0xca, 0xbb, 0x6e, 0xc5, 0x03, 0xc0, 0x91, 0x07, 0xe1,
	0xca, 0x4b, 0xf4, 0x58, 0xf1, 0x00, 0x11, 0xf2, 0x09, 0xf5, 0x29, 0x90, 0x37, 0x36, 0x6a, 0x84,
	0xb8, 0x78, 0xfc, 0x6b, 0xe6, 0xfb, 0xff, 0xd1, 0x2c, 0xed, 0x66, 0x32, 0x37, 0x02, 0x17, 0xbe,
	0xca, 0xa4, 0x91, 0xac, 0x5d, 0xc9, 0xe3, 0xc9, 0x42, 0x98, 0x65, 0x1e, 0xf9, 0xb1, 0x5c, 0x05,
	0x0a, 0x50, 0x73, 0x4c, 0x64, 0xa0, 0x2f, 0x83, 0x0b, 0x40, 0x11, 0x43, 0x90, 0x1b, 0x91, 0xea,
	0x80, 0x2b, 0xb1, 0x00, 0x0c, 0x38, 0xa2, 0x34, 0xdc, 0x08, 0x89, 0x3a, 0x10, 0x18, 0xa7, 0x79,
	0x02, 0x7a, 0xe3, 0x77, 0xfc, 0xf0, 0x3f, 0x36, 0x5c, 0x89, 0x60, 0x05, 0x86, 0x57, 0x63, 0x6e,
	0x19, 0x0b, 0x1b, 0x31, 0xfa, 0xd6, 0xa4, 0x9d, 0x77, 0x20, 0x16, 0xcb, 0x48, 0x66, 0xec, 0x94,
	0x92, 0xf3, 0x3e, 0x79, 0x40, 0x4e, 0xdc, 0xa7, 0x5d, 0x9f, 0x2b, 0xe1, 0x9f, 0x7f, 0x56, 0xf0,
	0x16, 0x0c, 0x0f, 0xef, 0x5e, 0xad, 0x87, 0x8d, 0xeb, 0xf5, 0x90, 0xdc, 0xac, 0x87, 0xed, 0x27,
	0x02, 0x53, 0x81, 0x30, 0xab, 0x7f, 0xd8, 0x4b, 0x4a, 0xa6, 0xfd, 0xa6, 0xe5, 0x7a, 0x96, 0x9b,
	0x46, 0x1f, 0x21, 0x36, 0x96, 0x3c, 0xbc, 0x45, 0xba, 0xe5, 0x16, 0x35, 0x7d, 0x5b, 0xb0, 0x53,
	0xea, 0xcc, 0x15, 0xc4, 0xfd, 0x1d, 0x6b, 0x72, 0xcf, 0x47, 0x30, 0x97, 0x32, 0xfb, 0xe4, 0x87,
	0xaf, 0xcf, 0xea, 0xed, 0xc2, 0xfd, 0xd2, 0xe9, 0x66, 0x3d, 0x74, 0xb4, 0x82, 0x78, 0x66, 0xbf,
	0xec, 0x05, 0x6d, 0xcd, 0x0d, 0x37, 0xb9, 0xee, 0x3b, 0x96, 0x3c, 0xf4, 0xeb, 0x13, 0xd7, 0xd8,
	0xa6, 0x1d, 0x1e, 0x54, 0x70, 0x4b, 0x5b, 0x3d, 0xab, 0xea, 0xf3, 0xde, 0xcf, 0x2f, 0x47, 0x2e,
	0xdb, 0xc3, 0x6a, 0x5a, 0x8f, 0x7e, 0x10, 0x7a, 0xb0, 0xcd, 0xb2, 0xf1, 0xdf, 0x90, 0xf2, 0x36,
	0x7b, 0xe1, 0xe3, 0xef, 0x5f, 0x8f, 0xee, 0xcf, 0x4d, 0x36, 0xc1, 0x7c, 0x75, 0xb2, 0x3d, 0xeb,
	0x97, 0x05, 0x1e, 0xfd, 0x1b, 0x34, 0x7a, 0x4f, 0x77, 0x6d, 0x8f, 0x75, 0xa8, 0xf3, 0x26, 0x49,
	0xc1, 0x6b, 0x30, 0x97, 0xb6, 0xc7, 0x12, 0x11, 0x62, 0xe3, 0x11, 0x46, 0x69, 0xeb, 0x55, 0x6c,
	0xc4, 0x05, 0x78, 0x4d, 0xb6, 0x4f, 0x3b, 0x53, 0x05, 0x38, 0x07, 0x34, 0xde, 0x0e, 0xbb, 0x43,
	0xbb, 0xa5, 0x1a, 0x4b, 0xfc, 0x20, 0xb2, 0x15, 0x24, 0x9e, 0xc3, 0x7a, 0xd4, 0x9d, 0x68, 0xc3,
	0xa3, 0x54, 0xe8, 0x25, 0x24, 0xde, 0x6e, 0xe8, 0x5d, 0x15, 0x03, 0x72, 0x5d, 0x0c, 0xc8, 0xaf,
	0x62, 0x40, 0x7e, 0x17, 0x83, 0xc6, 0x59, 0x23, 0x6a, 0xd9, 0x17, 0x7e, 0xf6, 0x27, 0x00, 0x00,
	0xff, 0xff, 0x78, 0x45, 0x16, 0xe7, 0x76, 0x02, 0x00, 0x00,
}