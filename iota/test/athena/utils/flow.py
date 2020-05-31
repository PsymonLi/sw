#!/usr/bin/python3

'''
Flow related definitions and methods
'''

class FlowInfo:

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



class FlowsReq:

    def __init__(self, vnic_type='L3', nat='no', proto ='UDP', \
                        flow_type='dynamic', flow_count=1):
        self.vnic_type = vnic_type
        self.nat = nat
        self.proto = proto
        self.flow_type = flow_type
        self.flow_count = flow_count







