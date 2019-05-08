// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: attributes.proto

/*
Package eventattrs is a generated protocol buffer package.

It is generated from these files:
	attributes.proto

It has these top-level messages:
	Key
*/
package eventattrs

import proto "github.com/gogo/protobuf/proto"
import fmt "fmt"
import math "math"

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

// list of V1 event types supported by the cluster service
type Category int32

const (
	// Cluster related events (leader election, K8s, CMD, NMD, etc..)
	Category_Cluster Category = 0
	// Network related events (linkmgr, etc..)
	Category_Network Category = 1
	// System related events (service events, system boot/reboot events, etc...)
	Category_System Category = 2
)

var Category_name = map[int32]string{
	0: "Cluster",
	1: "Network",
	2: "System",
}
var Category_value = map[string]int32{
	"Cluster": 0,
	"Network": 1,
	"System":  2,
}

func (x Category) String() string {
	return proto.EnumName(Category_name, int32(x))
}
func (Category) EnumDescriptor() ([]byte, []int) { return fileDescriptorAttributes, []int{0} }

// Severity Level of an event
type Severity int32

const (
	//
	Severity_INFO Severity = 0
	//
	Severity_WARN Severity = 1
	//
	Severity_CRITICAL Severity = 2
	//
	Severity_DEBUG Severity = 3
)

var Severity_name = map[int32]string{
	0: "INFO",
	1: "WARN",
	2: "CRITICAL",
	3: "DEBUG",
}
var Severity_value = map[string]int32{
	"INFO":     0,
	"WARN":     1,
	"CRITICAL": 2,
	"DEBUG":    3,
}

func (x Severity) String() string {
	return proto.EnumName(Severity_name, int32(x))
}
func (Severity) EnumDescriptor() ([]byte, []int) { return fileDescriptorAttributes, []int{1} }

// Common event key that can be used when there is no real object reference
// attached to the event. e.g. system/service events
type Key struct {
	// name of the service or system
	Name string `protobuf:"bytes,1,opt,name=Name,proto3" json:"Name,omitempty"`
}

func (m *Key) Reset()                    { *m = Key{} }
func (m *Key) String() string            { return proto.CompactTextString(m) }
func (*Key) ProtoMessage()               {}
func (*Key) Descriptor() ([]byte, []int) { return fileDescriptorAttributes, []int{0} }

func (m *Key) GetName() string {
	if m != nil {
		return m.Name
	}
	return ""
}

func init() {
	proto.RegisterType((*Key)(nil), "eventattrs.Key")
	proto.RegisterEnum("eventattrs.Category", Category_name, Category_value)
	proto.RegisterEnum("eventattrs.Severity", Severity_name, Severity_value)
}
func (m *Key) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *Key) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Name) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintAttributes(dAtA, i, uint64(len(m.Name)))
		i += copy(dAtA[i:], m.Name)
	}
	return i, nil
}

func encodeVarintAttributes(dAtA []byte, offset int, v uint64) int {
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return offset + 1
}
func (m *Key) Size() (n int) {
	var l int
	_ = l
	l = len(m.Name)
	if l > 0 {
		n += 1 + l + sovAttributes(uint64(l))
	}
	return n
}

func sovAttributes(x uint64) (n int) {
	for {
		n++
		x >>= 7
		if x == 0 {
			break
		}
	}
	return n
}
func sozAttributes(x uint64) (n int) {
	return sovAttributes(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *Key) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowAttributes
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
			return fmt.Errorf("proto: Key: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: Key: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Name", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowAttributes
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
				return ErrInvalidLengthAttributes
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Name = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipAttributes(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthAttributes
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
func skipAttributes(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowAttributes
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
					return 0, ErrIntOverflowAttributes
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
					return 0, ErrIntOverflowAttributes
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
				return 0, ErrInvalidLengthAttributes
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowAttributes
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
				next, err := skipAttributes(dAtA[start:])
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
	ErrInvalidLengthAttributes = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowAttributes   = fmt.Errorf("proto: integer overflow")
)

func init() { proto.RegisterFile("attributes.proto", fileDescriptorAttributes) }

var fileDescriptorAttributes = []byte{
	// 198 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0xe2, 0x12, 0x48, 0x2c, 0x29, 0x29,
	0xca, 0x4c, 0x2a, 0x2d, 0x49, 0x2d, 0xd6, 0x2b, 0x28, 0xca, 0x2f, 0xc9, 0x17, 0xe2, 0x4a, 0x2d,
	0x4b, 0xcd, 0x2b, 0x01, 0x09, 0x17, 0x2b, 0x49, 0x72, 0x31, 0x7b, 0xa7, 0x56, 0x0a, 0x09, 0x71,
	0xb1, 0xf8, 0x25, 0xe6, 0xa6, 0x4a, 0x30, 0x2a, 0x30, 0x6a, 0x70, 0x06, 0x81, 0xd9, 0x5a, 0x06,
	0x5c, 0x1c, 0xce, 0x89, 0x25, 0xa9, 0xe9, 0xf9, 0x45, 0x95, 0x42, 0xdc, 0x5c, 0xec, 0xce, 0x39,
	0xa5, 0xc5, 0x25, 0xa9, 0x45, 0x02, 0x0c, 0x20, 0x8e, 0x5f, 0x6a, 0x49, 0x79, 0x7e, 0x51, 0xb6,
	0x00, 0xa3, 0x10, 0x17, 0x17, 0x5b, 0x70, 0x65, 0x71, 0x49, 0x6a, 0xae, 0x00, 0x93, 0x96, 0x39,
	0x17, 0x47, 0x70, 0x6a, 0x59, 0x6a, 0x51, 0x66, 0x49, 0xa5, 0x10, 0x07, 0x17, 0x8b, 0xa7, 0x9f,
	0x9b, 0xbf, 0x00, 0x03, 0x88, 0x15, 0xee, 0x18, 0xe4, 0x27, 0xc0, 0x28, 0xc4, 0xc3, 0xc5, 0xe1,
	0x1c, 0xe4, 0x19, 0xe2, 0xe9, 0xec, 0xe8, 0x23, 0xc0, 0x24, 0xc4, 0xc9, 0xc5, 0xea, 0xe2, 0xea,
	0x14, 0xea, 0x2e, 0xc0, 0xec, 0xc4, 0x73, 0xe2, 0x91, 0x1c, 0xe3, 0x85, 0x47, 0x72, 0x8c, 0x0f,
	0x1e, 0xc9, 0x31, 0x26, 0xb1, 0x81, 0x9d, 0x69, 0x0c, 0x08, 0x00, 0x00, 0xff, 0xff, 0xf8, 0x87,
	0xbc, 0x3b, 0xba, 0x00, 0x00, 0x00,
}
