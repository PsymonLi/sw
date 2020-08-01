// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#include <memory>
#include <vector>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#include "config.h"
#include "setup.h"
#include "option.h"
#include "msg_undef.inc"
#include "msg_redef.inc"



using namespace boost::property_tree;
using namespace boost::property_tree::json_parser;

namespace axitrace {


static unique_ptr<config> config_ptr = unique_ptr<config>(new config());

config* config::instancep() 
{
    return config_ptr.get();
}


config& config::instance()
{
    return *instancep();
}


config::~config()
{
}



bool config::parse(const string& filename, pt::ptree* this_tree, bool merge2trunk)
{
    pt::ptree tmp_tree;
    if (this_tree == 0) this_tree = &tmp_tree;

    try {
        pt::read_json(filename, *this_tree);
    } 
    catch (const json_parser_error& e) {
        MY_ERROR << "[json] '" << e.what() << endl;
        return false;
    }

    if (merge2trunk) {
        travel2merge(*this_tree, "");
    }


    return true;
}



void config::travel2display(const pt::ptree& subtree, const string& pathstr)
{
    int i = 0;
    BOOST_FOREACH(const ptree::value_type& v, subtree) {
        string p;
        string k;
        if (v.first == "") {
            char sz[64];
            snprintf(sz, sizeof(sz), "[%d]", i++);
            k = sz;
        } 
        else {
            k = v.first;
        }

        if (pathstr == "") {
            p = k;
        }
        else {
            p = pathstr + "." + k;
        }

        if (v.second.empty()) {
            cout << "== " << p << " : " << v.second.data() << endl;
        } 
        else {
            travel2display(v.second, p);
        }
    }
}



void config::travel2merge(pt::ptree& subtree, const string& pathstr)
{
    BOOST_FOREACH(ptree::value_type& v, subtree) {
        string p;
        if (pathstr == "") {
            p = v.first;
        }
        else {
            p = pathstr + "." + v.first;
        }

        if (v.first == "") {
            boost::optional<ptree&> optiont = tree.get_child_optional(pathstr);
            if (optiont) {
                optiont.get().swap(subtree);
            } 
            else {
                tree.add(pathstr, "dummy");
                tree.get_child(pathstr).swap(subtree);
            }
            return;
        }
        else if (v.second.empty()) {
            boost::optional<ptree::data_type> optionv = tree.get_optional<ptree::data_type>(p);
            if (optionv) {
                tree.put(p, v.second.data());
            } 
            else {
                tree.add(p, v.second.data());
            }
        } 
        else {
            travel2merge(v.second, p);
        }
    }
}



bool config::test_json()
{
    pt::ptree this_tree;
    if (!parse(setup::instance().cfg + "/test.cfg", &this_tree)) {
        cout << "ERROR: syntax error in '" << setup::instance().cfg + "/test.cfg" << endl;
        return false;
    }

    this_tree.clear();
    if (!parse(setup::instance().cfg + "/test2.cfg", &this_tree)) {
        cout << "ERROR: syntax error in '" << setup::instance().cfg + "/test2.cfg" << endl;
        return false;
    }


    cout << "[axitrace][INFO] Displaying the whole cfg tree ... " << endl;
    travel2display(tree, "");

    return true;
}


template<> long long config::to_int<long long>(const string& v) const
{
    long long intv;
    char sz[64];
    unsigned j = 0;
    for (unsigned i = 0; i < v.length() && j < sizeof(sz) - 1; i++) {
        if (v[i] != '_' && v[i] != '-') {
            sz[j++] = v[i];
        }
    }
    sz[j] = '\0';

    if (v == "true") {
        intv = 1;
    }
    else if (v == "false") {
        intv = 0;
    }
    else if (v.length() > 2 && (v.substr(0, 2) == "0x" || v.substr(0, 2) == "0X")) {
        intv = strtoll(sz, 0, 16);
    }
    else if (v.empty()) {
        intv = 0;
    }
    else {
        intv = strtoll(sz, 0, 10);
    }

    return intv;

}


template<> uint64_t config::to_int<uint64_t>(const string& v) const
{
    return (uint64_t)to_int<long long>(v);
}


template<> uint32_t config::to_int<uint32_t>(const string& v) const
{
    return (uint32_t)to_int<long long>(v);
}


template<> uint8_t config::to_int<uint8_t>(const string& v) const
{
    return (uint8_t)to_int<long long>(v);
}


template<> vector<uint8_t> config::to_int<vector<uint8_t> > (const string& v) const
{
    vector<uint8_t> intv;
    vector<string> str_a = split_list(v);

    for (unsigned i = 0; i < str_a.size(); i++) {
        intv.push_back(to_int<uint8_t>(str_a[i]));
    }

    return intv;
}


void  config::to_int(const string& v, uint8_t* d, uint32_t size) const
{
    memset(d, 0, size);
    vector<uint8_t> intv = to_int<vector<uint8_t> >(v);
    for (unsigned i = 0; i < size && i < intv.size(); i++) {
        d[i] = intv[i];
    }
}


template <>
vector<string> config::split_list<string>(const string& v) const
{
    vector<string> ret;
    vector<char*> str_a;
    char* buff;
    char* p;
    char* p2;

    if (v.empty()) {
        ret.push_back("");
        return ret;
    }

    buff = new char[v.length() + 16];
    strncpy(buff, v.c_str(), v.length() + 2);
    p = buff + v.length() - 1;
    while (*p == ' ' || *p == '\t' || *p == ',') {
        *p = '\0';
        --p;
    }


    p = buff;
    while (p != 0 && *p != '\0') {
        while (*p == ' ' || *p == '\t' || *p == ',') {
            *p = '\0';
            ++p;
        }
        p2 = 0;
        if (*p != '\0') {
            str_a.push_back(p);
            p2 = strchr(p, ',');
        }
        if (*p != '\0' && p2 == 0) {
            p2 = strchr(p, ' ');
        }
        if (*p != '\0' && p2 == 0) {
            p2 = strchr(p, '\t');
        }
        p = p2;
        if (p2 != 0) --p2;
        while (p2 != 0 && (*p2 == ' ' || *p2 == ',' || *p2 == '\t')) {
            *p2 = '\0';
            --p2;
        }
    }

    for (unsigned i = 0; i < str_a.size(); i++) {
        ret.push_back(string(str_a[i]));
    }

    delete [] buff;
    return ret;
}


template<>
string config::join_list(const vector<string>& l, const string& delimiter) const
{
    string r = "";
    bool first = true;
    for (vector<string>::const_iterator it = l.begin(); it != l.end(); it++) {
        if (first) {
            r += *it;
            first = false;
        }
        else {
            r += delimiter + *it;
        }
        
    }
    return r;
}



template<>
string config::join_list_hex(const vector<uint64_t>& l, const string& delimiter) const
{
    string r = "";
    bool first = true;
    for (unsigned i = 0; i < l.size(); ++i) {
        if (first) {
            r += to_hex(l[i]);
            first = false;
        }
        else {
            r += delimiter + to_hex(l[i]);
        }
        
    }
    return r;
}



template<>
string config::join_list_hex(const vector<unsigned>& l, const string& delimiter) const
{
    string r = "";
    bool first = true;
    for (unsigned i = 0; i < l.size(); ++i) {
        if (first) {
            r += to_hex(l[i]);
            first = false;
        }
        else {
            r += delimiter + to_hex(l[i]);
        }
        
    }
    return r;
}



template<>
string config::join_list_dec(const vector<uint64_t>& l, const string& delimiter) const
{
    string r = "";
    bool first = true;
    for (unsigned i = 0; i < l.size(); ++i) {
        if (first) {
            r += to_dec(l[i]);
            first = false;
        }
        else {
            r += delimiter + to_dec(l[i]);
        }
        
    }
    return r;
}



template<>
string config::join_list_dec(const vector<unsigned>& l, const string& delimiter) const
{
    string r = "";
    bool first = true;
    for (unsigned i = 0; i < l.size(); ++i) {
        if (first) {
            r += to_dec(l[i]);
            first = false;
        }
        else {
            r += delimiter + to_dec(l[i]);
        }
        
    }
    return r;
}


template<>
string config::join_list_dec(const vector<asset::filter*>& l, const string& delimiter) const
{
    string r = "";
    bool first = true;
    for (unsigned i = 0; i < l.size(); ++i) {
        if (first) {
            r += to_dec(l[i]->id);
            first = false;
        }
        else {
            r += delimiter + to_dec(l[i]->id);
        }
        
    }
    return r;
}


string config::to_hex(uint64_t v) const
{
    char sz[64];
    snprintf(sz, sizeof(sz), "0x%llx", (long long)v);
    return string(sz);
}


string config::to_dec(uint64_t v) const
{
    char sz[64];
    snprintf(sz, sizeof(sz), "%lld", (long long)v);
    return string(sz);
}



bool config::to_bool(const string& v) const
{
    if (v == "true" || v == "1") return true;
    return false;
}



bool config::setup()
{
    if (!config::instance().parse(setup::instance().cfg + "/driver_calibrate.cfg")) return false;
    if (!config::instance().parse(setup::instance().cfg + "/" + axitrace::option::instance().opt["asic"][0].get() + ".cfg")) return false;
    for (unsigned i = 0; i < axitrace::option::instance().opt["cfg"].size(); i++) {
        if (!config::instance().parse(axitrace::option::instance().opt["cfg"][i].get())) return false;
    }
    if (!add_options()) return false;
    return true;
}



bool config::add_options()
{
    if (option::inst()["trace"].size() > 0) {
        boost::optional<ptree&> ot;
        if (ot = tree.get_child_optional(asset::instance().chip() + ".axi-trace.traces")) {
            ot.get().clear();
        }
        ptree traces;
        unsigned id = 1;

        for (unsigned i = 0; i < option::inst()["trace"].size(); ++i) {
            char szcfg[1024];
            strncpy(szcfg, option::inst()["trace"][i].get().c_str(), sizeof(szcfg));
            char *p0, *p1;
            p0 = szcfg;
            while (*p0 == ' ') ++p0;
            if (*p0 == '\0') goto parse_failure;
            p1 = szcfg + strlen(szcfg);
            if (p1 <= p0) goto parse_failure;
            --p1;
            while (*p1 == ' ') {
                *p1 = '\0';
                --p1;
            }
            while (p0 && *p0 != '\0') {
                p1 = strchr(p0, ' ');
                while (p1 && *p1 == ' ') {
                    *p1 = '\0';
                    ++p1;
                }

                string ports;
                string addr_lo;
                string addr_hi;
                bool always_on = false;
                bool always_log_error = false;

                char* pp;

                if ((pp = strchr(p0, ':'))) {
                    *pp = '\0';
                    ports = p0;
                    p0 = pp + 1;
                }
                else {
                    ports = "all";
                }

                if (strchr(p0, ':')) goto parse_failure;

                if (strcmp(p0, "always-on") == 0) {
                    always_on = true;
                }
                else if (strcmp(p0, "always-log-error") == 0) {
                    always_log_error = true;
                }
                else if ((pp = strchr(p0, '-'))) {
                    *pp = '\0';
                    addr_lo = p0;
                    p0 = pp + 1;
                    addr_hi = p0;
                }
                else {
                    addr_lo = p0;
                    addr_hi = p0;
                }


                //////
                ptree entry;
                entry.add("id", config::instance().to_dec(id++));
                entry.add("ports", ports);
                if (always_on) {
                    entry.add("always-on", "true");
                }
                else if (always_log_error) {
                    entry.add("always-log-error", "true");
                }
                else {
                    entry.add("addr-lo", addr_lo);
                    entry.add("addr-hi", addr_hi);
                }
                traces.push_back(std::make_pair("", entry));

                //////
                p0 = p1;
            }

            continue;
parse_failure:
            MY_ERROR << " Parsing failed for --trace \"" << option::inst()["trace"][i].get() << "\"" << endl;
            return false;
        }

        if (tree.get_child_optional(asset::instance().chip() + ".axi-trace.traces")) {
            tree.put_child(asset::instance().chip() + ".axi-trace.traces", traces);
        }
        else {
            tree.add_child(asset::instance().chip() + ".axi-trace.traces", traces);
        }

        if (DEBUG_MODE) {
            debug("debug0");
        }
    }

    return true;
}


void config::debug(const string& fname) const
{
    pt::json_parser::write_json(axitrace::option::inst()["odir"][0].get() + "/" + fname + ".json", tree);
}


} //namespace axitrace
