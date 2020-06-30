#!/usr/bin/python3

import sys
import socket

sys.path.insert(0, '../dol')
sys.path.insert(0, '../dol/third_party')
from infra.penscapy.penscapy import *
from scapy.contrib.mpls import MPLS
from scapy.contrib.geneve import GENEVE

def dump_pkt(pkt):
    print('***')
    for p in range(0, len(pkt), 8):
        chunk = bytes(pkt)[p:p+8]
        print(', '.join('0x{:02X}'.format(b) for b in chunk), end=",\n")


###############################################################################
# begin Athena IPv4 conntrack TCP
###############################################################################
print('H2S L2 IPv4 TCP S')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'
ipv4_tcp = Ether(dst='00:00:f1:d0:d1:d0', src='00:00:00:40:08:01') / \
        Dot1Q(vlan=16) / \
        IP(dst='192.0.2.1', src='2.0.0.1', id=0, ttl=64) / \
        TCP(sport=0x03e8, dport=0x2710, flags='S') / payload
dump_pkt(ipv4_tcp)

print('H2S L2 TCP EXP S')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_tcp_exp = Ether(dst='00:06:07:08:09:0A', src='00:01:02:03:04:05') / \
        Dot1Q(vlan=2) / \
        IP(dst='1.2.3.4', src='4.3.2.1', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:F1:D0:D1:D0', src='00:00:00:40:08:01') / \
        IP(dst='192.0.2.1', src='2.0.0.1', id=0, ttl=64) / \
        TCP(sport=0x03e8, dport=0x2710, flags='S') / payload

dump_pkt(geneve_ipv4_tcp_exp)

print('S2H L2 TCP INJ SA')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_tcp_inj = Ether(dst='00:01:02:03:04:05', src='00:06:07:08:09:0A') / \
        Dot1Q(vlan=2) / \
        IP(dst='4.3.2.1', src='1.2.3.4', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        IP(dst='2.0.0.1', src='192.0.2.1', id=0, ttl=64) / \
        TCP(sport=0x2710, dport=0x03e8, flags='SA') / payload

dump_pkt(geneve_ipv4_tcp_inj)

print('S2H L2 TCP EXP SA')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

ipv4_tcp_exp = Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        Dot1Q(vlan=16) / \
        IP(dst='2.0.0.1', src='192.0.2.1', id=0, ttl=64) / \
        TCP(sport=0x2710, dport=0x03e8, flags='SA') / payload

dump_pkt(ipv4_tcp_exp)

print('S2H L2 TCP INJ R')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_tcp_inj = Ether(dst='00:01:02:03:04:05', src='00:06:07:08:09:0A') / \
        Dot1Q(vlan=2) / \
        IP(dst='4.3.2.1', src='1.2.3.4', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        IP(dst='2.0.0.1', src='192.0.2.1', id=0, ttl=64) / \
        TCP(sport=0x2710, dport=0x03e8, flags='R') / payload

dump_pkt(geneve_ipv4_tcp_inj)

print('S2H L2 TCP EXP R')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

ipv4_tcp_exp = Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        Dot1Q(vlan=16) / \
        IP(dst='2.0.0.1', src='192.0.2.1', id=0, ttl=64) / \
        TCP(sport=0x2710, dport=0x03e8, flags='R') / payload

dump_pkt(ipv4_tcp_exp)

print('H2S L2 IPv4 TCP R')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'
ipv4_tcp = Ether(dst='00:00:f1:d0:d1:d0', src='00:00:00:40:08:01') / \
        Dot1Q(vlan=16) / \
        IP(dst='192.0.2.1', src='2.0.0.1', id=0, ttl=64) / \
        TCP(sport=0x03e8, dport=0x2710, flags='R') / payload
dump_pkt(ipv4_tcp)

print('H2S L2 TCP EXP R')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_tcp_exp = Ether(dst='00:06:07:08:09:0A', src='00:01:02:03:04:05') / \
        Dot1Q(vlan=2) / \
        IP(dst='1.2.3.4', src='4.3.2.1', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:F1:D0:D1:D0', src='00:00:00:40:08:01') / \
        IP(dst='192.0.2.1', src='2.0.0.1', id=0, ttl=64) / \
        TCP(sport=0x03e8, dport=0x2710, flags='R') / payload

dump_pkt(geneve_ipv4_tcp_exp)

print('H2S L2 IPv4 TCP F')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'
ipv4_tcp = Ether(dst='00:00:f1:d0:d1:d0', src='00:00:00:40:08:01') / \
        Dot1Q(vlan=16) / \
        IP(dst='192.0.2.1', src='2.0.0.1', id=0, ttl=64) / \
        TCP(sport=0x03e8, dport=0x2710, flags='F') / payload
dump_pkt(ipv4_tcp)

print('H2S L2 TCP EXP F')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_tcp_exp = Ether(dst='00:06:07:08:09:0A', src='00:01:02:03:04:05') / \
        Dot1Q(vlan=2) / \
        IP(dst='1.2.3.4', src='4.3.2.1', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:F1:D0:D1:D0', src='00:00:00:40:08:01') / \
        IP(dst='192.0.2.1', src='2.0.0.1', id=0, ttl=64) / \
        TCP(sport=0x03e8, dport=0x2710, flags='F') / payload

dump_pkt(geneve_ipv4_tcp_exp)

print('S2H L2 TCP INJ F')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_tcp_inj = Ether(dst='00:01:02:03:04:05', src='00:06:07:08:09:0A') / \
        Dot1Q(vlan=2) / \
        IP(dst='4.3.2.1', src='1.2.3.4', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        IP(dst='2.0.0.1', src='192.0.2.1', id=0, ttl=64) / \
        TCP(sport=0x2710, dport=0x03e8, flags='F') / payload

dump_pkt(geneve_ipv4_tcp_inj)

print('S2H L2 TCP EXP F')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

ipv4_tcp_exp = Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        Dot1Q(vlan=16) / \
        IP(dst='2.0.0.1', src='192.0.2.1', id=0, ttl=64) / \
        TCP(sport=0x2710, dport=0x03e8, flags='F') / payload

dump_pkt(ipv4_tcp_exp)

print('H2S L2 IPv4 TCP SA')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'
ipv4_tcp = Ether(dst='00:00:f1:d0:d1:d0', src='00:00:00:40:08:01') / \
        Dot1Q(vlan=16) / \
        IP(dst='192.0.2.1', src='2.0.0.1', id=0, ttl=64) / \
        TCP(sport=0x03e8, dport=0x2710, flags='SA') / payload
dump_pkt(ipv4_tcp)

print('H2S L2 TCP EXP SA')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_tcp_exp = Ether(dst='00:06:07:08:09:0A', src='00:01:02:03:04:05') / \
        Dot1Q(vlan=2) / \
        IP(dst='1.2.3.4', src='4.3.2.1', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:F1:D0:D1:D0', src='00:00:00:40:08:01') / \
        IP(dst='192.0.2.1', src='2.0.0.1', id=0, ttl=64) / \
        TCP(sport=0x03e8, dport=0x2710, flags='SA') / payload

dump_pkt(geneve_ipv4_tcp_exp)

print('H2S L2 IPv4 TCP A')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'
ipv4_tcp = Ether(dst='00:00:f1:d0:d1:d0', src='00:00:00:40:08:01') / \
        Dot1Q(vlan=16) / \
        IP(dst='192.0.2.1', src='2.0.0.1', id=0, ttl=64) / \
        TCP(sport=0x03e8, dport=0x2710, flags='A') / payload
dump_pkt(ipv4_tcp)

print('H2S L2 TCP EXP A')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_tcp_exp = Ether(dst='00:06:07:08:09:0A', src='00:01:02:03:04:05') / \
        Dot1Q(vlan=2) / \
        IP(dst='1.2.3.4', src='4.3.2.1', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:F1:D0:D1:D0', src='00:00:00:40:08:01') / \
        IP(dst='192.0.2.1', src='2.0.0.1', id=0, ttl=64) / \
        TCP(sport=0x03e8, dport=0x2710, flags='A') / payload

dump_pkt(geneve_ipv4_tcp_exp)


print('S2H L2 TCP INJ A')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_tcp_inj = Ether(dst='00:01:02:03:04:05', src='00:06:07:08:09:0A') / \
        Dot1Q(vlan=2) / \
        IP(dst='4.3.2.1', src='1.2.3.4', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        IP(dst='2.0.0.1', src='192.0.2.1', id=0, ttl=64) / \
        TCP(sport=0x2710, dport=0x03e8, flags='A') / payload

dump_pkt(geneve_ipv4_tcp_inj)

print('S2H L2 TCP EXP A')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

ipv4_tcp_exp = Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        Dot1Q(vlan=16) / \
        IP(dst='2.0.0.1', src='192.0.2.1', id=0, ttl=64) / \
        TCP(sport=0x2710, dport=0x03e8, flags='A') / payload

dump_pkt(ipv4_tcp_exp)

print('S2H L2 TCP INJ S')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_tcp_inj = Ether(dst='00:01:02:03:04:05', src='00:06:07:08:09:0A') / \
        Dot1Q(vlan=2) / \
        IP(dst='4.3.2.1', src='1.2.3.4', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        IP(dst='2.0.0.1', src='192.0.2.1', id=0, ttl=64) / \
        TCP(sport=0x2710, dport=0x03e8, flags='S') / payload

dump_pkt(geneve_ipv4_tcp_inj)

print('S2H L2 TCP EXP S')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

ipv4_tcp_exp = Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        Dot1Q(vlan=16) / \
        IP(dst='2.0.0.1', src='192.0.2.1', id=0, ttl=64) / \
        TCP(sport=0x2710, dport=0x03e8, flags='S') / payload

dump_pkt(ipv4_tcp_exp)

print('H2S L2 IPv4 TCP SA')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'
ipv4_tcp = Ether(dst='00:00:f1:d0:d1:d0', src='00:00:00:40:08:01') / \
        Dot1Q(vlan=16) / \
        IP(dst='192.0.2.1', src='2.0.0.1', id=0, ttl=64) / \
        TCP(sport=0x03e8, dport=0x2710, flags='SA') / payload
dump_pkt(ipv4_tcp)

print('H2S L2 TCP EXP SA')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_tcp_exp = Ether(dst='00:06:07:08:09:0A', src='00:01:02:03:04:05') / \
        Dot1Q(vlan=2) / \
        IP(dst='1.2.3.4', src='4.3.2.1', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:F1:D0:D1:D0', src='00:00:00:40:08:01') / \
        IP(dst='192.0.2.1', src='2.0.0.1', id=0, ttl=64) / \
        TCP(sport=0x03e8, dport=0x2710, flags='SA') / payload

dump_pkt(geneve_ipv4_tcp_exp)


###############################################################################
# end Athena Switch-to-Host IPv4 TCP
###############################################################################


###############################################################################
# begin Athena IPv6 conntrack TCP
###############################################################################
print('H2S L2 IPv6 TCP INJ S')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'
ipv6_tcp = Ether(dst='00:00:f1:d0:d1:d0', src='00:00:00:40:08:01') / \
        Dot1Q(vlan=16) / \
        IPv6(src='200::1', dst='c00::201') / \
        TCP(sport=0x03e8, dport=0x2710, flags='S') / payload
dump_pkt(ipv6_tcp)

print('H2S L2 IPv6 TCP EXP S')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv6_tcp_exp = Ether(dst='00:06:07:08:09:0A', src='00:01:02:03:04:05') / \
        Dot1Q(vlan=2) / \
        IP(dst='1.2.3.4', src='4.3.2.1', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:F1:D0:D1:D0', src='00:00:00:40:08:01') / \
        IPv6(src='200::1', dst='c00::201') / \
        TCP(sport=0x03e8, dport=0x2710, flags='S') / payload

dump_pkt(geneve_ipv6_tcp_exp)

print('S2H L2 IPv6 TCP INJ SA')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv6_tcp_inj = Ether(dst='00:01:02:03:04:05', src='00:06:07:08:09:0A') / \
        Dot1Q(vlan=2) / \
        IP(dst='4.3.2.1', src='1.2.3.4', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        IPv6(src='c00::201', dst='200::1') / \
        TCP(sport=0x2710, dport=0x03e8, flags='SA') / payload

dump_pkt(geneve_ipv6_tcp_inj)

print('S2H L2 IPV6 TCP EXP SA')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

ipv6_tcp_exp = Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        Dot1Q(vlan=16) / \
        IPv6(src='c00::201', dst='200::1') / \
        TCP(sport=0x2710, dport=0x03e8, flags='SA') / payload

dump_pkt(ipv6_tcp_exp)


print('S2H L2 IPv6 TCP INJ R')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv6_tcp_inj = Ether(dst='00:01:02:03:04:05', src='00:06:07:08:09:0A') / \
        Dot1Q(vlan=2) / \
        IP(dst='4.3.2.1', src='1.2.3.4', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        IPv6(src='c00::201', dst='200::1') / \
        TCP(sport=0x2710, dport=0x03e8, flags='R') / payload

dump_pkt(geneve_ipv6_tcp_inj)

print('S2H L2 IPv6 TCP INJ F')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv6_tcp_inj = Ether(dst='00:01:02:03:04:05', src='00:06:07:08:09:0A') / \
        Dot1Q(vlan=2) / \
        IP(dst='4.3.2.1', src='1.2.3.4', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        IPv6(src='c00::201', dst='200::1') / \
        TCP(sport=0x2710, dport=0x03e8, flags='F') / payload

dump_pkt(geneve_ipv6_tcp_inj)

print('H2S L2 IPv6 TCP INJ R')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'
ipv6_tcp = Ether(dst='00:00:f1:d0:d1:d0', src='00:00:00:40:08:01') / \
        Dot1Q(vlan=16) / \
        IPv6(src='200::1', dst='c00::201') / \
        TCP(sport=0x03e8, dport=0x2710, flags='R') / payload
dump_pkt(ipv6_tcp)

print('H2S L2 IPv6 TCP INJ F')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'
ipv6_tcp = Ether(dst='00:00:f1:d0:d1:d0', src='00:00:00:40:08:01') / \
        Dot1Q(vlan=16) / \
        IPv6(src='200::1', dst='c00::201') / \
        TCP(sport=0x03e8, dport=0x2710, flags='F') / payload
dump_pkt(ipv6_tcp)



print('H2S L2 IPv4 UDP ')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'
ipv4_udp = Ether(dst='00:00:f1:d0:d1:d0', src='00:00:00:40:08:01') / \
        Dot1Q(vlan=16) / \
        IP(dst='192.0.2.1', src='2.0.0.1', id=0, ttl=64) / \
        UDP(sport=0x03e9, dport=0x2710) / payload
dump_pkt(ipv4_udp)

print('H2S L2 UDP EXP')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_udp_exp = Ether(dst='00:06:07:08:09:0A', src='00:01:02:03:04:05') / \
        Dot1Q(vlan=2) / \
        IP(dst='1.2.3.4', src='4.3.2.1', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:F1:D0:D1:D0', src='00:00:00:40:08:01') / \
        IP(dst='192.0.2.1', src='2.0.0.1', id=0, ttl=64) / \
        UDP(sport=0x03e9, dport=0x2710) / payload

dump_pkt(geneve_ipv4_udp_exp)

print('S2H L2 UDP INJ')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_udp_inj = Ether(dst='00:01:02:03:04:05', src='00:06:07:08:09:0A') / \
        Dot1Q(vlan=2) / \
        IP(dst='4.3.2.1', src='1.2.3.4', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        IP(dst='2.0.0.1', src='192.0.2.1', id=0, ttl=64) / \
        UDP(sport=0x2710, dport=0x03e9) / payload

dump_pkt(geneve_ipv4_udp_inj)

print('S2H L2 UDP EXP ')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

ipv4_udp_exp = Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        Dot1Q(vlan=16) / \
        IP(dst='2.0.0.1', src='192.0.2.1', id=0, ttl=64) / \
        UDP(sport=0x2710, dport=0x03e9) / payload

dump_pkt(ipv4_udp_exp)

print('H2S L2 IPv4 ICMP ')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'
ipv4_icmp = Ether(dst='00:00:f1:d0:d1:d0', src='00:00:00:40:08:01') / \
        Dot1Q(vlan=16) / \
        IP(dst='192.0.2.1', src='2.0.0.1', id=0, ttl=64) / \
        ICMP(type="echo-request", code=0, id=0x1234) / payload
dump_pkt(ipv4_icmp)

print('H2S L2 ICMP EXP')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_icmp_exp = Ether(dst='00:06:07:08:09:0A', src='00:01:02:03:04:05') / \
        Dot1Q(vlan=2) / \
        IP(dst='1.2.3.4', src='4.3.2.1', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:F1:D0:D1:D0', src='00:00:00:40:08:01') / \
        IP(dst='192.0.2.1', src='2.0.0.1', id=0, ttl=64) / \
        ICMP(type="echo-request", code=0, id=0x1234) / payload

dump_pkt(geneve_ipv4_icmp_exp)

print('S2H L2 ICMP INJ')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_icmp_inj = Ether(dst='00:01:02:03:04:05', src='00:06:07:08:09:0A') / \
        Dot1Q(vlan=2) / \
        IP(dst='4.3.2.1', src='1.2.3.4', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        IP(dst='2.0.0.1', src='192.0.2.1', id=0, ttl=64) / \
        ICMP(type="echo-request", code=0, id=0x1234) / payload

dump_pkt(geneve_ipv4_icmp_inj)

print('S2H L2 ICMP EXP ')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

ipv4_icmp_exp = Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        Dot1Q(vlan=16) / \
        IP(dst='2.0.0.1', src='192.0.2.1', id=0, ttl=64) / \
        ICMP(type="echo-request", code=0, id=0x1234) / payload

dump_pkt(ipv4_icmp_exp)

print('H2S L2 IPv4 OTHERS ')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'
ipv4_others = Ether(dst='00:00:f1:d0:d1:d0', src='00:00:00:40:08:01') / \
        Dot1Q(vlan=16) / \
        IP(dst='192.0.2.1', src='2.0.0.1', proto=51, id=0, ttl=64) / payload
dump_pkt(ipv4_others)

print('H2S L2 OTHERS EXP')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_others_exp = Ether(dst='00:06:07:08:09:0A', src='00:01:02:03:04:05') / \
        Dot1Q(vlan=2) / \
        IP(dst='1.2.3.4', src='4.3.2.1', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:F1:D0:D1:D0', src='00:00:00:40:08:01') / \
        IP(dst='192.0.2.1', src='2.0.0.1', proto=51, id=0, ttl=64) / payload

dump_pkt(geneve_ipv4_others_exp)

print('S2H L2 OTHERS INJ')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

opt_src_slot_id='\x00\x00\x21\x01\x00\x10\x20\x30'
opt_dst_slot_id='\x00\x00\x22\x01\x00\x01\x23\x52'

geneve_options=opt_src_slot_id + opt_dst_slot_id

geneve_ipv4_others_inj = Ether(dst='00:01:02:03:04:05', src='00:06:07:08:09:0A') / \
        Dot1Q(vlan=2) / \
        IP(dst='4.3.2.1', src='1.2.3.4', id=0, ttl=64) / \
        UDP(sport=0x0, chksum=0) / \
        GENEVE(vni=0xA0B0C0, options=geneve_options) / \
        Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        IP(dst='2.0.0.1', src='192.0.2.1', proto=51, id=0, ttl=64) / payload

dump_pkt(geneve_ipv4_others_inj)

print('S2H L2 OTHERS EXP ')
payload = 'abcdefghijlkmnopqrstuvwzxyabcdefghijlkmnopqrstuvwzxy'

ipv4_others_exp = Ether(dst='00:00:00:40:08:01', src='00:00:F1:D0:D1:D0') / \
        Dot1Q(vlan=16) / \
        IP(dst='2.0.0.1', src='192.0.2.1', proto=51, id=0, ttl=64) / payload

dump_pkt(ipv4_others_exp)



###############################################################################
# end Athena Switch-to-Host IPv4 TCP
###############################################################################
