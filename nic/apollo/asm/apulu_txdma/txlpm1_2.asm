#include "../../p4/include/lpm_defines.h"

#include "nic/apollo/asm/include/apulu.h"
#include "INGRESS_p.h"
#include "ingress.h"

struct phv_                p;
struct txlpm1_2_k          k;
struct txlpm1_2_d          d;

// Define Table Name and Action Names
#define table_name         txlpm1_2
#define action_keys32b     match1_2_32b
#define action_keys64b     match1_2_64b

// Define table field names for the selected key-widths
#define keys32b(a)         d.u.match1_2_32b_d.key ## a
#define keys64b(a)         d.u.match1_2_64b_d.key ## a

// Define key field names
#define key                k.txdma_control_lpm1_key[63:0]
#define base_addr          k.txdma_control_lpm1_base_addr
#define curr_addr          k.txdma_control_lpm1_base_addr

// Define PHV field names
#define next_addr          p.txdma_control_lpm1_next_addr

%%

#include "../include/lpm.h"
