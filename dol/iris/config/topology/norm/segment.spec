# Segment Configuration Spec
meta:
    id: SEGMENT_NORM

type        : tenant
native      : False
broadcast   : flood
multicast   : flood
l4lb        : False
endpoints   :
    sgenable: True
    useg    : 0
    pvlan   : 1
    direct  : 0
    remote  : 2 # Remote TEPs
