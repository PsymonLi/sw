#define SACL_NUM_RULES                    1024      // Number of rules per policy

#define SACL_SPORT_CLASSID_WIDTH          7         // width in bits
#define SACL_PROTO_DPORT_CLASSID_WIDTH    8         // width in bits
#define SACL_SIP_STAG_CLASSID_WIDTH       10        // width in bits
#define SACL_DIP_DTAG_CLASSID_WIDTH       10        // width in bits
#define SACL_P1_CLASSID_WIDTH             10        // width in bits
#define SACL_P2_CLASSID_WIDTH             10        // width in bits
#define SACL_P3_ENTRY_WIDTH               10        // width in bits
#define SACL_RSLT_ENTRY_WIDTH             16        // width in bits

#define SACL_SPORT_TREE_MAX_CLASSES       (1<<SACL_SPORT_CLASSID_WIDTH)
#define SACL_PROTO_DPORT_TREE_MAX_CLASSES (1<<SACL_PROTO_DPORT_CLASSID_WIDTH)
#define SACL_STAG_TREE_MAX_CLASSES        (1<<SACL_SIP_STAG_CLASSID_WIDTH)
#define SACL_DTAG_TREE_MAX_CLASSES        (1<<SACL_DIP_DTAG_CLASSID_WIDTH)
#define SACL_IPV4_DIP_TREE_MAX_CLASSES    (1<<SACL_DIP_DTAG_CLASSID_WIDTH)
#define SACL_IPV6_DIP_TREE_MAX_CLASSES    (1<<SACL_DIP_DTAG_CLASSID_WIDTH)
#define SACL_IPV4_SIP_TREE_MAX_CLASSES    (1<<SACL_SIP_STAG_CLASSID_WIDTH)
#define SACL_IPV6_SIP_TREE_MAX_CLASSES    (1<<SACL_SIP_STAG_CLASSID_WIDTH)
#define SACL_P1_MAX_CLASSES               (1<<SACL_P1_CLASSID_WIDTH)
#define SACL_P2_MAX_CLASSES               (1<<SACL_P2_CLASSID_WIDTH)

#define SACL_SPORT_TABLE_SIZE             2112      // 64+(32*64)
#define SACL_PROTO_DPORT_TABLE_SIZE       17472     // 64+(16*64)+(16*16*64)
#define SACL_STAG_TABLE_SIZE              17472     // 64+(16*64)+(16*16*64)
#define SACL_DTAG_TABLE_SIZE              17472     // 64+(16*64)+(16*16*64)
#define SACL_IPV4_DIP_TABLE_SIZE          17472     // 64+(16*64)+(16*16*64)
#define SACL_IPV4_SIP_TABLE_SIZE          17472     // 64+(16*64)+(16*16*64)
#define SACL_IPV6_DIP_TABLE_SIZE          87360     // 64+(4*64)+(16*64)+(64*64)
                                                    // +(256*64)+(1024*64)
#define SACL_IPV6_SIP_TABLE_SIZE          87360     // 64+(4*64)+(16*64)+(64*64)
                                                    // +(256*64)+(1024*64)

#define SACL_SPORT_TREE_MAX_NODES         511       // (31 + 32 * 15)
#define SACL_PROTO_DPORT_TREE_MAX_NODES   2047      // 15 + (16 * 15) + (256 * 7)
#define SACL_STAG_TREE_MAX_NODES          2047      // 15 + (16 * 15) + (256 * 7)
#define SACL_DTAG_TREE_MAX_NODES          2047      // 15 + (16 * 15) + (256 * 7)
#define SACL_IPV4_DIP_TREE_MAX_NODES      2047      // for 1023 prefixes.
                                                    // 15+(16*15)+(256*7)
#define SACL_IPV6_DIP_TREE_MAX_NODES      2047      // for 1023 prefixes.
                                                    // 3+(4*3)+(16*3)+(64*3)
                                                    // +(256*3)+(1024*1)
#define SACL_IPV4_SIP_TREE_MAX_NODES      2047      // for 1023 prefixes.
                                                    // 15+(16*15)+(256*7)
#define SACL_IPV6_SIP_TREE_MAX_NODES      2047      // for 1023 prefixes.
                                                    // 3+(4*3)+(16*3)+(64*3)
                                                    // +(256*3)+(1024*1)

#define SACL_CACHE_LINE_SIZE              64
#define SACL_P1_ENTRIES_PER_CACHE_LINE    51        // 51 entries of 10 bits
#define SACL_P2_ENTRIES_PER_CACHE_LINE    51        // 51 entries of 10 bits
#define SACL_P3_ENTRIES_PER_CACHE_LINE    51        // 51 entries of 10 bits
#define SACL_RSLT_ENTRIES_PER_CACHE_LINE  32        // 32 entries of 16 bits

#define SACL_P1_TABLE_NUM_ENTRIES         (1 << 17) // SIP:SPORT (2^10 * 2^7)
#define SACL_P1_TABLE_SIZE                164544    // round64((1<<17)*(64/51))

#define SACL_P2_TABLE_NUM_ENTRIES         (1 << 18) // DIP:PROTO_DPORT (2^10 * 2^8)
#define SACL_P2_TABLE_SIZE                329024    // round64((1<<18)*(64/51))

#define SACL_P3_TABLE_NUM_ENTRIES         (1 << 20) // P1:P2 (2^10 * 2^10)
#define SACL_P3_TABLE_SIZE                1315904   // round64((1<<20)*(64/51))

#define SACL_RSLT_TABLE_NUM_ENTRIES       SACL_NUM_RULES
#define SACL_RSLT_TABLE_SIZE             (SACL_RSLT_TABLE_NUM_ENTRIES * \
                                          SACL_CACHE_LINE_SIZE / \
                                          SACL_RSLT_ENTRIES_PER_CACHE_LINE)

#define SACL_SPORT_TABLE_OFFSET         0
#define SACL_PROTO_DPORT_TABLE_OFFSET   (SACL_SPORT_TABLE_SIZE)
#define SACL_STAG_TABLE_OFFSET          (SACL_PROTO_DPORT_TABLE_OFFSET +\
                                         SACL_PROTO_DPORT_TABLE_SIZE)
#define SACL_DTAG_TABLE_OFFSET          (SACL_STAG_TABLE_OFFSET +\
                                         SACL_STAG_TABLE_SIZE)
#define SACL_P1_TABLE_OFFSET            (SACL_DTAG_TABLE_OFFSET +\
                                         SACL_DTAG_TABLE_SIZE)
#define SACL_P2_TABLE_OFFSET            (SACL_P1_TABLE_OFFSET +\
                                         SACL_P1_TABLE_SIZE)
#define SACL_P3_TABLE_OFFSET            (SACL_P2_TABLE_OFFSET +\
                                         SACL_P2_TABLE_SIZE)
#define SACL_RSLT_TABLE_OFFSET          (SACL_P3_TABLE_OFFSET +\
                                         SACL_P3_TABLE_SIZE)
#define SACL_DIP_TABLE_OFFSET           (SACL_RSLT_TABLE_OFFSET +\
                                         SACL_RSLT_TABLE_SIZE)
#define SACL_IPV4_SIP_TABLE_OFFSET      (SACL_DIP_TABLE_OFFSET +\
                                         SACL_IPV4_DIP_TABLE_SIZE)
#define SACL_IPV6_SIP_TABLE_OFFSET      (SACL_DIP_TABLE_OFFSET +\
                                         SACL_IPV6_DIP_TABLE_SIZE)
#define SACL_IPV4_BLOCK_SIZE            (SACL_IPV4_SIP_TABLE_OFFSET + \
                                         SACL_IPV4_SIP_TABLE_SIZE)
#define SACL_IPV6_BLOCK_SIZE            (SACL_IPV6_SIP_TABLE_OFFSET + \
                                         SACL_IPV6_SIP_TABLE_SIZE)

#define SACL_RULE_ID_DEFAULT            1023
#define SACL_PRIORITY_HIGHEST           0
#define SACL_PRIORITY_LOWEST            0x3FF
#define SACL_PRIORITY_INVALID           (SACL_PRIORITY_LOWEST+1)
#define SACL_RSLT_ENTRY_ACTION_ALLOW    0x0
#define SACL_RSLT_ENTRY_ACTION_DENY     0x1
#define SACL_RSLT_ENTRY_ACTION_SHIFT    0
#define SACL_RSLT_ENTRY_PRIORITY_SHIFT  1
#define SACL_RSLT_ENTRY_STATEFUL_SHIFT  11
#define SACL_RSLT_ENTRY_ALG_TYPE_SHIFT  12

#define SACL_RSLT_ENTRY_ACTION_MASK     0x0001
#define SACL_RSLT_ENTRY_PRIORITY_MASK   0x07FE
#define SACL_RSLT_ENTRY_STATEFUL_MASK   0x0800
#define SACL_RSLT_ENTRY_ALG_TYPE_MASK   0xF000

#define SACL_TAG_RSVD                  (0)
#define SACL_CLASSID_DEFAULT           (0)

#define SACL_PROC_STATE_IDLE           (0)
#define SACL_PROC_STATE_SPORT_DPORT    (1)
#define SACL_PROC_STATE_SIP_DIP        (2)
#define SACL_PROC_STATE_STAG0_DTAG0    (3)
#define SACL_PROC_STATE_STAG1_DTAG1    (4)
#define SACL_PROC_STATE_STAG2_DTAG2    (5)
#define SACL_PROC_STATE_STAG3_DTAG3    (6)
#define SACL_PROC_STATE_STAG4_DTAG4    (7)

#define SACL_COUNTER_SIZE               8
#define SACL_COUNTER_BLOCK_SIZE        (SACL_COUNTER_SIZE * \
                                        SACL_NUM_RULES)

#define SACL_ROOT_COUNT_MAX            6
