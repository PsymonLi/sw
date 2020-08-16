//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines API for operdctl CLI
///
//----------------------------------------------------------------------------

#ifndef __OPERDCTL_HPP__
#define __OPERDCTL_HPP__

typedef int (*cmd_handler_fn)(int argc, const char *argv[]);

extern int dump(int argc, const char *argv[]);
extern int level(int argc, const char *argv[]);
extern int syslog(int argc, const char *argv[]);

#endif    // __OPERDCTL_HPP__
