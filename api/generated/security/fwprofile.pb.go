// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: fwprofile.proto

package security

import proto "github.com/gogo/protobuf/proto"
import fmt "fmt"
import math "math"
import _ "github.com/pensando/grpc-gateway/third_party/googleapis/google/api"
import _ "github.com/pensando/sw/venice/utils/apigen/annotations"
import _ "github.com/gogo/protobuf/gogoproto"
import api "github.com/pensando/sw/api"

import io "io"

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

// FirewallProfile - defined one per tenant
type FirewallProfile struct {
	//
	api.TypeMeta `protobuf:"bytes,1,opt,name=T,json=,inline,embedded=T" json:",inline"`
	//
	api.ObjectMeta `protobuf:"bytes,2,opt,name=O,json=meta,omitempty,embedded=O" json:"meta,omitempty"`
	//
	Spec FirewallProfileSpec `protobuf:"bytes,3,opt,name=Spec,json=spec,omitempty" json:"spec,omitempty"`
	//
	Status FirewallProfileStatus `protobuf:"bytes,4,opt,name=Status,json=status,omitempty" json:"status,omitempty"`
}

func (m *FirewallProfile) Reset()                    { *m = FirewallProfile{} }
func (m *FirewallProfile) String() string            { return proto.CompactTextString(m) }
func (*FirewallProfile) ProtoMessage()               {}
func (*FirewallProfile) Descriptor() ([]byte, []int) { return fileDescriptorFwprofile, []int{0} }

func (m *FirewallProfile) GetSpec() FirewallProfileSpec {
	if m != nil {
		return m.Spec
	}
	return FirewallProfileSpec{}
}

func (m *FirewallProfile) GetStatus() FirewallProfileStatus {
	if m != nil {
		return m.Status
	}
	return FirewallProfileStatus{}
}

// FirewallProfileSpec - spec part of FirewallProfile object
type FirewallProfileSpec struct {
	// Session idle timeout removes/deletes the session/flow if there is inactivity; this value is superceded by any value specified in App object
	SessionIdleTimeout string `protobuf:"bytes,1,opt,name=SessionIdleTimeout,json=session-idle-timeout,omitempty,proto3" json:"session-idle-timeout,omitempty"`
	// TCP Connection Setup Timeout is the period TCP session is kept to see the response of a SYN
	TCPConnectionSetupTimeout string `protobuf:"bytes,2,opt,name=TCPConnectionSetupTimeout,json=tcp-connection-setup-timeout,omitempty,proto3" json:"tcp-connection-setup-timeout,omitempty"`
	// TCP Close Timeout is the time for which TCP session is kept after a FIN is seen
	TCPCloseTimeout string `protobuf:"bytes,3,opt,name=TCPCloseTimeout,json=tcp-close-timeout,omitempty,proto3" json:"tcp-close-timeout,omitempty"`
	// TCP Half Closed Timeout is the time for which tCP session is kept when connection is half closed i.e. FIN sent by FIN_Ack not received
	TCPHalfClosedTimeout string `protobuf:"bytes,4,opt,name=TCPHalfClosedTimeout,json=tcp-half-closed-timeout,omitempty,proto3" json:"tcp-half-closed-timeout,omitempty"`
	// TCP Drop Timeout is the period for which a drop entry is installed for a denied TCP flow
	TCPDropTimeout string `protobuf:"bytes,5,opt,name=TCPDropTimeout,json=tcp-drop-timeout,omitempty,proto3" json:"tcp-drop-timeout,omitempty"`
	// UDP Drop Timeout is the period for which a drop entry is installed for a denied UDP flow
	UDPDropTimeout string `protobuf:"bytes,6,opt,name=UDPDropTimeout,json=udp-drop-timeout,omitempty,proto3" json:"udp-drop-timeout,omitempty"`
	// ICMP Drop Timeout is the period for which a drop entry is installed for a denied ICMP flow
	ICMPDropTimeout string `protobuf:"bytes,7,opt,name=ICMPDropTimeout,json=icmp-drop-timeout,omitempty,proto3" json:"icmp-drop-timeout,omitempty"`
	// Drop Timeout is the period for which a drop entry is installed for a denied non tcp/udp/icmp flow
	DropTimeout string `protobuf:"bytes,8,opt,name=DropTimeout,json=drop-timeout,omitempty,proto3" json:"drop-timeout,omitempty"`
	// Tcp Timeout is the period for which a TCP session is kept alive during inactivity
	TcpTimeout string `protobuf:"bytes,9,opt,name=TcpTimeout,json=tcp-timeout,omitempty,proto3" json:"tcp-timeout,omitempty"`
	// Udp Timeout is the period for which a UDP session is kept alive during inactivity
	UdpTimeout string `protobuf:"bytes,10,opt,name=UdpTimeout,json=udp-timeout,omitempty,proto3" json:"udp-timeout,omitempty"`
	// Icmp Timeout is the period for which a ICMP session is kept alive during inactivity
	IcmpTimeout string `protobuf:"bytes,11,opt,name=IcmpTimeout,json=icmp-timeout,omitempty,proto3" json:"icmp-timeout,omitempty"`
	// Tcp half open session limit config after which new open requests will be dropped
	TcpHalfOpenSessionLimit uint32 `protobuf:"varint,12,opt,name=TcpHalfOpenSessionLimit,json=tcp-half-open-session-limit,proto3" json:"tcp-half-open-session-limit"`
	// Udp active session limit config after which new requests will be dropped
	UdpActiveSessionLimit uint32 `protobuf:"varint,13,opt,name=UdpActiveSessionLimit,json=udp-active-session-limit,proto3" json:"udp-active-session-limit"`
	// Icmp active session limit config after which new requests will be dropped
	IcmpActiveSessionLimit uint32 `protobuf:"varint,14,opt,name=IcmpActiveSessionLimit,json=icmp-active-session-limit,proto3" json:"icmp-active-session-limit"`
	// Set the Application Identification Detection config for DSCs
	DetectApp bool `protobuf:"varint,16,opt,name=DetectApp,json=detect-app,proto3" json:"detect-app"`
}

func (m *FirewallProfileSpec) Reset()                    { *m = FirewallProfileSpec{} }
func (m *FirewallProfileSpec) String() string            { return proto.CompactTextString(m) }
func (*FirewallProfileSpec) ProtoMessage()               {}
func (*FirewallProfileSpec) Descriptor() ([]byte, []int) { return fileDescriptorFwprofile, []int{1} }

func (m *FirewallProfileSpec) GetSessionIdleTimeout() string {
	if m != nil {
		return m.SessionIdleTimeout
	}
	return ""
}

func (m *FirewallProfileSpec) GetTCPConnectionSetupTimeout() string {
	if m != nil {
		return m.TCPConnectionSetupTimeout
	}
	return ""
}

func (m *FirewallProfileSpec) GetTCPCloseTimeout() string {
	if m != nil {
		return m.TCPCloseTimeout
	}
	return ""
}

func (m *FirewallProfileSpec) GetTCPHalfClosedTimeout() string {
	if m != nil {
		return m.TCPHalfClosedTimeout
	}
	return ""
}

func (m *FirewallProfileSpec) GetTCPDropTimeout() string {
	if m != nil {
		return m.TCPDropTimeout
	}
	return ""
}

func (m *FirewallProfileSpec) GetUDPDropTimeout() string {
	if m != nil {
		return m.UDPDropTimeout
	}
	return ""
}

func (m *FirewallProfileSpec) GetICMPDropTimeout() string {
	if m != nil {
		return m.ICMPDropTimeout
	}
	return ""
}

func (m *FirewallProfileSpec) GetDropTimeout() string {
	if m != nil {
		return m.DropTimeout
	}
	return ""
}

func (m *FirewallProfileSpec) GetTcpTimeout() string {
	if m != nil {
		return m.TcpTimeout
	}
	return ""
}

func (m *FirewallProfileSpec) GetUdpTimeout() string {
	if m != nil {
		return m.UdpTimeout
	}
	return ""
}

func (m *FirewallProfileSpec) GetIcmpTimeout() string {
	if m != nil {
		return m.IcmpTimeout
	}
	return ""
}

func (m *FirewallProfileSpec) GetTcpHalfOpenSessionLimit() uint32 {
	if m != nil {
		return m.TcpHalfOpenSessionLimit
	}
	return 0
}

func (m *FirewallProfileSpec) GetUdpActiveSessionLimit() uint32 {
	if m != nil {
		return m.UdpActiveSessionLimit
	}
	return 0
}

func (m *FirewallProfileSpec) GetIcmpActiveSessionLimit() uint32 {
	if m != nil {
		return m.IcmpActiveSessionLimit
	}
	return 0
}

func (m *FirewallProfileSpec) GetDetectApp() bool {
	if m != nil {
		return m.DetectApp
	}
	return false
}

// FirewallProfileStatus - status part of FirewallProfileObject
type FirewallProfileStatus struct {
	// The status of the configuration propagation to the Naples
	PropagationStatus PropagationStatus `protobuf:"bytes,1,opt,name=PropagationStatus,json=propagation-status" json:"propagation-status"`
}

func (m *FirewallProfileStatus) Reset()                    { *m = FirewallProfileStatus{} }
func (m *FirewallProfileStatus) String() string            { return proto.CompactTextString(m) }
func (*FirewallProfileStatus) ProtoMessage()               {}
func (*FirewallProfileStatus) Descriptor() ([]byte, []int) { return fileDescriptorFwprofile, []int{2} }

func (m *FirewallProfileStatus) GetPropagationStatus() PropagationStatus {
	if m != nil {
		return m.PropagationStatus
	}
	return PropagationStatus{}
}

func init() {
	proto.RegisterType((*FirewallProfile)(nil), "security.FirewallProfile")
	proto.RegisterType((*FirewallProfileSpec)(nil), "security.FirewallProfileSpec")
	proto.RegisterType((*FirewallProfileStatus)(nil), "security.FirewallProfileStatus")
}
func (m *FirewallProfile) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *FirewallProfile) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	dAtA[i] = 0xa
	i++
	i = encodeVarintFwprofile(dAtA, i, uint64(m.TypeMeta.Size()))
	n1, err := m.TypeMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n1
	dAtA[i] = 0x12
	i++
	i = encodeVarintFwprofile(dAtA, i, uint64(m.ObjectMeta.Size()))
	n2, err := m.ObjectMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n2
	dAtA[i] = 0x1a
	i++
	i = encodeVarintFwprofile(dAtA, i, uint64(m.Spec.Size()))
	n3, err := m.Spec.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n3
	dAtA[i] = 0x22
	i++
	i = encodeVarintFwprofile(dAtA, i, uint64(m.Status.Size()))
	n4, err := m.Status.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n4
	return i, nil
}

func (m *FirewallProfileSpec) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *FirewallProfileSpec) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.SessionIdleTimeout) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintFwprofile(dAtA, i, uint64(len(m.SessionIdleTimeout)))
		i += copy(dAtA[i:], m.SessionIdleTimeout)
	}
	if len(m.TCPConnectionSetupTimeout) > 0 {
		dAtA[i] = 0x12
		i++
		i = encodeVarintFwprofile(dAtA, i, uint64(len(m.TCPConnectionSetupTimeout)))
		i += copy(dAtA[i:], m.TCPConnectionSetupTimeout)
	}
	if len(m.TCPCloseTimeout) > 0 {
		dAtA[i] = 0x1a
		i++
		i = encodeVarintFwprofile(dAtA, i, uint64(len(m.TCPCloseTimeout)))
		i += copy(dAtA[i:], m.TCPCloseTimeout)
	}
	if len(m.TCPHalfClosedTimeout) > 0 {
		dAtA[i] = 0x22
		i++
		i = encodeVarintFwprofile(dAtA, i, uint64(len(m.TCPHalfClosedTimeout)))
		i += copy(dAtA[i:], m.TCPHalfClosedTimeout)
	}
	if len(m.TCPDropTimeout) > 0 {
		dAtA[i] = 0x2a
		i++
		i = encodeVarintFwprofile(dAtA, i, uint64(len(m.TCPDropTimeout)))
		i += copy(dAtA[i:], m.TCPDropTimeout)
	}
	if len(m.UDPDropTimeout) > 0 {
		dAtA[i] = 0x32
		i++
		i = encodeVarintFwprofile(dAtA, i, uint64(len(m.UDPDropTimeout)))
		i += copy(dAtA[i:], m.UDPDropTimeout)
	}
	if len(m.ICMPDropTimeout) > 0 {
		dAtA[i] = 0x3a
		i++
		i = encodeVarintFwprofile(dAtA, i, uint64(len(m.ICMPDropTimeout)))
		i += copy(dAtA[i:], m.ICMPDropTimeout)
	}
	if len(m.DropTimeout) > 0 {
		dAtA[i] = 0x42
		i++
		i = encodeVarintFwprofile(dAtA, i, uint64(len(m.DropTimeout)))
		i += copy(dAtA[i:], m.DropTimeout)
	}
	if len(m.TcpTimeout) > 0 {
		dAtA[i] = 0x4a
		i++
		i = encodeVarintFwprofile(dAtA, i, uint64(len(m.TcpTimeout)))
		i += copy(dAtA[i:], m.TcpTimeout)
	}
	if len(m.UdpTimeout) > 0 {
		dAtA[i] = 0x52
		i++
		i = encodeVarintFwprofile(dAtA, i, uint64(len(m.UdpTimeout)))
		i += copy(dAtA[i:], m.UdpTimeout)
	}
	if len(m.IcmpTimeout) > 0 {
		dAtA[i] = 0x5a
		i++
		i = encodeVarintFwprofile(dAtA, i, uint64(len(m.IcmpTimeout)))
		i += copy(dAtA[i:], m.IcmpTimeout)
	}
	if m.TcpHalfOpenSessionLimit != 0 {
		dAtA[i] = 0x60
		i++
		i = encodeVarintFwprofile(dAtA, i, uint64(m.TcpHalfOpenSessionLimit))
	}
	if m.UdpActiveSessionLimit != 0 {
		dAtA[i] = 0x68
		i++
		i = encodeVarintFwprofile(dAtA, i, uint64(m.UdpActiveSessionLimit))
	}
	if m.IcmpActiveSessionLimit != 0 {
		dAtA[i] = 0x70
		i++
		i = encodeVarintFwprofile(dAtA, i, uint64(m.IcmpActiveSessionLimit))
	}
	if m.DetectApp {
		dAtA[i] = 0x80
		i++
		dAtA[i] = 0x1
		i++
		if m.DetectApp {
			dAtA[i] = 1
		} else {
			dAtA[i] = 0
		}
		i++
	}
	return i, nil
}

func (m *FirewallProfileStatus) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *FirewallProfileStatus) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	dAtA[i] = 0xa
	i++
	i = encodeVarintFwprofile(dAtA, i, uint64(m.PropagationStatus.Size()))
	n5, err := m.PropagationStatus.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n5
	return i, nil
}

func encodeVarintFwprofile(dAtA []byte, offset int, v uint64) int {
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return offset + 1
}
func (m *FirewallProfile) Size() (n int) {
	var l int
	_ = l
	l = m.TypeMeta.Size()
	n += 1 + l + sovFwprofile(uint64(l))
	l = m.ObjectMeta.Size()
	n += 1 + l + sovFwprofile(uint64(l))
	l = m.Spec.Size()
	n += 1 + l + sovFwprofile(uint64(l))
	l = m.Status.Size()
	n += 1 + l + sovFwprofile(uint64(l))
	return n
}

func (m *FirewallProfileSpec) Size() (n int) {
	var l int
	_ = l
	l = len(m.SessionIdleTimeout)
	if l > 0 {
		n += 1 + l + sovFwprofile(uint64(l))
	}
	l = len(m.TCPConnectionSetupTimeout)
	if l > 0 {
		n += 1 + l + sovFwprofile(uint64(l))
	}
	l = len(m.TCPCloseTimeout)
	if l > 0 {
		n += 1 + l + sovFwprofile(uint64(l))
	}
	l = len(m.TCPHalfClosedTimeout)
	if l > 0 {
		n += 1 + l + sovFwprofile(uint64(l))
	}
	l = len(m.TCPDropTimeout)
	if l > 0 {
		n += 1 + l + sovFwprofile(uint64(l))
	}
	l = len(m.UDPDropTimeout)
	if l > 0 {
		n += 1 + l + sovFwprofile(uint64(l))
	}
	l = len(m.ICMPDropTimeout)
	if l > 0 {
		n += 1 + l + sovFwprofile(uint64(l))
	}
	l = len(m.DropTimeout)
	if l > 0 {
		n += 1 + l + sovFwprofile(uint64(l))
	}
	l = len(m.TcpTimeout)
	if l > 0 {
		n += 1 + l + sovFwprofile(uint64(l))
	}
	l = len(m.UdpTimeout)
	if l > 0 {
		n += 1 + l + sovFwprofile(uint64(l))
	}
	l = len(m.IcmpTimeout)
	if l > 0 {
		n += 1 + l + sovFwprofile(uint64(l))
	}
	if m.TcpHalfOpenSessionLimit != 0 {
		n += 1 + sovFwprofile(uint64(m.TcpHalfOpenSessionLimit))
	}
	if m.UdpActiveSessionLimit != 0 {
		n += 1 + sovFwprofile(uint64(m.UdpActiveSessionLimit))
	}
	if m.IcmpActiveSessionLimit != 0 {
		n += 1 + sovFwprofile(uint64(m.IcmpActiveSessionLimit))
	}
	if m.DetectApp {
		n += 3
	}
	return n
}

func (m *FirewallProfileStatus) Size() (n int) {
	var l int
	_ = l
	l = m.PropagationStatus.Size()
	n += 1 + l + sovFwprofile(uint64(l))
	return n
}

func sovFwprofile(x uint64) (n int) {
	for {
		n++
		x >>= 7
		if x == 0 {
			break
		}
	}
	return n
}
func sozFwprofile(x uint64) (n int) {
	return sovFwprofile(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *FirewallProfile) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowFwprofile
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
			return fmt.Errorf("proto: FirewallProfile: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: FirewallProfile: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field TypeMeta", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
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
				return ErrInvalidLengthFwprofile
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
					return ErrIntOverflowFwprofile
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
				return ErrInvalidLengthFwprofile
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
					return ErrIntOverflowFwprofile
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
				return ErrInvalidLengthFwprofile
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
					return ErrIntOverflowFwprofile
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
				return ErrInvalidLengthFwprofile
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
			skippy, err := skipFwprofile(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthFwprofile
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
func (m *FirewallProfileSpec) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowFwprofile
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
			return fmt.Errorf("proto: FirewallProfileSpec: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: FirewallProfileSpec: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field SessionIdleTimeout", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
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
				return ErrInvalidLengthFwprofile
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.SessionIdleTimeout = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field TCPConnectionSetupTimeout", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
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
				return ErrInvalidLengthFwprofile
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.TCPConnectionSetupTimeout = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 3:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field TCPCloseTimeout", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
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
				return ErrInvalidLengthFwprofile
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.TCPCloseTimeout = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 4:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field TCPHalfClosedTimeout", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
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
				return ErrInvalidLengthFwprofile
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.TCPHalfClosedTimeout = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 5:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field TCPDropTimeout", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
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
				return ErrInvalidLengthFwprofile
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.TCPDropTimeout = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 6:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field UDPDropTimeout", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
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
				return ErrInvalidLengthFwprofile
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.UDPDropTimeout = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 7:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field ICMPDropTimeout", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
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
				return ErrInvalidLengthFwprofile
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.ICMPDropTimeout = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 8:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field DropTimeout", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
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
				return ErrInvalidLengthFwprofile
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.DropTimeout = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 9:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field TcpTimeout", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
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
				return ErrInvalidLengthFwprofile
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.TcpTimeout = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 10:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field UdpTimeout", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
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
				return ErrInvalidLengthFwprofile
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.UdpTimeout = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 11:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field IcmpTimeout", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
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
				return ErrInvalidLengthFwprofile
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.IcmpTimeout = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 12:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field TcpHalfOpenSessionLimit", wireType)
			}
			m.TcpHalfOpenSessionLimit = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.TcpHalfOpenSessionLimit |= (uint32(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 13:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field UdpActiveSessionLimit", wireType)
			}
			m.UdpActiveSessionLimit = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.UdpActiveSessionLimit |= (uint32(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 14:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field IcmpActiveSessionLimit", wireType)
			}
			m.IcmpActiveSessionLimit = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.IcmpActiveSessionLimit |= (uint32(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 16:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field DetectApp", wireType)
			}
			var v int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
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
			m.DetectApp = bool(v != 0)
		default:
			iNdEx = preIndex
			skippy, err := skipFwprofile(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthFwprofile
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
func (m *FirewallProfileStatus) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowFwprofile
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
			return fmt.Errorf("proto: FirewallProfileStatus: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: FirewallProfileStatus: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field PropagationStatus", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowFwprofile
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
				return ErrInvalidLengthFwprofile
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if err := m.PropagationStatus.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipFwprofile(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthFwprofile
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
func skipFwprofile(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowFwprofile
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
					return 0, ErrIntOverflowFwprofile
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
					return 0, ErrIntOverflowFwprofile
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
				return 0, ErrInvalidLengthFwprofile
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowFwprofile
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
				next, err := skipFwprofile(dAtA[start:])
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
	ErrInvalidLengthFwprofile = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowFwprofile   = fmt.Errorf("proto: integer overflow")
)

func init() { proto.RegisterFile("fwprofile.proto", fileDescriptorFwprofile) }

var fileDescriptorFwprofile = []byte{
	// 946 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x8c, 0x96, 0xc1, 0x6e, 0xdb, 0x36,
	0x18, 0xc7, 0xab, 0x24, 0x4d, 0x13, 0x66, 0x71, 0x5c, 0xb6, 0x4e, 0xed, 0x64, 0xb3, 0xb3, 0x0c,
	0x1b, 0x3c, 0xc0, 0xb6, 0x9c, 0x18, 0x53, 0xd2, 0x0e, 0x3b, 0xd4, 0x4e, 0x8b, 0x05, 0x58, 0x11,
	0xc3, 0x56, 0x4e, 0x3b, 0x29, 0x14, 0xad, 0x72, 0x90, 0x49, 0x56, 0xa4, 0x1a, 0x04, 0xc3, 0xb6,
	0xd3, 0x7c, 0xde, 0x63, 0x0c, 0x39, 0xee, 0x29, 0x7a, 0x2c, 0xf6, 0x00, 0xc6, 0x90, 0x53, 0x91,
	0xa7, 0x18, 0x48, 0xd9, 0xaa, 0x92, 0x48, 0x76, 0x6f, 0x22, 0x3f, 0x7e, 0xff, 0xdf, 0x5f, 0xdf,
	0x47, 0x8a, 0x02, 0x1b, 0x83, 0x73, 0x1e, 0xb0, 0x01, 0xf1, 0x71, 0x83, 0x07, 0x4c, 0x32, 0xb8,
	0x22, 0x30, 0x0a, 0x03, 0x22, 0x2f, 0xb6, 0x3e, 0xf7, 0x18, 0xf3, 0x7c, 0x6c, 0x3a, 0x9c, 0x98,
	0x0e, 0xa5, 0x4c, 0x3a, 0x92, 0x30, 0x2a, 0xa2, 0x75, 0x5b, 0x2f, 0x3c, 0x22, 0x5f, 0x87, 0x67,
	0x0d, 0xc4, 0x86, 0x26, 0xc7, 0x54, 0x38, 0xd4, 0x65, 0xa6, 0x38, 0x37, 0xdf, 0x62, 0x4a, 0x10,
	0x36, 0x43, 0x49, 0x7c, 0xa1, 0x52, 0x3d, 0x4c, 0x93, 0xd9, 0x26, 0xa1, 0xc8, 0x0f, 0x5d, 0x3c,
	0x95, 0xa9, 0x27, 0x64, 0x3c, 0xe6, 0x31, 0x53, 0x4f, 0x9f, 0x85, 0x03, 0x3d, 0xd2, 0x03, 0xfd,
	0x34, 0x59, 0xfe, 0x75, 0x06, 0x55, 0x79, 0x1c, 0x62, 0xe9, 0x4c, 0x96, 0xe5, 0x84, 0xc7, 0x99,
	0x4f, 0xd0, 0x45, 0x34, 0xde, 0xfd, 0xb0, 0x00, 0x36, 0x5e, 0x92, 0x00, 0x9f, 0x3b, 0xbe, 0xdf,
	0x8d, 0x5e, 0x17, 0x5a, 0xc0, 0xb0, 0x8b, 0xc6, 0x8e, 0x51, 0x5d, 0xdb, 0x5f, 0x6f, 0x38, 0x9c,
	0x34, 0xec, 0x0b, 0x8e, 0x5f, 0x61, 0xe9, 0xb4, 0x1f, 0xbd, 0x1b, 0x57, 0xee, 0xbd, 0x1f, 0x57,
	0x8c, 0xeb, 0x71, 0xe5, 0x41, 0x8d, 0x50, 0x9f, 0x50, 0xdc, 0x9b, 0x3e, 0xc0, 0x97, 0xc0, 0x38,
	0x29, 0x2e, 0xe8, 0xbc, 0x0d, 0x9d, 0x77, 0x72, 0xf6, 0x0b, 0x46, 0x52, 0x67, 0x6e, 0x25, 0x32,
	0x73, 0xca, 0x4f, 0x8d, 0x0d, 0x89, 0xc4, 0x43, 0x2e, 0x2f, 0x7a, 0xb7, 0xc6, 0xb0, 0x0f, 0x96,
	0xfa, 0x1c, 0xa3, 0xe2, 0xa2, 0x96, 0xfa, 0xa2, 0x31, 0xad, 0x7b, 0xe3, 0x96, 0x51, 0xb5, 0xa8,
	0xbd, 0xa9, 0x84, 0x95, 0xa8, 0xe0, 0x18, 0x25, 0x45, 0x6f, 0x8e, 0xe1, 0xcf, 0x60, 0xb9, 0x2f,
	0x1d, 0x19, 0x8a, 0xe2, 0x92, 0x96, 0xad, 0x64, 0xcb, 0xea, 0x65, 0xed, 0xe2, 0x44, 0x38, 0x2f,
	0xf4, 0x38, 0x21, 0x7d, 0x67, 0xe6, 0xd9, 0x57, 0xff, 0xfe, 0x59, 0xaa, 0x80, 0x35, 0xf3, 0xd7,
	0x93, 0x86, 0x8d, 0xa9, 0x43, 0xe5, 0x6f, 0x30, 0x3f, 0x98, 0xa8, 0x4e, 0x36, 0x91, 0xd8, 0xfd,
	0x7b, 0x1d, 0x3c, 0x4a, 0x79, 0x03, 0xf8, 0x3b, 0x80, 0x7d, 0x2c, 0x04, 0x61, 0xf4, 0xd8, 0xf5,
	0xb1, 0x4d, 0x86, 0x98, 0x85, 0x52, 0xd7, 0x7f, 0xb5, 0xdd, 0xbe, 0x1c, 0x95, 0x0a, 0x47, 0x61,
	0xa0, 0xb7, 0x48, 0xb5, 0xd5, 0x14, 0xb5, 0xbd, 0x83, 0xfd, 0xc3, 0x66, 0x53, 0x7c, 0xfb, 0xcf,
	0xa8, 0xb4, 0xf8, 0xb4, 0x29, 0xae, 0xc7, 0x95, 0xb2, 0x88, 0xb2, 0xeb, 0xc4, 0xf5, 0x71, 0x5d,
	0x46, 0xf9, 0x09, 0xbb, 0x73, 0xe2, 0xf0, 0x2f, 0x03, 0x94, 0xec, 0x4e, 0xb7, 0xc3, 0x28, 0xc5,
	0x48, 0x71, 0xfa, 0x58, 0x86, 0x7c, 0xea, 0x63, 0x41, 0xfb, 0x78, 0x71, 0x39, 0x2a, 0xe5, 0x63,
	0x1f, 0x7b, 0xa2, 0x66, 0x4d, 0x2c, 0xb4, 0xb4, 0x85, 0x6f, 0x24, 0xe2, 0x75, 0x14, 0x0b, 0xd4,
	0x85, 0x52, 0x48, 0xb1, 0xf2, 0x89, 0xeb, 0xe0, 0x1b, 0xb0, 0xa1, 0x1c, 0xf9, 0x4c, 0xc4, 0xf5,
	0x58, 0xd4, 0x3e, 0xbe, 0xbf, 0x1c, 0x95, 0x1e, 0x26, 0x7d, 0xb4, 0xa6, 0xb5, 0xd8, 0xfb, 0x4e,
	0x19, 0xd9, 0xd6, 0x00, 0x95, 0x97, 0x42, 0x9f, 0x15, 0x84, 0x23, 0x03, 0x3c, 0xb6, 0x3b, 0xdd,
	0x1f, 0x1d, 0x7f, 0xa0, 0xb9, 0xee, 0x14, 0xbc, 0x14, 0x17, 0xe0, 0x71, 0x12, 0x9c, 0xe8, 0xc3,
	0xd2, 0xde, 0xbe, 0xae, 0xc2, 0x97, 0x4a, 0xff, 0xb5, 0xe3, 0x0f, 0x22, 0x88, 0x9b, 0x62, 0x61,
	0xfe, 0x12, 0x48, 0x41, 0xce, 0xee, 0x74, 0x8f, 0x02, 0x16, 0xb7, 0xe0, 0xbe, 0x76, 0xf0, 0x2c,
	0xf3, 0xd5, 0xa3, 0x6d, 0xb0, 0xa5, 0xa4, 0xdd, 0x80, 0xa5, 0xd5, 0x7d, 0x46, 0x0c, 0xbe, 0x01,
	0xb9, 0xd3, 0xa3, 0x1b, 0xbc, 0x65, 0xcd, 0xfb, 0x61, 0xc6, 0x1b, 0x2f, 0x5a, 0x11, 0x32, 0x74,
	0xb3, 0x91, 0xd9, 0x31, 0xd5, 0xde, 0xe3, 0xce, 0xab, 0x1b, 0xcc, 0x07, 0x73, 0xda, 0x1b, 0x01,
	0xb7, 0x09, 0x1a, 0x66, 0x12, 0x67, 0x05, 0x21, 0x02, 0x6b, 0x49, 0xdc, 0x8a, 0xc6, 0x59, 0x73,
	0x70, 0x9b, 0x19, 0xa4, 0x8c, 0x79, 0x88, 0x01, 0xb0, 0x51, 0xcc, 0x58, 0x8d, 0xdb, 0x96, 0x55,
	0xc6, 0xfb, 0x2d, 0xab, 0xa9, 0x41, 0x05, 0xd5, 0x9f, 0xbb, 0x9c, 0xf4, 0x69, 0x88, 0x00, 0x38,
	0x75, 0x63, 0x0c, 0xd0, 0x98, 0xc3, 0x59, 0xdd, 0x8a, 0x0e, 0x69, 0x41, 0x75, 0x24, 0x05, 0x92,
	0x3a, 0x0d, 0x31, 0x58, 0x3b, 0x46, 0xc3, 0x98, 0xb2, 0x36, 0x97, 0xb2, 0x60, 0xe9, 0x92, 0xe9,
	0x26, 0xa4, 0x94, 0x2c, 0x7d, 0x1e, 0x9e, 0x83, 0x27, 0x36, 0xe2, 0xea, 0xd4, 0x9d, 0x70, 0x4c,
	0x27, 0xdf, 0xc1, 0x9f, 0xc8, 0x90, 0xc8, 0xe2, 0x67, 0x3b, 0x46, 0x75, 0xbd, 0xfd, 0x54, 0xf5,
	0xe8, 0x98, 0xca, 0x9e, 0x43, 0x3d, 0x5c, 0x6d, 0xd6, 0x5a, 0xfb, 0x07, 0xd6, 0xa1, 0xe2, 0x19,
	0xcd, 0xe9, 0x79, 0xd7, 0xe7, 0x89, 0x71, 0xac, 0xbe, 0x27, 0xd1, 0x97, 0xce, 0x57, 0x02, 0xbd,
	0x59, 0x41, 0xc8, 0x40, 0xe1, 0xd4, 0xe5, 0xcf, 0x91, 0x24, 0x6f, 0xf1, 0x0d, 0xec, 0xba, 0xc6,
	0x5a, 0x33, 0xb1, 0x45, 0x55, 0x33, 0x47, 0xe7, 0xde, 0x62, 0x66, 0x46, 0x60, 0x00, 0x36, 0x55,
	0x41, 0x53, 0x88, 0x39, 0x4d, 0x3c, 0x98, 0x49, 0x2c, 0xe9, 0xfa, 0xa5, 0x22, 0xb3, 0x43, 0xd0,
	0x02, 0xab, 0x47, 0x58, 0x62, 0x24, 0x9f, 0x73, 0x5e, 0xcc, 0xef, 0x18, 0xd5, 0x95, 0xf6, 0x13,
	0xb5, 0xef, 0x06, 0x8e, 0x2f, 0xf0, 0xf5, 0xb8, 0x02, 0x5c, 0x1d, 0xad, 0x3b, 0x9c, 0xf7, 0x12,
	0xcf, 0xbb, 0x7f, 0x80, 0x42, 0xea, 0xa5, 0x08, 0x07, 0xe0, 0x61, 0x37, 0x60, 0xdc, 0xf1, 0xf4,
	0x0e, 0x98, 0x5c, 0xa8, 0xd1, 0xaf, 0xc2, 0xf6, 0xc7, 0x0b, 0xf5, 0xce, 0x92, 0xe8, 0xfa, 0xbf,
	0x1e, 0x57, 0x20, 0xff, 0x18, 0xaa, 0x47, 0xd7, 0x68, 0x2f, 0x65, 0xae, 0x9d, 0x7f, 0x77, 0x55,
	0x36, 0xde, 0x5f, 0x95, 0x8d, 0xff, 0xae, 0xca, 0xc6, 0x87, 0xab, 0xf2, 0xbd, 0xae, 0x71, 0xb6,
	0xac, 0xff, 0x58, 0x5a, 0xff, 0x07, 0x00, 0x00, 0xff, 0xff, 0x58, 0x43, 0x8e, 0x06, 0x99, 0x09,
	0x00, 0x00,
}
