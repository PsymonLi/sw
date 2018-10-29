// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: hal_metrics.proto.notready

package halproto

import proto "github.com/golang/protobuf/proto"
import fmt "fmt"
import math "math"
import delphi "github.com/pensando/sw/nic/delphi/proto/delphi"

import encoding_binary "encoding/binary"

import io "io"

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

type PortCounters struct {
	Count *delphi.Counter `protobuf:"bytes,1,opt,name=count" json:"count,omitempty"`
}

func (m *PortCounters) Reset()                    { *m = PortCounters{} }
func (m *PortCounters) String() string            { return proto.CompactTextString(m) }
func (*PortCounters) ProtoMessage()               {}
func (*PortCounters) Descriptor() ([]byte, []int) { return fileDescriptorHalMetricsNotready, []int{0} }

func (m *PortCounters) GetCount() *delphi.Counter {
	if m != nil {
		return m.Count
	}
	return nil
}

type PortMetrics struct {
	Key      uint32          `protobuf:"fixed32,1,opt,name=Key,proto3" json:"Key,omitempty"`
	Counters []*PortCounters `protobuf:"bytes,2,rep,name=counters" json:"counters,omitempty"`
}

func (m *PortMetrics) Reset()                    { *m = PortMetrics{} }
func (m *PortMetrics) String() string            { return proto.CompactTextString(m) }
func (*PortMetrics) ProtoMessage()               {}
func (*PortMetrics) Descriptor() ([]byte, []int) { return fileDescriptorHalMetricsNotready, []int{1} }

func (m *PortMetrics) GetKey() uint32 {
	if m != nil {
		return m.Key
	}
	return 0
}

func (m *PortMetrics) GetCounters() []*PortCounters {
	if m != nil {
		return m.Counters
	}
	return nil
}

func init() {
	proto.RegisterType((*PortCounters)(nil), "metrics.PortCounters")
	proto.RegisterType((*PortMetrics)(nil), "metrics.PortMetrics")
}
func (m *PortCounters) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *PortCounters) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if m.Count != nil {
		dAtA[i] = 0xa
		i++
		i = encodeVarintHalMetricsNotready(dAtA, i, uint64(m.Count.Size()))
		n1, err := m.Count.MarshalTo(dAtA[i:])
		if err != nil {
			return 0, err
		}
		i += n1
	}
	return i, nil
}

func (m *PortMetrics) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *PortMetrics) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if m.Key != 0 {
		dAtA[i] = 0xd
		i++
		encoding_binary.LittleEndian.PutUint32(dAtA[i:], uint32(m.Key))
		i += 4
	}
	if len(m.Counters) > 0 {
		for _, msg := range m.Counters {
			dAtA[i] = 0x12
			i++
			i = encodeVarintHalMetricsNotready(dAtA, i, uint64(msg.Size()))
			n, err := msg.MarshalTo(dAtA[i:])
			if err != nil {
				return 0, err
			}
			i += n
		}
	}
	return i, nil
}

func encodeVarintHalMetricsNotready(dAtA []byte, offset int, v uint64) int {
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return offset + 1
}
func (m *PortCounters) Size() (n int) {
	var l int
	_ = l
	if m.Count != nil {
		l = m.Count.Size()
		n += 1 + l + sovHalMetricsNotready(uint64(l))
	}
	return n
}

func (m *PortMetrics) Size() (n int) {
	var l int
	_ = l
	if m.Key != 0 {
		n += 5
	}
	if len(m.Counters) > 0 {
		for _, e := range m.Counters {
			l = e.Size()
			n += 1 + l + sovHalMetricsNotready(uint64(l))
		}
	}
	return n
}

func sovHalMetricsNotready(x uint64) (n int) {
	for {
		n++
		x >>= 7
		if x == 0 {
			break
		}
	}
	return n
}
func sozHalMetricsNotready(x uint64) (n int) {
	return sovHalMetricsNotready(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *PortCounters) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowHalMetricsNotready
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
			return fmt.Errorf("proto: PortCounters: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: PortCounters: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Count", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowHalMetricsNotready
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
				return ErrInvalidLengthHalMetricsNotready
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if m.Count == nil {
				m.Count = &delphi.Counter{}
			}
			if err := m.Count.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipHalMetricsNotready(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthHalMetricsNotready
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
func (m *PortMetrics) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowHalMetricsNotready
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
			return fmt.Errorf("proto: PortMetrics: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: PortMetrics: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 5 {
				return fmt.Errorf("proto: wrong wireType = %d for field Key", wireType)
			}
			m.Key = 0
			if (iNdEx + 4) > l {
				return io.ErrUnexpectedEOF
			}
			m.Key = uint32(encoding_binary.LittleEndian.Uint32(dAtA[iNdEx:]))
			iNdEx += 4
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Counters", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowHalMetricsNotready
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
				return ErrInvalidLengthHalMetricsNotready
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Counters = append(m.Counters, &PortCounters{})
			if err := m.Counters[len(m.Counters)-1].Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipHalMetricsNotready(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthHalMetricsNotready
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
func skipHalMetricsNotready(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowHalMetricsNotready
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
					return 0, ErrIntOverflowHalMetricsNotready
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
					return 0, ErrIntOverflowHalMetricsNotready
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
				return 0, ErrInvalidLengthHalMetricsNotready
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowHalMetricsNotready
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
				next, err := skipHalMetricsNotready(dAtA[start:])
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
	ErrInvalidLengthHalMetricsNotready = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowHalMetricsNotready   = fmt.Errorf("proto: integer overflow")
)

func init() { proto.RegisterFile("hal_metrics.proto.notready", fileDescriptorHalMetricsNotready) }

var fileDescriptorHalMetricsNotready = []byte{
	// 174 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0xe2, 0x92, 0xca, 0x48, 0xcc, 0x89,
	0xcf, 0x4d, 0x2d, 0x29, 0xca, 0x4c, 0x2e, 0xd6, 0x2b, 0x28, 0xca, 0x2f, 0xc9, 0xd7, 0xcb, 0xcb,
	0x2f, 0x29, 0x4a, 0x4d, 0x4c, 0xa9, 0x14, 0x62, 0x87, 0x8a, 0x4b, 0xf1, 0xa4, 0xa4, 0xe6, 0x14,
	0x64, 0x64, 0x42, 0xe4, 0x95, 0x4c, 0xb9, 0x78, 0x02, 0xf2, 0x8b, 0x4a, 0x9c, 0xf3, 0x4b, 0xf3,
	0x4a, 0x52, 0x8b, 0x8a, 0x85, 0x54, 0xb9, 0x58, 0x93, 0x41, 0x6c, 0x09, 0x46, 0x05, 0x46, 0x0d,
	0x6e, 0x23, 0x7e, 0x3d, 0xa8, 0x6a, 0xa8, 0x82, 0x20, 0x88, 0xac, 0x52, 0x10, 0x17, 0x37, 0x48,
	0x9b, 0x2f, 0xc4, 0x4c, 0x21, 0x01, 0x2e, 0x66, 0xef, 0xd4, 0x4a, 0xb0, 0x1e, 0xf6, 0x20, 0x10,
	0x53, 0xc8, 0x90, 0x8b, 0x23, 0x19, 0x6a, 0xa6, 0x04, 0x93, 0x02, 0xb3, 0x06, 0xb7, 0x91, 0xa8,
	0x1e, 0xcc, 0x65, 0xc8, 0x16, 0x06, 0xc1, 0x95, 0x39, 0x09, 0x9c, 0x78, 0x24, 0xc7, 0x78, 0xe1,
	0x91, 0x1c, 0xe3, 0x83, 0x47, 0x72, 0x8c, 0x33, 0x1e, 0xcb, 0x31, 0x24, 0xb1, 0x81, 0xdd, 0x68,
	0x0c, 0x08, 0x00, 0x00, 0xff, 0xff, 0x55, 0x8a, 0x72, 0xdb, 0xd8, 0x00, 0x00, 0x00,
}
