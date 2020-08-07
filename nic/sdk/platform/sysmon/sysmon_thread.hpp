//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// system monitor thread
///
//----------------------------------------------------------------------------

void sysmon_event_thread_init(void *ctxt);
void sysmon_event_thread_exit(void *ctxt);
void sysmon_event_handler(void *msg, void *ctxt);

extern sysmon_cfg_t g_sysmon_cfg;
