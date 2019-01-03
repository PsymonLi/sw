// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: match.proto

package netproto

import proto "github.com/golang/protobuf/proto"
import fmt "fmt"
import math "math"
import _ "google.golang.org/genproto/googleapis/api/annotations"
import _ "github.com/pensando/sw/venice/utils/apigen/annotations"
import _ "github.com/gogo/protobuf/gogoproto"
import _ "github.com/pensando/sw/api"

import io "io"

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

// Common MatchSelector. ToDo Add ICMP Match criteria
type MatchSelector struct {
	// Automatically interpret the string as an octet, a CIDR or an hyphen separated range
	Addresses []string `protobuf:"bytes,1,rep,name=Addresses" json:"addresses,omitempty"`
	// Match on the security group
	SecurityGroups []string `protobuf:"bytes,2,rep,name=SecurityGroups" json:"security-groups,omitempty"`
	// Match on the App info
	AppConfigs []*AppConfig `protobuf:"bytes,4,rep,name=AppConfigs" json:"app-configs,omitempty"`
}

func (m *MatchSelector) Reset()                    { *m = MatchSelector{} }
func (m *MatchSelector) String() string            { return proto.CompactTextString(m) }
func (*MatchSelector) ProtoMessage()               {}
func (*MatchSelector) Descriptor() ([]byte, []int) { return fileDescriptorMatch, []int{0} }

func (m *MatchSelector) GetAddresses() []string {
	if m != nil {
		return m.Addresses
	}
	return nil
}

func (m *MatchSelector) GetSecurityGroups() []string {
	if m != nil {
		return m.SecurityGroups
	}
	return nil
}

func (m *MatchSelector) GetAppConfigs() []*AppConfig {
	if m != nil {
		return m.AppConfigs
	}
	return nil
}

type AppConfig struct {
	// Protocol for the app. If the protocol is icmp, then the port field gets interpreted as ICMP Type.
	// If the app name is specified all the protocol port information will be fetched from the corresponding app object
	Protocol string `protobuf:"bytes,1,opt,name=Protocol,proto3" json:"protocol,omitempty"`
	// Port for the app
	Port string `protobuf:"bytes,2,opt,name=Port,proto3" json:"port,omitempty"`
}

func (m *AppConfig) Reset()                    { *m = AppConfig{} }
func (m *AppConfig) String() string            { return proto.CompactTextString(m) }
func (*AppConfig) ProtoMessage()               {}
func (*AppConfig) Descriptor() ([]byte, []int) { return fileDescriptorMatch, []int{1} }

func (m *AppConfig) GetProtocol() string {
	if m != nil {
		return m.Protocol
	}
	return ""
}

func (m *AppConfig) GetPort() string {
	if m != nil {
		return m.Port
	}
	return ""
}

func init() {
	proto.RegisterType((*MatchSelector)(nil), "netproto.MatchSelector")
	proto.RegisterType((*AppConfig)(nil), "netproto.AppConfig")
}
func (m *MatchSelector) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *MatchSelector) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Addresses) > 0 {
		for _, s := range m.Addresses {
			dAtA[i] = 0xa
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
	if len(m.SecurityGroups) > 0 {
		for _, s := range m.SecurityGroups {
			dAtA[i] = 0x12
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
	if len(m.AppConfigs) > 0 {
		for _, msg := range m.AppConfigs {
			dAtA[i] = 0x22
			i++
			i = encodeVarintMatch(dAtA, i, uint64(msg.Size()))
			n, err := msg.MarshalTo(dAtA[i:])
			if err != nil {
				return 0, err
			}
			i += n
		}
	}
	return i, nil
}

func (m *AppConfig) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *AppConfig) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Protocol) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintMatch(dAtA, i, uint64(len(m.Protocol)))
		i += copy(dAtA[i:], m.Protocol)
	}
	if len(m.Port) > 0 {
		dAtA[i] = 0x12
		i++
		i = encodeVarintMatch(dAtA, i, uint64(len(m.Port)))
		i += copy(dAtA[i:], m.Port)
	}
	return i, nil
}

func encodeVarintMatch(dAtA []byte, offset int, v uint64) int {
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return offset + 1
}
func (m *MatchSelector) Size() (n int) {
	var l int
	_ = l
	if len(m.Addresses) > 0 {
		for _, s := range m.Addresses {
			l = len(s)
			n += 1 + l + sovMatch(uint64(l))
		}
	}
	if len(m.SecurityGroups) > 0 {
		for _, s := range m.SecurityGroups {
			l = len(s)
			n += 1 + l + sovMatch(uint64(l))
		}
	}
	if len(m.AppConfigs) > 0 {
		for _, e := range m.AppConfigs {
			l = e.Size()
			n += 1 + l + sovMatch(uint64(l))
		}
	}
	return n
}

func (m *AppConfig) Size() (n int) {
	var l int
	_ = l
	l = len(m.Protocol)
	if l > 0 {
		n += 1 + l + sovMatch(uint64(l))
	}
	l = len(m.Port)
	if l > 0 {
		n += 1 + l + sovMatch(uint64(l))
	}
	return n
}

func sovMatch(x uint64) (n int) {
	for {
		n++
		x >>= 7
		if x == 0 {
			break
		}
	}
	return n
}
func sozMatch(x uint64) (n int) {
	return sovMatch(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *MatchSelector) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowMatch
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
			return fmt.Errorf("proto: MatchSelector: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: MatchSelector: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Addresses", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMatch
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
				return ErrInvalidLengthMatch
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Addresses = append(m.Addresses, string(dAtA[iNdEx:postIndex]))
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field SecurityGroups", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMatch
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
				return ErrInvalidLengthMatch
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.SecurityGroups = append(m.SecurityGroups, string(dAtA[iNdEx:postIndex]))
			iNdEx = postIndex
		case 4:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field AppConfigs", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMatch
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
				return ErrInvalidLengthMatch
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.AppConfigs = append(m.AppConfigs, &AppConfig{})
			if err := m.AppConfigs[len(m.AppConfigs)-1].Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipMatch(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthMatch
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
func (m *AppConfig) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowMatch
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
			return fmt.Errorf("proto: AppConfig: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: AppConfig: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Protocol", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMatch
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
				return ErrInvalidLengthMatch
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Protocol = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Port", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMatch
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
				return ErrInvalidLengthMatch
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Port = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipMatch(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthMatch
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
func skipMatch(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowMatch
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
					return 0, ErrIntOverflowMatch
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
					return 0, ErrIntOverflowMatch
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
				return 0, ErrInvalidLengthMatch
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowMatch
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
				next, err := skipMatch(dAtA[start:])
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
	ErrInvalidLengthMatch = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowMatch   = fmt.Errorf("proto: integer overflow")
)

func init() { proto.RegisterFile("match.proto", fileDescriptorMatch) }

var fileDescriptorMatch = []byte{
	// 363 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x74, 0x90, 0xdd, 0xca, 0xd3, 0x30,
	0x18, 0xc7, 0xcd, 0xfb, 0x0e, 0x59, 0x33, 0x1c, 0x92, 0xa1, 0x76, 0x43, 0xdb, 0x31, 0x50, 0x76,
	0xe0, 0x1a, 0x98, 0x78, 0x01, 0xab, 0x0c, 0x8f, 0x94, 0xe1, 0xae, 0x20, 0x4b, 0xb3, 0x2c, 0xd0,
	0xe6, 0x09, 0x4d, 0xaa, 0xec, 0x4e, 0xbc, 0x24, 0x0f, 0xbd, 0x01, 0x8b, 0xcc, 0xb3, 0x5e, 0x85,
	0x34, 0xfb, 0xb0, 0x08, 0xef, 0xd9, 0xf3, 0x3c, 0xff, 0xdf, 0xff, 0xd7, 0x0f, 0x3c, 0x28, 0x98,
	0xe3, 0x87, 0xc4, 0x94, 0xe0, 0x80, 0xf4, 0xb5, 0x70, 0x7e, 0x9a, 0xbc, 0x94, 0x00, 0x32, 0x17,
	0x94, 0x19, 0x45, 0x99, 0xd6, 0xe0, 0x98, 0x53, 0xa0, 0xed, 0x99, 0x9b, 0xac, 0xa5, 0x72, 0x87,
	0x6a, 0x97, 0x70, 0x28, 0xa8, 0x11, 0xda, 0x32, 0x9d, 0x01, 0xb5, 0xdf, 0xe8, 0x57, 0xa1, 0x15,
	0x17, 0xb4, 0x72, 0x2a, 0xb7, 0x6d, 0x55, 0x0a, 0xdd, 0x6d, 0x53, 0xa5, 0x79, 0x5e, 0x65, 0xe2,
	0xaa, 0x59, 0x74, 0x34, 0x12, 0x24, 0x50, 0x7f, 0xde, 0x55, 0x7b, 0xbf, 0xf9, 0xc5, 0x4f, 0x17,
	0xfc, 0xf5, 0x03, 0x4f, 0x6d, 0xdf, 0xb1, 0x10, 0x8e, 0x9d, 0xb1, 0xd9, 0x2f, 0x84, 0x9f, 0x7c,
	0x6a, 0x3f, 0x6a, 0x2b, 0x72, 0xc1, 0x1d, 0x94, 0xe4, 0x3d, 0x0e, 0x56, 0x59, 0x56, 0x0a, 0x6b,
	0x85, 0x0d, 0xd1, 0xf4, 0x7e, 0x1e, 0xa4, 0x2f, 0x9a, 0x3a, 0x1e, 0xb1, 0xeb, 0xf1, 0x2d, 0x14,
	0xca, 0x89, 0xc2, 0xb8, 0xe3, 0x97, 0x7f, 0x24, 0x59, 0xe3, 0xe1, 0x56, 0xf0, 0xaa, 0x54, 0xee,
	0xf8, 0xb1, 0x84, 0xca, 0xd8, 0xf0, 0xce, 0x77, 0x5f, 0x35, 0x75, 0x3c, 0xb6, 0x97, 0x64, 0x21,
	0x7d, 0xd4, 0x31, 0xfc, 0x57, 0x22, 0x9f, 0x31, 0x5e, 0x19, 0xf3, 0x01, 0xf4, 0x5e, 0x49, 0x1b,
	0xf6, 0xa6, 0xf7, 0xf3, 0xc1, 0x72, 0x94, 0x5c, 0xff, 0x74, 0x72, 0xcb, 0xd2, 0x71, 0x53, 0xc7,
	0xcf, 0x98, 0x31, 0x0b, 0x7e, 0x66, 0x3b, 0xce, 0x8e, 0x61, 0x26, 0x71, 0x70, 0xdb, 0xc8, 0x12,
	0xf7, 0x37, 0xad, 0x86, 0x43, 0x1e, 0xa2, 0x29, 0x9a, 0x07, 0xe9, 0xf3, 0xa6, 0x8e, 0x89, 0xb9,
	0xdc, 0x3a, 0x8a, 0x1b, 0x47, 0xde, 0xe0, 0xde, 0x06, 0x4a, 0x17, 0xde, 0x79, 0x9e, 0x34, 0x75,
	0x3c, 0x34, 0x50, 0xba, 0x0e, 0xeb, 0xf3, 0xf4, 0xe9, 0x8f, 0x53, 0x84, 0x7e, 0x9e, 0x22, 0xf4,
	0xfb, 0x14, 0xa1, 0xef, 0x7f, 0xa2, 0x47, 0x1b, 0xb4, 0x7b, 0xec, 0xcd, 0xef, 0xfe, 0x06, 0x00,
	0x00, 0xff, 0xff, 0x7b, 0xc9, 0xfd, 0x6c, 0x37, 0x02, 0x00, 0x00,
}
