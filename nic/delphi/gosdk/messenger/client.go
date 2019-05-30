package messenger

import (
	"fmt"
	"log"
	"net"

	"github.com/golang/protobuf/descriptor"
	"github.com/golang/protobuf/proto"

	"github.com/pensando/sw/nic/delphi/messenger/proto"
	"github.com/pensando/sw/nic/delphi/proto/delphi"
)

// ServerAddress is the hub address
const ServerAddress = "127.0.0.1"

// ServerPort is the hub port
const ServerPort = 7001

// Handler is the interface the messenger clients have to implement
type Handler interface {
	HandleMountResp(svcID uint16, status string,
		objlist []*delphi_messenger.ObjectData) error
	HandleNotify(objlist []*delphi_messenger.ObjectData) error
	HandleStatusResp() error
}

// Client in the interface of the messenger
type Client interface {
	Dial() error
	IsConnected() bool
	SendMountReq(svcName string, mounts []*delphi_messenger.MountData) error
	SendChangeReq(objlist []*delphi_messenger.ObjectData) error
	Close()
}

type client struct {
	connection net.Conn
	receiver   *receiver
	handler    Handler
	msgid      uint64
	svcid      uint32
}

// NewClient creates a new messenger instance
func NewClient(handler Handler) (Client, error) {
	client := &client{
		handler: handler,
	}

	return client, nil
}

func (c *client) Dial() error {
	conn, err := net.Dial("tcp", fmt.Sprintf("%s:%d", ServerAddress, ServerPort))
	if err != nil {
		return err
	}

	c.connection = conn
	c.receiver, err = newReceiver(conn, c)
	if err != nil {
		panic(err)
	}

	return nil
}

func (c *client) IsConnected() bool {
	return true
}

func (c *client) sendMessage(msgType delphi_messenger.MessageType, objlist []*delphi_messenger.ObjectData) error {
	c.msgid++
	message := delphi_messenger.Message{
		Type:      msgType,
		MessageId: c.msgid,
		Objects:   objlist,
	}

	e := make([]byte, 0)
	buffer := proto.NewBuffer(e)

	buffer.EncodeMessage(&message)

	_, err := c.connection.Write(buffer.Bytes())
	if err != nil {
		log.Printf("Error sending message. Err: %v", err)
		return err
	}

	return nil
}

func (c *client) SendMountReq(serviceName string, mounts []*delphi_messenger.MountData) error {

	mountRequest := delphi_messenger.MountReqMsg{
		ServiceName: serviceName,
		ServiceID:   c.svcid,
		Mounts:      mounts,
	}

	data, err := proto.Marshal(&mountRequest)
	if err != nil {
		panic(err)
	}

	_, desc := descriptor.ForMessage(&mountRequest)
	objects := []*delphi_messenger.ObjectData{
		&delphi_messenger.ObjectData{
			Meta: &delphi.ObjectMeta{
				Kind: *desc.Name,
			},
			Data: data,
		},
	}

	return c.sendMessage(delphi_messenger.MessageType_MountReq, objects)
}

func (c *client) SendChangeReq(objlist []*delphi_messenger.ObjectData) error {

	return c.sendMessage(delphi_messenger.MessageType_ChangeReq, objlist)
}

func (c *client) Close() {
	if c.connection != nil {
		c.connection.Close()
	}
}

// HandleMessage is responsible for halding messages received from the receiver
func (c *client) HandleMessage(message *delphi_messenger.Message) error {
	switch message.GetType() {
	case delphi_messenger.MessageType_MountResp:
		objects := message.GetObjects()
		if len(objects) != 1 {
			panic(len(objects))
		}
		var mountResp delphi_messenger.MountRespMsg
		proto.Unmarshal(objects[0].GetData(), &mountResp)
		c.svcid = mountResp.GetServiceID()
		c.handler.HandleMountResp(uint16(c.svcid),
			message.GetStatus(), mountResp.GetObjects())

	case delphi_messenger.MessageType_Notify:
		c.handler.HandleNotify(message.GetObjects())

	case delphi_messenger.MessageType_StatusResp:
		c.handler.HandleStatusResp()

	default:
		panic(message.GetType())
	}
	return nil
}

// Implementing the transportHandler interface for the receiver
func (c *client) SocketClosed() {
}

func (c *client) loopForever() {
}
