#!/usr/bin/python3

'''
Utils script to craft a packet in scapy based on 
given args and policy.json and send it on wire
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
from scapy.utils import rdpcap
from random import randint

logging.basicConfig(level=logging.INFO, handlers=[logging.StreamHandler(sys.stdout)])


def get_curr_time():
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()) 

def send_packet(intf, fname, pkt_cnt):

    pkts = rdpcap(fname)
    for pkt in pkts:
        #logging.debug("Tx packet: {}".format(pkt.show()))
        logging.info('[%s] Sending %d packets on interface %s' % (
                            get_curr_time(), int(pkt_cnt), intf))    
        sendp(pkt, iface=intf, count=pkt_cnt)


def main():
    parser = argparse.ArgumentParser(description='script to craft and send ' 
                                                'pkts on wire with scapy')

    parser.add_argument('--intf_name', type=str, default=None, 
                        help='Tx Interface')
    parser.add_argument('--pcap_fname', type=str, default=None,
                        help='pcap file name')
    parser.add_argument('--pkt_cnt', type=int, default=None,
                        help='num pkts to send')
    args = parser.parse_args()

    return send_packet(args.intf_name, args.pcap_fname, args.pkt_cnt)

if __name__ == '__main__':
    main()
