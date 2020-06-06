#!/usr/bin/python3

import json
import os
import logging
from scapy.all import *
from scapy.contrib.mpls import MPLS
from scapy.contrib.geneve import GENEVE
import iota.test.athena.utils.misc as utils
import iota.harness.api as api

DEFAULT_H2S_GEN_PKT_FILENAME     = '/h2s_pkt.pcap'
DEFAULT_H2S_RECV_PKT_FILENAME    = '/h2s_recv_pkt.pcap'
DEFAULT_S2H_RECV_PKT_FILENAME    = '/s2h_recv_pkt.pcap'
DEFAULT_S2H_GEN_PKT_FILENAME     = '/s2h_pkt.pcap'
DEFAULT_PAYLOAD                  = 'abcdefghijklmnopqrstuvwzxyabcdefghijklmnopqrstuvwzxy'
CURR_DIR                         = os.path.dirname(os.path.realpath(__file__))

logging.basicConfig(level=logging.INFO, handlers=[logging.StreamHandler(sys.stdout)])

# ==============================================
# Return: payload for given payload  size
# pyld_size: size of payload in bytes
# ==============================================
def get_payload(pyld_size):

    pyld=[]
    size = len(DEFAULT_PAYLOAD)
    for i in range(pyld_size):
        pyld.append(DEFAULT_PAYLOAD[i % size])

    return ''.join(pyld)

# ==============================================
# Return: crafted scapy packet based on input
# _types: list of header types
# _dicts: dictionary of header fields and their
#         values
# ==============================================
def craft_pkt(_types, _dicts):

    pkt = None
    for i in range(len(_types)):
        _type = _types[i]
        _dict = _dicts[i]
        logging.debug("type: {}, dict: {}".format(_type, _dict))

        if _type == "Ether":
            if 'smac' not in _dict.keys() or \
               'dmac' not in _dict.keys():
                raise Exception('Ether: smac and/or dmac not found')
            else:
                if pkt:
                    pkt = pkt/Ether(src = _dict['smac'],
                                dst = _dict['dmac'])
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

        elif _type == "TCP":
            if 'sport' not in _dict.keys() or \
               'dport' not in _dict.keys():
                raise Exception('TCP: sport and/or dport not found')
            else:
                pkt = pkt/TCP(sport=int(_dict['sport']),
                              dport=int(_dict['dport']))

        elif _type == "ICMP":
            if 'icmp_type' not in _dict.keys() or \
               'icmp_code' not in _dict.keys():
                raise Exception('ICMP: icmp_type and/or icmp_code not found')
            else:
                pkt = pkt/ICMP(type=int(_dict['icmp_type']),
                              code=int(_dict['icmp_code']))

        elif _type == "MPLS":
            if 'label' not in _dict.keys() or \
               's' not in _dict.keys():
                raise Exception('MPLS: label and/or s not found')
            else:
                pkt = pkt/MPLS(label=int(_dict['label']),
                               s=int(_dict['s']))

        elif _type == "GENEVE":
            if 'vni' not in _dict.keys() or \
               'options' not in _dict.keys():
                raise Exception('GENEVE: vni and/or options not found')
            else:
                pkt = pkt/GENEVE(vni=int(_dict['vni']),
                               options=_dict['options'])
    return pkt

# ==============================================
# Class to setup H2S Tx/Rx and S2H Tx/Rx
# packets
# ==============================================
class Pktgen():

    def __init__(self):
        self.encap = False
        self.node = None
        self.Rx = None
        self.dir_ = None
        self.proto = None
        self.nat = None
        self.pyld_size = None
        self.vnic = None
        self.nat_flows_h2s_tx = None
        self.nat_flows_h2s_rx = None
        self.nat_flows_s2h_tx = None
        self.nat_flows_s2h_rx = None

        self.flow_id = None
        self.smac = None
        self.dmac = None
        self.vlan = None
        self.sip = None
        self.dip = None
        self.sport = None
        self.dport = None
        self.icmp_type = None
        self.icmp_code = None

    # ========
    # Setters
    # ========
    def set_encap(self, encap):
        self.encap = encap

    def set_node(self, node):
        self.node = node

    def set_Rx(self, Rx):
        self.Rx = Rx

    def set_dir_(self, dir_):
        self.dir_ = dir_

    def set_proto(self, proto):
        self.proto = proto

    def set_nat(self, nat):
        self.nat = nat

    def set_pyld_size(self, pyld_size):
        self.pyld_size = pyld_size

    def set_vnic(self, vnic):
        self.vnic = vnic

    def set_nat_flows_h2s_tx(self, nat_flows_h2s_tx):
        self.nat_flows_h2s_tx = nat_flows_h2s_tx

    def set_nat_flows_h2s_rx(self, nat_flows_h2s_rx):
        self.nat_flows_h2s_rx = nat_flows_h2s_rx

    def set_nat_flows_s2h_tx(self, nat_flows_s2h_tx):
        self.nat_flows_s2h_tx = nat_flows_s2h_tx

    def set_nat_flows_s2h_rx(self, nat_flows_s2h_rx):
        self.nat_flows_s2h_rx = nat_flows_s2h_rx

    def set_flow_id(self, flow_id):
        self.flow_id = flow_id

    def set_smac(self, smac):
        self.smac = smac

    def set_dmac(self, dmac):
        self.dmac = dmac

    def set_vlan(self, vlan):
        self.vlan = vlan

    def set_sip(self, sip):
        self.sip = sip

    def set_dip(self, dip):
        self.dip = dip

    def set_sport(self, sport):
        self.sport = sport

    def set_dport(self, dport):
        self.dport = dport

    def set_icmp_type(self, icmp_type):
        self.icmp_type = icmp_type

    def set_icmp_code(self, icmp_code):
        self.icmp_code = icmp_code

    # ========
    # Getters
    # ========
    def get_encap(self):
        return self.encap

    def get_node(self):
        return self.node

    def get_Rx(self):
        return self.Rx

    def get_dir_(self):
        return self.dir_

    def get_proto(self):
        return self.proto

    def get_nat(self):
        return self.nat

    def get_pyld_size(self):
        return self.pyld_size

    def get_vnic(self):
        return self.vnic

    def get_nat_flows_h2s_tx(self):
        return self.nat_flows_h2s_tx

    def get_nat_flows_h2s_rx(self):
        return self.nat_flows_h2s_rx

    def get_nat_flows_s2h_tx(self):
        return self.nat_flows_s2h_tx

    def get_nat_flows_s2h_rx(self):
        return self.nat_flows_s2h_rx

    def get_flow_id(self):
        return self.flow_id

    def get_smac(self):
        return self.smac

    def get_dmac(self):
        return self.dmac

    def get_vlan(self):
        return self.vlan

    def get_sip(self):
        return self.sip

    def get_dip(self):
        return self.dip

    def get_sport(self):
        return self.sport

    def get_dport(self):
        return self.dport

    def get_icmp_type(self):
        return self.icmp_type

    def get_icmp_code(self):
        return self.icmp_code

    # ========
    # Return: sip & dip for nat flow
    #         based on H2S/S2H, Rx/Tx and flow_id 
    # ==============================================
    def get_nat_flow_sip_dip(self, dir_ = 'h2s', Rx = False, proto = 'UDP', flow_id = 0):
        if dir_ == 'h2s' and Rx:
            sip = self.nat_flows_h2s_rx[proto][flow_id].sip
            dip = self.nat_flows_h2s_rx[proto][flow_id].dip
        elif dir_ == 'h2s' and not Rx:
            sip = self.nat_flows_h2s_tx[proto][flow_id].sip
            dip = self.nat_flows_h2s_tx[proto][flow_id].dip
        elif dir_ == 's2h' and Rx:
            sip = self.nat_flows_s2h_rx[proto][flow_id].sip
            dip = self.nat_flows_s2h_rx[proto][flow_id].dip
        else:
            sip = self.nat_flows_s2h_tx[proto][flow_id].sip
            dip = self.nat_flows_s2h_tx[proto][flow_id].dip

        return (sip, dip)

    # ==============================================
    # Return: void
    # Generates a packet based on the information
    # provided in the object. The generated packet
    # could be encap (MPLSoUDP or GENEVE) or
    # non-encap. It is written to a pcap file and
    # copied to the node in the object
    # ==============================================
    def setup_pkt(self):
        types = []
        dicts = []

        logging.debug("encap: {}, dir: {}, Rx: {}".format(self.encap, self.dir_, self.Rx))

        if self.vnic is not None:
            # encap info
            encap_info = self.vnic['rewrite_underlay']
            # only applicable to L3 vnics
            if "vnic_type" not in self.vnic:
                # rewrite host info
                rewrite_host_info = self.vnic['rewrite_host']

        if self.encap:
            if not encap_info:
                raise Exception('vnic config not found encap vlan %s' % self.vlan)

            if self.dir_ == 's2h':
                outer_smac = self.smac
                outer_dmac = self.dmac

            if encap_info['type'] == 'mplsoudp':
                if self.dir_ == 'h2s':
                    outer_smac = encap_info['smac']
                    outer_dmac = encap_info['dmac']
                outer_sip = encap_info['ipv4_sip']
                outer_dip = encap_info['ipv4_dip']
                mpls_lbl1 = encap_info['mpls_label1']
                mpls_lbl2 = encap_info['mpls_label2']

            elif encap_info['type'] == 'geneve':
                if self.dir_ == 'h2s':
                    outer_smac = encap_info['smac']
                    outer_dmac = encap_info['dmac']
                outer_sip = encap_info['ipv4_sip']
                outer_dip = encap_info['ipv4_dip']
                vni = encap_info['vni']
                if self.dir_ == 'h2s':
                    dst_slot_id = encap_info['dst_slot_id']
                    src_slot_id = self.vnic['slot_id']
                elif self.dir_ == 's2h':
                    src_slot_id = encap_info['dst_slot_id']
                    dst_slot_id = self.vnic['slot_id']

            else:
                raise Exception('encap type %s not supported currently' % encap_info['type'])
            types.append("Ether")
            ether = {'smac' : outer_smac, 'dmac' : outer_dmac}
            dicts.append(ether)

            types.append("Dot1Q")
            dot1q = {'vlan' : self.vlan}
            dicts.append(dot1q)

            # append outer IP
            types.append("IP")
            ip = {'sip' : outer_sip, 'dip' : outer_dip}
            dicts.append(ip)

            # append outer UDP
            types.append("UDP") # outer is always UDP
            dst_port = '6635' if encap_info['type'] == 'mplsoudp' else '6081'
            proto = {'sport' : '0', 'dport' : dst_port}
            dicts.append(proto)

            if encap_info['type'] == 'mplsoudp':
                # append MPLS label 1
                types.append("MPLS")
                mpls = {'label' : mpls_lbl1, 's' : 0}
                dicts.append(mpls)

                # append MPLS label 2
                types.append("MPLS")
                mpls = {'label' : mpls_lbl2, 's' : 1}
                dicts.append(mpls)

            elif encap_info['type'] == 'geneve':
                # append GENEVE options
                types.append("GENEVE")
                # some of these fields are hard-coded for now
                option1 = (0x21 << (5 * 8)) | (0x1 << (4 * 8)) | int(src_slot_id)
                option2 = (0x22 << (5 * 8)) | (0x1 << (4 * 8)) | int(dst_slot_id)
                options = (option1 << (8 * 8)) | option2
                geneve = {'vni' : vni, 'options' : options.to_bytes(16, byteorder="big")}
                dicts.append(geneve)

                types.append("Ether")
                if self.dir_ == 'h2s':
                    ether = {'smac' : self.smac, 'dmac' : self.dmac}
                elif self.dir_ == 's2h':
                    ether = {'smac' : self.smac, 'dmac' : self.dmac}
                dicts.append(ether)
        else:
            types.append("Ether")
            if self.dir_ == 'h2s':
                ether = {'smac' : self.smac, 'dmac' : self.dmac}
            elif self.dir_ == 's2h':
                if utils.is_L2_vnic(self.vnic):
                    ether = {'smac' : self.smac, 'dmac' : self.dmac}
                else:
                    if not rewrite_host_info:
                        raise Exception('vnic config not found encap vlan %s' % self.vlan)
                    smac = rewrite_host_info['smac']
                    dmac = rewrite_host_info['dmac']
                    ether = {'smac' : smac, 'dmac' : dmac}
            dicts.append(ether)

            types.append("Dot1Q")
            dot1q = {'vlan' : self.vlan}
            dicts.append(dot1q)

        types.append("IP")
        ip = {'sip' : self.sip, 'dip' : self.dip}
        if self.nat == 'yes':
           nat_sip, nat_dip = self.get_nat_flow_sip_dip(self.dir_, self.Rx, self.proto, self.flow_id)
           ip = {'sip' : nat_sip, 'dip' : nat_dip}
        dicts.append(ip)

        types.append(self.proto)
        if self.proto == 'UDP' or self.proto == 'TCP':
            dicts.append({'sport' : self.sport, 'dport' : self.dport})
        else: # ICMP
            dicts.append({'icmp_type' : self.icmp_type, 
                        'icmp_code' : self.icmp_code})

        pkt = craft_pkt(types, dicts)
        pkt = pkt/get_payload(self.pyld_size)
        #logging.debug("Crafted pkt: {}".format(pkt.show()))

        if self.dir_ == 'h2s':
            fname = DEFAULT_H2S_RECV_PKT_FILENAME if self.Rx == True else DEFAULT_H2S_GEN_PKT_FILENAME
        elif self.dir_ == 's2h':
            fname = DEFAULT_S2H_RECV_PKT_FILENAME if self.Rx == True else DEFAULT_S2H_GEN_PKT_FILENAME

        with open(CURR_DIR + fname, 'w+') as fd:
            logging.debug('Writing crafted pkt to pcap file %s' % fd.name)
            wrpcap(fd.name, pkt)

        pcap_fname = fd.name

        # copy the pcap file to the host
        api.CopyToHost(self.node.Name(), [pcap_fname], "")
        os.remove(pcap_fname)
