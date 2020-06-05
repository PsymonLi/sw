#!/usr/bin/python3

'''
Utils script to receive a packet and validate it using 
scapy based on given args and policy.json config
'''
import argparse
import logging
import subprocess 
import json
import sys
import time

from ipaddress import ip_address
from scapy.all import *
from scapy.contrib.mpls import MPLS
from scapy.contrib.geneve import GENEVE

DEFAULT_UNMATCHED_RECV_PKTS_FILENAME = './unmatched_recv_pkts.pcap'

logging.basicConfig(level=logging.INFO, handlers=[logging.StreamHandler(sys.stdout)])


def clr_pkt_csum(pkt, hdr, layer):
    if pkt.getlayer(hdr, nb=layer) is not None:
        pkt.getlayer(hdr, nb=layer).chksum = 0

def get_curr_time():
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()) 

def validate_recv_pkt(recv_pkts, gen_pkts, exp_pkt_cnt):
    
    # validate received packets
    count = 0
    unmatched_pkts = []
    gen_pkt = gen_pkts[0]

    # zero out user pkt IP and UDP csum becoz of NAT translation in regular case
    # zero out outer udp csum in encap case since mplsoudp src port is random, 
    # but for now do both IP and UDP 
    clr_pkt_csum(gen_pkt, IP, 1)
    clr_pkt_csum(gen_pkt, IP, 2)
    
    for hdr in [UDP, TCP, ICMP]:
        for layer in [1, 2]:
            clr_pkt_csum(gen_pkt, hdr, layer)

    for r_pkt in recv_pkts:
        # ignore DHCP packets in the capture
        if r_pkt.haslayer('DHCP') or r_pkt.haslayer('ICMPv6ND_RS'):
            continue

        # skip if not IP pkt
        if r_pkt.getlayer(IP, nb=1) is None:
            continue

        clr_pkt_csum(r_pkt, IP, 1)
        
        # skip if pkt is not UDP/TCP/ICMP. Works for both h2s and s2h
        if not (r_pkt.getlayer(UDP, nb=1) or r_pkt.getlayer(TCP, nb=1) or
                r_pkt.getlayer(ICMP, nb=1)):
            continue

        for hdr in [UDP, TCP, ICMP]:
            clr_pkt_csum(r_pkt, hdr, 1)

        # clear inner hdr csum if hdr available
        clr_pkt_csum(r_pkt, IP, 2)
        for hdr in [UDP, TCP, ICMP]:
            clr_pkt_csum(r_pkt, hdr, 2)

        if MPLS in r_pkt: 
            # TODO: check if ttl value is a bug
            if r_pkt.getlayer(MPLS, nb=1) is not None:
                r_pkt.getlayer(MPLS, nb=1).ttl = 0
            if r_pkt.getlayer(MPLS, nb=2) is not None:
                r_pkt.getlayer(MPLS, nb=2).ttl = 0
            
            if UDP in r_pkt['MPLS'].underlayer:
                # ignore randomly generated sport when comparing mplsoudp pkts
                r_pkt.getlayer(UDP, nb=1).sport = 0
            
        elif GENEVE in r_pkt:
            if UDP in r_pkt['GENEVE'].underlayer:
                r_pkt.getlayer(UDP, nb=1).sport = 0

        
        if r_pkt == gen_pkt:
            count += 1    
        else:
            logging.debug("Received pkt: {}".format(r_pkt.show()))
            logging.debug("Generated pkt: {}".format(gen_pkts[0].show()))
            unmatched_pkts.append(r_pkt)  

    if count != exp_pkt_cnt:
        logging.error('FAIL !! - Received matching pkt count (%d pkts) '
                'doesn\'t equal sent pkt count (%d pkts). '
                'Writing unmatched pkts to %s.' % (count, exp_pkt_cnt,
                DEFAULT_UNMATCHED_RECV_PKTS_FILENAME))
            
        with open(DEFAULT_UNMATCHED_RECV_PKTS_FILENAME, 'w+') as fd:
            wrpcap(fd.name, unmatched_pkts)
        
        return 1

    else:
        logging.info('PASS !! - Received matching pkt count (%d pkts) equals '
                'sent pkt count (%d pkts)' % (count, exp_pkt_cnt))

        return 0


def recv_packet(intf, fname, timeout, exp_pkt_cnt):

    # receive packets
    logging.info('[%s] Started sniffing packets on interface %s' %
                (get_curr_time(), intf))

    recv_pkts = sniff(iface=intf, timeout=timeout)

    logging.info('[%s] Stopped sniffing packets on interface %s' %
            (get_curr_time(), intf))

    # expected pkt
    gen_pkts = rdpcap(fname)

    # validate received packets
    ret = validate_recv_pkt(recv_pkts, gen_pkts, exp_pkt_cnt)

    if ret != 0:
        sys.exit('Failed')

def main():
    parser = argparse.ArgumentParser(description='script to receive pkts and '
                                                'validate them with scapy')

    parser.add_argument('--intf_name', type=str, default=None, 
                        help='Rx Interface')
    parser.add_argument('--pcap_fname', type=str, default=None,
                        help='pcap file name')
    parser.add_argument('--timeout', type=int, default=None,
                        help='Timeout (secs)')
    parser.add_argument('--pkt_cnt', type=int, default=None,
                        help='num pkts expected')
    args = parser.parse_args()

    return recv_packet(args.intf_name, args.pcap_fname, args.timeout, 
                                                        args.pkt_cnt)

if __name__ == '__main__':
    main()
