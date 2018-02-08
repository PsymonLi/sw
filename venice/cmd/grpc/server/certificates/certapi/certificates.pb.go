// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: certificates.proto

/*
Package certapi is a generated protocol buffer package.

It is generated from these files:
	certificates.proto

It has these top-level messages:
	Empty
	Certificate
	CertificateSignReq
	CertificateSignResp
	CaTrustChain
	TrustRoots
*/
package certapi

import proto "github.com/gogo/protobuf/proto"
import fmt "fmt"
import math "math"

import context "golang.org/x/net/context"
import grpc "google.golang.org/grpc"

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

// Empty parameter
type Empty struct {
}

func (m *Empty) Reset()                    { *m = Empty{} }
func (m *Empty) String() string            { return proto.CompactTextString(m) }
func (*Empty) ProtoMessage()               {}
func (*Empty) Descriptor() ([]byte, []int) { return fileDescriptorCertificates, []int{0} }

type Certificate struct {
	Certificate []byte `protobuf:"bytes,1,opt,name=certificate,proto3" json:"certificate,omitempty"`
}

func (m *Certificate) Reset()                    { *m = Certificate{} }
func (m *Certificate) String() string            { return proto.CompactTextString(m) }
func (*Certificate) ProtoMessage()               {}
func (*Certificate) Descriptor() ([]byte, []int) { return fileDescriptorCertificates, []int{1} }

func (m *Certificate) GetCertificate() []byte {
	if m != nil {
		return m.Certificate
	}
	return nil
}

type CertificateSignReq struct {
	Csr []byte `protobuf:"bytes,1,opt,name=csr,proto3" json:"csr,omitempty"`
}

func (m *CertificateSignReq) Reset()                    { *m = CertificateSignReq{} }
func (m *CertificateSignReq) String() string            { return proto.CompactTextString(m) }
func (*CertificateSignReq) ProtoMessage()               {}
func (*CertificateSignReq) Descriptor() ([]byte, []int) { return fileDescriptorCertificates, []int{2} }

func (m *CertificateSignReq) GetCsr() []byte {
	if m != nil {
		return m.Csr
	}
	return nil
}

type CertificateSignResp struct {
	Certificate *Certificate `protobuf:"bytes,1,opt,name=certificate" json:"certificate,omitempty"`
}

func (m *CertificateSignResp) Reset()                    { *m = CertificateSignResp{} }
func (m *CertificateSignResp) String() string            { return proto.CompactTextString(m) }
func (*CertificateSignResp) ProtoMessage()               {}
func (*CertificateSignResp) Descriptor() ([]byte, []int) { return fileDescriptorCertificates, []int{3} }

func (m *CertificateSignResp) GetCertificate() *Certificate {
	if m != nil {
		return m.Certificate
	}
	return nil
}

type CaTrustChain struct {
	Certificates []*Certificate `protobuf:"bytes,1,rep,name=certificates" json:"certificates,omitempty"`
}

func (m *CaTrustChain) Reset()                    { *m = CaTrustChain{} }
func (m *CaTrustChain) String() string            { return proto.CompactTextString(m) }
func (*CaTrustChain) ProtoMessage()               {}
func (*CaTrustChain) Descriptor() ([]byte, []int) { return fileDescriptorCertificates, []int{4} }

func (m *CaTrustChain) GetCertificates() []*Certificate {
	if m != nil {
		return m.Certificates
	}
	return nil
}

type TrustRoots struct {
	TrustRoots []*Certificate `protobuf:"bytes,1,rep,name=trustRoots" json:"trustRoots,omitempty"`
}

func (m *TrustRoots) Reset()                    { *m = TrustRoots{} }
func (m *TrustRoots) String() string            { return proto.CompactTextString(m) }
func (*TrustRoots) ProtoMessage()               {}
func (*TrustRoots) Descriptor() ([]byte, []int) { return fileDescriptorCertificates, []int{5} }

func (m *TrustRoots) GetTrustRoots() []*Certificate {
	if m != nil {
		return m.TrustRoots
	}
	return nil
}

func init() {
	proto.RegisterType((*Empty)(nil), "certapi.Empty")
	proto.RegisterType((*Certificate)(nil), "certapi.Certificate")
	proto.RegisterType((*CertificateSignReq)(nil), "certapi.CertificateSignReq")
	proto.RegisterType((*CertificateSignResp)(nil), "certapi.CertificateSignResp")
	proto.RegisterType((*CaTrustChain)(nil), "certapi.CaTrustChain")
	proto.RegisterType((*TrustRoots)(nil), "certapi.TrustRoots")
}

// Reference imports to suppress errors if they are not otherwise used.
var _ context.Context
var _ grpc.ClientConn

// This is a compile-time assertion to ensure that this generated file
// is compatible with the grpc package it is being compiled against.
const _ = grpc.SupportPackageIsVersion4

// Client API for Certificates service

type CertificatesClient interface {
	// Have the Cluster CA sign the Supplied CSR
	SignCertificateRequest(ctx context.Context, in *CertificateSignReq, opts ...grpc.CallOption) (*CertificateSignResp, error)
	// Retrieve the trust chain of the Cluster CA
	// The first certificate is the CA certificate, the last is the trust root of the CA
	GetCaTrustChain(ctx context.Context, in *Empty, opts ...grpc.CallOption) (*CaTrustChain, error)
	// Retrieve trust roots that should be trusted by all cluster members
	// Includes the trust root of the Cluster CA
	GetTrustRoots(ctx context.Context, in *Empty, opts ...grpc.CallOption) (*TrustRoots, error)
}

type certificatesClient struct {
	cc *grpc.ClientConn
}

func NewCertificatesClient(cc *grpc.ClientConn) CertificatesClient {
	return &certificatesClient{cc}
}

func (c *certificatesClient) SignCertificateRequest(ctx context.Context, in *CertificateSignReq, opts ...grpc.CallOption) (*CertificateSignResp, error) {
	out := new(CertificateSignResp)
	err := grpc.Invoke(ctx, "/certapi.Certificates/SignCertificateRequest", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *certificatesClient) GetCaTrustChain(ctx context.Context, in *Empty, opts ...grpc.CallOption) (*CaTrustChain, error) {
	out := new(CaTrustChain)
	err := grpc.Invoke(ctx, "/certapi.Certificates/GetCaTrustChain", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *certificatesClient) GetTrustRoots(ctx context.Context, in *Empty, opts ...grpc.CallOption) (*TrustRoots, error) {
	out := new(TrustRoots)
	err := grpc.Invoke(ctx, "/certapi.Certificates/GetTrustRoots", in, out, c.cc, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

// Server API for Certificates service

type CertificatesServer interface {
	// Have the Cluster CA sign the Supplied CSR
	SignCertificateRequest(context.Context, *CertificateSignReq) (*CertificateSignResp, error)
	// Retrieve the trust chain of the Cluster CA
	// The first certificate is the CA certificate, the last is the trust root of the CA
	GetCaTrustChain(context.Context, *Empty) (*CaTrustChain, error)
	// Retrieve trust roots that should be trusted by all cluster members
	// Includes the trust root of the Cluster CA
	GetTrustRoots(context.Context, *Empty) (*TrustRoots, error)
}

func RegisterCertificatesServer(s *grpc.Server, srv CertificatesServer) {
	s.RegisterService(&_Certificates_serviceDesc, srv)
}

func _Certificates_SignCertificateRequest_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(CertificateSignReq)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(CertificatesServer).SignCertificateRequest(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/certapi.Certificates/SignCertificateRequest",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(CertificatesServer).SignCertificateRequest(ctx, req.(*CertificateSignReq))
	}
	return interceptor(ctx, in, info, handler)
}

func _Certificates_GetCaTrustChain_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(Empty)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(CertificatesServer).GetCaTrustChain(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/certapi.Certificates/GetCaTrustChain",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(CertificatesServer).GetCaTrustChain(ctx, req.(*Empty))
	}
	return interceptor(ctx, in, info, handler)
}

func _Certificates_GetTrustRoots_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(Empty)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(CertificatesServer).GetTrustRoots(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/certapi.Certificates/GetTrustRoots",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(CertificatesServer).GetTrustRoots(ctx, req.(*Empty))
	}
	return interceptor(ctx, in, info, handler)
}

var _Certificates_serviceDesc = grpc.ServiceDesc{
	ServiceName: "certapi.Certificates",
	HandlerType: (*CertificatesServer)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "SignCertificateRequest",
			Handler:    _Certificates_SignCertificateRequest_Handler,
		},
		{
			MethodName: "GetCaTrustChain",
			Handler:    _Certificates_GetCaTrustChain_Handler,
		},
		{
			MethodName: "GetTrustRoots",
			Handler:    _Certificates_GetTrustRoots_Handler,
		},
	},
	Streams:  []grpc.StreamDesc{},
	Metadata: "certificates.proto",
}

func (m *Empty) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *Empty) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	return i, nil
}

func (m *Certificate) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *Certificate) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Certificate) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintCertificates(dAtA, i, uint64(len(m.Certificate)))
		i += copy(dAtA[i:], m.Certificate)
	}
	return i, nil
}

func (m *CertificateSignReq) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *CertificateSignReq) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Csr) > 0 {
		dAtA[i] = 0xa
		i++
		i = encodeVarintCertificates(dAtA, i, uint64(len(m.Csr)))
		i += copy(dAtA[i:], m.Csr)
	}
	return i, nil
}

func (m *CertificateSignResp) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *CertificateSignResp) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if m.Certificate != nil {
		dAtA[i] = 0xa
		i++
		i = encodeVarintCertificates(dAtA, i, uint64(m.Certificate.Size()))
		n1, err := m.Certificate.MarshalTo(dAtA[i:])
		if err != nil {
			return 0, err
		}
		i += n1
	}
	return i, nil
}

func (m *CaTrustChain) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *CaTrustChain) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.Certificates) > 0 {
		for _, msg := range m.Certificates {
			dAtA[i] = 0xa
			i++
			i = encodeVarintCertificates(dAtA, i, uint64(msg.Size()))
			n, err := msg.MarshalTo(dAtA[i:])
			if err != nil {
				return 0, err
			}
			i += n
		}
	}
	return i, nil
}

func (m *TrustRoots) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalTo(dAtA)
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *TrustRoots) MarshalTo(dAtA []byte) (int, error) {
	var i int
	_ = i
	var l int
	_ = l
	if len(m.TrustRoots) > 0 {
		for _, msg := range m.TrustRoots {
			dAtA[i] = 0xa
			i++
			i = encodeVarintCertificates(dAtA, i, uint64(msg.Size()))
			n, err := msg.MarshalTo(dAtA[i:])
			if err != nil {
				return 0, err
			}
			i += n
		}
	}
	return i, nil
}

func encodeVarintCertificates(dAtA []byte, offset int, v uint64) int {
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return offset + 1
}
func (m *Empty) Size() (n int) {
	var l int
	_ = l
	return n
}

func (m *Certificate) Size() (n int) {
	var l int
	_ = l
	l = len(m.Certificate)
	if l > 0 {
		n += 1 + l + sovCertificates(uint64(l))
	}
	return n
}

func (m *CertificateSignReq) Size() (n int) {
	var l int
	_ = l
	l = len(m.Csr)
	if l > 0 {
		n += 1 + l + sovCertificates(uint64(l))
	}
	return n
}

func (m *CertificateSignResp) Size() (n int) {
	var l int
	_ = l
	if m.Certificate != nil {
		l = m.Certificate.Size()
		n += 1 + l + sovCertificates(uint64(l))
	}
	return n
}

func (m *CaTrustChain) Size() (n int) {
	var l int
	_ = l
	if len(m.Certificates) > 0 {
		for _, e := range m.Certificates {
			l = e.Size()
			n += 1 + l + sovCertificates(uint64(l))
		}
	}
	return n
}

func (m *TrustRoots) Size() (n int) {
	var l int
	_ = l
	if len(m.TrustRoots) > 0 {
		for _, e := range m.TrustRoots {
			l = e.Size()
			n += 1 + l + sovCertificates(uint64(l))
		}
	}
	return n
}

func sovCertificates(x uint64) (n int) {
	for {
		n++
		x >>= 7
		if x == 0 {
			break
		}
	}
	return n
}
func sozCertificates(x uint64) (n int) {
	return sovCertificates(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (m *Empty) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowCertificates
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
			return fmt.Errorf("proto: Empty: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: Empty: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		default:
			iNdEx = preIndex
			skippy, err := skipCertificates(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthCertificates
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
func (m *Certificate) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowCertificates
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
			return fmt.Errorf("proto: Certificate: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: Certificate: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Certificate", wireType)
			}
			var byteLen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowCertificates
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
				return ErrInvalidLengthCertificates
			}
			postIndex := iNdEx + byteLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Certificate = append(m.Certificate[:0], dAtA[iNdEx:postIndex]...)
			if m.Certificate == nil {
				m.Certificate = []byte{}
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipCertificates(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthCertificates
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
func (m *CertificateSignReq) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowCertificates
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
			return fmt.Errorf("proto: CertificateSignReq: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: CertificateSignReq: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Csr", wireType)
			}
			var byteLen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowCertificates
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
				return ErrInvalidLengthCertificates
			}
			postIndex := iNdEx + byteLen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Csr = append(m.Csr[:0], dAtA[iNdEx:postIndex]...)
			if m.Csr == nil {
				m.Csr = []byte{}
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipCertificates(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthCertificates
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
func (m *CertificateSignResp) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowCertificates
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
			return fmt.Errorf("proto: CertificateSignResp: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: CertificateSignResp: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Certificate", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowCertificates
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
				return ErrInvalidLengthCertificates
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if m.Certificate == nil {
				m.Certificate = &Certificate{}
			}
			if err := m.Certificate.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipCertificates(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthCertificates
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
func (m *CaTrustChain) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowCertificates
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
			return fmt.Errorf("proto: CaTrustChain: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: CaTrustChain: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Certificates", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowCertificates
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
				return ErrInvalidLengthCertificates
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Certificates = append(m.Certificates, &Certificate{})
			if err := m.Certificates[len(m.Certificates)-1].Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipCertificates(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthCertificates
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
func (m *TrustRoots) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowCertificates
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
			return fmt.Errorf("proto: TrustRoots: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: TrustRoots: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field TrustRoots", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowCertificates
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
				return ErrInvalidLengthCertificates
			}
			postIndex := iNdEx + msglen
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.TrustRoots = append(m.TrustRoots, &Certificate{})
			if err := m.TrustRoots[len(m.TrustRoots)-1].Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipCertificates(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthCertificates
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
func skipCertificates(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowCertificates
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
					return 0, ErrIntOverflowCertificates
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
					return 0, ErrIntOverflowCertificates
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
				return 0, ErrInvalidLengthCertificates
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowCertificates
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
				next, err := skipCertificates(dAtA[start:])
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
	ErrInvalidLengthCertificates = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowCertificates   = fmt.Errorf("proto: integer overflow")
)

func init() { proto.RegisterFile("certificates.proto", fileDescriptorCertificates) }

var fileDescriptorCertificates = []byte{
	// 285 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0xe2, 0x12, 0x4a, 0x4e, 0x2d, 0x2a,
	0xc9, 0x4c, 0xcb, 0x4c, 0x4e, 0x2c, 0x49, 0x2d, 0xd6, 0x2b, 0x28, 0xca, 0x2f, 0xc9, 0x17, 0x62,
	0x07, 0x89, 0x25, 0x16, 0x64, 0x2a, 0xb1, 0x73, 0xb1, 0xba, 0xe6, 0x16, 0x94, 0x54, 0x2a, 0xe9,
	0x73, 0x71, 0x3b, 0x23, 0xd4, 0x09, 0x29, 0x70, 0x71, 0x23, 0x69, 0x93, 0x60, 0x54, 0x60, 0xd4,
	0xe0, 0x09, 0x42, 0x16, 0x52, 0x52, 0xe3, 0x12, 0x42, 0xd2, 0x10, 0x9c, 0x99, 0x9e, 0x17, 0x94,
	0x5a, 0x28, 0x24, 0xc0, 0xc5, 0x9c, 0x5c, 0x5c, 0x04, 0x55, 0x0f, 0x62, 0x2a, 0xf9, 0x72, 0x09,
	0x63, 0xa8, 0x2b, 0x2e, 0x10, 0x32, 0xc3, 0xb4, 0x80, 0xdb, 0x48, 0x44, 0x0f, 0xea, 0x2e, 0x3d,
	0x24, 0x2d, 0xa8, 0xd6, 0x7a, 0x70, 0xf1, 0x38, 0x27, 0x86, 0x14, 0x95, 0x16, 0x97, 0x38, 0x67,
	0x24, 0x66, 0xe6, 0x09, 0x59, 0x70, 0xf1, 0x20, 0xfb, 0x4f, 0x82, 0x51, 0x81, 0x19, 0xa7, 0x41,
	0x28, 0x2a, 0x95, 0x9c, 0xb8, 0xb8, 0xc0, 0xe6, 0x04, 0xe5, 0xe7, 0x97, 0x14, 0x0b, 0x99, 0x70,
	0x71, 0x95, 0xc0, 0x79, 0x78, 0x4d, 0x41, 0x52, 0x67, 0x74, 0x93, 0x91, 0x8b, 0x07, 0x49, 0xae,
	0x58, 0x28, 0x94, 0x4b, 0x0c, 0xe4, 0x45, 0x64, 0xf5, 0xa9, 0x85, 0xa5, 0xa9, 0xc5, 0x25, 0x42,
	0xd2, 0xd8, 0x0c, 0x83, 0x06, 0x9b, 0x94, 0x0c, 0x6e, 0xc9, 0xe2, 0x02, 0x25, 0x06, 0x21, 0x2b,
	0x2e, 0x7e, 0xf7, 0xd4, 0x12, 0x14, 0x8f, 0xf3, 0xc1, 0xb5, 0x80, 0x23, 0x50, 0x4a, 0x14, 0x61,
	0x04, 0x92, 0x32, 0x25, 0x06, 0x21, 0x33, 0x2e, 0x5e, 0xf7, 0xd4, 0x12, 0x24, 0xaf, 0xa2, 0xeb,
	0x14, 0x86, 0xf3, 0x11, 0x8a, 0x94, 0x18, 0x9c, 0x04, 0x4e, 0x3c, 0x92, 0x63, 0xbc, 0xf0, 0x48,
	0x8e, 0xf1, 0xc1, 0x23, 0x39, 0xc6, 0x19, 0x8f, 0xe5, 0x18, 0x92, 0xd8, 0xc0, 0x89, 0xc7, 0x18,
	0x10, 0x00, 0x00, 0xff, 0xff, 0x39, 0xab, 0x91, 0x73, 0x52, 0x02, 0x00, 0x00,
}
