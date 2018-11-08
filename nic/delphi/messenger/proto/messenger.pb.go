// Code generated by protoc-gen-go. DO NOT EDIT.
// source: messenger.proto

/*
Package delphi_messenger is a generated protocol buffer package.

It is generated from these files:
	messenger.proto

It has these top-level messages:
	ObjectData
	MountData
	MountReqMsg
	MountRespMsg
	Message
	TestKey
	TestObject
*/
package delphi_messenger

import proto "github.com/golang/protobuf/proto"
import fmt "fmt"
import math "math"
import delphi "github.com/pensando/sw/nic/delphi/proto/delphi"

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

// This is a compile-time assertion to ensure that this generated file
// is compatible with the proto package it is being compiled against.
// A compilation error at this line likely means your copy of the
// proto package needs to be updated.
const _ = proto.ProtoPackageIsVersion2 // please upgrade the proto package

// message types
type MessageType int32

const (
	MessageType_Invalid    MessageType = 0
	MessageType_Notify     MessageType = 1
	MessageType_ChangeReq  MessageType = 2
	MessageType_StatusResp MessageType = 3
	MessageType_GetReq     MessageType = 4
	MessageType_GetResp    MessageType = 5
	MessageType_ListReq    MessageType = 6
	MessageType_ListResp   MessageType = 7
	MessageType_MountReq   MessageType = 8
	MessageType_MountResp  MessageType = 9
)

var MessageType_name = map[int32]string{
	0: "Invalid",
	1: "Notify",
	2: "ChangeReq",
	3: "StatusResp",
	4: "GetReq",
	5: "GetResp",
	6: "ListReq",
	7: "ListResp",
	8: "MountReq",
	9: "MountResp",
}
var MessageType_value = map[string]int32{
	"Invalid":    0,
	"Notify":     1,
	"ChangeReq":  2,
	"StatusResp": 3,
	"GetReq":     4,
	"GetResp":    5,
	"ListReq":    6,
	"ListResp":   7,
	"MountReq":   8,
	"MountResp":  9,
}

func (x MessageType) String() string {
	return proto.EnumName(MessageType_name, int32(x))
}
func (MessageType) EnumDescriptor() ([]byte, []int) { return fileDescriptor0, []int{0} }

// message container for one object
type ObjectData struct {
	Meta *delphi.ObjectMeta     `protobuf:"bytes,1,opt,name=Meta" json:"Meta,omitempty"`
	Op   delphi.ObjectOperation `protobuf:"varint,2,opt,name=Op,enum=delphi.ObjectOperation" json:"Op,omitempty"`
	Data []byte                 `protobuf:"bytes,3,opt,name=Data,proto3" json:"Data,omitempty"`
}

func (m *ObjectData) Reset()                    { *m = ObjectData{} }
func (m *ObjectData) String() string            { return proto.CompactTextString(m) }
func (*ObjectData) ProtoMessage()               {}
func (*ObjectData) Descriptor() ([]byte, []int) { return fileDescriptor0, []int{0} }

func (m *ObjectData) GetMeta() *delphi.ObjectMeta {
	if m != nil {
		return m.Meta
	}
	return nil
}

func (m *ObjectData) GetOp() delphi.ObjectOperation {
	if m != nil {
		return m.Op
	}
	return delphi.ObjectOperation_InvalidOp
}

func (m *ObjectData) GetData() []byte {
	if m != nil {
		return m.Data
	}
	return nil
}

// mount data
type MountData struct {
	Kind string           `protobuf:"bytes,1,opt,name=Kind" json:"Kind,omitempty"`
	Key  string           `protobuf:"bytes,2,opt,name=Key" json:"Key,omitempty"`
	Mode delphi.MountMode `protobuf:"varint,3,opt,name=Mode,enum=delphi.MountMode" json:"Mode,omitempty"`
}

func (m *MountData) Reset()                    { *m = MountData{} }
func (m *MountData) String() string            { return proto.CompactTextString(m) }
func (*MountData) ProtoMessage()               {}
func (*MountData) Descriptor() ([]byte, []int) { return fileDescriptor0, []int{1} }

func (m *MountData) GetKind() string {
	if m != nil {
		return m.Kind
	}
	return ""
}

func (m *MountData) GetKey() string {
	if m != nil {
		return m.Key
	}
	return ""
}

func (m *MountData) GetMode() delphi.MountMode {
	if m != nil {
		return m.Mode
	}
	return delphi.MountMode_InvalidMode
}

// mount request
type MountReqMsg struct {
	ServiceName string       `protobuf:"bytes,1,opt,name=ServiceName" json:"ServiceName,omitempty"`
	ServiceID   uint32       `protobuf:"varint,2,opt,name=ServiceID" json:"ServiceID,omitempty"`
	Mounts      []*MountData `protobuf:"bytes,3,rep,name=Mounts" json:"Mounts,omitempty"`
}

func (m *MountReqMsg) Reset()                    { *m = MountReqMsg{} }
func (m *MountReqMsg) String() string            { return proto.CompactTextString(m) }
func (*MountReqMsg) ProtoMessage()               {}
func (*MountReqMsg) Descriptor() ([]byte, []int) { return fileDescriptor0, []int{2} }

func (m *MountReqMsg) GetServiceName() string {
	if m != nil {
		return m.ServiceName
	}
	return ""
}

func (m *MountReqMsg) GetServiceID() uint32 {
	if m != nil {
		return m.ServiceID
	}
	return 0
}

func (m *MountReqMsg) GetMounts() []*MountData {
	if m != nil {
		return m.Mounts
	}
	return nil
}

// mount response
type MountRespMsg struct {
	ServiceName string        `protobuf:"bytes,1,opt,name=ServiceName" json:"ServiceName,omitempty"`
	ServiceID   uint32        `protobuf:"varint,2,opt,name=ServiceID" json:"ServiceID,omitempty"`
	Objects     []*ObjectData `protobuf:"bytes,6,rep,name=Objects" json:"Objects,omitempty"`
}

func (m *MountRespMsg) Reset()                    { *m = MountRespMsg{} }
func (m *MountRespMsg) String() string            { return proto.CompactTextString(m) }
func (*MountRespMsg) ProtoMessage()               {}
func (*MountRespMsg) Descriptor() ([]byte, []int) { return fileDescriptor0, []int{3} }

func (m *MountRespMsg) GetServiceName() string {
	if m != nil {
		return m.ServiceName
	}
	return ""
}

func (m *MountRespMsg) GetServiceID() uint32 {
	if m != nil {
		return m.ServiceID
	}
	return 0
}

func (m *MountRespMsg) GetObjects() []*ObjectData {
	if m != nil {
		return m.Objects
	}
	return nil
}

// all messages exchanged by delphi components are in this format
type Message struct {
	Type       MessageType   `protobuf:"varint,1,opt,name=Type,enum=delphi.messenger.MessageType" json:"Type,omitempty"`
	MessageId  uint64        `protobuf:"varint,2,opt,name=MessageId" json:"MessageId,omitempty"`
	ResponseTo uint64        `protobuf:"varint,3,opt,name=ResponseTo" json:"ResponseTo,omitempty"`
	Status     string        `protobuf:"bytes,4,opt,name=Status" json:"Status,omitempty"`
	Objects    []*ObjectData `protobuf:"bytes,6,rep,name=Objects" json:"Objects,omitempty"`
}

func (m *Message) Reset()                    { *m = Message{} }
func (m *Message) String() string            { return proto.CompactTextString(m) }
func (*Message) ProtoMessage()               {}
func (*Message) Descriptor() ([]byte, []int) { return fileDescriptor0, []int{4} }

func (m *Message) GetType() MessageType {
	if m != nil {
		return m.Type
	}
	return MessageType_Invalid
}

func (m *Message) GetMessageId() uint64 {
	if m != nil {
		return m.MessageId
	}
	return 0
}

func (m *Message) GetResponseTo() uint64 {
	if m != nil {
		return m.ResponseTo
	}
	return 0
}

func (m *Message) GetStatus() string {
	if m != nil {
		return m.Status
	}
	return ""
}

func (m *Message) GetObjects() []*ObjectData {
	if m != nil {
		return m.Objects
	}
	return nil
}

type TestKey struct {
	Idx uint32 `protobuf:"varint,1,opt,name=Idx" json:"Idx,omitempty"`
}

func (m *TestKey) Reset()                    { *m = TestKey{} }
func (m *TestKey) String() string            { return proto.CompactTextString(m) }
func (*TestKey) ProtoMessage()               {}
func (*TestKey) Descriptor() ([]byte, []int) { return fileDescriptor0, []int{5} }

func (m *TestKey) GetIdx() uint32 {
	if m != nil {
		return m.Idx
	}
	return 0
}

type TestObject struct {
	Meta      *delphi.ObjectMeta `protobuf:"bytes,1,opt,name=Meta" json:"Meta,omitempty"`
	Key       *TestKey           `protobuf:"bytes,2,opt,name=Key" json:"Key,omitempty"`
	TestData1 string             `protobuf:"bytes,3,opt,name=TestData1" json:"TestData1,omitempty"`
	TestData2 string             `protobuf:"bytes,4,opt,name=TestData2" json:"TestData2,omitempty"`
	TestData3 string             `protobuf:"bytes,5,opt,name=TestData3" json:"TestData3,omitempty"`
	TestData4 string             `protobuf:"bytes,6,opt,name=TestData4" json:"TestData4,omitempty"`
	TestData5 string             `protobuf:"bytes,7,opt,name=TestData5" json:"TestData5,omitempty"`
}

func (m *TestObject) Reset()                    { *m = TestObject{} }
func (m *TestObject) String() string            { return proto.CompactTextString(m) }
func (*TestObject) ProtoMessage()               {}
func (*TestObject) Descriptor() ([]byte, []int) { return fileDescriptor0, []int{6} }

func (m *TestObject) GetMeta() *delphi.ObjectMeta {
	if m != nil {
		return m.Meta
	}
	return nil
}

func (m *TestObject) GetKey() *TestKey {
	if m != nil {
		return m.Key
	}
	return nil
}

func (m *TestObject) GetTestData1() string {
	if m != nil {
		return m.TestData1
	}
	return ""
}

func (m *TestObject) GetTestData2() string {
	if m != nil {
		return m.TestData2
	}
	return ""
}

func (m *TestObject) GetTestData3() string {
	if m != nil {
		return m.TestData3
	}
	return ""
}

func (m *TestObject) GetTestData4() string {
	if m != nil {
		return m.TestData4
	}
	return ""
}

func (m *TestObject) GetTestData5() string {
	if m != nil {
		return m.TestData5
	}
	return ""
}

func init() {
	proto.RegisterType((*ObjectData)(nil), "delphi.messenger.ObjectData")
	proto.RegisterType((*MountData)(nil), "delphi.messenger.MountData")
	proto.RegisterType((*MountReqMsg)(nil), "delphi.messenger.MountReqMsg")
	proto.RegisterType((*MountRespMsg)(nil), "delphi.messenger.MountRespMsg")
	proto.RegisterType((*Message)(nil), "delphi.messenger.Message")
	proto.RegisterType((*TestKey)(nil), "delphi.messenger.TestKey")
	proto.RegisterType((*TestObject)(nil), "delphi.messenger.TestObject")
	proto.RegisterEnum("delphi.messenger.MessageType", MessageType_name, MessageType_value)
}

func init() { proto.RegisterFile("messenger.proto", fileDescriptor0) }

var fileDescriptor0 = []byte{
	// 537 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0xac, 0x54, 0xc1, 0x6e, 0xd3, 0x5a,
	0x10, 0x7d, 0x8e, 0x5d, 0xbb, 0x1e, 0x27, 0x79, 0x97, 0xbb, 0x00, 0x43, 0x03, 0x8a, 0x2c, 0x01,
	0x11, 0x48, 0x91, 0xea, 0xb4, 0xfc, 0x00, 0x95, 0x50, 0x54, 0xdc, 0x48, 0xb7, 0x59, 0xb0, 0x75,
	0xeb, 0x21, 0x35, 0x6a, 0x6d, 0x27, 0xd7, 0xad, 0xc8, 0x96, 0x05, 0x9f, 0xc0, 0x47, 0xf1, 0x43,
	0x6c, 0xd1, 0x8c, 0x9d, 0xd8, 0xa1, 0x2c, 0x10, 0x62, 0x37, 0x73, 0xce, 0xd1, 0xcc, 0x1c, 0xcf,
	0x5c, 0xc3, 0xff, 0x37, 0xa8, 0x35, 0x66, 0x0b, 0x5c, 0x8d, 0x8b, 0x55, 0x5e, 0xe6, 0x52, 0x24,
	0x78, 0x5d, 0x5c, 0xa5, 0xe3, 0x2d, 0xfe, 0xa4, 0x5b, 0x23, 0xcc, 0x07, 0x4b, 0x80, 0xd9, 0xc5,
	0x27, 0xbc, 0x2c, 0x4f, 0xe2, 0x32, 0x96, 0x2f, 0xc0, 0x8a, 0xb0, 0x8c, 0x7d, 0x63, 0x68, 0x8c,
	0xbc, 0x50, 0x8e, 0x6b, 0x69, 0xa5, 0x20, 0x46, 0x31, 0x2f, 0x5f, 0x42, 0x67, 0x56, 0xf8, 0x9d,
	0xa1, 0x31, 0xea, 0x87, 0x8f, 0x76, 0x55, 0xb3, 0x02, 0x57, 0x71, 0x99, 0xe6, 0x99, 0xea, 0xcc,
	0x0a, 0x29, 0xc1, 0xa2, 0xc2, 0xbe, 0x39, 0x34, 0x46, 0x5d, 0xc5, 0x71, 0xf0, 0x01, 0xdc, 0x28,
	0xbf, 0xcd, 0xaa, 0x8e, 0x12, 0xac, 0xd3, 0x34, 0x4b, 0xb8, 0xa3, 0xab, 0x38, 0x96, 0x02, 0xcc,
	0x53, 0x5c, 0x73, 0x79, 0x57, 0x51, 0x28, 0x9f, 0x83, 0x15, 0xe5, 0x09, 0x72, 0x99, 0x7e, 0xf8,
	0x60, 0xd3, 0x91, 0xcb, 0x10, 0xa1, 0x98, 0x0e, 0xbe, 0x18, 0xe0, 0x31, 0xa6, 0x70, 0x19, 0xe9,
	0x85, 0x1c, 0x82, 0x77, 0x8e, 0xab, 0xbb, 0xf4, 0x12, 0xcf, 0xe2, 0x1b, 0xac, 0x7b, 0xb4, 0x21,
	0x39, 0x00, 0xb7, 0x4e, 0xa7, 0x27, 0xdc, 0xb0, 0xa7, 0x1a, 0x40, 0x4e, 0xc0, 0xe6, 0x72, 0xda,
	0x37, 0x87, 0xe6, 0xc8, 0x0b, 0x0f, 0xc6, 0xbf, 0x7e, 0xcd, 0xf1, 0xd6, 0x89, 0xaa, 0xa5, 0xc1,
	0x57, 0x03, 0xba, 0xf5, 0x10, 0xba, 0xf8, 0x17, 0x53, 0xbc, 0x01, 0xa7, 0xfa, 0xb4, 0xda, 0xb7,
	0x79, 0x8c, 0xc1, 0xfd, 0x31, 0x9a, 0x1d, 0xaa, 0x8d, 0x38, 0xf8, 0x6e, 0x80, 0x13, 0xa1, 0xd6,
	0xf1, 0x02, 0xe5, 0x21, 0x58, 0xf3, 0x75, 0x51, 0x35, 0xef, 0x87, 0x4f, 0x7f, 0xe3, 0xa3, 0x12,
	0x92, 0x48, 0xb1, 0x94, 0x86, 0xaa, 0xc1, 0x69, 0xc2, 0x43, 0x59, 0xaa, 0x01, 0xe4, 0x33, 0x00,
	0xf2, 0x97, 0x67, 0x1a, 0xe7, 0x39, 0xef, 0xc5, 0x52, 0x2d, 0x44, 0x3e, 0x04, 0xfb, 0xbc, 0x8c,
	0xcb, 0x5b, 0xed, 0x5b, 0xec, 0xb7, 0xce, 0xfe, 0xda, 0xcc, 0x01, 0x38, 0x73, 0xd4, 0x25, 0x1d,
	0x83, 0x00, 0x73, 0x9a, 0x7c, 0x66, 0x2b, 0x3d, 0x45, 0x61, 0xf0, 0xc3, 0x00, 0x20, 0xb6, 0x12,
	0xff, 0xf1, 0x15, 0xbf, 0x6e, 0xee, 0xcc, 0x0b, 0x1f, 0xdf, 0x9f, 0xa3, 0x6e, 0x58, 0x9d, 0xe0,
	0x00, 0x5c, 0xca, 0x69, 0xaa, 0x43, 0xf6, 0xeb, 0xaa, 0x06, 0x68, 0xb3, 0x61, 0xed, 0xb8, 0x01,
	0xda, 0xec, 0xc4, 0xdf, 0xdb, 0x65, 0x27, 0x6d, 0xf6, 0xc8, 0xb7, 0x77, 0xd9, 0xa3, 0x36, 0x7b,
	0xec, 0x3b, 0xbb, 0xec, 0xf1, 0xab, 0x6f, 0x74, 0xf1, 0xcd, 0xea, 0xa4, 0x07, 0xce, 0x34, 0xbb,
	0x8b, 0xaf, 0xd3, 0x44, 0xfc, 0x27, 0x01, 0xec, 0xb3, 0xbc, 0x4c, 0x3f, 0xae, 0x85, 0x21, 0x7b,
	0xe0, 0xbe, 0xbd, 0x8a, 0xb3, 0x05, 0x2a, 0x5c, 0x8a, 0x8e, 0xec, 0x03, 0x54, 0x0b, 0xa1, 0x95,
	0x09, 0x93, 0xa4, 0xef, 0x90, 0x9e, 0x8d, 0xb0, 0xa8, 0x06, 0xc7, 0xba, 0x10, 0x7b, 0x94, 0xbc,
	0x4f, 0x35, 0x33, 0xb6, 0xec, 0xc2, 0x7e, 0x95, 0xe8, 0x42, 0x38, 0x94, 0x6d, 0x1e, 0x9b, 0xd8,
	0xa7, 0x06, 0xdb, 0xab, 0x17, 0xee, 0x85, 0xcd, 0xbf, 0x97, 0xc9, 0xcf, 0x00, 0x00, 0x00, 0xff,
	0xff, 0x43, 0x5a, 0x86, 0xc8, 0x91, 0x04, 0x00, 0x00,
}
