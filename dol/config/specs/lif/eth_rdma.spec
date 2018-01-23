# LIF Configuration Template
meta:
    id: LIF_ETH_RDMA

rdma:
    enable: True
    max_pt_entries: 1024
    max_keys: 1024

queue_types:
    - queue_type:
        id          : RDMA_SQ
        type        : 0
        purpose     : LIF_QUEUE_PURPOSE_RDMA_SEND
        size        : 1024
        count       : 32
        queues:
            - queue:
                id          : Q0
                count       : 32
                rings:
                    - ring:
                        id          : SQ
                        pi          : 0
                        ci          : 0
                        size        : 32
                        desc        : ref://factory/templates/id=DESC_RDMA_SQ
                    - ring:
                        id          : RRQ
                        pi          : 0
                        ci          : 0
                        size        : 32
                        desc        : ref://factory/templates/id=DESC_RDMA_RRQ

    - queue_type:
        id          : RDMA_RQ
        type        : 1
        purpose     : LIF_QUEUE_PURPOSE_RDMA_RECV
        size        : 1024
        count       : 32
        queues:
            - queue:
                id          : Q0
                count       : 32
                rings:
                    - ring:
                        id          : RQ
                        pi          : 0
                        ci          : 0
                        size        : 32
                        desc        : ref://factory/templates/id=DESC_RDMA_RQ
                    - ring:
                        id          : RSQ
                        pi          : 0
                        ci          : 0
                        size        : 32
                        desc        : ref://factory/templates/id=DESC_RDMA_RSQ

    - queue_type:
        id          : TX
        type        : 2
        purpose     : LIF_QUEUE_PURPOSE_TX
        size        : 64    # Size of Qstate
        count       : 16    # Number of queues of this type
        queues:
            - queue:
                size        : 256    # Number of descriptors in each ring
                rings:
                    - ring:
                        id          : R0
                        buf         : ref://factory/templates/id=ETH_BUFFER
                        desc        : ref://factory/templates/id=DESCR_ETH_TX

                    - ring:
                        id          : R1
                        desc        : ref://factory/templates/id=DESCR_ETH_TX_CQ

    - queue_type:
        id          : ADMIN
        type        : 3
        purpose     : LIF_QUEUE_PURPOSE_ADMIN
        size        : 64    # Size of Qstate
        count       : 32    # Number of queues of this type
        queues:
            - queue:
                size        : 256    # Number of descriptors in each ring
                rings:
                    - ring:
                        id          : R0
                        desc        : ref://factory/templates/id=DESCR_ADMIN

                    - ring:
                        id          : R1
                        desc        : ref://factory/templates/id=DESCR_ADMIN_CQ

    - queue_type:
        id          : RX
        type        : 4
        purpose     : LIF_QUEUE_PURPOSE_RX
        size        : 64    # Size of Qstate
        count       : 16    # Number of queues of this type
        queues:
            - queue:
                size        : 256    # Number of descriptors in each ring
                rings:
                    - ring:
                        id          : R0
                        buf         : ref://factory/templates/id=ETH_BUFFER
                        desc        : ref://factory/templates/id=DESCR_ETH_RX

                    - ring:
                        id          : R1
                        desc        : ref://factory/templates/id=DESCR_ETH_RX_CQ

    - queue_type:
        id          : RDMA_CQ
        type        : 5
        purpose     : LIF_QUEUE_PURPOSE_CQ
        size        : 32
        count       : 32
        queues:
            - queue:
                id          : Q0
                count       : 32
                rings:
                    - ring:
                        id          : CQ
                        pi          : 0
                        ci          : 0
                        size        : 32
                        desc        : ref://factory/templates/id=DESC_RDMA_CQ

    - queue_type:
        id          : RDMA_EQ
        type        : 6
        purpose     : LIF_QUEUE_PURPOSE_EQ
        size        : 32
        count       : 32
        queues:
            - queue:
                id          : Q0
                count       : 32
                rings:
                    - ring:
                        id          : EQ
                        pi          : 0
                        ci          : 0
                        size        : 32
                        desc        : ref://factory/templates/id=DESC_RDMA_EQ
