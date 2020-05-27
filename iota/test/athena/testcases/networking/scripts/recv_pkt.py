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

logging.basicConfig(level=logging.INFO, handlers=[logging.StreamHandler(sys.stdout)])

DEFAULT_NUM_RECV_PKTS = 10
DEFAULT_UNMATCHED_RECV_PKTS_FILENAME = './unmatched_recv_pkts.pcap'

def get_curr_time():
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()) 

def validate_recv_pkt(recv_pkts, gen_pkts):
    
    # validate received packets
    count = 0
    unmatched_pkts = []

    # zero out outer udp csum since we ignore it for validation
    gen_pkts[0]['UDP'].chksum = 0

    for r_pkt in recv_pkts:
        if True:
        #if encap_info['type'] == 'mplsoudp':
            r_pkt['UDP'].chksum = 0
            if MPLS in r_pkt: 
                # TODO: check if it's a bug
                r_pkt.getlayer(MPLS, nb=1).ttl = 0
                r_pkt.getlayer(MPLS, nb=2).ttl = 0
                if UDP in r_pkt['MPLS'].underlayer:
                    # ignore randomly generated sport when comparing mpls-in-udp pkts
                    r_pkt['UDP'].sport = 0  
            #print("Received pkt: {}".format(r_pkt.show()))
            #print("Generated pkt: {}".format(gen_pkts[0].show()))

            if r_pkt == gen_pkts[0]:
                count += 1    
            else:
                unmatched_pkts.append(r_pkt)  

    if count != DEFAULT_NUM_RECV_PKTS:
        logging.error('FAIL !! - Received matching pkt count (%d pkts) '
                'doesn\'t equal sent pkt count '
                'Writing unmatched pkts to %s.' % (count, 
                DEFAULT_UNMATCHED_RECV_PKTS_FILENAME))
            
        with open(DEFAULT_UNMATCHED_RECV_PKTS_FILENAME, 'w+') as fd:
            wrpcap(fd.name, unmatched_pkts)
        
        return 1

    else:
        logging.info('PASS !! - Received matching pkt count (%d pkts) equals '
                'sent pkt count ' % (count))

        return 0


def recv_packet(intf, fname, timeout):

    # receive packets
    logging.info('[%s] Started sniffing packets on interface %s' %
                (get_curr_time(), intf))

    recv_pkts = sniff(iface=intf, timeout=timeout)

    logging.info('[%s] Stopped sniffing packets on interface %s' %
            (get_curr_time(), intf))

    # expected pkt
    gen_pkts = rdpcap(fname)

    # validate received packets
    ret = validate_recv_pkt(recv_pkts, gen_pkts)

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
    args = parser.parse_args()

    return recv_packet(args.intf_name, args.pcap_fname, args.timeout)

if __name__ == '__main__':
    main()
