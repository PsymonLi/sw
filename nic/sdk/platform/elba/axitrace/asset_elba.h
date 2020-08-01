// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __AXITRACE_ASSET_ELBA_HH__
#define __AXITRACE_ASSET_ELBA_HH__

#include "asset.h"

namespace axitrace {


class asset_elba : public asset
{
public:
class axi_condition;
class axi_action;
class axi_filter;


class axi_condition: public asset::condition
{
public:
    axi_condition(const string& nm, asset::filter*);
    virtual ~axi_condition();
    virtual bool                            overlap(const condition* cond) const;
};



class axi_action: public asset::action
{
public:
    axi_action(const string& nm, asset::filter*);
    virtual ~axi_action();
};


class axi_condition_hit_count : public asset_elba::axi_condition
{
public:
    axi_condition_hit_count(asset::filter*);
    virtual ~axi_condition_hit_count();
    virtual bool                            overlap(const condition* cond) const;
    virtual asset::element*                 clone(asset::filter* f) const;
    virtual unsigned                        show_data2(unsigned i = 0) const;                       
};


class axi_condition_addr_range: public asset_elba::axi_condition
{
public:
    axi_condition_addr_range(asset::filter*);
    virtual ~axi_condition_addr_range();
    virtual bool                            overlap(const condition* cond) const;
    virtual asset::element*                 clone(asset::filter* f) const;
    virtual unsigned                        show_data2(unsigned i = 0) const;                       
};


class axi_condition_addr_host : public asset_elba::axi_condition
{
public:
    axi_condition_addr_host(asset::filter*);
    virtual ~axi_condition_addr_host();
    virtual asset::element*                 clone(asset::filter* f) const;
};


class axi_condition_addr_pic : public asset_elba::axi_condition
{
public:
    axi_condition_addr_pic(asset::filter*);
    virtual ~axi_condition_addr_pic();
    virtual asset::element*                 clone(asset::filter* f) const;
};



class axi_condition_addr_lif : public asset_elba::axi_condition
{
public:
    axi_condition_addr_lif(asset::filter*);
    virtual ~axi_condition_addr_lif();
    virtual asset::element*                 clone(asset::filter* f) const;
};



class axi_condition_src_id: public asset_elba::axi_condition
{
public:
    axi_condition_src_id(asset::filter*);
    virtual ~axi_condition_src_id();
    virtual asset::element*                 clone(asset::filter* f) const;
};


class axi_condition_awprot: public asset_elba::axi_condition
{
public:
    axi_condition_awprot(asset::filter*);
    virtual ~axi_condition_awprot();
    virtual asset::element*                 clone(asset::filter* f) const;
};


class axi_condition_arprot: public asset_elba::axi_condition
{
public:
    axi_condition_arprot(asset::filter*);
    virtual ~axi_condition_arprot();
    virtual asset::element*                 clone(asset::filter* f) const;
};


class axi_condition_awcache: public asset_elba::axi_condition
{
public:
    axi_condition_awcache(asset::filter*);
    virtual ~axi_condition_awcache();
    virtual asset::element*                 clone(asset::filter* f) const;
};


class axi_condition_arcache: public asset_elba::axi_condition
{
public:
    axi_condition_arcache(asset::filter*);
    virtual ~axi_condition_arcache();
    virtual asset::element*                 clone(asset::filter* f) const;
};



class axi_action_awprot: public asset_elba::axi_action
{
public:
    axi_action_awprot(asset::filter* f);
    virtual ~axi_action_awprot();
    virtual asset::element*                 clone(asset::filter* f) const;
};


class axi_action_arprot: public asset_elba::axi_action
{
public:
    axi_action_arprot(asset::filter* f);
    virtual ~axi_action_arprot();
    virtual asset::element*                 clone(asset::filter* f) const;
};



class axi_action_awcache: public asset_elba::axi_action
{
public:
    axi_action_awcache(asset::filter* f);
    virtual ~axi_action_awcache();
    virtual asset::element*                 clone(asset::filter* f) const;
};


class axi_action_arcache: public asset_elba::axi_action
{
public:
    axi_action_arcache(asset::filter* f);
    virtual ~axi_action_arcache();
    virtual asset::element*                 clone(asset::filter* f) const;
};


class axi_action_awacp_ena : public asset_elba::axi_action
{
public:
    axi_action_awacp_ena(asset::filter* f);
    virtual ~axi_action_awacp_ena();
    virtual asset::element*                 clone(asset::filter* f) const;
};


class axi_action_awacp_val: public asset_elba::axi_action
{
public:
    axi_action_awacp_val(asset::filter* f);
    virtual ~axi_action_awacp_val();
    virtual asset::element*                 clone(asset::filter* f) const;
};


class axi_action_aracp_ena : public asset_elba::axi_action
{
public:
    axi_action_aracp_ena(asset::filter* f);
    virtual ~axi_action_aracp_ena();
    virtual asset::element*                 clone(asset::filter* f) const;
};


class axi_action_aracp_val: public asset_elba::axi_action
{
public:
    axi_action_aracp_val(asset::filter* f);
    virtual ~axi_action_aracp_val();
    virtual asset::element*                 clone(asset::filter* f) const;
};


class axi_action_control : public asset_elba::axi_action
{
public:
    axi_action_control(asset::filter* f);
    virtual ~axi_action_control();
    virtual asset::element*                 clone(asset::filter* f) const;
};


class axi_action_trace: public asset_elba::axi_action
{
public:
    axi_action_trace(asset::filter* f);
    virtual ~axi_action_trace();
    virtual unsigned                        show_data2(unsigned i = 0) const;                       
};


class axi_condition_trace_status: public asset_elba::axi_condition
{
public:
    axi_condition_trace_status(asset::filter* f);
    virtual ~axi_condition_trace_status();
    virtual unsigned                        show_data2(unsigned i = 0) const;                       
};



class axi_filter: public asset::filter
{
public:
    axi_filter(const string& port, int id);
    virtual ~axi_filter();
    virtual bool                            build();
    virtual filter*                         clone(const string& port, int id) const;
};


//////////////////////////////////////// //////////////////////////////////////////
////////////////////////// class asset  ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
public:
    string                                  chip() const { return "elba"; }
    virtual void                            test();

protected:
    virtual bool                            init_constants();
    virtual bool                            build();                                   
    virtual bool                            load(); 
    virtual bool                            program();
    virtual bool                            poll(const string& port);
    virtual bool                            reset(const string& port);
    virtual asset::filter*                  create(const string& port, unsigned id) const;

    ////
private:
    static bool                             load_fields_hit_count(asset::filter* f, const vector<uint64_t>& d, bool stash = false);
    static bool                             load_fields_addr_lo(asset::filter* f, const vector<uint64_t>& d, bool stash = false);
    static bool                             load_fields_addr_hi(asset::filter* f, const vector<uint64_t>& d, bool stash = false);
    static bool                             load_fields_addr_host(asset::filter* f, const vector<uint64_t>& d, bool stash = false);
    static bool                             load_fields_addr_pic(asset::filter* f, const vector<uint64_t>& d, bool stash = false);
    static bool                             load_fields_addr_lif(asset::filter* f, const vector<uint64_t>& d, bool stash = false);
    static bool                             load_fields_src_id(asset::filter* f, const vector<uint64_t>& d, bool stash = false);
    static bool                             load_fields_prot(asset::filter* f, const vector<uint64_t>& d, bool stash = false);
    static bool                             load_fields_cache(asset::filter* f, const vector<uint64_t>& d, bool stash = false);
    static bool                             load_fields_acp(asset::filter* f, const vector<uint64_t>& d, bool stash = false);
    static bool                             load_fields_control(asset::filter* f, const vector<uint64_t>& d, bool stash = false);
    static bool                             load_fields_trace(asset::filter* f, const vector<uint64_t>& d, bool stash = false);
    static bool                             load_fields_trace_status(asset::filter* f, const vector<uint64_t>& d, bool stash = false);

    static bool                             deposit_fields_addr_lo(asset::filter* f, vector<uint64_t>& d);
    static bool                             deposit_fields_addr_hi(asset::filter* f, vector<uint64_t>& d);
    static bool                             deposit_fields_addr_host(asset::filter* f, vector<uint64_t>& d);
    static bool                             deposit_fields_addr_pic(asset::filter* f, vector<uint64_t>& d);
    static bool                             deposit_fields_addr_lif(asset::filter* f, vector<uint64_t>& d);
    static bool                             deposit_fields_src_id(asset::filter* f, vector<uint64_t>& d);
    static bool                             deposit_fields_prot(asset::filter* f, vector<uint64_t>& d);
    static bool                             deposit_fields_cache(asset::filter* f, vector<uint64_t>& d);
    static bool                             deposit_fields_acp(asset::filter* f, vector<uint64_t>& d);
    static bool                             deposit_fields_control(asset::filter* f, vector<uint64_t>& d);
    static bool                             deposit_fields_trace(asset::filter* f, vector<uint64_t>& d);
};




} /// namespace axitrace


#endif //__AXITRACE_ASSET_ELBA_HH__
