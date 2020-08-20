//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include "nic/apollo/api/include/pds.hpp"
#include "nic/infra/operd/event/event.hpp"
#include "event.h"

static operd::event::event_recorder_ptr g_event_recorder = NULL;

void
operd_event_vnic_session_limit_exhaustion (const uint8_t *vnic_key,
                                           uint32_t current_sessions,
                                           uint32_t max_sessions)
{
    pds_obj_key_t key;
    if (unlikely(!g_event_recorder)) {
        g_event_recorder = operd::event::event_recorder::get();
        if (!g_event_recorder) {
            return;
        }
    }

    memcpy(key.id, vnic_key, PDS_MAX_KEY_LEN);
    g_event_recorder->event(operd::event::VNIC_SESSION_LIMIT_EXCEEDED,
                            "vnic %s session limit %u",
                            key.str(), max_sessions);
}

void
operd_event_vnic_session_limit_approach (const uint8_t *vnic_key,
                                         uint32_t current_sessions,
                                         uint32_t max_sessions)
{
    pds_obj_key_t key;
    if (unlikely(!g_event_recorder)) {
        g_event_recorder = operd::event::event_recorder::get();
        if (!g_event_recorder) {
            return;
        }
    }

    memcpy(key.id, vnic_key, PDS_MAX_KEY_LEN);
    g_event_recorder->event(operd::event::VNIC_SESSION_THRESHOLD_EXCEEDED,
                            "vnic %s sessions %u limit %u",
                            key.str(), current_sessions, max_sessions);
}

void
operd_event_vnic_session_within_limit (const uint8_t *vnic_key,
                                       uint32_t current_sessions,
                                       uint32_t max_sessions)
{
    pds_obj_key_t key;
    if (unlikely(!g_event_recorder)) {
        g_event_recorder = operd::event::event_recorder::get();
        if (!g_event_recorder) {
            return;
        }
    }

    memcpy(key.id, vnic_key, PDS_MAX_KEY_LEN);
    g_event_recorder->event(operd::event::VNIC_SESSION_WITHIN_THRESHOLD,
                            "vnic %s sessions %u limit %u",
                            key.str(), current_sessions, max_sessions);
}
