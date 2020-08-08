// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: vr_peering_group.proto

package network

import proto "github.com/gogo/protobuf/proto"
import fmt "fmt"
import math "math"
import _ "github.com/pensando/grpc-gateway/third_party/googleapis/google/api"
import _ "github.com/pensando/sw/venice/utils/apigen/annotations"
import _ "github.com/gogo/protobuf/gogoproto"
import api "github.com/pensando/sw/api"
import security "github.com/pensando/sw/api/generated/security"

import io "io"

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

//
type VirtualRouterPeeringGroup struct {
	//
	api.TypeMeta `protobuf:"bytes,1,opt,name=T,json=,inline,embedded=T" json:",inline"`
	//
	api.ObjectMeta `protobuf:"bytes,2,opt,name=O,json=meta,omitempty,embedded=O" json:"meta,omitempty"`
	//
	Spec VirtualRouterPeeringGroupSpec `protobuf:"bytes,3,opt,name=Spec,json=spec,omitempty" json:"spec,omitempty"`
	//
	Status VirtualRouterPeeringGroupStatus `protobuf:"bytes,4,opt,name=Status,json=status,omitempty" json:"status,omitempty"`
}

func (m *VirtualRouterPeeringGroup) Reset()         { *m = VirtualRouterPeeringGroup{} }
func (m *VirtualRouterPeeringGroup) String() string { return proto.CompactTextString(m) }
func (*VirtualRouterPeeringGroup) ProtoMessage()    {}
func (*VirtualRouterPeeringGroup) Descriptor() ([]byte, []int) {
	return fileDescriptorVrPeeringGroup, []int{0}
}

func (m *VirtualRouterPeeringGroup) GetSpec() VirtualRouterPeeringGroupSpec {
	if m != nil {
		return m.Spec
	}
	return VirtualRouterPeeringGroupSpec{}
}

func (m *VirtualRouterPeeringGroup) GetStatus() VirtualRouterPeeringGroupStatus {
	if m != nil {
		return m.Status
	}
	return VirtualRouterPeeringGroupStatus{}
}

//
type VirtualRouterPeeringGroupSpec struct {
	//
	Items []VirtualRouterPeeringSpec `protobuf:"bytes,1,rep,name=Items,json=items,omitempty" json:"items,omitempty"`
}

func (m *VirtualRouterPeeringGroupSpec) Reset()         { *m = VirtualRouterPeeringGroupSpec{} }
func (m *VirtualRouterPeeringGroupSpec) String() string { return proto.CompactTextString(m) }
func (*VirtualRouterPeeringGroupSpec) ProtoMessage()    {}
func (*VirtualRouterPeeringGroupSpec) Descriptor() ([]byte, []int) {
	return fileDescriptorVrPeeringGroup, []int{1}
}

func (m *VirtualRouterPeeringGroupSpec) GetItems() []VirtualRouterPeeringSpec {
	if m != nil {
		return m.Items
	}
	return nil
}

//
type VirtualRouterPeeringGroupStatus struct {
	// The status of the configuration propagation to the DSC
	PropagationStatus security.PropagationStatus `protobuf:"bytes,1,opt,name=PropagationStatus,json=propagation-status" json:"propagation-status"`
	// VirtualRouter -> Route table derived from the prefixes exposed by other VirtualRouters in this peering group
	RouteTables map[string]*VirtualRouterPeeringRouteTable `protobuf:"bytes,2,rep,name=RouteTables,json=route-tables,omitempty" json:"route-tables,omitempty" protobuf_key:"bytes,1,opt,name=key,proto3" protobuf_val:"bytes,2,opt,name=value"`
}

func (m *VirtualRouterPeeringGroupStatus) Reset()         { *m = VirtualRouterPeeringGroupStatus{} }
func (m *VirtualRouterPeeringGroupStatus) String() string { return proto.CompactTextString(m) }
func (*VirtualRouterPeeringGroupStatus) ProtoMessage()    {}
func (*VirtualRouterPeeringGroupStatus) Descriptor() ([]byte, []int) {
	return fileDescriptorVrPeeringGroup, []int{2}
}

func (m *VirtualRouterPeeringGroupStatus) GetPropagationStatus() security.PropagationStatus {
	if m != nil {
		return m.PropagationStatus
	}
	return security.PropagationStatus{}
}

func (m *VirtualRouterPeeringGroupStatus) GetRouteTables() map[string]*VirtualRouterPeeringRouteTable {
	if m != nil {
		return m.RouteTables
	}
	return nil
}

//
type VirtualRouterPeeringRoute struct {
	//
	IPv4Prefix string `protobuf:"bytes,1,opt,name=IPv4Prefix,json=ipv4-prefix,omitempty,proto3" json:"ipv4-prefix,omitempty"`
	// Destination VirtualRouter this prefix is reachable on
	DestVirtualRouter string `protobuf:"bytes,2,opt,name=DestVirtualRouter,json=dest-virtual-router,omitempty,proto3" json:"dest-virtual-router,omitempty"`
}

func (m *VirtualRouterPeeringRoute) Reset()         { *m = VirtualRouterPeeringRoute{} }
func (m *VirtualRouterPeeringRoute) String() string { return proto.CompactTextString(m) }
func (*VirtualRouterPeeringRoute) ProtoMessage()    {}
func (*VirtualRouterPeeringRoute) Descriptor() ([]byte, []int) {
	return fileDescriptorVrPeeringGroup, []int{3}
}

func (m *VirtualRouterPeeringRoute) GetIPv4Prefix() string {
	if m != nil {
		return m.IPv4Prefix
	}
	return ""
}

func (m *VirtualRouterPeeringRoute) GetDestVirtualRouter() string {
	if m != nil {
		return m.DestVirtualRouter
	}
	return ""
}

//
type VirtualRouterPeeringRouteTable struct {
	//
	Routes []VirtualRouterPeeringRoute `protobuf:"bytes,1,rep,name=Routes,json=routes,omitempty" json:"routes,omitempty"`
}

func (m *VirtualRouterPeeringRouteTable) Reset()         { *m = VirtualRouterPeeringRouteTable{} }
func (m *VirtualRouterPeeringRouteTable) String() string { return proto.CompactTextString(m) }
func (*VirtualRouterPeeringRouteTable) ProtoMessage()    {}
func (*VirtualRouterPeeringRouteTable) Descriptor() ([]byte, []int) {
	return fileDescriptorVrPeeringGroup, []int{4}
}

func (m *VirtualRouterPeeringRouteTable) GetRoutes() []VirtualRouterPeeringRoute {
	if m != nil {
		return m.Routes
	}
	return nil
}

//
type VirtualRouterPeeringSpec struct {
	//
	VirtualRouter string `protobuf:"bytes,1,opt,name=VirtualRouter,json=virtual-router,omitempty,proto3" json:"virtual-router,omitempty"`
	// List of destination prefixes located in this Virtual Router exposed as
	// reachable from any other Virtual Router participating in this peering group
	IPv4Prefixes []string `protobuf:"bytes,2,rep,name=IPv4Prefixes,json=ipv4-prefixes,omitempty" json:"ipv4-prefixes,omitempty"`
}

func (m *VirtualRouterPeeringSpec) Reset()         { *m = VirtualRouterPeeringSpec{} }
func (m *VirtualRouterPeeringSpec) String() string { return proto.CompactTextString(m) }
func (*VirtualRouterPeeringSpec) ProtoMessage()    {}
func (*VirtualRouterPeeringSpec) Descriptor() ([]byte, []int) {
	return fileDescriptorVrPeeringGroup, []int{5}
}

func (m *VirtualRouterPeeringSpec) GetVirtualRouter() string {
	if m != nil {
		return m.VirtualRouter
	}
	return ""
}

func (m *VirtualRouterPeeringSpec) GetIPv4Prefixes() []string {
	if m != nil {
		return m.IPv4Prefixes
	}
	return nil
}

func init() {
	proto.RegisterType((*VirtualRouterPeeringGroup)(nil), "network.VirtualRouterPeeringGroup")
	proto.RegisterType((*VirtualRouterPeeringGroupSpec)(nil), "network.VirtualRouterPeeringGroupSpec")
	proto.RegisterType((*VirtualRouterPeeringGroupStatus)(nil), "network.VirtualRouterPeeringGroupStatus")
	proto.RegisterType((*VirtualRouterPeeringRoute)(nil), "network.VirtualRouterPeeringRoute")
	proto.RegisterType((*VirtualRouterPeeringRouteTable)(nil), "network.VirtualRouterPeeringRouteTable")
	proto.RegisterType((*VirtualRouterPeeringSpec)(nil), "network.VirtualRouterPeeringSpec")
}
func (m *VirtualRouterPeeringGroup) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *VirtualRouterPeeringGroup) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	dAtA[i] = 0xa
	i++
	i = encodeVarintVrPeeringGroup(dAtA, i, uint64(m.TypeMeta.Size()))
	n1, err := m.TypeMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n1
	dAtA[i] = 0x12
	i++
	i = encodeVarintVrPeeringGroup(dAtA, i, uint64(m.ObjectMeta.Size()))
	n2, err := m.ObjectMeta.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n2
	dAtA[i] = 0x1a
	i++
	i = encodeVarintVrPeeringGroup(dAtA, i, uint64(m.Spec.Size()))
	n3, err := m.Spec.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n3
	dAtA[i] = 0x22
	i++
	i = encodeVarintVrPeeringGroup(dAtA, i, uint64(m.Status.Size()))
	n4, err := m.Status.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n4
	return i, nil
}

func (m *VirtualRouterPeeringGroupSpec) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *VirtualRouterPeeringGroupSpec) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Items) > 0 {
		for _, msg := range m.Items {
			dAtA[i] = 0xa
			i++
			i = encodeVarintVrPeeringGroup(dAtA, i, uint64(msg.Size()))
			n, err := msg.MarshalTo(dAtA[i:])
			if err != nil {
				return 0, err
			}
			i += n
		}
	}
	return i, nil
}

func (m *VirtualRouterPeeringGroupStatus) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *VirtualRouterPeeringGroupStatus) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	dAtA[i] = 0xa
	i++
	i = encodeVarintVrPeeringGroup(dAtA, i, uint64(m.PropagationStatus.Size()))
	n5, err := m.PropagationStatus.MarshalTo(dAtA[i:])
	if err != nil {
		return 0, err
	}
	i += n5
	if len(m.RouteTables) > 0 {
		for k, _ := range m.RouteTables {
			dAtA[i] = 0x12
			i++
			v := m.RouteTables[k]
			msgSize := 0
			if v != nil {
				msgSize = v.Size()
				msgSize += 1 + sovVrPeeringGroup(uint64(msgSize))
			}
			mapSize := 1 + len(k) + sovVrPeeringGroup(uint64(len(k))) + msgSize
			i = encodeVarintVrPeeringGroup(dAtA, i, uint64(mapSize))
			dAtA[i] = 0xa
			i++
			i = encodeVarintVrPeeringGroup(dAtA, i, uint64(len(k)))
			i += copy(dAtA[i:], k)
			if v != nil {
				dAtA[i] = 0x12
				i++
				i = encodeVarintVrPeeringGroup(dAtA, i, uint64(v.Size()))
				n6, err := v.MarshalTo(dAtA[i:])
				if err != nil {
					return 0, err
				}
				i += n6
			}
		}
	}
	return i, nil
}

func (m *VirtualRouterPeeringRoute) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *VirtualRouterPeeringRoute) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.IPv4Prefix) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintVrPeeringGroup(dAtA, i, uint64(len(m.IPv4Prefix)))
		i += copy(dAtA[i:], m.IPv4Prefix)
	}
	if len(m.DestVirtualRouter) > 0 {
		dAtA[i] = 0x12
		i++
		i = encodeVarintVrPeeringGroup(dAtA, i, uint64(len(m.DestVirtualRouter)))
		i += copy(dAtA[i:], m.DestVirtualRouter)
	}
	return i, nil
}

func (m *VirtualRouterPeeringRouteTable) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *VirtualRouterPeeringRouteTable) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Routes) > 0 {
		for _, msg := range m.Routes {
			dAtA[i] = 0xa
			i++
			i = encodeVarintVrPeeringGroup(dAtA, i, uint64(msg.Size()))
			n, err := msg.MarshalTo(dAtA[i:])
			if err != nil {
				return 0, err
			}
			i += n
		}
	}
	return i, nil
}

func (m *VirtualRouterPeeringSpec) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *VirtualRouterPeeringSpec) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.VirtualRouter) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintVrPeeringGroup(dAtA, i, uint64(len(m.VirtualRouter)))
		i += copy(dAtA[i:], m.VirtualRouter)
	}
	if len(m.IPv4Prefixes) > 0 {
		for _, s := range m.IPv4Prefixes {
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
	return i, nil
}

func encodeVarintVrPeeringGroup(dAtA []byte, offset int, v uint64) int {
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return offset + 1
}
func (m *VirtualRouterPeeringGroup) Size() (n int) {
	var l int
	_ = l
	l = m.TypeMeta.Size()
	n += 1 + l + sovVrPeeringGroup(uint64(l))
	l = m.ObjectMeta.Size()
	n += 1 + l + sovVrPeeringGroup(uint64(l))
	l = m.Spec.Size()
	n += 1 + l + sovVrPeeringGroup(uint64(l))
	l = m.Status.Size()
	n += 1 + l + sovVrPeeringGroup(uint64(l))
	return n
}

func (m *VirtualRouterPeeringGroupSpec) Size() (n int) {
	var l int
	_ = l
	if len(m.Items) > 0 {
		for _, e := range m.Items {
			l = e.Size()
			n += 1 + l + sovVrPeeringGroup(uint64(l))
		}
	}
	return n
}

func (m *VirtualRouterPeeringGroupStatus) Size() (n int) {
	var l int
	_ = l
	l = m.PropagationStatus.Size()
	n += 1 + l + sovVrPeeringGroup(uint64(l))
	if len(m.RouteTables) > 0 {
		for k, v := range m.RouteTables {
			_ = k
			_ = v
			l = 0
			if v != nil {
				l = v.Size()
				l += 1 + sovVrPeeringGroup(uint64(l))
			}
			mapEntrySize := 1 + len(k) + sovVrPeeringGroup(uint64(len(k))) + l
			n += mapEntrySize + 1 + sovVrPeeringGroup(uint64(mapEntrySize))
		}
	}
	return n
}

func (m *VirtualRouterPeeringRoute) Size() (n int) {
	var l int
	_ = l
	l = len(m.IPv4Prefix)
	if l > 0 {
		n += 1 + l + sovVrPeeringGroup(uint64(l))
	}
	l = len(m.DestVirtualRouter)
	if l > 0 {
		n += 1 + l + sovVrPeeringGroup(uint64(l))
	}
	return n
}

func (m *VirtualRouterPeeringRouteTable) Size() (n int) {
	var l int
	_ = l
	if len(m.Routes) > 0 {
		for _, e := range m.Routes {
			l = e.Size()
			n += 1 + l + sovVrPeeringGroup(uint64(l))
		}
	}
	return n
}

func (m *VirtualRouterPeeringSpec) Size() (n int) {
	var l int
	_ = l
	l = len(m.VirtualRouter)
	if l > 0 {
		n += 1 + l + sovVrPeeringGroup(uint64(l))
	}
	if len(m.IPv4Prefixes) > 0 {
		for _, s := range m.IPv4Prefixes {
			l = len(s)
			n += 1 + l + sovVrPeeringGroup(uint64(l))
		}
	}
	return n
}

func sovVrPeeringGroup(x uint64) (n int) {
	for {
		n++
		x >>= 7
		if x == 0 {
			break
		}
	}
	return n
}
func sozVrPeeringGroup(x uint64) (n int) {
	return sovVrPeeringGroup(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *VirtualRouterPeeringGroup) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowVrPeeringGroup
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
			return fmt.Errorf("proto: VirtualRouterPeeringGroup: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: VirtualRouterPeeringGroup: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field TypeMeta", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowVrPeeringGroup
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
				return ErrInvalidLengthVrPeeringGroup
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
					return ErrIntOverflowVrPeeringGroup
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
				return ErrInvalidLengthVrPeeringGroup
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
					return ErrIntOverflowVrPeeringGroup
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
				return ErrInvalidLengthVrPeeringGroup
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
					return ErrIntOverflowVrPeeringGroup
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
				return ErrInvalidLengthVrPeeringGroup
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
			skippy, err := skipVrPeeringGroup(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthVrPeeringGroup
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
func (m *VirtualRouterPeeringGroupSpec) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowVrPeeringGroup
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
			return fmt.Errorf("proto: VirtualRouterPeeringGroupSpec: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: VirtualRouterPeeringGroupSpec: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Items", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowVrPeeringGroup
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
				return ErrInvalidLengthVrPeeringGroup
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Items = append(m.Items, VirtualRouterPeeringSpec{})
			if err := m.Items[len(m.Items)-1].Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipVrPeeringGroup(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthVrPeeringGroup
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
func (m *VirtualRouterPeeringGroupStatus) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowVrPeeringGroup
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
			return fmt.Errorf("proto: VirtualRouterPeeringGroupStatus: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: VirtualRouterPeeringGroupStatus: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field PropagationStatus", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowVrPeeringGroup
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
				return ErrInvalidLengthVrPeeringGroup
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if err := m.PropagationStatus.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field RouteTables", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowVrPeeringGroup
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
				return ErrInvalidLengthVrPeeringGroup
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if m.RouteTables == nil {
				m.RouteTables = make(map[string]*VirtualRouterPeeringRouteTable)
			}
			var mapkey string
			var mapvalue *VirtualRouterPeeringRouteTable
			for iNdEx < postIndex {
				entryPreIndex := iNdEx
				var wire uint64
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return ErrIntOverflowVrPeeringGroup
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
				if fieldNum == 1 {
					var stringLenmapkey uint64
					for shift := uint(0); ; shift += 7 {
						if shift >= 64 {
							return ErrIntOverflowVrPeeringGroup
						}
						if iNdEx >= l {
							return io.ErrUnexpectedEOF
						}
						b := dAtA[iNdEx]
						iNdEx++
						stringLenmapkey |= (uint64(b) & 0x7F) << shift
						if b < 0x80 {
							break
						}
					}
					intStringLenmapkey := int(stringLenmapkey)
					if intStringLenmapkey < 0 {
						return ErrInvalidLengthVrPeeringGroup
					}
					postStringIndexmapkey := iNdEx + intStringLenmapkey
					if postStringIndexmapkey > l {
						return io.ErrUnexpectedEOF
					}
					mapkey = string(dAtA[iNdEx:postStringIndexmapkey])
					iNdEx = postStringIndexmapkey
				} else if fieldNum == 2 {
					var mapmsglen int
					for shift := uint(0); ; shift += 7 {
						if shift >= 64 {
							return ErrIntOverflowVrPeeringGroup
						}
						if iNdEx >= l {
							return io.ErrUnexpectedEOF
						}
						b := dAtA[iNdEx]
						iNdEx++
						mapmsglen |= (int(b) & 0x7F) << shift
						if b < 0x80 {
							break
						}
					}
					if mapmsglen < 0 {
						return ErrInvalidLengthVrPeeringGroup
					}
					postmsgIndex := iNdEx + mapmsglen
					if mapmsglen < 0 {
						return ErrInvalidLengthVrPeeringGroup
					}
					if postmsgIndex > l {
						return io.ErrUnexpectedEOF
					}
					mapvalue = &VirtualRouterPeeringRouteTable{}
					if err := mapvalue.Unmarshal(dAtA[iNdEx:postmsgIndex]); err != nil {
						return err
					}
					iNdEx = postmsgIndex
				} else {
					iNdEx = entryPreIndex
					skippy, err := skipVrPeeringGroup(dAtA[iNdEx:])
					if err != nil {
						return err
					}
					if skippy < 0 {
						return ErrInvalidLengthVrPeeringGroup
					}
					if (iNdEx + skippy) > postIndex {
						return io.ErrUnexpectedEOF
					}
					iNdEx += skippy
				}
			}
			m.RouteTables[mapkey] = mapvalue
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipVrPeeringGroup(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthVrPeeringGroup
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
func (m *VirtualRouterPeeringRoute) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowVrPeeringGroup
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
			return fmt.Errorf("proto: VirtualRouterPeeringRoute: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: VirtualRouterPeeringRoute: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field IPv4Prefix", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowVrPeeringGroup
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
				return ErrInvalidLengthVrPeeringGroup
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.IPv4Prefix = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field DestVirtualRouter", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowVrPeeringGroup
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
				return ErrInvalidLengthVrPeeringGroup
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.DestVirtualRouter = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipVrPeeringGroup(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthVrPeeringGroup
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
func (m *VirtualRouterPeeringRouteTable) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowVrPeeringGroup
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
			return fmt.Errorf("proto: VirtualRouterPeeringRouteTable: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: VirtualRouterPeeringRouteTable: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Routes", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowVrPeeringGroup
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
				return ErrInvalidLengthVrPeeringGroup
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Routes = append(m.Routes, VirtualRouterPeeringRoute{})
			if err := m.Routes[len(m.Routes)-1].Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipVrPeeringGroup(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthVrPeeringGroup
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
func (m *VirtualRouterPeeringSpec) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowVrPeeringGroup
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
			return fmt.Errorf("proto: VirtualRouterPeeringSpec: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: VirtualRouterPeeringSpec: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field VirtualRouter", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowVrPeeringGroup
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
				return ErrInvalidLengthVrPeeringGroup
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.VirtualRouter = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field IPv4Prefixes", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowVrPeeringGroup
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
				return ErrInvalidLengthVrPeeringGroup
			}
			postIndex := iNdEx + intStringLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.IPv4Prefixes = append(m.IPv4Prefixes, string(dAtA[iNdEx:postIndex]))
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipVrPeeringGroup(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthVrPeeringGroup
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
func skipVrPeeringGroup(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowVrPeeringGroup
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
					return 0, ErrIntOverflowVrPeeringGroup
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
					return 0, ErrIntOverflowVrPeeringGroup
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
				return 0, ErrInvalidLengthVrPeeringGroup
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowVrPeeringGroup
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
				next, err := skipVrPeeringGroup(dAtA[start:])
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
	ErrInvalidLengthVrPeeringGroup = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowVrPeeringGroup   = fmt.Errorf("proto: integer overflow")
)

func init() { proto.RegisterFile("vr_peering_group.proto", fileDescriptorVrPeeringGroup) }

var fileDescriptorVrPeeringGroup = []byte{
	// 761 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x8c, 0x55, 0x5f, 0x4f, 0xd3, 0x50,
	0x14, 0xa7, 0x1b, 0x0c, 0x77, 0x27, 0x30, 0xae, 0x01, 0xba, 0x29, 0x1b, 0xcc, 0xa8, 0x23, 0xa1,
	0xad, 0x41, 0x62, 0x94, 0xc4, 0x07, 0x2b, 0x68, 0x78, 0x10, 0x96, 0xb2, 0xa8, 0x31, 0x46, 0xd2,
	0x75, 0x77, 0xf5, 0x4a, 0xd7, 0x7b, 0xd3, 0xde, 0x0e, 0x17, 0x62, 0xf4, 0xc5, 0xbd, 0xfb, 0x15,
	0x78, 0xf4, 0x93, 0xf0, 0x48, 0xfc, 0x00, 0x8b, 0xd9, 0x13, 0x59, 0xfc, 0x06, 0xbe, 0x98, 0xde,
	0x96, 0xac, 0x1b, 0x6c, 0xf0, 0xd6, 0xf3, 0xef, 0x77, 0x7e, 0xe7, 0x77, 0xce, 0x4d, 0xc1, 0x7c,
	0xc3, 0xd9, 0xa7, 0x08, 0x39, 0xd8, 0x36, 0xf7, 0x4d, 0x87, 0x78, 0x54, 0xa6, 0x0e, 0x61, 0x04,
	0x4e, 0xda, 0x88, 0x1d, 0x12, 0xe7, 0x20, 0x7b, 0xc7, 0x24, 0xc4, 0xb4, 0x90, 0xa2, 0x53, 0xac,
	0xe8, 0xb6, 0x4d, 0x98, 0xce, 0x30, 0xb1, 0xdd, 0x20, 0x2d, 0xbb, 0x65, 0x62, 0xf6, 0xc9, 0xab,
	0xc8, 0x06, 0xa9, 0x2b, 0x14, 0xd9, 0xae, 0x6e, 0x57, 0x89, 0xe2, 0x1e, 0x2a, 0x0d, 0x64, 0x63,
	0x03, 0x29, 0x1e, 0xc3, 0x96, 0xeb, 0x97, 0x9a, 0xc8, 0x8e, 0x56, 0x2b, 0xd8, 0x36, 0x2c, 0xaf,
	0x8a, 0xce, 0x61, 0xa4, 0x08, 0x8c, 0x49, 0x4c, 0xa2, 0x70, 0x77, 0xc5, 0xab, 0x71, 0x8b, 0x1b,
	0xfc, 0x2b, 0x4c, 0xbf, 0x37, 0xa4, 0xab, 0xcf, 0xb1, 0x8e, 0x98, 0x1e, 0xa6, 0x25, 0x1b, 0x4e,
	0x2d, 0xfc, 0x9c, 0x76, 0x4d, 0x4a, 0x2c, 0x6c, 0x34, 0x03, 0xbb, 0xf0, 0x33, 0x0e, 0x32, 0x6f,
	0xb0, 0xc3, 0x3c, 0xdd, 0xd2, 0x88, 0xc7, 0x90, 0x53, 0x0a, 0x34, 0x78, 0xe5, 0x4b, 0x00, 0x1f,
	0x03, 0xa1, 0x2c, 0x0a, 0x4b, 0x42, 0x31, 0xb5, 0x36, 0x25, 0xeb, 0x14, 0xcb, 0xe5, 0x26, 0x45,
	0xaf, 0x11, 0xd3, 0xd5, 0x5b, 0x27, 0xed, 0xfc, 0xd8, 0x69, 0x3b, 0x2f, 0x74, 0xdb, 0xf9, 0xc9,
	0x55, 0x6c, 0x5b, 0xd8, 0x46, 0xda, 0xf9, 0x07, 0x7c, 0x09, 0x84, 0x5d, 0x31, 0xc6, 0xeb, 0x66,
	0x78, 0xdd, 0x6e, 0xe5, 0x33, 0x32, 0x18, 0xaf, 0xcc, 0x46, 0x2a, 0xa7, 0x7d, 0x92, 0xab, 0xa4,
	0x8e, 0x19, 0xaa, 0x53, 0xd6, 0xd4, 0x06, 0x6c, 0xf8, 0x01, 0x8c, 0xef, 0x51, 0x64, 0x88, 0x71,
	0x0e, 0x75, 0x5f, 0x0e, 0x77, 0x21, 0x0f, 0x65, 0xec, 0x67, 0xab, 0xf3, 0x7e, 0x07, 0x1f, 0xdd,
	0xa5, 0xc8, 0x88, 0xa2, 0xf7, 0xdb, 0xb0, 0x0a, 0x12, 0x7b, 0x4c, 0x67, 0x9e, 0x2b, 0x8e, 0x73,
	0xfc, 0xe2, 0x35, 0xf0, 0x79, 0xbe, 0x2a, 0x86, 0x1d, 0xd2, 0x2e, 0xb7, 0x23, 0x3d, 0x2e, 0x78,
	0x36, 0x1e, 0xfe, 0xfe, 0x91, 0x59, 0x05, 0x29, 0xe5, 0x68, 0x57, 0x2e, 0x23, 0x5b, 0xb7, 0xd9,
	0x57, 0xb8, 0xd8, 0x08, 0xf0, 0x25, 0x87, 0x37, 0x90, 0xc2, 0xbb, 0x93, 0xf8, 0xdd, 0xb9, 0x85,
	0x23, 0xb0, 0x38, 0x72, 0x40, 0xf8, 0x1e, 0x4c, 0x6c, 0x33, 0x54, 0x77, 0x45, 0x61, 0x29, 0x5e,
	0x4c, 0xad, 0x2d, 0x8f, 0xe4, 0xcd, 0x25, 0x59, 0x08, 0x09, 0xcf, 0xf8, 0xac, 0xa2, 0x7c, 0x07,
	0x1d, 0x85, 0x7f, 0x31, 0x90, 0xbf, 0x62, 0x7c, 0x58, 0x03, 0xb3, 0x25, 0x87, 0x50, 0xdd, 0xe4,
	0x47, 0x1c, 0x6a, 0x18, 0x9c, 0xc9, 0x6d, 0xd9, 0x45, 0x86, 0xe7, 0x60, 0xd6, 0x94, 0x2f, 0xa4,
	0x04, 0xab, 0xef, 0xb6, 0xf3, 0x90, 0xf6, 0x42, 0x52, 0x20, 0x98, 0x76, 0x89, 0x0f, 0xb6, 0x04,
	0x90, 0xe2, 0x24, 0xca, 0x7a, 0xc5, 0x42, 0xae, 0x18, 0xe3, 0xe3, 0x3e, 0xbd, 0xee, 0x9a, 0xe4,
	0x48, 0xed, 0x96, 0xcd, 0x9c, 0xa6, 0x9a, 0x3b, 0x09, 0xee, 0x6e, 0x9e, 0x0b, 0x2f, 0x31, 0x1e,
	0x8a, 0xa8, 0x31, 0xc4, 0x9f, 0x35, 0x41, 0x7a, 0x10, 0x0b, 0xa6, 0x41, 0xfc, 0x00, 0x35, 0xf9,
	0xd8, 0x49, 0xcd, 0xff, 0x84, 0xcf, 0xc0, 0x44, 0x43, 0xb7, 0x3c, 0x14, 0x5e, 0xfe, 0x83, 0x91,
	0x3c, 0x7b, 0x78, 0x5a, 0x50, 0xb5, 0x11, 0x7b, 0x22, 0x14, 0xfe, 0x0a, 0x97, 0x3f, 0x47, 0x6e,
	0xc0, 0x1d, 0x00, 0xb6, 0x4b, 0x8d, 0xf5, 0x92, 0x83, 0x6a, 0xf8, 0x4b, 0xd0, 0x59, 0xbd, 0xfb,
	0xab, 0x95, 0x49, 0xbc, 0xd8, 0xde, 0xd4, 0x8a, 0x2b, 0xdd, 0x76, 0x7e, 0x0e, 0xd3, 0xc6, 0xba,
	0x44, 0x79, 0x42, 0x64, 0xae, 0xcb, 0xdd, 0xf0, 0x1b, 0x98, 0xdd, 0x44, 0x2e, 0xeb, 0x6b, 0xc8,
	0xc9, 0x27, 0xd5, 0xad, 0xe3, 0x56, 0x66, 0x09, 0x4c, 0xbe, 0x45, 0xfa, 0x81, 0x86, 0x6a, 0x70,
	0x2e, 0x1c, 0x45, 0xe9, 0xcb, 0xed, 0xb6, 0xf3, 0x8b, 0x55, 0xe4, 0x32, 0xa9, 0xff, 0x9e, 0x23,
	0x8d, 0x47, 0x87, 0x0b, 0xdf, 0x05, 0x90, 0x1b, 0x2d, 0x0e, 0xfc, 0x08, 0x12, 0xdc, 0x3a, 0x3f,
	0xf6, 0xc2, 0xd5, 0xaa, 0xf6, 0x9e, 0x27, 0x6f, 0xd8, 0xf7, 0x3c, 0x07, 0x3d, 0x85, 0x33, 0x01,
	0x88, 0xc3, 0x9e, 0x0d, 0x74, 0xc0, 0x54, 0xbf, 0x38, 0x81, 0xe6, 0xcf, 0x8f, 0x5b, 0x99, 0x65,
	0x70, 0x63, 0x47, 0xaf, 0xa3, 0xea, 0x48, 0x75, 0xc4, 0xa1, 0xc2, 0x0c, 0x8d, 0xc0, 0x77, 0xe0,
	0x66, 0x6f, 0xc9, 0xe1, 0xd1, 0x27, 0xd5, 0x95, 0xde, 0x9a, 0xc3, 0xe1, 0x16, 0x22, 0x3b, 0xed,
	0x9b, 0x71, 0x58, 0x40, 0x4d, 0x9f, 0x74, 0x72, 0xc2, 0x69, 0x27, 0x27, 0xfc, 0xe9, 0xe4, 0x84,
	0xb3, 0x4e, 0x6e, 0xac, 0x24, 0x54, 0x12, 0xfc, 0x37, 0xf0, 0xe8, 0x7f, 0x00, 0x00, 0x00, 0xff,
	0xff, 0xdd, 0x54, 0xe9, 0xd4, 0xff, 0x06, 0x00, 0x00,
}
