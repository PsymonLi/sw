# Flow generation configuration template.

meta:
    id: SESSION_UDP_BASIC

proto: udp

entries:
    - entry:
        responder: const/128
        initiator: const/47273
        span     : rspan

    - entry:
        responder: const/1
        initiator: const/40000
        span     : rspan_vxlan
