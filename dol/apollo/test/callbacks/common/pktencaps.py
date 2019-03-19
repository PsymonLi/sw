#! /usr/bin/python3

import pdb

def AddPacketEncapQtag(pkt, encap):
    if pkt.hdrsorder[0] == 'outereth':
        pkt.headers.qtag.fields.type = pkt.headers.outereth.fields.type
        pkt.headers.outereth.fields.type = encap.headers.eth.fields.type
    elif  pkt.hdrsorder[0] == 'eth':
        pkt.headers.qtag.fields.type = pkt.headers.eth.fields.type
        pkt.headers.eth.fields.type = encap.headers.eth.fields.type
    pkt.hdrsorder.insert(1, 'qtag')
    return

def AddPacketEncapQinQ(pkt, encap):
    if pkt.hdrsorder[0] == 'outereth':
        pkt.headers.qtag.fields.type = pkt.headers.outereth.fields.type
        pkt.headers.outereth.fields.type = encap.headers.eth.fields.type
    elif  pkt.hdrsorder[0] == 'eth':
        pkt.headers.qtag.fields.type = pkt.headers.eth.fields.type
        pkt.headers.eth.fields.type = encap.headers.eth.fields.type
    pkt.hdrsorder.insert(1, 'qtag')
    pkt.hdrsorder.insert(1, 'outerqtag')
    return

def AddPacketEncapVxlan(pkt, encap):
    pkt.hdrsorder = ['outereth', 'outeripv4', 'outerudp', 'vxlan'] + pkt.hdrsorder
    return

def AddPacketEncapMpls(pkt, encap):
    #pdb.set_trace()
    pkt.hdrsorder = pkt.hdrsorder[1:] # Remove both ethernet and vlan if present
    if pkt.hdrsorder[0] == 'qtag':
        pkt.hdrsorder = pkt.hdrsorder[1:]
    pkt.hdrsorder = ['outereth', 'outeripv4', 'outerudp', 'mpls'] + pkt.hdrsorder
    return

def AddPacketEncapMpls2(pkt, encap):
    #pdb.set_trace()
    pkt.hdrsorder = pkt.hdrsorder[1:] # Remove both ethernet and vlan if present
    if pkt.hdrsorder[0] == 'qtag':
        pkt.hdrsorder = pkt.hdrsorder[1:]
    pkt.hdrsorder = ['outereth', 'outeripv4', 'outerudp', 'outermpls', 'mpls'] + pkt.hdrsorder
    return


def AddPacketEncapErspan(pkt, encap):
    pkt.hdrsorder = ['erspaneth', 'erspanqtag', 'erspanipv4', 'erspangre', 'erspan'] + pkt.hdrsorder
    return

def AddPacketEncapGre(pkt, encap):
    pkt.hdrsorder = ['outereth', 'outeripv4', 'outergre'] + pkt.hdrsorder
    return
