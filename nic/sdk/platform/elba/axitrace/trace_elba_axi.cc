#include <boost/algorithm/string.hpp>
#include "trace_elba_axi.h"
#include "asset_elba.h"
#include "msg_undef.inc"
#include "msg_redef.inc"
#include "console.h"


namespace axitrace {


trace_elba_axi::trace_elba_axi(unsigned id, const ptree& tn)
    : trace(id, tn)
{
}



trace_elba_axi::~trace_elba_axi()
{
}



trace* trace_elba_axi::create(unsigned id, const ptree& tn)
{
    return new trace_elba_axi(id, tn);
}



bool trace_elba_axi::setup()
{
    cfg_filter = new asset_elba::axi_filter("trace", 0);
    cfg_filter->build();
    cfg_action = new asset_elba::axi_action_trace(cfg_filter);
    boost::optional<string> ov;
    string cfg_str_default = asset::instance().chip() + ".axi-filter.defaults";
    uint64_t d;

    
    d = config::instance().to_int<uint64_t>(cfg_node.get("always-on", "false"));
    MY_DEBUG << "[trace " << id << "] cfg 'always-on' = " << d << endl;
    cfg_action->set("always_on", d);
    
    d = config::instance().to_int<uint64_t>(cfg_node.get("always-log-error", "false"));
    MY_DEBUG << "[trace " << id << "] cfg 'always-log-error' = " << d << endl;
    cfg_action->set("resp_err", d);

    ///buffer-wrap
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".buffer-wrap", ""));
    if (ov = cfg_node.get_optional<string>("buffer-wrap")) {
        d = config::instance().to_int<uint64_t>(ov.get());
    }
    MY_DEBUG << "[trace " << id << "] cfg 'buffer-wrap' = " << d << endl;
    cfg_action->set("wrap", d);

    ///trace-awcache
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".trace-awcache", ""));
    if (ov = cfg_node.get_optional<string>("trace-awcache")) {
        d = config::instance().to_int<uint64_t>(ov.get());
    }
    MY_DEBUG << "[trace " << id << "] cfg 'trace-awcache' = 0x" << hex << d << dec << endl;
    cfg_action->set("awcache", d);

    
    cfg_filter->enabled = false;

    ///addr-lo
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".addr-lo", ""));
    if (ov = cfg_node.get_optional<string>("addr-lo")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    d >>= 6;
    MY_DEBUG << "[trace " << id << "] cfg 'addr-lo' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("addr_range")->set("addr_lo", d);
    
    ///addr-hi
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".addr-hi", ""));
    if (ov = cfg_node.get_optional<string>("addr-hi")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    d >>= 6;
    MY_DEBUG << "[trace " << id << "] cfg 'addr-hi' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("addr_range")->set("addr_hi", d);
    
    ///addr-host-mask
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".addr-host-mask", ""));
    if (ov = cfg_node.get_optional<string>("addr-host-mask")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    MY_DEBUG << "[trace " << id << "] cfg 'addr-host-mask' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("addr_host")->set("mask", d);
    
    ///addr-host-match
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".addr-host-match", ""));
    if (ov = cfg_node.get_optional<string>("addr-host-match")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    MY_DEBUG << "[trace " << id << "] cfg 'addr-host-match' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("addr_host")->set("match", d);
    
    ///addr-pic-mask
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".addr-pic-mask", ""));
    if (ov = cfg_node.get_optional<string>("addr-pic-mask")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    MY_DEBUG << "[trace " << id << "] cfg 'addr-pic-mask' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("addr_pic")->set("mask", d);
    
    ///addr-pic-match
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".addr-pic-match", ""));
    if (ov = cfg_node.get_optional<string>("addr-pic-match")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    MY_DEBUG << "[trace " << id << "] cfg 'addr-pic-match' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("addr_pic")->set("match", d);
    
    ///addr-lif-mask
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".addr-lif-mask", ""));
    if (ov = cfg_node.get_optional<string>("addr-lif-mask")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    MY_DEBUG << "[trace " << id << "] cfg 'addr-lif-mask' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("addr_lif")->set("mask", d);
    
    ///addr-lif-match
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".addr-lif-match", ""));
    if (ov = cfg_node.get_optional<string>("addr-lif-match")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    MY_DEBUG << "[trace " << id << "] cfg 'addr-lif-match' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("addr_lif")->set("match", d);
    
    ///src-id-mask
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".src-id-mask", ""));
    if (ov = cfg_node.get_optional<string>("src-id-mask")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    MY_DEBUG << "[trace " << id << "] cfg 'src-id-mask' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("src_id")->set("mask", d);
    
    ///src-id-match
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".src-id-match", ""));
    if (ov = cfg_node.get_optional<string>("src-id-match")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    MY_DEBUG << "[trace " << id << "] cfg 'src-id-match' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("src_id")->set("match", d);
    
    ///awprot-mask
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".awprot-mask", ""));
    if (ov = cfg_node.get_optional<string>("awprot-mask")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    MY_DEBUG << "[trace " << id << "] cfg 'awprot-mask' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("awprot")->set("mask", d);
    
    ///awprot-match
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".awprot-match", ""));
    if (ov = cfg_node.get_optional<string>("awprot-match")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    MY_DEBUG << "[trace " << id << "] cfg 'awprot-match' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("awprot")->set("match", d);
    
    ///arprot-mask
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".arprot-mask", ""));
    if (ov = cfg_node.get_optional<string>("arprot-mask")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    MY_DEBUG << "[trace " << id << "] cfg 'arprot-mask' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("arprot")->set("mask", d);
    
    ///arprot-match
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".arprot-match", ""));
    if (ov = cfg_node.get_optional<string>("arprot-match")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    MY_DEBUG << "[trace " << id << "] cfg 'arprot-match' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("arprot")->set("match", d);
    
    ///awcache-mask
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".awcache-mask", ""));
    if (ov = cfg_node.get_optional<string>("awcache-mask")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    MY_DEBUG << "[trace " << id << "] cfg 'awcache-mask' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("awcache")->set("mask", d);
    
    ///awcache-match
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".awcache-match", ""));
    if (ov = cfg_node.get_optional<string>("awcache-match")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    MY_DEBUG << "[trace " << id << "] cfg 'awcache-match' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("awcache")->set("match", d);
    
    ///arcache-mask
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".arcache-mask", ""));
    if (ov = cfg_node.get_optional<string>("arcache-mask")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    MY_DEBUG << "[trace " << id << "] cfg 'arcache-mask' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("arcache")->set("mask", d);
    
    ///arcache-match
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".arcache-match", ""));
    if (ov = cfg_node.get_optional<string>("arcache-match")) {
        d = config::instance().to_int<uint64_t>(ov.get());
        cfg_filter->enabled = true;
    }
    MY_DEBUG << "[trace " << id << "] cfg 'arcache-match' = 0x" << hex << d << dec << endl;
    cfg_filter->get<asset::condition*>("arcache")->set("match", d);
 

    ////////////////////////////////////////
    ///log-response
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".log-response", ""));
    if (ov = cfg_node.get_optional<string>("log-response")) {
        d = config::instance().to_int<uint64_t>(ov.get());
    }
    MY_DEBUG << "[trace " << id << "] cfg 'log-response' = " << d << endl;
    cfg_filter->get<asset::action*>("control")->set("log_resp", d);
 
    ///read-interrupt
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".read-interrupt", ""));
    if (ov = cfg_node.get_optional<string>("read-interrupt")) {
        d = config::instance().to_int<uint64_t>(ov.get());
    }
    MY_DEBUG << "[trace " << id << "] cfg 'read-interrupt' = " << d << endl;
    cfg_filter->get<asset::action*>("control")->set("read_interrupt", d);
 
    ///write-interrupt
    d = config::instance().to_int<uint64_t>(config::instance()->get(cfg_str_default + ".write-interrupt", ""));
    if (ov = cfg_node.get_optional<string>("write-interrupt")) {
        d = config::instance().to_int<uint64_t>(ov.get());
    }
    MY_DEBUG << "[trace " << id << "] cfg 'write-interrupt' = " << d << endl;
    cfg_filter->get<asset::action*>("control")->set("write_interrupt", d);
 
    ////////////////////////////////////////
    cfg_filter->get<asset::action*>("control")->set("log_req", 1);
    cfg_filter->get<asset::action*>("control")->set("read_access", 1);
    cfg_filter->get<asset::action*>("control")->set("write_access", 1);
    cfg_filter->get<asset::action*>("control")->set("read_freeze", 0);
    cfg_filter->get<asset::action*>("control")->set("write_freeze", 0);


    ///////////////////////
    bool all_ports = false;
    if (!(ov = cfg_node.get_optional<string>("ports")))  {
        all_ports = true;
    }
    else {
        ports = config::instance().split_list(ov.get());
        for (unsigned i = 0; i < ports.size(); ++i) {
            boost::to_upper(ports[i]);
            if (ports[i] == "ALL") {
                all_ports = true;
                break;
            }
            else if (!asset::instance().active(ports[i])) {
                MY_ERROR << "[trace " << id << "] port '" << ports[i] << "' does not exist or is inactive" << endl;
                return false;
            }
        }
    }

    if (all_ports) {
        ports.clear();
        ports = asset::instance().ports;
    }


    MY_INFO << "[trace " << id << "] trace will be applied to ports '" << config::instance().join_list(ports) << "'" << endl;
    return true;
}



bool trace_elba_axi::generate(trace::man* man) 
{
    for (unsigned i = 0; i < asset::instance().ports.size(); ++i) {
        string& port = asset::instance().ports[i];
        if (!asset::instance().active(port)) continue;
        if (man->port_traces[port].size() == 0) continue;
        
        ////check  existing trace
        for (unsigned j = 0; j < asset::instance().filters[port].size(); ++j) {
            asset::filter* f = asset::instance().filters[port][j];
            if (!f->enabled) continue;
            if (f->get<asset::action*>("control")->get("log_req") || f->get<asset::action*>("control")->get("log_resp")) {
                if (option::inst()["clean"][0].get<bool>()) {
                    if (!option::instance().has("suppress_warning", "existing-trace")) {
                        MY_WARNING << "[existing-trace][" << port << "] the existing trace filter '" << f->id << "' will be disabled since '--cleanup' is specified" << endl;
                    }
                    else {
                        MY_INFO << "[" << port << "] the existing trace filter '" << f->id << "' will be disabled since '--cleanup' is specified" << endl;
                    }
                    ++man->setup_warnings;
                    f->get<asset::action*>("control")->set("log_req", 0);
                    f->get<asset::action*>("control")->set("log_resp", 0);
                    f->get<asset::action*>("control")->update(true);
                }
                else {
                    MY_ERROR << "[" << port << "] the existing trace filter '" << f->id << "' needs to be disabled, please use '--cleanup'" << endl;
                    return false;
                }
            }
        }
        asset::action* act = asset::instance().get<asset::action*>(port, "trace");
        if (act->enabled) {
            if (option::inst()["clean"][0].get<bool>()) {
                if (!option::instance().has("suppress_warning", "existing-trace")) {
                    MY_WARNING << "[existing-trace][" << port << "] the existing global trace will be disabled since '--cleanup' is specified" << endl;
                }
                else {
                    MY_INFO << "[" << port << "] the existing global trace will be disabled since '--cleanup' is specified" << endl;
                }
                ++man->setup_warnings;
            }
            else {
                MY_ERROR << "[" << port << "] the existing global trace needs to be disabled, please use '--cleanup'" << endl;
                return false;
            }
        }
        act->enabled = true;
        act->set("always_on", 0);
        act->set("resp_err", 0);
        act->set("rst", 0);
        act->update(true);


        ////retrieve global settings
        uint64_t d;
        boost::optional<string> ov;

        ////////buffer start
        d = config::instance().to_int<uint64_t>(config::instance()->get<string>(asset::instance().chip() + ".axi-filter.defaults.buffer-start." + port, ""));
        if (ov = config::instance()->get_optional<string>(asset::instance().chip() + ".axi-trace.settings." + port + ".buffer-start")) {
            d = config::instance().to_int<uint64_t>(ov.get());
        }
        act->set("base_addr", d >> 6);

        ////////buffer size
        d = config::instance().to_int<uint64_t>(config::instance()->get<string>(asset::instance().chip() + ".axi-filter.defaults.buffer-size", ""));
        if (ov = config::instance()->get_optional<string>(asset::instance().chip() + ".axi-trace.settings." + port + ".buffer-size")) {
            d = config::instance().to_int<uint64_t>(ov.get());
        }
        act->set("buf_size", d);

        ////////buffer wrap 
        d = config::instance().to_int<uint64_t>(config::instance()->get<string>(asset::instance().chip() + ".axi-filter.defaults.buffer-wrap", ""));
        if (ov = config::instance()->get_optional<string>(asset::instance().chip() + ".axi-trace.settings." + port + ".buffer-wrap")) {
            d = config::instance().to_int<uint64_t>(ov.get());
        }
        act->set("wrap", d);

        ////////buffer awcache
        d = config::instance().to_int<uint64_t>(config::instance()->get<string>(asset::instance().chip() + ".axi-filter.defaults.trace-awcache", ""));
        if (ov = config::instance()->get_optional<string>(asset::instance().chip() + ".axi-trace.settings." + port + ".trace-awcache")) {
            d = config::instance().to_int<uint64_t>(ov.get());
        }
        act->set("awcache", d);



        ////check log-error
        for (unsigned j = 0; j < man->port_traces[port].size(); ++j) {
            trace* t = man->port_traces[port][j];
            if (t->cfg_action->get("resp_err")) {
                act->set("resp_err", 1);
                break;
            }
        }

        ///check always on
        bool always_on = false;
        for (unsigned j = 0; j < man->port_traces[port].size(); ++j) {
            trace* t = man->port_traces[port][j];
            if (t->cfg_action->get("always_on")) {
                always_on = true;
                break;
            }
        }
        if (always_on) {
            act->set("always_on", 1);
            MY_DEBUG << "[" << port << "] global action  =======" << endl;
            if (DEBUG_MODE) {
                console::instance() << "==****** DEBUG ******==" << endl;
                asset::instance().get<asset::action*>(port, "trace")->show(4, 40);
            }
            MY_INFO << "[" << port << "] 0 filter is generated to activate 'always_on' trace" << endl;
            continue;
        }

        bool only_global = true;
        for (unsigned j = 0; j < man->port_traces[port].size(); ++j) {
            trace* t = man->port_traces[port][j];
            if (t->cfg_filter->enabled) {
                only_global = false;
                break;
            }
        }
        if (only_global) {
            if (act->get("resp_err") == 0) {
                MY_ERROR << "[" << port << "] at least 1 of the followings has to be turned on: 'always-on', 'always-log-error' or a trace filter" << endl;
                return false;
            }
            continue;
        }


        ///check trace overlapping
        for (unsigned j = 0; j < man->port_traces[port].size() - 1; ++j) for (unsigned k = j + 1; k < man->port_traces[port].size(); ++k) {
            trace* t0 = man->port_traces[port][j];
            trace* t1 = man->port_traces[port][k];
            if (t0->cfg_filter->enabled && t1->cfg_filter->enabled && t0->cfg_filter->overlap(t1->cfg_filter)) {
                MY_ERROR << "[" << port << "] trace overlapping detected: " << t0->id << " vs " << t1->id << endl;
                return false;
            }
        }


        unsigned alloc_filter_num = 0;
        vector<asset::filter*> overlaps;
        for (unsigned j = 0; j < man->port_traces[port].size(); ++j) {
            trace* t = man->port_traces[port][j];
            overlaps.clear();
            for (unsigned k = 0; k < asset::instance().filters[port].size(); ++k) {
                asset::filter* f = asset::instance().filters[port][k];
                if (!f->enabled) continue;
                if (t->cfg_filter->overlap(f)) {
                    overlaps.push_back(f);
                    if (f->overlapped) {
                        MY_ERROR << "[" << port << "] trace '" << t->id << "' overlaps with filter '" << f->id << "' that already overlaps with other filters, check the previous [overlap] messages" << endl;
                        return false;
                    }
                    if (f->runt) {
                        MY_ERROR << "[" << port << "] trace '" << t->id << "' overlaps with filter '" << f->id << "' that is too small, check the previous [tiny-range] messages" << endl;
                        return false;
                    }
                    MY_DEBUG << "[" << port << "] trace '" << t->id << "' overlaps with filter '" << f->id << "'" << endl;
                }
            }

            if (overlaps.size() == 0) {
                MY_DEBUG << "[" << port << "] trace '" << t->id << "' overlaps with nothing, creating 1 filter for it" << endl;
                asset::filter* newf = asset::instance().create(port, man->port_filters[port].size());
                man->port_filters[port].push_back(newf);
                man->map_filter2trace[newf] = t;
                ++alloc_filter_num;
                newf->copy(t->cfg_filter);
                //newf->get<asset::action*>("control")->set("read_access", 1);
                //newf->get<asset::action*>("control")->set("write_access", 1);
                //newf->get<asset::action*>("control")->set("log_req", 1);
                if (DEBUG_MODE) {
                    console::instance() << "==****** DEBUG ******==" << endl;
                    newf->show(4, 40);
                }
            }
            else {
                vector<asset::filter*> newfs;
                if (!resolve_overlap(man, overlaps, t->cfg_filter, port, man->port_filters[port].size(), newfs)) {
                    for (unsigned k = 0; k < newfs.size(); ++k) delete newfs[k];
                    return false;
                }

                if (!option::instance().has("suppress_warning", "resolve-overlap")) {
                    MY_WARNING << "[resolve-overlap][" << port << "] trace '" << t->id << "' overlaps with " << overlaps.size() << " filters '" << config::instance().join_list_dec(overlaps) << "', creating " << newfs.size() << " filters to resolve the overlap" << endl;
                }
                else {
                    MY_INFO << "[" << port << "] trace '" << t->id << "' overlaps with " << overlaps.size() << " filters '" << config::instance().join_list_dec(overlaps) << "', creating " << newfs.size() << " filters to resolve the overlap" << endl;
                }
                ++man->setup_warnings;

                for (unsigned k = 0; k < newfs.size(); ++k) {
                    newfs[k]->get<asset::action*>("control")->set("log_req", 1);
                    newfs[k]->get<asset::action*>("control")->set("log_resp", t->cfg_filter->get<asset::action*>("control")->get("log_resp"));
                    if (DEBUG_MODE) {
                        console::instance() << "==****** DEBUG ******==" << endl;
                        newfs[k]->show(4, 40);
                    }
                    man->port_filters[port].push_back(newfs[k]);
                    man->map_filter2trace[newfs[k]] = t;
                }
                alloc_filter_num += newfs.size();
            }
            
        }
        MY_DEBUG << "[" << port << "] global action =======" << endl;
        if (DEBUG_MODE) {
            console::instance() << "==****** DEBUG ******==" << endl;
            asset::instance().get<asset::action*>(port, "trace")->show(4, 40);
        }
        MY_INFO << "[" << port << "] " << alloc_filter_num << " filters are generated to activate " << man->port_traces[port].size() << " traces" << endl;
    }

    return true;
}



typedef struct {
    uint64_t        p;  //point
    asset::filter*  f;  //filter
    bool            s;  //start or end
    bool            o;  //overlapped or the cover (overlapping)
} overlap_point_t;

static bool compare_overlap_point_t(overlap_point_t* p0, overlap_point_t* p1)
{
    return p0->p < p1->p;
}

bool  trace_elba_axi::resolve_overlap(trace::man* man, const vector<asset::filter*>& overlaps, asset::filter* f, const string& port, unsigned start_id, vector<asset::filter*>& r)
{
    if (f->get<asset::condition*>("addr_range")->get("addr_lo") == f->get<asset::condition*>("addr_range")->get("addr_hi")) {
        assert(overlaps.size() == 1);
        asset::filter* newf = asset::instance().create(port, start_id++);
        r.push_back(newf);
        newf->copy(f, true, false);
        newf->copy(overlaps[0], false, true);
        return true;
    }

    ///////
    vector<overlap_point_t*> points;
    overlap_point_t* point;

    for (unsigned i = 0; i < overlaps.size(); ++i) {
        point = new overlap_point_t;
        point->p = overlaps[i]->get<asset::condition*>("addr_range")->get("addr_lo");
        point->f = overlaps[i];
        point->s = true;
        point->o = true;
        points.push_back(point);
        point = new overlap_point_t;
        point->p = overlaps[i]->get<asset::condition*>("addr_range")->get("addr_hi");
        point->f = overlaps[i];
        point->s = false;
        point->o = true;
        points.push_back(point);
    }
    point = new overlap_point_t;
    point->p = f->get<asset::condition*>("addr_range")->get("addr_lo");
    point->f = f;
    point->s = true;
    point->o = false;
    points.push_back(point);
    point = new overlap_point_t;
    point->p = f->get<asset::condition*>("addr_range")->get("addr_hi");
    point->f = f;
    point->s = false;
    point->o = false;
    points.push_back(point);
    std::sort(points.begin(), points.end(), compare_overlap_point_t);

    ///====
    asset::filter* overlap_processing = 0;
    asset::filter* newf = 0;
    r.clear();
    for (unsigned i = 0; i < points.size(); ++i) {
        point = points[i];
        if (point->s) {
            if (point->o) {
                //cout << "xxxxxxxxxxxxx 1" << endl;
                assert(overlap_processing == 0);
                overlap_processing = point->f;
                if (newf) {
                    if (point->p == newf->get<asset::condition*>("addr_range")->get("addr_lo")) {
                        newf->copy(point->f, false, true);
                    }
                    else {
                        newf->get<asset::condition*>("addr_range")->set("addr_hi", point->p - 1);
                        newf = asset::instance().create(port, start_id++);
                        r.push_back(newf);
                        newf->copy(f);
                        newf->copy(point->f, false, true);
                        newf->get<asset::condition*>("addr_range")->set("addr_lo", point->p);
                    }
                } 
            }
            else {
                //cout << "xxxxxxxxxxxxx 3" << endl;
                assert(newf == 0);
                newf = asset::instance().create(port, start_id++);
                r.push_back(newf);
                newf->copy(f);
                if (overlap_processing) {
                    newf->copy(overlap_processing, false, true);
                    newf->get<asset::condition*>("addr_range")->set("addr_lo", point->p);
                }
                else if (points[1]->o && !points[1]->s && points[1]->p == point->p) {
                    newf->copy(points[1]->f, false, true);
                    newf->get<asset::condition*>("addr_range")->set("addr_lo", point->p);
                    newf->get<asset::condition*>("addr_range")->set("addr_hi", point->p);
                    newf = asset::instance().create(port, start_id++);
                    r.push_back(newf);
                    newf->copy(f);
                    newf->get<asset::condition*>("addr_range")->set("addr_lo", point->p + 1);
                }
            }
        }
        else {
            if (point->o) {
                //cout << "xxxxxxxxxxxxx 2" << endl;
                assert(overlap_processing);
                if (newf) {
                    newf->copy(overlap_processing, false, true);
                    newf->get<asset::condition*>("addr_range")->set("addr_hi", point->p);
                    
                    if (!(!points[points.size() - 2]->s && !points[points.size() - 1]->s && points[points.size() - 2]->p == points[points.size() - 1]->p)) {
                        newf = asset::instance().create(port, start_id++);
                        r.push_back(newf);
                        newf->copy(f);
                        newf->get<asset::condition*>("addr_range")->set("addr_lo", point->p + 1);
                    }
                }
                overlap_processing = 0;
            }
            else {
                //cout << "xxxxxxxxxxxxx 4" << endl;
                assert(newf);
                if (overlap_processing) {
                    newf->get<asset::condition*>("addr_range")->set("addr_hi", point->p);
                }
                else if (points[points.size() - 2]->o && points[points.size() - 2]->s && points[points.size() - 2]->p == point->p) {
                    newf->get<asset::condition*>("addr_range")->set("addr_hi", point->p - 1);
                    newf = asset::instance().create(port, start_id++);
                    r.push_back(newf);
                    newf->copy(f);
                    newf->copy(points[points.size() - 2]->f, false, true);
                    newf->get<asset::condition*>("addr_range")->set("addr_lo", point->p);
                    newf->get<asset::condition*>("addr_range")->set("addr_hi", point->p);
                }
                else {
                    newf->get<asset::condition*>("addr_range")->set("addr_hi", point->p);
                }
                newf = 0;
                break;
            }
        }
    }
    

    ///====
    for (unsigned i = 0; i < points.size(); ++i) delete points[i];
    return true;
}



bool trace_elba_axi::allocate(trace::man* man)
{

    for (unsigned i = 0; i < asset::instance().ports.size(); ++i) {
        string& port = asset::instance().ports[i];
        if (!asset::instance().active(port)) continue;
        if (man->port_traces[port].size() == 0) continue;
    
        if (man->port_filters[port].size() == 0) {
            ///handle global trace already
            continue;
        }

        bool enough = false;
        unsigned filters_to_allocate = man->port_filters[port].size();
        unsigned filters_available = 0;
        unsigned cutoff_id = 0;
        for (int j = (int)asset::instance().filters[port].size() - 1; j >= 0; --j) {
            asset::filter* f = asset::instance().filters[port][j];
            if (f->enabled) {
                ++filters_to_allocate;
                ++filters_available;
            }
            else {
                ++filters_available;
            }

            if (filters_available >= filters_to_allocate) {
                enough = true;
                cutoff_id = (unsigned)j;
                break;
            }
        }

        if (!enough) {
            MY_ERROR << "[" << port << "] insufficient available filters to allocate " << man->port_filters[port].size() << " filters that are generated by " << man->port_traces[port].size() << " traces" << endl;
            return false;
        }

        assert(!asset::instance().filters[port][cutoff_id]->enabled);
        unsigned cur_new_idx = 0;

        for (unsigned j = cutoff_id; j < asset::instance().filters[port].size(); ++j) {
            asset::filter* f0 = asset::instance().filters[port][j];
            if (f0->enabled) continue;

            bool alloc = false;

            for (unsigned k = j + 1; k < asset::instance().filters[port].size(); ++k) {
                asset::filter* f1 = asset::instance().filters[port][k];
                if (f1->enabled) {
                    assert(cur_new_idx == 0);
                    if (!option::instance().has("suppress_warning", "move-filter")) {
                        MY_WARNING << "[move-filter][" << port << "] Moving the existing filter '" << f1->id << "' to spot '" << f0->id << "'" << endl;
                    }
                    else {
                        MY_INFO << "[" << port << "] Moving the existing filter '" << f1->id << "' to spot '" << f0->id << "'" << endl;
                    }
                    ++man->setup_warnings;
                    f0->copy(f1);
                    f0->enabled = true;
                    f1->enabled = false;
                    f0->update(true);
                    f1->update(true);
                    alloc = true;
                    break;
                }   
            }
         
            if (!alloc) {
                assert(cur_new_idx < man->port_filters[port].size());
                asset::filter* f1 = man->port_filters[port][cur_new_idx];
                MY_INFO << "[" << port << "] Allocating the new filter '" << f1->id << "' to spot '" << f0->id << "'" << endl;
                f0->copy(f1);
                f0->enabled = true;
                f0->update(true);
                cur_new_idx++;
            }   
        }

        assert(asset::instance().filters[port][asset::instance().filters[port].size() - 1]->enabled);
        MY_INFO << "[" << port << "] trace filters are allocated successfully" << endl;
    }    
    return true;
}



} ///namespace axitrace
