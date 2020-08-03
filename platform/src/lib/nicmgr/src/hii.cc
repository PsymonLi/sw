//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------
#include "nicmgr_utils.hpp"
#include "nic/sdk/lib/runenv/runenv.h"
#include "hii_ipc.hpp"
#include "hii.hpp"
#include "eth_if.h"
#include "logger.hpp"

#define HII_DEFAULT_DEV_NAME    "DEFAULT"

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
    this->oob_en = false;
    this->uid_led_on = false;
    InitCapabilities();
    InitDevConfig();
}

HII::~HII()
{
}

void
HII::InitCapabilities()
{
    bool ncsi_cap;

    capabilities = 0;
    ncsi_cap = sdk::lib::runenv::is_feature_capable(sdk::lib::NCSI_FEATURE) == sdk::lib::FEATURE_ENABLED;
    capabilities |= ncsi_cap << IONIC_HII_CAPABILITY_NCSI;
}

void
HII::InitDevConfig()
{
    hii_dev_cfg_t *dcfg;

    for (int i = 0; i < 2; i++) {
        std::string dev_name = "eth" + std::to_string(i);
        dcfg = new hii_dev_cfg_t {};
        dcfg->vlan_en = true;
        dcfg->vlan = 10;
        dev_cfgs[dev_name] = dcfg;
    }
}

void
HII::Reset()
{
    oob_en = false;
    uid_led_on = false;
    InitDevConfig();
}

sdk_ret_t
HII::SetLedStatus(bool uid_led_on)
{
    if (!dev_api) {
        NIC_LOG_ERR("HII: Uninitialized devapi, cannot set led");
        return (SDK_RET_ERR);
    }
    if (dev_api->hii_set_uid_led(uid_led_on) != SDK_RET_OK) {
        NIC_LOG_ERR("HII: Failed to set led status: {}", uid_led_on ? "On" : "Off");
        return (SDK_RET_ERR);
    }
    this->uid_led_on = uid_led_on;

    return (SDK_RET_OK);
}

hii_dev_cfg_t *
HII::GetDevCfg(string dev_name)
{
    auto it = dev_cfgs.find(dev_name);

    if (it != dev_cfgs.end()) {
        return it->second;
    } else {
        return dev_cfgs[HII_DEFAULT_DEV_NAME];
    }
}

bool
HII::GetVlanEn(string dev_name)
{
    hii_dev_cfg_t *dcfg = GetDevCfg(dev_name);

    return dcfg->vlan_en;
}

void
HII::SetVlanEn(string dev_name, bool vlan_en)
{
    hii_dev_cfg_t *dcfg = GetDevCfg(dev_name);

    dcfg->vlan_en = vlan_en;
}

uint16_t
HII::GetVlan(string dev_name)
{
    hii_dev_cfg_t *dcfg = GetDevCfg(dev_name);

    return dcfg->vlan;
}

void
HII::SetVlan(string dev_name, uint16_t vlan)
{
    hii_dev_cfg_t *dcfg = GetDevCfg(dev_name);

    dcfg->vlan = vlan;
}
