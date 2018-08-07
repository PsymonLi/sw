# Flow generation configuration template.

meta:
    id: SESSION_UDP_SERVICES_PROXY

proto: udp

entries:
    - entry:
        label: vxlan
        responder:
            port : const/4789
        initiator: 
            port : const/0
    - entry:
        label: ipsec-proxy 
        responder:
            port : const/44444
        initiator: 
            port : const/44445
   
