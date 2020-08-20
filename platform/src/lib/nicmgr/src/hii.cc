//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------
#include "nicmgr_utils.hpp"
#include "nic/sdk/lib/runenv/runenv.h"
#include "hii.hpp"
#include "eth_if.h"
#include "logger.hpp"

using boost::property_tree::ptree;
using boost::optional;


std::string
hii_attr_to_str (int attr)
{
    switch (attr) {
    CASE(IONIC_HII_ATTR_OOB_EN);
    CASE(IONIC_HII_ATTR_UID_LED);
    CASE(IONIC_HII_ATTR_VLAN);
    default:
        return "IONIC_HII_ATTR_UNKNOWN";
    }
}

HII::HII(devapi *dev_api)
{
    this->dev_api = dev_api;
    _InitConfig();
    _InitCapabilities();
}

HII::~HII()
{

}

void
HII::_InitConfig()
{
    NIC_LOG_DEBUG("HII: Reading config");

    // UID led config is not persistent, so setting default
    uid_led_on = DEFAULT_UID_LED_STATE;
    gbl_cfg = sdk::lib::config::create(HII_CFG_FILE);
    if (gbl_cfg == nullptr) {
        NIC_LOG_ERR("HII: couldn't instantiate global config, exiting");
        return;
    }
    optional<ptree &> tempcfg = gbl_cfg->get_child_optional(HII_CFG_KEY);
    if (!tempcfg) {
        NIC_LOG_WARN("HII: HII CFG not found, using defaults");
        _SetDefaultConfig();
    } else {
        cfg = tempcfg.get();
    }
}

sdk_ret_t
HII::_SetDefaultConfig()
{
    // UID led config is not persistent, so setting default
    uid_led_on = DEFAULT_UID_LED_STATE;
    cfg.clear();
    if (!gbl_cfg) {
        NIC_LOG_ERR("HII: global config is not instantiated, exiting");
        return SDK_RET_ERR;
    }
    return gbl_cfg->erase(this->HII_CFG_KEY);
}

void
HII::_InitCapabilities()
{
    bool ncsi_cap;

    capabilities = 0;
    ncsi_cap = sdk::lib::runenv::is_feature_capable(sdk::lib::NCSI_FEATURE) == sdk::lib::FEATURE_ENABLED;
    capabilities |= ncsi_cap << IONIC_HII_CAPABILITY_NCSI;
}

sdk_ret_t
HII::Reset()
{
    return _SetDefaultConfig();
}

sdk_ret_t
HII::_Flush()
{
    if (!gbl_cfg) {
        NIC_LOG_ERR("HII: global config is not instantiated, exiting");
        return SDK_RET_ERR;
    }
    return gbl_cfg->set_child(this->HII_CFG_KEY, cfg);
}

bool
HII::GetLedStatus()
{
    return uid_led_on;
}

sdk_ret_t
HII::SetLedStatus(bool uid_led_on)
{
    if (this->uid_led_on == uid_led_on) {
        NIC_LOG_INFO("HII: NOP, uid led is already: {}", uid_led_on ? "On" : "Off");
        return (SDK_RET_OK);
    }
    if (!dev_api) {
        NIC_LOG_ERR("HII: Uninitialized devapi, cannot set led");
        return (SDK_RET_ERR);
    }
    if (dev_api->hii_set_uid_led(uid_led_on) != SDK_RET_OK) {
        NIC_LOG_ERR("HII: Failed to set uid led state: {}", uid_led_on ? "On" : "Off");
        return (SDK_RET_ERR);
    }
    this->uid_led_on = uid_led_on;

    return (SDK_RET_OK);
}

bool
HII::GetVlanEn(string dev_name)
{
    std::string path;

    path = "device_cfg." + dev_name + ".vlan_en";
    return cfg.get<bool>(path, DEFAULT_VLAN_EN);
}

uint16_t
HII::GetVlan(string dev_name)
{
    std::string path;

    path = "device_cfg." + dev_name + ".vlan";
    return cfg.get<uint16_t>(path, DEFAULT_VLAN);
}

sdk_ret_t
HII::SetVlanCfg(string dev_name, bool vlan_en, uint16_t vlan)
{
    std::string path;

    path = "device_cfg." + dev_name + ".vlan_en";
    cfg.put(path, vlan_en);

    path = "device_cfg." + dev_name + ".vlan";
    cfg.put(path, vlan);

    if (_Flush() != SDK_RET_OK) {
        NIC_LOG_ERR("HII: Unable to save dev: {} vlan_en: {} vlan: {}", dev_name,
                    vlan_en, vlan);
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

bool
HII::GetOobEn()
{
    std::string path = "oob_en";

    return cfg.get<bool>(path, DEFAULT_OOB_EN);
}

sdk_ret_t
HII::SetOobEn(bool oob_en)
{
    std::string path = "oob_en";

    cfg.put(path, oob_en);
    if (_Flush() != SDK_RET_OK) {
        NIC_LOG_ERR("HII: Unable to save oob_en: {}", oob_en ? "True" : "False");
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}
