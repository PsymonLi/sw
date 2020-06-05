#!/usr/bin/python3

'''
Flow related definitions and methods
'''

class FlowsReq:

    def __init__(self, vnic_type='L3', nat='no', proto ='UDP', \
                        flow_type='dynamic', flow_count=1):
        self.vnic_type = vnic_type
        self.nat = nat
        self.proto = proto
        self.flow_type = flow_type
        self.flow_count = flow_count







