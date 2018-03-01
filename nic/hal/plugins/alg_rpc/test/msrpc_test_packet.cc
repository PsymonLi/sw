#include <gtest/gtest.h>
#include "nic/hal/src/session.hpp"
#include "nic/hal/plugins/alg_rpc/msrpc_proto_def.hpp"
#include "nic/hal/plugins/alg_utils/core.hpp"
#include "nic/hal/plugins/alg_rpc/core.hpp"
#include "nic/hal/plugins/alg_rpc/alg_msrpc.hpp"
#include "nic/fte/fte_ctx.hpp"
#include "nic/fte/fte_flow.hpp"
#include "nic/hal/test/utils/hal_test_utils.hpp"
#include "nic/hal/test/utils/hal_base_test.hpp"

using namespace std;
using namespace fte;
using namespace hal;
using namespace hal::plugins::alg_utils;
using namespace hal::plugins::alg_rpc;

using sdk::lib::twheel;

/*
 * Note: For now timer wheel is supposed to be non-thread safe.
 * Reason: Update tick locks that slice and calls the callback function.
 * During the callback function, we try to add or delete timer which will
 * obviously wait indefinitely as there is not lock available.
 */

namespace hal {
namespace periodic {
extern twheel *g_twheel;
}
}  // namespace hal

uint8_t MSRPC_BIND_REQ1[] = {0x00, 0x0c, 0x29, 0xbe, 0x2e, 0xe1, 0x00, 0x50, 
                                0x56, 0xc0, 0x00, 0x09, 0x08, 0x00, 0x45, 0x00, 
                                0x00, 0x7c, 0xf6, 0x10, 0x40, 0x00, 0x40, 0x06, 
                                0xd9, 0x58, 0xac, 0x1f, 0x09, 0x01, 0xac, 0x1f, 
                                0x09, 0xd3, 0xd3, 0x24, 0x00, 0x87, 0x0c, 0xc5, 
                                0x4a, 0xc6, 0xbc, 0x78, 0x75, 0xf0, 0x80, 0x18, 
                                0x00, 0x2e, 0xac, 0x79, 0x00, 0x00, 0x01, 0x01, 
                                0x08, 0x0a, 0x04, 0x9a, 0xbc, 0xcb, 0x00, 0x0a, 
                                0xcc, 0x7b, 0x05, 0x00, 0x0b, 0x03, 0x10, 0x00, 
                                0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x01, 0x00, 
                                0x00, 0x00, 0xd0, 0x16, 0xd0, 0x16, 0x00, 0x00, 
                                0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                0x01, 0x00, 0x08, 0x83, 0xaf, 0xe1, 0x1f, 0x5d, 
                                0xc9, 0x11, 0x91, 0xa4, 0x08, 0x00, 0x2b, 0x14, 
                                0xa0, 0xfa, 0x03, 0x00, 0x00, 0x00, 0x04, 0x5d, 
                                0x88, 0x8a, 0xeb, 0x1c, 0xc9, 0x11, 0x9f, 0xe8, 
                                0x08, 0x00, 0x2b, 0x10, 0x48, 0x60, 0x02, 0x00, 
                                0x00, 0x00};
#define MSRPC_BIND_REQ1_SZ 138
#define MSRPC_BIND_REQ1_PAYLOAD_OFFSET 66

uint8_t MSRPC_BIND_RSP1[] = {0x00, 0x50, 0x56, 0xc0, 0x00, 0x09, 0x00, 0x0c, 
                                 0x29, 0xbe, 0x2e, 0xe1, 0x08, 0x00, 0x45, 0x00, 
                                 0x00, 0x70, 0x03, 0x81, 0x40, 0x00, 0x80, 0x06, 
                                 0x8b, 0xf4, 0xac, 0x1f, 0x09, 0xd3, 0xac, 0x1f, 
                                 0x09, 0x01, 0x00, 0x87, 0xd3, 0x24, 0xbc, 0x78, 
                                 0x75, 0xf0, 0x0c, 0xc5, 0x4b, 0x0e, 0x80, 0x18, 
                                 0x01, 0x04, 0x23, 0xb6, 0x00, 0x00, 0x01, 0x01, 
                                 0x08, 0x0a, 0x00, 0x0a, 0xcc, 0x7b, 0x04, 0x9a, 
                                 0xbc, 0xcb, 0x05, 0x00, 0x0c, 0x03, 0x10, 0x00, 
                                 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x01, 0x00, 
                                 0x00, 0x00, 0xd0, 0x16, 0xd0, 0x16, 0x95, 0xb7, 
                                 0x00, 0x00, 0x04, 0x00, 0x31, 0x33, 0x35, 0x00, 
                                 0x9d, 0x4d, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                 0x00, 0x00, 0x04, 0x5d, 0x88, 0x8a, 0xeb, 0x1c, 
                                 0xc9, 0x11, 0x9f, 0xe8, 0x08, 0x00, 0x2b, 0x10, 
                                 0x48, 0x60, 0x02, 0x00, 0x00, 0x00};
#define MSRPC_BIND_RSP1_SZ 126
#define MSRPC_BIND_RSP1_PAYLOAD_OFFSET 66

uint8_t MSRPC_EPM_REQ1[] = {0x00, 0x0c, 0x29, 0xbe, 0x2e, 0xe1, 0x00, 0x50, 
                            0x56, 0xc0, 0x00, 0x09, 0x08, 0x00, 0x45, 0x00, 
                            0x00, 0xd0, 0xf6, 0x12, 0x40, 0x00, 0x40, 0x06, 
                            0xd9, 0x02, 0xac, 0x1f, 0x09, 0x01, 0xac, 0x1f, 
                            0x09, 0xd3, 0xd3, 0x24, 0x00, 0x87, 0x0c, 0xc5, 
                            0x4b, 0x0e, 0xbc, 0x78, 0x76, 0x2c, 0x80, 0x18, 
                            0x00, 0x2e, 0x4d, 0xb5, 0x00, 0x00, 0x01, 0x01, 
                            0x08, 0x0a, 0x04, 0x9a, 0xbc, 0xcc, 0x00, 0x0a, 
                            0xcc, 0x7b, 0x05, 0x00, 0x00, 0x03, 0x10, 0x00, 
                            0x00, 0x00, 0x9c, 0x00, 0x00, 0x00, 0x02, 0x00, 
                            0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 
                            0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
                            0x00, 0x00, 0x4b, 0x00, 0x00, 0x00, 0x4b, 0x00, 
                            0x00, 0x00, 0x05, 0x00, 0x13, 0x00, 0x0d, 0x78, 
                            0x57, 0x34, 0x12, 0x34, 0x12, 0xcd, 0xab, 0xef, 
                            0x00, 0x01, 0x23, 0x45, 0x67, 0x89, 0xac, 0x01, 
                            0x00, 0x02, 0x00, 0x00, 0x00, 0x13, 0x00, 0x0d, 
                            0x04, 0x5d, 0x88, 0x8a, 0xeb, 0x1c, 0xc9, 0x11, 
                            0x9f, 0xe8, 0x08, 0x00, 0x2b, 0x10, 0x48, 0x60, 
                            0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 
                            0x0b, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x07, 
                            0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x09, 0x04, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
#define MSRPC_EPM_REQ1_SZ 222
#define MSRPC_EPM_REQ1_PAYLOAD_OFFSET 66

uint8_t MSRPC_EPM_RSP1[] = {0x00, 0x50, 0x56, 0xc0, 0x00, 0x09, 0x00, 0x0c, 
                            0x29, 0xbe, 0x2e, 0xe1, 0x08, 0x00, 0x45, 0x00, 
                            0x00, 0xcc, 0x03, 0x82, 0x40, 0x00, 0x80, 0x06, 
                            0x8b, 0x97, 0xac, 0x1f, 0x09, 0xd3, 0xac, 0x1f, 
                            0x09, 0x01, 0x00, 0x87, 0xd3, 0x24, 0xbc, 0x78, 
                            0x76, 0x2c, 0x0c, 0xc5, 0x4b, 0xaa, 0x80, 0x18, 
                            0x01, 0x03, 0xba, 0x1f, 0x00, 0x00, 0x01, 0x01, 
                            0x08, 0x0a, 0x00, 0x0a, 0xcc, 0x7c, 0x04, 0x9a, 
                            0xbc, 0xcc, 0x05, 0x00, 0x02, 0x03, 0x10, 0x00, 
                            0x00, 0x00, 0x98, 0x00, 0x00, 0x00, 0x02, 0x00, 
                            0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0xff, 
                            0xf1, 0x31, 0x1f, 0xe4, 0x64, 0x4a, 0x94, 0xd0, 
                            0x75, 0x99, 0xe8, 0x82, 0x4a, 0x22, 0x01, 0x00, 
                            0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 
                            0x00, 0x00, 0x4b, 0x00, 0x00, 0x00, 0x4b, 0x00, 
                            0x00, 0x00, 0x05, 0x00, 0x13, 0x00, 0x0d, 0x78, 
                            0x57, 0x34, 0x12, 0x34, 0x12, 0xcd, 0xab, 0xef, 
                            0x00, 0x01, 0x23, 0x45, 0x67, 0x89, 0xac, 0x01, 
                            0x00, 0x02, 0x00, 0x00, 0x00, 0x13, 0x00, 0x0d, 
                            0x04, 0x5d, 0x88, 0x8a, 0xeb, 0x1c, 0xc9, 0x11, 
                            0x9f, 0xe8, 0x08, 0x00, 0x2b, 0x10, 0x48, 0x60, 
                            0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 
                            0x0b, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x07, 
                            0x02, 0x00, 0xc0, 0x02, 0x01, 0x00, 0x09, 0x04, 
                            0x00, 0xac, 0x1f, 0x09, 0xd3, 0x00, 0x00, 0x00, 
                            0x00, 0x00};
#define MSRPC_EPM_RSP1_SZ 218
#define MSRPC_EPM_RSP1_PAYLOAD_OFFSET 66

uint8_t MSRPC_BIND_REQ2[] = {0x52, 0x54, 0x00, 0x12, 0x35, 0x02, 0x08, 0x00, 
                                 0x27, 0x96, 0xcb, 0x7c, 0x08, 0x00, 0x45, 0x00, 
                                 0x00, 0xc8, 0x0b, 0xfc, 0x40, 0x00, 0x80, 0x06, 
                                 0x00, 0x00, 0x0a, 0x00, 0x02, 0x0f, 0xc0, 0xa8, 
                                 0x03, 0x2b, 0xc8, 0x55, 0x00, 0x87, 0xb1, 0xe9, 
                                 0x3d, 0xdb, 0x24, 0xf2, 0x02, 0x02, 0x50, 0x18, 
                                 0xfa, 0xf0, 0xd0, 0x9c, 0x00, 0x00, 0x05, 0x00, 
                                 0x0b, 0x03, 0x10, 0x00, 0x00, 0x00, 0xa0, 0x00, 
                                 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xd0, 0x16, 
                                 0xd0, 0x16, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 
                                 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x83, 
                                 0xaf, 0xe1, 0x1f, 0x5d, 0xc9, 0x11, 0x91, 0xa4, 
                                 0x08, 0x00, 0x2b, 0x14, 0xa0, 0xfa, 0x03, 0x00, 
                                 0x00, 0x00, 0x04, 0x5d, 0x88, 0x8a, 0xeb, 0x1c, 
                                 0xc9, 0x11, 0x9f, 0xe8, 0x08, 0x00, 0x2b, 0x10, 
                                 0x48, 0x60, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 
                                 0x01, 0x00, 0x08, 0x83, 0xaf, 0xe1, 0x1f, 0x5d, 
                                 0xc9, 0x11, 0x91, 0xa4, 0x08, 0x00, 0x2b, 0x14, 
                                 0xa0, 0xfa, 0x03, 0x00, 0x00, 0x00, 0x33, 0x05, 
                                 0x71, 0x71, 0xba, 0xbe, 0x37, 0x49, 0x83, 0x19, 
                                 0xb5, 0xdb, 0xef, 0x9c, 0xcc, 0x36, 0x01, 0x00, 
                                 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x08, 0x83, 
                                 0xaf, 0xe1, 0x1f, 0x5d, 0xc9, 0x11, 0x91, 0xa4, 
                                 0x08, 0x00, 0x2b, 0x14, 0xa0, 0xfa, 0x03, 0x00, 
                                 0x00, 0x00, 0x2c, 0x1c, 0xb7, 0x6c, 0x12, 0x98, 
                                 0x40, 0x45, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
#define MSRPC_BIND_REQ2_SZ 214
#define MSRPC_BIND_REQ2_PAYLOAD_OFFSET 54

uint8_t MSRPC_BIND_RSP2[] = {0x08, 0x00, 0x27, 0x96, 0xcb, 0x7c, 0x52, 0x54, 
                                 0x00, 0x12, 0x35, 0x02, 0x08, 0x00, 0x45, 0x00, 
                                 0x00, 0x94, 0xa4, 0xbd, 0x00, 0x00, 0x40, 0x06, 
                                 0x05, 0xc5, 0xc0, 0xa8, 0x03, 0x2b, 0x0a, 0x00, 
                                 0x02, 0x0f, 0x00, 0x87, 0xc8, 0x55, 0x24, 0xf2, 
                                 0x02, 0x02, 0xb1, 0xe9, 0x3e, 0x7b, 0x50, 0x18, 
                                 0xff, 0xff, 0x01, 0x8c, 0x00, 0x00, 0x05, 0x00, 
                                 0x0c, 0x03, 0x10, 0x00, 0x00, 0x00, 0x6c, 0x00, 
                                 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xd0, 0x16, 
                                 0xd0, 0x16, 0xcb, 0x10, 0x00, 0x00, 0x04, 0x00, 
                                 0x31, 0x33, 0x35, 0x00, 0x00, 0x00, 0x03, 0x00, 
                                 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x05, 
                                 0x71, 0x71, 0xba, 0xbe, 0x37, 0x49, 0x83, 0x19, 
                                 0xb5, 0xdb, 0xef, 0x9c, 0xcc, 0x36, 0x01, 0x00, 
                                 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                 0x00, 0x00}; 
#define MSRPC_BIND_RSP2_SZ 162
#define MSRPC_BIND_RSP2_PAYLOAD_OFFSET 54

uint8_t MSRPC_EPM_REQ2[] = {0x52, 0x54, 0x00, 0x12, 0x35, 0x02, 0x08, 0x00, 
                            0x27, 0x96, 0xcb, 0x7c, 0x08, 0x00, 0x45, 0x00, 
                            0x00, 0xd0, 0x0b, 0xfd, 0x40, 0x00, 0x80, 0x06, 
                            0x00, 0x00, 0x0a, 0x00, 0x02, 0x0f, 0xc0, 0xa8, 
                            0x03, 0x2b, 0xc8, 0x55, 0x00, 0x87, 0xb1, 0xe9, 
                            0x3e, 0x7b, 0x24, 0xf2, 0x02, 0x6e, 0x50, 0x18, 
                            0xfa, 0x84, 0xd0, 0xa4, 0x00, 0x00, 0x05, 0x00, 
                            0x00, 0x03, 0x10, 0x00, 0x00, 0x00, 0xa8, 0x00, 
                            0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x90, 0x00, 
                            0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, 0xc0, 
                            0xd8, 0xcc, 0xe5, 0xd0, 0x40, 0x4a, 0x92, 0xb4, 
                            0xd0, 0x74, 0xfa, 0xa6, 0xba, 0x28, 0x02, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4b, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4b, 0x00, 
                            0x00, 0x00, 0x05, 0x00, 0x13, 0x00, 0x0d, 0x74, 
                            0xc0, 0xd8, 0xcc, 0xe5, 0xd0, 0x40, 0x4a, 0x92, 
                            0xb4, 0xd0, 0x74, 0xfa, 0xa6, 0xba, 0x28, 0x01, 
                            0x00, 0x02, 0x00, 0x01, 0x00, 0x13, 0x00, 0x0d, 
                            0x04, 0x5d, 0x88, 0x8a, 0xeb, 0x1c, 0xc9, 0x11, 
                            0x9f, 0xe8, 0x08, 0x00, 0x2b, 0x10, 0x48, 0x60, 
                            0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 
                            0x0b, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x07, 
                            0x02, 0x00, 0x00, 0x87, 0x01, 0x00, 0x09, 0x04, 
                            0x00, 0xc0, 0xa8, 0x03, 0x2b, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x04, 0x00, 0x00, 0x00};
#define MSRPC_EPM_REQ2_SZ 222
#define MSRPC_EPM_REQ2_PAYLOAD_OFFSET 54

uint8_t MSRPC_EPM_RSP2[] = {0x08, 0x00, 0x27, 0x96, 0xcb, 0x7c, 0x52, 0x54, 
                            0x00, 0x12, 0x35, 0x02, 0x08, 0x00, 0x45, 0x00, 
                            0x00, 0xd4, 0xa4, 0xbf, 0x00, 0x00, 0x40, 0x06, 
                            0x05, 0x83, 0xc0, 0xa8, 0x03, 0x2b, 0x0a, 0x00, 
                            0x02, 0x0f, 0x00, 0x87, 0xc8, 0x55, 0x24, 0xf2, 
                            0x02, 0x6e, 0xb1, 0xe9, 0x3f, 0x23, 0x50, 0x18, 
                            0xff, 0xff, 0x31, 0x68, 0x00, 0x00, 0x05, 0x00, 
                            0x02, 0x03, 0x10, 0x00, 0x00, 0x00, 0xac, 0x00, 
                            0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x94, 0x00, 
                            0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4b, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4b, 0x00, 
                            0x00, 0x00, 0x05, 0x00, 0x13, 0x00, 0x0d, 0x74, 
                            0xc0, 0xd8, 0xcc, 0xe5, 0xd0, 0x40, 0x4a, 0x92, 
                            0xb4, 0xd0, 0x74, 0xfa, 0xa6, 0xba, 0x28, 0x01, 
                            0x00, 0x02, 0x00, 0x01, 0x00, 0x13, 0x00, 0x0d, 
                            0x04, 0x5d, 0x88, 0x8a, 0xeb, 0x1c, 0xc9, 0x11, 
                            0x9f, 0xe8, 0x08, 0x00, 0x2b, 0x10, 0x48, 0x60, 
                            0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 
                            0x0b, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x07, 
                            0x02, 0x00, 0xc0, 0x96, 0x01, 0x00, 0x09, 0x04, 
                            0x00, 0xc0, 0xa8, 0x03, 0x2b, 0x00, 0x00, 0x00,
                            0x00, 0x00};
#define MSRPC_EPM_RSP2_SZ 226
#define MSRPC_EPM_RSP2_PAYLOAD_OFFSET 54

class msrpc_test : public hal_base_test {
 protected:
  msrpc_test() {
  }

  virtual ~msrpc_test() {}

  // will be called immediately after the constructor before each test
  virtual void SetUp() {}

    // will be called immediately after each test before the destructor
  virtual void TearDown() {}

  static void SetUpTestCase() {
        hal_base_test::SetUpTestCase();
  }
};

TEST_F(msrpc_test, msrpc_parse_bind_req_rsp1) {
    hal_ret_t        ret;
    ctx_t            ctx1, ctx2, ctx3, ctx4;
    fte::flow_t      iflow[2], rflow[2];
    uint16_t         num_features = 0;
    feature_state_t  feature_state;
    l4_alg_status_t  alg_state;
    rpc_info_t       rpc_info;
    cpu_rxhdr_t      rxhdr;
    hal::flow_key_t  key;
    app_session_t    app_sess;

    key.proto = (types::IPProtocol)IP_PROTO_TCP;
    memset(&alg_state, 0, sizeof(l4_alg_status_t));
    memset(&rpc_info, 0, sizeof(rpc_info_t));
    memset(&rxhdr, 0, sizeof(cpu_rxhdr_t));
    memset(&app_sess, 0, sizeof(app_session_t));
    HAL_SPINLOCK_INIT(&app_sess.slock, PTHREAD_PROCESS_PRIVATE);
    alg_state.info = &rpc_info;
    alg_state.app_session = &app_sess;
    rpc_info.pkt_type = PDU_NONE;
    rxhdr.payload_offset = MSRPC_BIND_REQ1_PAYLOAD_OFFSET;
    ctx1.init(&rxhdr, MSRPC_BIND_REQ1, MSRPC_BIND_REQ1_SZ, iflow, rflow, &feature_state, num_features);
    ctx1.set_key(key);

    ret = parse_msrpc_cn_control_flow(ctx1, &alg_state);
    ASSERT_EQ(ret, HAL_RET_OK);
    ASSERT_EQ(rpc_info.num_msrpc_ctxt, 1);
    ASSERT_EQ(rpc_info.pkt_type, PDU_BIND);

    memset(&rxhdr, 0, sizeof(cpu_rxhdr_t));
    rxhdr.payload_offset = MSRPC_BIND_RSP1_PAYLOAD_OFFSET;
    ctx2.init(&rxhdr, MSRPC_BIND_RSP1, MSRPC_BIND_RSP1_SZ, iflow, rflow, &feature_state, num_features);
    ctx2.set_key(key);
    
    ret = parse_msrpc_cn_control_flow(ctx2, &alg_state);
    ASSERT_EQ(ret, HAL_RET_OK);
    ASSERT_EQ(rpc_info.pkt_type, PDU_BIND_ACK);

    memset(&rxhdr, 0, sizeof(cpu_rxhdr_t));
    rxhdr.payload_offset = MSRPC_EPM_REQ1_PAYLOAD_OFFSET;
    ctx3.init(&rxhdr, MSRPC_EPM_REQ1, MSRPC_EPM_REQ1_SZ, iflow, rflow, &feature_state, num_features);
    ctx3.set_key(key);

    ret = parse_msrpc_cn_control_flow(ctx3, &alg_state);
    ASSERT_EQ(ret, HAL_RET_OK);
    ASSERT_EQ(rpc_info.pkt_type, PDU_REQ);

    memset(&rxhdr, 0, sizeof(cpu_rxhdr_t));
    rxhdr.payload_offset = MSRPC_EPM_RSP1_PAYLOAD_OFFSET;
    ctx4.init(&rxhdr, MSRPC_EPM_RSP1, MSRPC_EPM_RSP1_SZ, iflow, rflow, &feature_state, num_features);
    ctx4.set_key(key);

    ret = parse_msrpc_cn_control_flow(ctx4, &alg_state);
    ASSERT_EQ(ret, HAL_RET_OK);
    ASSERT_EQ(rpc_info.pkt_type, PDU_NONE); 
}

TEST_F(msrpc_test, msrpc_parse_bind_req_rsp2) {
    hal_ret_t        ret;
    ctx_t            ctx1, ctx2, ctx3, ctx4;
    fte::flow_t      iflow[2], rflow[2];
    uint16_t         num_features = 0;
    feature_state_t  feature_state;
    l4_alg_status_t  alg_state;
    rpc_info_t       rpc_info;
    cpu_rxhdr_t      rxhdr;
    hal::flow_key_t  key;
    app_session_t    app_sess;

    key.proto = (types::IPProtocol)IP_PROTO_TCP;
    memset(&alg_state, 0, sizeof(l4_alg_status_t));
    memset(&rpc_info, 0, sizeof(rpc_info_t));
    memset(&rxhdr, 0, sizeof(cpu_rxhdr_t));
    memset(&app_sess, 0, sizeof(app_session_t));
    HAL_SPINLOCK_INIT(&app_sess.slock, PTHREAD_PROCESS_PRIVATE);
    alg_state.info = &rpc_info;
    alg_state.app_session = &app_sess;
    alg_state.isCtrl = TRUE;
    rpc_info.pkt_type = PDU_NONE;
    rxhdr.payload_offset = MSRPC_BIND_REQ2_PAYLOAD_OFFSET;
    ctx1.init(&rxhdr, MSRPC_BIND_REQ2, MSRPC_BIND_REQ2_SZ, iflow, rflow, &feature_state, num_features);
    ctx1.set_key(key);

    ret = parse_msrpc_cn_control_flow(ctx1, &alg_state);
    ASSERT_EQ(ret, HAL_RET_OK);
    ASSERT_EQ(rpc_info.num_msrpc_ctxt, 3);
    ASSERT_EQ(rpc_info.pkt_type, PDU_BIND);

    memset(&rxhdr, 0, sizeof(cpu_rxhdr_t));
    rxhdr.payload_offset = MSRPC_BIND_RSP2_PAYLOAD_OFFSET;
    ctx2.init(&rxhdr, MSRPC_BIND_RSP2, MSRPC_BIND_RSP2_SZ, iflow, rflow, &feature_state, num_features);
    ctx2.set_key(key);

    ret = parse_msrpc_cn_control_flow(ctx2, &alg_state);
    ASSERT_EQ(ret, HAL_RET_OK);
    ASSERT_EQ(rpc_info.pkt_type, PDU_BIND_ACK);
 
    memset(&rxhdr, 0, sizeof(cpu_rxhdr_t));
    rxhdr.payload_offset = MSRPC_EPM_REQ1_PAYLOAD_OFFSET;
    ctx3.init(&rxhdr, MSRPC_EPM_REQ1, MSRPC_EPM_REQ1_SZ, iflow, rflow, &feature_state, num_features);
    ctx3.set_key(key);

    ret = parse_msrpc_cn_control_flow(ctx3, &alg_state);
    ASSERT_EQ(ret, HAL_RET_OK);
    ASSERT_EQ(rpc_info.pkt_type, PDU_REQ);

    memset(&rxhdr, 0, sizeof(cpu_rxhdr_t));
    rxhdr.payload_offset = MSRPC_EPM_RSP1_PAYLOAD_OFFSET;
    ctx4.init(&rxhdr, MSRPC_EPM_RSP1, MSRPC_EPM_RSP1_SZ, iflow, rflow, &feature_state, num_features);
    ctx4.set_key(key);

    ret = parse_msrpc_cn_control_flow(ctx4, &alg_state);
    ASSERT_EQ(ret, HAL_RET_OK);
    ASSERT_EQ(rpc_info.pkt_type, PDU_NONE);
}

TEST_F(msrpc_test, msrpc_exp_flow_timeout) {
    app_session_t   *app_sess = NULL;
    l4_alg_status_t *exp_flow = NULL;
    hal::flow_key_t  app_sess_key, exp_flow_key;
    
    app_sess_key.proto = (types::IPProtocol)IP_PROTO_TCP;
    app_sess_key.sip.v4_addr = 0x14000001;
    app_sess_key.dip.v4_addr = 0x14000002;
    app_sess_key.sport = 12345;
    app_sess_key.dport = 111;    

    g_rpc_state->alloc_and_init_app_sess(app_sess_key, &app_sess);

    exp_flow_key = app_sess_key;
    exp_flow_key.dport = 22345;
    g_rpc_state->alloc_and_insert_exp_flow(app_sess, exp_flow_key, 
                                         &exp_flow, true, 8);
    sleep(10);
    ASSERT_EQ(dllist_count(&app_sess->exp_flow_lhead), 0);

    exp_flow_key.dport = 22346;
    g_rpc_state->alloc_and_insert_exp_flow(app_sess, exp_flow_key, 
                                         &exp_flow, true, 5);
    exp_flow->entry.ref_count.count++;
    sleep(5);
    ASSERT_EQ(exp_flow->entry.deleting, true);
    exp_flow->entry.ref_count.count--;
    sleep(15);
    ASSERT_EQ(dllist_count(&app_sess->exp_flow_lhead), 0);
}

TEST_F(msrpc_test, app_sess_force_delete) {
    pipeline_action_t      fte_ret;
    feature_state_t        feature_state[1];
    app_session_t         *app_sess = NULL;
    hal::flow_key_t        app_sess_key;
    ctx_t                  ctx;
    fte::flow_t            iflow[2], rflow[2];
    uint16_t               num_features = 1;
    l4_alg_status_t       *l4_sess = NULL;
    cpu_rxhdr_t            rxhdr;
    hal_ret_t              ret;
     
    app_sess_key.proto = (types::IPProtocol)IP_PROTO_TCP;
    app_sess_key.sip.v4_addr = 0x14000001;
    app_sess_key.dip.v4_addr = 0x14000002;
    app_sess_key.sport = 12345;
    app_sess_key.dport = 111;   

    g_rpc_state->alloc_and_init_app_sess(app_sess_key, &app_sess);

    // Test session force delete
    ret = g_rpc_state->alloc_and_insert_l4_sess(app_sess, &l4_sess);
    ASSERT_EQ(ret, HAL_RET_OK);
    ASSERT_EQ(l4_sess->app_session, app_sess);
  
    // Init ctxt 
    memset(&rxhdr, 0, sizeof(cpu_rxhdr_t));
    HAL_SPINLOCK_INIT(&app_sess->slock, PTHREAD_PROCESS_PRIVATE);
    l4_sess->isCtrl = TRUE;
    ctx.init(&rxhdr, MSRPC_BIND_REQ2, MSRPC_BIND_REQ2_SZ, iflow, rflow, 
                                    feature_state, num_features);
  
    // Set the feature state
    feature_state[0].session_state = &l4_sess->fte_feature_state;

    // Invoke delete callback
    ctx.set_force_delete(TRUE);
    fte_ret = alg_rpc_session_delete_cb(ctx);
    ASSERT_EQ(fte_ret, PIPELINE_CONTINUE);
    ASSERT_EQ(g_rpc_state->lookup_app_sess(app_sess_key, app_sess), 
                                               HAL_RET_ENTRY_NOT_FOUND); 
}
