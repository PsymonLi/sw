// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __RUNENV_H__
#define __RUNENV_H__

#include "include/sdk/platform.hpp"
#include "lib/catalog/catalog.hpp"
#include "platform/pal/include/pal.h"

using namespace std;

namespace sdk {
namespace lib {

typedef enum
{
    FEATURE_DISABLED,
    FEATURE_ENABLED,
    FEATURE_QUERY_FAILED,
    FEATURE_QUERY_UNSUPPORTED,
} feature_query_ret_e;

typedef enum
{
    NCSI_FEATURE,
    //more feature query can be added here
} feature_e;

class runenv {
public:
    static feature_query_ret_e is_feature_enabled(feature_e feature);
private:
    static catalog *catalog_db;
};

} // namespace lib
} // namespace sdk


#endif    //__RUNENV_H__
