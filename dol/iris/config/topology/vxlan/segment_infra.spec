# Segment Configuration Spec
meta:
    id: VXLANTOPO_SEGMENT_INFRA

type        : infra
fabencap    : vlan
native      : False
broadcast   : flood
multicast   : flood
l4lb        : False
endpoints   :
    sgenable: True
    useg    : 0
    pvlan   : 0
    direct  : 0
    remote  : 4 # Remote TEPs
