#ifndef __E_PSE_H__
#define __E_PSE_H__

#include "pse_utils.h"
#include "pse_mem.h"
#include "nic/sdk/include/sdk/pse_intf.h"
#include <stdio.h>
#include <string.h>
#include <openssl/engine.h>
#include "nic/hal/pd/pd_api_c.h"

//#define NO_PEN_HW_OFFLOAD

#ifndef min
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) > (b) ? (a) : (b))
#endif

extern const char *engine_pse_id;
extern const char *engine_pse_name;

#endif /* __E_PSE_H__ */
