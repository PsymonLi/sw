//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include <session_internal.pb.h>
#include <ftl_wrapper.h>
#include "session.h"

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
