//---------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// PDS-MS stub indirect pathset store
//---------------------------------------------------------------

#ifndef __PDS_MS_INDIRECT_PS_STORE_HPP__
#define __PDS_MS_INDIRECT_PS_STORE_HPP__

#include "nic/metaswitch/stubs/common/pds_ms_defs.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_object_store.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_slab_object.hpp"
#include "nic/sdk/lib/slab/slab.hpp"
#include <unordered_set>

namespace pds_ms {

// Indirect Pathset (Cascaded mode) that points to direct pathsets
// In cascaded mode, MS creates unique Indirect pathset for each
// destination IP that its tracking
// Multiple objects that have the same Dest IP can share the same
// indirect pathset.
// This object holds the back-ref to the list of objects that are
// using this indirect pathset
class indirect_ps_obj_t : public slab_obj_t<indirect_ps_obj_t>,
                             public base_obj_t {
public:
    indirect_ps_obj_t();

    ms_hw_tbl_id_t direct_ps_dpcorr(void) {return direct_ps_dpcorr_;}
    ip_addr_t& destip(void) {return destip_;}
    void walk_ip_track_obj_keys(std::function<bool(const pds_obj_key_t& k)> fn) {
        for (auto& k: ip_track_obj_keys_) {
            if (!fn(k)) {
                return;
            }
        }
    }
    void set_direct_ps_dpcorr(ms_hw_tbl_id_t direct_ps_dpcorr) {
        direct_ps_dpcorr_ = direct_ps_dpcorr;
    }

    // MS EVPN Tunnel uses this Indirect pathset
    // Each MS EVPN TEP has separate Dest IP so each
    // indirect pathset will be used by only 1 MS EVPN Tunnel
    // VXLAN Ports for the same Tunnel will share the same indirect pathset.
    bool is_ms_evpn_tep_ip(void) {return ms_evpn_tep_ip_;}
    void set_ms_evpn_tep_ip(const ip_addr_t& destip) {
        destip_ = destip; ms_evpn_tep_ip_ = true;
    }
    void reset_ms_evpn_tep_ip() {
        ms_evpn_tep_ip_ = false;
        chk_and_reset_destip_();
    }

    // Non-MS external IP tracking object(s) are using this Indirect pathset
    // There could be multiple such external objects tracking the same Dest IP
    void add_ip_track_obj(const pds_obj_key_t& objkey,
                          const ip_addr_t& destip) {
        ip_track_obj_keys_.emplace(objkey);
        destip_ = destip; ms_evpn_tep_ip_ = false;
    }
    void del_ip_track_obj(const pds_obj_key_t& objkey) {
        ip_track_obj_keys_.erase(objkey);
        chk_and_reset_destip_();
    }

private:
    ms_hw_tbl_id_t direct_ps_dpcorr_ = 0; // direct pathset DP correlator
    ip_addr_t  destip_;       // Dest IP that this Pathset tracks

    // Back-ref: Indirect pathset -> List of Obj keys of the IP tracked object
    std::unordered_set<pds_obj_key_t, pds_obj_key_hash> ip_track_obj_keys_;

    bool  ms_evpn_tep_ip_ = true; // Is the IP being tracked a TEP IP
                                   // created from MS EVPN
    void chk_and_reset_destip_(void);
};

class indirect_ps_store_t : public obj_store_t <ms_ps_id_t, indirect_ps_obj_t> {
};

void indirect_ps_slab_init (slab_uptr_t slabs[], sdk::lib::slab_id_t slab_id);

} // End namespace

#endif

