# Configuration Template.
meta:
    id: TENANT_RDMA

type    : tenant
overlay : vlan

segments:
    - spec  : ref://store/specs/id=SEGMENT_RDMA
      count : 1

# NFV Endpoints:
# - They will attach to a 'TRUNK' Enic.
# - All the segments will be enabled on these Enics              
nfveps: 0

security_profile: ref://store/objects/id=SEC_PROF_DEFAULT

sessions:
    unidest:
        ipv4:
            - ref://store/specs/id=SESSION_UDP_RDMA
        ipv6:
            - ref://store/specs/id=SESSION_UDP_RDMA
        mac: None

lif: ref://store/specs/id=LIF_ETH_RDMA
