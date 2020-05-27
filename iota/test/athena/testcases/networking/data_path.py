#!/usr/bin/python3

import iota.harness.api as api
import json
import os
import time
from iota.test.athena.config.netagent.hw_push_config import AddWorkloads
from scapy.all import *
from scapy.contrib.mpls import MPLS

FLOWS = [ {'sip' : '10.0.0.1', 'dip' : '10.0.0.65', 'proto' : 'UDP', 
            'src_port' : 100, 'dst_port' : 200},
            {'sip' : '192.168.0.1', 'dip' : '192.168.0.100', 'proto' : 'UDP', 
            'src_port' : 5000, 'dst_port' : 6000},
            {'sip' : '100.0.0.1', 'dip' : '100.0.10.1', 'proto' : 'UDP', 
            'src_port' : 60, 'dst_port' : 80} ]

PROTO_NUM = {'UDP' : 17, 'TCP': 6, 'ICMP': 1}
SNIFF_TIMEOUT = 5
ATHENA_SEC_APP_RSTRT_SLEEP = 140 # secs

DEFAULT_PAYLOAD = 'abcdefghijklmnopqrstuvwzxyabcdefghijklmnopqrstuvwzxy'
DEFAULT_H2S_GEN_PKT_FILENAME = './h2s_pkt.pcap'
DEFAULT_H2S_RECV_PKT_FILENAME = './h2s_recv_pkt.pcap'
DEFAULT_S2H_RECV_PKT_FILENAME = './s2h_recv_pkt.pcap'
DEFAULT_S2H_GEN_PKT_FILENAME = './s2h_pkt.pcap'
DEFAULT_POLICY_JSON_FILENAME = './policy.json'
CURR_DIR = os.path.dirname(os.path.realpath(__file__))


class FlowInfo():

    def __init__(self):
        self.smac = None
        self.dmac = None
        self.sip = None
        self.dip = None
        self.proto = None
        self.src_port = None
        self.dst_port = None
        self.icmp_type = None
        self.icmp_code = None 

    # Setters
    def set_smac(self, smac):
        self.smac = smac

    def set_dmac(self, dmac):
        self.dmac = dmac

    def set_sip(self, sip):
        self.sip = sip

    def set_dip(self, dip):
        self.dip = dip

    def set_proto(self, proto):
        self.proto = proto

    def set_src_port(self, src_port):
        self.src_port = src_port

    def set_dst_port(self, dst_port):
        self.dst_port = dst_port

    def set_icmp_type(self, icmp_type):
        self.icmp_type = icmp_type

    def set_icmp_code(self, icmp_code):
        self.icmp_code = icmp_code

    # Getters
    def get_smac(self):
        return self.smac

    def get_dmac(self):
        return self.dmac

    def get_sip(self):
        return self.sip

    def get_dip(self):
        return self.dip

    def get_proto(self):
        return self.proto

    def get_src_port(self):
        return self.src_port

    def get_dst_port(self):
        return self.dst_port

    def get_icmp_type(self):
        return self.icmp_type

    def get_icmp_code(self):
        return self.icmp_code
    
    def display(self):
        return ('{{SIP: {}, DIP: {}, Prot: {}, SrcPort: {}, '
                'DstPort: {}}}'.format(self.sip, self.dip, self.proto,
                self.src_port, self.dst_port))

def Athena_sec_app_restart(tc):
    
    athena_sec_app_pid = None
    req = api.Trigger_CreateExecuteCommandsRequest()
    
    cmd = "mv /policy.json /data/policy.json"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node.Name(), cmd)
    
    cmd = "ps -aef | grep athena_app | grep soft-init | grep -v grep"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node.Name(), cmd)

    resp = api.Trigger(req)
    for cmd in resp.commands:
        api.PrintCommandResults(cmd)
        if "mv" in cmd.command and cmd.exit_code != 0:
            return api.types.status.FAILURE
    
        if "ps" in cmd.command and "athena_app" in cmd.stdout:
            athena_sec_app_pid = cmd.stdout.strip().split()[1]
    
    if athena_sec_app_pid:
        api.Logger.info("Athena sec app already running with pid %s. Kill and "
                "restart." % athena_sec_app_pid)
        
        req = api.Trigger_CreateExecuteCommandsRequest()
        api.Trigger_AddNaplesCommand(req, tc.bitw_node.Name(), "kill -SIGTERM "\
                                    + athena_sec_app_pid)
        resp = api.Trigger(req)
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)
            if cmd.exit_code != 0:
                return api.types.status.FAILURE

    req = api.Trigger_CreateExecuteCommandsRequest()
    cmd = "/nic/tools/start-sec-agent-iota.sh"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node.Name(), cmd, background = True)
    resp = api.Trigger(req)
    for cmd in resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            return api.types.status.FAILURE
    
    # sleep for init to complete
    time.sleep(ATHENA_SEC_APP_RSTRT_SLEEP)

    athena_sec_app_pid = None
    req = api.Trigger_CreateExecuteCommandsRequest()
    cmd = "ps -aef | grep athena_app | grep soft-init | grep -v grep"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node.Name(), cmd)

    resp = api.Trigger(req)
    for cmd in resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            return api.types.status.FAILURE
    
        if "ps" in cmd.command and "athena_app" in cmd.stdout:
            athena_sec_app_pid = cmd.stdout.strip().split()[1]
    
    if athena_sec_app_pid:
        api.Logger.info('Athena sec app restarted and has pid %s' % \
                        athena_sec_app_pid)
    else:
        api.Logger.info('Athena sec app failed to restart')
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def craft_pkt(_types, _dicts):

    for i in range(len(_types)):
        _type = _types[i]
        _dict = _dicts[i]

        if _type == "Ether":
            if 'smac' not in _dict.keys() or \
               'dmac' not in _dict.keys():
                raise Exception('Ether: smac and/or dmac not found')
            else:
                pkt = Ether(src = _dict['smac'],
                            dst = _dict['dmac'])

        elif _type == "Dot1Q":
            if 'vlan' not in _dict.keys():
                raise Exception('Dot1Q: vlan not found')
            else:
                pkt = pkt/Dot1Q(vlan = int(_dict['vlan']))

        elif _type == "IP":
            if 'sip' not in _dict.keys() or \
               'dip' not in _dict.keys():
                raise Exception('IP: sip and/or dip not found')
            else:
                pkt = pkt/IP(src = _dict['sip'],
                             dst = _dict['dip'],
                             id = 0)

        elif _type == "UDP":
            if 'sport' not in _dict.keys() or \
               'dport' not in _dict.keys():
                raise Exception('UDP: sport and/or dport not found')
            else:
                pkt = pkt/UDP(sport=int(_dict['sport']),
                              dport=int(_dict['dport']))

        elif _type == "MPLS":
            if 'label' not in _dict.keys() or \
               's' not in _dict.keys():
                raise Exception('MPLS: label and/or s not found')
            else:
                pkt = pkt/MPLS(label=int(_dict['label']),
                               s=int(_dict['s']))

    return pkt

class Args():

    def __init__(self):
        self.encap = False
        self.node = None
        self.Rx = None
        self._dir = None
        self.proto = None

        self.smac = None
        self.dmac = None
        self.vlan = None
        self.sip = None
        self.dip = None
        self.sport = None
        self.dport = None

def setup_pkt(_args):
    types = []
    dicts = []

    #print("encap: {}, dir: {}, Rx: {}".format(_args.encap, _args._dir, _args.Rx))
    with open(CURR_DIR + "/config/" + DEFAULT_POLICY_JSON_FILENAME) as json_fd:
        plcy_obj = json.load(json_fd)

    vnics = plcy_obj['vnic']
    trg_vnic = None

    for vnic in vnics:
        if _args.encap:
            encap_info = vnic['rewrite_underlay']
            vlan = encap_info['vlan_id']
        else:
            vlan = vnic['vlan_id']
        if vlan == str(_args.vlan):
            trg_vnic = vnic
            break

    if trg_vnic is not None:
        # encap info
        encap_info = trg_vnic['rewrite_underlay']
        # rewrite host info
        rewrite_host_info = trg_vnic['rewrite_host']


    if _args.encap:
        if encap_info is None:
            raise Exception('vnic config not found encap vlan %s' % _args.vlan)

        if _args._dir == 's2h':
            outer_smac = _args.smac
            outer_dmac = _args.dmac

        if encap_info['type'] == 'mplsoudp':
            if _args._dir == 'h2s':
                outer_smac = encap_info['smac']
                outer_dmac = encap_info['dmac']
            outer_sip = encap_info['ipv4_sip']
            outer_dip = encap_info['ipv4_dip']
            mpls_lbl1 = encap_info['mpls_label1']
            mpls_lbl2 = encap_info['mpls_label2']

        else:
            raise Exception('encap type %s not supported currently' % encap_info['type'])

        types.append("Ether")
        ether = {'smac' : outer_smac, 'dmac' : outer_dmac}
        dicts.append(ether)

        types.append("Dot1Q")
        dot1q = {'vlan' : _args.vlan}
        dicts.append(dot1q)

        # append outer IP
        types.append("IP")
        ip = {'sip' : outer_sip, 'dip' : outer_dip}
        dicts.append(ip)
        
        # append outer UDP
        types.append(_args.proto)
        proto = {'sport' : '0', 'dport' : '6635'}
        dicts.append(proto)

        # append MPLS label 1
        types.append("MPLS")
        mpls = {'label' : mpls_lbl1, 's' : 0}
        dicts.append(mpls)
        
        # append MPLS label 2
        types.append("MPLS")
        mpls = {'label' : mpls_lbl2, 's' : 1}
        dicts.append(mpls)
        
    else:
        types.append("Ether")
        if _args._dir == 'h2s':
            ether = {'smac' : _args.smac, 'dmac' : _args.dmac}
        elif _args._dir == 's2h':
            if rewrite_host_info is None:
                raise Exception('vnic config not found encap vlan %s' % _args.vlan)
            smac = rewrite_host_info['smac']
            dmac = rewrite_host_info['dmac']
            ether = {'smac' : smac, 'dmac' : dmac}
        dicts.append(ether)

        types.append("Dot1Q")
        dot1q = {'vlan' : _args.vlan}
        dicts.append(dot1q)

    types.append("IP")
    ip = {'sip' : _args.sip, 'dip' : _args.dip}
    dicts.append(ip)

    types.append("UDP")
    udp = {'sport' : _args.sport, 'dport' : _args.dport}
    dicts.append(udp)

    pkt = craft_pkt(types, dicts)
    pkt = pkt/DEFAULT_PAYLOAD
    #print(pkt.show())

    if _args._dir == 'h2s':
        fname = DEFAULT_H2S_RECV_PKT_FILENAME if _args.Rx == True else DEFAULT_H2S_GEN_PKT_FILENAME
    elif _args._dir == 's2h':
        fname = DEFAULT_S2H_RECV_PKT_FILENAME if _args.Rx == True else DEFAULT_S2H_GEN_PKT_FILENAME

    with open(CURR_DIR + '/config/' + fname, 'w+') as fd:
        logging.info('Writing crafted pkt to pcap file %s' % fd.name)
        wrpcap(fd.name, pkt)

    # copy the pcap file to the host
    api.CopyToHost(_args.node.Name(), [fd.name], "")
    fd.close()

def Setup(tc):

    # set up workloads
    AddWorkloads()
    
    workloads = api.GetWorkloads()
    if len(workloads) == 0:
        api.Logger.error('No workloads available')
        return api.types.status.FAILURE

    # All workloads created will be on same node currently
    tc.wl_node_name = workloads[0].node_name
    
    tc.wl_node = None
    tc.nodes = api.GetNodes() 
    for node in tc.nodes:
        if node.Name() == tc.wl_node_name:
            tc.wl_node = node
            break

    if not tc.wl_node:
        api.Logger.error('Failed to get workload node')
        return api.types.status.FAILURE

        
    host_intfs = api.GetNaplesHostInterfaces(tc.wl_node_name)
    
    # Assuming single nic per host 
    if len(host_intfs) != 2:
        api.Logger.error('Failed to get host interfaces')
        return api.types.status.FAILURE

    tc.up0_intf = host_intfs[0]
    tc.up1_intf = host_intfs[1] 
    tc.up0_vlan, tc.up1_vlan = None, None
    tc.up0_mac, tc.up1_mac = None, None

    # Assuming only two workloads today. One associated 
    # with up0 and vlanA, and other with up1 and vlanB
    for wl in workloads:
        if wl.parent_interface == tc.up0_intf:
            tc.up0_vlan = wl.uplink_vlan
            tc.up0_mac = wl.mac_address  
    
        if wl.parent_interface == tc.up1_intf:
            tc.up1_vlan = wl.uplink_vlan
            tc.up1_mac = wl.mac_address

    if not tc.up0_vlan or not tc.up1_vlan:
        api.Logger.error('Failed to get workload sub-intf VLANs')
        return api.types.status.FAILURE

    if not tc.up0_mac or not tc.up1_mac:
        api.Logger.error('Failed to get workload sub-intf mac addresses')
        return api.types.status.FAILURE

    api.Logger.info('Workload0: up0_intf %s up0_vlan %s up0_mac %s' % (
                    tc.up0_intf, tc.up0_vlan, tc.up0_mac))
    api.Logger.info('Workload1: up1_intf %s up1_vlan %s up1_mac %s' % (
                    tc.up1_intf, tc.up1_vlan, tc.up1_mac))


    # create flows for testing 
    tc.flows = []

    if not FLOWS:
        api.Logger.error('Flow list is empty')
        return api.types.status.FAILURE

    for flow in FLOWS:
        flow_info = FlowInfo()
        flow_info.set_sip(flow['sip'])
        flow_info.set_dip(flow['dip'])
        flow_info.set_proto(flow['proto'])

        if flow['proto'] == 'UDP':
            flow_info.set_src_port(flow['src_port'])
            flow_info.set_dst_port(flow['dst_port'])
        
        else:
            api.Logger.error('flow protocol %s not supported' % flow['proto'])
            return api.types.status.FAILURE

        tc.flows.append(flow_info)
   

    # setup policy.json file
    curr_dir = os.path.dirname(os.path.realpath(__file__))
    plcy_obj = None
    vnic_id = '1'
    
    # read from template file 
    with open(curr_dir + '/config/template_policy.json') as fd:
        plcy_obj = json.load(fd)

    vnics = plcy_obj['vnic']
        
    # Assuming one vnic only
    vnic_id = vnics[0]['vnic_id']

    vnics[0]['vlan_id'] = str(tc.up1_vlan)
    vnics[0]['session']['to_switch']['host_mac'] = str(tc.up1_mac)
    vnics[0]['rewrite_underlay']['vlan_id'] = str(tc.up0_vlan)
    vnics[0]['rewrite_underlay']['dmac'] = str(tc.up0_mac)
    vnics[0]['rewrite_host']['dmac'] = str(tc.up1_mac)

    for flow in tc.flows:
        v4_flow = {'vnic_lo': vnic_id, 'vnic_hi': vnic_id, 'sip_lo': flow.get_sip(), 
                    'sip_hi': flow.get_sip(), 'dip_lo': flow.get_dip(), 
                    'dip_hi': flow.get_dip(), 'proto':
                    str(PROTO_NUM[flow.get_proto()]), 'sport_lo':
                    str(flow.get_src_port()), 'sport_hi':
                    str(flow.get_src_port()), 'dport_lo':
                    str(flow.get_dst_port()), 'dport_hi':
                    str(flow.get_dst_port())}
    
        plcy_obj['v4_flows'].append(v4_flow)

    # write vlan/mac addr and flow info to actual file 
    with open(curr_dir + '/config/policy.json', 'w+') as fd:
        json.dump(plcy_obj, fd, indent=4)


    # copy policy.json file and send/recv scripts to node
    tc.nodes = api.GetNodes()
    tc.bitw_node = None

    for node in tc.nodes:
        if node.IsNaplesHwWithBumpInTheWire():
            policy_json_fname = curr_dir + '/config/policy.json'
            api.CopyToNaples(node.Name(), [policy_json_fname], "") 
            tc.bitw_node = node
            continue
         

        if node is tc.wl_node:
            send_pkt_script_fname = curr_dir + '/scripts/send_pkt.py' 
            recv_pkt_script_fname = curr_dir + '/scripts/recv_pkt.py' 
            policy_json_fname = curr_dir + '/config/policy.json'

            api.CopyToHost(node.Name(), [send_pkt_script_fname], "")
            api.CopyToHost(node.Name(), [recv_pkt_script_fname], "")
            api.CopyToHost(node.Name(), [policy_json_fname], "")

    # init response list
    tc.resp = []

    return api.types.status.SUCCESS


def Trigger(tc):
    
    # Copy policy.json to /data and restart Athena sec app on Athena Node
    ret = Athena_sec_app_restart(tc) 
    if ret != api.types.status.SUCCESS:
        return (ret)

    for node in tc.nodes:
        if node is not tc.wl_node:
            continue
    
        # convention: regular pkts sent on up1 and encap pkts sent on up0
        for flow in tc.flows:

            # common args to setup scapy pkt
            args = Args()
            args.node = node
            # TODO pick this from iterator
            args.proto = 'UDP'
            args.sip = flow.sip
            args.dip = flow.dip
            args.sport = flow.src_port
            args.dport = flow.dst_port

            # ==========================================
            # Send and Receive packets in H2S direction
            # ==========================================
            args._dir = 'h2s'
            h2s_req = api.Trigger_CreateExecuteCommandsRequest(serial=False)

            # ==========
            # Rx Packet
            # ==========
            args.encap =  True
            args.Rx = True
            args.vlan = tc.up0_vlan

            setup_pkt(args)
            recv_cmd = "./recv_pkt.py --intf_name %s --pcap_fname %s --timeout %s" \
                        % (tc.up0_intf, DEFAULT_H2S_RECV_PKT_FILENAME, str(SNIFF_TIMEOUT))

            api.Trigger_AddHostCommand(h2s_req, node.Name(), recv_cmd,
                                                background=True)

            # ==========
            # Tx Packet
            # ==========
            args.encap = False
            args.Rx = False
            args.smac = tc.up1_mac
            args.dmac = "00:aa:bb:cc:dd:ee"
            args.vlan = tc.up1_vlan

            setup_pkt(args)
            send_cmd = "./send_pkt.py --intf_name %s --pcap_fname %s" \
                        % (tc.up1_intf, DEFAULT_H2S_GEN_PKT_FILENAME)

            api.Trigger_AddHostCommand(h2s_req, node.Name(), 'sleep 2')
            api.Trigger_AddHostCommand(h2s_req, node.Name(), send_cmd)

            trig_resp = api.Trigger(h2s_req)
            time.sleep(SNIFF_TIMEOUT) 
            term_resp = api.Trigger_TerminateAllCommands(trig_resp)

            h2s_resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)
            tc.resp.append(h2s_resp)

            # ==========================================
            # Send and Receive packets in S2H direction
            # ==========================================
            args._dir = 's2h'
            s2h_req = api.Trigger_CreateExecuteCommandsRequest(serial=False)

            # ==========
            # Rx Packet
            # ==========
            args.encap = False
            args.Rx = True
            args.vlan = tc.up1_vlan

            setup_pkt(args)
            recv_cmd = "./recv_pkt.py --intf_name %s --pcap_fname %s --timeout %s" \
                        % (tc.up1_intf, DEFAULT_S2H_RECV_PKT_FILENAME, str(SNIFF_TIMEOUT))

            api.Trigger_AddHostCommand(s2h_req, node.Name(), recv_cmd,
                                                background=True)
   
            # ==========
            # Tx Packet
            # ==========
            args.encap =  True
            args.Rx = False
            args.smac = tc.up1_mac
            args.dmac = "00:aa:bb:cc:dd:ee"
            args.vlan = tc.up0_vlan

            setup_pkt(args)
            send_cmd = "./send_pkt.py --intf_name %s --pcap_fname %s" \
                        % (tc.up0_intf, DEFAULT_S2H_GEN_PKT_FILENAME)

            api.Trigger_AddHostCommand(s2h_req, node.Name(), 'sleep 2')
            api.Trigger_AddHostCommand(s2h_req, node.Name(), send_cmd)

            trig_resp = api.Trigger(s2h_req)
            time.sleep(SNIFF_TIMEOUT) 
            term_resp = api.Trigger_TerminateAllCommands(trig_resp)

            s2h_resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)
            tc.resp.append(s2h_resp)

    return api.types.status.SUCCESS

def Verify(tc):
    if len(tc.resp) == 0:
        return api.types.status.FAILURE

    for resp in tc.resp:
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)
            if cmd.exit_code != 0:
                if 'send_pkt' in cmd.command:
                    api.Logger.error("send pkts script failed")
                if 'recv_pkt' in cmd.command:
                    api.Logger.error("recv pkts script failed")
                return api.types.status.FAILURE
    
            if 'recv_pkt' in cmd.command and 'FAIL' in cmd.stdout:
                api.Logger.error("Datapath test failed")
                return api.types.status.FAILURE

    api.Logger.info('Datapath test passed')

    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS

