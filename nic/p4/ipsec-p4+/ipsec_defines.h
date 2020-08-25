#define RING_INDEX_WIDTH            16

#define ESP_FIXED_HDR_SIZE          8 
#define AOL_OFFSET_WIDTH            32
#define AOL_LENGTH_WIDTH            32

#define IPSEC_WIN_REPLAY_MAX_DIFF   63

/*****************************************************************************/
/* Flags in IPSEC CB                                                         */
/*****************************************************************************/
#define IPSEC_FLAGS_V6_MASK         0x1
#define IPSEC_FLAGS_NATT_MASK       0x2
#define IPSEC_FLAGS_RANDOM_MASK     0x4
#define IPSEC_FLAGS_EXTRA_PAD       0x8
#define IPSEC_FLAGS_ENCAP_VLAN_MASK 0x10
#define IPSEC_FLAGS_MODE_TRANSPORT  0x20
