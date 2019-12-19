// Code generated by protoc-gen-go. DO NOT EDIT.
// source: event.proto

package halproto

import proto "github.com/golang/protobuf/proto"
import fmt "fmt"
import math "math"

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

// events that HAL generates and app(s) can listen to
type EventId int32

const (
	EventId_EVENT_ID_NONE            EventId = 0
	EventId_EVENT_ID_PORT_STATE      EventId = 1
	EventId_EVENT_ID_LIF_ADD_UPDATE  EventId = 2
	EventId_EVENT_ID_MICRO_SEG_STATE EventId = 3
)

var EventId_name = map[int32]string{
	0: "EVENT_ID_NONE",
	1: "EVENT_ID_PORT_STATE",
	2: "EVENT_ID_LIF_ADD_UPDATE",
	3: "EVENT_ID_MICRO_SEG_STATE",
}
var EventId_value = map[string]int32{
	"EVENT_ID_NONE":            0,
	"EVENT_ID_PORT_STATE":      1,
	"EVENT_ID_LIF_ADD_UPDATE":  2,
	"EVENT_ID_MICRO_SEG_STATE": 3,
}

func (x EventId) String() string {
	return proto.EnumName(EventId_name, int32(x))
}
func (EventId) EnumDescriptor() ([]byte, []int) { return fileDescriptor7, []int{0} }

type EventOp int32

const (
	EventOp_EVENT_OP_NONE        EventOp = 0
	EventOp_EVENT_OP_SUBSCRIBE   EventOp = 1
	EventOp_EVENT_OP_UNSUBSCRIBE EventOp = 2
)

var EventOp_name = map[int32]string{
	0: "EVENT_OP_NONE",
	1: "EVENT_OP_SUBSCRIBE",
	2: "EVENT_OP_UNSUBSCRIBE",
}
var EventOp_value = map[string]int32{
	"EVENT_OP_NONE":        0,
	"EVENT_OP_SUBSCRIBE":   1,
	"EVENT_OP_UNSUBSCRIBE": 2,
}

func (x EventOp) String() string {
	return proto.EnumName(EventOp_name, int32(x))
}
func (EventOp) EnumDescriptor() ([]byte, []int) { return fileDescriptor7, []int{1} }

// EventSpec captures the event of interest to the app
type EventRequest struct {
	EventId        EventId `protobuf:"varint,1,opt,name=event_id,json=eventId,enum=event.EventId" json:"event_id,omitempty"`
	EventOperation EventOp `protobuf:"varint,2,opt,name=event_operation,json=eventOperation,enum=event.EventOp" json:"event_operation,omitempty"`
}

func (m *EventRequest) Reset()                    { *m = EventRequest{} }
func (m *EventRequest) String() string            { return proto.CompactTextString(m) }
func (*EventRequest) ProtoMessage()               {}
func (*EventRequest) Descriptor() ([]byte, []int) { return fileDescriptor7, []int{0} }

func (m *EventRequest) GetEventId() EventId {
	if m != nil {
		return m.EventId
	}
	return EventId_EVENT_ID_NONE
}

func (m *EventRequest) GetEventOperation() EventOp {
	if m != nil {
		return m.EventOperation
	}
	return EventOp_EVENT_OP_NONE
}

type EventResponse struct {
	ApiStatus ApiStatus `protobuf:"varint,1,opt,name=api_status,json=apiStatus,enum=types.ApiStatus" json:"api_status,omitempty"`
	EventId   EventId   `protobuf:"varint,2,opt,name=event_id,json=eventId,enum=event.EventId" json:"event_id,omitempty"`
	// Types that are valid to be assigned to EventInfo:
	//	*EventResponse_LifEvent
	//	*EventResponse_PortEvent
	//	*EventResponse_MicroSegEvent
	EventInfo isEventResponse_EventInfo `protobuf_oneof:"event_info"`
}

func (m *EventResponse) Reset()                    { *m = EventResponse{} }
func (m *EventResponse) String() string            { return proto.CompactTextString(m) }
func (*EventResponse) ProtoMessage()               {}
func (*EventResponse) Descriptor() ([]byte, []int) { return fileDescriptor7, []int{1} }

type isEventResponse_EventInfo interface{ isEventResponse_EventInfo() }

type EventResponse_LifEvent struct {
	LifEvent *LifGetResponse `protobuf:"bytes,3,opt,name=lif_event,json=lifEvent,oneof"`
}
type EventResponse_PortEvent struct {
	PortEvent *PortResponse `protobuf:"bytes,4,opt,name=port_event,json=portEvent,oneof"`
}
type EventResponse_MicroSegEvent struct {
	MicroSegEvent *MicroSegEvent `protobuf:"bytes,5,opt,name=micro_seg_event,json=microSegEvent,oneof"`
}

func (*EventResponse_LifEvent) isEventResponse_EventInfo()      {}
func (*EventResponse_PortEvent) isEventResponse_EventInfo()     {}
func (*EventResponse_MicroSegEvent) isEventResponse_EventInfo() {}

func (m *EventResponse) GetEventInfo() isEventResponse_EventInfo {
	if m != nil {
		return m.EventInfo
	}
	return nil
}

func (m *EventResponse) GetApiStatus() ApiStatus {
	if m != nil {
		return m.ApiStatus
	}
	return ApiStatus_API_STATUS_OK
}

func (m *EventResponse) GetEventId() EventId {
	if m != nil {
		return m.EventId
	}
	return EventId_EVENT_ID_NONE
}

func (m *EventResponse) GetLifEvent() *LifGetResponse {
	if x, ok := m.GetEventInfo().(*EventResponse_LifEvent); ok {
		return x.LifEvent
	}
	return nil
}

func (m *EventResponse) GetPortEvent() *PortResponse {
	if x, ok := m.GetEventInfo().(*EventResponse_PortEvent); ok {
		return x.PortEvent
	}
	return nil
}

func (m *EventResponse) GetMicroSegEvent() *MicroSegEvent {
	if x, ok := m.GetEventInfo().(*EventResponse_MicroSegEvent); ok {
		return x.MicroSegEvent
	}
	return nil
}

// XXX_OneofFuncs is for the internal use of the proto package.
func (*EventResponse) XXX_OneofFuncs() (func(msg proto.Message, b *proto.Buffer) error, func(msg proto.Message, tag, wire int, b *proto.Buffer) (bool, error), func(msg proto.Message) (n int), []interface{}) {
	return _EventResponse_OneofMarshaler, _EventResponse_OneofUnmarshaler, _EventResponse_OneofSizer, []interface{}{
		(*EventResponse_LifEvent)(nil),
		(*EventResponse_PortEvent)(nil),
		(*EventResponse_MicroSegEvent)(nil),
	}
}

func _EventResponse_OneofMarshaler(msg proto.Message, b *proto.Buffer) error {
	m := msg.(*EventResponse)
	// event_info
	switch x := m.EventInfo.(type) {
	case *EventResponse_LifEvent:
		b.EncodeVarint(3<<3 | proto.WireBytes)
		if err := b.EncodeMessage(x.LifEvent); err != nil {
			return err
		}
	case *EventResponse_PortEvent:
		b.EncodeVarint(4<<3 | proto.WireBytes)
		if err := b.EncodeMessage(x.PortEvent); err != nil {
			return err
		}
	case *EventResponse_MicroSegEvent:
		b.EncodeVarint(5<<3 | proto.WireBytes)
		if err := b.EncodeMessage(x.MicroSegEvent); err != nil {
			return err
		}
	case nil:
	default:
		return fmt.Errorf("EventResponse.EventInfo has unexpected type %T", x)
	}
	return nil
}

func _EventResponse_OneofUnmarshaler(msg proto.Message, tag, wire int, b *proto.Buffer) (bool, error) {
	m := msg.(*EventResponse)
	switch tag {
	case 3: // event_info.lif_event
		if wire != proto.WireBytes {
			return true, proto.ErrInternalBadWireType
		}
		msg := new(LifGetResponse)
		err := b.DecodeMessage(msg)
		m.EventInfo = &EventResponse_LifEvent{msg}
		return true, err
	case 4: // event_info.port_event
		if wire != proto.WireBytes {
			return true, proto.ErrInternalBadWireType
		}
		msg := new(PortResponse)
		err := b.DecodeMessage(msg)
		m.EventInfo = &EventResponse_PortEvent{msg}
		return true, err
	case 5: // event_info.micro_seg_event
		if wire != proto.WireBytes {
			return true, proto.ErrInternalBadWireType
		}
		msg := new(MicroSegEvent)
		err := b.DecodeMessage(msg)
		m.EventInfo = &EventResponse_MicroSegEvent{msg}
		return true, err
	default:
		return false, nil
	}
}

func _EventResponse_OneofSizer(msg proto.Message) (n int) {
	m := msg.(*EventResponse)
	// event_info
	switch x := m.EventInfo.(type) {
	case *EventResponse_LifEvent:
		s := proto.Size(x.LifEvent)
		n += proto.SizeVarint(3<<3 | proto.WireBytes)
		n += proto.SizeVarint(uint64(s))
		n += s
	case *EventResponse_PortEvent:
		s := proto.Size(x.PortEvent)
		n += proto.SizeVarint(4<<3 | proto.WireBytes)
		n += proto.SizeVarint(uint64(s))
		n += s
	case *EventResponse_MicroSegEvent:
		s := proto.Size(x.MicroSegEvent)
		n += proto.SizeVarint(5<<3 | proto.WireBytes)
		n += proto.SizeVarint(uint64(s))
		n += s
	case nil:
	default:
		panic(fmt.Sprintf("proto: unexpected type %T in oneof", x))
	}
	return n
}

func init() {
	proto.RegisterType((*EventRequest)(nil), "halproto.EventRequest")
	proto.RegisterType((*EventResponse)(nil), "halproto.EventResponse")
	proto.RegisterEnum("halproto.EventId", EventId_name, EventId_value)
	proto.RegisterEnum("halproto.EventOp", EventOp_name, EventOp_value)
}

func init() { proto.RegisterFile("event.proto", fileDescriptor7) }

var fileDescriptor7 = []byte{
	// 443 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x7c, 0x92, 0xcf, 0x6f, 0xd3, 0x30,
	0x14, 0xc7, 0x9b, 0x8c, 0xb2, 0xf6, 0xf5, 0x27, 0x5e, 0xc5, 0xa2, 0xc2, 0x61, 0xea, 0x69, 0xec,
	0x10, 0x50, 0x7b, 0xe0, 0xc2, 0xa5, 0x5d, 0xc3, 0x16, 0xa9, 0x4b, 0x2a, 0x27, 0xe5, 0xc0, 0xc5,
	0x0a, 0x9b, 0x33, 0x2c, 0xb5, 0xb1, 0x89, 0x3d, 0xa4, 0xfe, 0x39, 0xfc, 0xa7, 0x28, 0xb6, 0x9b,
	0x75, 0x42, 0xe2, 0x94, 0xe7, 0xef, 0x7b, 0x1f, 0x7f, 0xdf, 0x7b, 0x31, 0x74, 0xe8, 0x6f, 0x5a,
	0x28, 0x5f, 0x94, 0x5c, 0x71, 0xd4, 0xd4, 0x87, 0x71, 0x47, 0xed, 0x05, 0x95, 0x46, 0x1b, 0x0f,
	0x58, 0xa1, 0x68, 0x99, 0x67, 0xf7, 0xd4, 0x0a, 0x20, 0x78, 0x69, 0x81, 0x71, 0x57, 0xee, 0xa5,
	0xa2, 0x3b, 0x73, 0x9a, 0x94, 0xd0, 0x0d, 0xaa, 0x0b, 0x30, 0xfd, 0xf5, 0x44, 0xa5, 0x42, 0x1f,
	0xa0, 0xa5, 0x2f, 0x24, 0xec, 0xc1, 0x73, 0x2e, 0x9c, 0xcb, 0xfe, 0xb4, 0xef, 0x1b, 0x3b, 0x5d,
	0x16, 0x3e, 0xe0, 0x53, 0x6a, 0x02, 0xf4, 0x19, 0x06, 0xa6, 0x94, 0x0b, 0x5a, 0x66, 0x8a, 0xf1,
	0xc2, 0x73, 0xff, 0x25, 0x62, 0x81, 0xfb, 0xd4, 0x04, 0xb6, 0x6a, 0xf2, 0xc7, 0x85, 0x9e, 0x35,
	0x95, 0x82, 0x17, 0x92, 0xa2, 0x8f, 0x00, 0x99, 0x60, 0x44, 0xaa, 0x4c, 0x3d, 0x49, 0xeb, 0x3b,
	0xf4, 0xcd, 0x48, 0x73, 0xc1, 0x12, 0xad, 0xe3, 0x76, 0x76, 0x08, 0x5f, 0xb4, 0xe9, 0xfe, 0xbf,
	0xcd, 0x19, 0xb4, 0xb7, 0x2c, 0x27, 0xfa, 0xe8, 0x9d, 0x5c, 0x38, 0x97, 0x9d, 0xe9, 0xc8, 0x67,
	0x85, 0xca, 0xfd, 0x15, 0xcb, 0x6f, 0x68, 0xdd, 0xc4, 0x6d, 0x03, 0xb7, 0xb6, 0x2c, 0xd7, 0x3c,
	0x9a, 0x81, 0x5e, 0x99, 0xa5, 0x5e, 0x69, 0x0a, 0xf9, 0x7a, 0x8b, 0x6b, 0x5e, 0x1e, 0x33, 0xed,
	0x4a, 0x34, 0xd0, 0x17, 0x18, 0xec, 0xd8, 0x7d, 0xc9, 0x89, 0xa4, 0x8f, 0x96, 0x6c, 0x5a, 0x52,
	0xee, 0xa5, 0x7f, 0x57, 0xe5, 0x12, 0xfa, 0xa8, 0x8b, 0x6f, 0x1b, 0xb8, 0xb7, 0x3b, 0x16, 0x16,
	0x5d, 0x00, 0x3b, 0x52, 0x91, 0xf3, 0x2b, 0x01, 0xa7, 0x76, 0x12, 0xf4, 0x06, 0x7a, 0xc1, 0xb7,
	0x20, 0x4a, 0x49, 0xb8, 0x24, 0x51, 0x1c, 0x05, 0xc3, 0x06, 0x3a, 0x87, 0xb3, 0x5a, 0x5a, 0xc7,
	0x38, 0x25, 0x49, 0x3a, 0x4f, 0x83, 0xa1, 0x83, 0xde, 0xc1, 0x79, 0x9d, 0x58, 0x85, 0x5f, 0xc9,
	0x7c, 0xb9, 0x24, 0x9b, 0xf5, 0xb2, 0x4a, 0xba, 0xe8, 0x3d, 0x78, 0x75, 0xf2, 0x2e, 0xbc, 0xc6,
	0x31, 0x49, 0x82, 0x1b, 0x8b, 0x9e, 0x5c, 0x45, 0xd6, 0x31, 0x16, 0xcf, 0x8e, 0xf1, 0xfa, 0xe0,
	0xf8, 0x16, 0x50, 0x2d, 0x25, 0x9b, 0x45, 0x72, 0x8d, 0xc3, 0x45, 0x65, 0xe8, 0xc1, 0xa8, 0xd6,
	0x37, 0xd1, 0x73, 0xc6, 0x9d, 0x06, 0xd0, 0x3c, 0xac, 0xa5, 0xa3, 0x83, 0x15, 0x93, 0x8a, 0x16,
	0xe8, 0xec, 0xf8, 0x47, 0xd9, 0x67, 0x37, 0x1e, 0xbd, 0x14, 0xcd, 0x76, 0x27, 0x8d, 0x4f, 0xce,
	0x02, 0xbe, 0xb7, 0x7e, 0x66, 0x5b, 0xfd, 0x58, 0x7f, 0xbc, 0xd6, 0x9f, 0xd9, 0xdf, 0x00, 0x00,
	0x00, 0xff, 0xff, 0x68, 0xe2, 0xd0, 0x39, 0x01, 0x03, 0x00, 0x00,
}
