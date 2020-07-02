//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include "nic/apollo/test/api/utils/device.hpp"

namespace test {
namespace api {

//----------------------------------------------------------------------------
// Device feeder class routines
//----------------------------------------------------------------------------

void
device_feeder::init(const char *device_ip_str, std::string mac_addr_str,
                    const char *gw_ip_str, bool bridge_en,
                    pds_learn_mode_t learn_mode, uint32_t learn_age_timeout,
                    bool overlay_routing_en, pds_device_profile_t dp,
                    pds_memory_profile_t mp, pds_device_oper_mode_t dev_op_mode,
                    int num_device, bool stash) {

    memset(&spec, 0, sizeof(pds_device_spec_t));

    str2ipaddr(device_ip_str, &spec.device_ip_addr);
    str2ipaddr(gw_ip_str, &spec.gateway_ip_addr);
    if (!apulu()) {
        mac_str_to_addr((char *)mac_addr_str.c_str(), spec.device_mac_addr);
    }
    spec.bridging_en = bridge_en;
    spec.learn_spec.learn_mode = learn_mode;
    spec.learn_spec.learn_age_timeout = learn_age_timeout;
    spec.overlay_routing_en = overlay_routing_en;
    spec.device_profile = dp;
    spec.memory_profile = mp;
    if (apulu()) {
        spec.dev_oper_mode = dev_op_mode;
    } else {
        spec.dev_oper_mode = PDS_DEV_OPER_MODE_BITW_SMART_SWITCH;
    }
    num_obj = num_device;
    stash_ = stash;
}

device_feeder::device_feeder(const device_feeder& feeder) {
    memcpy(&this->spec, &feeder.spec, sizeof(pds_device_spec_t));
    num_obj = feeder.num_obj;
    this->stash_  = feeder.stash();
}

void
device_feeder::spec_build(pds_device_spec_t *spec) const {
    memcpy(spec, &this->spec, sizeof(pds_device_spec_t));
}

bool
device_feeder::spec_compare(const pds_device_spec_t *spec) const {
    if (!IPADDR_EQ(&this->spec.device_ip_addr, &spec->device_ip_addr)) {
        return false;
    }
    if (!apulu()) {
        if (memcmp(this->spec.device_mac_addr, spec->device_mac_addr,
                   ETH_ADDR_LEN)) {
            return false;
        }
    }
    if (!IPADDR_EQ(&this->spec.gateway_ip_addr, &spec->gateway_ip_addr)) {
        return false;
    }
    return true;
}

bool
device_feeder::status_compare(const pds_device_status_t *status1,
                              const pds_device_status_t *status2) const {
    return true;
}

//----------------------------------------------------------------------------
// DEVICE CRUD helper routines
//----------------------------------------------------------------------------

void
device_create (device_feeder& feeder)
{
    pds_batch_ctxt_t bctxt = batch_start();

    SDK_ASSERT_RETURN_VOID(
        (SDK_RET_OK == many_create<device_feeder>(bctxt, feeder)));
    batch_commit(bctxt);
}

void
device_read (device_feeder& feeder, sdk_ret_t exp_result)
{
    SDK_ASSERT_RETURN_VOID(
        (SDK_RET_OK == many_read<device_feeder>(feeder, exp_result)));
}

static void
device_attr_update (device_feeder& feeder, pds_device_spec_t *spec,
                    uint64_t chg_bmap)
{
    if (bit_isset(chg_bmap, DEVICE_ATTR_DEVICE_IP)) {
        feeder.spec.device_ip_addr = spec->device_ip_addr;
    }
    if (bit_isset(chg_bmap, DEVICE_ATTR_DEVICE_MAC)) {
        memcpy(&feeder.spec.device_mac_addr, &spec->device_mac_addr,
               sizeof(spec->device_mac_addr));
    }
    if (bit_isset(chg_bmap, DEVICE_ATTR_GATEWAY_IP)) {
        feeder.spec.gateway_ip_addr = spec->gateway_ip_addr;
    }
    if (bit_isset(chg_bmap, DEVICE_ATTR_BRIDGING_EN)) {
        feeder.spec.bridging_en = spec->bridging_en;
    }
    if (bit_isset(chg_bmap, DEVICE_ATTR_LEARNING_EN)) {
        feeder.spec.learn_spec.learn_mode = spec->learn_spec.learn_mode;
    }
    if (bit_isset(chg_bmap, DEVICE_ATTR_LEARN_AGE_TIME_OUT)) {
        feeder.spec.learn_spec.learn_age_timeout =
            spec->learn_spec.learn_age_timeout;
    }
    if (bit_isset(chg_bmap, DEVICE_ATTR_OVERLAY_ROUTING_EN)) {
        feeder.spec.overlay_routing_en = spec->overlay_routing_en;
    }
    if (bit_isset(chg_bmap, DEVICE_ATTR_DEVICE_PROFILE)) {
        feeder.spec.device_profile = spec->device_profile;
    }
    if (bit_isset(chg_bmap, DEVICE_ATTR_MEMORY_PROFILE)) {
        feeder.spec.memory_profile = spec->memory_profile;
    }
    if (bit_isset(chg_bmap, DEVICE_ATTR_DEV_OPER_MODE)) {
        feeder.spec.dev_oper_mode = spec->dev_oper_mode;
    }
}

void
device_update (device_feeder& feeder, pds_device_spec_t *spec,
               uint64_t chg_bmap, sdk_ret_t exp_result)
{
    pds_batch_ctxt_t bctxt = batch_start();

    device_attr_update(feeder, spec, chg_bmap);
    SDK_ASSERT_RETURN_VOID(
        (SDK_RET_OK == many_update<device_feeder>(bctxt, feeder)));

    // if expected result is err, batch commit should fail
    if (exp_result == SDK_RET_ERR)
        batch_commit_fail(bctxt);
    else
        batch_commit(bctxt);
}

void
device_delete (device_feeder& feeder)
{
    pds_batch_ctxt_t bctxt = batch_start();

    SDK_ASSERT_RETURN_VOID(
        (SDK_RET_OK == many_delete<device_feeder>(bctxt, feeder)));
    batch_commit(bctxt);
}

//----------------------------------------------------------------------------
// Misc routines
//----------------------------------------------------------------------------

// do not modify these sample values as rest of system is sync with these
const char *  k_device_ip ="91.0.0.1";
static device_feeder k_device_feeder;

void sample_device_setup(pds_batch_ctxt_t bctxt) {
    // setup and teardown parameters should be in sync
    k_device_feeder.init(k_device_ip, "00:00:01:02:0a:0b", "90.0.0.2", false,
                         PDS_LEARN_MODE_NONE, 0, false,
                         PDS_DEVICE_PROFILE_DEFAULT, PDS_MEMORY_PROFILE_DEFAULT,
                         PDS_DEV_OPER_MODE_HOST);
    many_create(bctxt, k_device_feeder);
}

void sample_device_setup_validate() {
    k_device_feeder.init(k_device_ip, "00:00:01:02:0a:0b", "90.0.0.2", false,
                         PDS_LEARN_MODE_NONE, 0, false,
                         PDS_DEVICE_PROFILE_DEFAULT, PDS_MEMORY_PROFILE_DEFAULT,
                         PDS_DEV_OPER_MODE_HOST);
    many_read(k_device_feeder);
}

void sample_device_teardown(pds_batch_ctxt_t bctxt) {
    // this feeder base values doesn't matter in case of deletes
    k_device_feeder.init(k_device_ip, "00:00:01:02:0a:0b", "90.0.0.2", false,
                         PDS_LEARN_MODE_NONE, 0, false,
                         PDS_DEVICE_PROFILE_DEFAULT, PDS_MEMORY_PROFILE_DEFAULT,
                         PDS_DEV_OPER_MODE_HOST);
    many_delete(bctxt, k_device_feeder);
}

}    // namespace api
}    // namespace test
