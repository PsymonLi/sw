//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//------------------------------------------------------------------------------

#ifndef __PEN_OPER_CORE_STATE_HPP__
#define __PEN_OPER_CORE_STATE_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/eventmgr/eventmgr.hpp"

using sdk::lib::eventmgr;

typedef enum pen_oper_info_type_s {
    PENOPER_INFO_TYPE_NONE,
    PENOPER_INFO_TYPE_MIN,
    PENOPER_INFO_TYPE_EVENT = PENOPER_INFO_TYPE_MIN,
    PENOPER_INFO_TYPE_MAX,
} pen_oper_info_type_t;

namespace core {

class pen_oper_state {
public:
    static sdk_ret_t init(void);
    static class pen_oper_state *state(void);

    /// \brief          get eventmgr instance corresponding to info_type
    /// \param[in]      info_type    oper info type
    /// \return         pointer to eventmgr instance corresponding to info_type
    eventmgr *client_mgr(pen_oper_info_type_t info_type);

    /// \brief          check if client is interested in atleast one oper info
    /// \param[in]      ctxt    client info (grpc stream)
    /// \return         true    if client is still subscribed to any oper info
    ///                 false   otherwise
    bool is_client_active(void *ctxt);

    /// \brief          add the client as listener for info_id in
    ///                 eventmgr instance of info_type
    /// \param[in]      info_type    oper info type client is interested in
    /// \param[in]      info_id      oper info identifier specific to info_type
    /// \param[in]      ctxt         client info
    /// \return         SDK_RET_OK   on success, failure status code on error
    sdk_ret_t add_client(pen_oper_info_type_t info_type, uint32_t info_id,
                         void *ctxt);

    /// \brief          remove the client as listener for info_id in
    ///                 eventmgr instance of info_type
    /// \param[in]      info_type    oper info type client was interested in
    /// \param[in]      info_id      oper info identifier specific to info_type
    /// \param[in]      ctxt         client info
    /// \return         SDK_RET_OK   on success, failure status code on error
    sdk_ret_t delete_client(pen_oper_info_type_t info_type, uint32_t info_id,
                            void *ctxt);

    /// \brief          remove the client as listener from
    ///                 eventmgr instance of info_type
    /// \param[in]      info_type    oper info type client was interested in
    /// \param[in]      ctxt         client info
    /// \return         SDK_RET_OK   on success, failure status code on error
    sdk_ret_t delete_client(pen_oper_info_type_t info_type, void *ctxt);

    /// \brief          remove the client as listener for all info_type
    /// \param[in]      ctxt         client info
    /// \return         SDK_RET_OK   on success, failure status code on error
    void delete_client(void *ctxt);

private:
    /// \brief constructor
    pen_oper_state();

    /// \brief destructor
    ~pen_oper_state();

private:
    eventmgr *client_mgr_[PENOPER_INFO_TYPE_MAX];  ///< client manager
};

}    // namespace core

#endif    // __PEN_OPER_CORE_STATE_HPP__
