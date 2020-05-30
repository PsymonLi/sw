// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#include "runenv.h"

namespace sdk {
namespace lib {

catalog *runenv::catalog_db = NULL;

feature_query_ret_e
runenv::is_feature_enabled(feature_e feature)
{
    int32_t cpld_cntl_reg;

    if (!catalog_db) {
        catalog_db = catalog::factory();

        if (!catalog_db)
            return FEATURE_QUERY_FAILED;
    }

    switch (feature) {
        case NCSI_FEATURE:
            if (catalog_db->is_card_naples25_swm()) {

                if (catalog_db->card_id() == CARD_ID_NAPLES25_OCP)
                    return FEATURE_ENABLED;

                else if (catalog_db->card_id() == CARD_ID_NAPLES25_SWM) {
                    cpld_cntl_reg = cpld_reg_rd(CPLD_REGISTER_CTRL);
                    if (cpld_cntl_reg == -1)
                        return FEATURE_QUERY_FAILED;

                    if (cpld_cntl_reg & CPLD_ALOM_PRESENT_BIT)
                        return FEATURE_ENABLED;
                }

                // this should never happen but its here to handle the case
                return FEATURE_DISABLED;
            }
            else //NCSI feature is available only on SWM cards
                return FEATURE_DISABLED;

            break;
        default:
            return FEATURE_QUERY_UNSUPPORTED;
    }
}

}    // namespace lib
}    // namespace sdk
