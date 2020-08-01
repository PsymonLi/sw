// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __AXITRACE_TRACE_ELBA_AXI_HH__
#define __AXITRACE_TRACE_ELBA_AXI_HH__
#include "trace.h"


namespace axitrace {

class trace_elba_axi : public trace
{
public:
    trace_elba_axi(unsigned aid, const ptree& tn);
    virtual ~trace_elba_axi();
    bool                                        setup();
    
    static trace*                               create(unsigned id, const ptree& tn);
    static bool                                 generate(trace::man* man);
    static bool                                 allocate(trace::man* man);

protected:
    static bool                                 resolve_overlap(trace::man* man, const vector<asset::filter*>& overlaps, asset::filter* f, const string& port, unsigned start_id, vector<asset::filter*>& r);
};


} /// namespace axitrace 

#endif //__AXITRACE_TRACE_ELBA_AXI_HH__
