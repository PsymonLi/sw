//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains event structure definitions
///
//----------------------------------------------------------------------------

#ifndef __OPERD_ALERT_TYPE_HPP__
#define __OPERD_ALERT_TYPE_HPP__

#include <stddef.h>
#include <stdint.h>
#include "nic/apollo/api/include/pds.hpp"

typedef struct alert_ {
    const char *name;
    const char *category;
    const char *severity;
    const char *description;
    const char *message;
} alert_t;

// generic datatype used by producers and operd decoder for event generation.
// message can be plain text or one of the datatypes defined below
// based on event id
typedef struct {
    uint16_t event_id;
    char     message[0];
} __PACK__ operd_event_data_t;

typedef struct {
     pds_obj_key_t host_if;     ///< host interface on which packet was received
     uint16_t      pkt_len;     ///< packet length
     char          pkt_data[0]; ///< packet contents
 } __PACK__ learn_operd_msg_t;

#endif    // __OPERD_ALERT_TYPE_HPP__
