# Segment Configuration Spec
meta:
    id  : SEGMENT_NATIVE_EPLEARN

type        : tenant
fabencap    : vlan
native      : True
broadcast   : flood
multicast   : flood
l4lb        : False
proxy_arp_enabled : True
eplearn :
    remote            : False
    arp_entry_timeout : 10
    arp_probe         : True
    dhcp              : True
    dpkt              : True
endpoints   :
    sgenable: True
    useg    : 0
    pvlan   : 4
    direct  : 0
    remote  : 2
