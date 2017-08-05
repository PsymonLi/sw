# Test Spec
config_filter:
    src:
        tenant  : filter/any
        segment : filter/any
        endpoint: filter/any
    dst:
        tenant  : filter/any
        segment : filter/any
        endpoint: filter/any
    flow        : filter/fwtype=L2,proto=UDP,dport=128
    maxflows    : 1

packets:
    - packet:
        id          : PKT1     # Input packet 1
        payloadsize : ref://factory/payloads/id=PAYLOAD_ZERO64/size
        template    : ref://factory/packets/id=ETH_IPV4_UDP
        encaps      : 
            - ref://factory/packets/id=ENCAP_QTAG
        headers     :
            eth:
                src     : ref://testcase/config/src/endpoint/macaddr
                dst     : ref://testcase/config/dst/endpoint/macaddr
            qtag:
                vlan    : ref://testcase/config/src/segment/vlan_id
            ipv4:
                src     : ref://testcase/config/flow/sip
                dst     : ref://testcase/config/flow/dip
                ttl     : callback://networking/packets/GetIpv4Ttl
            udp:
                sport   : ref://testcase/config/flow/sport
                dport   : ref://testcase/config/flow/dport
            payload:
                data    : ref://factory/payloads/id=PAYLOAD_ZERO64/data

trigger:
    packets:
        - packet:
            object  : ref://testcase/packets/id=PKT1
            port    : 1

expect:
    packets:
        - packet:
            object  : ref://testcase/packets/id=PKT1
            port    : 0
