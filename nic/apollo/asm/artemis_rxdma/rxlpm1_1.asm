#include "../../p4/include/lpm_defines.h"

#include "artemis_rxdma.h"
#include "INGRESS_p.h"
#include "ingress.h"

struct phv_                p;
struct rxlpm1_1_k          k;
struct rxlpm1_1_d          d;

// Define Table Name and Action Names
#define table_name         rxlpm1_1
#define action_keys32b     match1_1_32b
#define action_keys128b    match1_1_128b

// Define table field names for the selected actions
#define keys32b(a)         d.u.match1_1_32b_d.key ## a
#define keys128bhi(a)      d.u.match1_1_128b_d.key ## a[127:64]
#define keys128blo(a)      d.u.match1_1_128b_d.key ## a[63:0]

// Define key field names
#define key                k.lpm_metadata_lpm1_key
#define keylo              k.lpm_metadata_lpm1_key[63:0]
#define keyhi              k.lpm_metadata_lpm1_key[127:64]
#define base_addr          k.lpm_metadata_lpm1_base_addr
#define curr_addr          k.lpm_metadata_lpm1_next_addr

// Define PHV field names
#define next_addr          p.lpm_metadata_lpm1_next_addr

%%

#include "../include/lpm.h"
