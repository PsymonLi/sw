// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package rpcserver

import (
	"errors"
	"fmt"
	"time"

	"golang.org/x/net/context"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/netagent/protos/netproto"
	"github.com/pensando/sw/venice/ctrler/npm/statemgr"
	"github.com/pensando/sw/venice/utils/debug"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
)

const maxWatchEventsPerMesage = 100

// NetworkRPCServer is the network RPC server
type NetworkRPCServer struct {
	stateMgr   *statemgr.Statemgr // reference to network manager
	debugStats *debug.Stats
}

// GetNetwork returns an instance of network
func (n *NetworkRPCServer) GetNetwork(ctx context.Context, ometa *api.ObjectMeta) (*netproto.Network, error) {
	// find the network
	nw, err := n.stateMgr.FindNetwork(ometa.Tenant, ometa.Name)
	if err != nil {
		log.Errorf("Could not find the network %s|%s", ometa.Tenant, ometa.Name)
		return nil, fmt.Errorf("Could not find the network")
	}

	nt := netproto.Network{
		TypeMeta:   nw.TypeMeta,
		ObjectMeta: nw.ObjectMeta,
		Spec: netproto.NetworkSpec{
			IPv4Subnet:  nw.Spec.IPv4Subnet,
			IPv4Gateway: nw.Spec.IPv4Gateway,
			IPv6Subnet:  nw.Spec.IPv6Subnet,
			IPv6Gateway: nw.Spec.IPv6Gateway,
			VlanID:      nw.Spec.VlanID,
		},
		Status: netproto.NetworkStatus{
			AllocatedVlanID: nw.Spec.VlanID,
		},
	}
	return &nt, nil
}

// ListNetworks lists all networks matching object selector
func (n *NetworkRPCServer) ListNetworks(ctx context.Context, sel *api.ObjectMeta) (*netproto.NetworkList, error) {
	var netlist netproto.NetworkList
	// get all networks
	networks, err := n.stateMgr.ListNetworks()
	if err != nil {
		return nil, err
	}

	// walk all networks and add it to the list
	for _, net := range networks {
		nt := netproto.Network{
			TypeMeta:   net.TypeMeta,
			ObjectMeta: net.ObjectMeta,
			Spec: netproto.NetworkSpec{
				IPv4Subnet:  net.Spec.IPv4Subnet,
				IPv4Gateway: net.Spec.IPv4Gateway,
				IPv6Subnet:  net.Spec.IPv6Subnet,
				IPv6Gateway: net.Spec.IPv6Gateway,
				VlanID:      net.Spec.VlanID,
			},
			Status: netproto.NetworkStatus{
				AllocatedVlanID: net.Spec.VlanID,
			},
		}
		netlist.Networks = append(netlist.Networks, &nt)
	}

	return &netlist, nil
}

// WatchNetworks watches networks for changes and sends them as streaming rpc
func (n *NetworkRPCServer) WatchNetworks(sel *api.ObjectMeta, stream netproto.NetworkApi_WatchNetworksServer) error {
	// watch for changes
	watchChan := make(chan memdb.Event, memdb.WatchLen)
	defer close(watchChan)
	n.stateMgr.WatchObjects("Network", watchChan)
	defer n.stateMgr.StopWatchObjects("Network", watchChan)

	// first get a list of all networks
	networks, err := n.ListNetworks(context.Background(), sel)
	if err != nil {
		log.Errorf("Error getting a list of networks. Err: %v", err)
		return err
	}

	ctx := stream.Context()
	watchEvtList := netproto.NetworkEventList{}

	// send the objects out as a stream
	for _, net := range networks.Networks {
		watchEvtList.NetworkEvents = append(watchEvtList.NetworkEvents, &netproto.NetworkEvent{
			EventType: api.EventType_CreateEvent,
			Network:   *net,
		})
	}
	if len(watchEvtList.NetworkEvents) > 0 {
		err = stream.Send(&watchEvtList)
		if err != nil {
			log.Errorf("Error sending stream. Err: %v", err)
			return err
		}
	}

	// loop forever on watch channel
	for {
		select {
		// read from channel
		case evt, ok := <-watchChan:
			if !ok {
				log.Errorf("Error reading from channel. Closing watch")
				return errors.New("Error reading from channel")
			}
			watchEvtList = netproto.NetworkEventList{}
			done := false
			for done != true {
				// get event type from memdb event
				var etype api.EventType
				switch evt.EventType {
				case memdb.CreateEvent:
					etype = api.EventType_CreateEvent
				case memdb.UpdateEvent:
					etype = api.EventType_UpdateEvent
				case memdb.DeleteEvent:
					etype = api.EventType_DeleteEvent
				}

				// convert to network object
				net, err := statemgr.NetworkStateFromObj(evt.Obj)
				if err != nil {
					return err
				}

				// construct the netproto object
				watchEvt := netproto.NetworkEvent{
					EventType: etype,
					Network: netproto.Network{
						TypeMeta:   net.TypeMeta,
						ObjectMeta: net.ObjectMeta,
						Spec: netproto.NetworkSpec{
							IPv4Subnet:  net.Spec.IPv4Subnet,
							IPv4Gateway: net.Spec.IPv4Gateway,
							IPv6Subnet:  net.Spec.IPv6Subnet,
							IPv6Gateway: net.Spec.IPv6Gateway,
							VlanID:      net.Spec.VlanID,
						},
						Status: netproto.NetworkStatus{
							AllocatedVlanID: net.Spec.VlanID,
						},
					},
				}
				watchEvtList.NetworkEvents = append(watchEvtList.NetworkEvents, &watchEvt)

				if len(watchChan) > 0 && len(watchEvtList.NetworkEvents) < maxWatchEventsPerMesage {
					evt = <-watchChan
				} else {
					done = true
				}
			}
			err = stream.Send(&watchEvtList)
			if err != nil {
				log.Errorf("Error sending stream. Err: %v", err)
				return err
			}
			if len(watchEvtList.NetworkEvents) != maxWatchEventsPerMesage {
				time.Sleep(50 * time.Millisecond)
			}
		case <-ctx.Done():
			return ctx.Err()
		}
	}

	// done
}

// NewNetworkRPCServer returns a network RPC server
func NewNetworkRPCServer(stateMgr *statemgr.Statemgr, debugStats *debug.Stats) (*NetworkRPCServer, error) {
	return &NetworkRPCServer{stateMgr: stateMgr, debugStats: debugStats}, nil
}
