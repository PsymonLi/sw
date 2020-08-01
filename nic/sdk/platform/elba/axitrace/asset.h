// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __AXITRACE_ASSET_HH__
#define __AXITRACE_ASSET_HH__

#include <vector>
#include <map>
#include <set>
#include <cassert>


using namespace std;

namespace axitrace {

class asset 
{
public:
class element;
class condition;
class action;
class filter;

typedef bool (* func_load_fields_t)(asset::filter* f, const vector<uint64_t>& d, bool stash);
typedef bool (* func_deposit_fields_t)(asset::filter* f, vector<uint64_t>& d);

class element
{
public:
    element(const string& nm, asset::filter* f);
    virtual ~element();
    const string                            name;
    asset::filter* const                    filter;
    bool                                    enabled;
    bool                                    oenabled;
    bool                                    updated;
    bool                                    oupdated;
    bool                                    global; ///filter is borrowed
    bool                                    readonly;

    unsigned                                size() const;
    uint64_t                                get(const string& field, bool origin = false) const;
    uint64_t                                get(unsigned idx, bool origin = false) const;
    void                                    set(const string& field, uint64_t v, bool origin = false);
    void                                    set(unsigned idx, uint64_t v, bool origin = false);

    void                                    show(unsigned indent, unsigned tab) const;
    virtual unsigned                        show_tag(unsigned i = 0) const;                       
    virtual unsigned                        show_data(unsigned i = 0) const;                       
    virtual unsigned                        show_data2(unsigned i = 0) const;                       

    void                                    update(bool u = true);
    void                                    copy(const element* o);
    virtual element*                        clone(asset::filter* f) const { assert(0); return 0; }
    void                                    zero();
    bool                                    restore();

protected:
    void                                    clear();
    void                                    add_field(const string& field);
    vector<string>                          fields;
    vector<uint64_t>                        vals;
    vector<uint64_t>                        ovals;
    map<string, unsigned>                   map_field2idx;
};


class condition : public element
{
public:
    condition(const string& nm, asset::filter* f);
    virtual ~condition();
    virtual bool                            overlap(const condition* cond) const { assert(0); return true; }
    virtual unsigned                        show_tag(unsigned i = 0) const;
    virtual unsigned                        show_data(unsigned i = 0) const { return 0; }
};


class action : public element
{
public:
    action(const string& nm, asset::filter* f);
    virtual ~action();
    virtual unsigned                        show_tag(unsigned i = 0) const;
};


class filter 
{
public:
    filter(const string& port, int id);
    virtual ~filter();
    const string                            port;
    const int                               id;
    bool                                    enabled;
    bool                                    oenabled;
    vector<condition*>                      conditions;
    vector<action*>                         actions;
    bool                                    updated;
    bool                                    oupdated;
    bool                                    overlapped;
    bool                                    runt;
    virtual bool                            build() { return false; }
    template<typename T> T                  get(const string&);
    template<typename T> T                  get(unsigned);
    void                                    show(unsigned indent, unsigned tab) const;
    void                                    update(bool u = true);
    virtual bool                            overlap(const filter* f) const;
    void                                    copy(const filter* o, bool copy_condition = true, bool copy_action = true);
    virtual filter*                         clone(const string& port, int id) const { assert(0); return 0; }
    void                                    zero();
    bool                                    restore();
    map<string, unsigned>                   map_condition_name2idx;
    map<string, unsigned>                   map_action_name2idx;
};




//////////////////////////////////////////////////////////////////////////////////////
////////////////////////// class asset     ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
public:
    typedef map<string, vector<filter*> >  filters_t;
    asset();
    virtual ~asset();
    static asset*                           instancep();
    static asset&                           instance();
    virtual void                            test() {}
    virtual string                          chip() const = 0;
    filters_t                               filters;
    vector<string>                          ports;
    set<string>                             port_lkup;
    set<string>                             active_ports; 

    bool                                    active(const string& port) const { return active_ports.find(port) != active_ports.end(); }
    template<typename T> T                  get(const string& port, const string&);
    template<typename T> T                  get(const string& port, unsigned);
    virtual bool                            setup();                                   
    virtual bool                            load()                      { return false; } 
    void                                    show() const;
    virtual bool                            check() const;
    virtual bool                            program()                   { return false; }   
    virtual bool                            restore();

    virtual bool                            poll(const string& port)    { return false; }
    virtual bool                            reset(const string& port)   { return false; }

    static bool                             element_skip_build_fields;
    virtual filter*                         create(const string& port, unsigned id) const { assert(0); return 0; }

    map<string, uint64_t>                               addr_base;  
    map<string, map<string, uint64_t> >                 addr_ofst;  
    map<string, map<string, uint64_t> >                 addr_elen; //entry length in bytes
    map<string, map<string, string> >                   addr_labl; //CSR strings  
    map<string, map<string, func_load_fields_t> >       func_load; //CSR strings  
    map<string, map<string, func_deposit_fields_t> >    func_deposit; //CSR strings  
    

protected:
    map<string, vector<action*> >           actions;
    map<string, vector<condition*> >        conditions;
    map<string, map<string, unsigned> >     map_action_name2idx;
    map<string, map<string, unsigned> >     map_condition_name2idx;
    vector<filter*>                         spare_filters;
    virtual bool                            build();
    virtual bool                            init_constants()            { return false; }
    virtual bool                            read_csr(const string& port, uint64_t addr, const string& lable, unsigned idx, uint32_t& data);
    virtual bool                            write_csr(const string& port, uint64_t addr, uint32_t data);
};




} ///namespace axitrace

#endif //__AXITRACE_ASSET_HH__
