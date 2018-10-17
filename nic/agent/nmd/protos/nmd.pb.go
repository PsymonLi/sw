// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: nmd.proto

/*
	Package nmd is a generated protocol buffer package.

	Service name

	It is generated from these files:
		nmd.proto

	It has these top-level messages:
		Naples
		NaplesSpec
		NaplesStatus
		NaplesCmdExecute
*/
package nmd

import proto "github.com/gogo/protobuf/proto"
import fmt "fmt"
import math "math"
import _ "google.golang.org/genproto/googleapis/api/annotations"
import _ "github.com/pensando/sw/venice/utils/apigen/annotations"
import _ "github.com/gogo/protobuf/gogoproto"
import api "github.com/pensando/sw/api"
import cluster "github.com/pensando/sw/api/generated/cluster"

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

// Operational mode of a Naples
type NaplesMode int32

const (
	NaplesMode_CLASSIC_MODE NaplesMode = 0
	NaplesMode_MANAGED_MODE NaplesMode = 1
)

var NaplesMode_name = map[int32]string{
	0: "CLASSIC_MODE",
	1: "MANAGED_MODE",
}
var NaplesMode_value = map[string]int32{
	"CLASSIC_MODE": 0,
	"MANAGED_MODE": 1,
}

func (x NaplesMode) String() string {
	return proto.EnumName(NaplesMode_name, int32(x))
}
func (NaplesMode) EnumDescriptor() ([]byte, []int) { return fileDescriptorNmd, []int{0} }

// Naples config object
type Naples struct {
	api.TypeMeta   `protobuf:"bytes,1,opt,name=T,embedded=T" json:",inline"`
	api.ObjectMeta `protobuf:"bytes,2,opt,name=O,embedded=O" json:"meta,omitempty"`
	// Spec contains the configuration of the NIC.
	Spec NaplesSpec `protobuf:"bytes,3,opt,name=Spec" json:"spec,omitempty"`
	// Status contains the current state of the NIC.
	Status NaplesStatus `protobuf:"bytes,4,opt,name=Status" json:"status,omitempty"`
}

func (m *Naples) Reset()                    { *m = Naples{} }
func (m *Naples) String() string            { return proto.CompactTextString(m) }
func (*Naples) ProtoMessage()               {}
func (*Naples) Descriptor() ([]byte, []int) { return fileDescriptorNmd, []int{0} }

func (m *Naples) GetSpec() NaplesSpec {
	if m != nil {
		return m.Spec
	}
	return NaplesSpec{}
}

func (m *Naples) GetStatus() NaplesStatus {
	if m != nil {
		return m.Status
	}
	return NaplesStatus{}
}

// NaplesSpec contains initial bootstrap configuration of the Naples I/O subsystem
type NaplesSpec struct {
	// Operational mode of the NIC
	Mode NaplesMode `protobuf:"varint,1,opt,name=Mode,proto3,enum=nmd.NaplesMode" json:"mode"`
	// List of IP/hostname:Port address of Venice quorum nodes in comma separated format
	// For eg: IP1:port1,IP2:port2,IP3:port3
	ClusterAddress []string `protobuf:"bytes,2,rep,name=ClusterAddress" json:"clusterAddress,omitempty"`
	// PrimaryMac of the NIC adapter
	PrimaryMac string `protobuf:"bytes,3,opt,name=PrimaryMac,proto3" json:"primaryMac,omitempty"`
	// Name of the Host
	HostName string `protobuf:"bytes,4,opt,name=HostName,proto3" json:"hostName,omitempty"`
	// Management IP address of the naples node
	MgmtIp string `protobuf:"bytes,5,opt,name=MgmtIp,proto3" json:"mgmtIp,omitempty"`
}

func (m *NaplesSpec) Reset()                    { *m = NaplesSpec{} }
func (m *NaplesSpec) String() string            { return proto.CompactTextString(m) }
func (*NaplesSpec) ProtoMessage()               {}
func (*NaplesSpec) Descriptor() ([]byte, []int) { return fileDescriptorNmd, []int{1} }

func (m *NaplesSpec) GetMode() NaplesMode {
	if m != nil {
		return m.Mode
	}
	return NaplesMode_CLASSIC_MODE
}

func (m *NaplesSpec) GetClusterAddress() []string {
	if m != nil {
		return m.ClusterAddress
	}
	return nil
}

func (m *NaplesSpec) GetPrimaryMac() string {
	if m != nil {
		return m.PrimaryMac
	}
	return ""
}

func (m *NaplesSpec) GetHostName() string {
	if m != nil {
		return m.HostName
	}
	return ""
}

func (m *NaplesSpec) GetMgmtIp() string {
	if m != nil {
		return m.MgmtIp
	}
	return ""
}

// NaplesStatus contains current status of a Naples I/O subsystem
type NaplesStatus struct {
	// Current phase of the NIC adapter in the system
	Phase cluster.SmartNICSpec_SmartNICPhase `protobuf:"varint,1,opt,name=Phase,proto3,enum=cluster.SmartNICSpec_SmartNICPhase" json:"phase,omitempty"`
}

func (m *NaplesStatus) Reset()                    { *m = NaplesStatus{} }
func (m *NaplesStatus) String() string            { return proto.CompactTextString(m) }
func (*NaplesStatus) ProtoMessage()               {}
func (*NaplesStatus) Descriptor() ([]byte, []int) { return fileDescriptorNmd, []int{2} }

func (m *NaplesStatus) GetPhase() cluster.SmartNICSpec_SmartNICPhase {
	if m != nil {
		return m.Phase
	}
	return cluster.SmartNICSpec_UNKNOWN
}

// NaplesCmdExecute contains information on what command to execute
type NaplesCmdExecute struct {
	// Executable to run on naples side
	Executable string `protobuf:"bytes,1,opt,name=Executable,proto3" json:"executable,omitempty"`
	// Flags for the executable to run on naples side
	Opts string `protobuf:"bytes,2,opt,name=Opts,proto3" json:"opts,omitempty"`
}

func (m *NaplesCmdExecute) Reset()                    { *m = NaplesCmdExecute{} }
func (m *NaplesCmdExecute) String() string            { return proto.CompactTextString(m) }
func (*NaplesCmdExecute) ProtoMessage()               {}
func (*NaplesCmdExecute) Descriptor() ([]byte, []int) { return fileDescriptorNmd, []int{3} }

func (m *NaplesCmdExecute) GetExecutable() string {
	if m != nil {
		return m.Executable
	}
	return ""
}

func (m *NaplesCmdExecute) GetOpts() string {
	if m != nil {
		return m.Opts
	}
	return ""
}

func init() {
	proto.RegisterType((*Naples)(nil), "nmd.Naples")
	proto.RegisterType((*NaplesSpec)(nil), "nmd.NaplesSpec")
	proto.RegisterType((*NaplesStatus)(nil), "nmd.NaplesStatus")
	proto.RegisterType((*NaplesCmdExecute)(nil), "nmd.NaplesCmdExecute")
	proto.RegisterEnum("nmd.NaplesMode", NaplesMode_name, NaplesMode_value)
}
func (m *Naples) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *Naples) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	dAtA[i] = 0xa
	i++
	i = encodeVarintNmd(dAtA, i, uint64(m.TypeMeta.Size()))
	n1, err := m.TypeMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n1
	dAtA[i] = 0x12
	i++
	i = encodeVarintNmd(dAtA, i, uint64(m.ObjectMeta.Size()))
	n2, err := m.ObjectMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n2
	dAtA[i] = 0x1a
	i++
	i = encodeVarintNmd(dAtA, i, uint64(m.Spec.Size()))
	n3, err := m.Spec.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n3
	dAtA[i] = 0x22
	i++
	i = encodeVarintNmd(dAtA, i, uint64(m.Status.Size()))
	n4, err := m.Status.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n4
	return i, nil
}

func (m *NaplesSpec) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *NaplesSpec) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if m.Mode != 0 {
		dAtA[i] = 0x8
		i++
		i = encodeVarintNmd(dAtA, i, uint64(m.Mode))
	}
	if len(m.ClusterAddress) > 0 {
		for _, s := range m.ClusterAddress {
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
	if len(m.PrimaryMac) > 0 {
		dAtA[i] = 0x1a
		i++
		i = encodeVarintNmd(dAtA, i, uint64(len(m.PrimaryMac)))
		i += copy(dAtA[i:], m.PrimaryMac)
	}
	if len(m.HostName) > 0 {
		dAtA[i] = 0x22
		i++
		i = encodeVarintNmd(dAtA, i, uint64(len(m.HostName)))
		i += copy(dAtA[i:], m.HostName)
	}
	if len(m.MgmtIp) > 0 {
		dAtA[i] = 0x2a
		i++
		i = encodeVarintNmd(dAtA, i, uint64(len(m.MgmtIp)))
		i += copy(dAtA[i:], m.MgmtIp)
	}
	return i, nil
}

func (m *NaplesStatus) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *NaplesStatus) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if m.Phase != 0 {
		dAtA[i] = 0x8
		i++
		i = encodeVarintNmd(dAtA, i, uint64(m.Phase))
	}
	return i, nil
}

func (m *NaplesCmdExecute) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *NaplesCmdExecute) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Executable) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintNmd(dAtA, i, uint64(len(m.Executable)))
		i += copy(dAtA[i:], m.Executable)
	}
	if len(m.Opts) > 0 {
		dAtA[i] = 0x12
		i++
		i = encodeVarintNmd(dAtA, i, uint64(len(m.Opts)))
		i += copy(dAtA[i:], m.Opts)
	}
	return i, nil
}

func encodeVarintNmd(dAtA []byte, offset int, v uint64) int {
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return offset + 1
}
func (m *Naples) Size() (n int) {
	var l int
	_ = l
	l = m.TypeMeta.Size()
	n += 1 + l + sovNmd(uint64(l))
	l = m.ObjectMeta.Size()
	n += 1 + l + sovNmd(uint64(l))
	l = m.Spec.Size()
	n += 1 + l + sovNmd(uint64(l))
	l = m.Status.Size()
	n += 1 + l + sovNmd(uint64(l))
	return n
}

func (m *NaplesSpec) Size() (n int) {
	var l int
	_ = l
	if m.Mode != 0 {
		n += 1 + sovNmd(uint64(m.Mode))
	}
	if len(m.ClusterAddress) > 0 {
		for _, s := range m.ClusterAddress {
			l = len(s)
			n += 1 + l + sovNmd(uint64(l))
		}
	}
	l = len(m.PrimaryMac)
	if l > 0 {
		n += 1 + l + sovNmd(uint64(l))
	}
	l = len(m.HostName)
	if l > 0 {
		n += 1 + l + sovNmd(uint64(l))
	}
	l = len(m.MgmtIp)
	if l > 0 {
		n += 1 + l + sovNmd(uint64(l))
	}
	return n
}

func (m *NaplesStatus) Size() (n int) {
	var l int
	_ = l
	if m.Phase != 0 {
		n += 1 + sovNmd(uint64(m.Phase))
	}
	return n
}

func (m *NaplesCmdExecute) Size() (n int) {
	var l int
	_ = l
	l = len(m.Executable)
	if l > 0 {
		n += 1 + l + sovNmd(uint64(l))
	}
	l = len(m.Opts)
	if l > 0 {
		n += 1 + l + sovNmd(uint64(l))
	}
	return n
}

func sovNmd(x uint64) (n int) {
	for {
		n++
		x >>= 7
		if x == 0 {
			break
		}
	}
	return n
}
func sozNmd(x uint64) (n int) {
	return sovNmd(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *Naples) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowNmd
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
			return fmt.Errorf("proto: Naples: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: Naples: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field TypeMeta", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNmd
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
				return ErrInvalidLengthNmd
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
					return ErrIntOverflowNmd
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
				return ErrInvalidLengthNmd
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
					return ErrIntOverflowNmd
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
				return ErrInvalidLengthNmd
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
					return ErrIntOverflowNmd
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
				return ErrInvalidLengthNmd
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
			skippy, err := skipNmd(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthNmd
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
func (m *NaplesSpec) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowNmd
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
			return fmt.Errorf("proto: NaplesSpec: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: NaplesSpec: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field Mode", wireType)
			}
			m.Mode = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNmd
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.Mode |= (NaplesMode(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field ClusterAddress", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNmd
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
				return ErrInvalidLengthNmd
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.ClusterAddress = append(m.ClusterAddress, string(dAtA[iNdEx:postIndex]))
			iNdEx = postIndex
		case 3:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field PrimaryMac", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNmd
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
				return ErrInvalidLengthNmd
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.PrimaryMac = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 4:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field HostName", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNmd
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
				return ErrInvalidLengthNmd
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.HostName = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 5:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field MgmtIp", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNmd
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
				return ErrInvalidLengthNmd
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.MgmtIp = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipNmd(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthNmd
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
func (m *NaplesStatus) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowNmd
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
			return fmt.Errorf("proto: NaplesStatus: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: NaplesStatus: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field Phase", wireType)
			}
			m.Phase = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNmd
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.Phase |= (cluster.SmartNICSpec_SmartNICPhase(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		default:
			iNdEx = preIndex
			skippy, err := skipNmd(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthNmd
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
func (m *NaplesCmdExecute) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowNmd
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
			return fmt.Errorf("proto: NaplesCmdExecute: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: NaplesCmdExecute: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Executable", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNmd
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
				return ErrInvalidLengthNmd
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Executable = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Opts", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNmd
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
				return ErrInvalidLengthNmd
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Opts = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipNmd(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthNmd
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
func skipNmd(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowNmd
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
					return 0, ErrIntOverflowNmd
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
					return 0, ErrIntOverflowNmd
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
				return 0, ErrInvalidLengthNmd
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowNmd
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
				next, err := skipNmd(dAtA[start:])
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
	ErrInvalidLengthNmd = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowNmd   = fmt.Errorf("proto: integer overflow")
)

func init() { proto.RegisterFile("nmd.proto", fileDescriptorNmd) }

var fileDescriptorNmd = []byte{
	// 609 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x7c, 0x93, 0xdf, 0x4e, 0x13, 0x4f,
	0x14, 0xc7, 0x99, 0x52, 0xfa, 0xa3, 0xf3, 0xab, 0x50, 0x47, 0xa3, 0x2b, 0x21, 0x40, 0x6a, 0x4c,
	0x08, 0x62, 0x17, 0x35, 0x31, 0xf1, 0xcf, 0x4d, 0xb7, 0x34, 0x42, 0x62, 0x5b, 0xd2, 0x72, 0x6f,
	0xa6, 0xbb, 0xc3, 0x32, 0x66, 0xe7, 0x4f, 0x3a, 0xb3, 0x2a, 0x89, 0xb7, 0x3c, 0x90, 0x6f, 0xc1,
	0x25, 0xf1, 0x01, 0x88, 0xe1, 0xd2, 0xa7, 0x30, 0x73, 0x76, 0x5b, 0x06, 0x13, 0xbd, 0xdb, 0x73,
	0xe6, 0xfb, 0x39, 0xe7, 0xcc, 0x77, 0xce, 0xe2, 0xba, 0x14, 0x49, 0x5b, 0x4f, 0x95, 0x55, 0x64,
	0x51, 0x8a, 0x64, 0x6d, 0x3d, 0x55, 0x2a, 0xcd, 0x58, 0x48, 0x35, 0x0f, 0xa9, 0x94, 0xca, 0x52,
	0xcb, 0x95, 0x34, 0x85, 0x64, 0xad, 0x97, 0x72, 0x7b, 0x9a, 0x4f, 0xda, 0xb1, 0x12, 0xa1, 0x66,
	0xd2, 0x50, 0x99, 0xa8, 0xd0, 0x7c, 0x09, 0x3f, 0x33, 0xc9, 0x63, 0x16, 0xe6, 0x96, 0x67, 0xc6,
	0xa1, 0x29, 0x93, 0x3e, 0x1d, 0x72, 0x19, 0x67, 0x79, 0xc2, 0x66, 0x65, 0x9e, 0x79, 0x65, 0x52,
	0x95, 0xaa, 0x10, 0xd2, 0x93, 0xfc, 0x04, 0x22, 0x08, 0xe0, 0xab, 0x94, 0x3f, 0xf9, 0x4b, 0x57,
	0x37, 0xa3, 0x60, 0x96, 0x96, 0xb2, 0xa7, 0xff, 0x90, 0x81, 0xc2, 0x84, 0xf1, 0xec, 0xb2, 0xad,
	0xf3, 0x0a, 0xae, 0x0d, 0xa8, 0xce, 0x98, 0x21, 0x7b, 0x18, 0x1d, 0x07, 0x68, 0x0b, 0x6d, 0xff,
	0xff, 0xe2, 0x4e, 0x9b, 0x6a, 0xde, 0x3e, 0x3e, 0xd3, 0xac, 0xcf, 0x2c, 0x8d, 0xee, 0x5d, 0x5c,
	0x6d, 0x2e, 0x5c, 0x5e, 0x6d, 0xa2, 0x5f, 0x57, 0x9b, 0xff, 0xed, 0x72, 0x99, 0x71, 0xc9, 0x46,
	0xe8, 0x98, 0xbc, 0xc6, 0x68, 0x18, 0x54, 0x80, 0x58, 0x05, 0x62, 0x38, 0xf9, 0xc4, 0x62, 0x0b,
	0xcc, 0x9a, 0xc7, 0xac, 0xb8, 0xe9, 0x76, 0x95, 0xe0, 0x96, 0x09, 0x6d, 0xcf, 0x46, 0x68, 0x48,
	0xde, 0xe2, 0xea, 0x58, 0xb3, 0x38, 0x58, 0x2c, 0x69, 0x67, 0x7f, 0x31, 0x87, 0x4b, 0x47, 0x0f,
	0x1c, 0xed, 0x48, 0xa3, 0x59, 0xec, 0x91, 0x00, 0x91, 0x2e, 0xae, 0x8d, 0x2d, 0xb5, 0xb9, 0x09,
	0xaa, 0x80, 0xdf, 0xf5, 0x71, 0x38, 0x88, 0x82, 0xb2, 0x40, 0xd3, 0x40, 0xec, 0x95, 0x28, 0xd1,
	0x37, 0x8d, 0x1f, 0xe7, 0x8f, 0x96, 0x49, 0x2d, 0x56, 0xf2, 0x84, 0xa7, 0xad, 0xef, 0x15, 0x8c,
	0x6f, 0xfa, 0x93, 0xe7, 0xb8, 0xda, 0x57, 0x09, 0x03, 0x3b, 0x56, 0x6e, 0x8d, 0xe7, 0xd2, 0x51,
	0xe3, 0xa2, 0xb8, 0x58, 0x55, 0xa8, 0x84, 0x8d, 0x40, 0x4a, 0x0e, 0xf0, 0x4a, 0x37, 0xcb, 0x8d,
	0x65, 0xd3, 0x4e, 0x92, 0x4c, 0x99, 0x31, 0x41, 0x65, 0x6b, 0x71, 0xbb, 0x1e, 0x6d, 0x95, 0xda,
	0x20, 0xbe, 0x75, 0xea, 0x4d, 0xf4, 0x07, 0x47, 0xde, 0x61, 0x7c, 0x34, 0xe5, 0x82, 0x4e, 0xcf,
	0xfa, 0xb4, 0x70, 0xa8, 0x1e, 0xad, 0x97, 0x55, 0xee, 0xeb, 0xf9, 0x89, 0x57, 0xc1, 0xd3, 0x93,
	0x57, 0x78, 0xf9, 0x40, 0x19, 0x3b, 0xa0, 0x82, 0x81, 0x3d, 0x75, 0x78, 0x0a, 0xc7, 0x92, 0xd3,
	0x32, 0xef, 0x91, 0x73, 0x2d, 0xd9, 0xc3, 0xb5, 0x7e, 0x2a, 0xec, 0xa1, 0x0e, 0x96, 0x80, 0x0a,
	0x4a, 0xaa, 0x29, 0x20, 0xeb, 0x3b, 0x58, 0xe8, 0x5a, 0x13, 0xdc, 0xf0, 0x3d, 0x27, 0x23, 0xbc,
	0x74, 0x74, 0x4a, 0xcd, 0xcc, 0xb5, 0xc7, 0xed, 0xf2, 0xc6, 0xed, 0xb1, 0xa0, 0x53, 0x3b, 0x38,
	0xec, 0x3a, 0x6b, 0xe7, 0x01, 0x48, 0xa3, 0x87, 0x65, 0x97, 0x55, 0xed, 0x42, 0xaf, 0x49, 0x51,
	0xaa, 0xf5, 0x0d, 0x37, 0x8b, 0x1e, 0x5d, 0x91, 0xf4, 0xbe, 0xb2, 0x38, 0xb7, 0xcc, 0xf9, 0x53,
	0x7c, 0xd2, 0x49, 0x56, 0x34, 0xf3, 0xfc, 0x61, 0xf3, 0x13, 0xdf, 0x9f, 0x1b, 0x3d, 0xd9, 0xc1,
	0xd5, 0xa1, 0xb6, 0x06, 0xf6, 0xb6, 0x0e, 0x8b, 0x06, 0x2b, 0xaa, 0xb4, 0xf5, 0xdf, 0x04, 0x34,
	0x3b, 0x7b, 0xb3, 0xa5, 0x80, 0x17, 0x6e, 0xe2, 0x46, 0xf7, 0x43, 0x67, 0x3c, 0x3e, 0xec, 0x7e,
	0xec, 0x0f, 0xf7, 0x7b, 0xcd, 0x05, 0x97, 0xe9, 0x77, 0x06, 0x9d, 0xf7, 0xbd, 0xfd, 0x22, 0x83,
	0xa2, 0xc6, 0xc5, 0xf5, 0x06, 0xba, 0xbc, 0xde, 0x40, 0x3f, 0xaf, 0x37, 0xd0, 0x11, 0x9a, 0xd4,
	0xe0, 0x37, 0x7b, 0xf9, 0x3b, 0x00, 0x00, 0xff, 0xff, 0x55, 0x0f, 0x8d, 0xa6, 0x60, 0x04, 0x00,
	0x00,
}
