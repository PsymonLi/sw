 #! /usr/bin/python3

import sys
import grpc
import bgp_pb2
import types_pb2
import socket
import struct
import defines

stub = None
channel = None

def process_response(req_msg, resp_msg):
    if (resp_msg.ApiStatus != types_pb2.API_STATUS_OK):
        print ("Op failed: %d" % resp_msg.ApiStatus)
        return
    if "GetResponse" in resp_msg.DESCRIPTOR.name:
        print ("Number of entries retrieved: %d" % len(resp_msg.Response))
        for idx in range(len(resp_msg.Response)):
            resp = resp_msg.Response[idx]
            st = resp.Status
            print ("\nEntry %d" %idx)
            print ("-----------------------");
            print ("Afi: %d" % st.Afi)
            print ("Safi: %d" % st.Safi)
            print ("Prefix-Len: %d" % len(st.Prefix))
            print ("Prefix: "),
            for i in st.Prefix:
                mm=int(i.encode('hex'),16)
                print(hex(mm) ),
            print
            print ("PrefixLen: %d" % st.PrefixLen)
            print ("RouteSource: %d" % st.RouteSource)
            print ("RouteSourceIndex: %d" % st.RouteSourceIndex)
            print ("PathID: %d" % st.PathID)
            print ("BestRoute: %d "% st.BestRoute)
            print ("ASPathStr: "),
            for i in st.ASPathStr:
                mm=int(i.encode('hex'),16)
                print(hex(mm) ),
            print
            print ("PathOrigId: %s"% st.PathOrigId)
            print ("NexthopAddr: "),
            for i in st.NextHopAddr:
                mm=int(i.encode('hex'), 16)
                print((mm) ),
            print
    else:
        print ("Op Success")
    return

def get_nlri():
    req = bgp_pb2.BGPNLRIPrefixGetRequest()
    resp =  stub.BGPNLRIPrefixGet(req)
    process_response(req, resp)
    return

def get_nlri_ext():
    req = bgp_pb2.BGPNLRIPrefixGetRequest()
    req.Filter.ExtComm = rt_str
    resp =  stub.BGPNLRIPrefixGet(req)
    process_response(req, resp)
    return

def get_nlri_route_type():
    req = bgp_pb2.BGPNLRIPrefixGetRequest()
    req.Filter.type = route_type
    resp =  stub.BGPNLRIPrefixGet(req)
    process_response(req, resp)
    return

def get_nlri_vnid():
    req = bgp_pb2.BGPNLRIPrefixGetRequest()
    req.Filter.Vnid = vnid
    resp =  stub.BGPNLRIPrefixGet(req)
    process_response(req, resp)
    return

def read_args():
    global rt_str
    global route_type
    global vnid
    rt_str = ""
    route_type = 0
    vnid = 0
    opt = int (sys.argv[1])
    if opt == 1:
        print ("ext-comm:: sys.argv[1]: %s"%sys.argv[2])
        rt = sys.argv[2]
        for x in rt.split(':'):
            rt_str = rt_str+chr(int(x))
    if opt == 2:
        print ("route-type:: sys.argv[1]: %s"%sys.argv[2])
        route_type = int (sys.argv[2])
    if opt == 3:
        print ("vnid:: sys.argv[1]: %s"%sys.argv[2])
        vnid = int (sys.argv[2])
    return

def init():
    global channel
    global stub
    server = 'localhost:' + str(defines.AGENT_GRPC_PORT)
    channel = grpc.insecure_channel(server)
    stub = bgp_pb2.BGPSvcStub(channel)
    return

def ip2long(ip):
    packedIP = socket.inet_aton(ip)
    return struct.unpack("!L", packedIP)[0]

def long2ip(addr):
    return socket.inet_ntoa(struct.pack('!L', addr))

def print_help():
    print ("Usage: %s <filter_type> <filter_data>" % sys.argv[0])
    print ("filter_type: 1: ext-comm\t2: route-type\t3: vnid")
    print ("no options mean no filter")
    print ("eg : %s 1 00:02:00:00:00:00:00:01"%sys.argv[0])
    print ("eg : %s 2 2"%sys.argv[0]) 
    print ("eg : %s"%sys.argv[0]) 
    return

if __name__ == '__main__':
    init()
    args = len (sys.argv) - 1
    if not args:
        print ("calling get_nlri")
        get_nlri()
    else:
        read_args()
        opt = int (sys.argv[1])
        if opt == 1:
            print ("calling get_nlri_ext")
            get_nlri_ext()
        elif opt == 2:
            print ("calling get_nlri_route_type")
            get_nlri_route_type()
        elif opt == 3:
            print ("calling get_nlri_vnid")
            get_nlri_vnid()
        else:
            print_help()
