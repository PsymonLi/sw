// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: ipam.proto

/*
	Package network is a generated protocol buffer package.

	It is generated from these files:
		ipam.proto
		lb.proto
		network.proto
		networkinterface.proto
		route.proto
		service.proto
		svc_network.proto
		vrf.proto

	It has these top-level messages:
		DHCPRelayPolicy
		DHCPServer
		IPAMPolicy
		IPAMPolicySpec
		IPAMPolicyStatus
		HealthCheckSpec
		LbPolicy
		LbPolicySpec
		LbPolicyStatus
		Network
		NetworkSpec
		NetworkStatus
		OrchestratorInfo
		NetworkInterface
		NetworkInterfaceHostStatus
		NetworkInterfaceSpec
		NetworkInterfaceStatus
		NetworkInterfaceUplinkStatus
		PauseSpec
		TransceiverStatus
		BGPAuthStatus
		BGPConfig
		BGPNeighbor
		EVPNConfig
		RDSpec
		Route
		RouteDistinguisher
		RouteTable
		RouteTableSpec
		RouteTableStatus
		RoutingConfig
		RoutingConfigSpec
		RoutingConfigStatus
		Service
		ServiceSpec
		ServiceStatus
		TLSClientPolicySpec
		TLSServerPolicySpec
		AutoMsgIPAMPolicyWatchHelper
		AutoMsgLbPolicyWatchHelper
		AutoMsgNetworkInterfaceWatchHelper
		AutoMsgNetworkWatchHelper
		AutoMsgRouteTableWatchHelper
		AutoMsgRoutingConfigWatchHelper
		AutoMsgServiceWatchHelper
		AutoMsgVirtualRouterWatchHelper
		IPAMPolicyList
		LbPolicyList
		NetworkInterfaceList
		NetworkList
		RouteTableList
		RoutingConfigList
		ServiceList
		VirtualRouterList
		VirtualRouter
		VirtualRouterSpec
		VirtualRouterStatus
*/
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
import security "github.com/pensando/sw/api/generated/security"

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
type IPAMPolicySpec_IPAMType int32

const (
	//
	IPAMPolicySpec_DHCP_Relay IPAMPolicySpec_IPAMType = 0
)

var IPAMPolicySpec_IPAMType_name = map[int32]string{
	0: "DHCP_Relay",
}
var IPAMPolicySpec_IPAMType_value = map[string]int32{
	"DHCP_Relay": 0,
}

func (IPAMPolicySpec_IPAMType) EnumDescriptor() ([]byte, []int) {
	return fileDescriptorIpam, []int{3, 0}
}

//
type DHCPRelayPolicy struct {
	//
	Servers []*DHCPServer `protobuf:"bytes,1,rep,name=Servers,json=relay-servers,omitempty" json:"relay-servers,omitempty"`
}

func (m *DHCPRelayPolicy) Reset()                    { *m = DHCPRelayPolicy{} }
func (m *DHCPRelayPolicy) String() string            { return proto.CompactTextString(m) }
func (*DHCPRelayPolicy) ProtoMessage()               {}
func (*DHCPRelayPolicy) Descriptor() ([]byte, []int) { return fileDescriptorIpam, []int{0} }

func (m *DHCPRelayPolicy) GetServers() []*DHCPServer {
	if m != nil {
		return m.Servers
	}
	return nil
}

// DHCPServer specifies details about each server.
type DHCPServer struct {
	// IP Address of the server.
	IPAddress string `protobuf:"bytes,1,opt,name=IPAddress,json=ip-address,omitempty,proto3" json:"ip-address,omitempty"`
	// Destination VRF where the server is connected. An empty value specifies that the server is reachable in the same vrf as the one where the policy is attached.
	VirtualRouter string `protobuf:"bytes,2,opt,name=VirtualRouter,json=virtual-router,omitempty,proto3" json:"virtual-router,omitempty"`
}

func (m *DHCPServer) Reset()                    { *m = DHCPServer{} }
func (m *DHCPServer) String() string            { return proto.CompactTextString(m) }
func (*DHCPServer) ProtoMessage()               {}
func (*DHCPServer) Descriptor() ([]byte, []int) { return fileDescriptorIpam, []int{1} }

func (m *DHCPServer) GetIPAddress() string {
	if m != nil {
		return m.IPAddress
	}
	return ""
}

func (m *DHCPServer) GetVirtualRouter() string {
	if m != nil {
		return m.VirtualRouter
	}
	return ""
}

//
type IPAMPolicy struct {
	//
	api.TypeMeta `protobuf:"bytes,1,opt,name=T,json=,inline,embedded=T" json:",inline"`
	//
	api.ObjectMeta `protobuf:"bytes,2,opt,name=O,json=meta,omitempty,embedded=O" json:"meta,omitempty"`
	// Spec contains the configuration for the IPAM service.
	Spec IPAMPolicySpec `protobuf:"bytes,3,opt,name=Spec,json=spec,omitempty" json:"spec,omitempty"`
	// Status contains the current state of the IPAM service.
	Status IPAMPolicyStatus `protobuf:"bytes,4,opt,name=Status,json=status,omitempty" json:"status,omitempty"`
}

func (m *IPAMPolicy) Reset()                    { *m = IPAMPolicy{} }
func (m *IPAMPolicy) String() string            { return proto.CompactTextString(m) }
func (*IPAMPolicy) ProtoMessage()               {}
func (*IPAMPolicy) Descriptor() ([]byte, []int) { return fileDescriptorIpam, []int{2} }

func (m *IPAMPolicy) GetSpec() IPAMPolicySpec {
	if m != nil {
		return m.Spec
	}
	return IPAMPolicySpec{}
}

func (m *IPAMPolicy) GetStatus() IPAMPolicyStatus {
	if m != nil {
		return m.Status
	}
	return IPAMPolicyStatus{}
}

//
type IPAMPolicySpec struct {
	//
	Type string `protobuf:"bytes,1,opt,name=Type,json=type,omitempty,proto3" json:"type,omitempty"`
	//
	DHCPRelay *DHCPRelayPolicy `protobuf:"bytes,2,opt,name=DHCPRelay,json=dhcp-relay,omitempty" json:"dhcp-relay,omitempty"`
}

func (m *IPAMPolicySpec) Reset()                    { *m = IPAMPolicySpec{} }
func (m *IPAMPolicySpec) String() string            { return proto.CompactTextString(m) }
func (*IPAMPolicySpec) ProtoMessage()               {}
func (*IPAMPolicySpec) Descriptor() ([]byte, []int) { return fileDescriptorIpam, []int{3} }

func (m *IPAMPolicySpec) GetType() string {
	if m != nil {
		return m.Type
	}
	return ""
}

func (m *IPAMPolicySpec) GetDHCPRelay() *DHCPRelayPolicy {
	if m != nil {
		return m.DHCPRelay
	}
	return nil
}

//
type IPAMPolicyStatus struct {
	// The status of the configuration propagation to the Naples
	PropagationStatus security.PropagationStatus `protobuf:"bytes,1,opt,name=PropagationStatus,json=propagation-status" json:"propagation-status"`
}

func (m *IPAMPolicyStatus) Reset()                    { *m = IPAMPolicyStatus{} }
func (m *IPAMPolicyStatus) String() string            { return proto.CompactTextString(m) }
func (*IPAMPolicyStatus) ProtoMessage()               {}
func (*IPAMPolicyStatus) Descriptor() ([]byte, []int) { return fileDescriptorIpam, []int{4} }

func (m *IPAMPolicyStatus) GetPropagationStatus() security.PropagationStatus {
	if m != nil {
		return m.PropagationStatus
	}
	return security.PropagationStatus{}
}

func init() {
	proto.RegisterType((*DHCPRelayPolicy)(nil), "network.DHCPRelayPolicy")
	proto.RegisterType((*DHCPServer)(nil), "network.DHCPServer")
	proto.RegisterType((*IPAMPolicy)(nil), "network.IPAMPolicy")
	proto.RegisterType((*IPAMPolicySpec)(nil), "network.IPAMPolicySpec")
	proto.RegisterType((*IPAMPolicyStatus)(nil), "network.IPAMPolicyStatus")
	proto.RegisterEnum("network.IPAMPolicySpec_IPAMType", IPAMPolicySpec_IPAMType_name, IPAMPolicySpec_IPAMType_value)
}
func (m *DHCPRelayPolicy) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *DHCPRelayPolicy) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Servers) > 0 {
		for _, msg := range m.Servers {
			dAtA[i] = 0xa
			i++
			i = encodeVarintIpam(dAtA, i, uint64(msg.Size()))
			n, err := msg.MarshalTo(dAtA[i:])
			if err != nil {
				return 0, err
			}
			i += n
		}
	}
	return i, nil
}

func (m *DHCPServer) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *DHCPServer) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.IPAddress) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintIpam(dAtA, i, uint64(len(m.IPAddress)))
		i += copy(dAtA[i:], m.IPAddress)
	}
	if len(m.VirtualRouter) > 0 {
		dAtA[i] = 0x12
		i++
		i = encodeVarintIpam(dAtA, i, uint64(len(m.VirtualRouter)))
		i += copy(dAtA[i:], m.VirtualRouter)
	}
	return i, nil
}

func (m *IPAMPolicy) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *IPAMPolicy) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	dAtA[i] = 0xa
	i++
	i = encodeVarintIpam(dAtA, i, uint64(m.TypeMeta.Size()))
	n1, err := m.TypeMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n1
	dAtA[i] = 0x12
	i++
	i = encodeVarintIpam(dAtA, i, uint64(m.ObjectMeta.Size()))
	n2, err := m.ObjectMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n2
	dAtA[i] = 0x1a
	i++
	i = encodeVarintIpam(dAtA, i, uint64(m.Spec.Size()))
	n3, err := m.Spec.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n3
	dAtA[i] = 0x22
	i++
	i = encodeVarintIpam(dAtA, i, uint64(m.Status.Size()))
	n4, err := m.Status.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n4
	return i, nil
}

func (m *IPAMPolicySpec) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *IPAMPolicySpec) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Type) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintIpam(dAtA, i, uint64(len(m.Type)))
		i += copy(dAtA[i:], m.Type)
	}
	if m.DHCPRelay != nil {
		dAtA[i] = 0x12
		i++
		i = encodeVarintIpam(dAtA, i, uint64(m.DHCPRelay.Size()))
		n5, err := m.DHCPRelay.MarshalTo(dAtA[i:])
		if err != nil {
			return 0, err
		}
		i += n5
	}
	return i, nil
}

func (m *IPAMPolicyStatus) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *IPAMPolicyStatus) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	dAtA[i] = 0xa
	i++
	i = encodeVarintIpam(dAtA, i, uint64(m.PropagationStatus.Size()))
	n6, err := m.PropagationStatus.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n6
	return i, nil
}

func encodeVarintIpam(dAtA []byte, offset int, v uint64) int {
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return offset + 1
}
func (m *DHCPRelayPolicy) Size() (n int) {
	var l int
	_ = l
	if len(m.Servers) > 0 {
		for _, e := range m.Servers {
			l = e.Size()
			n += 1 + l + sovIpam(uint64(l))
		}
	}
	return n
}

func (m *DHCPServer) Size() (n int) {
	var l int
	_ = l
	l = len(m.IPAddress)
	if l > 0 {
		n += 1 + l + sovIpam(uint64(l))
	}
	l = len(m.VirtualRouter)
	if l > 0 {
		n += 1 + l + sovIpam(uint64(l))
	}
	return n
}

func (m *IPAMPolicy) Size() (n int) {
	var l int
	_ = l
	l = m.TypeMeta.Size()
	n += 1 + l + sovIpam(uint64(l))
	l = m.ObjectMeta.Size()
	n += 1 + l + sovIpam(uint64(l))
	l = m.Spec.Size()
	n += 1 + l + sovIpam(uint64(l))
	l = m.Status.Size()
	n += 1 + l + sovIpam(uint64(l))
	return n
}

func (m *IPAMPolicySpec) Size() (n int) {
	var l int
	_ = l
	l = len(m.Type)
	if l > 0 {
		n += 1 + l + sovIpam(uint64(l))
	}
	if m.DHCPRelay != nil {
		l = m.DHCPRelay.Size()
		n += 1 + l + sovIpam(uint64(l))
	}
	return n
}

func (m *IPAMPolicyStatus) Size() (n int) {
	var l int
	_ = l
	l = m.PropagationStatus.Size()
	n += 1 + l + sovIpam(uint64(l))
	return n
}

func sovIpam(x uint64) (n int) {
	for {
		n++
		x >>= 7
		if x == 0 {
			break
		}
	}
	return n
}
func sozIpam(x uint64) (n int) {
	return sovIpam(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *DHCPRelayPolicy) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowIpam
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
			return fmt.Errorf("proto: DHCPRelayPolicy: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: DHCPRelayPolicy: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Servers", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowIpam
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
				return ErrInvalidLengthIpam
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Servers = append(m.Servers, &DHCPServer{})
			if err := m.Servers[len(m.Servers)-1].Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipIpam(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthIpam
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
func (m *DHCPServer) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowIpam
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
			return fmt.Errorf("proto: DHCPServer: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: DHCPServer: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field IPAddress", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowIpam
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
				return ErrInvalidLengthIpam
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.IPAddress = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field VirtualRouter", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowIpam
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
				return ErrInvalidLengthIpam
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.VirtualRouter = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipIpam(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthIpam
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
func (m *IPAMPolicy) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowIpam
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
			return fmt.Errorf("proto: IPAMPolicy: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: IPAMPolicy: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field TypeMeta", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowIpam
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
				return ErrInvalidLengthIpam
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
					return ErrIntOverflowIpam
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
				return ErrInvalidLengthIpam
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
					return ErrIntOverflowIpam
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
				return ErrInvalidLengthIpam
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
					return ErrIntOverflowIpam
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
				return ErrInvalidLengthIpam
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
			skippy, err := skipIpam(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthIpam
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
func (m *IPAMPolicySpec) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowIpam
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
			return fmt.Errorf("proto: IPAMPolicySpec: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: IPAMPolicySpec: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Type", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowIpam
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
				return ErrInvalidLengthIpam
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Type = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field DHCPRelay", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowIpam
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
				return ErrInvalidLengthIpam
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if m.DHCPRelay == nil {
				m.DHCPRelay = &DHCPRelayPolicy{}
			}
			if err := m.DHCPRelay.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipIpam(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthIpam
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
func (m *IPAMPolicyStatus) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowIpam
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
			return fmt.Errorf("proto: IPAMPolicyStatus: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: IPAMPolicyStatus: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field PropagationStatus", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowIpam
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
				return ErrInvalidLengthIpam
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
			skippy, err := skipIpam(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthIpam
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
func skipIpam(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowIpam
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
					return 0, ErrIntOverflowIpam
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
					return 0, ErrIntOverflowIpam
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
				return 0, ErrInvalidLengthIpam
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowIpam
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
				next, err := skipIpam(dAtA[start:])
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
	ErrInvalidLengthIpam = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowIpam   = fmt.Errorf("proto: integer overflow")
)

func init() { proto.RegisterFile("ipam.proto", fileDescriptorIpam) }

var fileDescriptorIpam = []byte{
	// 689 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x7c, 0x54, 0x4f, 0x4f, 0x13, 0x4f,
	0x18, 0x66, 0x81, 0xf0, 0x67, 0xfa, 0xa3, 0xf4, 0xb7, 0xa8, 0x6c, 0x0b, 0x69, 0xb1, 0xd1, 0xa4,
	0x1a, 0xba, 0x6b, 0x30, 0xe1, 0xe0, 0x8d, 0x2a, 0x46, 0x0e, 0x95, 0xa6, 0xad, 0x1e, 0xf4, 0x60,
	0xa6, 0xdb, 0x97, 0x65, 0x74, 0x77, 0x67, 0x32, 0x33, 0x0b, 0xa9, 0xc6, 0xa3, 0xc4, 0xcf, 0xe1,
	0x91, 0x4f, 0xc2, 0x4d, 0x62, 0x3c, 0x37, 0xa6, 0x27, 0xd3, 0x8b, 0x5f, 0xc1, 0xcc, 0xec, 0x42,
	0xb7, 0x50, 0xbc, 0xed, 0xfb, 0xbc, 0xcf, 0xf3, 0xcc, 0x3b, 0x33, 0xcf, 0x2c, 0x42, 0x84, 0xe1,
	0xc0, 0x66, 0x9c, 0x4a, 0x6a, 0xce, 0x87, 0x20, 0x8f, 0x29, 0xff, 0x50, 0x58, 0xf7, 0x28, 0xf5,
	0x7c, 0x70, 0x30, 0x23, 0x0e, 0x0e, 0x43, 0x2a, 0xb1, 0x24, 0x34, 0x14, 0x31, 0xad, 0xb0, 0xeb,
	0x11, 0x79, 0x18, 0x75, 0x6c, 0x97, 0x06, 0x0e, 0x83, 0x50, 0xe0, 0xb0, 0x4b, 0x1d, 0x71, 0xec,
	0x1c, 0x41, 0x48, 0x5c, 0x70, 0x22, 0x49, 0x7c, 0xa1, 0xa4, 0x1e, 0x84, 0x69, 0xb5, 0x43, 0x42,
	0xd7, 0x8f, 0xba, 0x70, 0x61, 0x53, 0x4d, 0xd9, 0x78, 0xd4, 0xa3, 0x8e, 0x86, 0x3b, 0xd1, 0x81,
	0xae, 0x74, 0xa1, 0xbf, 0x12, 0xfa, 0xfd, 0x1b, 0x56, 0x55, 0x33, 0x06, 0x20, 0x71, 0x42, 0x7b,
	0xf4, 0x0f, 0x9a, 0x8f, 0x3b, 0xe0, 0x0b, 0x47, 0x80, 0x0f, 0xae, 0xa4, 0x3c, 0x51, 0xfc, 0x27,
	0x21, 0xc4, 0xa1, 0x4c, 0xaa, 0xac, 0xf0, 0x18, 0xf5, 0x89, 0xdb, 0x8b, 0xeb, 0x72, 0x80, 0x96,
	0x9f, 0xbd, 0x78, 0xda, 0x68, 0x82, 0x8f, 0x7b, 0x0d, 0xdd, 0x30, 0xdf, 0xa0, 0xf9, 0x16, 0xf0,
	0x23, 0xe0, 0xc2, 0x32, 0x36, 0x66, 0x2a, 0x99, 0xad, 0x15, 0x3b, 0x39, 0x38, 0x5b, 0x51, 0xe3,
	0x5e, 0x6d, 0x6d, 0xd8, 0x2f, 0xad, 0x72, 0x25, 0xab, 0x8a, 0x98, 0xbd, 0x49, 0x03, 0x22, 0x21,
	0x60, 0xb2, 0xd7, 0xbc, 0xa9, 0x51, 0xfe, 0x6e, 0x20, 0x34, 0x32, 0x31, 0xeb, 0x68, 0x71, 0xaf,
	0xb1, 0xd3, 0xed, 0x72, 0x10, 0x6a, 0x31, 0xa3, 0xb2, 0x58, 0xbb, 0x77, 0x7a, 0x92, 0x5f, 0x88,
	0xc1, 0xca, 0x83, 0x61, 0xbf, 0x74, 0x8b, 0xb0, 0x2a, 0x8e, 0x19, 0xa9, 0x05, 0x26, 0xa2, 0x26,
	0x47, 0x4b, 0xaf, 0x09, 0x97, 0x11, 0xf6, 0x9b, 0x34, 0x92, 0xc0, 0xad, 0x69, 0x6d, 0xb9, 0xf3,
	0xed, 0x24, 0x7f, 0x17, 0x2d, 0xbc, 0xc4, 0x01, 0x74, 0x9b, 0x70, 0x60, 0xde, 0x4e, 0xb6, 0xe3,
	0x8c, 0x91, 0x87, 0xfd, 0x92, 0x75, 0x14, 0x03, 0x55, 0xae, 0x91, 0xd4, 0x7a, 0x37, 0x76, 0xca,
	0x3f, 0xa7, 0x11, 0xda, 0x6b, 0xec, 0xd4, 0x93, 0xc3, 0xdb, 0x46, 0x46, 0x5b, 0xef, 0x24, 0xb3,
	0xb5, 0x64, 0x63, 0x46, 0xec, 0x76, 0x8f, 0x41, 0x1d, 0x24, 0xae, 0xad, 0x9c, 0xf5, 0x4b, 0x53,
	0xe7, 0xfd, 0x92, 0x31, 0xec, 0x97, 0xe6, 0x37, 0x49, 0xe8, 0x93, 0x10, 0x9a, 0x17, 0x1f, 0xe6,
	0x73, 0x64, 0xec, 0xeb, 0x71, 0x33, 0x5b, 0xcb, 0x5a, 0xb7, 0xdf, 0x79, 0x0f, 0xae, 0xd4, 0xca,
	0x42, 0x4a, 0x99, 0x55, 0x59, 0x48, 0x0d, 0x76, 0xa5, 0x36, 0xeb, 0x68, 0xb6, 0xc5, 0xc0, 0xb5,
	0x66, 0xb4, 0xd5, 0xea, 0xe5, 0xcd, 0x8d, 0x46, 0x54, 0xed, 0xda, 0x1d, 0x65, 0xa9, 0xec, 0x04,
	0x03, 0x37, 0x6d, 0x37, 0x5e, 0x9b, 0xaf, 0xd0, 0x5c, 0x4b, 0x62, 0x19, 0x09, 0x6b, 0x56, 0x1b,
	0xe6, 0x27, 0x19, 0x6a, 0x42, 0xcd, 0x4a, 0x2c, 0x73, 0x42, 0xd7, 0x29, 0xd3, 0x6b, 0xc8, 0x93,
	0x8d, 0x1f, 0x5f, 0xf2, 0xeb, 0x28, 0xe3, 0x7c, 0xda, 0xb7, 0xdb, 0x3a, 0x9d, 0x9f, 0xcd, 0x25,
	0xf5, 0x50, 0xab, 0x3a, 0x9b, 0x04, 0x44, 0xf9, 0x8f, 0x81, 0xb2, 0xe3, 0x33, 0x9b, 0x6d, 0x34,
	0xab, 0x0e, 0x33, 0xc9, 0xc9, 0xf6, 0xe9, 0x49, 0x7e, 0xa3, 0x25, 0xf9, 0x6e, 0x18, 0x05, 0x95,
	0x71, 0xa6, 0x9e, 0x4d, 0x51, 0x55, 0x7e, 0xb2, 0xb2, 0xc7, 0x20, 0xbd, 0xc3, 0xf1, 0xda, 0x7c,
	0x8b, 0x16, 0x2f, 0x1f, 0x40, 0x72, 0x01, 0xd6, 0x58, 0xde, 0x53, 0x4f, 0xa3, 0x66, 0xa9, 0x40,
	0x76, 0x0f, 0x5d, 0x56, 0xd5, 0x01, 0x4f, 0x07, 0x72, 0x12, 0x5a, 0x7e, 0x88, 0x16, 0x2e, 0x66,
	0x31, 0x8b, 0x71, 0xf2, 0xdf, 0x69, 0xbf, 0xdc, 0x54, 0x21, 0x3b, 0xf8, 0x9a, 0x47, 0x23, 0x55,
	0xf9, 0x23, 0xca, 0x5d, 0x3d, 0x53, 0xf3, 0x00, 0xfd, 0xdf, 0xe0, 0x94, 0x61, 0x4f, 0xff, 0x62,
	0x92, 0x9b, 0x88, 0xd3, 0xb5, 0x66, 0x0b, 0x70, 0x23, 0x4e, 0x64, 0xcf, 0xbe, 0x46, 0x89, 0x13,
	0x33, 0xec, 0x97, 0x4c, 0x36, 0x6a, 0x55, 0xe3, 0x5b, 0x68, 0x4e, 0xc0, 0x6a, 0xb9, 0xb3, 0x41,
	0xd1, 0x38, 0x1f, 0x14, 0x8d, 0x5f, 0x83, 0xa2, 0xf1, 0x7b, 0x50, 0x9c, 0x6a, 0x18, 0x9d, 0x39,
	0xfd, 0x83, 0x78, 0xfc, 0x37, 0x00, 0x00, 0xff, 0xff, 0x12, 0x5e, 0x32, 0x80, 0x42, 0x05, 0x00,
	0x00,
}
