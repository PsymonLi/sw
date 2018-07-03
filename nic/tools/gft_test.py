#!/usr/bin/python3

import sys
import socket

#sys.path.insert(0, 'model_sim/src')
#import model_wrap

sys.path.insert(0, '../dol')
sys.path.insert(0, '../dol/third_party')
from infra.penscapy.penscapy import *
from infra.factory.scapyfactory import IcrcHeaderBuilder

def dump_pkt(pkt):
    print('***')
    for p in range(0, len(pkt), 8):
        chunk = bytes(pkt)[p:p+8]
        print(', '.join('0x{:02X}'.format(b) for b in chunk), end=",\n")
    for p in range(0, len(pkt), 8):
        chunk = bytes(pkt)[p:p+8]
        print(' '.join('{:02X}'.format(b) for b in chunk), end="\n")

###############################################################################

pkt = bytes(Ether(dst='00:01:02:03:04:05', src='00:A1:A2:A3:A4:A5') / \
        IP(dst='10.1.2.3', src='11.1.2.3') / \
        UDP(dport=4789) / VXLAN() / \
        Ether(dst='00:11:12:13:14:15', src='00:B1:B2:B3:B4:B5') / \
        IP(dst='12.1.2.3', src='13.1.2.3') / \
        UDP(dport=4789) / VXLAN() / \
        Ether(dst='00:21:22:23:24:25', src='00:C1:C2:C3:C4:C5') / \
        IP(dst='14.1.2.3', src='15.1.2.3') / \
        TCP(sport=0xABBA, dport=0xBEEF))

rpkt = bytes(Ether(dst='00:01:02:03:04:05', src='A3:21:A2:A3:A4:A5') / \
        IP(dst='10.1.2.3', src='11.1.2.3') / \
        UDP(dport=4789) / VXLAN() / \
        Ether(dst='00:11:12:13:14:15', src='00:B1:B2:B3:B4:B5') / \
        IP(dst='12.1.2.3', src='13.1.2.3') / \
        UDP(dport=4789) / VXLAN() / \
        Ether(dst='00:21:22:23:24:25', src='00:C1:C2:C3:C4:C5') / \
        IP(dst='14.1.2.3', src='15.1.2.3') / \
        TCP(sport=0xABBA, dport=0xBEEF))

dump_pkt(pkt)
dump_pkt(rpkt)

#model_wrap.zmq_connect()
#model_wrap.step_network_pkt(pkt, 1)
#(rpkt, port, cos) = model_wrap.get_next_pkt()

###############################################################################

pkt = Ether(dst='00:01:02:03:04:05', src='00:A1:A2:A3:A4:A5') / \
        IP(dst='10.1.2.3', src='11.1.2.3') / \
        UDP(dport=4789) / VXLAN() / \
        Ether(dst='00:11:12:13:14:15', src='00:B1:B2:B3:B4:B5') / \
        IP(dst='12.1.2.3', src='13.1.2.3') / \
        UDP(dport=4789) / VXLAN() / \
        Ether(dst='00:21:22:23:24:25', src='00:C1:C2:C3:C4:C5') / \
        IP(dst='14.1.2.3', src='15.1.2.3') / \
        TCP(sport=0x1234, dport=0x5678)

rpkt = Ether(dst='00:FE:ED:FE:ED:FE', src='00:DE:AE:AE:AE:AF') / \
        IP(dst='14.10.11.12', src='15.10.11.12') / \
        TCP(sport=0x1234, dport=0x5678)

dump_pkt(pkt)
dump_pkt(rpkt)

#model_wrap.zmq_connect()
#model_wrap.step_network_pkt(pkt, 1)
#(rpkt, port, cos) = model_wrap.get_next_pkt()

###############################################################################

payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'
pkt = Ether(dst='00:21:22:23:24:25', src='00:C1:C2:C3:C4:C5') / \
        IP(dst='14.1.2.3', src='15.1.2.3') / \
        TCP(sport=0x1234, dport=0x5678) / payload

rpkt = Ether(dst='00:01:02:03:04:05', src='00:A1:A2:A3:A4:A5') / \
        IP(dst='10.1.2.3', src='11.1.2.3', id=0) / \
        UDP(dport=4789, chksum=0) / VXLAN() / \
        Ether(dst='00:11:12:13:14:15', src='00:B1:B2:B3:B4:B5') / \
        IP(dst='12.1.2.3', src='13.1.2.3', id=0) / \
        UDP(dport=4789, chksum=0) / VXLAN() / \
        Ether(dst='00:21:22:23:24:25', src='00:C1:C2:C3:C4:C5') / \
        IP(dst='14.1.2.3', src='15.1.2.3') / \
        TCP(sport=0x1234, dport=0x5678) / payload

dump_pkt(pkt)
dump_pkt(rpkt)

#model_wrap.zmq_connect()
#model_wrap.step_network_pkt(pkt, 1)
#(rpkt, port, cos) = model_wrap.get_next_pkt()

###############################################################################

pkt = Ether(dst='00:01:02:03:04:05', src='00:A1:A2:A3:A4:A5') / \
        IP(dst='10.1.2.3', src='11.1.2.3') / \
        UDP() / BTH(opcode=4) / bytes([b for b in range(0,32)]) / ICRC()
builder = IcrcHeaderBuilder(pkt)
icrc = socket.htonl(builder.GetIcrc())
pkt[ICRC].icrc = icrc

dump_pkt(pkt)

###############################################################################


ipkt = Ether(dst='00:11:12:13:14:15', src='00:B1:B2:B3:B4:B5') / \
        IP(dst='12.1.2.3', src='13.1.2.3') / \
        UDP() / BTH(opcode=4) / bytes([b for b in range(0,32)]) / ICRC()
builder = IcrcHeaderBuilder(ipkt)
icrc = socket.htonl(builder.GetIcrc())
ipkt[ICRC].icrc = icrc

pkt = Ether(dst='00:01:02:03:04:05', src='00:A1:A2:A3:A4:A5') / \
        IP(dst='10.1.2.3', src='11.1.2.3') / \
        UDP() / VXLAN() / ipkt

dump_pkt(pkt)

###############################################################################

pkt = Ether(dst='00:E1:E2:E3:E4:E5', src='00:A1:A2:A3:A4:A5') / \
        IP(dst='10.1.2.3', src='11.1.2.3') / \
        UDP(chksum=0) / BTH(opcode=4) / bytes([b for b in range(0,32)]) / ICRC()
builder = IcrcHeaderBuilder(pkt)
icrc = socket.htonl(builder.GetIcrc())
pkt[ICRC].icrc = icrc

dump_pkt(pkt)

###############################################################################

pkt = Ether(dst='00:F1:F2:F3:F4:F5', src='00:A1:A2:A3:A4:A5') / \
        IP(dst='10.1.2.3', src='11.1.2.3') / \
        UDP(chksum=0) / BTH(opcode=4) / bytes([b for b in range(0,32)]) / ICRC()

rpkt = Ether(dst='00:01:02:03:04:05', src='00:A1:A2:A3:A4:A5') / \
        IP(dst='10.1.2.3', src='11.1.2.3', id=0) / \
        UDP(dport=4789, chksum=0) / VXLAN() / \
        Ether(dst='00:F1:F2:F3:F4:F5', src='00:A1:A2:A3:A4:A5') / \
        IP(dst='10.1.2.3', src='11.1.2.3') / \
        UDP(chksum=0) / BTH(opcode=4) / bytes([b for b in range(0,32)]) / ICRC()
builder = IcrcHeaderBuilder(pkt)
icrc = socket.htonl(builder.GetIcrc())
rpkt[ICRC].icrc = icrc

dump_pkt(pkt)
dump_pkt(rpkt)

###############################################################################

#vlan=100, 10.10.1.1/1023/80/TCP
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'
pkt = bytes(Ether(dst='00:01:02:03:04:05', src='00:A1:A2:A3:A4:A5') / \
        Dot1Q(vlan=100) / \
        IP(dst='100.1.2.3', src='101.1.2.3', id=0) / \
        UDP(dport=4789, chksum=0) / VXLAN() / \
        Ether(dst='00:F1:F2:F3:F4:F5', src='00:A1:A2:A3:A4:A5') / \
        IP(dst='10.10.1.1', src='11.1.2.3') / \
        TCP(sport=1023, dport=80) / payload)

dump_pkt(pkt)

#model_wrap.zmq_connect()
#model_wrap.step_network_pkt(pkt, 1)
#(rpkt, port, cos) = model_wrap.get_next_pkt()

###############################################################################

###############################################################################

#vlan=100 11.11.2.2/10000/1234/UDP
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'
pkt = bytes(Ether(dst='00:01:02:03:04:05', src='00:A1:A2:A3:A4:A5') / \
        Dot1Q(vlan=100) / \
        IP(dst='100.1.2.3', src='101.1.2.3', id=0) / \
        UDP(dport=4789, chksum=0) / VXLAN() / \
        Ether(dst='00:F1:F2:F3:F4:F5', src='00:B1:B2:B3:B4:B5') / \
        IP(dst='11.11.2.2', src='10.10.2.3') / \
        UDP(sport=10000, dport=1234) / payload)

dump_pkt(pkt)

#model_wrap.zmq_connect()
#model_wrap.step_network_pkt(pkt, 1)
#(rpkt, port, cos) = model_wrap.get_next_pkt()

###############################################################################
