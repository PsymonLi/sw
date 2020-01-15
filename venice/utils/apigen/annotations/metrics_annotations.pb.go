// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: metrics_annotations.proto

/*
	Package venice is a generated protocol buffer package.

	It is generated from these files:
		metrics_annotations.proto

	It has these top-level messages:
		MetricInfo
		MetricFieldInfo
*/
package venice

import proto "github.com/gogo/protobuf/proto"
import fmt "fmt"
import math "math"
import google_protobuf "github.com/gogo/protobuf/protoc-gen-gogo/descriptor"

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

type MetricUnitType int32

const (
	MetricUnitType_Count      MetricUnitType = 0
	MetricUnitType_Percentage MetricUnitType = 1
	MetricUnitType_Bytes      MetricUnitType = 2
	MetricUnitType_ID         MetricUnitType = 3
	MetricUnitType_Gauge      MetricUnitType = 4
	MetricUnitType_Bitmap     MetricUnitType = 5
	MetricUnitType_Celsius    MetricUnitType = 6
	MetricUnitType_KB         MetricUnitType = 7
	MetricUnitType_MWatts     MetricUnitType = 8
	MetricUnitType_MHz        MetricUnitType = 9
)

var MetricUnitType_name = map[int32]string{
	0: "Count",
	1: "Percentage",
	2: "Bytes",
	3: "ID",
	4: "Gauge",
	5: "Bitmap",
	6: "Celsius",
	7: "KB",
	8: "MWatts",
	9: "MHz",
}
var MetricUnitType_value = map[string]int32{
	"Count":      0,
	"Percentage": 1,
	"Bytes":      2,
	"ID":         3,
	"Gauge":      4,
	"Bitmap":     5,
	"Celsius":    6,
	"KB":         7,
	"MWatts":     8,
	"MHz":        9,
}

func (x MetricUnitType) String() string {
	return proto.EnumName(MetricUnitType_name, int32(x))
}
func (MetricUnitType) EnumDescriptor() ([]byte, []int) {
	return fileDescriptorMetricsAnnotations, []int{0}
}

// defines as a hint of type of metric
type MetricScope int32

const (
	MetricScope_UnknownScope      MetricScope = 0
	MetricScope_PerNode           MetricScope = 1
	MetricScope_PerASIC           MetricScope = 2
	MetricScope_PerFTE            MetricScope = 3
	MetricScope_PerPort           MetricScope = 4
	MetricScope_PerLIF            MetricScope = 5
	MetricScope_PerLIFPerQ        MetricScope = 6
	MetricScope_PerRingPerSubRing MetricScope = 7
	MetricScope_PerFwRule         MetricScope = 8
	MetricScope_PerPciePort       MetricScope = 9
)

var MetricScope_name = map[int32]string{
	0: "UnknownScope",
	1: "PerNode",
	2: "PerASIC",
	3: "PerFTE",
	4: "PerPort",
	5: "PerLIF",
	6: "PerLIFPerQ",
	7: "PerRingPerSubRing",
	8: "PerFwRule",
	9: "PerPciePort",
}
var MetricScope_value = map[string]int32{
	"UnknownScope":      0,
	"PerNode":           1,
	"PerASIC":           2,
	"PerFTE":            3,
	"PerPort":           4,
	"PerLIF":            5,
	"PerLIFPerQ":        6,
	"PerRingPerSubRing": 7,
	"PerFwRule":         8,
	"PerPciePort":       9,
}

func (x MetricScope) String() string {
	return proto.EnumName(MetricScope_name, int32(x))
}
func (MetricScope) EnumDescriptor() ([]byte, []int) { return fileDescriptorMetricsAnnotations, []int{1} }

type MetricTags int32

const (
	MetricTags_Unknown  MetricTags = 0
	MetricTags_Level1   MetricTags = 1
	MetricTags_Level2   MetricTags = 2
	MetricTags_Level3   MetricTags = 3
	MetricTags_Level4   MetricTags = 4
	MetricTags_Level5   MetricTags = 5
	MetricTags_Level6   MetricTags = 6
	MetricTags_Level7   MetricTags = 7
	MetricTags_FuncRate MetricTags = 8
)

var MetricTags_name = map[int32]string{
	0: "Unknown",
	1: "Level1",
	2: "Level2",
	3: "Level3",
	4: "Level4",
	5: "Level5",
	6: "Level6",
	7: "Level7",
	8: "FuncRate",
}
var MetricTags_value = map[string]int32{
	"Unknown":  0,
	"Level1":   1,
	"Level2":   2,
	"Level3":   3,
	"Level4":   4,
	"Level5":   5,
	"Level6":   6,
	"Level7":   7,
	"FuncRate": 8,
}

func (x MetricTags) String() string {
	return proto.EnumName(MetricTags_name, int32(x))
}
func (MetricTags) EnumDescriptor() ([]byte, []int) { return fileDescriptorMetricsAnnotations, []int{2} }

type MetricInfo struct {
	DisplayName string `protobuf:"bytes,1,opt,name=DisplayName,proto3" json:"DisplayName,omitempty"`
	Description string `protobuf:"bytes,2,opt,name=Description,proto3" json:"Description,omitempty"`
	// defines what fields in the message are used to identify the metric measurement.
	KeyFields string       `protobuf:"bytes,3,opt,name=KeyFields,proto3" json:"KeyFields,omitempty"`
	Scope     MetricScope  `protobuf:"varint,4,opt,name=Scope,proto3,enum=venice.MetricScope" json:"Scope,omitempty"`
	Tags      []MetricTags `protobuf:"varint,5,rep,packed,name=Tags,enum=venice.MetricTags" json:"Tags,omitempty"`
}

func (m *MetricInfo) Reset()                    { *m = MetricInfo{} }
func (m *MetricInfo) String() string            { return proto.CompactTextString(m) }
func (*MetricInfo) ProtoMessage()               {}
func (*MetricInfo) Descriptor() ([]byte, []int) { return fileDescriptorMetricsAnnotations, []int{0} }

func (m *MetricInfo) GetDisplayName() string {
	if m != nil {
		return m.DisplayName
	}
	return ""
}

func (m *MetricInfo) GetDescription() string {
	if m != nil {
		return m.Description
	}
	return ""
}

func (m *MetricInfo) GetKeyFields() string {
	if m != nil {
		return m.KeyFields
	}
	return ""
}

func (m *MetricInfo) GetScope() MetricScope {
	if m != nil {
		return m.Scope
	}
	return MetricScope_UnknownScope
}

func (m *MetricInfo) GetTags() []MetricTags {
	if m != nil {
		return m.Tags
	}
	return nil
}

type MetricFieldInfo struct {
	DisplayName string         `protobuf:"bytes,1,opt,name=DisplayName,proto3" json:"DisplayName,omitempty"`
	Description string         `protobuf:"bytes,2,opt,name=Description,proto3" json:"Description,omitempty"`
	Units       MetricUnitType `protobuf:"varint,3,opt,name=Units,proto3,enum=venice.MetricUnitType" json:"Units,omitempty"`
	ScaleMin    int32          `protobuf:"varint,4,opt,name=ScaleMin,proto3" json:"ScaleMin,omitempty"`
	ScaleMax    int32          `protobuf:"varint,5,opt,name=ScaleMax,proto3" json:"ScaleMax,omitempty"`
	AllowedVal  string         `protobuf:"bytes,6,opt,name=AllowedVal,proto3" json:"AllowedVal,omitempty"`
	Tags        []MetricTags   `protobuf:"varint,7,rep,packed,name=Tags,enum=venice.MetricTags" json:"Tags,omitempty"`
}

func (m *MetricFieldInfo) Reset()         { *m = MetricFieldInfo{} }
func (m *MetricFieldInfo) String() string { return proto.CompactTextString(m) }
func (*MetricFieldInfo) ProtoMessage()    {}
func (*MetricFieldInfo) Descriptor() ([]byte, []int) {
	return fileDescriptorMetricsAnnotations, []int{1}
}

func (m *MetricFieldInfo) GetDisplayName() string {
	if m != nil {
		return m.DisplayName
	}
	return ""
}

func (m *MetricFieldInfo) GetDescription() string {
	if m != nil {
		return m.Description
	}
	return ""
}

func (m *MetricFieldInfo) GetUnits() MetricUnitType {
	if m != nil {
		return m.Units
	}
	return MetricUnitType_Count
}

func (m *MetricFieldInfo) GetScaleMin() int32 {
	if m != nil {
		return m.ScaleMin
	}
	return 0
}

func (m *MetricFieldInfo) GetScaleMax() int32 {
	if m != nil {
		return m.ScaleMax
	}
	return 0
}

func (m *MetricFieldInfo) GetAllowedVal() string {
	if m != nil {
		return m.AllowedVal
	}
	return ""
}

func (m *MetricFieldInfo) GetTags() []MetricTags {
	if m != nil {
		return m.Tags
	}
	return nil
}

var E_MetricInfo = &proto.ExtensionDesc{
	ExtendedType:  (*google_protobuf.MessageOptions)(nil),
	ExtensionType: (*MetricInfo)(nil),
	Field:         90001,
	Name:          "venice.metricInfo",
	Tag:           "bytes,90001,opt,name=metricInfo",
	Filename:      "metrics_annotations.proto",
}

var E_MetricsField = &proto.ExtensionDesc{
	ExtendedType:  (*google_protobuf.FieldOptions)(nil),
	ExtensionType: (*MetricFieldInfo)(nil),
	Field:         91001,
	Name:          "venice.metricsField",
	Tag:           "bytes,91001,opt,name=metricsField",
	Filename:      "metrics_annotations.proto",
}

func init() {
	proto.RegisterType((*MetricInfo)(nil), "venice.MetricInfo")
	proto.RegisterType((*MetricFieldInfo)(nil), "venice.MetricFieldInfo")
	proto.RegisterEnum("venice.MetricUnitType", MetricUnitType_name, MetricUnitType_value)
	proto.RegisterEnum("venice.MetricScope", MetricScope_name, MetricScope_value)
	proto.RegisterEnum("venice.MetricTags", MetricTags_name, MetricTags_value)
	proto.RegisterExtension(E_MetricInfo)
	proto.RegisterExtension(E_MetricsField)
}
func (m *MetricInfo) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *MetricInfo) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.DisplayName) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintMetricsAnnotations(dAtA, i, uint64(len(m.DisplayName)))
		i += copy(dAtA[i:], m.DisplayName)
	}
	if len(m.Description) > 0 {
		dAtA[i] = 0x12
		i++
		i = encodeVarintMetricsAnnotations(dAtA, i, uint64(len(m.Description)))
		i += copy(dAtA[i:], m.Description)
	}
	if len(m.KeyFields) > 0 {
		dAtA[i] = 0x1a
		i++
		i = encodeVarintMetricsAnnotations(dAtA, i, uint64(len(m.KeyFields)))
		i += copy(dAtA[i:], m.KeyFields)
	}
	if m.Scope != 0 {
		dAtA[i] = 0x20
		i++
		i = encodeVarintMetricsAnnotations(dAtA, i, uint64(m.Scope))
	}
	if len(m.Tags) > 0 {
		dAtA2 := make([]byte, len(m.Tags)*10)
		var j1 int
		for _, num := range m.Tags {
			for num >= 1<<7 {
				dAtA2[j1] = uint8(uint64(num)&0x7f | 0x80)
				num >>= 7
				j1++
			}
			dAtA2[j1] = uint8(num)
			j1++
		}
		dAtA[i] = 0x2a
		i++
		i = encodeVarintMetricsAnnotations(dAtA, i, uint64(j1))
		i += copy(dAtA[i:], dAtA2[:j1])
	}
	return i, nil
}

func (m *MetricFieldInfo) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *MetricFieldInfo) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.DisplayName) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintMetricsAnnotations(dAtA, i, uint64(len(m.DisplayName)))
		i += copy(dAtA[i:], m.DisplayName)
	}
	if len(m.Description) > 0 {
		dAtA[i] = 0x12
		i++
		i = encodeVarintMetricsAnnotations(dAtA, i, uint64(len(m.Description)))
		i += copy(dAtA[i:], m.Description)
	}
	if m.Units != 0 {
		dAtA[i] = 0x18
		i++
		i = encodeVarintMetricsAnnotations(dAtA, i, uint64(m.Units))
	}
	if m.ScaleMin != 0 {
		dAtA[i] = 0x20
		i++
		i = encodeVarintMetricsAnnotations(dAtA, i, uint64(m.ScaleMin))
	}
	if m.ScaleMax != 0 {
		dAtA[i] = 0x28
		i++
		i = encodeVarintMetricsAnnotations(dAtA, i, uint64(m.ScaleMax))
	}
	if len(m.AllowedVal) > 0 {
		dAtA[i] = 0x32
		i++
		i = encodeVarintMetricsAnnotations(dAtA, i, uint64(len(m.AllowedVal)))
		i += copy(dAtA[i:], m.AllowedVal)
	}
	if len(m.Tags) > 0 {
		dAtA4 := make([]byte, len(m.Tags)*10)
		var j3 int
		for _, num := range m.Tags {
			for num >= 1<<7 {
				dAtA4[j3] = uint8(uint64(num)&0x7f | 0x80)
				num >>= 7
				j3++
			}
			dAtA4[j3] = uint8(num)
			j3++
		}
		dAtA[i] = 0x3a
		i++
		i = encodeVarintMetricsAnnotations(dAtA, i, uint64(j3))
		i += copy(dAtA[i:], dAtA4[:j3])
	}
	return i, nil
}

func encodeVarintMetricsAnnotations(dAtA []byte, offset int, v uint64) int {
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return offset + 1
}
func (m *MetricInfo) Size() (n int) {
	var l int
	_ = l
	l = len(m.DisplayName)
	if l > 0 {
		n += 1 + l + sovMetricsAnnotations(uint64(l))
	}
	l = len(m.Description)
	if l > 0 {
		n += 1 + l + sovMetricsAnnotations(uint64(l))
	}
	l = len(m.KeyFields)
	if l > 0 {
		n += 1 + l + sovMetricsAnnotations(uint64(l))
	}
	if m.Scope != 0 {
		n += 1 + sovMetricsAnnotations(uint64(m.Scope))
	}
	if len(m.Tags) > 0 {
		l = 0
		for _, e := range m.Tags {
			l += sovMetricsAnnotations(uint64(e))
		}
		n += 1 + sovMetricsAnnotations(uint64(l)) + l
	}
	return n
}

func (m *MetricFieldInfo) Size() (n int) {
	var l int
	_ = l
	l = len(m.DisplayName)
	if l > 0 {
		n += 1 + l + sovMetricsAnnotations(uint64(l))
	}
	l = len(m.Description)
	if l > 0 {
		n += 1 + l + sovMetricsAnnotations(uint64(l))
	}
	if m.Units != 0 {
		n += 1 + sovMetricsAnnotations(uint64(m.Units))
	}
	if m.ScaleMin != 0 {
		n += 1 + sovMetricsAnnotations(uint64(m.ScaleMin))
	}
	if m.ScaleMax != 0 {
		n += 1 + sovMetricsAnnotations(uint64(m.ScaleMax))
	}
	l = len(m.AllowedVal)
	if l > 0 {
		n += 1 + l + sovMetricsAnnotations(uint64(l))
	}
	if len(m.Tags) > 0 {
		l = 0
		for _, e := range m.Tags {
			l += sovMetricsAnnotations(uint64(e))
		}
		n += 1 + sovMetricsAnnotations(uint64(l)) + l
	}
	return n
}

func sovMetricsAnnotations(x uint64) (n int) {
	for {
		n++
		x >>= 7
		if x == 0 {
			break
		}
	}
	return n
}
func sozMetricsAnnotations(x uint64) (n int) {
	return sovMetricsAnnotations(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *MetricInfo) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowMetricsAnnotations
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
			return fmt.Errorf("proto: MetricInfo: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: MetricInfo: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field DisplayName", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMetricsAnnotations
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
				return ErrInvalidLengthMetricsAnnotations
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.DisplayName = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Description", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMetricsAnnotations
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
				return ErrInvalidLengthMetricsAnnotations
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Description = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 3:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field KeyFields", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMetricsAnnotations
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
				return ErrInvalidLengthMetricsAnnotations
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.KeyFields = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 4:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field Scope", wireType)
			}
			m.Scope = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMetricsAnnotations
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.Scope |= (MetricScope(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 5:
			if wireType == 0 {
				var v MetricTags
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return ErrIntOverflowMetricsAnnotations
					}
					if iNdEx >= l {
						return io.ErrUnexpectedEOF
					}
					b := dAtA[iNdEx]
					iNdEx++
					v |= (MetricTags(b) & 0x7F) << shift
					if b < 0x80 {
						break
					}
				}
				m.Tags = append(m.Tags, v)
			} else if wireType == 2 {
				var packedLen int
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return ErrIntOverflowMetricsAnnotations
					}
					if iNdEx >= l {
						return io.ErrUnexpectedEOF
					}
					b := dAtA[iNdEx]
					iNdEx++
					packedLen |= (int(b) & 0x7F) << shift
					if b < 0x80 {
						break
					}
				}
				if packedLen < 0 {
					return ErrInvalidLengthMetricsAnnotations
				}
				postIndex := iNdEx + packedLen
				if postIndex > l {
					return io.ErrUnexpectedEOF
				}
				for iNdEx < postIndex {
					var v MetricTags
					for shift := uint(0); ; shift += 7 {
						if shift >= 64 {
							return ErrIntOverflowMetricsAnnotations
						}
						if iNdEx >= l {
							return io.ErrUnexpectedEOF
						}
						b := dAtA[iNdEx]
						iNdEx++
						v |= (MetricTags(b) & 0x7F) << shift
						if b < 0x80 {
							break
						}
					}
					m.Tags = append(m.Tags, v)
				}
			} else {
				return fmt.Errorf("proto: wrong wireType = %d for field Tags", wireType)
			}
		default:
			iNdEx = preIndex
			skippy, err := skipMetricsAnnotations(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthMetricsAnnotations
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
func (m *MetricFieldInfo) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowMetricsAnnotations
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
			return fmt.Errorf("proto: MetricFieldInfo: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: MetricFieldInfo: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field DisplayName", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMetricsAnnotations
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
				return ErrInvalidLengthMetricsAnnotations
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.DisplayName = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Description", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMetricsAnnotations
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
				return ErrInvalidLengthMetricsAnnotations
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Description = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 3:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field Units", wireType)
			}
			m.Units = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMetricsAnnotations
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.Units |= (MetricUnitType(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 4:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field ScaleMin", wireType)
			}
			m.ScaleMin = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMetricsAnnotations
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.ScaleMin |= (int32(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 5:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field ScaleMax", wireType)
			}
			m.ScaleMax = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMetricsAnnotations
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.ScaleMax |= (int32(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 6:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field AllowedVal", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowMetricsAnnotations
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
				return ErrInvalidLengthMetricsAnnotations
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.AllowedVal = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 7:
			if wireType == 0 {
				var v MetricTags
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return ErrIntOverflowMetricsAnnotations
					}
					if iNdEx >= l {
						return io.ErrUnexpectedEOF
					}
					b := dAtA[iNdEx]
					iNdEx++
					v |= (MetricTags(b) & 0x7F) << shift
					if b < 0x80 {
						break
					}
				}
				m.Tags = append(m.Tags, v)
			} else if wireType == 2 {
				var packedLen int
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return ErrIntOverflowMetricsAnnotations
					}
					if iNdEx >= l {
						return io.ErrUnexpectedEOF
					}
					b := dAtA[iNdEx]
					iNdEx++
					packedLen |= (int(b) & 0x7F) << shift
					if b < 0x80 {
						break
					}
				}
				if packedLen < 0 {
					return ErrInvalidLengthMetricsAnnotations
				}
				postIndex := iNdEx + packedLen
				if postIndex > l {
					return io.ErrUnexpectedEOF
				}
				for iNdEx < postIndex {
					var v MetricTags
					for shift := uint(0); ; shift += 7 {
						if shift >= 64 {
							return ErrIntOverflowMetricsAnnotations
						}
						if iNdEx >= l {
							return io.ErrUnexpectedEOF
						}
						b := dAtA[iNdEx]
						iNdEx++
						v |= (MetricTags(b) & 0x7F) << shift
						if b < 0x80 {
							break
						}
					}
					m.Tags = append(m.Tags, v)
				}
			} else {
				return fmt.Errorf("proto: wrong wireType = %d for field Tags", wireType)
			}
		default:
			iNdEx = preIndex
			skippy, err := skipMetricsAnnotations(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthMetricsAnnotations
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
func skipMetricsAnnotations(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowMetricsAnnotations
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
					return 0, ErrIntOverflowMetricsAnnotations
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
					return 0, ErrIntOverflowMetricsAnnotations
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
				return 0, ErrInvalidLengthMetricsAnnotations
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowMetricsAnnotations
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
				next, err := skipMetricsAnnotations(dAtA[start:])
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
	ErrInvalidLengthMetricsAnnotations = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowMetricsAnnotations   = fmt.Errorf("proto: integer overflow")
)

func init() { proto.RegisterFile("metrics_annotations.proto", fileDescriptorMetricsAnnotations) }

var fileDescriptorMetricsAnnotations = []byte{
	// 634 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0xac, 0x54, 0xcb, 0x6e, 0xd3, 0x40,
	0x14, 0xad, 0x93, 0xda, 0x49, 0x6e, 0x42, 0x3a, 0x0c, 0x02, 0x4c, 0x05, 0x21, 0xea, 0x02, 0x95,
	0x0a, 0xa5, 0x22, 0xe5, 0x21, 0x75, 0xd7, 0xb4, 0x04, 0xa2, 0x36, 0xc5, 0x38, 0x2d, 0x6c, 0x90,
	0xd0, 0xd4, 0xb9, 0x8d, 0x2c, 0x9c, 0x99, 0xc8, 0x33, 0x6e, 0x1b, 0x58, 0xb0, 0xe0, 0x0b, 0xf8,
	0x08, 0xbe, 0x03, 0xb1, 0x41, 0x2c, 0xf9, 0x04, 0xd4, 0xbf, 0x60, 0x87, 0x66, 0x9c, 0x3a, 0x09,
	0x48, 0xac, 0xd8, 0xdd, 0x39, 0xe7, 0xf8, 0xcc, 0xb9, 0x33, 0x73, 0x0d, 0x37, 0x86, 0xa8, 0xe2,
	0x30, 0x90, 0x6f, 0x18, 0xe7, 0x42, 0x31, 0x15, 0x0a, 0x2e, 0x1b, 0xa3, 0x58, 0x28, 0x41, 0x9d,
	0x13, 0xe4, 0x61, 0x80, 0xcb, 0xf5, 0x81, 0x10, 0x83, 0x08, 0xd7, 0x0d, 0x7a, 0x94, 0x1c, 0xaf,
	0xf7, 0x51, 0x06, 0x71, 0x38, 0x52, 0x22, 0x4e, 0x95, 0x2b, 0x5f, 0x2d, 0x80, 0xae, 0xf1, 0xe9,
	0xf0, 0x63, 0x41, 0xeb, 0x50, 0xde, 0x09, 0xe5, 0x28, 0x62, 0xe3, 0x7d, 0x36, 0x44, 0xd7, 0xaa,
	0x5b, 0xab, 0x25, 0x7f, 0x16, 0x32, 0x8a, 0x89, 0x49, 0x28, 0xb8, 0x9b, 0x9b, 0x28, 0xa6, 0x10,
	0xbd, 0x09, 0xa5, 0x5d, 0x1c, 0xb7, 0x43, 0x8c, 0xfa, 0xd2, 0xcd, 0x1b, 0x7e, 0x0a, 0xd0, 0xbb,
	0x60, 0xf7, 0x02, 0x31, 0x42, 0x77, 0xb1, 0x6e, 0xad, 0x56, 0x9b, 0x57, 0x1a, 0x69, 0xd4, 0x46,
	0x1a, 0xc2, 0x50, 0x7e, 0xaa, 0xa0, 0x77, 0x60, 0xf1, 0x80, 0x0d, 0xa4, 0x6b, 0xd7, 0xf3, 0xab,
	0xd5, 0x26, 0x9d, 0x57, 0x6a, 0xc6, 0x37, 0xfc, 0xca, 0xc7, 0x1c, 0x2c, 0xa5, 0xa0, 0xd9, 0xe3,
	0xbf, 0x35, 0x72, 0x0f, 0xec, 0x43, 0x1e, 0xaa, 0xb4, 0x89, 0x6a, 0xf3, 0xda, 0x7c, 0x00, 0x4d,
	0x1d, 0x8c, 0x75, 0x5a, 0x23, 0xa2, 0xcb, 0x50, 0xec, 0x05, 0x2c, 0xc2, 0x6e, 0xc8, 0x4d, 0x6f,
	0xb6, 0x9f, 0xad, 0xa7, 0x1c, 0x3b, 0x73, 0xed, 0x59, 0x8e, 0x9d, 0xd1, 0x1a, 0xc0, 0x56, 0x14,
	0x89, 0x53, 0xec, 0xbf, 0x64, 0x91, 0xeb, 0x98, 0x18, 0x33, 0x48, 0x76, 0x0a, 0x85, 0x7f, 0x9f,
	0xc2, 0xda, 0x07, 0xa8, 0xce, 0x07, 0xa3, 0x25, 0xb0, 0xb7, 0x45, 0xc2, 0x15, 0x59, 0xa0, 0x55,
	0x00, 0x0f, 0xe3, 0x00, 0xb9, 0x62, 0x03, 0x24, 0x96, 0xa6, 0x5a, 0x63, 0x85, 0x92, 0xe4, 0xa8,
	0x03, 0xb9, 0xce, 0x0e, 0xc9, 0x6b, 0xe8, 0x29, 0x4b, 0x06, 0x48, 0x16, 0x29, 0x80, 0xd3, 0x0a,
	0xd5, 0x90, 0x8d, 0x88, 0x4d, 0xcb, 0x50, 0xd8, 0xc6, 0x48, 0x86, 0x89, 0x24, 0x8e, 0xd6, 0xee,
	0xb6, 0x48, 0x41, 0x0b, 0xba, 0xaf, 0x98, 0x52, 0x92, 0x14, 0x69, 0x01, 0xf2, 0xdd, 0x67, 0xef,
	0x48, 0x69, 0xed, 0xb3, 0x05, 0xe5, 0x99, 0x5b, 0xa4, 0x04, 0x2a, 0x87, 0xfc, 0x2d, 0x17, 0xa7,
	0xdc, 0xac, 0xc9, 0x82, 0xf6, 0xf2, 0x30, 0xde, 0x17, 0x7d, 0x1d, 0x21, 0x5d, 0x6c, 0xf5, 0x3a,
	0xdb, 0x24, 0xa7, 0x0d, 0x3d, 0x8c, 0xdb, 0x07, 0x4f, 0x48, 0x7e, 0x42, 0x78, 0x22, 0x56, 0x69,
	0x14, 0x0f, 0xe3, 0xbd, 0x4e, 0x9b, 0xd8, 0x93, 0x26, 0xf6, 0x3a, 0x6d, 0x0f, 0xe3, 0x17, 0xc4,
	0xa1, 0x57, 0xe1, 0xb2, 0x87, 0xb1, 0x1f, 0xf2, 0x81, 0x87, 0x71, 0x2f, 0x39, 0xd2, 0x15, 0x29,
	0xd0, 0x4b, 0x50, 0xd2, 0x5e, 0xa7, 0x7e, 0x12, 0x21, 0x29, 0xd2, 0x25, 0x28, 0x6b, 0xbb, 0x20,
	0x44, 0x63, 0x59, 0x5a, 0x7b, 0x7f, 0xf1, 0xe2, 0xf5, 0xb1, 0xe9, 0xdd, 0x26, 0x29, 0xc9, 0x82,
	0xde, 0x6d, 0x0f, 0x4f, 0x30, 0xba, 0x4f, 0xac, 0xac, 0x6e, 0xa6, 0xf1, 0x4c, 0xbd, 0x41, 0xf2,
	0x59, 0xfd, 0x20, 0x4d, 0x67, 0xea, 0x87, 0xc4, 0xce, 0xea, 0x47, 0xc4, 0xc9, 0xea, 0xc7, 0xa4,
	0x40, 0x2b, 0x50, 0x6c, 0x27, 0x3c, 0xf0, 0x99, 0x42, 0x52, 0xdc, 0x3c, 0x04, 0x18, 0x4e, 0xc7,
	0xed, 0x76, 0x23, 0x1d, 0xd0, 0xc6, 0xc5, 0x80, 0x36, 0xba, 0x28, 0x25, 0x1b, 0xe0, 0x73, 0xf3,
	0x02, 0xa5, 0xfb, 0xe9, 0x8b, 0x7e, 0x2e, 0xe5, 0x3f, 0xaf, 0x5d, 0x7f, 0xec, 0xcf, 0x18, 0x6d,
	0xbe, 0x86, 0xca, 0xe4, 0x6f, 0x60, 0x46, 0x80, 0xde, 0xfa, 0xcb, 0xd8, 0xe0, 0x17, 0xb6, 0xbf,
	0xbe, 0xa5, 0xb6, 0xd7, 0xe7, 0x6d, 0xb3, 0xf1, 0xf1, 0xe7, 0xdc, 0x5a, 0x95, 0xef, 0xe7, 0x35,
	0xeb, 0xc7, 0x79, 0xcd, 0xfa, 0x79, 0x5e, 0xb3, 0x8e, 0x1c, 0xe3, 0xb9, 0xf1, 0x3b, 0x00, 0x00,
	0xff, 0xff, 0x60, 0x36, 0x27, 0xf2, 0x80, 0x04, 0x00, 0x00,
}
