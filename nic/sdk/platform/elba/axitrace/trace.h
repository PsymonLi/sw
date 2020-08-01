// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __AXITRACE_TRACE_HH__
#define __AXITRACE_TRACE_HH__

#include "asset.h"
#include "config.h"


namespace axitrace {

class trace
{
public:
class man;

class man 
{
public:
    man() {}
    ~man();
    vector<trace*>                          traces;
    map<unsigned, trace*>                   trace_lkup;
    virtual bool                            setup();
    trace*                                  get(unsigned id);
    const trace*                            get(unsigned id) const;
    trace*                                  create(unsigned id, const ptree& tn) const;
    map<string, vector<trace*> >            port_traces;
    map<string, map<unsigned, trace*> >     port_trace_lkup;
    map<string, vector<asset::filter*> >    port_filters;
    map<asset::filter*, trace*>             map_filter2trace;
    unsigned                                setup_warnings;

protected:
    bool                                    distribute();
    bool                                    generate();
    bool                                    allocate();
    bool                                    check();
};


////////////////////////////////////////////////////////////////////////////////////////////////////
///// class trace  /////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
public:
    trace(unsigned aid, const ptree& tn);
    virtual ~trace();
    static man*                             instancep();
    static man&                             instance();
    static map<unsigned, trace*>&           inst();
    const unsigned                          id;
    virtual bool                            setup()                 { return false; }
    vector<string>                          ports;

    const ptree&                            cfg_node;
    asset::filter*                          cfg_filter;
    asset::action*                          cfg_action;
};


} /// namespace axitrace


#endif //__AXITRACE_TRACE_HH__
