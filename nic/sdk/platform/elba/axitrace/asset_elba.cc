// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#include <iostream>
#include <boost/algorithm/string.hpp>

#include "asset_elba.h"
#include "config.h"
#include "option.h"
#include "generic.h"
#include "msg_undef.inc"
#include "msg_redef.inc"
#include "pio.h"
#include "console.h"


#include "asic_hpp/elb_top_csr_defines.h"
#include "asic_hpp/elb_top_csr_defines.h"
#include "asic_hpp/elb_filter_csr_define.h"
#include "asic_hpp/elb_pxb_c_hdr.h"
#include "asic_hpp/elb_ap_c_hdr.h"
#include "asic_hpp/elb_ms_c_hdr.h"
#include "asic_hpp/elb_soc_c_hdr.h"
#include "asic_hpp/elb_wa_c_hdr.h"
#include "asic_hpp/elb_pics_c_hdr.h"
#include "asic_hpp/elb_xd_c_hdr.h"
#include "asic_hpp/elb_hese_c_hdr.h"
#include "asic_hpp/elb_pbc_c_hdr.h"



namespace axitrace {

static map<string, map<string, map<string, uint64_t> > > helper;

void  asset_elba::test()
{
    cout << "this is elba" << endl;
}



asset_elba::axi_condition::axi_condition(const string& nm, asset::filter* f)
    : asset::condition(nm, f)
{
    enabled = true;
    oenabled = true;
    add_field("mask");
    add_field("match");
}



asset_elba::axi_condition::~axi_condition()
{
}



bool asset_elba::axi_condition::overlap(const condition* cond) const
{
    assert(vals.size() == 2);
    if ((get("mask") & cond->get("mask")) == 0) return true;
    return ((get("mask") & cond->get("mask") & get("match")) == (get("mask") & cond->get("mask") & cond->get("match")));
}



asset_elba::axi_action::axi_action(const string& nm, asset::filter* f)
    : asset::action(nm, f)
{
    add_field("overwrite");
}



asset_elba::axi_action::~axi_action()
{
}



asset_elba::axi_condition_addr_range::axi_condition_addr_range(asset::filter* f)
    : asset_elba::axi_condition("addr_range", f)
{
    clear();
    add_field("addr_lo");
    add_field("addr_hi");
}



asset_elba::axi_condition_addr_range::~axi_condition_addr_range()
{
}


asset::element* asset_elba::axi_condition_addr_range::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset_elba::axi_condition_addr_range* p = new asset_elba::axi_condition_addr_range(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}


bool asset_elba::axi_condition_addr_range::overlap(const condition* cond) const
{
    assert(dynamic_cast<const asset_elba::axi_condition_addr_range*>(cond));
    if (get("addr_hi") < cond->get("addr_lo") || get("addr_lo") > cond->get("addr_hi")) return false;
    return true;
}


asset_elba::axi_condition_hit_count::axi_condition_hit_count(asset::filter* f)
    : asset_elba::axi_condition("hit_count", f)
{
    clear();
    add_field("hit_count_read");
    add_field("hit_count_write");
}


asset_elba::axi_condition_hit_count::~axi_condition_hit_count()
{
}


asset::element* asset_elba::axi_condition_hit_count::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset_elba::axi_condition_hit_count* p = new asset_elba::axi_condition_hit_count(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}



bool asset_elba::axi_condition_hit_count::overlap(const condition* cond) const
{
    return false;
}



asset_elba::axi_condition_addr_host::axi_condition_addr_host(asset::filter* f)
    : asset_elba::axi_condition("addr_host", f)
{
}


asset::element* asset_elba::axi_condition_addr_host::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_condition_addr_host(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}




asset_elba::axi_condition_addr_host::~axi_condition_addr_host()
{
}



asset_elba::axi_condition_addr_pic::axi_condition_addr_pic(asset::filter* f)
    : asset_elba::axi_condition("addr_pic", f)
{
}


asset::element* asset_elba::axi_condition_addr_pic::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_condition_addr_pic(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}



asset_elba::axi_condition_addr_pic::~axi_condition_addr_pic()
{
}



asset_elba::axi_condition_addr_lif::axi_condition_addr_lif(asset::filter* f)
    : asset_elba::axi_condition("addr_lif", f)
{
}


asset::element* asset_elba::axi_condition_addr_lif::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_condition_addr_lif(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}


asset_elba::axi_condition_addr_lif::~axi_condition_addr_lif()
{
}



asset_elba::axi_condition_src_id::axi_condition_src_id(asset::filter* f)
    : asset_elba::axi_condition("src_id", f)
{
}


asset::element* asset_elba::axi_condition_src_id::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_condition_src_id(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}


asset_elba::axi_condition_src_id::~axi_condition_src_id()
{
}


asset_elba::axi_condition_awprot::axi_condition_awprot(asset::filter* f)
    : asset_elba::axi_condition("awprot", f)
{
}


asset::element* asset_elba::axi_condition_awprot::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_condition_awprot(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}


asset_elba::axi_condition_awprot::~axi_condition_awprot()
{
}



asset_elba::axi_condition_arprot::axi_condition_arprot(asset::filter* f)
    : asset_elba::axi_condition("arprot", f)
{
}


asset::element* asset_elba::axi_condition_arprot::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_condition_arprot(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}


asset_elba::axi_condition_arprot::~axi_condition_arprot()
{
}



asset_elba::axi_condition_awcache::axi_condition_awcache(asset::filter* f)
    : asset_elba::axi_condition("awcache", f)
{
}


asset::element* asset_elba::axi_condition_awcache::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_condition_awcache(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}


asset_elba::axi_condition_awcache::~axi_condition_awcache()
{
}



asset_elba::axi_condition_arcache::axi_condition_arcache(asset::filter* f)
    : asset_elba::axi_condition("arcache", f)
{
}


asset::element* asset_elba::axi_condition_arcache::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_condition_arcache(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}


asset_elba::axi_condition_arcache::~axi_condition_arcache()
{
}



asset_elba::axi_action_awprot::axi_action_awprot(asset::filter* f)
    : asset_elba::axi_action("awprot", f)
{
}


asset::element* asset_elba::axi_action_awprot::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_action_awprot(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}


asset_elba::axi_action_awprot::~axi_action_awprot()
{
}



asset_elba::axi_action_arprot::axi_action_arprot(asset::filter* f)
    : asset_elba::axi_action("arprot", f)
{
}


asset::element* asset_elba::axi_action_arprot::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_action_arprot(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}


asset_elba::axi_action_arprot::~axi_action_arprot()
{
}



asset_elba::axi_action_awcache::axi_action_awcache(asset::filter* f)
    : asset_elba::axi_action("awcache", f)
{
}


asset::element* asset_elba::axi_action_awcache::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_action_awcache(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}


asset_elba::axi_action_awcache::~axi_action_awcache()
{
}



asset_elba::axi_action_arcache::axi_action_arcache(asset::filter* f)
    : asset_elba::axi_action("arcache", f)
{
}


asset::element* asset_elba::axi_action_arcache::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_action_arcache(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}


asset_elba::axi_action_arcache::~axi_action_arcache()
{
}



asset_elba::axi_action_awacp_ena::axi_action_awacp_ena(asset::filter* f)
    : asset_elba::axi_action("awacp_ena", f)
{
}


asset::element* asset_elba::axi_action_awacp_ena::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_action_awacp_ena(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}


asset_elba::axi_action_awacp_ena::~axi_action_awacp_ena()
{
}


asset::element* asset_elba::axi_action_awacp_val::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_action_awacp_val(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}


asset_elba::axi_action_awacp_val::axi_action_awacp_val(asset::filter* f)
    : asset_elba::axi_action("awacp_val", f)
{
}



asset_elba::axi_action_awacp_val::~axi_action_awacp_val()
{
}



asset_elba::axi_action_aracp_ena::axi_action_aracp_ena(asset::filter* f)
    : asset_elba::axi_action("aracp_ena", f)
{
}


asset::element* asset_elba::axi_action_aracp_ena::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_action_aracp_ena(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}


asset_elba::axi_action_aracp_ena::~axi_action_aracp_ena()
{
}



asset_elba::axi_action_aracp_val::axi_action_aracp_val(asset::filter* f)
    : asset_elba::axi_action("aracp_val", f)
{
}


asset::element* asset_elba::axi_action_aracp_val::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_action_aracp_val(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}


asset_elba::axi_action_aracp_val::~axi_action_aracp_val()
{
}



asset_elba::axi_action_control::axi_action_control(asset::filter* f)
    : asset_elba::axi_action("control", f)
{
    clear();
    enabled = true;
    oenabled = true;

    add_field("read_access");
    add_field("write_access");
    add_field("read_freeze");
    add_field("write_freeze");
    add_field("read_interrupt");
    add_field("write_interrupt");
    add_field("log_req");
    add_field("log_resp");
    add_field("use_cache");
    add_field("inval_send");
    add_field("inval_receive");
    add_field("local_ack");
    add_field("round64");
}


asset::element* asset_elba::axi_action_control::clone(asset::filter* f) const
{
    asset::element_skip_build_fields = true;
    asset::element* p = new asset_elba::axi_action_control(f);
    p->copy(this);
    asset::element_skip_build_fields = false;
    return p;
}


asset_elba::axi_action_control::~axi_action_control()
{
}


asset_elba::axi_action_trace::axi_action_trace(asset::filter* f)
    : asset_elba::axi_action("trace", f)
{
    clear();
    enabled = false;
    oenabled = false;
    global = true;

    add_field("always_on");
    add_field("resp_err");
    add_field("wrap");
    add_field("rst");
    add_field("awcache");
    add_field("base_addr");
    add_field("buf_size");
}


asset_elba::axi_action_trace::~axi_action_trace()
{
}


asset_elba::axi_condition_trace_status::axi_condition_trace_status(asset::filter* f)
    : asset_elba::axi_condition("trace_status", f)
{
    clear();
    enabled = true;
    oenabled = true;
    global = true;

    add_field("index");
    add_field("generation");
}


asset_elba::axi_condition_trace_status::~axi_condition_trace_status()
{
}


asset_elba::axi_filter::axi_filter(const string& port, int id)
    : asset::filter(port, id)
{
}


asset_elba::axi_filter::~axi_filter()
{
}


asset::filter*  asset_elba::axi_filter::clone(const string& port, int id) const
{
    asset::filter* p = new asset_elba::axi_filter(port, id);
    for (unsigned i = 0; i < conditions.size(); ++i) {
        asset::element* e = conditions[i]->clone(p);
        assert(dynamic_cast<asset::condition*>(e));
        p->conditions.push_back(dynamic_cast<asset::condition*>(e));
    }
    for (unsigned i = 0; i < actions.size(); ++i) {
        asset::element* e = actions[i]->clone(p);
        assert(dynamic_cast<asset::action*>(e));
        p->actions.push_back(dynamic_cast<asset::action*>(e));
    }
    p->map_condition_name2idx = map_condition_name2idx;
    p->map_action_name2idx = map_action_name2idx;
    return p;
}



bool asset_elba::axi_filter::build()
{
#ifdef BUILD_CONDITION
#undef BUILD_CONDITION
#undef BUILD_ACTION
#endif
#define BUILD_CONDITION(X) \
    condition_ptr = new asset_elba::axi_condition_##X(this); \
    map_condition_name2idx[condition_ptr->name] = conditions.size(); \
    conditions.push_back(condition_ptr);
#define BUILD_ACTION(X) \
    action_ptr = new asset_elba::axi_action_##X(this); \
    map_action_name2idx[action_ptr->name] = actions.size(); \
    actions.push_back(action_ptr);

    asset_elba::axi_condition* condition_ptr;
    asset_elba::axi_action* action_ptr;

    BUILD_CONDITION(hit_count);
    BUILD_CONDITION(addr_range);
    BUILD_CONDITION(addr_host);
    BUILD_CONDITION(addr_pic);
    BUILD_CONDITION(addr_lif);
    BUILD_CONDITION(src_id);
    BUILD_CONDITION(awprot);
    BUILD_CONDITION(arprot);
    BUILD_CONDITION(awcache);
    BUILD_CONDITION(arcache);
    
    BUILD_ACTION(awprot);
    BUILD_ACTION(arprot);
    BUILD_ACTION(awcache);
    BUILD_ACTION(arcache);
    BUILD_ACTION(awacp_ena);
    BUILD_ACTION(awacp_val);
    BUILD_ACTION(aracp_ena);
    BUILD_ACTION(aracp_val);
    BUILD_ACTION(control);

    return true;
}



bool asset_elba::init_constants()
{
    //=======
    addr_base["PX"]                 = ELB_ADDR_BASE_PXB_PXB_OFFSET;
    addr_ofst["PX"]["hit_count"]    =     ELB_PXB_CSR_INVF_FILTER_HIT_COUNT_BYTE_ADDRESS;
    addr_labl["PX"]["hit_count"]    =    "ELB_PXB_CSR_INVF_FILTER_HIT_COUNT";
    addr_elen["PX"]["hit_count"]    = 4 / ELB_PXB_CSR_INVF_FILTER_HIT_COUNT_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["PX"]["addr_lo"]      =     ELB_PXB_CSR_INVF_FILTER_ADDR_LO_BYTE_ADDRESS;
    addr_labl["PX"]["addr_lo"]      =    "ELB_PXB_CSR_INVF_FILTER_ADDR_LO_DATA";
    addr_elen["PX"]["addr_lo"]      = 4 / ELB_PXB_CSR_INVF_FILTER_ADDR_LO_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["PX"]["addr_hi"]      =     ELB_PXB_CSR_INVF_FILTER_ADDR_HI_BYTE_ADDRESS;
    addr_labl["PX"]["addr_hi"]      =    "ELB_PXB_CSR_INVF_FILTER_ADDR_HI_DATA";
    addr_elen["PX"]["addr_hi"]      = 4 / ELB_PXB_CSR_INVF_FILTER_ADDR_HI_DATA_ARRAY_ELEMENT_SIZE;;
    addr_ofst["PX"]["addr_host"]    =     ELB_PXB_CSR_INVF_FILTER_ADDR_HOST_DATA_BYTE_ADDRESS;
    addr_labl["PX"]["addr_host"]    =    "ELB_PXB_CSR_INVF_FILTER_ADDR_HOST_DATA";
    addr_elen["PX"]["addr_host"]    = 4 / ELB_PXB_CSR_INVF_FILTER_ADDR_HOST_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["PX"]["addr_pic"]     =     ELB_PXB_CSR_INVF_FILTER_ADDR_PIC_DATA_BYTE_ADDRESS;
    addr_labl["PX"]["addr_pic"]     =    "ELB_PXB_CSR_INVF_FILTER_ADDR_PIC_DATA";
    addr_elen["PX"]["addr_pic"]     = 4 / ELB_PXB_CSR_INVF_FILTER_ADDR_PIC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["PX"]["addr_lif"]     =     ELB_PXB_CSR_INVF_FILTER_ADDR_LIF_DATA_BYTE_ADDRESS;
    addr_labl["PX"]["addr_lif"]     =    "ELB_PXB_CSR_INVF_FILTER_ADDR_LIF_DATA";
    addr_elen["PX"]["addr_lif"]     = 4 / ELB_PXB_CSR_INVF_FILTER_ADDR_LIF_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["PX"]["src_id"]       =     ELB_PXB_CSR_INVF_FILTER_SRC_DATA_BYTE_ADDRESS;
    addr_labl["PX"]["src_id"]       =    "ELB_PXB_CSR_INVF_FILTER_SRC_DATA";
    addr_elen["PX"]["src_id"]       = 4 / ELB_PXB_CSR_INVF_FILTER_SRC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["PX"]["prot"]         =     ELB_PXB_CSR_INVF_FILTER_AXI_PROT_DATA_BYTE_ADDRESS;
    addr_labl["PX"]["prot"]         =    "ELB_PXB_CSR_INVF_FILTER_AXI_PROT_DATA";
    addr_elen["PX"]["prot"]         = 4 / ELB_PXB_CSR_INVF_FILTER_AXI_PROT_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["PX"]["cache"]        =     ELB_PXB_CSR_INVF_FILTER_AXI_CACHE_DATA_BYTE_ADDRESS;
    addr_labl["PX"]["cache"]        =    "ELB_PXB_CSR_INVF_FILTER_AXI_CACHE_DATA";
    addr_elen["PX"]["cache"]        = 4 / ELB_PXB_CSR_INVF_FILTER_AXI_CACHE_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["PX"]["acp"]          =     ELB_PXB_CSR_INVF_FILTER_ACP_DATA_BYTE_ADDRESS;
    addr_labl["PX"]["acp"]          =    "ELB_PXB_CSR_INVF_FILTER_ACP_DATA";
    addr_elen["PX"]["acp"]          = 4 / ELB_PXB_CSR_INVF_FILTER_ACP_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["PX"]["control"]      =     ELB_PXB_CSR_INVF_FILTER_ADDR_CTL_VALUE_BYTE_ADDRESS;
    addr_labl["PX"]["control"]      =    "ELB_PXB_CSR_INVF_FILTER_ADDR_CTL_VALUE";
    addr_elen["PX"]["control"]      = 4 / ELB_PXB_CSR_INVF_FILTER_ADDR_CTL_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["PX"]["trace"]        =     ELB_PXB_CSR_INVF_FILTER_TRACE_BYTE_ADDRESS;
    addr_labl["PX"]["trace"]        =    "ELB_PXB_CSR_INVF_FILTER_TRACE";
    addr_elen["PX"]["trace"]        = 12;

    func_load["PX"]["hit_count"]    =     load_fields_hit_count;
    func_load["PX"]["addr_lo"]      =     load_fields_addr_lo;
    func_load["PX"]["addr_hi"]      =     load_fields_addr_hi;
    func_load["PX"]["addr_host"]    =     load_fields_addr_host;
    func_load["PX"]["addr_pic"]     =     load_fields_addr_pic;
    func_load["PX"]["addr_lif"]     =     load_fields_addr_lif;
    func_load["PX"]["src_id"]       =     load_fields_src_id;
    func_load["PX"]["prot"]         =     load_fields_prot;
    func_load["PX"]["cache"]        =     load_fields_cache;
    func_load["PX"]["acp"]          =     load_fields_acp;
    func_load["PX"]["control"]      =     load_fields_control;
    func_load["PX"]["trace"]        =     load_fields_trace;
    func_load["PX"]["trace_status"] =     load_fields_trace_status;
    
    func_deposit["PX"]["addr_lo"]      =     deposit_fields_addr_lo;
    func_deposit["PX"]["addr_hi"]      =     deposit_fields_addr_hi;
    func_deposit["PX"]["addr_host"]    =     deposit_fields_addr_host;
    func_deposit["PX"]["addr_pic"]     =     deposit_fields_addr_pic;
    func_deposit["PX"]["addr_lif"]     =     deposit_fields_addr_lif;
    func_deposit["PX"]["src_id"]       =     deposit_fields_src_id;
    func_deposit["PX"]["prot"]         =     deposit_fields_prot;
    func_deposit["PX"]["cache"]        =     deposit_fields_cache;
    func_deposit["PX"]["acp"]          =     deposit_fields_acp;
    func_deposit["PX"]["control"]      =     deposit_fields_control;
    func_deposit["PX"]["trace"]        =     deposit_fields_trace;
    
    helper["PX"]["src_id"]["load_c0"]           =             ELB_PXINVF_CSR_FILTER_SRC_WIDTH / 2;
    helper["PX"]["src_id"]["load_c1"]           = ~(-1ull << (ELB_PXINVF_CSR_FILTER_SRC_WIDTH / 2));


    //=======
    addr_base["AP"]                 =     ELB_ADDR_BASE_MS_AP_OFFSET;
    addr_ofst["AP"]["hit_count"]    =     ELB_AP_CSR_FILTER_HIT_COUNT_BYTE_ADDRESS;
    addr_labl["AP"]["hit_count"]    =    "ELB_AP_CSR_FILTER_HIT_COUNT";
    addr_elen["AP"]["hit_count"]    = 4 / ELB_AP_CSR_FILTER_HIT_COUNT_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["AP"]["addr_lo"]      =     ELB_AP_CSR_FILTER_ADDR_LO_DATA_BYTE_ADDRESS;
    addr_labl["AP"]["addr_lo"]      =    "ELB_AP_CSR_FILTER_ADDR_LO_DATA";
    addr_elen["AP"]["addr_lo"]      = 4 / ELB_AP_CSR_FILTER_ADDR_LO_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["AP"]["addr_hi"]      =     ELB_AP_CSR_FILTER_ADDR_HI_BYTE_ADDRESS;
    addr_labl["AP"]["addr_hi"]      =    "ELB_AP_CSR_FILTER_ADDR_HI_DATA";
    addr_elen["AP"]["addr_hi"]      = 4 / ELB_AP_CSR_FILTER_ADDR_HI_DATA_ARRAY_ELEMENT_SIZE;;
    addr_ofst["AP"]["addr_host"]    =     ELB_AP_CSR_FILTER_ADDR_HOST_DATA_BYTE_ADDRESS;
    addr_labl["AP"]["addr_host"]    =    "ELB_AP_CSR_FILTER_ADDR_HOST_DATA";
    addr_elen["AP"]["addr_host"]    = 4 / ELB_AP_CSR_FILTER_ADDR_HOST_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["AP"]["addr_pic"]     =     ELB_AP_CSR_FILTER_ADDR_PIC_DATA_BYTE_ADDRESS;
    addr_labl["AP"]["addr_pic"]     =    "ELB_AP_CSR_FILTER_ADDR_PIC_DATA";
    addr_elen["AP"]["addr_pic"]     = 4 / ELB_AP_CSR_FILTER_ADDR_PIC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["AP"]["addr_lif"]     =     ELB_AP_CSR_FILTER_ADDR_LIF_DATA_BYTE_ADDRESS;
    addr_labl["AP"]["addr_lif"]     =    "ELB_AP_CSR_FILTER_ADDR_LIF_DATA";
    addr_elen["AP"]["addr_lif"]     = 4 / ELB_AP_CSR_FILTER_ADDR_LIF_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["AP"]["src_id"]       =     ELB_AP_CSR_FILTER_SRC_DATA_BYTE_ADDRESS;
    addr_labl["AP"]["src_id"]       =    "ELB_AP_CSR_FILTER_SRC_DATA";
    addr_elen["AP"]["src_id"]       = 4 / ELB_AP_CSR_FILTER_SRC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["AP"]["prot"]         =     ELB_AP_CSR_FILTER_AXI_PROT_DATA_BYTE_ADDRESS;
    addr_labl["AP"]["prot"]         =    "ELB_AP_CSR_FILTER_AXI_PROT_DATA";
    addr_elen["AP"]["prot"]         = 4 / ELB_AP_CSR_FILTER_AXI_PROT_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["AP"]["cache"]        =     ELB_AP_CSR_FILTER_AXI_CACHE_DATA_BYTE_ADDRESS;
    addr_labl["AP"]["cache"]        =    "ELB_AP_CSR_FILTER_AXI_CACHE_DATA";
    addr_elen["AP"]["cache"]        = 4 / ELB_AP_CSR_FILTER_AXI_CACHE_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["AP"]["acp"]          =     ELB_AP_CSR_FILTER_ACP_DATA_BYTE_ADDRESS;
    addr_labl["AP"]["acp"]          =    "ELB_AP_CSR_FILTER_ACP_DATA";
    addr_elen["AP"]["acp"]          = 4 / ELB_AP_CSR_FILTER_ACP_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["AP"]["control"]      =     ELB_AP_CSR_FILTER_ADDR_CTL_VALUE_BYTE_ADDRESS;
    addr_labl["AP"]["control"]      =    "ELB_AP_CSR_FILTER_ADDR_CTL_VALUE";
    addr_elen["AP"]["control"]      = 4 / ELB_AP_CSR_FILTER_ADDR_CTL_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["AP"]["trace"]        =     ELB_AP_CSR_FILTER_TRACE_BYTE_ADDRESS;
    addr_labl["AP"]["trace"]        =    "ELB_AP_CSR_FILTER_TRACE";
    addr_elen["AP"]["trace"]        = 12;

    func_load["AP"]["hit_count"]    =     load_fields_hit_count;
    func_load["AP"]["addr_lo"]      =     load_fields_addr_lo;
    func_load["AP"]["addr_hi"]      =     load_fields_addr_hi;
    func_load["AP"]["addr_host"]    =     load_fields_addr_host;
    func_load["AP"]["addr_pic"]     =     load_fields_addr_pic;
    func_load["AP"]["addr_lif"]     =     load_fields_addr_lif;
    func_load["AP"]["src_id"]       =     load_fields_src_id;
    func_load["AP"]["prot"]         =     load_fields_prot;
    func_load["AP"]["cache"]        =     load_fields_cache;
    func_load["AP"]["acp"]          =     load_fields_acp;
    func_load["AP"]["control"]      =     load_fields_control;
    func_load["AP"]["trace"]        =     load_fields_trace;
    func_load["AP"]["trace_status"] =     load_fields_trace_status;
    
    func_deposit["AP"]["addr_lo"]      =     deposit_fields_addr_lo;
    func_deposit["AP"]["addr_hi"]      =     deposit_fields_addr_hi;
    func_deposit["AP"]["addr_host"]    =     deposit_fields_addr_host;
    func_deposit["AP"]["addr_pic"]     =     deposit_fields_addr_pic;
    func_deposit["AP"]["addr_lif"]     =     deposit_fields_addr_lif;
    func_deposit["AP"]["src_id"]       =     deposit_fields_src_id;
    func_deposit["AP"]["prot"]         =     deposit_fields_prot;
    func_deposit["AP"]["cache"]        =     deposit_fields_cache;
    func_deposit["AP"]["acp"]          =     deposit_fields_acp;
    func_deposit["AP"]["control"]      =     deposit_fields_control;
    func_deposit["AP"]["trace"]        =     deposit_fields_trace;
    
    helper["AP"]["src_id"]["load_c0"]           =             ELB_AP_CSR_FILTER_SRC_WIDTH / 2;
    helper["AP"]["src_id"]["load_c1"]           = ~(-1ull << (ELB_AP_CSR_FILTER_SRC_WIDTH / 2));
    

    //=======
    addr_base["SOC"]                 =     ELB_ADDR_BASE_MS_SOC_OFFSET;
    addr_ofst["SOC"]["hit_count"]    =     ELB_SOC_CSR_FILTER_HIT_COUNT_BYTE_ADDRESS;
    addr_labl["SOC"]["hit_count"]    =    "ELB_SOC_CSR_FILTER_HIT_COUNT";
    addr_elen["SOC"]["hit_count"]    = 4 / ELB_SOC_CSR_FILTER_HIT_COUNT_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["SOC"]["addr_lo"]      =     ELB_SOC_CSR_FILTER_ADDR_LO_DATA_BYTE_ADDRESS;
    addr_labl["SOC"]["addr_lo"]      =    "ELB_SOC_CSR_FILTER_ADDR_LO_DATA";
    addr_elen["SOC"]["addr_lo"]      = 4 / ELB_SOC_CSR_FILTER_ADDR_LO_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SOC"]["addr_hi"]      =     ELB_SOC_CSR_FILTER_ADDR_HI_BYTE_ADDRESS;
    addr_labl["SOC"]["addr_hi"]      =    "ELB_SOC_CSR_FILTER_ADDR_HI_DATA";
    addr_elen["SOC"]["addr_hi"]      = 4 / ELB_SOC_CSR_FILTER_ADDR_HI_DATA_ARRAY_ELEMENT_SIZE;;
    addr_ofst["SOC"]["addr_host"]    =     ELB_SOC_CSR_FILTER_ADDR_HOST_DATA_BYTE_ADDRESS;
    addr_labl["SOC"]["addr_host"]    =    "ELB_SOC_CSR_FILTER_ADDR_HOST_DATA";
    addr_elen["SOC"]["addr_host"]    = 4 / ELB_SOC_CSR_FILTER_ADDR_HOST_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SOC"]["addr_pic"]     =     ELB_SOC_CSR_FILTER_ADDR_PIC_DATA_BYTE_ADDRESS;
    addr_labl["SOC"]["addr_pic"]     =    "ELB_SOC_CSR_FILTER_ADDR_PIC_DATA";
    addr_elen["SOC"]["addr_pic"]     = 4 / ELB_SOC_CSR_FILTER_ADDR_PIC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SOC"]["addr_lif"]     =     ELB_SOC_CSR_FILTER_ADDR_LIF_DATA_BYTE_ADDRESS;
    addr_labl["SOC"]["addr_lif"]     =    "ELB_SOC_CSR_FILTER_ADDR_LIF_DATA";
    addr_elen["SOC"]["addr_lif"]     = 4 / ELB_SOC_CSR_FILTER_ADDR_LIF_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SOC"]["src_id"]       =     ELB_SOC_CSR_FILTER_SRC_DATA_BYTE_ADDRESS;
    addr_labl["SOC"]["src_id"]       =    "ELB_SOC_CSR_FILTER_SRC_DATA";
    addr_elen["SOC"]["src_id"]       = 4 / ELB_SOC_CSR_FILTER_SRC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SOC"]["prot"]         =     ELB_SOC_CSR_FILTER_AXI_PROT_DATA_BYTE_ADDRESS;
    addr_labl["SOC"]["prot"]         =    "ELB_SOC_CSR_FILTER_AXI_PROT_DATA";
    addr_elen["SOC"]["prot"]         = 4 / ELB_SOC_CSR_FILTER_AXI_PROT_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SOC"]["cache"]        =     ELB_SOC_CSR_FILTER_AXI_CACHE_DATA_BYTE_ADDRESS;
    addr_labl["SOC"]["cache"]        =    "ELB_SOC_CSR_FILTER_AXI_CACHE_DATA";
    addr_elen["SOC"]["cache"]        = 4 / ELB_SOC_CSR_FILTER_AXI_CACHE_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SOC"]["acp"]          =     ELB_SOC_CSR_FILTER_ACP_DATA_BYTE_ADDRESS;
    addr_labl["SOC"]["acp"]          =    "ELB_SOC_CSR_FILTER_ACP_DATA";
    addr_elen["SOC"]["acp"]          = 4 / ELB_SOC_CSR_FILTER_ACP_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SOC"]["control"]      =     ELB_SOC_CSR_FILTER_ADDR_CTL_VALUE_BYTE_ADDRESS;
    addr_labl["SOC"]["control"]      =    "ELB_SOC_CSR_FILTER_ADDR_CTL_VALUE";
    addr_elen["SOC"]["control"]      = 4 / ELB_SOC_CSR_FILTER_ADDR_CTL_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["SOC"]["trace"]        =     ELB_SOC_CSR_FILTER_TRACE_BYTE_ADDRESS;
    addr_labl["SOC"]["trace"]        =    "ELB_SOC_CSR_FILTER_TRACE";
    addr_elen["SOC"]["trace"]        = 12;

    func_load["SOC"]["hit_count"]    =     load_fields_hit_count;
    func_load["SOC"]["addr_lo"]      =     load_fields_addr_lo;
    func_load["SOC"]["addr_hi"]      =     load_fields_addr_hi;
    func_load["SOC"]["addr_host"]    =     load_fields_addr_host;
    func_load["SOC"]["addr_pic"]     =     load_fields_addr_pic;
    func_load["SOC"]["addr_lif"]     =     load_fields_addr_lif;
    func_load["SOC"]["src_id"]       =     load_fields_src_id;
    func_load["SOC"]["prot"]         =     load_fields_prot;
    func_load["SOC"]["cache"]        =     load_fields_cache;
    func_load["SOC"]["acp"]          =     load_fields_acp;
    func_load["SOC"]["control"]      =     load_fields_control;
    func_load["SOC"]["trace"]        =     load_fields_trace;
    func_load["SOC"]["trace_status"] =     load_fields_trace_status;
    
    func_deposit["SOC"]["addr_lo"]      =     deposit_fields_addr_lo;
    func_deposit["SOC"]["addr_hi"]      =     deposit_fields_addr_hi;
    func_deposit["SOC"]["addr_host"]    =     deposit_fields_addr_host;
    func_deposit["SOC"]["addr_pic"]     =     deposit_fields_addr_pic;
    func_deposit["SOC"]["addr_lif"]     =     deposit_fields_addr_lif;
    func_deposit["SOC"]["src_id"]       =     deposit_fields_src_id;
    func_deposit["SOC"]["prot"]         =     deposit_fields_prot;
    func_deposit["SOC"]["cache"]        =     deposit_fields_cache;
    func_deposit["SOC"]["acp"]          =     deposit_fields_acp;
    func_deposit["SOC"]["control"]      =     deposit_fields_control;
    func_deposit["SOC"]["trace"]        =     deposit_fields_trace;
    
    helper["SOC"]["src_id"]["load_c0"]           =             ELB_SOC_CSR_FILTER_SRC_WIDTH / 2;
    helper["SOC"]["src_id"]["load_c1"]           = ~(-1ull << (ELB_SOC_CSR_FILTER_SRC_WIDTH / 2));

    
    //=======
    addr_base["WA"]                 =     ELB_ADDR_BASE_DB_WA_OFFSET;
    addr_ofst["WA"]["hit_count"]    =     ELB_WA_CSR_WAINVF_FILTER_HIT_COUNT_BYTE_ADDRESS;
    addr_labl["WA"]["hit_count"]    =    "ELB_WA_CSR_WAINVF_FILTER_HIT_COUNT";
    addr_elen["WA"]["hit_count"]    = 4 / ELB_WA_CSR_WAINVF_FILTER_HIT_COUNT_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["WA"]["addr_lo"]      =     ELB_WA_CSR_WAINVF_FILTER_ADDR_LO_DATA_BYTE_ADDRESS;
    addr_labl["WA"]["addr_lo"]      =    "ELB_WA_CSR_WAINVF_FILTER_ADDR_LO_DATA";
    addr_elen["WA"]["addr_lo"]      = 4 / ELB_WA_CSR_WAINVF_FILTER_ADDR_LO_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["WA"]["addr_hi"]      =     ELB_WA_CSR_WAINVF_FILTER_ADDR_HI_BYTE_ADDRESS;
    addr_labl["WA"]["addr_hi"]      =    "ELB_WA_CSR_WAINVF_FILTER_ADDR_HI_DATA";
    addr_elen["WA"]["addr_hi"]      = 4 / ELB_WA_CSR_WAINVF_FILTER_ADDR_HI_DATA_ARRAY_ELEMENT_SIZE;;
    addr_ofst["WA"]["addr_host"]    =     ELB_WA_CSR_WAINVF_FILTER_ADDR_HOST_DATA_BYTE_ADDRESS;
    addr_labl["WA"]["addr_host"]    =    "ELB_WA_CSR_WAINVF_FILTER_ADDR_HOST_DATA";
    addr_elen["WA"]["addr_host"]    = 4 / ELB_WA_CSR_WAINVF_FILTER_ADDR_HOST_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["WA"]["addr_pic"]     =     ELB_WA_CSR_WAINVF_FILTER_ADDR_PIC_DATA_BYTE_ADDRESS;
    addr_labl["WA"]["addr_pic"]     =    "ELB_WA_CSR_WAINVF_FILTER_ADDR_PIC_DATA";
    addr_elen["WA"]["addr_pic"]     = 4 / ELB_WA_CSR_WAINVF_FILTER_ADDR_PIC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["WA"]["addr_lif"]     =     ELB_WA_CSR_WAINVF_FILTER_ADDR_LIF_DATA_BYTE_ADDRESS;
    addr_labl["WA"]["addr_lif"]     =    "ELB_WA_CSR_WAINVF_FILTER_ADDR_LIF_DATA";
    addr_elen["WA"]["addr_lif"]     = 4 / ELB_WA_CSR_WAINVF_FILTER_ADDR_LIF_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["WA"]["src_id"]       =     ELB_WA_CSR_WAINVF_FILTER_SRC_DATA_BYTE_ADDRESS;
    addr_labl["WA"]["src_id"]       =    "ELB_WA_CSR_WAINVF_FILTER_SRC_DATA";
    addr_elen["WA"]["src_id"]       = 4 / ELB_WA_CSR_WAINVF_FILTER_SRC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["WA"]["prot"]         =     ELB_WA_CSR_WAINVF_FILTER_AXI_PROT_DATA_BYTE_ADDRESS;
    addr_labl["WA"]["prot"]         =    "ELB_WA_CSR_WAINVF_FILTER_AXI_PROT_DATA";
    addr_elen["WA"]["prot"]         = 4 / ELB_WA_CSR_WAINVF_FILTER_AXI_PROT_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["WA"]["cache"]        =     ELB_WA_CSR_WAINVF_FILTER_AXI_CACHE_DATA_BYTE_ADDRESS;
    addr_labl["WA"]["cache"]        =    "ELB_WA_CSR_WAINVF_FILTER_AXI_CACHE_DATA";
    addr_elen["WA"]["cache"]        = 4 / ELB_WA_CSR_WAINVF_FILTER_AXI_CACHE_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["WA"]["acp"]          =     ELB_WA_CSR_WAINVF_FILTER_ACP_DATA_BYTE_ADDRESS;
    addr_labl["WA"]["acp"]          =    "ELB_WA_CSR_WAINVF_FILTER_ACP_DATA";
    addr_elen["WA"]["acp"]          = 4 / ELB_WA_CSR_WAINVF_FILTER_ACP_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["WA"]["control"]      =     ELB_WA_CSR_WAINVF_FILTER_ADDR_CTL_VALUE_BYTE_ADDRESS;
    addr_labl["WA"]["control"]      =    "ELB_WA_CSR_WAINVF_FILTER_ADDR_CTL_VALUE";
    addr_elen["WA"]["control"]      = 4 / ELB_WA_CSR_WAINVF_FILTER_ADDR_CTL_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["WA"]["trace"]        =     ELB_WA_CSR_WAINVF_FILTER_TRACE_BYTE_ADDRESS;
    addr_labl["WA"]["trace"]        =    "ELB_WA_CSR_WAINVF_FILTER_TRACE";
    addr_elen["WA"]["trace"]        = 12;

    func_load["WA"]["hit_count"]    =     load_fields_hit_count;
    func_load["WA"]["addr_lo"]      =     load_fields_addr_lo;
    func_load["WA"]["addr_hi"]      =     load_fields_addr_hi;
    func_load["WA"]["addr_host"]    =     load_fields_addr_host;
    func_load["WA"]["addr_pic"]     =     load_fields_addr_pic;
    func_load["WA"]["addr_lif"]     =     load_fields_addr_lif;
    func_load["WA"]["src_id"]       =     load_fields_src_id;
    func_load["WA"]["prot"]         =     load_fields_prot;
    func_load["WA"]["cache"]        =     load_fields_cache;
    func_load["WA"]["acp"]          =     load_fields_acp;
    func_load["WA"]["control"]      =     load_fields_control;
    func_load["WA"]["trace"]        =     load_fields_trace;
    func_load["WA"]["trace_status"] =     load_fields_trace_status;
    
    func_deposit["WA"]["addr_lo"]      =     deposit_fields_addr_lo;
    func_deposit["WA"]["addr_hi"]      =     deposit_fields_addr_hi;
    func_deposit["WA"]["addr_host"]    =     deposit_fields_addr_host;
    func_deposit["WA"]["addr_pic"]     =     deposit_fields_addr_pic;
    func_deposit["WA"]["addr_lif"]     =     deposit_fields_addr_lif;
    func_deposit["WA"]["src_id"]       =     deposit_fields_src_id;
    func_deposit["WA"]["prot"]         =     deposit_fields_prot;
    func_deposit["WA"]["cache"]        =     deposit_fields_cache;
    func_deposit["WA"]["acp"]          =     deposit_fields_acp;
    func_deposit["WA"]["control"]      =     deposit_fields_control;
    func_deposit["WA"]["trace"]        =     deposit_fields_trace;
    
    helper["WA"]["src_id"]["load_c0"]           =             ELB_WAINVF_CSR_FILTER_SRC_WIDTH / 2;
    helper["WA"]["src_id"]["load_c1"]           = ~(-1ull << (ELB_WAINVF_CSR_FILTER_SRC_WIDTH / 2));


    
    //=======
    addr_base["TD"]                 =     ELB_ADDR_BASE_TD_PICS_OFFSET;
    addr_ofst["TD"]["hit_count"]    =     ELB_PICS_CSR_P4INVF_FILTER_HIT_COUNT_BYTE_ADDRESS;
    addr_labl["TD"]["hit_count"]    =    "ELB_PICS_CSR_P4INVF_FILTER_HIT_COUNT";
    addr_elen["TD"]["hit_count"]    = 4 / ELB_PICS_CSR_P4INVF_FILTER_HIT_COUNT_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["TD"]["addr_lo"]      =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_LO_DATA_BYTE_ADDRESS;
    addr_labl["TD"]["addr_lo"]      =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_LO_DATA";
    addr_elen["TD"]["addr_lo"]      = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_LO_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["TD"]["addr_hi"]      =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_HI_BYTE_ADDRESS;
    addr_labl["TD"]["addr_hi"]      =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_HI_DATA";
    addr_elen["TD"]["addr_hi"]      = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_HI_DATA_ARRAY_ELEMENT_SIZE;;
    addr_ofst["TD"]["addr_host"]    =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_HOST_DATA_BYTE_ADDRESS;
    addr_labl["TD"]["addr_host"]    =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_HOST_DATA";
    addr_elen["TD"]["addr_host"]    = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_HOST_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["TD"]["addr_pic"]     =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_PIC_DATA_BYTE_ADDRESS;
    addr_labl["TD"]["addr_pic"]     =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_PIC_DATA";
    addr_elen["TD"]["addr_pic"]     = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_PIC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["TD"]["addr_lif"]     =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_LIF_DATA_BYTE_ADDRESS;
    addr_labl["TD"]["addr_lif"]     =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_LIF_DATA";
    addr_elen["TD"]["addr_lif"]     = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_LIF_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["TD"]["src_id"]       =     ELB_PICS_CSR_P4INVF_FILTER_SRC_DATA_BYTE_ADDRESS;
    addr_labl["TD"]["src_id"]       =    "ELB_PICS_CSR_P4INVF_FILTER_SRC_DATA";
    addr_elen["TD"]["src_id"]       = 4 / ELB_PICS_CSR_P4INVF_FILTER_SRC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["TD"]["prot"]         =     ELB_PICS_CSR_P4INVF_FILTER_AXI_PROT_DATA_BYTE_ADDRESS;
    addr_labl["TD"]["prot"]         =    "ELB_PICS_CSR_P4INVF_FILTER_AXI_PROT_DATA";
    addr_elen["TD"]["prot"]         = 4 / ELB_PICS_CSR_P4INVF_FILTER_AXI_PROT_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["TD"]["cache"]        =     ELB_PICS_CSR_P4INVF_FILTER_AXI_CACHE_DATA_BYTE_ADDRESS;
    addr_labl["TD"]["cache"]        =    "ELB_PICS_CSR_P4INVF_FILTER_AXI_CACHE_DATA";
    addr_elen["TD"]["cache"]        = 4 / ELB_PICS_CSR_P4INVF_FILTER_AXI_CACHE_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["TD"]["acp"]          =     ELB_PICS_CSR_P4INVF_FILTER_ACP_DATA_BYTE_ADDRESS;
    addr_labl["TD"]["acp"]          =    "ELB_PICS_CSR_P4INVF_FILTER_ACP_DATA";
    addr_elen["TD"]["acp"]          = 4 / ELB_PICS_CSR_P4INVF_FILTER_ACP_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["TD"]["control"]      =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_CTL_VALUE_BYTE_ADDRESS;
    addr_labl["TD"]["control"]      =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_CTL_VALUE";
    addr_elen["TD"]["control"]      = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_CTL_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["TD"]["trace"]        =     ELB_PICS_CSR_P4INVF_FILTER_TRACE_BYTE_ADDRESS;
    addr_labl["TD"]["trace"]        =    "ELB_PICS_CSR_P4INVF_FILTER_TRACE";
    addr_elen["TD"]["trace"]        = 12;

    func_load["TD"]["hit_count"]    =     load_fields_hit_count;
    func_load["TD"]["addr_lo"]      =     load_fields_addr_lo;
    func_load["TD"]["addr_hi"]      =     load_fields_addr_hi;
    func_load["TD"]["addr_host"]    =     load_fields_addr_host;
    func_load["TD"]["addr_pic"]     =     load_fields_addr_pic;
    func_load["TD"]["addr_lif"]     =     load_fields_addr_lif;
    func_load["TD"]["src_id"]       =     load_fields_src_id;
    func_load["TD"]["prot"]         =     load_fields_prot;
    func_load["TD"]["cache"]        =     load_fields_cache;
    func_load["TD"]["acp"]          =     load_fields_acp;
    func_load["TD"]["control"]      =     load_fields_control;
    func_load["TD"]["trace"]        =     load_fields_trace;
    func_load["TD"]["trace_status"] =     load_fields_trace_status;
    
    func_deposit["TD"]["addr_lo"]      =     deposit_fields_addr_lo;
    func_deposit["TD"]["addr_hi"]      =     deposit_fields_addr_hi;
    func_deposit["TD"]["addr_host"]    =     deposit_fields_addr_host;
    func_deposit["TD"]["addr_pic"]     =     deposit_fields_addr_pic;
    func_deposit["TD"]["addr_lif"]     =     deposit_fields_addr_lif;
    func_deposit["TD"]["src_id"]       =     deposit_fields_src_id;
    func_deposit["TD"]["prot"]         =     deposit_fields_prot;
    func_deposit["TD"]["cache"]        =     deposit_fields_cache;
    func_deposit["TD"]["acp"]          =     deposit_fields_acp;
    func_deposit["TD"]["control"]      =     deposit_fields_control;
    func_deposit["TD"]["trace"]        =     deposit_fields_trace;
    
    
    helper["TD"]["src_id"]["load_c0"]           =             ELB_P4INVF_CSR_FILTER_SRC_WIDTH / 2;
    helper["TD"]["src_id"]["load_c1"]           = ~(-1ull << (ELB_P4INVF_CSR_FILTER_SRC_WIDTH / 2));


    
    //=======
    addr_base["RD"]                 =     ELB_ADDR_BASE_RD_PICS_OFFSET;
    addr_ofst["RD"]["hit_count"]    =     ELB_PICS_CSR_P4INVF_FILTER_HIT_COUNT_BYTE_ADDRESS;
    addr_labl["RD"]["hit_count"]    =    "ELB_PICS_CSR_P4INVF_FILTER_HIT_COUNT";
    addr_elen["RD"]["hit_count"]    = 4 / ELB_PICS_CSR_P4INVF_FILTER_HIT_COUNT_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["RD"]["addr_lo"]      =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_LO_DATA_BYTE_ADDRESS;
    addr_labl["RD"]["addr_lo"]      =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_LO_DATA";
    addr_elen["RD"]["addr_lo"]      = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_LO_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["RD"]["addr_hi"]      =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_HI_BYTE_ADDRESS;
    addr_labl["RD"]["addr_hi"]      =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_HI_DATA";
    addr_elen["RD"]["addr_hi"]      = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_HI_DATA_ARRAY_ELEMENT_SIZE;;
    addr_ofst["RD"]["addr_host"]    =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_HOST_DATA_BYTE_ADDRESS;
    addr_labl["RD"]["addr_host"]    =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_HOST_DATA";
    addr_elen["RD"]["addr_host"]    = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_HOST_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["RD"]["addr_pic"]     =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_PIC_DATA_BYTE_ADDRESS;
    addr_labl["RD"]["addr_pic"]     =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_PIC_DATA";
    addr_elen["RD"]["addr_pic"]     = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_PIC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["RD"]["addr_lif"]     =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_LIF_DATA_BYTE_ADDRESS;
    addr_labl["RD"]["addr_lif"]     =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_LIF_DATA";
    addr_elen["RD"]["addr_lif"]     = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_LIF_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["RD"]["src_id"]       =     ELB_PICS_CSR_P4INVF_FILTER_SRC_DATA_BYTE_ADDRESS;
    addr_labl["RD"]["src_id"]       =    "ELB_PICS_CSR_P4INVF_FILTER_SRC_DATA";
    addr_elen["RD"]["src_id"]       = 4 / ELB_PICS_CSR_P4INVF_FILTER_SRC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["RD"]["prot"]         =     ELB_PICS_CSR_P4INVF_FILTER_AXI_PROT_DATA_BYTE_ADDRESS;
    addr_labl["RD"]["prot"]         =    "ELB_PICS_CSR_P4INVF_FILTER_AXI_PROT_DATA";
    addr_elen["RD"]["prot"]         = 4 / ELB_PICS_CSR_P4INVF_FILTER_AXI_PROT_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["RD"]["cache"]        =     ELB_PICS_CSR_P4INVF_FILTER_AXI_CACHE_DATA_BYTE_ADDRESS;
    addr_labl["RD"]["cache"]        =    "ELB_PICS_CSR_P4INVF_FILTER_AXI_CACHE_DATA";
    addr_elen["RD"]["cache"]        = 4 / ELB_PICS_CSR_P4INVF_FILTER_AXI_CACHE_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["RD"]["acp"]          =     ELB_PICS_CSR_P4INVF_FILTER_ACP_DATA_BYTE_ADDRESS;
    addr_labl["RD"]["acp"]          =    "ELB_PICS_CSR_P4INVF_FILTER_ACP_DATA";
    addr_elen["RD"]["acp"]          = 4 / ELB_PICS_CSR_P4INVF_FILTER_ACP_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["RD"]["control"]      =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_CTL_VALUE_BYTE_ADDRESS;
    addr_labl["RD"]["control"]      =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_CTL_VALUE";
    addr_elen["RD"]["control"]      = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_CTL_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["RD"]["trace"]        =     ELB_PICS_CSR_P4INVF_FILTER_TRACE_BYTE_ADDRESS;
    addr_labl["RD"]["trace"]        =    "ELB_PICS_CSR_P4INVF_FILTER_TRACE";
    addr_elen["RD"]["trace"]        = 12;

    func_load["RD"]["hit_count"]    =     load_fields_hit_count;
    func_load["RD"]["addr_lo"]      =     load_fields_addr_lo;
    func_load["RD"]["addr_hi"]      =     load_fields_addr_hi;
    func_load["RD"]["addr_host"]    =     load_fields_addr_host;
    func_load["RD"]["addr_pic"]     =     load_fields_addr_pic;
    func_load["RD"]["addr_lif"]     =     load_fields_addr_lif;
    func_load["RD"]["src_id"]       =     load_fields_src_id;
    func_load["RD"]["prot"]         =     load_fields_prot;
    func_load["RD"]["cache"]        =     load_fields_cache;
    func_load["RD"]["acp"]          =     load_fields_acp;
    func_load["RD"]["control"]      =     load_fields_control;
    func_load["RD"]["trace"]        =     load_fields_trace;
    func_load["RD"]["trace_status"] =     load_fields_trace_status;
    
    func_deposit["RD"]["addr_lo"]      =     deposit_fields_addr_lo;
    func_deposit["RD"]["addr_hi"]      =     deposit_fields_addr_hi;
    func_deposit["RD"]["addr_host"]    =     deposit_fields_addr_host;
    func_deposit["RD"]["addr_pic"]     =     deposit_fields_addr_pic;
    func_deposit["RD"]["addr_lif"]     =     deposit_fields_addr_lif;
    func_deposit["RD"]["src_id"]       =     deposit_fields_src_id;
    func_deposit["RD"]["prot"]         =     deposit_fields_prot;
    func_deposit["RD"]["cache"]        =     deposit_fields_cache;
    func_deposit["RD"]["acp"]          =     deposit_fields_acp;
    func_deposit["RD"]["control"]      =     deposit_fields_control;
    func_deposit["RD"]["trace"]        =     deposit_fields_trace;
    
    helper["RD"]["src_id"]["load_c0"]           =             ELB_P4INVF_CSR_FILTER_SRC_WIDTH / 2;
    helper["RD"]["src_id"]["load_c1"]           = ~(-1ull << (ELB_P4INVF_CSR_FILTER_SRC_WIDTH / 2));


    
    //=======
    addr_base["SI"]                 =     ELB_ADDR_BASE_SSI_PICS_OFFSET;
    addr_ofst["SI"]["hit_count"]    =     ELB_PICS_CSR_P4INVF_FILTER_HIT_COUNT_BYTE_ADDRESS;
    addr_labl["SI"]["hit_count"]    =    "ELB_PICS_CSR_P4INVF_FILTER_HIT_COUNT";
    addr_elen["SI"]["hit_count"]    = 4 / ELB_PICS_CSR_P4INVF_FILTER_HIT_COUNT_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["SI"]["addr_lo"]      =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_LO_DATA_BYTE_ADDRESS;
    addr_labl["SI"]["addr_lo"]      =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_LO_DATA";
    addr_elen["SI"]["addr_lo"]      = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_LO_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SI"]["addr_hi"]      =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_HI_BYTE_ADDRESS;
    addr_labl["SI"]["addr_hi"]      =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_HI_DATA";
    addr_elen["SI"]["addr_hi"]      = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_HI_DATA_ARRAY_ELEMENT_SIZE;;
    addr_ofst["SI"]["addr_host"]    =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_HOST_DATA_BYTE_ADDRESS;
    addr_labl["SI"]["addr_host"]    =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_HOST_DATA";
    addr_elen["SI"]["addr_host"]    = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_HOST_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SI"]["addr_pic"]     =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_PIC_DATA_BYTE_ADDRESS;
    addr_labl["SI"]["addr_pic"]     =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_PIC_DATA";
    addr_elen["SI"]["addr_pic"]     = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_PIC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SI"]["addr_lif"]     =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_LIF_DATA_BYTE_ADDRESS;
    addr_labl["SI"]["addr_lif"]     =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_LIF_DATA";
    addr_elen["SI"]["addr_lif"]     = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_LIF_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SI"]["src_id"]       =     ELB_PICS_CSR_P4INVF_FILTER_SRC_DATA_BYTE_ADDRESS;
    addr_labl["SI"]["src_id"]       =    "ELB_PICS_CSR_P4INVF_FILTER_SRC_DATA";
    addr_elen["SI"]["src_id"]       = 4 / ELB_PICS_CSR_P4INVF_FILTER_SRC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SI"]["prot"]         =     ELB_PICS_CSR_P4INVF_FILTER_AXI_PROT_DATA_BYTE_ADDRESS;
    addr_labl["SI"]["prot"]         =    "ELB_PICS_CSR_P4INVF_FILTER_AXI_PROT_DATA";
    addr_elen["SI"]["prot"]         = 4 / ELB_PICS_CSR_P4INVF_FILTER_AXI_PROT_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SI"]["cache"]        =     ELB_PICS_CSR_P4INVF_FILTER_AXI_CACHE_DATA_BYTE_ADDRESS;
    addr_labl["SI"]["cache"]        =    "ELB_PICS_CSR_P4INVF_FILTER_AXI_CACHE_DATA";
    addr_elen["SI"]["cache"]        = 4 / ELB_PICS_CSR_P4INVF_FILTER_AXI_CACHE_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SI"]["acp"]          =     ELB_PICS_CSR_P4INVF_FILTER_ACP_DATA_BYTE_ADDRESS;
    addr_labl["SI"]["acp"]          =    "ELB_PICS_CSR_P4INVF_FILTER_ACP_DATA";
    addr_elen["SI"]["acp"]          = 4 / ELB_PICS_CSR_P4INVF_FILTER_ACP_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SI"]["control"]      =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_CTL_VALUE_BYTE_ADDRESS;
    addr_labl["SI"]["control"]      =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_CTL_VALUE";
    addr_elen["SI"]["control"]      = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_CTL_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["SI"]["trace"]        =     ELB_PICS_CSR_P4INVF_FILTER_TRACE_BYTE_ADDRESS;
    addr_labl["SI"]["trace"]        =    "ELB_PICS_CSR_P4INVF_FILTER_TRACE";
    addr_elen["SI"]["trace"]        = 12;

    func_load["SI"]["hit_count"]    =     load_fields_hit_count;
    func_load["SI"]["addr_lo"]      =     load_fields_addr_lo;
    func_load["SI"]["addr_hi"]      =     load_fields_addr_hi;
    func_load["SI"]["addr_host"]    =     load_fields_addr_host;
    func_load["SI"]["addr_pic"]     =     load_fields_addr_pic;
    func_load["SI"]["addr_lif"]     =     load_fields_addr_lif;
    func_load["SI"]["src_id"]       =     load_fields_src_id;
    func_load["SI"]["prot"]         =     load_fields_prot;
    func_load["SI"]["cache"]        =     load_fields_cache;
    func_load["SI"]["acp"]          =     load_fields_acp;
    func_load["SI"]["control"]      =     load_fields_control;
    func_load["SI"]["trace"]        =     load_fields_trace;
    func_load["SI"]["trace_status"] =     load_fields_trace_status;
    
    func_deposit["SI"]["addr_lo"]      =     deposit_fields_addr_lo;
    func_deposit["SI"]["addr_hi"]      =     deposit_fields_addr_hi;
    func_deposit["SI"]["addr_host"]    =     deposit_fields_addr_host;
    func_deposit["SI"]["addr_pic"]     =     deposit_fields_addr_pic;
    func_deposit["SI"]["addr_lif"]     =     deposit_fields_addr_lif;
    func_deposit["SI"]["src_id"]       =     deposit_fields_src_id;
    func_deposit["SI"]["prot"]         =     deposit_fields_prot;
    func_deposit["SI"]["cache"]        =     deposit_fields_cache;
    func_deposit["SI"]["acp"]          =     deposit_fields_acp;
    func_deposit["SI"]["control"]      =     deposit_fields_control;
    func_deposit["SI"]["trace"]        =     deposit_fields_trace;
    
    helper["SI"]["src_id"]["load_c0"]           =             ELB_P4INVF_CSR_FILTER_SRC_WIDTH / 2;
    helper["SI"]["src_id"]["load_c1"]           = ~(-1ull << (ELB_P4INVF_CSR_FILTER_SRC_WIDTH / 2));


    
    //=======
    addr_base["SE"]                 =     ELB_ADDR_BASE_SSE_PICS_OFFSET;
    addr_ofst["SE"]["hit_count"]    =     ELB_PICS_CSR_P4INVF_FILTER_HIT_COUNT_BYTE_ADDRESS;
    addr_labl["SE"]["hit_count"]    =    "ELB_PICS_CSR_P4INVF_FILTER_HIT_COUNT";
    addr_elen["SE"]["hit_count"]    = 4 / ELB_PICS_CSR_P4INVF_FILTER_HIT_COUNT_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["SE"]["addr_lo"]      =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_LO_DATA_BYTE_ADDRESS;
    addr_labl["SE"]["addr_lo"]      =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_LO_DATA";
    addr_elen["SE"]["addr_lo"]      = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_LO_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SE"]["addr_hi"]      =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_HI_BYTE_ADDRESS;
    addr_labl["SE"]["addr_hi"]      =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_HI_DATA";
    addr_elen["SE"]["addr_hi"]      = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_HI_DATA_ARRAY_ELEMENT_SIZE;;
    addr_ofst["SE"]["addr_host"]    =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_HOST_DATA_BYTE_ADDRESS;
    addr_labl["SE"]["addr_host"]    =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_HOST_DATA";
    addr_elen["SE"]["addr_host"]    = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_HOST_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SE"]["addr_pic"]     =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_PIC_DATA_BYTE_ADDRESS;
    addr_labl["SE"]["addr_pic"]     =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_PIC_DATA";
    addr_elen["SE"]["addr_pic"]     = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_PIC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SE"]["addr_lif"]     =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_LIF_DATA_BYTE_ADDRESS;
    addr_labl["SE"]["addr_lif"]     =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_LIF_DATA";
    addr_elen["SE"]["addr_lif"]     = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_LIF_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SE"]["src_id"]       =     ELB_PICS_CSR_P4INVF_FILTER_SRC_DATA_BYTE_ADDRESS;
    addr_labl["SE"]["src_id"]       =    "ELB_PICS_CSR_P4INVF_FILTER_SRC_DATA";
    addr_elen["SE"]["src_id"]       = 4 / ELB_PICS_CSR_P4INVF_FILTER_SRC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SE"]["prot"]         =     ELB_PICS_CSR_P4INVF_FILTER_AXI_PROT_DATA_BYTE_ADDRESS;
    addr_labl["SE"]["prot"]         =    "ELB_PICS_CSR_P4INVF_FILTER_AXI_PROT_DATA";
    addr_elen["SE"]["prot"]         = 4 / ELB_PICS_CSR_P4INVF_FILTER_AXI_PROT_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SE"]["cache"]        =     ELB_PICS_CSR_P4INVF_FILTER_AXI_CACHE_DATA_BYTE_ADDRESS;
    addr_labl["SE"]["cache"]        =    "ELB_PICS_CSR_P4INVF_FILTER_AXI_CACHE_DATA";
    addr_elen["SE"]["cache"]        = 4 / ELB_PICS_CSR_P4INVF_FILTER_AXI_CACHE_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SE"]["acp"]          =     ELB_PICS_CSR_P4INVF_FILTER_ACP_DATA_BYTE_ADDRESS;
    addr_labl["SE"]["acp"]          =    "ELB_PICS_CSR_P4INVF_FILTER_ACP_DATA";
    addr_elen["SE"]["acp"]          = 4 / ELB_PICS_CSR_P4INVF_FILTER_ACP_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["SE"]["control"]      =     ELB_PICS_CSR_P4INVF_FILTER_ADDR_CTL_VALUE_BYTE_ADDRESS;
    addr_labl["SE"]["control"]      =    "ELB_PICS_CSR_P4INVF_FILTER_ADDR_CTL_VALUE";
    addr_elen["SE"]["control"]      = 4 / ELB_PICS_CSR_P4INVF_FILTER_ADDR_CTL_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["SE"]["trace"]        =     ELB_PICS_CSR_P4INVF_FILTER_TRACE_BYTE_ADDRESS;
    addr_labl["SE"]["trace"]        =    "ELB_PICS_CSR_P4INVF_FILTER_TRACE";
    addr_elen["SE"]["trace"]        = 12;

    func_load["SE"]["hit_count"]    =     load_fields_hit_count;
    func_load["SE"]["addr_lo"]      =     load_fields_addr_lo;
    func_load["SE"]["addr_hi"]      =     load_fields_addr_hi;
    func_load["SE"]["addr_host"]    =     load_fields_addr_host;
    func_load["SE"]["addr_pic"]     =     load_fields_addr_pic;
    func_load["SE"]["addr_lif"]     =     load_fields_addr_lif;
    func_load["SE"]["src_id"]       =     load_fields_src_id;
    func_load["SE"]["prot"]         =     load_fields_prot;
    func_load["SE"]["cache"]        =     load_fields_cache;
    func_load["SE"]["acp"]          =     load_fields_acp;
    func_load["SE"]["control"]      =     load_fields_control;
    func_load["SE"]["trace"]        =     load_fields_trace;
    func_load["SE"]["trace_status"] =     load_fields_trace_status;
    
    func_deposit["SE"]["addr_lo"]      =     deposit_fields_addr_lo;
    func_deposit["SE"]["addr_hi"]      =     deposit_fields_addr_hi;
    func_deposit["SE"]["addr_host"]    =     deposit_fields_addr_host;
    func_deposit["SE"]["addr_pic"]     =     deposit_fields_addr_pic;
    func_deposit["SE"]["addr_lif"]     =     deposit_fields_addr_lif;
    func_deposit["SE"]["src_id"]       =     deposit_fields_src_id;
    func_deposit["SE"]["prot"]         =     deposit_fields_prot;
    func_deposit["SE"]["cache"]        =     deposit_fields_cache;
    func_deposit["SE"]["acp"]          =     deposit_fields_acp;
    func_deposit["SE"]["control"]      =     deposit_fields_control;
    func_deposit["SE"]["trace"]        =     deposit_fields_trace;
    
    helper["SE"]["src_id"]["load_c0"]           =             ELB_P4INVF_CSR_FILTER_SRC_WIDTH / 2;
    helper["SE"]["src_id"]["load_c1"]           = ~(-1ull << (ELB_P4INVF_CSR_FILTER_SRC_WIDTH / 2));


    
    //=======
    addr_base["XD"]                 =     ELB_ADDR_BASE_XD_XD_OFFSET;
    addr_ofst["XD"]["hit_count"]    =     ELB_XD_CSR_P4INVF_FILTER_HIT_COUNT_BYTE_ADDRESS;
    addr_labl["XD"]["hit_count"]    =    "ELB_XD_CSR_P4INVF_FILTER_HIT_COUNT";
    addr_elen["XD"]["hit_count"]    = 4 / ELB_XD_CSR_P4INVF_FILTER_HIT_COUNT_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["XD"]["addr_lo"]      =     ELB_XD_CSR_P4INVF_FILTER_ADDR_LO_DATA_BYTE_ADDRESS;
    addr_labl["XD"]["addr_lo"]      =    "ELB_XD_CSR_P4INVF_FILTER_ADDR_LO_DATA";
    addr_elen["XD"]["addr_lo"]      = 4 / ELB_XD_CSR_P4INVF_FILTER_ADDR_LO_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["XD"]["addr_hi"]      =     ELB_XD_CSR_P4INVF_FILTER_ADDR_HI_BYTE_ADDRESS;
    addr_labl["XD"]["addr_hi"]      =    "ELB_XD_CSR_P4INVF_FILTER_ADDR_HI_DATA";
    addr_elen["XD"]["addr_hi"]      = 4 / ELB_XD_CSR_P4INVF_FILTER_ADDR_HI_DATA_ARRAY_ELEMENT_SIZE;;
    addr_ofst["XD"]["addr_host"]    =     ELB_XD_CSR_P4INVF_FILTER_ADDR_HOST_DATA_BYTE_ADDRESS;
    addr_labl["XD"]["addr_host"]    =    "ELB_XD_CSR_P4INVF_FILTER_ADDR_HOST_DATA";
    addr_elen["XD"]["addr_host"]    = 4 / ELB_XD_CSR_P4INVF_FILTER_ADDR_HOST_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["XD"]["addr_pic"]     =     ELB_XD_CSR_P4INVF_FILTER_ADDR_PIC_DATA_BYTE_ADDRESS;
    addr_labl["XD"]["addr_pic"]     =    "ELB_XD_CSR_P4INVF_FILTER_ADDR_PIC_DATA";
    addr_elen["XD"]["addr_pic"]     = 4 / ELB_XD_CSR_P4INVF_FILTER_ADDR_PIC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["XD"]["addr_lif"]     =     ELB_XD_CSR_P4INVF_FILTER_ADDR_LIF_DATA_BYTE_ADDRESS;
    addr_labl["XD"]["addr_lif"]     =    "ELB_XD_CSR_P4INVF_FILTER_ADDR_LIF_DATA";
    addr_elen["XD"]["addr_lif"]     = 4 / ELB_XD_CSR_P4INVF_FILTER_ADDR_LIF_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["XD"]["src_id"]       =     ELB_XD_CSR_P4INVF_FILTER_SRC_DATA_BYTE_ADDRESS;
    addr_labl["XD"]["src_id"]       =    "ELB_XD_CSR_P4INVF_FILTER_SRC_DATA";
    addr_elen["XD"]["src_id"]       = 4 / ELB_XD_CSR_P4INVF_FILTER_SRC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["XD"]["prot"]         =     ELB_XD_CSR_P4INVF_FILTER_AXI_PROT_DATA_BYTE_ADDRESS;
    addr_labl["XD"]["prot"]         =    "ELB_XD_CSR_P4INVF_FILTER_AXI_PROT_DATA";
    addr_elen["XD"]["prot"]         = 4 / ELB_XD_CSR_P4INVF_FILTER_AXI_PROT_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["XD"]["cache"]        =     ELB_XD_CSR_P4INVF_FILTER_AXI_CACHE_DATA_BYTE_ADDRESS;
    addr_labl["XD"]["cache"]        =    "ELB_XD_CSR_P4INVF_FILTER_AXI_CACHE_DATA";
    addr_elen["XD"]["cache"]        = 4 / ELB_XD_CSR_P4INVF_FILTER_AXI_CACHE_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["XD"]["acp"]          =     ELB_XD_CSR_P4INVF_FILTER_ACP_DATA_BYTE_ADDRESS;
    addr_labl["XD"]["acp"]          =    "ELB_XD_CSR_P4INVF_FILTER_ACP_DATA";
    addr_elen["XD"]["acp"]          = 4 / ELB_XD_CSR_P4INVF_FILTER_ACP_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["XD"]["control"]      =     ELB_XD_CSR_P4INVF_FILTER_ADDR_CTL_VALUE_BYTE_ADDRESS;
    addr_labl["XD"]["control"]      =    "ELB_XD_CSR_P4INVF_FILTER_ADDR_CTL_VALUE";
    addr_elen["XD"]["control"]      = 4 / ELB_XD_CSR_P4INVF_FILTER_ADDR_CTL_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["XD"]["trace"]        =     ELB_XD_CSR_P4INVF_FILTER_TRACE_BYTE_ADDRESS;
    addr_labl["XD"]["trace"]        =    "ELB_XD_CSR_P4INVF_FILTER_TRACE";
    addr_elen["XD"]["trace"]        = 12;

    func_load["XD"]["hit_count"]    =     load_fields_hit_count;
    func_load["XD"]["addr_lo"]      =     load_fields_addr_lo;
    func_load["XD"]["addr_hi"]      =     load_fields_addr_hi;
    func_load["XD"]["addr_host"]    =     load_fields_addr_host;
    func_load["XD"]["addr_pic"]     =     load_fields_addr_pic;
    func_load["XD"]["addr_lif"]     =     load_fields_addr_lif;
    func_load["XD"]["src_id"]       =     load_fields_src_id;
    func_load["XD"]["prot"]         =     load_fields_prot;
    func_load["XD"]["cache"]        =     load_fields_cache;
    func_load["XD"]["acp"]          =     load_fields_acp;
    func_load["XD"]["control"]      =     load_fields_control;
    func_load["XD"]["trace"]        =     load_fields_trace;
    func_load["XD"]["trace_status"] =     load_fields_trace_status;
    
    func_deposit["XD"]["addr_lo"]      =     deposit_fields_addr_lo;
    func_deposit["XD"]["addr_hi"]      =     deposit_fields_addr_hi;
    func_deposit["XD"]["addr_host"]    =     deposit_fields_addr_host;
    func_deposit["XD"]["addr_pic"]     =     deposit_fields_addr_pic;
    func_deposit["XD"]["addr_lif"]     =     deposit_fields_addr_lif;
    func_deposit["XD"]["src_id"]       =     deposit_fields_src_id;
    func_deposit["XD"]["prot"]         =     deposit_fields_prot;
    func_deposit["XD"]["cache"]        =     deposit_fields_cache;
    func_deposit["XD"]["acp"]          =     deposit_fields_acp;
    func_deposit["XD"]["control"]      =     deposit_fields_control;
    func_deposit["XD"]["trace"]        =     deposit_fields_trace;
    
    helper["XD"]["src_id"]["load_c0"]           =             ELB_XDP4INVF_CSR_FILTER_SRC_WIDTH / 2;
    helper["XD"]["src_id"]["load_c1"]           = ~(-1ull << (ELB_XDP4INVF_CSR_FILTER_SRC_WIDTH / 2));


    
    //=======
    addr_base["MD0"]                 =     ELB_ADDR_BASE_MD_HESE_OFFSET;
    addr_ofst["MD0"]["hit_count"]    =     ELB_HESE_CSR_MDINVF0_FILTER_HIT_COUNT_BYTE_ADDRESS;
    addr_labl["MD0"]["hit_count"]    =    "ELB_HESE_CSR_MDINVF0_FILTER_HIT_COUNT";
    addr_elen["MD0"]["hit_count"]    = 4 / ELB_HESE_CSR_MDINVF0_FILTER_HIT_COUNT_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD0"]["addr_lo"]      =     ELB_HESE_CSR_MDINVF0_FILTER_ADDR_LO_DATA_BYTE_ADDRESS;
    addr_labl["MD0"]["addr_lo"]      =    "ELB_HESE_CSR_MDINVF0_FILTER_ADDR_LO_DATA";
    addr_elen["MD0"]["addr_lo"]      = 4 / ELB_HESE_CSR_MDINVF0_FILTER_ADDR_LO_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD0"]["addr_hi"]      =     ELB_HESE_CSR_MDINVF0_FILTER_ADDR_HI_BYTE_ADDRESS;
    addr_labl["MD0"]["addr_hi"]      =    "ELB_HESE_CSR_MDINVF0_FILTER_ADDR_HI_DATA";
    addr_elen["MD0"]["addr_hi"]      = 4 / ELB_HESE_CSR_MDINVF0_FILTER_ADDR_HI_DATA_ARRAY_ELEMENT_SIZE;;
    addr_ofst["MD0"]["addr_host"]    =     ELB_HESE_CSR_MDINVF0_FILTER_ADDR_HOST_DATA_BYTE_ADDRESS;
    addr_labl["MD0"]["addr_host"]    =    "ELB_HESE_CSR_MDINVF0_FILTER_ADDR_HOST_DATA";
    addr_elen["MD0"]["addr_host"]    = 4 / ELB_HESE_CSR_MDINVF0_FILTER_ADDR_HOST_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD0"]["addr_pic"]     =     ELB_HESE_CSR_MDINVF0_FILTER_ADDR_PIC_DATA_BYTE_ADDRESS;
    addr_labl["MD0"]["addr_pic"]     =    "ELB_HESE_CSR_MDINVF0_FILTER_ADDR_PIC_DATA";
    addr_elen["MD0"]["addr_pic"]     = 4 / ELB_HESE_CSR_MDINVF0_FILTER_ADDR_PIC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD0"]["addr_lif"]     =     ELB_HESE_CSR_MDINVF0_FILTER_ADDR_LIF_DATA_BYTE_ADDRESS;
    addr_labl["MD0"]["addr_lif"]     =    "ELB_HESE_CSR_MDINVF0_FILTER_ADDR_LIF_DATA";
    addr_elen["MD0"]["addr_lif"]     = 4 / ELB_HESE_CSR_MDINVF0_FILTER_ADDR_LIF_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD0"]["src_id"]       =     ELB_HESE_CSR_MDINVF0_FILTER_SRC_DATA_BYTE_ADDRESS;
    addr_labl["MD0"]["src_id"]       =    "ELB_HESE_CSR_MDINVF0_FILTER_SRC_DATA";
    addr_elen["MD0"]["src_id"]       = 4 / ELB_HESE_CSR_MDINVF0_FILTER_SRC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD0"]["prot"]         =     ELB_HESE_CSR_MDINVF0_FILTER_AXI_PROT_DATA_BYTE_ADDRESS;
    addr_labl["MD0"]["prot"]         =    "ELB_HESE_CSR_MDINVF0_FILTER_AXI_PROT_DATA";
    addr_elen["MD0"]["prot"]         = 4 / ELB_HESE_CSR_MDINVF0_FILTER_AXI_PROT_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD0"]["cache"]        =     ELB_HESE_CSR_MDINVF0_FILTER_AXI_CACHE_DATA_BYTE_ADDRESS;
    addr_labl["MD0"]["cache"]        =    "ELB_HESE_CSR_MDINVF0_FILTER_AXI_CACHE_DATA";
    addr_elen["MD0"]["cache"]        = 4 / ELB_HESE_CSR_MDINVF0_FILTER_AXI_CACHE_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD0"]["acp"]          =     ELB_HESE_CSR_MDINVF0_FILTER_ACP_DATA_BYTE_ADDRESS;
    addr_labl["MD0"]["acp"]          =    "ELB_HESE_CSR_MDINVF0_FILTER_ACP_DATA";
    addr_elen["MD0"]["acp"]          = 4 / ELB_HESE_CSR_MDINVF0_FILTER_ACP_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD0"]["control"]      =     ELB_HESE_CSR_MDINVF0_FILTER_ADDR_CTL_VALUE_BYTE_ADDRESS;
    addr_labl["MD0"]["control"]      =    "ELB_HESE_CSR_MDINVF0_FILTER_ADDR_CTL_VALUE";
    addr_elen["MD0"]["control"]      = 4 / ELB_HESE_CSR_MDINVF0_FILTER_ADDR_CTL_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD0"]["trace"]        =     ELB_HESE_CSR_MDINVF0_FILTER_TRACE_BYTE_ADDRESS;
    addr_labl["MD0"]["trace"]        =    "ELB_HESE_CSR_MDINVF0_FILTER_TRACE";
    addr_elen["MD0"]["trace"]        = 12;

    func_load["MD0"]["hit_count"]    =     load_fields_hit_count;
    func_load["MD0"]["addr_lo"]      =     load_fields_addr_lo;
    func_load["MD0"]["addr_hi"]      =     load_fields_addr_hi;
    func_load["MD0"]["addr_host"]    =     load_fields_addr_host;
    func_load["MD0"]["addr_pic"]     =     load_fields_addr_pic;
    func_load["MD0"]["addr_lif"]     =     load_fields_addr_lif;
    func_load["MD0"]["src_id"]       =     load_fields_src_id;
    func_load["MD0"]["prot"]         =     load_fields_prot;
    func_load["MD0"]["cache"]        =     load_fields_cache;
    func_load["MD0"]["acp"]          =     load_fields_acp;
    func_load["MD0"]["control"]      =     load_fields_control;
    func_load["MD0"]["trace"]        =     load_fields_trace;
    func_load["MD0"]["trace_status"] =     load_fields_trace_status;
    
    func_deposit["MD0"]["addr_lo"]      =     deposit_fields_addr_lo;
    func_deposit["MD0"]["addr_hi"]      =     deposit_fields_addr_hi;
    func_deposit["MD0"]["addr_host"]    =     deposit_fields_addr_host;
    func_deposit["MD0"]["addr_pic"]     =     deposit_fields_addr_pic;
    func_deposit["MD0"]["addr_lif"]     =     deposit_fields_addr_lif;
    func_deposit["MD0"]["src_id"]       =     deposit_fields_src_id;
    func_deposit["MD0"]["prot"]         =     deposit_fields_prot;
    func_deposit["MD0"]["cache"]        =     deposit_fields_cache;
    func_deposit["MD0"]["acp"]          =     deposit_fields_acp;
    func_deposit["MD0"]["control"]      =     deposit_fields_control;
    func_deposit["MD0"]["trace"]        =     deposit_fields_trace;
    
    helper["MD0"]["src_id"]["load_c0"]           =             ELB_MDINVF_CSR_FILTER_SRC_WIDTH / 2;
    helper["MD0"]["src_id"]["load_c1"]           = ~(-1ull << (ELB_MDINVF_CSR_FILTER_SRC_WIDTH / 2));



    //=======
    addr_base["MD1"]                 =     ELB_ADDR_BASE_MD_HESE_OFFSET;
    addr_ofst["MD1"]["hit_count"]    =     ELB_HESE_CSR_MDINVF1_FILTER_HIT_COUNT_BYTE_ADDRESS;
    addr_labl["MD1"]["hit_count"]    =    "ELB_HESE_CSR_MDINVF1_FILTER_HIT_COUNT";
    addr_elen["MD1"]["hit_count"]    = 4 / ELB_HESE_CSR_MDINVF1_FILTER_HIT_COUNT_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD1"]["addr_lo"]      =     ELB_HESE_CSR_MDINVF1_FILTER_ADDR_LO_DATA_BYTE_ADDRESS;
    addr_labl["MD1"]["addr_lo"]      =    "ELB_HESE_CSR_MDINVF1_FILTER_ADDR_LO_DATA";
    addr_elen["MD1"]["addr_lo"]      = 4 / ELB_HESE_CSR_MDINVF1_FILTER_ADDR_LO_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD1"]["addr_hi"]      =     ELB_HESE_CSR_MDINVF1_FILTER_ADDR_HI_BYTE_ADDRESS;
    addr_labl["MD1"]["addr_hi"]      =    "ELB_HESE_CSR_MDINVF1_FILTER_ADDR_HI_DATA";
    addr_elen["MD1"]["addr_hi"]      = 4 / ELB_HESE_CSR_MDINVF1_FILTER_ADDR_HI_DATA_ARRAY_ELEMENT_SIZE;;
    addr_ofst["MD1"]["addr_host"]    =     ELB_HESE_CSR_MDINVF1_FILTER_ADDR_HOST_DATA_BYTE_ADDRESS;
    addr_labl["MD1"]["addr_host"]    =    "ELB_HESE_CSR_MDINVF1_FILTER_ADDR_HOST_DATA";
    addr_elen["MD1"]["addr_host"]    = 4 / ELB_HESE_CSR_MDINVF1_FILTER_ADDR_HOST_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD1"]["addr_pic"]     =     ELB_HESE_CSR_MDINVF1_FILTER_ADDR_PIC_DATA_BYTE_ADDRESS;
    addr_labl["MD1"]["addr_pic"]     =    "ELB_HESE_CSR_MDINVF1_FILTER_ADDR_PIC_DATA";
    addr_elen["MD1"]["addr_pic"]     = 4 / ELB_HESE_CSR_MDINVF1_FILTER_ADDR_PIC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD1"]["addr_lif"]     =     ELB_HESE_CSR_MDINVF1_FILTER_ADDR_LIF_DATA_BYTE_ADDRESS;
    addr_labl["MD1"]["addr_lif"]     =    "ELB_HESE_CSR_MDINVF1_FILTER_ADDR_LIF_DATA";
    addr_elen["MD1"]["addr_lif"]     = 4 / ELB_HESE_CSR_MDINVF1_FILTER_ADDR_LIF_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD1"]["src_id"]       =     ELB_HESE_CSR_MDINVF1_FILTER_SRC_DATA_BYTE_ADDRESS;
    addr_labl["MD1"]["src_id"]       =    "ELB_HESE_CSR_MDINVF1_FILTER_SRC_DATA";
    addr_elen["MD1"]["src_id"]       = 4 / ELB_HESE_CSR_MDINVF1_FILTER_SRC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD1"]["prot"]         =     ELB_HESE_CSR_MDINVF1_FILTER_AXI_PROT_DATA_BYTE_ADDRESS;
    addr_labl["MD1"]["prot"]         =    "ELB_HESE_CSR_MDINVF1_FILTER_AXI_PROT_DATA";
    addr_elen["MD1"]["prot"]         = 4 / ELB_HESE_CSR_MDINVF1_FILTER_AXI_PROT_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD1"]["cache"]        =     ELB_HESE_CSR_MDINVF1_FILTER_AXI_CACHE_DATA_BYTE_ADDRESS;
    addr_labl["MD1"]["cache"]        =    "ELB_HESE_CSR_MDINVF1_FILTER_AXI_CACHE_DATA";
    addr_elen["MD1"]["cache"]        = 4 / ELB_HESE_CSR_MDINVF1_FILTER_AXI_CACHE_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD1"]["acp"]          =     ELB_HESE_CSR_MDINVF1_FILTER_ACP_DATA_BYTE_ADDRESS;
    addr_labl["MD1"]["acp"]          =    "ELB_HESE_CSR_MDINVF1_FILTER_ACP_DATA";
    addr_elen["MD1"]["acp"]          = 4 / ELB_HESE_CSR_MDINVF1_FILTER_ACP_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD1"]["control"]      =     ELB_HESE_CSR_MDINVF1_FILTER_ADDR_CTL_VALUE_BYTE_ADDRESS;
    addr_labl["MD1"]["control"]      =    "ELB_HESE_CSR_MDINVF1_FILTER_ADDR_CTL_VALUE";
    addr_elen["MD1"]["control"]      = 4 / ELB_HESE_CSR_MDINVF1_FILTER_ADDR_CTL_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["MD1"]["trace"]        =     ELB_HESE_CSR_MDINVF1_FILTER_TRACE_BYTE_ADDRESS;
    addr_labl["MD1"]["trace"]        =    "ELB_HESE_CSR_MDINVF1_FILTER_TRACE";
    addr_elen["MD1"]["trace"]        = 12;

    func_load["MD1"]["hit_count"]    =     load_fields_hit_count;
    func_load["MD1"]["addr_lo"]      =     load_fields_addr_lo;
    func_load["MD1"]["addr_hi"]      =     load_fields_addr_hi;
    func_load["MD1"]["addr_host"]    =     load_fields_addr_host;
    func_load["MD1"]["addr_pic"]     =     load_fields_addr_pic;
    func_load["MD1"]["addr_lif"]     =     load_fields_addr_lif;
    func_load["MD1"]["src_id"]       =     load_fields_src_id;
    func_load["MD1"]["prot"]         =     load_fields_prot;
    func_load["MD1"]["cache"]        =     load_fields_cache;
    func_load["MD1"]["acp"]          =     load_fields_acp;
    func_load["MD1"]["control"]      =     load_fields_control;
    func_load["MD1"]["trace"]        =     load_fields_trace;
    func_load["MD1"]["trace_status"] =     load_fields_trace_status;
    
    func_deposit["MD1"]["addr_lo"]      =     deposit_fields_addr_lo;
    func_deposit["MD1"]["addr_hi"]      =     deposit_fields_addr_hi;
    func_deposit["MD1"]["addr_host"]    =     deposit_fields_addr_host;
    func_deposit["MD1"]["addr_pic"]     =     deposit_fields_addr_pic;
    func_deposit["MD1"]["addr_lif"]     =     deposit_fields_addr_lif;
    func_deposit["MD1"]["src_id"]       =     deposit_fields_src_id;
    func_deposit["MD1"]["prot"]         =     deposit_fields_prot;
    func_deposit["MD1"]["cache"]        =     deposit_fields_cache;
    func_deposit["MD1"]["acp"]          =     deposit_fields_acp;
    func_deposit["MD1"]["control"]      =     deposit_fields_control;
    func_deposit["MD1"]["trace"]        =     deposit_fields_trace;
    
    helper["MD1"]["src_id"]["load_c0"]           =             ELB_MDINVF_CSR_FILTER_SRC_WIDTH / 2;
    helper["MD1"]["src_id"]["load_c1"]           = ~(-1ull << (ELB_MDINVF_CSR_FILTER_SRC_WIDTH / 2));


    
    //=======
    addr_base["PB"]                 =     ELB_ADDR_BASE_PB_PBC_OFFSET;
    addr_ofst["PB"]["hit_count"]    =     ELB_PBC_CSR_AXI_FILTER_FILTER_HIT_COUNT_BYTE_ADDRESS;
    addr_labl["PB"]["hit_count"]    =    "ELB_PBC_CSR_AXI_FILTER_FILTER_HIT_COUNT";
    addr_elen["PB"]["hit_count"]    = 4 / ELB_PBC_CSR_AXI_FILTER_FILTER_HIT_COUNT_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["PB"]["addr_lo"]      =     ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_LO_DATA_BYTE_ADDRESS;
    addr_labl["PB"]["addr_lo"]      =    "ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_LO_DATA";
    addr_elen["PB"]["addr_lo"]      = 4 / ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_LO_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["PB"]["addr_hi"]      =     ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_HI_BYTE_ADDRESS;
    addr_labl["PB"]["addr_hi"]      =    "ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_HI_DATA";
    addr_elen["PB"]["addr_hi"]      = 4 / ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_HI_DATA_ARRAY_ELEMENT_SIZE;;
    addr_ofst["PB"]["addr_host"]    =     ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_HOST_DATA_BYTE_ADDRESS;
    addr_labl["PB"]["addr_host"]    =    "ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_HOST_DATA";
    addr_elen["PB"]["addr_host"]    = 4 / ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_HOST_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["PB"]["addr_pic"]     =     ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_PIC_DATA_BYTE_ADDRESS;
    addr_labl["PB"]["addr_pic"]     =    "ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_PIC_DATA";
    addr_elen["PB"]["addr_pic"]     = 4 / ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_PIC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["PB"]["addr_lif"]     =     ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_LIF_DATA_BYTE_ADDRESS;
    addr_labl["PB"]["addr_lif"]     =    "ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_LIF_DATA";
    addr_elen["PB"]["addr_lif"]     = 4 / ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_LIF_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["PB"]["src_id"]       =     ELB_PBC_CSR_AXI_FILTER_FILTER_SRC_DATA_BYTE_ADDRESS;
    addr_labl["PB"]["src_id"]       =    "ELB_PBC_CSR_AXI_FILTER_FILTER_SRC_DATA";
    addr_elen["PB"]["src_id"]       = 4 / ELB_PBC_CSR_AXI_FILTER_FILTER_SRC_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["PB"]["prot"]         =     ELB_PBC_CSR_AXI_FILTER_FILTER_AXI_PROT_DATA_BYTE_ADDRESS;
    addr_labl["PB"]["prot"]         =    "ELB_PBC_CSR_AXI_FILTER_FILTER_AXI_PROT_DATA";
    addr_elen["PB"]["prot"]         = 4 / ELB_PBC_CSR_AXI_FILTER_FILTER_AXI_PROT_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["PB"]["cache"]        =     ELB_PBC_CSR_AXI_FILTER_FILTER_AXI_CACHE_DATA_BYTE_ADDRESS;
    addr_labl["PB"]["cache"]        =    "ELB_PBC_CSR_AXI_FILTER_FILTER_AXI_CACHE_DATA";
    addr_elen["PB"]["cache"]        = 4 / ELB_PBC_CSR_AXI_FILTER_FILTER_AXI_CACHE_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["PB"]["acp"]          =     ELB_PBC_CSR_AXI_FILTER_FILTER_ACP_DATA_BYTE_ADDRESS;
    addr_labl["PB"]["acp"]          =    "ELB_PBC_CSR_AXI_FILTER_FILTER_ACP_DATA";
    addr_elen["PB"]["acp"]          = 4 / ELB_PBC_CSR_AXI_FILTER_FILTER_ACP_DATA_ARRAY_ELEMENT_SIZE;
    addr_ofst["PB"]["control"]      =     ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_CTL_VALUE_BYTE_ADDRESS;
    addr_labl["PB"]["control"]      =    "ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_CTL_VALUE";
    addr_elen["PB"]["control"]      = 4 / ELB_PBC_CSR_AXI_FILTER_FILTER_ADDR_CTL_VALUE_ARRAY_ELEMENT_SIZE;
    addr_ofst["PB"]["trace"]        =     ELB_PBC_CSR_AXI_FILTER_FILTER_TRACE_BYTE_ADDRESS;
    addr_labl["PB"]["trace"]        =    "ELB_PBC_CSR_AXI_FILTER_FILTER_TRACE";
    addr_elen["PB"]["trace"]        = 12;

    func_load["PB"]["hit_count"]    =     load_fields_hit_count;
    func_load["PB"]["addr_lo"]      =     load_fields_addr_lo;
    func_load["PB"]["addr_hi"]      =     load_fields_addr_hi;
    func_load["PB"]["addr_host"]    =     load_fields_addr_host;
    func_load["PB"]["addr_pic"]     =     load_fields_addr_pic;
    func_load["PB"]["addr_lif"]     =     load_fields_addr_lif;
    func_load["PB"]["src_id"]       =     load_fields_src_id;
    func_load["PB"]["prot"]         =     load_fields_prot;
    func_load["PB"]["cache"]        =     load_fields_cache;
    func_load["PB"]["acp"]          =     load_fields_acp;
    func_load["PB"]["control"]      =     load_fields_control;
    func_load["PB"]["trace"]        =     load_fields_trace;
    func_load["PB"]["trace_status"] =     load_fields_trace_status;
    
    func_deposit["PB"]["addr_lo"]      =     deposit_fields_addr_lo;
    func_deposit["PB"]["addr_hi"]      =     deposit_fields_addr_hi;
    func_deposit["PB"]["addr_host"]    =     deposit_fields_addr_host;
    func_deposit["PB"]["addr_pic"]     =     deposit_fields_addr_pic;
    func_deposit["PB"]["addr_lif"]     =     deposit_fields_addr_lif;
    func_deposit["PB"]["src_id"]       =     deposit_fields_src_id;
    func_deposit["PB"]["prot"]         =     deposit_fields_prot;
    func_deposit["PB"]["cache"]        =     deposit_fields_cache;
    func_deposit["PB"]["acp"]          =     deposit_fields_acp;
    func_deposit["PB"]["control"]      =     deposit_fields_control;
    func_deposit["PB"]["trace"]        =     deposit_fields_trace;
    
    helper["PB"]["src_id"]["load_c0"]           =             ELB_PBCAXIFILTER_CSR_FILTER_SRC_WIDTH / 2;
    helper["PB"]["src_id"]["load_c1"]           = ~(-1ull << (ELB_PBCAXIFILTER_CSR_FILTER_SRC_WIDTH / 2));

    
    return true;
}



bool asset_elba::build()
{
    if (!asset::build()) return false;

    //////////////////////
    // constants
    //////////////////////
    if (!init_constants()) return false;

    string prefix = chip() + ".axi-filter";

    //////////////////////
    // collecting ports
    //////////////////////
    string strv = config::instance()->get<string>(prefix + ".modes." + axitrace::option::inst()["mode"][0].get() + ".ports", "");
    string strv2 = strv;
    boost::to_lower(strv2);
    if (strv2 == "all") {
        strv = config::instance()->get<string>(prefix + ".ports", "");
    }
    MY_DEBUG << " ports found: " << strv << endl;
    ports = config::instance().split_list(strv);
    if (ports.size() == 0) {
        MY_ERROR << " no ports are found" << endl;
        return false;
    }
    for (unsigned i = 0; i < ports.size(); ++i) {
        port_lkup.insert(ports[i]);
    }

    if (active_ports.size() != 0) {
        for (set<string>::iterator it = active_ports.begin(); it != active_ports.end(); ++it) {
            if (port_lkup.find(*it) == port_lkup.end()) {
                MY_ERROR << " Port '" << *it << "' is unknown" << endl;
                return false;
            }
        }
    }
    if (active_ports.size() == 0) {
        for (unsigned i = 0; i < ports.size(); i++) active_ports.insert(ports[i]);
    } 


    //////////////////////
    // collecting filters
    //////////////////////
    unsigned default_num = config::instance()->get<unsigned>(prefix + ".defaults.num-filters", 0);
    for (unsigned i = 0; i < ports.size(); i++) {
        unsigned num = default_num;
        optional<unsigned> rval = config::instance()->get_optional<unsigned>(prefix + "." + ports[i] + ".num-filters");
        if (rval) num = *rval;
        for (unsigned j = 0; j < num; j++) {
            asset_elba::axi_filter* f = new asset_elba::axi_filter(ports[i], j);
            if (!f->build()) {
                MY_ERROR << " failed in building filter " << j << " for port '" << ports[i] << "'" << endl;
                return false;
            }
            filters[ports[i]].push_back(f);

            ////spare filters
            if (spare_filters.size() == 0) {
                spare_filters.push_back(f->clone("spare", 0));
                spare_filters.push_back(f->clone("spare", 1));
            }
        }

        asset_elba::axi_action_trace* action_ptr = new asset_elba::axi_action_trace(filters[ports[i]][0]);
        map_action_name2idx[ports[i]][action_ptr->name] = actions[ports[i]].size();
        actions[ports[i]].push_back(action_ptr);

        asset_elba::axi_condition_trace_status* condition_ptr = new asset_elba::axi_condition_trace_status(filters[ports[i]][0]);
        map_condition_name2idx[ports[i]][condition_ptr->name] = conditions[ports[i]].size();
        conditions[ports[i]].push_back(condition_ptr);

        MY_INFO << " Created " << filters[ports[i]].size() << " filters for port '" << ports[i] << "'" << endl;
    }


    return true;
}


bool asset_elba::load_fields_hit_count(asset::filter* f, const vector<uint64_t>& d, bool stash)
{
    f->get<condition*>("hit_count")->set("hit_count_read", d[0] & 0xffffull, false);
    f->get<condition*>("hit_count")->set("hit_count_write", (d[0] >> 16) & 0xffffull, false);
    return true;
}


bool asset_elba::load_fields_addr_lo(asset::filter* f, const vector<uint64_t>& d, bool stash)
{
    f->get<condition*>("addr_range")->set("addr_lo", d[0] & 0x7fffffffull, false);
    if (stash) f->get<condition*>("addr_range")->set("addr_lo", d[0] & 0x7fffffffull, true);
    return true;
}


bool asset_elba::deposit_fields_addr_lo(asset::filter* f, vector<uint64_t>& d)
{
    uint64_t data = f->get<condition*>("addr_range")->get("addr_lo") & 0x7fffffffull;
    d.push_back(data);
    return true;
}


bool asset_elba::load_fields_addr_hi(asset::filter* f, const vector<uint64_t>& d, bool stash)
{
    f->get<condition*>("addr_range")->set("addr_hi", d[0] & 0x7fffffffull, false);
    if (stash) f->get<condition*>("addr_range")->set("addr_hi", d[0] & 0x7fffffffull, true);
    return true;
}


bool asset_elba::deposit_fields_addr_hi(asset::filter* f, vector<uint64_t>& d)
{
    uint64_t data = f->get<condition*>("addr_range")->get("addr_hi") & 0x7fffffffull;
    d.push_back(data);
    return true;
}


bool asset_elba::load_fields_addr_host(asset::filter* f, const vector<uint64_t>& d, bool stash)
{
    f->get<condition*>("addr_host")->set("mask", d[0] & 0x1ull, false);
    f->get<condition*>("addr_host")->set("match", (d[0] >> 1) & 0x1ull, false);
    if (stash) {
        f->get<condition*>("addr_host")->set("mask", d[0] & 0x1ull, true);
        f->get<condition*>("addr_host")->set("match", (d[0] >> 1) & 0x1ull, true);
    }

    return true;
}


bool asset_elba::deposit_fields_addr_host(asset::filter* f, vector<uint64_t>& d)
{
    uint64_t data = f->get<condition*>("addr_host")->get("mask") & 0x1ull;
    data |= (f->get<condition*>("addr_host")->get("match") & 0x1ull) << 1;
    d.push_back(data);
    return true;
}


bool asset_elba::load_fields_addr_pic(asset::filter* f, const vector<uint64_t>& d, bool stash)
{

    f->get<condition*>("addr_pic")->set("mask", d[0] & 0x1ull, false);
    f->get<condition*>("addr_pic")->set("match", (d[0] >> 1) & 0x1ull, false);
    if (stash) {
        f->get<condition*>("addr_pic")->set("mask", d[0] & 0x1ull, true);
        f->get<condition*>("addr_pic")->set("match", (d[0] >> 1) & 0x1ull, true);
    }

    return true;
}


bool asset_elba::deposit_fields_addr_pic(asset::filter* f, vector<uint64_t>& d)
{

    uint64_t data = f->get<condition*>("addr_pic")->get("mask") & 0x1ull;
    data |= (f->get<condition*>("addr_pic")->get("match") & 0x1ull) << 1;
    d.push_back(data);
    return true;
}


bool asset_elba::load_fields_addr_lif(asset::filter* f, const vector<uint64_t>& d, bool stash)
{
    f->get<condition*>("addr_lif")->set("mask", d[0] & 0x7ffull, false);
    f->get<condition*>("addr_lif")->set("match", (d[0] >> 11) & 0x7ffull, false);
    if (stash) {
        f->get<condition*>("addr_lif")->set("mask", d[0] & 0x7ffull, true);
        f->get<condition*>("addr_lif")->set("match", (d[0] >> 11) & 0x7ffull, true);
    }

    return true;
}


bool asset_elba::deposit_fields_addr_lif(asset::filter* f, vector<uint64_t>& d)
{
    uint64_t data = f->get<condition*>("addr_lif")->get("mask") & 0x7ffull;
    data |= (f->get<condition*>("addr_lif")->get("match") & 0x7ffull) << 11;
    d.push_back(data);
    return true;
}


bool asset_elba::load_fields_src_id(asset::filter* f, const vector<uint64_t>& d, bool stash)
{
    f->get<condition*>("src_id")->set("mask", d[0] & helper[f->port]["src_id"]["load_c1"], false);
    f->get<condition*>("src_id")->set("match", (d[0] >> helper[f->port]["src_id"]["load_c0"]) & helper[f->port]["src_id"]["load_c1"], false);
    if (stash) {
        f->get<condition*>("src_id")->set("mask", d[0] & helper[f->port]["src_id"]["load_c1"], true);
        f->get<condition*>("src_id")->set("match", (d[0] >> helper[f->port]["src_id"]["load_c0"]) & helper[f->port]["src_id"]["load_c1"], true);
    }

    return true;
}


bool asset_elba::deposit_fields_src_id(asset::filter* f, vector<uint64_t>& d)
{
    uint64_t data = f->get<condition*>("src_id")->get("mask") & helper[f->port]["src_id"]["load_c1"];
    data |= (f->get<condition*>("src_id")->get("match") & helper[f->port]["src_id"]["load_c1"]) << helper[f->port]["src_id"]["load_c0"];
    d.push_back(data);
    return true;
}


bool asset_elba::load_fields_prot(asset::filter* f, const vector<uint64_t>& d, bool stash)
{
    f->get<condition*>("awprot")->set("mask", d[0] & 0x7ull, false);
    f->get<condition*>("awprot")->set("match", (d[0] >> 3) & 0x7ull, false);
    f->get<condition*>("arprot")->set("mask", (d[0] >> 6) & 0x7ull, false);
    f->get<condition*>("arprot")->set("match", (d[0] >> 9) & 0x7ull, false);
    f->get<action*>("awprot")->set("overwrite", (d[0] >> 12) & 0x7ull, false);
    f->get<action*>("awprot")->enabled = (d[0] >> 15) & 0x1ull;
    f->get<action*>("arprot")->set("overwrite", (d[0] >> 16) & 0x7ull, false);
    f->get<action*>("arprot")->enabled = (d[0] >> 19) & 0x1ull;
    if (stash) {
        f->get<condition*>("awprot")->set("mask", d[0] & 0x7ull, true);
        f->get<condition*>("awprot")->set("match", (d[0] >> 3) & 0x7ull, true);
        f->get<condition*>("arprot")->set("mask", (d[0] >> 6) & 0x7ull, true);
        f->get<condition*>("arprot")->set("match", (d[0] >> 9) & 0x7ull, true);
        f->get<action*>("awprot")->set("overwrite", (d[0] >> 12) & 0x7ull, true);
        f->get<action*>("awprot")->oenabled = (d[0] >> 15) & 0x1ull;
        f->get<action*>("arprot")->set("overwrite", (d[0] >> 16) & 0x7ull, true);
        f->get<action*>("arprot")->oenabled = (d[0] >> 19) & 0x1ull;
    }
    return true;
}


bool asset_elba::deposit_fields_prot(asset::filter* f, vector<uint64_t>& d)
{
    uint64_t data = f->get<condition*>("awprot")->get("mask") & 0x7ull;
    data |= (f->get<condition*>("awprot")->get("match") & 0x7ull) << 3;
    data |= (f->get<condition*>("arprot")->get("mask") & 0x7ull) << 6;
    data |= (f->get<condition*>("arprot")->get("match") & 0x7ull) << 9;
    data |= (f->get<action*>("awprot")->get("overwrite") & 0x7ull) << 12;
    data |= (f->get<action*>("awprot")->enabled & 0x1ull) << 15;
    data |= (f->get<action*>("arprot")->get("overwrite") & 0x7ull) << 16;
    data |= (f->get<action*>("arprot")->enabled & 0x1ull) << 19;
    d.push_back(data);
    return true;
}


bool asset_elba::load_fields_cache(asset::filter* f, const vector<uint64_t>& d, bool stash)
{
    f->get<condition*>("awcache")->set("mask", d[0] & 0xfull, false);
    f->get<condition*>("awcache")->set("match", (d[0] >> 4) & 0xfull, false);
    f->get<condition*>("arcache")->set("mask", (d[0] >> 8) & 0xfull, false);
    f->get<condition*>("arcache")->set("match", (d[0] >> 12) & 0xfull, false);
    f->get<action*>("awcache")->set("overwrite", (d[0] >> 16) & 0xfull, false);
    f->get<action*>("awcache")->enabled = (d[0] >> 20) & 0x1ull;
    f->get<action*>("arcache")->set("overwrite", (d[0] >> 21) & 0xfull, false);
    f->get<action*>("arcache")->enabled = (d[0] >> 25) & 0x1ull;
    if (stash) {
        f->get<condition*>("awcache")->set("mask", d[0] & 0xfull, true);
        f->get<condition*>("awcache")->set("match", (d[0] >> 4) & 0xfull, true);
        f->get<condition*>("arcache")->set("mask", (d[0] >> 8) & 0xfull, true);
        f->get<condition*>("arcache")->set("match", (d[0] >> 12) & 0xfull, true);
        f->get<action*>("awcache")->set("overwrite", (d[0] >> 16) & 0xfull, true);
        f->get<action*>("awcache")->oenabled = (d[0] >> 20) & 0x1ull;
        f->get<action*>("arcache")->set("overwrite", (d[0] >> 21) & 0xfull, true);
        f->get<action*>("arcache")->oenabled = (d[0] >> 25) & 0x1ull;
    }
    return true;
}


bool asset_elba::deposit_fields_cache(asset::filter* f, vector<uint64_t>& d)
{
    uint64_t data = f->get<condition*>("awcache")->get("mask") & 0xfull;
    data |= (f->get<condition*>("awcache")->get("match") & 0xfull) << 4;
    data |= (f->get<condition*>("arcache")->get("mask") & 0xfull) << 8;
    data |= (f->get<condition*>("arcache")->get("match") & 0xfull) << 12;
    data |= (f->get<action*>("awcache")->get("overwrite") & 0xfull) << 16;
    data |= (f->get<action*>("awcache")->enabled & 0x1ull) << 20;
    data |= (f->get<action*>("arcache")->get("overwrite") & 0xfull) << 21;
    data |= (f->get<action*>("arcache")->enabled & 0x1ull) << 25;
    d.push_back(data);
    return true;
}


bool asset_elba::load_fields_acp(asset::filter* f, const vector<uint64_t>& d, bool stash)
{
    f->get<action*>("awacp_val")->set("overwrite", (d[0] >> 0) & 0x3ull, false);
    f->get<action*>("awacp_val")->enabled = (d[0] >> 2) & 0x1ull;
    f->get<action*>("awacp_ena")->set("overwrite", (d[0] >> 3) & 0x1ull, false);
    f->get<action*>("awacp_ena")->enabled = (d[0] >> 4) & 0x1ull;
    if (stash) {
        f->get<action*>("awacp_val")->set("overwrite", (d[0] >> 0) & 0x3ull, true);
        f->get<action*>("awacp_val")->oenabled = (d[0] >> 2) & 0x1ull;
        f->get<action*>("awacp_ena")->set("overwrite", (d[0] >> 3) & 0x1ull, true);
        f->get<action*>("awacp_ena")->oenabled = (d[0] >> 4) & 0x1ull;
    }

    return true;
}


bool asset_elba::deposit_fields_acp(asset::filter* f, vector<uint64_t>& d)
{
    uint64_t data = f->get<action*>("awacp_val")->get("overwrite") & 0x3ull;
    data |= (f->get<action*>("awacp_val")->enabled & 0x1ull) << 2;
    data |= (f->get<action*>("awacp_ena")->get("overwrite") & 0x1ull) << 3;
    data |= (f->get<action*>("awacp_ena")->enabled & 0x1ull) << 4;
    d.push_back(data);
    return true;
}


bool asset_elba::load_fields_control(asset::filter* f, const vector<uint64_t>& d, bool stash)
{
    f->enabled = d[0] & 0x1ull;
    f->get<action*>("control")->set("read_access", (d[0] >> 1) & 0x1ull, false);
    f->get<action*>("control")->set("write_access", (d[0] >> 2) & 0x1ull, false);
    f->get<action*>("control")->set("read_freeze", (d[0] >> 3) & 0x1ull, false);
    f->get<action*>("control")->set("write_freeze", (d[0] >> 4) & 0x1ull, false);
    f->get<action*>("control")->set("read_interrupt", (d[0] >> 5) & 0x1ull, false);
    f->get<action*>("control")->set("write_interrupt", (d[0] >> 6) & 0x1ull, false);
    f->get<action*>("control")->set("log_req", (d[0] >> 7) & 0x1ull, false);
    f->get<action*>("control")->set("log_resp", (d[0] >> 8) & 0x1ull, false);
    f->get<action*>("control")->set("use_cache", (d[0] >> 9) & 0x1ull, false);
    f->get<action*>("control")->set("inval_send", (d[0] >> 10) & 0x1ull, false);
    f->get<action*>("control")->set("inval_receive", (d[0] >> 11) & 0x1ull, false);
    f->get<action*>("control")->set("local_ack", (d[0] >> 12) & 0x1ull, false);
    f->get<action*>("control")->set("round64", (d[0] >> 13) & 0x1ull, false);
    if (stash) {
        f->oenabled = d[0] & 0x1ull;
        f->get<action*>("control")->set("read_access", (d[0] >> 1) & 0x1ull, true);
        f->get<action*>("control")->set("write_access", (d[0] >> 2) & 0x1ull, true);
        f->get<action*>("control")->set("read_freeze", (d[0] >> 3) & 0x1ull, true);
        f->get<action*>("control")->set("write_freeze", (d[0] >> 4) & 0x1ull, true);
        f->get<action*>("control")->set("read_interrupt", (d[0] >> 5) & 0x1ull, true);
        f->get<action*>("control")->set("write_interrupt", (d[0] >> 6) & 0x1ull, true);
        f->get<action*>("control")->set("log_req", (d[0] >> 7) & 0x1ull, true);
        f->get<action*>("control")->set("log_resp", (d[0] >> 8) & 0x1ull, true);
        f->get<action*>("control")->set("use_cache", (d[0] >> 9) & 0x1ull, true);
        f->get<action*>("control")->set("inval_send", (d[0] >> 10) & 0x1ull, true);
        f->get<action*>("control")->set("inval_receive", (d[0] >> 11) & 0x1ull, true);
        f->get<action*>("control")->set("local_ack", (d[0] >> 12) & 0x1ull, true);
        f->get<action*>("control")->set("round64", (d[0] >> 13) & 0x1ull, true);
    }

    return true;
}


bool asset_elba::deposit_fields_control(asset::filter* f, vector<uint64_t>& d)
{
    uint64_t data = f->enabled & 0x1ull;
    data |= (f->get<action*>("control")->get("read_access") & 0x1ull) << 1;
    data |= (f->get<action*>("control")->get("write_access") & 0x1ull) << 2;
    data |= (f->get<action*>("control")->get("read_freeze") & 0x1ull) << 3;
    data |= (f->get<action*>("control")->get("write_freeze") & 0x1ull) << 4;
    data |= (f->get<action*>("control")->get("read_interrupt") & 0x1ull) << 5;
    data |= (f->get<action*>("control")->get("write_interrupt") & 0x1ull) << 6;
    data |= (f->get<action*>("control")->get("log_req") & 0x1ull) << 7;
    data |= (f->get<action*>("control")->get("log_resp") & 0x1ull) << 8;
    data |= (f->get<action*>("control")->get("use_cache") & 0x1ull) << 9;
    data |= (f->get<action*>("control")->get("inval_send") & 0x1ull) << 10;
    data |= (f->get<action*>("control")->get("inval_receive") & 0x1ull) << 11;
    data |= (f->get<action*>("control")->get("local_ack") & 0x1ull) << 12;
    data |= (f->get<action*>("control")->get("round64") & 0x1ull) << 13;
    d.push_back(data);
    return true;
}


bool asset_elba::load_fields_trace(asset::filter* f, const vector<uint64_t>& d, bool stash)
{
    asset::instance().get<action*>(f->port, "trace")->enabled = (d[0] >> 0) & 0x1ull;
    asset::instance().get<action*>(f->port, "trace")->set("always_on", (d[0] >> 1) & 0x1ull, false);
    asset::instance().get<action*>(f->port, "trace")->set("resp_err", (d[0] >> 2) & 0x1ull, false);
    asset::instance().get<action*>(f->port, "trace")->set("wrap", (d[0] >> 3) & 0x1ull, false);
    asset::instance().get<action*>(f->port, "trace")->set("rst", (d[0] >> 4) & 0x1ull, false);
    asset::instance().get<action*>(f->port, "trace")->set("awcache", (d[0] >> 5) & 0xfull, false);
    asset::instance().get<action*>(f->port, "trace")->set("base_addr", ((d[0] >> 9) & 0x7fffffull) | ((d[1] & 0xffull) << 23), false);
    asset::instance().get<action*>(f->port, "trace")->set("buf_size", (d[1] >> 8) & 0x1full, false);
    asset::instance().get<condition*>(f->port, "trace_status")->set("index", ((d[1] >> 13) & 0x7ffffull) | ((d[2] & 0x7ffull) << 19), false);
    asset::instance().get<condition*>(f->port, "trace_status")->set("generation", (d[2] >> 11) & 0x1ull, false);

    if (stash) {
        asset::instance().get<action*>(f->port, "trace")->oenabled = (d[0] >> 0) & 0x1ull;
        asset::instance().get<action*>(f->port, "trace")->set("always_on", (d[0] >> 1) & 0x1ull, true);
        asset::instance().get<action*>(f->port, "trace")->set("resp_err", (d[0] >> 2) & 0x1ull, true);
        asset::instance().get<action*>(f->port, "trace")->set("wrap", (d[0] >> 3) & 0x1ull, true);
        asset::instance().get<action*>(f->port, "trace")->set("rst", (d[0] >> 4) & 0x1ull, true);
        asset::instance().get<action*>(f->port, "trace")->set("awcache", (d[0] >> 5) & 0xfull, true);
        asset::instance().get<action*>(f->port, "trace")->set("base_addr", ((d[0] >> 9) & 0x7fffffull) | ((d[1] & 0xffull) << 23), true);
        asset::instance().get<action*>(f->port, "trace")->set("buf_size", (d[1] >> 8) & 0x1f, true);
        asset::instance().get<condition*>(f->port, "trace_status")->set("index", ((d[1] >> 13) & 0x7ffffull) | ((d[2] & 0x7ffull) << 19), true);
        asset::instance().get<condition*>(f->port, "trace_status")->set("generation", (d[2] >> 11) & 0x1ull, true);
    }

    return true;
}


bool asset_elba::deposit_fields_trace(asset::filter* f, vector<uint64_t>& d)
{
    uint64_t data = asset::instance().get<action*>(f->port, "trace")->enabled & 0x1ull;
    data |= (asset::instance().get<action*>(f->port, "trace")->get("always_on") & 0x1ull) << 1;
    data |= (asset::instance().get<action*>(f->port, "trace")->get("resp_err") & 0x1ull) << 2;
    data |= (asset::instance().get<action*>(f->port, "trace")->get("wrap") & 0x1ull) << 3;
    data |= (asset::instance().get<action*>(f->port, "trace")->get("rst") & 0x1ull) << 4;
    data |= (asset::instance().get<action*>(f->port, "trace")->get("awcache") & 0xfull) << 5;
    data |= (asset::instance().get<action*>(f->port, "trace")->get("base_addr") & 0x7fffffull) << 9;
    d.push_back(data);

    data = (asset::instance().get<action*>(f->port, "trace")->get("base_addr") >> 23) & 0xffull;
    data |= (asset::instance().get<action*>(f->port, "trace")->get("buf_size") & 0x1full) << 8;
    d.push_back(data);
    return true;
}


bool asset_elba::load_fields_trace_status(asset::filter* f, const vector<uint64_t>& d, bool stash)
{
    asset::instance().get<condition*>(f->port, "trace_status")->set("index", ((d[0] >> 13) & 0x7ffffull) | ((d[1] & 0x7ffull) << 19), false);
    asset::instance().get<condition*>(f->port, "trace_status")->set("generation", (d[1] >> 11) & 0x1ull, false);
    return true;
}


bool asset_elba::poll(const string& port) 
{
    uint64_t addr = addr_base[port] + addr_ofst[port]["trace"];
    uint32_t data;
    vector<uint64_t> dv;
    if (!pio::instance().reg_read(addr + 4, data)) return false;
    dv.push_back(data);
    if (!pio::instance().reg_read(addr + 8, data)) return false;
    dv.push_back(data);
    return func_load[port]["trace_status"](filters[port][0], dv, false);
}



bool asset_elba::load()
{
    vector<uint64_t> dv;
    uint32_t data;
    char tokens[][32] = {
        "hit_count"
    ,   "addr_lo"
    ,   "addr_hi"
    ,   "addr_host"
    ,   "addr_pic"
    ,   "addr_lif"
    ,   "src_id"
    ,   "prot"
    ,   "cache"
    ,   "acp"
    };

    for (unsigned i = 0; i < ports.size(); i++) {
        if (!active(ports[i])) continue;
        for (unsigned j = 0; j < filters[ports[i]].size(); j++) {
            ////control
            dv.clear();
            if (!read_csr(ports[i], addr_base[ports[i]] + addr_ofst[ports[i]]["control"] + j * 4, addr_labl[ports[i]]["control"], j, data)) return false; 
            dv.push_back(data);
            if (!func_load[ports[i]]["control"](filters[ports[i]][j], dv, true)) return false;
        }

        unsigned filter_num = filters[ports[i]].size();
        for (unsigned j = 0; j < sizeof(tokens) / 32; ++j) {
            unsigned read_num = filter_num * addr_elen[ports[i]][tokens[j]] / 4;
            for (unsigned k = 0; k < read_num; k++) {
                bool skip_read = true;
                unsigned filter_num_per_read = (4 + addr_elen[ports[i]][tokens[j]] - 1) / addr_elen[ports[i]][tokens[j]];
                for (unsigned m = 0; m < filter_num_per_read; ++m) {
                    unsigned filter_idx = k * 4 / addr_elen[ports[i]][tokens[j]] + m;
                    if (filters[ports[i]][filter_idx]->enabled)  {
                        skip_read = false;
                        break; 
                    }
                }
                if (skip_read) continue;

                if (!read_csr(ports[i], addr_base[ports[i]] + addr_ofst[ports[i]][tokens[j]] + k * 4, addr_labl[ports[i]][tokens[j]], k, data)) return false;

                unsigned read_num_per_filter = (addr_elen[ports[i]][tokens[j]] + 4 - 1) / 4;
                if ((k % read_num_per_filter) == 0) dv.clear();
                if ((k % read_num_per_filter) != (read_num_per_filter - 1)) {
                    dv.push_back(data);
                }
                else {
                    for (unsigned m = 0; m < filter_num_per_read; ++m) {
                        if ((k % read_num_per_filter) == 0) dv.clear();
                        unsigned filter_idx = k * 4 / addr_elen[ports[i]][tokens[j]] + m;
                        uint32_t data2 = data >>(addr_elen[ports[i]][tokens[j]] * m * 8);
                        dv.push_back(data2);
                        if (!func_load[ports[i]][tokens[j]](filters[ports[i]][filter_idx], dv, true)) return false;
                    }
                }
            }
        }

        //////trace
        dv.clear();
        uint64_t addr = addr_base[ports[i]] + addr_ofst[ports[i]]["trace"];
        if (!read_csr(ports[i], addr, addr_labl[ports[i]]["trace"], 0, data)) return false;
        dv.push_back(data);
        if (!read_csr(ports[i], addr + 4, addr_labl[ports[i]]["trace"], 1, data)) return false;
        dv.push_back(data);
        if (!read_csr(ports[i], addr + 8, addr_labl[ports[i]]["trace"], 2, data)) return false;
        dv.push_back(data);
        if (!func_load[ports[i]]["trace"](filters[ports[i]][0], dv, true)) return false;
    }

    return true;
}



bool asset_elba::program()
{
    vector<uint64_t> dv;
    uint32_t data;
    char tokens[][32] = {
        "addr_lo"
    ,   "addr_hi"
    ,   "addr_host"
    ,   "addr_pic"
    ,   "addr_lif"
    ,   "src_id"
    ,   "prot"
    ,   "cache"
    ,   "acp"
    };

    for (unsigned i = 0; i < ports.size(); i++) {
        if (!active(ports[i])) continue;

        unsigned filter_num = filters[ports[i]].size();
        for (unsigned j = 0; j < sizeof(tokens) / 32; ++j) {
            unsigned write_num = filter_num * addr_elen[ports[i]][tokens[j]] / 4;
            for (unsigned k = 0; k < write_num; k++) {
                unsigned filter_num_per_write = (4 + addr_elen[ports[i]][tokens[j]] - 1) / addr_elen[ports[i]][tokens[j]];
                bool skip_write = true;
                for (unsigned m = 0; m < filter_num_per_write; ++m) {
                    unsigned filter_idx = k * 4 / addr_elen[ports[i]][tokens[j]] + m;
                    if (filters[ports[i]][filter_idx]->updated) {
                        skip_write = false;
                        break; 
                    }
                }
                if (skip_write) continue;

                bool only_disable = true;
                for (unsigned m = 0; m < filter_num_per_write; ++m) {
                    unsigned filter_idx = k * 4 / addr_elen[ports[i]][tokens[j]] + m;
                    if (filters[ports[i]][filter_idx]->enabled) {
                        only_disable = false;
                        break; 
                    }
                }

                if (only_disable) continue;

                unsigned write_num_per_filter = (addr_elen[ports[i]][tokens[j]] + 4 - 1) / 4;
                if ((k % write_num_per_filter) == 0) {
                    data = 0;
                    for (unsigned m = 0; m < filter_num_per_write; ++m) {
                        unsigned filter_idx = k * 4 / addr_elen[ports[i]][tokens[j]] + m;
                        dv.clear();
                        if (!func_deposit[ports[i]][tokens[j]](filters[ports[i]][filter_idx], dv)) return false;
                        data |= dv[0] << (addr_elen[ports[i]][tokens[j]] * m * 8);
                    }
                } 
                else {
                    data = dv[k % write_num_per_filter];
                }

                if (!write_csr(ports[i], addr_base[ports[i]] + addr_ofst[ports[i]][tokens[j]] + k * 4, data)) return false;
            }
        }

        ////control
        for (unsigned j = 0; j < filters[ports[i]].size(); j++) {
            if (!filters[ports[i]][j]->updated) continue;
            ////control
            dv.clear();
            if (!func_deposit[ports[i]]["control"](filters[ports[i]][j], dv)) return false;
            if (!write_csr(ports[i], addr_base[ports[i]] + addr_ofst[ports[i]]["control"] + j * 4, (uint32_t)dv[0])) return false; 
            filters[ports[i]][j]->update(false);
        }

        //////trace
        if (!get<action*>(ports[i], "trace")->updated) continue;
        dv.clear();
        if (!func_deposit[ports[i]]["trace"](filters[ports[i]][0], dv)) return false;
        uint64_t addr = addr_base[ports[i]] + addr_ofst[ports[i]]["trace"];
        if (!write_csr(ports[i], addr + 4, (uint32_t)dv[1])) return false;
        if (!write_csr(ports[i], addr + 0, (uint32_t)dv[0])) return false;
        get<action*>(ports[i], "trace")->updated = false;
    }

    return true;
}


bool  asset_elba::reset(const string& port)
{
    vector<uint64_t> dv;
    assert(active(port));

    uint64_t addr = addr_base[port] + addr_ofst[port]["trace"];
    get<action*>(port, "trace")->set("rst", 1);
    if (!func_deposit[port]["trace"](filters[port][0], dv)) return false;
    if (!write_csr(port, addr + 0, (uint32_t)dv[0])) return false;

    get<action*>(port, "trace")->set("rst", 0);
    if (!func_deposit[port]["trace"](filters[port][0], dv)) return false;
    if (!write_csr(port, addr + 0, (uint32_t)dv[0])) return false;

    return true;
}



asset::filter*  asset_elba::create(const string& port, unsigned id) const
{
    asset::filter* f = new asset_elba::axi_filter(port, id);
    f->build();
    return f;
}



unsigned asset_elba::axi_condition_addr_range::show_data2(unsigned i) const
{
    if (fields[i] == "addr_lo") {
        console::instance() << " (0x" << hex << (vals[i] << 6) << dec << ")";
    }
    if (fields[i] == "addr_hi") {
        console::instance() << " (0x" << hex << ((vals[i] << 6) | 0x3full) << dec << ")";
    }
    return 0;
}



unsigned asset_elba::axi_action_trace::show_data2(unsigned i) const
{

    if (fields[i] == "base_addr") {
        console::instance() << " (0x" << hex << ((vals[i] << 6)) << dec << ")";
    }
    else if (fields[i] == "buf_size") {
        console::instance() << " (" << vals[i] << ")";
    }
    else {
        return asset::element::show_data2(i);
    }

    return 0;
}



unsigned asset_elba::axi_condition_hit_count::show_data2(unsigned i) const
{
    console::instance() << " (" << vals[i] << ")";
    return 0;
}



unsigned asset_elba::axi_condition_trace_status::show_data2(unsigned i) const
{
    if (fields[i] == "index") {
        console::instance() << " (" << vals[i] << ")";
    }
    else {
        return asset::element::show_data2(i);
    }
    return 0;
}




} //namespace axitrace 
