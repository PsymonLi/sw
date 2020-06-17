//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains the object util class
///
//----------------------------------------------------------------------------

#ifndef __TEST_API_UTILS_API_BASE_HPP__
#define __TEST_API_UTILS_API_BASE_HPP__

#include <iostream>
#include <vector>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/apollo/test/api/utils/base.hpp"
#ifdef AGENT_MODE
#include "nic/apollo/agent/test/client/app.hpp"
#endif
#include "nic/apollo/test/scale/api.hpp"

namespace test {
namespace api {

/// \addtogroup  PDS_FEEDER
/// @{

/// \brief Compare the config with values read from hardware
/// Compares both key and spec
template <typename feeder_T, typename info_T>
sdk_ret_t api_info_compare(feeder_T& feeder, info_T *info,
                           info_T *bkup_info = NULL) {

    if (!feeder.key_compare(&info->spec.key)) {
        cerr << "key compare failed" << endl;
        cerr << "\t" << feeder << endl << "\t" << info;
        return sdk::SDK_RET_ERR;
    }

    if (!feeder.spec_compare(&info->spec)) {
        cerr << "spec compare failed" << endl;
        cerr << "\t" << feeder << endl << "\t" << info;
        return sdk::SDK_RET_ERR;
    }

    if (bkup_info) {
        if (!feeder.status_compare(&info->status, &bkup_info->status)) {
            std::cout << "status compare failed; " << feeder << bkup_info
                      << info << std::endl;
            return sdk::SDK_RET_ERR;
        }
    }
    cout << "info compare success" << endl;
    cout << "\t" << feeder << endl << "\t" << info;

    return SDK_RET_OK;
}

/// \brief Compare the config with values read from hardware
/// Compares only spec
template <typename feeder_T, typename info_T>
sdk_ret_t api_info_compare_singleton(
    feeder_T& feeder, info_T *info,
    info_T *bkup_info = NULL) {

    if (bkup_info) {
        memcpy(&feeder.spec, &bkup_info->spec, sizeof(feeder.spec));
    }

    if (!feeder.spec_compare(&info->spec)) {
        cerr << "spec compare failed" << endl;
        cerr << "\t" << feeder << endl << "\t" << info;
        return sdk::SDK_RET_ERR;
    }

    cout << "info compare success" << endl;
    cout << "\t" << feeder << endl << "\t" << info;

    return SDK_RET_OK;
}

/// \brief Invokes the PDS create apis for test objects
#define API_CREATE(_api_str)                                                 \
inline sdk_ret_t                                                             \
create(pds_batch_ctxt_t bctxt, _api_str##_feeder& feeder) {                  \
    pds_##_api_str##_spec_t spec;                                            \
                                                                             \
    memset(&spec, 0, sizeof(spec));                                          \
    feeder.spec_build(&spec);                                                \
    return (pds_##_api_str##_create(&spec, bctxt));                          \
}

/// \brief Dummy function for Read
#define API_NO_READ(_api_str)                                                \
inline sdk_ret_t                                                             \
read(_api_str##_feeder& feeder) {                                            \
    return SDK_RET_OK;                                                       \
}

/// \brief Invokes the PDS read apis for test objects
/// Also it compares the config values with values read from hardware
/// only if current method is read
#define API_READ(_api_str)                                                   \
inline sdk_ret_t                                                             \
read(_api_str##_feeder& feeder) {                                            \
    sdk_ret_t rv;                                                            \
    pds_obj_key_t key;                                                       \
    pds_##_api_str##_info_t *info;                                           \
                                                                             \
    memset(&key, 0, sizeof(key));                                            \
    feeder.key_build(&key);                                                  \
    info = new (pds_##_api_str##_info_t);                                    \
    memset(info, 0, sizeof(pds_##_api_str##_info_t));                        \
    feeder.spec_alloc(&info->spec);                                          \
    if ((rv = pds_##_api_str##_read(&key, info)) != SDK_RET_OK)              \
        return rv;                                                           \
                                                                             \
    if (feeder.stash()) {                                                    \
        feeder.vec.push_back(info);                                          \
        return rv;                                                           \
    }                                                                        \
    rv = api_info_compare<_api_str##_feeder, pds_##_api_str##_info_t>(       \
             feeder, info);                                                  \
    delete(info);                                                            \
    return rv;                                                               \
}


/// \brief Invokes the PDS read api for route/policy_rule test object
/// Also it compares the config values with values read from hardware
/// only if current method is read
/// We need to special handle for route/policy_rule object as
/// route/policy_rule spec has a key which is not generic
#define API_READ_1(_api_str)                                                 \
inline sdk_ret_t                                                             \
read(_api_str##_feeder& feeder) {                                            \
    sdk_ret_t rv;                                                            \
    pds_##_api_str##_key_t key;                                              \
    pds_##_api_str##_info_t *info;                                           \
    memset(&key, 0, sizeof(key));                                            \
    feeder.key_build(&key);                                                  \
    info = new (pds_##_api_str##_info_t);                                    \
    memset(info, 0, sizeof(pds_##_api_str##_info_t));                        \
    if ((rv = pds_##_api_str##_read(&key, info)) != SDK_RET_OK)              \
        return rv;                                                           \
    if (feeder.stash()) {                                                    \
        feeder.vec.push_back(info);                                          \
        return rv;                                                           \
    }                                                                        \
    rv = api_info_compare<_api_str##_feeder, pds_##_api_str##_info_t>(       \
        feeder, info);                                                       \
    delete(info);                                                            \
    return rv;                                                               \
}

/// \brief Invokes the PDS read api for route-table test object
/// Also it compares the config values with values read from hardware
/// only if current method is read
#define API_ROUTE_TABLE_READ(_api_str)                                       \
inline sdk_ret_t                                                             \
read(_api_str##_feeder& feeder) {                                            \
    sdk_ret_t rv;                                                            \
    pds_obj_key_t key;                                                       \
    pds_route_table_info_t *info;                                            \
    uint32_t num_routes;                                                     \
                                                                             \
    memset(&key, 0, sizeof(key));                                            \
    feeder.key_build(&key);                                                  \
    info = new (pds_route_table_info_t);                                     \
    memset(info, 0, sizeof(pds_route_table_info_t));                         \
    info->spec.route_info =                                                  \
    (route_info_t *)SDK_CALLOC(PDS_MEM_ALLOC_ID_ROUTE_TABLE,                 \
                               ROUTE_INFO_SIZE(0));                          \
    if ((rv = pds_route_table_read(&key, info)) != SDK_RET_OK)               \
        return rv;                                                           \
                                                                             \
    num_routes = info->spec.route_info->num_routes;                          \
    if (num_routes) {                                                        \
        SDK_FREE(PDS_MEM_ALLOC_ID_ROUTE_TABLE, info->spec.route_info);       \
        info->spec.route_info =                                              \
        (route_info_t *)SDK_CALLOC(PDS_MEM_ALLOC_ID_ROUTE_TABLE,             \
                                   ROUTE_INFO_SIZE(num_routes));             \
        info->spec.route_info->num_routes = num_routes;                      \
        rv = pds_route_table_read(&key, info);                               \
        if (rv != SDK_RET_OK) {                                              \
            SDK_FREE(PDS_MEM_ALLOC_ID_ROUTE_TABLE, info->spec.route_info);   \
            return rv;                                                       \
        }                                                                    \
    }                                                                        \
    if (feeder.stash()) {                                                    \
        feeder.vec.push_back(info);                                          \
        return rv;                                                           \
    }                                                                        \
    rv = api_info_compare<_api_str##_feeder, pds_##_api_str##_info_t>(       \
            feeder, info);                                                   \
    SDK_FREE(PDS_MEM_ALLOC_ID_ROUTE_TABLE, info->spec.route_info);           \
    delete(info);                                                            \
    return rv;                                                               \
}

/// \brief invokes the PDS read apis for test objects
/// also it compares the config values with values previously
/// stashed in peristent storage during backup
#define API_READ_CMP(_api_str)                                               \
inline sdk_ret_t                                                             \
read_cmp(_api_str##_feeder& feeder) {                                        \
    sdk_ret_t rv;                                                            \
    pds_obj_key_t key;                                                       \
    pds_##_api_str##_info_t info;                                            \
    pds_##_api_str##_info_t *bkup_info;                                      \
                                                                             \
    memset(&key, 0, sizeof(key));                                            \
    feeder.key_build(&key);                                                  \
    memset(&info, 0, sizeof(pds_##_api_str##_info_t));                       \
    if ((rv = pds_##_api_str##_read(&key, &info)) != SDK_RET_OK)             \
        return rv;                                                           \
                                                                             \
    bkup_info  = feeder.vec.front();                                         \
    rv = (api_info_compare<_api_str##_feeder, pds_##_api_str##_info_t>(      \
              feeder, &info, bkup_info));                                    \
    delete(bkup_info );                                                      \
    feeder.vec.erase(feeder.vec.begin());                                    \
    return rv;                                                               \
}

/// \brief invokes the PDS read apis for test objects
/// also it compares the config values with values previously
/// stashed in peristent storage during backup
#define API_READ_CMP_SINGLETON(_api_str)                                     \
inline sdk_ret_t                                                             \
read_cmp(_api_str##_feeder& feeder) {                                        \
    sdk_ret_t rv;                                                            \
    pds_##_api_str##_info_t info;                                            \
    pds_##_api_str##_info_t *bkup_info;                                      \
                                                                             \
    memset(&info, 0, sizeof(pds_##_api_str##_info_t));                       \
    if ((rv = pds_##_api_str##_read(&info)) != SDK_RET_OK)                   \
        return rv;                                                           \
                                                                             \
    bkup_info  = feeder.vec.front();                                         \
    rv = (api_info_compare_singleton<_api_str##_feeder,                      \
             pds_##_api_str##_info_t>(feeder, &info, bkup_info));            \
    delete(bkup_info );                                                      \
    feeder.vec.erase(feeder.vec.begin());                                    \
    return rv;                                                               \
}

/// \brief Invokes the PDS read apis for test objects
/// Also it compares the config values with values read from hardware
#define API_READ_SINGLETON(_api_str)                                         \
inline sdk_ret_t                                                             \
read(_api_str##_feeder& feeder) {                                            \
    sdk_ret_t rv;                                                            \
    pds_##_api_str##_info_t *info;                                           \
                                                                             \
    info = new (pds_##_api_str##_info_t);                                    \
    memset(info, 0, sizeof(pds_##_api_str##_info_t));                        \
    if ((rv = pds_##_api_str##_read(info)) != SDK_RET_OK)                    \
        return rv;                                                           \
                                                                             \
    if (feeder.stash()) {                                                    \
        feeder.vec.push_back(info);                                          \
        return rv;                                                           \
    }                                                                        \
    rv = api_info_compare_singleton<_api_str##_feeder,                       \
            pds_##_api_str##_info_t>(feeder, info);                          \
    delete(info);                                                            \
    return rv;                                                               \
}

/// \brief Invokes the PDS update apis for test objects
#define API_UPDATE(_api_str)                                                 \
inline sdk_ret_t                                                             \
update(pds_batch_ctxt_t bctxt, _api_str##_feeder& feeder) {                  \
    pds_##_api_str##_spec_t spec;                                            \
                                                                             \
    memset(&spec, 0, sizeof(spec));                                          \
    feeder.spec_build(&spec);                                                \
    return (pds_##_api_str##_update(&spec, bctxt));                          \
}

/// \brief Invokes the PDS delete apis for test objects
#define API_DELETE(_api_str)                                                 \
inline sdk_ret_t                                                             \
del(pds_batch_ctxt_t bctxt, _api_str##_feeder& feeder) {                     \
    pds_obj_key_t key;                                                       \
                                                                             \
    memset(&key, 0, sizeof(key));                                            \
    feeder.key_build(&key);                                                  \
    return (pds_##_api_str##_delete(&key, bctxt));                           \
}

/// \brief Invokes the PDS delete apis for route test object
/// We need to special handle for route object as route spec has a key
/// which is not generic
#define API_DELETE_1(_api_str)                                               \
inline sdk_ret_t                                                             \
del(pds_batch_ctxt_t bctxt, _api_str##_feeder& feeder) {                     \
    pds_##_api_str##_key_t key;                                              \
                                                                             \
    memset(&key, 0, sizeof(key));                                            \
    feeder.key_build(&key);                                                  \
    return (pds_##_api_str##_delete(&key, bctxt));                           \
}

// TOD: remove this macro later
#define API_READ_TMP(_api_str)                                               \
inline sdk_ret_t                                                             \
read(_api_str##_feeder& feeder) {                                            \
    sdk_ret_t rv;                                                            \
    pds_##_api_str##_key_t key;                                              \
    pds_##_api_str##_info_t info;                                            \
                                                                             \
    memset(&key, 0, sizeof(key));                                            \
    feeder.key_build(&key);                                                  \
    memset(&info, 0, sizeof(info));                                          \
    if ((rv = pds_##_api_str##_read(&key, &info)) != SDK_RET_OK)             \
        return rv;                                                           \
                                                                             \
    return (api_info_compare<_api_str##_feeder, pds_##_api_str##_info_t>(    \
                feeder, &info));                                             \
}

// TOD: remove this macro later
/// \brief Invokes the PDS delete apis for test objects
#define API_DELETE_TMP(_api_str)                                             \
inline sdk_ret_t                                                             \
del(pds_batch_ctxt_t bctxt, _api_str##_feeder& feeder) {                     \
    pds_##_api_str##_key_t key;                                              \
                                                                             \
    memset(&key, 0, sizeof(key));                                            \
    feeder.key_build(&key);                                                  \
    return (pds_##_api_str##_delete(&key, bctxt));                           \
}

/// \brief Invokes the PDS delete apis for test objects
#define API_DELETE_SINGLETON(_api_str)                                       \
inline sdk_ret_t                                                             \
del(pds_batch_ctxt_t bctxt, _api_str##_feeder& feeder) {                     \
    return (pds_##_api_str##_delete(bctxt));                                 \
}

// TODO - Defining the following APIs temporarily till all objects are changed
#define API_CREATE1(_api_str)                                                \
inline sdk_ret_t                                                             \
create(_api_str##_feeder& feeder) {                                          \
    pds_##_api_str##_spec_t spec;                                            \
                                                                             \
    if (feeder.num_obj == 0)                                                 \
        return (create_##_api_str(NULL));                                    \
    feeder.spec_build(&spec);                                                \
    return (create_##_api_str(&spec));                                       \
}

#define API_READ1(_api_str)                                                  \
inline sdk_ret_t                                                             \
read(_api_str##_feeder& feeder) {                                            \
    sdk_ret_t rv;                                                            \
    pds_obj_key_t key;                                                       \
    pds_##_api_str##_info_t info;                                            \
                                                                             \
    feeder.key_build(&key);                                                  \
    memset(&info, 0, sizeof(info));                                          \
    if ((rv = read_##_api_str(&key, &info)) != SDK_RET_OK)                   \
        return rv;                                                           \
                                                                             \
    return (api_info_compare<_api_str##_feeder, pds_##_api_str##_info_t>(    \
                feeder, &info));                                             \
}

#define API_UPDATE1(_api_str)                                                \
inline sdk_ret_t                                                             \
update(_api_str##_feeder& feeder) {                                          \
    pds_##_api_str##_spec_t spec;                                            \
                                                                             \
    if (feeder.num_obj == 0)                                                 \
        return (update_##_api_str(NULL));                                    \
    feeder.spec_build(&spec);                                                \
    return (update_##_api_str(&spec));                                       \
}

#define API_DELETE1(_api_str)                                                \
inline sdk_ret_t                                                             \
del(_api_str##_feeder& feeder) {                                             \
    pds_obj_key_t key;                                                       \
                                                                             \
    if (feeder.num_obj == 0)                                                 \
        return (delete_##_api_str(NULL));                                    \
    feeder.key_build(&key);                                                  \
    return (delete_##_api_str(&key));                                        \
}

/// \brief Invokes the create apis for all the config objects
template <typename feeder_T>
sdk_ret_t many_create(pds_batch_ctxt_t bctxt, feeder_T& feeder) {
    sdk_ret_t ret;
    feeder_T tmp = feeder;

    for (tmp.iter_init(); tmp.iter_more(); tmp.iter_next()) {
        if ((ret = create(bctxt, tmp)) != SDK_RET_OK) {
            cerr << "MANY CREATE failed" << endl;
            cerr << "\t" << tmp << endl;
            return ret;
        }
    }
    return ret;
}

/// \brief Invokes the read apis for all the config objects
template <typename feeder_T>
sdk_ret_t many_read(feeder_T& feeder, sdk_ret_t exp_result = SDK_RET_OK) {
    feeder_T tmp = feeder;

    if (feeder.read_unsupported())
        return SDK_RET_OK;

    for (tmp.iter_init(); tmp.iter_more(); tmp.iter_next()) {
        if (read(tmp) != exp_result) {
            cerr << "MANY READ failed" << endl;
            cerr << "\t" << tmp << endl;
            return SDK_RET_ERR;
        }
    }
    if (feeder.stash()) {
        feeder.vec = tmp.vec;
    }
    return SDK_RET_OK;
}

/// \brief invokes the read apis for all the config objects
///  and compares the read info values with info stashed in persistent storage
template <typename feeder_T>
sdk_ret_t many_read_cmp(feeder_T& feeder,
                        sdk::sdk_ret_t expected_result = sdk::SDK_RET_OK) {
    if (feeder.read_unsupported())
        return SDK_RET_OK;

    feeder_T tmp = feeder;
    tmp.vec = feeder.vec;
    for (tmp.iter_init(); tmp.iter_more(); tmp.iter_next()) {
        SDK_ASSERT(read_cmp(tmp) == expected_result);
    }
    return SDK_RET_OK;
}

/// \brief Invokes the update apis for all the config objects
template <typename feeder_T>
sdk_ret_t many_update(pds_batch_ctxt_t bctxt, feeder_T& feeder) {
    sdk_ret_t ret;
    feeder_T tmp = feeder;

    for (tmp.iter_init(); tmp.iter_more(); tmp.iter_next()) {
        if ((ret = update(bctxt, tmp)) != SDK_RET_OK) {
            cerr << "MANY UPDATE failed" << endl;
            cerr << "\t" << tmp << endl;
            return ret;
        }
    }
    return ret;
}

/// \brief Invokes the delete apis for all the config objects
template <typename feeder_T>
sdk_ret_t many_delete(pds_batch_ctxt_t bctxt, feeder_T& feeder) {
    sdk_ret_t ret;
    feeder_T tmp = feeder;

    for (tmp.iter_init(); tmp.iter_more(); tmp.iter_next()) {
        if ((ret = del(bctxt, tmp)) != SDK_RET_OK) {
            cerr << "MANY DELETE failed" << endl;
            cerr << tmp << endl;
            return ret;
        }
    }
    return ret;
}

/// @}

}    // namespace api
}    // namespace test

#endif    // __TEST_API_UTILS_API_BASE_HPP__
