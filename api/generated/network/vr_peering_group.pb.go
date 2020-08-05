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
	IPv4Prefix []string `protobuf:"bytes,2,rep,name=IPv4Prefix,json=ipv4-prefix,omitempty" json:"ipv4-prefix,omitempty"`
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

func (m *VirtualRouterPeeringSpec) GetIPv4Prefix() []string {
	if m != nil {
		return m.IPv4Prefix
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
	if len(m.IPv4Prefix) > 0 {
		for _, s := range m.IPv4Prefix {
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
	if len(m.IPv4Prefix) > 0 {
		for _, s := range m.IPv4Prefix {
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
			m.IPv4Prefix = append(m.IPv4Prefix, string(dAtA[iNdEx:postIndex]))
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
	// 759 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x94, 0x55, 0x41, 0x4f, 0xdb, 0x48,
	0x14, 0xc6, 0x09, 0x84, 0xcd, 0x44, 0x40, 0x98, 0x15, 0xac, 0x93, 0x5d, 0x12, 0xc8, 0x6a, 0x97,
	0xac, 0x84, 0xed, 0x15, 0x45, 0x55, 0x8b, 0xd4, 0x43, 0x5d, 0x68, 0xc5, 0xa1, 0x10, 0x99, 0xa8,
	0x95, 0xaa, 0xaa, 0xc8, 0x71, 0x5e, 0xdc, 0x29, 0x8e, 0xc7, 0xb2, 0xc7, 0xa1, 0x11, 0xaa, 0xda,
	0x4b, 0x73, 0xef, 0x5f, 0xe0, 0xd8, 0x5f, 0xc2, 0x11, 0xf5, 0x07, 0x44, 0x55, 0x4e, 0x55, 0xd4,
	0x7f, 0xd0, 0x4b, 0xe5, 0xb1, 0x51, 0x9c, 0x40, 0x02, 0xbd, 0xcd, 0x7b, 0xf3, 0xde, 0xf7, 0xbe,
	0xf7, 0xbd, 0x37, 0x1a, 0xb4, 0xdc, 0x72, 0x8f, 0x1c, 0x00, 0x97, 0xd8, 0xe6, 0x91, 0xe9, 0x52,
	0xdf, 0x91, 0x1d, 0x97, 0x32, 0x8a, 0x67, 0x6d, 0x60, 0x27, 0xd4, 0x3d, 0xce, 0xff, 0x65, 0x52,
	0x6a, 0x5a, 0xa0, 0xe8, 0x0e, 0x51, 0x74, 0xdb, 0xa6, 0x4c, 0x67, 0x84, 0xda, 0x5e, 0x18, 0x96,
	0xdf, 0x35, 0x09, 0x7b, 0xed, 0xd7, 0x64, 0x83, 0x36, 0x15, 0x07, 0x6c, 0x4f, 0xb7, 0xeb, 0x54,
	0xf1, 0x4e, 0x94, 0x16, 0xd8, 0xc4, 0x00, 0xc5, 0x67, 0xc4, 0xf2, 0x82, 0x54, 0x13, 0xec, 0x78,
	0xb6, 0x42, 0x6c, 0xc3, 0xf2, 0xeb, 0x70, 0x09, 0x23, 0xc5, 0x60, 0x4c, 0x6a, 0x52, 0x85, 0xbb,
	0x6b, 0x7e, 0x83, 0x5b, 0xdc, 0xe0, 0xa7, 0x28, 0xfc, 0x9f, 0x31, 0x55, 0x03, 0x8e, 0x4d, 0x60,
	0x7a, 0x14, 0x96, 0x6e, 0xb9, 0x8d, 0xe8, 0x38, 0xef, 0x99, 0x0e, 0xb5, 0x88, 0xd1, 0x0e, 0xed,
	0xd2, 0xa7, 0x24, 0xca, 0x3d, 0x23, 0x2e, 0xf3, 0x75, 0x4b, 0xa3, 0x3e, 0x03, 0xb7, 0x12, 0x6a,
	0xf0, 0x24, 0x90, 0x00, 0xdf, 0x45, 0x42, 0x55, 0x14, 0x56, 0x85, 0x72, 0x66, 0x73, 0x4e, 0xd6,
	0x1d, 0x22, 0x57, 0xdb, 0x0e, 0x3c, 0x05, 0xa6, 0xab, 0xbf, 0x9f, 0x77, 0x8b, 0x53, 0x17, 0xdd,
	0xa2, 0xd0, 0xef, 0x16, 0x67, 0x37, 0x88, 0x6d, 0x11, 0x1b, 0xb4, 0xcb, 0x03, 0x7e, 0x8c, 0x84,
	0x03, 0x31, 0xc1, 0xf3, 0x16, 0x78, 0xde, 0x41, 0xed, 0x0d, 0x18, 0x8c, 0x67, 0xe6, 0x63, 0x99,
	0xf3, 0x01, 0xc9, 0x0d, 0xda, 0x24, 0x0c, 0x9a, 0x0e, 0x6b, 0x6b, 0x23, 0x36, 0x7e, 0x89, 0xa6,
	0x0f, 0x1d, 0x30, 0xc4, 0x24, 0x87, 0xfa, 0x57, 0x8e, 0x66, 0x21, 0x8f, 0x65, 0x1c, 0x44, 0xab,
	0xcb, 0x41, 0x85, 0x00, 0xdd, 0x73, 0xc0, 0x88, 0xa3, 0x0f, 0xdb, 0xb8, 0x8e, 0x52, 0x87, 0x4c,
	0x67, 0xbe, 0x27, 0x4e, 0x73, 0xfc, 0xf2, 0x2d, 0xf0, 0x79, 0xbc, 0x2a, 0x46, 0x15, 0xb2, 0x1e,
	0xb7, 0x63, 0x35, 0xae, 0x78, 0xb6, 0xff, 0xff, 0xf2, 0x31, 0xb7, 0x81, 0x32, 0xca, 0xe9, 0x81,
	0x5c, 0x05, 0x5b, 0xb7, 0xd9, 0x3b, 0xbc, 0xd2, 0x0a, 0xf1, 0x25, 0x97, 0x17, 0x90, 0xa2, 0xbd,
	0x93, 0xf8, 0xde, 0x79, 0xa5, 0x53, 0xb4, 0x32, 0xb1, 0x41, 0xfc, 0x02, 0xcd, 0xec, 0x31, 0x68,
	0x7a, 0xa2, 0xb0, 0x9a, 0x2c, 0x67, 0x36, 0xd7, 0x26, 0xf2, 0xe6, 0x92, 0xfc, 0x11, 0x11, 0x5e,
	0x08, 0x58, 0xc5, 0xf9, 0x8e, 0x3a, 0x4a, 0x3f, 0x12, 0xa8, 0x78, 0x43, 0xfb, 0xb8, 0x81, 0x16,
	0x2b, 0x2e, 0x75, 0x74, 0x93, 0x2f, 0x71, 0xa4, 0x61, 0xb8, 0x26, 0x7f, 0xca, 0x1e, 0x18, 0xbe,
	0x4b, 0x58, 0x5b, 0xbe, 0x12, 0x12, 0x8e, 0xbe, 0xdf, 0x2d, 0x62, 0x67, 0x70, 0x25, 0x85, 0x82,
	0x69, 0xd7, 0xf8, 0x70, 0x47, 0x40, 0x19, 0x4e, 0xa2, 0xaa, 0xd7, 0x2c, 0xf0, 0xc4, 0x04, 0x6f,
	0xf7, 0xfe, 0x6d, 0xc7, 0x24, 0xc7, 0x72, 0x77, 0x6d, 0xe6, 0xb6, 0xd5, 0xc2, 0x79, 0xb8, 0x77,
	0xcb, 0x5c, 0x78, 0x89, 0xf1, 0xab, 0x98, 0x1a, 0x63, 0xfc, 0x79, 0x13, 0x65, 0x47, 0xb1, 0x70,
	0x16, 0x25, 0x8f, 0xa1, 0xcd, 0xdb, 0x4e, 0x6b, 0xc1, 0x11, 0x3f, 0x40, 0x33, 0x2d, 0xdd, 0xf2,
	0x21, 0xda, 0xfc, 0xf5, 0x89, 0x3c, 0x07, 0x78, 0x5a, 0x98, 0xb5, 0x9d, 0xb8, 0x27, 0x94, 0xbe,
	0x0b, 0xd7, 0x3f, 0x47, 0x6e, 0xe0, 0x7d, 0x84, 0xf6, 0x2a, 0xad, 0xad, 0x8a, 0x0b, 0x0d, 0xf2,
	0x36, 0xac, 0xac, 0xfe, 0xfd, 0xb9, 0x93, 0x4b, 0x3d, 0xda, 0xdb, 0xd1, 0xca, 0xff, 0xf5, 0xbb,
	0xc5, 0x25, 0xe2, 0xb4, 0xb6, 0x24, 0x87, 0x07, 0xc4, 0xfa, 0xba, 0xde, 0x8d, 0xdf, 0xa3, 0xc5,
	0x1d, 0xf0, 0xd8, 0x50, 0x41, 0x4e, 0x3e, 0xad, 0xee, 0x9e, 0x75, 0x72, 0xab, 0x68, 0xf6, 0x39,
	0xe8, 0xc7, 0x1a, 0x34, 0xf0, 0x52, 0xd4, 0x8a, 0x32, 0x14, 0xdb, 0xef, 0x16, 0x57, 0xea, 0xe0,
	0x31, 0x69, 0x78, 0x9f, 0x63, 0x85, 0x27, 0x5f, 0x97, 0x3e, 0x08, 0xa8, 0x30, 0x59, 0x1c, 0xfc,
	0x0a, 0xa5, 0xb8, 0x75, 0xb9, 0xec, 0xa5, 0x9b, 0x55, 0x1d, 0x3c, 0x4f, 0x5e, 0x70, 0xe8, 0x79,
	0x8e, 0x7a, 0x4a, 0x3d, 0x01, 0x89, 0xe3, 0x9e, 0x0d, 0x76, 0xd1, 0xdc, 0xb0, 0x38, 0xa1, 0xe6,
	0x0f, 0xcf, 0x3a, 0xb9, 0x35, 0xf4, 0xdb, 0xbe, 0xde, 0x84, 0xfa, 0x44, 0x75, 0xc4, 0xb1, 0xc2,
	0x8c, 0xbd, 0xc1, 0xda, 0xd0, 0x90, 0x83, 0x95, 0x4f, 0xab, 0xeb, 0x83, 0x21, 0x47, 0xad, 0xfd,
	0xd2, 0xa0, 0xd5, 0xec, 0x79, 0xaf, 0x20, 0x5c, 0xf4, 0x0a, 0xc2, 0xd7, 0x5e, 0x41, 0xf8, 0xd6,
	0x2b, 0x4c, 0x55, 0x84, 0x5a, 0x8a, 0x7f, 0x00, 0x77, 0x7e, 0x06, 0x00, 0x00, 0xff, 0xff, 0x5a,
	0x71, 0x0f, 0x7c, 0xf9, 0x06, 0x00, 0x00,
}
