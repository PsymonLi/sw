//---------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//---------------------------------------------------------------
#include "nic/metaswitch/stubs/mgmt/pds_ms_stubs_utils.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_utils.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_state.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_ctm.hpp"
extern "C" {
#include <amxpenapi.h>
}

namespace pds_ms {
extern NBB_VOID
pds_ms_li_stub_create (pds_ms_config_t& conf);

void pds_ms_stubs_create ()
{
    if (pds_ms::mgmt_state_t::thread_context().state()->is_upg_ht_mode()) {
        PDS_TRACE_INFO ("hitless upgrade skip MS components init");
        // Stubs initialization transaction is successful
        // We can now signal that the nbase thread is ready
        PDS_TRACE_INFO("Nbase thread is ready");
        cout << "N-Base thread is ready\n";
        auto ctx = pds_ms::mgmt_state_t::thread_context();
        ctx.state()->nbase_thread()->set_ready(true);
        return;
    }
    // Local variables
    pds_ms_config_t   conf = {0};
    
    PDS_TRACE_DEBUG ("Start all Metaswitch components and Stubs");

#ifdef AMX_DEFAULT_OPEN
    PDS_TRACE_DEBUG ("Opening AMX port by default");
    auto ret = amx_pen_open_socket();
    if (ret == 1) {
        mgmt_state_t::thread_context().state()->set_amx_open(true);
    }
#endif

    /***************************************************************************/
    /* Get the lock for the SHARED LOCAL data.                                 */
    /***************************************************************************/
    NBS_ENTER_SHARED_CONTEXT(sms_our_pid);
    NBS_GET_SHARED_DATA();

    pds_ms_ctm_send_transaction_start (PDS_MS_CTM_STUB_INIT_CORRELATOR);

    NBB_TRC_FLOW ((NBB_FORMAT "ROW UPDATES to initialize Stubs and Components"));

    conf.correlator  = PDS_MS_CTM_STUB_INIT_CORRELATOR;
    conf.row_status  = AMB_ROW_ACTIVE;

    pds_ms_l2f_stub_create (&conf);   // L2F stub
    pds_ms_li_stub_create (&conf);    // LI stub
    pds_ms_lim_create (&conf);        // LI Mgr
    pds_ms_smi_stub_create (&conf);   // SMI stub
    pds_ms_sck_stub_create (&conf);   // Sck stub
    pds_ms_ftm_create (&conf);        // FT Mgr
    pds_ms_nar_stub_create (&conf);   // NAR stub
    pds_ms_nrm_create (&conf);        // NR Mgr
    pds_ms_psm_create (&conf);        // PS Mgr
    pds_ms_rtm_create (&conf, PDS_MS_RTM_DEF_ENT_INDEX,
                       true /* Default VRF */);  // RTM
    pds_ms_bgp_create (&conf);        // BGP component
    pds_ms_evpn_create (&conf);       // EVPN component

    pds_ms_ctm_send_transaction_end (PDS_MS_CTM_STUB_INIT_CORRELATOR);

    /***************************************************************************/ 
    /* Release the lock on the SHARED LOCAL data.                              */
    /***************************************************************************/
    NBS_RELEASE_SHARED_DATA();
    NBS_EXIT_SHARED_CONTEXT();

    PDS_TRACE_DEBUG ("Successfully created all Metaswitch components and Stubs");
}

void pds_ms_hals_stub_init()
{
    if (mgmt_state_t::thread_context().state()->is_upg_ht_mode()) {
        PDS_TRACE_INFO ("hitless upgrade skip MS HAL stub init");
        return;
    }
    // Local variables
    pds_ms_config_t   conf = {0};
    PDS_TRACE_DEBUG("Start HALS Stub");

    PDS_MS_START_TXN(PDS_MS_CTM_GRPC_CORRELATOR);

    conf.correlator  = PDS_MS_CTM_GRPC_CORRELATOR;
    conf.row_status  = AMB_ROW_ACTIVE;

    pds_ms_hals_stub_create (&conf);  // HALS Stub
    PDS_MS_END_TXN(PDS_MS_CTM_GRPC_CORRELATOR);

    // blocking on response from MS
    auto ret_status = pds_ms::mgmt_state_t::ms_response_wait();
    if (ret_status != types::ApiStatus::API_STATUS_OK) {
        PDS_TRACE_ERR ("Failed to create HALS stub err %d", ret_status);
    } else {
        PDS_TRACE_DEBUG("HALS Stub created successfully");
    }
}

} // End namespace
