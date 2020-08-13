//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------
#ifndef __HII_HPP__
#define __HII_HPP__

#include <map>

#include "nic/sdk/platform/devapi/devapi.hpp"

std::string hii_attr_to_str (int attr);

class HII
{
private:
    /* data */
    boost::property_tree::ptree cfg;
    bool uid_led_on;
    devapi *dev_api;
    uint32_t capabilities;
    void _InitConfig();
    void _InitCapabilities();
    void _SetDefaultConfig();
    sdk_ret_t _Flush();

public:
    HII(devapi *dev_api);
    ~HII();
    bool GetOobEn();
    sdk_ret_t SetOobEn(bool oob_en);

    bool GetLedStatus();
    sdk_ret_t SetLedStatus(bool uid_led_on);

    bool GetVlanEn(string dev_name);
    uint16_t GetVlan(string dev_name);
    sdk_ret_t SetVlanCfg(string dev_name, bool vlan_en, uint16_t vlan);

    uint32_t GetCapabilities() { return this->capabilities; }

    void Reset();

    /* Default values */
    const bool DEFAULT_OOB_EN = false;
    const bool DEFAULT_UID_LED_STATE = false;
    const bool DEFAULT_VLAN_EN = false;
    const uint16_t DEFAULT_VLAN = 0;
    const std::string HII_CFG_FILE = "/sysconfig/config0/hii_cfg.json";
};

#endif  // __HII_HPP__
