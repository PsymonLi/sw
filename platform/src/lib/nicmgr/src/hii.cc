//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------
#include "nicmgr_utils.hpp"
#include "nic/sdk/lib/runenv/runenv.h"
#include "hii_ipc.hpp"
#include "hii.hpp"
#include "eth_if.h"
#include "logger.hpp"

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

    try {
        std::ifstream hii_json_cfg(HII_CFG_FILE);
        boost::property_tree::read_json(hii_json_cfg, cfg);
    } catch (...) {
        NIC_LOG_DEBUG("HII: CFG file not found, using defaults");
        _SetDefaultConfig();
    }
}

void
HII::_SetDefaultConfig()
{
    cfg.clear();

    // UID led config is not persistent, so setting default
    uid_led_on = DEFAULT_UID_LED_STATE;
}

void
HII::_InitCapabilities()
{
    bool ncsi_cap;

    capabilities = 0;
    ncsi_cap = sdk::lib::runenv::is_feature_capable(sdk::lib::NCSI_FEATURE) == sdk::lib::FEATURE_ENABLED;
    capabilities |= ncsi_cap << IONIC_HII_CAPABILITY_NCSI;
}

void
HII::Reset()
{
    std::remove(HII_CFG_FILE.c_str());
    _SetDefaultConfig();
}

sdk_ret_t
HII::_Flush()
{
    try {
        boost::property_tree::write_json(HII_CFG_FILE, cfg);
        return (SDK_RET_OK);
    } catch (...) {
        NIC_LOG_ERR("Failed to flush config");
        return (SDK_RET_ERR);
    }
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
