//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines pdsdbutil.bin
///
//----------------------------------------------------------------------------

#include <getopt.h>
#include <stdio.h>
#include "pdsdbutil.hpp"
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/string_generator.hpp>

using namespace std;

bool     g_routetable_query = false;
bool     g_policy_query     = false;
bool     g_mapping_query     = false;
bool     g_dump_all         = false;
kvstore  *g_kvstore;

static inline bool
is_valid_uuid (string const& maybe_uuid, uuid& result) {
    try {
        result = string_generator()(maybe_uuid);
        return true;
    } catch (...) {
        return false;
    }
}

static inline void
print_usage (void) {
    printf("Error: Required flag(s) have/has not been set\n");
    printf("Usage:\n");
    printf("    pdsdbutil <MandatoryFlag> [OptionalFlag]\n\n");
    printf("MandatoryFlags:\n");
    printf("%-25s %s\n", "--route-table, -r", "Display route-table object information");
    printf("%-25s %s\n", "--security-policy, -p", "Display security-policy object information");
    printf("%-25s %s\n", "--mapping, -m", "Display mapping object information");
    printf("\nOptionalFlags:\n");
    printf("%-25s %s\n", "--id, -i", "Specify the id of the object to display");
    printf("%-25s %s\n", "--dump-all, -a", "Dump all db contents (Ex: pdsdbutil --dump-all)");
}

int
main (int argc, char **argv) {
    int            oc;
    string         path("");
    sdk_ret_t      ret = SDK_RET_OK;
    uuid           uuid = nil_uuid();

    static struct option longopts[] = {
        {"route-table",     no_argument,       0,  'r' },
        {"security-policy", no_argument,       0,  'p' },
        {"mapping",         no_argument,       0,  'm' },
        {"id",              required_argument, 0,  'i' },
        {"dump-all",        no_argument      , 0,  'a' },
        {"help",            no_argument,       0,  'h' },
        {0,                 0,                 0,   0 }
    };

    // parse CLI options
    while ((oc = getopt_long(argc, argv, ":rphami:",
                             longopts, NULL)) != -1) {
        switch (oc) {
        case 'r':
            g_routetable_query = true;
            break;

        case 'p':
            g_policy_query = true;
            break;

        case 'm':
            g_mapping_query = true;
            break;

        case 'a':
            g_dump_all = true;
            break;

        case 'i':
            if (!is_valid_uuid(optarg, uuid)) {
                printf("Invalid uuid format, please try again\n");
                return SDK_RET_INVALID_ARG;
            }
            break;
        
        case '?':
        default:
            print_usage();
            // ignore all other options
            break;
        }
    }

    if (argc == 1 || (!g_routetable_query && !g_policy_query && !g_dump_all &&
        !g_mapping_query)) {
        print_usage();
        return ret;
    }

    // pdsdbutil is expected to be used on the hardware, not sim
    path += "/data/pdsagent.db";
    g_kvstore = kvstore::factory(path, 0, kvstore::KVSTORE_MODE_READ_ONLY);
    if (g_kvstore == NULL) {
        printf("Operation failed, could not open kvstore\n");
        return SDK_RET_ERR;
    }

    g_kvstore->txn_start(kvstore::TXN_TYPE_READ_ONLY);
    if (g_routetable_query) {
        ret = pds_get_route_table(&uuid);
    } else if (g_policy_query) {
        ret = pds_get_policy(&uuid);
    } else if (g_mapping_query) {
        ret = pds_get_mapping(&uuid);
    } else if (g_dump_all) {
        uuid = nil_uuid();
        printf("pdsdbutil --route-table\n");
        ret = pds_get_route_table(&uuid);
        if (ret != SDK_RET_OK) {
            printf("Error dumping \"route-table\" from db, err %d\n", ret);
        }
        printf("pdsdbutil --security-policy\n");
        ret = pds_get_policy(&uuid);
        if (ret != SDK_RET_OK) {
            printf("Error dumping \"security-policy\" from db, err %d\n", ret);
        }
        printf("pdsdbutil --mapping\n");
        ret = pds_get_mapping(&uuid);
        if (ret != SDK_RET_OK) {
            printf("Error dumping \"mapping\" from db, err %d\n", ret);
        }
    }
    kvstore::destroy(g_kvstore);
    return ret;
}

