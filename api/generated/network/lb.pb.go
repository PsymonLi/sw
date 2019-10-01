// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: lb.proto

package network

import proto "github.com/gogo/protobuf/proto"
import fmt "fmt"
import math "math"
import _ "github.com/pensando/grpc-gateway/third_party/googleapis/google/api"
import _ "github.com/pensando/sw/venice/utils/apigen/annotations"
import _ "github.com/gogo/protobuf/gogoproto"
import api "github.com/pensando/sw/api"
import _ "github.com/pensando/sw/api/labels"
import _ "github.com/pensando/sw/api/generated/cluster"

import io "io"

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

//
type HealthCheckSpec struct {
	// Health check interval
	Interval uint32 `protobuf:"varint,1,opt,name=Interval,json=interval,omitempty,proto3" json:"interval,omitempty"`
	// # of probes per interval
	ProbesPerInterval uint32 `protobuf:"varint,2,opt,name=ProbesPerInterval,json=probes-per-interval,omitempty,proto3" json:"probes-per-interval,omitempty"`
	// probe URL
	ProbePortOrUrl string `protobuf:"bytes,3,opt,name=ProbePortOrUrl,json=probe-port-or-url,omitempty,proto3" json:"probe-port-or-url,omitempty"`
	// timeout for declaring backend down
	MaxTimeouts uint32 `protobuf:"varint,4,opt,name=MaxTimeouts,json=max-timeouts,omitempty,proto3" json:"max-timeouts,omitempty"`
	// # of successful probes before we declare the backend back up
	DeclareHealthyCount uint32 `protobuf:"varint,5,opt,name=DeclareHealthyCount,json=declare-healthy-count,omitempty,proto3" json:"declare-healthy-count,omitempty"`
}

func (m *HealthCheckSpec) Reset()                    { *m = HealthCheckSpec{} }
func (m *HealthCheckSpec) String() string            { return proto.CompactTextString(m) }
func (*HealthCheckSpec) ProtoMessage()               {}
func (*HealthCheckSpec) Descriptor() ([]byte, []int) { return fileDescriptorLb, []int{0} }

func (m *HealthCheckSpec) GetInterval() uint32 {
	if m != nil {
		return m.Interval
	}
	return 0
}

func (m *HealthCheckSpec) GetProbesPerInterval() uint32 {
	if m != nil {
		return m.ProbesPerInterval
	}
	return 0
}

func (m *HealthCheckSpec) GetProbePortOrUrl() string {
	if m != nil {
		return m.ProbePortOrUrl
	}
	return ""
}

func (m *HealthCheckSpec) GetMaxTimeouts() uint32 {
	if m != nil {
		return m.MaxTimeouts
	}
	return 0
}

func (m *HealthCheckSpec) GetDeclareHealthyCount() uint32 {
	if m != nil {
		return m.DeclareHealthyCount
	}
	return 0
}

// LbPolicy represents a load balancer policy
type LbPolicy struct {
	//
	api.TypeMeta `protobuf:"bytes,1,opt,name=T,json=,inline,embedded=T" json:",inline"`
	//
	api.ObjectMeta `protobuf:"bytes,2,opt,name=O,json=meta,omitempty,embedded=O" json:"meta,omitempty"`
	// Spec contains the configuration of the LbPolicy.
	Spec LbPolicySpec `protobuf:"bytes,3,opt,name=Spec,json=spec,omitempty" json:"spec,omitempty"`
	// Status contains the current state of the LbPolicy.
	Status LbPolicyStatus `protobuf:"bytes,4,opt,name=Status,json=status,omitempty" json:"status,omitempty"`
}

func (m *LbPolicy) Reset()                    { *m = LbPolicy{} }
func (m *LbPolicy) String() string            { return proto.CompactTextString(m) }
func (*LbPolicy) ProtoMessage()               {}
func (*LbPolicy) Descriptor() ([]byte, []int) { return fileDescriptorLb, []int{1} }

func (m *LbPolicy) GetSpec() LbPolicySpec {
	if m != nil {
		return m.Spec
	}
	return LbPolicySpec{}
}

func (m *LbPolicy) GetStatus() LbPolicyStatus {
	if m != nil {
		return m.Status
	}
	return LbPolicyStatus{}
}

//
type LbPolicySpec struct {
	// load balancing type
	Type string `protobuf:"bytes,1,opt,name=Type,json=type,omitempty,proto3" json:"type,omitempty"`
	// load balancing algorithm
	Algorithm string `protobuf:"bytes,2,opt,name=Algorithm,json=algorithm,omitempty,proto3" json:"algorithm,omitempty"`
	// session affinity
	SessionAffinity string `protobuf:"bytes,3,opt,name=SessionAffinity,json=session-affinity,omitempty,proto3" json:"session-affinity,omitempty"`
	// health check policy
	HealthCheck *HealthCheckSpec `protobuf:"bytes,4,opt,name=HealthCheck,json=health-check,omitempty" json:"health-check,omitempty"`
}

func (m *LbPolicySpec) Reset()                    { *m = LbPolicySpec{} }
func (m *LbPolicySpec) String() string            { return proto.CompactTextString(m) }
func (*LbPolicySpec) ProtoMessage()               {}
func (*LbPolicySpec) Descriptor() ([]byte, []int) { return fileDescriptorLb, []int{2} }

func (m *LbPolicySpec) GetType() string {
	if m != nil {
		return m.Type
	}
	return ""
}

func (m *LbPolicySpec) GetAlgorithm() string {
	if m != nil {
		return m.Algorithm
	}
	return ""
}

func (m *LbPolicySpec) GetSessionAffinity() string {
	if m != nil {
		return m.SessionAffinity
	}
	return ""
}

func (m *LbPolicySpec) GetHealthCheck() *HealthCheckSpec {
	if m != nil {
		return m.HealthCheck
	}
	return nil
}

//
type LbPolicyStatus struct {
	// list of service objects referring this lb-policy
	Services []string `protobuf:"bytes,1,rep,name=Services,json=type,omitempty" json:"type,omitempty"`
}

func (m *LbPolicyStatus) Reset()                    { *m = LbPolicyStatus{} }
func (m *LbPolicyStatus) String() string            { return proto.CompactTextString(m) }
func (*LbPolicyStatus) ProtoMessage()               {}
func (*LbPolicyStatus) Descriptor() ([]byte, []int) { return fileDescriptorLb, []int{3} }

func (m *LbPolicyStatus) GetServices() []string {
	if m != nil {
		return m.Services
	}
	return nil
}

func init() {
	proto.RegisterType((*HealthCheckSpec)(nil), "network.HealthCheckSpec")
	proto.RegisterType((*LbPolicy)(nil), "network.LbPolicy")
	proto.RegisterType((*LbPolicySpec)(nil), "network.LbPolicySpec")
	proto.RegisterType((*LbPolicyStatus)(nil), "network.LbPolicyStatus")
}
func (m *HealthCheckSpec) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *HealthCheckSpec) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if m.Interval != 0 {
		dAtA[i] = 0x8
		i++
		i = encodeVarintLb(dAtA, i, uint64(m.Interval))
	}
	if m.ProbesPerInterval != 0 {
		dAtA[i] = 0x10
		i++
		i = encodeVarintLb(dAtA, i, uint64(m.ProbesPerInterval))
	}
	if len(m.ProbePortOrUrl) > 0 {
		dAtA[i] = 0x1a
		i++
		i = encodeVarintLb(dAtA, i, uint64(len(m.ProbePortOrUrl)))
		i += copy(dAtA[i:], m.ProbePortOrUrl)
	}
	if m.MaxTimeouts != 0 {
		dAtA[i] = 0x20
		i++
		i = encodeVarintLb(dAtA, i, uint64(m.MaxTimeouts))
	}
	if m.DeclareHealthyCount != 0 {
		dAtA[i] = 0x28
		i++
		i = encodeVarintLb(dAtA, i, uint64(m.DeclareHealthyCount))
	}
	return i, nil
}

func (m *LbPolicy) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *LbPolicy) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	dAtA[i] = 0xa
	i++
	i = encodeVarintLb(dAtA, i, uint64(m.TypeMeta.Size()))
	n1, err := m.TypeMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n1
	dAtA[i] = 0x12
	i++
	i = encodeVarintLb(dAtA, i, uint64(m.ObjectMeta.Size()))
	n2, err := m.ObjectMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n2
	dAtA[i] = 0x1a
	i++
	i = encodeVarintLb(dAtA, i, uint64(m.Spec.Size()))
	n3, err := m.Spec.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n3
	dAtA[i] = 0x22
	i++
	i = encodeVarintLb(dAtA, i, uint64(m.Status.Size()))
	n4, err := m.Status.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n4
	return i, nil
}

func (m *LbPolicySpec) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *LbPolicySpec) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Type) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintLb(dAtA, i, uint64(len(m.Type)))
		i += copy(dAtA[i:], m.Type)
	}
	if len(m.Algorithm) > 0 {
		dAtA[i] = 0x12
		i++
		i = encodeVarintLb(dAtA, i, uint64(len(m.Algorithm)))
		i += copy(dAtA[i:], m.Algorithm)
	}
	if len(m.SessionAffinity) > 0 {
		dAtA[i] = 0x1a
		i++
		i = encodeVarintLb(dAtA, i, uint64(len(m.SessionAffinity)))
		i += copy(dAtA[i:], m.SessionAffinity)
	}
	if m.HealthCheck != nil {
		dAtA[i] = 0x22
		i++
		i = encodeVarintLb(dAtA, i, uint64(m.HealthCheck.Size()))
		n5, err := m.HealthCheck.MarshalTo(dAtA[i:])
		if err != nil {
			return 0, err
		}
		i += n5
	}
	return i, nil
}

func (m *LbPolicyStatus) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *LbPolicyStatus) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Services) > 0 {
		for _, s := range m.Services {
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
	return i, nil
}

func encodeVarintLb(dAtA []byte, offset int, v uint64) int {
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return offset + 1
}
func (m *HealthCheckSpec) Size() (n int) {
	var l int
	_ = l
	if m.Interval != 0 {
		n += 1 + sovLb(uint64(m.Interval))
	}
	if m.ProbesPerInterval != 0 {
		n += 1 + sovLb(uint64(m.ProbesPerInterval))
	}
	l = len(m.ProbePortOrUrl)
	if l > 0 {
		n += 1 + l + sovLb(uint64(l))
	}
	if m.MaxTimeouts != 0 {
		n += 1 + sovLb(uint64(m.MaxTimeouts))
	}
	if m.DeclareHealthyCount != 0 {
		n += 1 + sovLb(uint64(m.DeclareHealthyCount))
	}
	return n
}

func (m *LbPolicy) Size() (n int) {
	var l int
	_ = l
	l = m.TypeMeta.Size()
	n += 1 + l + sovLb(uint64(l))
	l = m.ObjectMeta.Size()
	n += 1 + l + sovLb(uint64(l))
	l = m.Spec.Size()
	n += 1 + l + sovLb(uint64(l))
	l = m.Status.Size()
	n += 1 + l + sovLb(uint64(l))
	return n
}

func (m *LbPolicySpec) Size() (n int) {
	var l int
	_ = l
	l = len(m.Type)
	if l > 0 {
		n += 1 + l + sovLb(uint64(l))
	}
	l = len(m.Algorithm)
	if l > 0 {
		n += 1 + l + sovLb(uint64(l))
	}
	l = len(m.SessionAffinity)
	if l > 0 {
		n += 1 + l + sovLb(uint64(l))
	}
	if m.HealthCheck != nil {
		l = m.HealthCheck.Size()
		n += 1 + l + sovLb(uint64(l))
	}
	return n
}

func (m *LbPolicyStatus) Size() (n int) {
	var l int
	_ = l
	if len(m.Services) > 0 {
		for _, s := range m.Services {
			l = len(s)
			n += 1 + l + sovLb(uint64(l))
		}
	}
	return n
}

func sovLb(x uint64) (n int) {
	for {
		n++
		x >>= 7
		if x == 0 {
			break
		}
	}
	return n
}
func sozLb(x uint64) (n int) {
	return sovLb(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *HealthCheckSpec) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowLb
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
			return fmt.Errorf("proto: HealthCheckSpec: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: HealthCheckSpec: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field Interval", wireType)
			}
			m.Interval = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowLb
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.Interval |= (uint32(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 2:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field ProbesPerInterval", wireType)
			}
			m.ProbesPerInterval = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowLb
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.ProbesPerInterval |= (uint32(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 3:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field ProbePortOrUrl", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowLb
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
				return ErrInvalidLengthLb
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.ProbePortOrUrl = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 4:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field MaxTimeouts", wireType)
			}
			m.MaxTimeouts = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowLb
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.MaxTimeouts |= (uint32(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 5:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field DeclareHealthyCount", wireType)
			}
			m.DeclareHealthyCount = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowLb
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.DeclareHealthyCount |= (uint32(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		default:
			iNdEx = preIndex
			skippy, err := skipLb(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthLb
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
func (m *LbPolicy) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowLb
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
			return fmt.Errorf("proto: LbPolicy: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: LbPolicy: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field TypeMeta", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowLb
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
				return ErrInvalidLengthLb
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
					return ErrIntOverflowLb
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
				return ErrInvalidLengthLb
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
					return ErrIntOverflowLb
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
				return ErrInvalidLengthLb
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
					return ErrIntOverflowLb
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
				return ErrInvalidLengthLb
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
			skippy, err := skipLb(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthLb
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
func (m *LbPolicySpec) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowLb
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
			return fmt.Errorf("proto: LbPolicySpec: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: LbPolicySpec: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Type", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowLb
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
				return ErrInvalidLengthLb
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Type = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Algorithm", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowLb
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
				return ErrInvalidLengthLb
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Algorithm = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 3:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field SessionAffinity", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowLb
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
				return ErrInvalidLengthLb
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.SessionAffinity = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 4:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field HealthCheck", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowLb
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
				return ErrInvalidLengthLb
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if m.HealthCheck == nil {
				m.HealthCheck = &HealthCheckSpec{}
			}
			if err := m.HealthCheck.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipLb(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthLb
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
func (m *LbPolicyStatus) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowLb
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
			return fmt.Errorf("proto: LbPolicyStatus: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: LbPolicyStatus: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Services", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowLb
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
				return ErrInvalidLengthLb
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Services = append(m.Services, string(dAtA[iNdEx:postIndex]))
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipLb(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthLb
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
func skipLb(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowLb
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
					return 0, ErrIntOverflowLb
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
					return 0, ErrIntOverflowLb
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
				return 0, ErrInvalidLengthLb
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowLb
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
				next, err := skipLb(dAtA[start:])
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
	ErrInvalidLengthLb = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowLb   = fmt.Errorf("proto: integer overflow")
)

func init() { proto.RegisterFile("lb.proto", fileDescriptorLb) }

var fileDescriptorLb = []byte{
	// 697 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x8c, 0x94, 0xcf, 0x4f, 0x13, 0x41,
	0x14, 0xc7, 0x59, 0x40, 0x68, 0xa7, 0xf2, 0xc3, 0x21, 0xe2, 0x5a, 0xb0, 0x25, 0x35, 0x44, 0x0e,
	0xec, 0xae, 0xa9, 0xc6, 0x18, 0x4f, 0x52, 0xd4, 0xa8, 0x48, 0xda, 0xd0, 0x1a, 0xbd, 0x99, 0xd9,
	0xe5, 0xd1, 0x8e, 0x6c, 0x67, 0x36, 0xb3, 0xb3, 0x40, 0x63, 0x3c, 0xfa, 0x8f, 0x79, 0x42, 0x4f,
	0xc4, 0x3f, 0x80, 0x98, 0x9e, 0x8c, 0x7f, 0x85, 0xd9, 0xd9, 0x29, 0x0e, 0x50, 0xaa, 0xb7, 0x7d,
	0xef, 0x7d, 0xbf, 0x9f, 0x79, 0x79, 0xf3, 0x66, 0x51, 0x2e, 0xf4, 0xdd, 0x48, 0x70, 0xc9, 0xf1,
	0x34, 0x03, 0x79, 0xc8, 0xc5, 0x7e, 0x71, 0xb9, 0xcd, 0x79, 0x3b, 0x04, 0x8f, 0x44, 0xd4, 0x23,
	0x8c, 0x71, 0x49, 0x24, 0xe5, 0x2c, 0xce, 0x64, 0xc5, 0xe7, 0x6d, 0x2a, 0x3b, 0x89, 0xef, 0x06,
	0xbc, 0xeb, 0x45, 0xc0, 0x62, 0xc2, 0x76, 0xb9, 0x17, 0x1f, 0x7a, 0x07, 0xc0, 0x68, 0x00, 0x5e,
	0x22, 0x69, 0x18, 0xa7, 0xd6, 0x36, 0x30, 0xd3, 0xed, 0x51, 0x16, 0x84, 0xc9, 0x2e, 0x0c, 0x30,
	0x8e, 0x81, 0x69, 0xf3, 0x36, 0xf7, 0x54, 0xda, 0x4f, 0xf6, 0x54, 0xa4, 0x02, 0xf5, 0xa5, 0xe5,
	0xab, 0x57, 0x9c, 0x9a, 0xf6, 0xd8, 0x05, 0x49, 0xb4, 0xec, 0xfe, 0x08, 0x59, 0x48, 0x7c, 0x08,
	0x63, 0x2f, 0x86, 0x10, 0x02, 0xc9, 0x85, 0x76, 0xb8, 0x23, 0x1c, 0x4a, 0x11, 0x7b, 0x12, 0x18,
	0x61, 0x32, 0xd3, 0x57, 0xbe, 0x4e, 0xa0, 0xb9, 0x97, 0x40, 0x42, 0xd9, 0xd9, 0xec, 0x40, 0xb0,
	0xdf, 0x8c, 0x20, 0xc0, 0x4f, 0x51, 0xee, 0x15, 0x93, 0x20, 0x0e, 0x48, 0x68, 0x5b, 0x2b, 0xd6,
	0xda, 0x4c, 0xad, 0x78, 0x7c, 0x5a, 0xb6, 0x7e, 0x9f, 0x96, 0x31, 0xd5, 0xf9, 0x75, 0xde, 0xa5,
	0x12, 0xba, 0x91, 0xec, 0xed, 0x0c, 0xc9, 0xe1, 0x0f, 0xe8, 0x46, 0x43, 0x70, 0x1f, 0xe2, 0x06,
	0x88, 0x33, 0xd4, 0xb8, 0x42, 0xad, 0x6a, 0xd4, 0x9d, 0x48, 0x09, 0x9c, 0x08, 0x84, 0x33, 0x84,
	0x3a, 0xba, 0x8c, 0xdf, 0xa3, 0x59, 0x75, 0x40, 0x83, 0x0b, 0x59, 0x17, 0x6f, 0x45, 0x68, 0x4f,
	0xac, 0x58, 0x6b, 0xf9, 0xda, 0x5d, 0x4d, 0x5f, 0x52, 0x76, 0x27, 0xe2, 0x42, 0x3a, 0x5c, 0x38,
	0x89, 0x30, 0xd9, 0xa3, 0x8a, 0x78, 0x0b, 0x15, 0xb6, 0xc9, 0x51, 0x8b, 0x76, 0x81, 0x27, 0x32,
	0xb6, 0x27, 0x55, 0xd3, 0x25, 0x8d, 0x5d, 0xec, 0x92, 0x23, 0x47, 0xea, 0x9a, 0x41, 0xbc, 0x22,
	0x8f, 0x01, 0x2d, 0x3c, 0x83, 0x20, 0x24, 0x02, 0xb2, 0x19, 0xf7, 0x36, 0x79, 0xc2, 0xa4, 0x7d,
	0x4d, 0x41, 0xef, 0x69, 0x68, 0x79, 0x37, 0x93, 0x38, 0x9d, 0x4c, 0xe3, 0x04, 0xa9, 0xc8, 0xa0,
	0xff, 0x4b, 0x50, 0xf9, 0x3e, 0x8e, 0x72, 0x6f, 0xfc, 0x06, 0x0f, 0x69, 0xd0, 0xc3, 0x8f, 0x90,
	0xd5, 0x52, 0xd7, 0x56, 0xa8, 0xce, 0xb8, 0x24, 0xa2, 0x6e, 0xab, 0x17, 0xc1, 0x36, 0x48, 0x52,
	0x5b, 0x38, 0x3e, 0x2d, 0x8f, 0x9d, 0x64, 0x87, 0x4e, 0xaf, 0x53, 0x16, 0x52, 0x06, 0x3b, 0x83,
	0x0f, 0xfc, 0x02, 0x59, 0x75, 0x75, 0x47, 0x85, 0xea, 0x9c, 0xf2, 0xd5, 0xfd, 0x8f, 0x10, 0x48,
	0xe5, 0x2c, 0x1a, 0xce, 0xd9, 0x74, 0x3f, 0x8d, 0xee, 0x2e, 0xc4, 0x78, 0x0b, 0x4d, 0xa6, 0x5b,
	0xa4, 0x2e, 0xa4, 0x50, 0xbd, 0xe9, 0xea, 0x67, 0xe8, 0x0e, 0x1a, 0x4c, 0x8b, 0xb5, 0xc5, 0x14,
	0x98, 0xc2, 0xe2, 0x08, 0x02, 0x13, 0x76, 0x3e, 0xc6, 0x4d, 0x34, 0xd5, 0x94, 0x44, 0x26, 0xd9,
	0x45, 0x14, 0xaa, 0xb7, 0x2e, 0xe3, 0x54, 0xb9, 0x66, 0x6b, 0xe0, 0x7c, 0xac, 0x62, 0x03, 0x79,
	0x29, 0xf3, 0x64, 0xf9, 0xc7, 0x97, 0xdb, 0x36, 0x2a, 0x78, 0x9f, 0xea, 0x6e, 0x4b, 0xbd, 0x85,
	0xcf, 0x38, 0x1f, 0xfa, 0x4e, 0xa4, 0x78, 0x95, 0x6f, 0xe3, 0xe8, 0xba, 0xd9, 0x2b, 0x7e, 0x88,
	0x26, 0xd3, 0x11, 0xaa, 0x99, 0xe6, 0x55, 0xe7, 0x6a, 0x0c, 0xb2, 0x17, 0x81, 0xd9, 0xf9, 0xf9,
	0x18, 0x6f, 0xa2, 0xfc, 0x46, 0xd8, 0xe6, 0x82, 0xca, 0x4e, 0x57, 0x8d, 0x35, 0x5f, 0x5b, 0xd2,
	0xd6, 0x05, 0x32, 0x28, 0x18, 0xfe, 0x61, 0x49, 0xfc, 0x0e, 0xcd, 0x35, 0x21, 0x8e, 0x29, 0x67,
	0x1b, 0x7b, 0x7b, 0x94, 0x51, 0xd9, 0xd3, 0x7b, 0x5e, 0xd1, 0xa8, 0x62, 0x9c, 0x95, 0x1d, 0xa2,
	0xeb, 0x06, 0x71, 0x44, 0x0d, 0x03, 0x2a, 0x18, 0xaf, 0x5e, 0x0f, 0xd7, 0x3e, 0x1b, 0xee, 0x85,
	0x3f, 0xc2, 0xdf, 0xfd, 0xcf, 0x36, 0xd0, 0x09, 0xd2, 0x8a, 0xb9, 0xff, 0xc3, 0xf3, 0x95, 0xd7,
	0x68, 0xf6, 0xfc, 0x3d, 0xe1, 0xc7, 0x28, 0xd7, 0x04, 0x71, 0x40, 0x03, 0x88, 0x6d, 0x6b, 0x65,
	0xe2, 0xff, 0x07, 0x5a, 0x9b, 0x3f, 0xee, 0x97, 0xac, 0x93, 0x7e, 0xc9, 0xfa, 0xd9, 0x2f, 0x59,
	0xbf, 0xfa, 0xa5, 0xb1, 0x86, 0xe5, 0x4f, 0xa9, 0x9f, 0xd8, 0x83, 0x3f, 0x01, 0x00, 0x00, 0xff,
	0xff, 0x76, 0xe2, 0xe4, 0xfe, 0xf6, 0x05, 0x00, 0x00,
}
