// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#include <memory>
#include "asset.h"
#include "asset_elba.h"
#include "config.h"
#include "generic.h"
#include "msg_undef.inc"
#include "msg_redef.inc"
#include "option.h"
#include "pio.h"
#include "console.h"
#include "task.h"



namespace axitrace {


static unique_ptr<asset_elba> asset_elba_ptr = unique_ptr<asset_elba>(new asset_elba());
bool asset::element_skip_build_fields = false;



asset::asset()
{
}


asset::~asset()
{
    for (asset::filters_t::iterator it = filters.begin(); it != filters.end(); it++) for (vector<filter*>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++) {
        delete *it2;
    }
    for (map<string, vector<action*> >::iterator it2 = actions.begin(); it2 != actions.end(); it2++) for (vector<action*>::iterator it = it2->second.begin(); it != it2->second.end(); it++) {
        delete *it;
    }
    for (map<string, vector<condition*> >::iterator it2 = conditions.begin(); it2 != conditions.end(); it2++) for (vector<condition*>::iterator it = it2->second.begin(); it != it2->second.end(); it++) {
        delete *it;
    }
    for (vector<filter*>::iterator it = spare_filters.begin(); it != spare_filters.end(); it++) {
        delete *it;
    }
}


asset* asset::instancep()
{
    if (axitrace::option::instance().opt["asic"][0].get() == "elba")
        return asset_elba_ptr.get();
    else {
        MY_ERROR << "'asic' is not defined correctly (only 'elba' is supported currently), please use --asic" << endl;
        exit(1);
    }
}


asset& asset::instance()
{
    return *instancep();
}


asset::element::element(const string& nm, asset::filter* f)
    : name(nm), filter(f)
{
    enabled = false;
    oenabled = false;
    updated = false;
    oupdated = false;
    global = false;
    readonly = false;
}


asset::element::~element()
{
}


void asset::element::clear()
{
    fields.clear();
    vals.clear();
    ovals.clear();
    map_field2idx.clear();
}


void asset::element::add_field(const string& field)
{
    if (!asset::element_skip_build_fields) {
        map_field2idx[field] = fields.size();
        fields.push_back(field);
        vals.push_back(0);
        ovals.push_back(0);
    }
}


unsigned asset::element::size() const
{
    return fields.size();
}


void asset::element::zero()
{
    for (unsigned i = 0; i < size(); ++i) {
        vals[i] = 0;
    }
    update(true);
}


void asset::element::copy(const element* o) 
{
    enabled = o->enabled;
    fields = o->fields;
    vals = o->vals;
    map_field2idx = o->map_field2idx;
    
    if (ovals.size() != vals.size()) {
        oenabled = o->oenabled;
        ovals = o->ovals;
    }
    assert(ovals.size() == vals.size());
    assert(ovals.size() == fields.size());
    assert(ovals.size() == map_field2idx.size());

    update(true);
}



uint64_t asset::element::get(const string& field, bool origin) const
{
    assert(map_field2idx.find(field) != map_field2idx.end());
    return get(map_field2idx.at(field), origin);
}


uint64_t asset::element::get(unsigned idx, bool origin) const
{
    assert(idx < vals.size() && idx < ovals.size());
    if (origin) return ovals[idx];
    else return vals[idx];
}


void asset::element::set(const string& field, uint64_t v, bool origin)
{
    assert(map_field2idx.find(field) != map_field2idx.end());
    set(map_field2idx[field], v, origin);
}


void asset::element::set(unsigned idx, uint64_t v, bool origin)
{
    assert(idx < vals.size() && idx < ovals.size());
    if (origin) ovals[idx] = v;
    else vals[idx] = v;
}


bool asset::element::restore()
{
    assert(vals.size() == ovals.size());
    enabled = oenabled;
    vals = ovals;
    update(true);
    oupdated = false;
    return true;
}


asset::condition::condition(const string& nm, asset::filter* f)
    : element(nm, f)
{
}


asset::condition::~condition()
{
}


asset::action::action(const string& nm, asset::filter* f)
    : element(nm, f)
{
}


asset::action::~action()
{
}


asset::filter::filter(const string& argport, int argid)
    : port(argport)
    , id(argid)
{
    enabled = false;
    oenabled = false;
    updated = false;
    oupdated = false;
    overlapped = false;
    runt = false;
}


void asset::filter::copy(const filter* o, bool copy_condition, bool copy_action)
{
    enabled = o->enabled;
    assert(conditions.size() == o->conditions.size());
    assert(actions.size() == o->actions.size());

    if (copy_condition) {
        for (unsigned i = 0; i < conditions.size(); ++i) {
            conditions[i]->copy(o->conditions.at(i));
        }
    }
    if (copy_action) {
        for (unsigned i = 0; i < actions.size(); ++i) {
            actions[i]->copy(o->actions.at(i));
        }
    }
    update(true);
}



asset::filter::~filter()
{
    for (vector<condition*>::iterator it = conditions.begin(); it != conditions.end(); it++) {
        delete *it;
    }
    for (vector<action*>::iterator it = actions.begin(); it != actions.end(); it++) {
        delete *it;
    }
}


bool asset::filter::restore()
{
    for (unsigned i = 0; i < conditions.size(); ++i) if (!conditions[i]->restore()) return false;
    for (unsigned i = 0; i < actions.size(); ++i) if (!actions[i]->restore()) return false;
    enabled = oenabled;
    update(true);
    oupdated = false;
    return true;
}


template<> asset::condition* asset::filter::get<asset::condition*>(const string& nm)
{
    assert(map_condition_name2idx.find(nm) != map_condition_name2idx.end());
    assert(map_condition_name2idx[nm] < conditions.size());
    return conditions[map_condition_name2idx[nm]];
}


template<> asset::condition* asset::filter::get<asset::condition*>(unsigned idx)
{
    assert(idx < conditions.size());
    return conditions[idx];
}



template<> asset::action* asset::filter::get<asset::action*>(const string& nm)
{
    assert(map_action_name2idx.find(nm) != map_action_name2idx.end());
    assert(map_action_name2idx[nm] < actions.size());
    return actions[map_action_name2idx[nm]];
}


template<> asset::action* asset::filter::get<asset::action*>(unsigned idx)
{
    assert(idx < actions.size());
    return actions[idx];
}



template<> asset::action* asset::get<asset::action*>(const string& port, const string& nm)
{
    assert(map_action_name2idx.find(port) != map_action_name2idx.end());
    assert(map_action_name2idx[port].find(nm) != map_action_name2idx[port].end());
    assert(actions.find(port) != actions.end());
    assert(map_action_name2idx[port][nm] < actions[port].size());
    return actions[port][map_action_name2idx[port][nm]];
}


template<> asset::action* asset::get<asset::action*>(const string& port, unsigned idx)
{
    assert(actions.find(port) != actions.end());
    assert(idx < actions[port].size());
    return actions[port][idx];
}


template<> asset::condition* asset::get<asset::condition*>(const string& port, const string& nm)
{
    assert(map_condition_name2idx.find(port) != map_condition_name2idx.end());
    assert(map_condition_name2idx[port].find(nm) != map_condition_name2idx[port].end());
    assert(conditions.find(port) != conditions.end());
    assert(map_condition_name2idx[port][nm] < conditions[port].size());
    return conditions[port][map_condition_name2idx[port][nm]];
}


template<> asset::condition* asset::get<asset::condition*>(const string& port, unsigned idx)
{
    assert(conditions.find(port) != conditions.end());
    assert(idx < conditions[port].size());
    return conditions[port][idx];
}


unsigned asset::element::show_tag(unsigned i) const
{
    return 0;
}


unsigned asset::element::show_data(unsigned i) const
{
    string s;
    if (enabled) s = "enable";
    else s = "disable";

    console::instance() << s;
    return s.length();
}


unsigned asset::element::show_data2(unsigned i) const
{
    return 0;
}


void asset::element::show(unsigned indent, unsigned tab) const
{
    for (unsigned i = 0; i < indent; ++i) console::instance() << "    ";
    unsigned bytes = show_tag();
    console::instance() << name;
    bytes += name.length();
    for (unsigned i = 0; i < tab - indent*4 - bytes; ++i) console::instance() << ' ';
    console::instance() << ": ";
    show_data();
    console::instance() << endl;
    
    for (unsigned i = 0; i < size(); ++i) {
        for (unsigned j = 0; j < indent + 1; ++j) console::instance() << "    ";
        console::instance() << fields[i];
        for (unsigned j = 0; j < tab - indent*4 - 4 - fields[i].length(); ++j) console::instance() << ' ';
        console::instance() << ": 0x" << hex << vals[i] << dec;
        show_data2(i);
        console::instance() << endl;
    }
}


void asset::filter::show(unsigned indent, unsigned tab) const
{
    for (unsigned i = 0; i < indent; ++i) console::instance() << "    ";
    console::instance() << "filter " << id;
    for (unsigned i = 0; i < tab - indent*4 - 9; ++i) console::instance() << ' ';
    console::instance() << ": ";
    if (enabled) console::instance() << "enable";
    else console::instance() << "disable";
    if (updated) console::instance() << " (updated)";
    else console::instance() << " (no updates)";
    console::instance() << endl;
    if (enabled) {
        for (unsigned i = 0; i < conditions.size(); ++i) conditions[i]->show(indent + 1, tab);
        for (unsigned i = 0; i < actions.size(); ++i) actions[i]->show(indent + 1, tab);
    }
    for (unsigned i = 0; i < indent; ++i) console::instance() << "    ";
    console::instance() << "-------" << endl;
    console::instance() << endl;
}


void asset::filter::zero() 
{
    for (unsigned i = 0; i < conditions.size(); ++i) conditions[i]->zero();
    for (unsigned i = 0; i < actions.size(); ++i) actions[i]->zero();
    update(true);
}



bool asset::read_csr(const string& port, uint64_t addr, const string& label, unsigned idx, uint32_t& data)
{
    if (!pio::instance().reg_read(addr, data)) return false;
    pt::ptree entry;
    if (idx == -1ul) {
        entry.add("label", label);
    }
    else {
        entry.add("label", label + "[" + config::instance().to_dec(idx) + "]");
    }
    entry.add("addr", config::instance().to_hex(addr));
    entry.add("data", config::instance().to_hex(data));
    config::instance()->add_child(chip() + ".rawdata.axi-filter." + port + ".read", entry);
    return true;
}


void asset::show() const
{
    unsigned ports_shown = 0;
    for (unsigned i = 0; i < ports.size(); ++i) {
        if (!active(ports[i])) continue;
        console::instance() << "== " << ports[i] << " ==" << endl;
        for (unsigned j = 0; j < filters.at(ports[i]).size(); ++j) {
            filters.at(ports[i]).at(j)->show(2, 40);
        }

        for (unsigned j = 0; j < actions.at(ports[i]).size(); ++j) {
            actions.at(ports[i]).at(j)->show(2, 40);
        }
        for (unsigned j = 0; j < conditions.at(ports[i]).size(); ++j) {
            conditions.at(ports[i]).at(j)->show(2, 40);
        }
        console::instance() << "===============" << endl;
        ++ports_shown;
    }
    console::instance() << ports_shown << " ports shown" << endl;
    console::instance() << "===============" << endl;
}



bool asset::write_csr(const string& port, uint64_t addr, uint32_t data)
{
    if (!pio::instance().reg_write(addr, data)) return false;

    string path = chip() + ".debug.write-4-reg-init." + chip() + ".reg-init";
    if (DEBUG_MODE && DEBUG_WRITE_FILTER_CFG) {
        pt::ptree entry;
        entry.add("addr", config::instance().to_hex(addr));
        entry.add("data", config::instance().to_hex(data));
        boost::optional<ptree&> optsubtree = config::instance()->get_child_optional(path);
        if (optsubtree) {
            (*optsubtree).push_back(std::make_pair("", entry));
        }
        else {
            pt::ptree subtree;
            subtree.push_back(std::make_pair("", entry));
            config::instance()->add_child(path, subtree);
        }
    }

    return true;
}



bool asset::check() const 
{
    bool ret = true;
    for (unsigned i = 0; i < ports.size(); ++i) {
        if (!active(ports[i])) continue;
        unsigned fnum = filters.at(ports[i]).size();
        for (unsigned j = 0; j < fnum; ++j) {
            asset::filter* f0 = filters.at(ports[i]).at(j);
            if (f0->enabled && (f0->get<asset::condition*>("addr_range")->get("addr_lo") == f0->get<asset::condition*>("addr_range")->get("addr_hi"))) {
                f0->runt = true;
                if (!axitrace::option::instance().has("error2warning", "tiny-range")) {
                    MY_ERROR << "[tiny-range][" << ports[i] << "] filter '" << f0->id << "' is too small" << endl;
                    ret = false;
                }
                else if (!axitrace::option::instance().has("suppress_warning", "tiny-range")) {
                    MY_WARNING << "[tiny-range][" << ports[i] << "] filter '" << f0->id << "' is too small" << endl;
                }
                else {
                    MY_INFO << "[" << ports[i] << "] filter '" << f0->id << "' is too small" << endl;
                }
            }
        }

        for (unsigned j = 0; j < fnum - 1; ++j) for (unsigned k = j + 1; k < fnum; ++k) {
            asset::filter* f0 = filters.at(ports[i]).at(j);
            asset::filter* f1 = filters.at(ports[i]).at(k);
            if (f0->enabled && f1->enabled && f0->overlap(f1)) {
                f0->overlapped = true;
                f1->overlapped = true;
                if (!axitrace::option::instance().has("error2warning", "overlap")) {
                    MY_ERROR << "[overlap][" << ports[i] << "] filter overlap detected: filter " << j << " overlapping filter " << k << endl;
                    ret = false;
                } 
                else if (!axitrace::option::instance().has("suppress_warning", "overlap")) {
                    MY_WARNING << "[overlap][" << ports[i] << "] filter overlap detected: filter " << j << " overlapping filter " << k << endl;
                }
                else {
                    MY_INFO << "[" << ports[i] << "] filter overlap detected: filter " << j << " overlapping filter " << k << endl;
                }
            }
        }
    }

    return ret;
}


bool asset::build() 
{
    task::instance().collect_active_ports();
    return true;
}



bool asset::setup() 
{
    bool ret;



    if (!build()) return false;
    ret = load();
    pt::json_parser::write_json(axitrace::option::inst()["odir"][0].get() + "/axi_filter_csr_read.json", config::instance()->get_child(chip() + ".rawdata.axi-filter"));
    if (!ret) return false;
    if (!check()) return false;

    return true;
}



void asset::element::update(bool u)
{
    updated = u;
    if (u) {
        oupdated = true;
        if (!global) filter->update(u);
    }
}


void asset::filter::update(bool u)
{
    updated = u;
    if (u) {
        oupdated = true;
    }
    else {
        for (vector<condition*>::iterator it = conditions.begin(); it != conditions.end(); it++) (*it)->update(u);
        for (vector<action*>::iterator it = actions.begin(); it != actions.end(); it++) (*it)->update(u);
    }
}



bool asset::filter::overlap(const asset::filter* f) const
{
    assert(conditions.size() == f->conditions.size());

    for (unsigned i = 0; i < conditions.size(); ++i) {
        if (conditions[i]->enabled && f->conditions[i]->enabled && !conditions[i]->overlap(f->conditions.at(i))) return false;
    }
    return true;
}



bool asset::restore()
{
    for (unsigned i = 0; i < ports.size(); ++i) {
        for (unsigned j = 0; j < filters[ports[i]].size(); ++j) {
            if (filters[ports[i]][j]->oupdated) filters[ports[i]][j]->restore();
        }
        if (get<asset::action*>(ports[i], "trace")->oupdated) get<asset::action*>(ports[i], "trace")->restore();
    }
    return true;
}



unsigned asset::condition::show_tag(unsigned i) const
{
    console::instance() << "[C] ";
    return 4;
}



unsigned asset::action::show_tag(unsigned i) const
{
    console::instance() << "[A] ";
    return 4;
}




}  //// namespace axitrace
