//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include "nic/apollo/api/include/pds.hpp"
#include "nic/operd/alerts/alerts.hpp"
#include "alerts.h"

static operd::alerts::alert_recorder_ptr g_alert_recorder = NULL;

void
operd_alert_vnic_session_limit_exhaustion (const uint8_t *vnic_key,
                                           uint32_t current_sessions,
                                           uint32_t max_sessions)
{
    pds_obj_key_t key;
    if (unlikely(!g_alert_recorder)) {
        g_alert_recorder = operd::alerts::alert_recorder::get();
        if (!g_alert_recorder) {
            return;
        }
    }

    memcpy(key.id, vnic_key, PDS_MAX_KEY_LEN);
    g_alert_recorder->alert(operd::alerts::VNIC_SESSION_LIMIT_EXCEEDED,
                            "vnic %s session limit %u",
                            key.str(), max_sessions);
}

void
operd_alert_vnic_session_limit_approach (const uint8_t *vnic_key,
                                         uint32_t current_sessions,
                                         uint32_t max_sessions)
{
    pds_obj_key_t key;
    if (unlikely(!g_alert_recorder)) {
        g_alert_recorder = operd::alerts::alert_recorder::get();
        if (!g_alert_recorder) {
            return;
        }
    }

    memcpy(key.id, vnic_key, PDS_MAX_KEY_LEN);
    g_alert_recorder->alert(operd::alerts::VNIC_SESSION_THRESHOLD_EXCEEDED,
                            "vnic %s sessions %u limit %u",
                            key.str(), current_sessions, max_sessions);
}

void
operd_alert_vnic_session_within_limit (const uint8_t *vnic_key,
                                       uint32_t current_sessions,
                                       uint32_t max_sessions)
{
    pds_obj_key_t key;
    if (unlikely(!g_alert_recorder)) {
        g_alert_recorder = operd::alerts::alert_recorder::get();
        if (!g_alert_recorder) {
            return;
        }
    }

    memcpy(key.id, vnic_key, PDS_MAX_KEY_LEN);
    g_alert_recorder->alert(operd::alerts::VNIC_SESSION_WITHIN_THRESHOLD,
                            "vnic %s sessions %u limit %u",
                            key.str(), current_sessions, max_sessions);
}
