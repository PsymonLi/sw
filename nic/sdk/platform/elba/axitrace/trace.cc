#include <memory>
#include <iostream>
#include "trace.h"
#include "asset.h"
#include "trace_elba_axi.h"
#include "config.h"
#include "msg_undef.inc"
#include "msg_redef.inc"
#include "option.h"
#include "generic.h"
#include "console.h"


namespace axitrace {

static unique_ptr<trace::man> trace_man_ptr = unique_ptr<trace::man>(new trace::man());


trace::man::~man()
{
    for (unsigned i = 0; i < traces.size(); ++i) {
        delete traces[i];
    }
    for (map<asset::filter*, trace*>::iterator it = map_filter2trace.begin(); it != map_filter2trace.end(); it++) {
        delete it->first;
    }
}


trace* trace::man::get(unsigned id)
{
    if (trace_lkup.find(id) == trace_lkup.end()) {
        assert(0);
        return 0;
    }
    return trace_lkup[id];
}



const trace* trace::man::get(unsigned id) const
{
    if (trace_lkup.find(id) == trace_lkup.end()) {
        assert(0);
        return 0;
    }
    return trace_lkup.at(id);
}



trace::man* trace::instancep()
{
    return trace_man_ptr.get();
}


trace::man& trace::instance()
{
    return *instancep();
}



map<unsigned, trace*>& trace::inst()
{
    return instancep()->trace_lkup;
}



trace::trace(unsigned aid, const ptree& tn)
    : id(aid), cfg_node(tn)
{
    cfg_filter = 0;
    cfg_action = 0;
}



trace::~trace()
{
    if (cfg_filter) delete cfg_filter;
    if (cfg_action) delete cfg_action;
}


trace* trace::man::create(unsigned id, const ptree& tn) const
{
    if (asset::instance().chip() == "elba") {
        return trace_elba_axi::create(id, tn);
    }
    else {
        assert(0);
        return 0;
    }
}



template<> string config::join_list_dec<trace*>(const vector<trace*>& l, const string& delimiter) const
{
    vector<unsigned> ids;
    for (unsigned i = 0; i < l.size(); ++i) ids.push_back(l[i]->id);
    return join_list_dec(ids, delimiter);
}


bool trace::man::distribute()
{
    map<string, bool> always_on_found;
    map<string, bool> always_on_suppressing;
    map<string, trace*> always_on_trace;

    for (unsigned i = 0; i < traces.size(); ++i) for (unsigned j = 0; j < traces[i]->ports.size(); ++j) {
        if (traces[i]->cfg_action->get("always_on")) {
            always_on_found[traces[i]->ports[j]] = true;
            always_on_trace[traces[i]->ports[j]] = traces[i];
            if (port_traces[traces[i]->ports[j]].size() > 0) {
                always_on_suppressing[traces[i]->ports[j]] = true;
            }
        } 
        else if (always_on_found[traces[i]->ports[j]]) {
            always_on_suppressing[traces[i]->ports[j]] = true;
        }   
        port_trace_lkup[traces[i]->ports[j]][traces[i]->id] = traces[i];
        port_traces[traces[i]->ports[j]].push_back(traces[i]);
    }

    for (unsigned i = 0; i < asset::instance().ports.size(); ++i) {
        string& port = asset::instance().ports[i];
        if (!asset::instance().active(port)) continue;
        MY_INFO << "[" << port << "] " << port_traces[port].size() << " traces are distributed to port '" << port << "' : '" << config::instance().join_list_dec(port_traces[port]) << "'" << endl;
        if (always_on_suppressing[port] && !axitrace::option::instance().has("suppress_warning", "always-on")) {
            MY_WARNING << "[always-on][" << port << "] trace '" << always_on_trace[port]->id << "' has 'always_on' = 1 which will suppress other traces for the same port" << endl;
        }
    }

    return true;
}



bool trace::man::generate()
{
    if (asset::instance().chip() == "elba") {
        if (!trace_elba_axi::generate(this)) return false;
    }

    return true;
}



bool trace::man::allocate()
{
    if (asset::instance().chip() == "elba") {
        if (!trace_elba_axi::allocate(this)) return false;
    }

    return true;
}



bool trace::man::check()
{
    MY_INFO << " Checking all updated filter registers ... " << endl;
    console::instance() << console::control("timestamp");
    console::instance() << "##==========================================================================" << endl;
    console::instance() << "##====== Preview of programming filter registers " << endl;
    console::instance() << "##==========================================================================" << endl;
    for (unsigned i = 0; i < asset::instance().ports.size(); ++i) {
        string& port = asset::instance().ports[i];
        if (!asset::instance().active(port)) continue;
        if (port_traces[port].size() == 0) continue;
        console::instance() << "== " << port << " ==" << endl;
        for (unsigned j = 0; j < asset::instance().filters[port].size(); ++j) {
            asset::instance().filters[port][j]->show(2, 40);
        }
        
        console::instance() << "        trace                           : ";
        if (asset::instance().get<asset::action*>(port, "trace")->updated) console::instance() << "(updated)";
        else console::instance() << "(no updates)";
        console::instance() << endl;
        asset::instance().get<asset::action*>(port, "trace")->show(2, 40);
    } 

    return true;
}



bool trace::man::setup()
{
    setup_warnings = 0;
    BOOST_FOREACH(const ptree::value_type& p, config::instance()->get_child(asset::instance().chip() + ".axi-trace.traces")) {
        unsigned id = p.second.get<unsigned>("id");
        if (trace_lkup.find(id) != trace_lkup.end()) {
            MY_ERROR << " Same trace ID found for '" << id << "', please revise the cfg file before continue" << endl;
            return false;
        }
        MY_INFO << "[trace] A new trace is found with ID = '" << id << "'" << endl;
        trace* t = create(id, p.second);
        trace_lkup[id] = t;
        traces.push_back(t);

        if (!t->setup()) return false;
    }
    if (traces.size() == 0) {
        MY_ERROR << " There is 0 trace found" << endl;
        return false;
    }
    else {
        MY_INFO << " There are " << traces.size() << " traces found" << endl;
    }

    if (!distribute()) return false;
    if (!generate()) return false;
    if (!allocate()) return false;
    if (!check()) return false;


    if (setup_warnings > 0) {
        MY_INFO << "[trace] " << setup_warnings << " warnings are found during filter resource setup for trace, '--dry-run' mode will be turned on automatically" << endl;
        option::inst()["dry_run"][0].set("true");
    }
    return true;
}



} /// namespace axitrace
