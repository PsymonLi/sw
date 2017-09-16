# Flow generation configuration template.
meta:
    id: SESSION_ICMP_FTE

proto: icmp

entries:
    - entry:
        label: fte
        fte: True
        initiator:
            type: const/8   # Echo
            code: const/0
            id  : const/1
        responder:
            type: const/0   # Echo Reply
            code: const/0
            id  : const/1
