//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include "impl_db.h"
#include "pdsa_impl_db_hdlr.h"
#include "mapping.h"

static void
clear_ses_count (pds_impl_db_vnic_entry_t *entry)
{
    entry->active_ses_count = 0;
}

void
pds_vnic_active_session_clear (void) {
    pds_impl_db_foreach_vnic(&clear_ses_count);
}

int
pds_dst_vnic_info_get (uint16_t lkp_id, uint32_t addr, uint16_t *vnic_id,
                       uint16_t *vnic_nh_hw_id)
{
    pds_impl_db_vnic_entry_t *vnic;
    pds_impl_db_subnet_entry_t *subnet;
    int ret;

    subnet = pds_impl_db_subnet_get(lkp_id);
    if (subnet == NULL) {
        return -1;
    }

    ret = pds_local_mapping_vnic_id_get(subnet->vpc_id, addr, vnic_id);
    if (ret != 0) {
        return ret;
    }

    vnic = pds_impl_db_vnic_get(*vnic_id);
    if (vnic == NULL) {
        return -1;
    }

    *vnic_nh_hw_id = vnic->nh_hw_id;
    return 0;
}

int
pds_src_vnic_info_get (uint16_t lkp_id, uint32_t addr, uint8_t **rewrite,
                       uint16_t *host_lif_hw_id)
{
    pds_impl_db_vnic_entry_t *vnic;
    pds_impl_db_subnet_entry_t *subnet;
    uint16_t vnic_id;
    int ret;

    subnet = pds_impl_db_subnet_get(lkp_id);
    if (subnet == NULL) {
        return -1;
    }

    ret = pds_local_mapping_vnic_id_get(subnet->vpc_id, addr, &vnic_id);
    if (ret != 0) {
        return ret;
    }

    vnic = pds_impl_db_vnic_get(vnic_id);
    if (vnic == NULL) {
        return -1;
    }

    *rewrite = vnic->rewrite;
    *host_lif_hw_id = vnic->host_lif_hw_id;
    return 0;
}
