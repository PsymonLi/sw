#ifndef __AXITRACE_GENERIC_HH__
#define __AXITRACE_GENERIC_HH__

#include "option.h"

#define DEBUG_MODE              axitrace::option::instance().opt["debug_mode"][0].get<bool>()
#define PIO_PAL_MODE            1
#define PIO_REGMODEL_MODE       0

#define DEBUG_WRITE_FILTER_CFG  0
#define DEBUG_CREATE_BUFFER     0


#define TOKEN_APP               "[axitrace]"
#define TOKEN_ERROR             "\033[1;31m[ERROR]\033[0m"
#define TOKEN_INFO              "[INFO]"
#define TOKEN_WARNING           "\033[0;34m[WARNING]\033[0m"
#define TOKEN_DEBUG             "\033[0;32m[DEBUG]\033[0m"

#endif //__AXITRACE_GENERIC_HH__
