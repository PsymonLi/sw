# Segment Configuration Spec
meta:
    id: SEGMENT_ETH

type        : tenant
native      : False
broadcast   : flood
multicast   : flood
l4lb        : False
endpoints   :
    sgenable: True
    useg    : 0
    pvlan   : 2
    direct  : 0
    remote  : 4 # Remote TEPs
