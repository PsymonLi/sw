# Segment Configuration Spec
meta:
    id: SEGMENT_AGENT_UP2UP

type        : tenant
native      : False
broadcast   : drop
multicast   : drop
l4lb        : False
endpoints   :
    sgenable: True
    useg    : 0
    pvlan   : 0
    direct  : 0
    remote  : 2 # Remote TEPs
