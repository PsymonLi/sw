# Segment Configuration Spec
meta:
    id: SEGMENT_DOS_SRC_DST

label       : dos_remote
type        : tenant
native      : False
broadcast   : flood
multicast   : flood
l4lb        : False
networks    :
    sgenable: True
endpoints   :
    sgenable: True
    useg    : 0
    pvlan   : 0
    direct  : 0
    remote  : 2
