//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include <sess_sync.pb.h>
#include <ftl_wrapper.h>
#include "sess_restore.h"

bool
pds_decode_one_v4_session (const uint8_t *data, const uint8_t len,
                           sess_info_t *sess, const uint16_t thread_index)
{
    // Not yet implemented
    return false;
}

void
pds_encode_one_v4_session (uint8_t *data, uint8_t *len, sess_info_t *sess,
                           const uint16_t thread_index)
{
}
