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

typedef struct alert_ {
    const char *name;
    const char *category;
    const char *severity;
    const char *description;
    const char *message;
} alert_t;

#endif    // __OPERD_ALERT_TYPE_HPP__
