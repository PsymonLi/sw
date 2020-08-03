//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------
#ifndef __HII_HPP__
#define __HII_HPP__

#include <map>

#include "nic/sdk/platform/devapi/devapi.hpp"

std::string hii_attr_to_str (int attr);

typedef struct hii_dev_cfg_s {
    bool vlan_en;
    uint16_t vlan;
} hii_dev_cfg_t;

class HII
{
private:
    /* data */
    devapi *dev_api;
    bool oob_en;
    bool uid_led_on;
    std::map<std::string, hii_dev_cfg_t *> dev_cfgs;
    uint32_t capabilities;
    void InitCapabilities();
    void InitDevConfig();

public:
    HII(devapi *dev_api);
    ~HII();
    bool GetOobEn() { return this->oob_en; }
    void SetOobEn(bool oob_en) { this->oob_en = oob_en; }

    bool GetLedStatus() { return this->uid_led_on; }
    sdk_ret_t SetLedStatus(bool uid_led_on);

    bool GetVlanEn(string dev_name);
    void SetVlanEn(string dev_name, bool vlan_en);

    uint16_t GetVlan(string dev_name);
    void SetVlan(string dev_name, uint16_t vlan);

    uint32_t GetCapabilities() { return this->capabilities; }

    void Reset();

    hii_dev_cfg_t *GetDevCfg(string dev_name);
};

#endif  // __HII_HPP__
