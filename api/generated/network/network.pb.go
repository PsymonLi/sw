// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: network.proto

package network

import proto "github.com/gogo/protobuf/proto"
import fmt "fmt"
import math "math"
import _ "github.com/pensando/grpc-gateway/third_party/googleapis/google/api"
import _ "github.com/pensando/sw/venice/utils/apigen/annotations"
import _ "github.com/gogo/protobuf/gogoproto"
import api "github.com/pensando/sw/api"
import _ "github.com/pensando/sw/api/labels"
import _ "github.com/pensando/sw/api/generated/orchestration"
import _ "github.com/pensando/sw/api/generated/cluster"
import _ "github.com/pensando/sw/api/generated/cluster"

import io "io"

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

//
type NetworkType int32

const (
	//
	NetworkType_Bridged NetworkType = 0
	//
	NetworkType_Routed NetworkType = 1
)

var NetworkType_name = map[int32]string{
	0: "Bridged",
	1: "Routed",
}
var NetworkType_value = map[string]int32{
	"Bridged": 0,
	"Routed":  1,
}

func (NetworkType) EnumDescriptor() ([]byte, []int) { return fileDescriptorNetwork, []int{0} }

// Network represents a subnet
type Network struct {
	//
	api.TypeMeta `protobuf:"bytes,1,opt,name=T,json=,inline,embedded=T" json:",inline"`
	//
	api.ObjectMeta `protobuf:"bytes,2,opt,name=O,json=meta,omitempty,embedded=O" json:"meta,omitempty"`
	// Spec contains the configuration of the network.
	Spec NetworkSpec `protobuf:"bytes,3,opt,name=Spec,json=spec,omitempty" json:"spec,omitempty"`
	// Status contains the current state of the network.
	Status NetworkStatus `protobuf:"bytes,4,opt,name=Status,json=status,omitempty" json:"status,omitempty"`
}

func (m *Network) Reset()                    { *m = Network{} }
func (m *Network) String() string            { return proto.CompactTextString(m) }
func (*Network) ProtoMessage()               {}
func (*Network) Descriptor() ([]byte, []int) { return fileDescriptorNetwork, []int{0} }

func (m *Network) GetSpec() NetworkSpec {
	if m != nil {
		return m.Spec
	}
	return NetworkSpec{}
}

func (m *Network) GetStatus() NetworkStatus {
	if m != nil {
		return m.Status
	}
	return NetworkStatus{}
}

// spec part of network object
type NetworkSpec struct {
	// type of network. (vlan/vxlan/routed etc)
	Type string `protobuf:"bytes,1,opt,name=Type,json=type,omitempty,proto3" json:"type,omitempty"`
	// IPv4 subnet CIDR
	IPv4Subnet string `protobuf:"bytes,2,opt,name=IPv4Subnet,json=ipv4-subnet,omitempty,proto3" json:"ipv4-subnet,omitempty"`
	// IPv4 gateway for this subnet
	IPv4Gateway string `protobuf:"bytes,3,opt,name=IPv4Gateway,json=ipv4-gateway,omitempty,proto3" json:"ipv4-gateway,omitempty"`
	// IPv6 subnet CIDR
	IPv6Subnet string `protobuf:"bytes,4,opt,name=IPv6Subnet,json=ipv6-subnet,omitempty,proto3" json:"ipv6-subnet,omitempty"`
	// IPv6 gateway
	IPv6Gateway string `protobuf:"bytes,5,opt,name=IPv6Gateway,json=ipv6-gateway,omitempty,proto3" json:"ipv6-gateway,omitempty"`
	// Vlan ID for the network
	VlanID uint32 `protobuf:"varint,6,opt,name=VlanID,json=vlan-id,omitempty,proto3" json:"vlan-id,omitempty"`
	// Vxlan VNI for the network
	VxlanVNI uint32 `protobuf:"varint,7,opt,name=VxlanVNI,json=vxlan-vni,omitempty,proto3" json:"vxlan-vni,omitempty"`
	// VirtualRouter specifies the VRF this network belongs to
	VirtualRouter string `protobuf:"bytes,8,opt,name=VirtualRouter,json=virtual-router,omitempty,proto3" json:"virtual-router,omitempty"`
	// Relay Configuration if any
	IPAMPolicy string `protobuf:"bytes,9,opt,name=IPAMPolicy,json=ipam-policy,omitempty,proto3" json:"ipam-policy,omitempty"`
	// RouteImportExport specifies what routes will be imported to this Router and how routes are tagged when exported.
	RouteImportExport *RDSpec `protobuf:"bytes,10,opt,name=RouteImportExport,json=route-imoport-export,omitempty" json:"route-imoport-export,omitempty"`
	// If supplied, this network will only be applied to the orchestrators specified.
	Orchestrators []*OrchestratorInfo `protobuf:"bytes,11,rep,name=Orchestrators,json=orchestrators,omitempty" json:"orchestrators,omitempty"`
}

func (m *NetworkSpec) Reset()                    { *m = NetworkSpec{} }
func (m *NetworkSpec) String() string            { return proto.CompactTextString(m) }
func (*NetworkSpec) ProtoMessage()               {}
func (*NetworkSpec) Descriptor() ([]byte, []int) { return fileDescriptorNetwork, []int{1} }

func (m *NetworkSpec) GetType() string {
	if m != nil {
		return m.Type
	}
	return ""
}

func (m *NetworkSpec) GetIPv4Subnet() string {
	if m != nil {
		return m.IPv4Subnet
	}
	return ""
}

func (m *NetworkSpec) GetIPv4Gateway() string {
	if m != nil {
		return m.IPv4Gateway
	}
	return ""
}

func (m *NetworkSpec) GetIPv6Subnet() string {
	if m != nil {
		return m.IPv6Subnet
	}
	return ""
}

func (m *NetworkSpec) GetIPv6Gateway() string {
	if m != nil {
		return m.IPv6Gateway
	}
	return ""
}

func (m *NetworkSpec) GetVlanID() uint32 {
	if m != nil {
		return m.VlanID
	}
	return 0
}

func (m *NetworkSpec) GetVxlanVNI() uint32 {
	if m != nil {
		return m.VxlanVNI
	}
	return 0
}

func (m *NetworkSpec) GetVirtualRouter() string {
	if m != nil {
		return m.VirtualRouter
	}
	return ""
}

func (m *NetworkSpec) GetIPAMPolicy() string {
	if m != nil {
		return m.IPAMPolicy
	}
	return ""
}

func (m *NetworkSpec) GetRouteImportExport() *RDSpec {
	if m != nil {
		return m.RouteImportExport
	}
	return nil
}

func (m *NetworkSpec) GetOrchestrators() []*OrchestratorInfo {
	if m != nil {
		return m.Orchestrators
	}
	return nil
}

// status part of network object
type NetworkStatus struct {
	// list of all workloads in this network
	Workloads []string `protobuf:"bytes,1,rep,name=Workloads,json=workloads,omitempty" json:"workloads,omitempty"`
	// allocated IPv4 addresses (bitmap)
	AllocatedIPv4Addrs []byte `protobuf:"bytes,2,opt,name=AllocatedIPv4Addrs,json=allocated-ipv4-addrs,omitempty,proto3" json:"allocated-ipv4-addrs,omitempty" venice:"sskip"`
	// Handle is the internal Handle allocated to this network
	Handle uint64 `protobuf:"varint,3,opt,name=Handle,json=id,omitempty,proto3" json:"id,omitempty"`
}

func (m *NetworkStatus) Reset()                    { *m = NetworkStatus{} }
func (m *NetworkStatus) String() string            { return proto.CompactTextString(m) }
func (*NetworkStatus) ProtoMessage()               {}
func (*NetworkStatus) Descriptor() ([]byte, []int) { return fileDescriptorNetwork, []int{2} }

func (m *NetworkStatus) GetWorkloads() []string {
	if m != nil {
		return m.Workloads
	}
	return nil
}

func (m *NetworkStatus) GetAllocatedIPv4Addrs() []byte {
	if m != nil {
		return m.AllocatedIPv4Addrs
	}
	return nil
}

func (m *NetworkStatus) GetHandle() uint64 {
	if m != nil {
		return m.Handle
	}
	return 0
}

//
type OrchestratorInfo struct {
	// Name of Orchestrator object to which this network should be applied to
	Name string `protobuf:"bytes,1,opt,name=Name,json=orchestrator-name, omitempty,proto3" json:"orchestrator-name, omitempty"`
	// Namespace in the orchestrator in which this network should be created in.
	Namespace string `protobuf:"bytes,2,opt,name=Namespace,json=namespace, omitempty,proto3" json:"namespace, omitempty"`
}

func (m *OrchestratorInfo) Reset()                    { *m = OrchestratorInfo{} }
func (m *OrchestratorInfo) String() string            { return proto.CompactTextString(m) }
func (*OrchestratorInfo) ProtoMessage()               {}
func (*OrchestratorInfo) Descriptor() ([]byte, []int) { return fileDescriptorNetwork, []int{3} }

func (m *OrchestratorInfo) GetName() string {
	if m != nil {
		return m.Name
	}
	return ""
}

func (m *OrchestratorInfo) GetNamespace() string {
	if m != nil {
		return m.Namespace
	}
	return ""
}

func init() {
	proto.RegisterType((*Network)(nil), "network.Network")
	proto.RegisterType((*NetworkSpec)(nil), "network.NetworkSpec")
	proto.RegisterType((*NetworkStatus)(nil), "network.NetworkStatus")
	proto.RegisterType((*OrchestratorInfo)(nil), "network.OrchestratorInfo")
	proto.RegisterEnum("network.NetworkType", NetworkType_name, NetworkType_value)
}
func (m *Network) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *Network) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	dAtA[i] = 0xa
	i++
	i = encodeVarintNetwork(dAtA, i, uint64(m.TypeMeta.Size()))
	n1, err := m.TypeMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n1
	dAtA[i] = 0x12
	i++
	i = encodeVarintNetwork(dAtA, i, uint64(m.ObjectMeta.Size()))
	n2, err := m.ObjectMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n2
	dAtA[i] = 0x1a
	i++
	i = encodeVarintNetwork(dAtA, i, uint64(m.Spec.Size()))
	n3, err := m.Spec.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n3
	dAtA[i] = 0x22
	i++
	i = encodeVarintNetwork(dAtA, i, uint64(m.Status.Size()))
	n4, err := m.Status.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n4
	return i, nil
}

func (m *NetworkSpec) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *NetworkSpec) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Type) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintNetwork(dAtA, i, uint64(len(m.Type)))
		i += copy(dAtA[i:], m.Type)
	}
	if len(m.IPv4Subnet) > 0 {
		dAtA[i] = 0x12
		i++
		i = encodeVarintNetwork(dAtA, i, uint64(len(m.IPv4Subnet)))
		i += copy(dAtA[i:], m.IPv4Subnet)
	}
	if len(m.IPv4Gateway) > 0 {
		dAtA[i] = 0x1a
		i++
		i = encodeVarintNetwork(dAtA, i, uint64(len(m.IPv4Gateway)))
		i += copy(dAtA[i:], m.IPv4Gateway)
	}
	if len(m.IPv6Subnet) > 0 {
		dAtA[i] = 0x22
		i++
		i = encodeVarintNetwork(dAtA, i, uint64(len(m.IPv6Subnet)))
		i += copy(dAtA[i:], m.IPv6Subnet)
	}
	if len(m.IPv6Gateway) > 0 {
		dAtA[i] = 0x2a
		i++
		i = encodeVarintNetwork(dAtA, i, uint64(len(m.IPv6Gateway)))
		i += copy(dAtA[i:], m.IPv6Gateway)
	}
	if m.VlanID != 0 {
		dAtA[i] = 0x30
		i++
		i = encodeVarintNetwork(dAtA, i, uint64(m.VlanID))
	}
	if m.VxlanVNI != 0 {
		dAtA[i] = 0x38
		i++
		i = encodeVarintNetwork(dAtA, i, uint64(m.VxlanVNI))
	}
	if len(m.VirtualRouter) > 0 {
		dAtA[i] = 0x42
		i++
		i = encodeVarintNetwork(dAtA, i, uint64(len(m.VirtualRouter)))
		i += copy(dAtA[i:], m.VirtualRouter)
	}
	if len(m.IPAMPolicy) > 0 {
		dAtA[i] = 0x4a
		i++
		i = encodeVarintNetwork(dAtA, i, uint64(len(m.IPAMPolicy)))
		i += copy(dAtA[i:], m.IPAMPolicy)
	}
	if m.RouteImportExport != nil {
		dAtA[i] = 0x52
		i++
		i = encodeVarintNetwork(dAtA, i, uint64(m.RouteImportExport.Size()))
		n5, err := m.RouteImportExport.MarshalTo(dAtA[i:])
		if err != nil {
			return 0, err
		}
		i += n5
	}
	if len(m.Orchestrators) > 0 {
		for _, msg := range m.Orchestrators {
			dAtA[i] = 0x5a
			i++
			i = encodeVarintNetwork(dAtA, i, uint64(msg.Size()))
			n, err := msg.MarshalTo(dAtA[i:])
			if err != nil {
				return 0, err
			}
			i += n
		}
	}
	return i, nil
}

func (m *NetworkStatus) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *NetworkStatus) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Workloads) > 0 {
		for _, s := range m.Workloads {
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
	if len(m.AllocatedIPv4Addrs) > 0 {
		dAtA[i] = 0x12
		i++
		i = encodeVarintNetwork(dAtA, i, uint64(len(m.AllocatedIPv4Addrs)))
		i += copy(dAtA[i:], m.AllocatedIPv4Addrs)
	}
	if m.Handle != 0 {
		dAtA[i] = 0x18
		i++
		i = encodeVarintNetwork(dAtA, i, uint64(m.Handle))
	}
	return i, nil
}

func (m *OrchestratorInfo) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *OrchestratorInfo) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Name) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintNetwork(dAtA, i, uint64(len(m.Name)))
		i += copy(dAtA[i:], m.Name)
	}
	if len(m.Namespace) > 0 {
		dAtA[i] = 0x12
		i++
		i = encodeVarintNetwork(dAtA, i, uint64(len(m.Namespace)))
		i += copy(dAtA[i:], m.Namespace)
	}
	return i, nil
}

func encodeVarintNetwork(dAtA []byte, offset int, v uint64) int {
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return offset + 1
}
func (m *Network) Size() (n int) {
	var l int
	_ = l
	l = m.TypeMeta.Size()
	n += 1 + l + sovNetwork(uint64(l))
	l = m.ObjectMeta.Size()
	n += 1 + l + sovNetwork(uint64(l))
	l = m.Spec.Size()
	n += 1 + l + sovNetwork(uint64(l))
	l = m.Status.Size()
	n += 1 + l + sovNetwork(uint64(l))
	return n
}

func (m *NetworkSpec) Size() (n int) {
	var l int
	_ = l
	l = len(m.Type)
	if l > 0 {
		n += 1 + l + sovNetwork(uint64(l))
	}
	l = len(m.IPv4Subnet)
	if l > 0 {
		n += 1 + l + sovNetwork(uint64(l))
	}
	l = len(m.IPv4Gateway)
	if l > 0 {
		n += 1 + l + sovNetwork(uint64(l))
	}
	l = len(m.IPv6Subnet)
	if l > 0 {
		n += 1 + l + sovNetwork(uint64(l))
	}
	l = len(m.IPv6Gateway)
	if l > 0 {
		n += 1 + l + sovNetwork(uint64(l))
	}
	if m.VlanID != 0 {
		n += 1 + sovNetwork(uint64(m.VlanID))
	}
	if m.VxlanVNI != 0 {
		n += 1 + sovNetwork(uint64(m.VxlanVNI))
	}
	l = len(m.VirtualRouter)
	if l > 0 {
		n += 1 + l + sovNetwork(uint64(l))
	}
	l = len(m.IPAMPolicy)
	if l > 0 {
		n += 1 + l + sovNetwork(uint64(l))
	}
	if m.RouteImportExport != nil {
		l = m.RouteImportExport.Size()
		n += 1 + l + sovNetwork(uint64(l))
	}
	if len(m.Orchestrators) > 0 {
		for _, e := range m.Orchestrators {
			l = e.Size()
			n += 1 + l + sovNetwork(uint64(l))
		}
	}
	return n
}

func (m *NetworkStatus) Size() (n int) {
	var l int
	_ = l
	if len(m.Workloads) > 0 {
		for _, s := range m.Workloads {
			l = len(s)
			n += 1 + l + sovNetwork(uint64(l))
		}
	}
	l = len(m.AllocatedIPv4Addrs)
	if l > 0 {
		n += 1 + l + sovNetwork(uint64(l))
	}
	if m.Handle != 0 {
		n += 1 + sovNetwork(uint64(m.Handle))
	}
	return n
}

func (m *OrchestratorInfo) Size() (n int) {
	var l int
	_ = l
	l = len(m.Name)
	if l > 0 {
		n += 1 + l + sovNetwork(uint64(l))
	}
	l = len(m.Namespace)
	if l > 0 {
		n += 1 + l + sovNetwork(uint64(l))
	}
	return n
}

func sovNetwork(x uint64) (n int) {
	for {
		n++
		x >>= 7
		if x == 0 {
			break
		}
	}
	return n
}
func sozNetwork(x uint64) (n int) {
	return sovNetwork(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *Network) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowNetwork
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
			return fmt.Errorf("proto: Network: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: Network: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field TypeMeta", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
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
				return ErrInvalidLengthNetwork
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
					return ErrIntOverflowNetwork
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
				return ErrInvalidLengthNetwork
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
					return ErrIntOverflowNetwork
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
				return ErrInvalidLengthNetwork
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
					return ErrIntOverflowNetwork
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
				return ErrInvalidLengthNetwork
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
			skippy, err := skipNetwork(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthNetwork
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
func (m *NetworkSpec) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowNetwork
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
			return fmt.Errorf("proto: NetworkSpec: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: NetworkSpec: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Type", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
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
				return ErrInvalidLengthNetwork
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Type = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field IPv4Subnet", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
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
				return ErrInvalidLengthNetwork
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.IPv4Subnet = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 3:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field IPv4Gateway", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
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
				return ErrInvalidLengthNetwork
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.IPv4Gateway = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 4:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field IPv6Subnet", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
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
				return ErrInvalidLengthNetwork
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.IPv6Subnet = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 5:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field IPv6Gateway", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
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
				return ErrInvalidLengthNetwork
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.IPv6Gateway = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 6:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field VlanID", wireType)
			}
			m.VlanID = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.VlanID |= (uint32(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 7:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field VxlanVNI", wireType)
			}
			m.VxlanVNI = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.VxlanVNI |= (uint32(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 8:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field VirtualRouter", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
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
				return ErrInvalidLengthNetwork
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.VirtualRouter = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 9:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field IPAMPolicy", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
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
				return ErrInvalidLengthNetwork
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.IPAMPolicy = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 10:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field RouteImportExport", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
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
				return ErrInvalidLengthNetwork
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if m.RouteImportExport == nil {
				m.RouteImportExport = &RDSpec{}
			}
			if err := m.RouteImportExport.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		case 11:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Orchestrators", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
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
				return ErrInvalidLengthNetwork
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Orchestrators = append(m.Orchestrators, &OrchestratorInfo{})
			if err := m.Orchestrators[len(m.Orchestrators)-1].Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipNetwork(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthNetwork
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
func (m *NetworkStatus) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowNetwork
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
			return fmt.Errorf("proto: NetworkStatus: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: NetworkStatus: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Workloads", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
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
				return ErrInvalidLengthNetwork
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Workloads = append(m.Workloads, string(dAtA[iNdEx:postIndex]))
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field AllocatedIPv4Addrs", wireType)
			}
			var byteLen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				byteLen |= (int(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			if byteLen < 0 {
				return ErrInvalidLengthNetwork
			}
			postIndex := iNdEx + byteLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.AllocatedIPv4Addrs = append(m.AllocatedIPv4Addrs[:0], dAtA[iNdEx:postIndex]...)
			if m.AllocatedIPv4Addrs == nil {
				m.AllocatedIPv4Addrs = []byte{}
			}
			iNdEx = postIndex
		case 3:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field Handle", wireType)
			}
			m.Handle = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.Handle |= (uint64(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		default:
			iNdEx = preIndex
			skippy, err := skipNetwork(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthNetwork
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
func (m *OrchestratorInfo) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowNetwork
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
			return fmt.Errorf("proto: OrchestratorInfo: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: OrchestratorInfo: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Name", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
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
				return ErrInvalidLengthNetwork
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Name = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Namespace", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowNetwork
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
				return ErrInvalidLengthNetwork
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Namespace = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipNetwork(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthNetwork
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
func skipNetwork(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowNetwork
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
					return 0, ErrIntOverflowNetwork
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
					return 0, ErrIntOverflowNetwork
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
				return 0, ErrInvalidLengthNetwork
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowNetwork
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
				next, err := skipNetwork(dAtA[start:])
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
	ErrInvalidLengthNetwork = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowNetwork   = fmt.Errorf("proto: integer overflow")
)

func init() { proto.RegisterFile("network.proto", fileDescriptorNetwork) }

var fileDescriptorNetwork = []byte{
	// 966 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x84, 0x55, 0x4f, 0x6f, 0x1b, 0x45,
	0x14, 0xcf, 0x36, 0xc6, 0x8e, 0xc7, 0x71, 0x70, 0xa7, 0x69, 0xb2, 0x4e, 0x8b, 0x37, 0x58, 0xa2,
	0x32, 0x28, 0xeb, 0x45, 0xa5, 0x58, 0xa8, 0x20, 0x21, 0xaf, 0x1a, 0xa8, 0x0b, 0x4d, 0x22, 0x27,
	0x0a, 0xe7, 0xf1, 0xee, 0xc4, 0x9d, 0x66, 0x77, 0x66, 0xb5, 0x33, 0x76, 0x6a, 0x21, 0x8e, 0xe4,
	0x83, 0xe4, 0xc8, 0xd7, 0x40, 0x42, 0xb9, 0x20, 0x55, 0x7c, 0x00, 0x0b, 0xe5, 0x84, 0x7c, 0xe4,
	0xc0, 0x19, 0xcd, 0x78, 0x9c, 0x8c, 0x6b, 0x3b, 0xbd, 0x58, 0x7e, 0x7f, 0x7e, 0xbf, 0xf7, 0xdb,
	0x37, 0xef, 0xcd, 0x80, 0x22, 0xc5, 0xe2, 0x8c, 0xa5, 0xa7, 0xf5, 0x24, 0x65, 0x82, 0xc1, 0x9c,
	0x36, 0xb7, 0x1e, 0x76, 0x19, 0xeb, 0x46, 0xd8, 0x43, 0x09, 0xf1, 0x10, 0xa5, 0x4c, 0x20, 0x41,
	0x18, 0xe5, 0xe3, 0xb4, 0xad, 0xdd, 0x2e, 0x11, 0xaf, 0x7a, 0x9d, 0x7a, 0xc0, 0x62, 0x2f, 0xc1,
	0x94, 0x23, 0x1a, 0x32, 0x8f, 0x9f, 0x79, 0x7d, 0x4c, 0x49, 0x80, 0xbd, 0x9e, 0x20, 0x11, 0x97,
	0xd0, 0x2e, 0xa6, 0x26, 0xda, 0x23, 0x34, 0x88, 0x7a, 0x21, 0x9e, 0xd0, 0xb8, 0x06, 0x4d, 0x97,
	0x75, 0x99, 0xa7, 0xdc, 0x9d, 0xde, 0x89, 0xb2, 0x94, 0xa1, 0xfe, 0xe9, 0xf4, 0x4f, 0x16, 0x54,
	0x95, 0x1a, 0x63, 0x2c, 0x90, 0x4e, 0xfb, 0xfc, 0x96, 0xb4, 0x08, 0x75, 0x70, 0xc4, 0x3d, 0x8e,
	0x23, 0x1c, 0x08, 0x96, 0x6a, 0xc4, 0x97, 0xb7, 0x20, 0x54, 0x06, 0xf7, 0x58, 0x1a, 0xbc, 0xc2,
	0x5c, 0xa4, 0xea, 0x43, 0x34, 0x6c, 0x55, 0x60, 0x8a, 0xa8, 0xd0, 0x56, 0x21, 0x65, 0x3d, 0x81,
	0xb5, 0xb1, 0xc6, 0x63, 0x94, 0x0a, 0x4a, 0x82, 0xb1, 0x5d, 0xfd, 0xe3, 0x0e, 0xc8, 0xed, 0x8d,
	0x5b, 0x0b, 0x1b, 0xc0, 0x3a, 0xb2, 0xad, 0x6d, 0xab, 0x56, 0x78, 0x5c, 0xac, 0xa3, 0x84, 0xd4,
	0x8f, 0x06, 0x09, 0x7e, 0x89, 0x05, 0xf2, 0xef, 0x5d, 0x0e, 0x9d, 0xa5, 0xb7, 0x43, 0xc7, 0x1a,
	0x0d, 0x9d, 0xdc, 0x0e, 0xa1, 0x11, 0xa1, 0xb8, 0x3d, 0xf9, 0x03, 0xbf, 0x03, 0xd6, 0xbe, 0x7d,
	0x47, 0xe1, 0x3e, 0x54, 0xb8, 0xfd, 0xce, 0x6b, 0x1c, 0x08, 0x85, 0xdc, 0x32, 0x90, 0x6b, 0xb2,
	0x17, 0x3b, 0x2c, 0x26, 0x02, 0xc7, 0x89, 0x18, 0xb4, 0xdf, 0xb1, 0xe1, 0x0b, 0x90, 0x39, 0x4c,
	0x70, 0x60, 0x2f, 0x2b, 0xaa, 0xf5, 0xfa, 0x64, 0x02, 0xb4, 0x3e, 0x19, 0xf3, 0x37, 0x24, 0x9f,
	0xe4, 0xe2, 0x09, 0x0e, 0x4c, 0xae, 0x69, 0x1b, 0xb6, 0x41, 0xf6, 0x50, 0x20, 0xd1, 0xe3, 0x76,
	0x46, 0xb1, 0x6d, 0xcc, 0xb0, 0xa9, 0xa8, 0x6f, 0x6b, 0xbe, 0x12, 0x57, 0xb6, 0xc1, 0x38, 0xe3,
	0x79, 0xfa, 0xe0, 0xaf, 0x5f, 0xcb, 0x9b, 0xa0, 0xe0, 0xfd, 0xbc, 0x5f, 0x3f, 0x52, 0x0d, 0xfe,
	0x05, 0xae, 0x68, 0x56, 0x5e, 0xfd, 0x3d, 0x07, 0x0a, 0x86, 0x50, 0xf8, 0x23, 0xc8, 0xc8, 0xf6,
	0xa9, 0x7e, 0xe6, 0xfd, 0xfa, 0x6f, 0xe7, 0xe5, 0xf5, 0x43, 0x91, 0xee, 0xd2, 0x5e, 0x5c, 0xd3,
	0x69, 0x32, 0xfc, 0xe9, 0xa5, 0x6e, 0x8d, 0x18, 0x24, 0xd8, 0xfc, 0x9c, 0x69, 0x1b, 0x3e, 0x07,
	0xa0, 0x75, 0xd0, 0x7f, 0x72, 0xd8, 0xeb, 0x50, 0x2c, 0x54, 0xaf, 0xf3, 0xfe, 0x47, 0x1a, 0x7b,
	0x9f, 0x24, 0xfd, 0x27, 0x2e, 0x57, 0x21, 0x83, 0x62, 0xbe, 0x1b, 0xfe, 0x00, 0x0a, 0x92, 0xe9,
	0x7b, 0x24, 0xf0, 0x19, 0x1a, 0xa8, 0x5e, 0xe7, 0xfd, 0x8a, 0xa6, 0xda, 0x50, 0x98, 0xee, 0x38,
	0x66, 0x70, 0x2d, 0xf0, 0x6b, 0x59, 0x0d, 0x2d, 0x2b, 0x33, 0x23, 0xab, 0x31, 0x5f, 0x56, 0x63,
	0x91, 0xac, 0xc6, 0x44, 0xd6, 0x07, 0x33, 0xb2, 0x1a, 0x0b, 0x64, 0xcd, 0xf1, 0xc3, 0x6f, 0x40,
	0xf6, 0x38, 0x42, 0xb4, 0xf5, 0xcc, 0xce, 0x6e, 0x5b, 0xb5, 0xa2, 0x5f, 0xd6, 0x3c, 0x77, 0xfb,
	0x11, 0xa2, 0x2e, 0x09, 0x0d, 0x8a, 0x59, 0x17, 0xf4, 0xc1, 0xca, 0xf1, 0x9b, 0x08, 0xd1, 0xe3,
	0xbd, 0x96, 0x9d, 0x53, 0xf8, 0x07, 0x1a, 0x7f, 0xaf, 0x2f, 0xfd, 0x6e, 0x9f, 0x12, 0x83, 0x61,
	0x9e, 0x13, 0xa6, 0xa0, 0x78, 0x4c, 0x52, 0xd1, 0x43, 0x51, 0x5b, 0x2e, 0x5f, 0x6a, 0xaf, 0xa8,
	0x0f, 0x6a, 0x5e, 0x9c, 0x97, 0x3f, 0x06, 0x2b, 0x7b, 0x28, 0xc6, 0x61, 0x1b, 0x9f, 0xc0, 0xfb,
	0x7a, 0x7c, 0xbc, 0xa9, 0xe4, 0xd1, 0xd0, 0xb1, 0xfb, 0x63, 0x87, 0xab, 0x76, 0x37, 0x35, 0xca,
	0x2d, 0x8c, 0xc0, 0x13, 0x79, 0x18, 0xcd, 0x97, 0x07, 0x2c, 0x22, 0xc1, 0xc0, 0xce, 0xab, 0x82,
	0x5f, 0x5f, 0x9c, 0x97, 0x2b, 0x46, 0x41, 0x38, 0x29, 0x78, 0x93, 0x39, 0x3e, 0x2a, 0x14, 0xbb,
	0x89, 0x32, 0xa7, 0x8f, 0x6a, 0x8e, 0x1b, 0x52, 0x70, 0x57, 0xe9, 0x6c, 0xc5, 0x09, 0x4b, 0xc5,
	0xee, 0x1b, 0xf9, 0x6b, 0x03, 0xbd, 0xfe, 0x93, 0x2d, 0x6b, 0x3f, 0x53, 0xeb, 0x5a, 0x1d, 0x0d,
	0x9d, 0x8a, 0x52, 0xea, 0x92, 0x98, 0xc9, 0x4c, 0x17, 0x2b, 0x80, 0x51, 0xe6, 0x3d, 0x71, 0xf8,
	0x1a, 0x14, 0xf7, 0xaf, 0x2f, 0x39, 0x96, 0x72, 0xbb, 0xb0, 0xbd, 0x5c, 0x2b, 0x3c, 0x2e, 0x5f,
	0xd7, 0x32, 0xa3, 0x2d, 0x7a, 0xc2, 0x7c, 0x47, 0x9f, 0xd7, 0x26, 0x33, 0x71, 0x46, 0xc9, 0x45,
	0x81, 0xea, 0x7f, 0x16, 0x28, 0x4e, 0x5d, 0x10, 0xf0, 0x5b, 0x90, 0xff, 0x89, 0xa5, 0xa7, 0x11,
	0x43, 0x21, 0xb7, 0xad, 0xed, 0xe5, 0x5a, 0xde, 0xdf, 0x94, 0xa3, 0x70, 0x36, 0x71, 0x9a, 0xa3,
	0x30, 0xc7, 0x09, 0x05, 0x80, 0xcd, 0x28, 0x62, 0x01, 0x12, 0x38, 0x94, 0x9b, 0xd7, 0x0c, 0xc3,
	0x94, 0xab, 0x15, 0x5e, 0xf5, 0xbf, 0xd2, 0x42, 0x2b, 0x68, 0x92, 0xe1, 0xaa, 0x4d, 0x43, 0x32,
	0xe7, 0x86, 0xe3, 0xdf, 0xa1, 0xb3, 0x36, 0x7e, 0xc1, 0x9e, 0x56, 0x39, 0x3f, 0x25, 0x49, 0xb5,
	0xfd, 0x1e, 0x04, 0xac, 0x83, 0xec, 0x73, 0x44, 0xc3, 0x08, 0xab, 0x0d, 0xcf, 0xf8, 0xa5, 0xd1,
	0xd0, 0x59, 0x9d, 0x9a, 0xfc, 0x29, 0xab, 0xfa, 0xa7, 0x05, 0x4a, 0xef, 0xf6, 0x11, 0xf6, 0x41,
	0x46, 0x4e, 0x8e, 0xbe, 0xc3, 0x5e, 0x5c, 0x9c, 0x97, 0x1f, 0x19, 0xb3, 0xb4, 0x35, 0xf5, 0xf4,
	0x78, 0x26, 0x7a, 0x34, 0x74, 0x1e, 0x9a, 0x2d, 0x76, 0x29, 0x8a, 0xf1, 0xce, 0xf6, 0x4d, 0xf1,
	0x5b, 0xa3, 0xb0, 0x09, 0xf2, 0xb2, 0x0a, 0x4f, 0x50, 0x80, 0xf5, 0x65, 0x67, 0x8f, 0x86, 0xce,
	0x3a, 0x9d, 0x38, 0x4d, 0xaa, 0xb9, 0xde, 0xcf, 0x1e, 0x5d, 0xdf, 0xc6, 0xf2, 0x9a, 0x85, 0x05,
	0x90, 0xf3, 0x53, 0x12, 0x76, 0x71, 0x58, 0x5a, 0x82, 0x00, 0x64, 0xd5, 0x00, 0x87, 0x25, 0xcb,
	0x2f, 0x5d, 0x5e, 0x55, 0xac, 0xb7, 0x57, 0x15, 0xeb, 0xef, 0xab, 0x8a, 0xf5, 0xcf, 0x55, 0x65,
	0xe9, 0xc0, 0xea, 0x64, 0xd5, 0xd3, 0xf8, 0xc5, 0xff, 0x01, 0x00, 0x00, 0xff, 0xff, 0x83, 0x8e,
	0xfd, 0x9a, 0x83, 0x08, 0x00, 0x00,
}
