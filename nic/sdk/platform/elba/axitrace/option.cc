// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#include "option.h"
#include <sys/stat.h>
#include <memory>
#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <cstring>
#include <regex>
#include "rand.h"
#include "generic.h"


#include "msg_undef.inc"
#include "msg_redef.inc"

namespace axitrace {

static unique_ptr<option> option_ptr = unique_ptr<option>(new option());


void option::usage()
{
    cout << "axitrace [<options>]" << endl;
    cout << endl;
    cout << "options:" << endl << endl;
    cout << "-h, --help\t\t\t: print this help message" << endl;
    cout << "-t, --trace <address ranges>\t: trace address, format is '<PORT>:<ADDR>' | '<PORT>:<ADDR_LO>-<ADDR_HI>', can be multiply applied" << endl;
    cout << "-c, --cfg <file>\t\t: cfg files, can be multiply applied" << endl;
    cout << "--dry-run\t\t\t: dry-run mode, no write to asic" << endl;
    cout << "-f, --force\t\t\t: force-run mode, continue programming asic even error/warning detected" << endl;
    cout << "--no-restore\t\t\t: do not restore original CSR registers if ever changed" << endl;
    cout << "--cleanup\t\t\t: clean up existing traces if any" << endl;
    cout << "--show <ports>\t\t\t: read and print filter registers, can be multiply applied, or 'all'" << endl;
    cout << "--big-endian\t\t\t: print out memory data in ascending address order" << endl;
    cout << "--clean-console\t\t\t: cleanup console.log when this program launches, defaultly console.log is appending" << endl;
    cout << "--suppress-log <string>\t\t: don't dump the corresponding log files, e.g. all (excluding stdout) | stdout | console | trace_.* " << endl;
    cout << "--error-to-warning <string>\t: downgrade error to warning messages, can be multiply applied : tokens can be found in the messages" << endl;
    cout << "--suppress-warning <string>\t: suppress warning messages, can be multiply applied : tokens can be found in the messages" << endl;
    cout << "-o, --output-dir <dir>\t\t: output directory, default is './axitrace.reports'" << endl;
    cout << "--debug\t\t\t\t: debug mode" << endl;
    //cout << "--mode <string>\t\t\t: working modes: sw(default) | haps" << endl;
    //cout << "--calibr\t\t\t: driver calibration" << endl;
    //cout << "-s, --seed <int> \t\t: seed for randomization" << endl;
}



bool option::parse(int argc, char* const* argv)
{   
    int c;
    int index;
    struct stat stat_buff;
    struct ::option getopt_options[] = {
        {"cfg",                         required_argument, 0, 'c'}
    ,   {"seed",                        required_argument, 0, 's'}
    ,   {"trace",                       required_argument, 0, 't'}
    ,   {"force",                       no_argument,       0, 'f'}
    ,   {"help",                        no_argument,       0, 'h'}
    ,   {"output-dir",                  no_argument,       0, 'o'}
    ,   {"asic",                        required_argument, 0, 0x70000001}
    ,   {"show",                        required_argument, 0, 0x70000002}
    ,   {"clean-console",               no_argument,       0, 0x70000003}
    ,   {"error-to-warning",            required_argument, 0, 0x70000004}
    ,   {"suppress-warning",            required_argument, 0, 0x70000005}
    ,   {"dry-run",                     no_argument,       0, 0x70000006}
    ,   {"cleanup",                     no_argument,       0, 0x70000007}
    ,   {"mode",                        required_argument, 0, 0x70000008}
    ,   {"debug",                       no_argument,       0, 0x70000009}
    ,   {"calibr",                      no_argument,       0, 0x7000000a}
    ,   {"job",                         required_argument, 0, 0x7000000b}
    ,   {"no-restore",                  no_argument,       0, 0x7000000c}
    ,   {"big-endian",                  no_argument,       0, 0x7000000d}
    ,   {"suppress-log",                required_argument, 0, 0x7000000e}
    ,   {0, 0, 0, 0}
    };


    opt["driver_calibrate_mode"].push_back(option_val("false"));
    opt["debug_mode"].push_back(option_val("false"));
    opt["asic"].push_back(option_val("elba"));
    opt["mode"].push_back(option_val("sw"));
    opt["odir"].push_back(option_val("./axitrace.reports"));
    opt["clean_console"].push_back(option_val("false"));
    opt["dry_run"].push_back(option_val("false"));
    opt["force"].push_back(option_val("false"));
    opt["clean"].push_back(option_val("false"));
    opt["restore"].push_back(option_val("true"));
    opt["reverse_bytes"].push_back(option_val("false"));

    while ((c = getopt_long(argc, argv, "hfc:s:o:t:", getopt_options, &index)) != -1) {
        switch (c) {
            case 'h': 
                usage();
                return false;

            case 's': 
                {
                int seed = atoi(::optarg);
                cout << "[axitrace][INFO] Setting seed to '" << seed << "'" << endl;
                axitrace::rand::instance().seed(seed);
                }
                break;
    
            case 'o':
                opt["odir"][0].set(::optarg);
                break;

            case 0x7000000a:
                opt["driver_calibrate_mode"][0].set("true");
                break;

            case 'c':
                if (stat(::optarg, &stat_buff) != 0) {
                    MY_ERROR << " --cfg, cfg file '" << ::optarg << "' doesn't exist" << endl;
                    return false;
                }
                opt["cfg"].push_back(option_val(::optarg));
                break;

            case 't':
                opt["trace"].push_back(option_val(::optarg));
                break;

            case 0x70000009:
                opt["debug_mode"][0].set("true");
                break;

            case 0x70000008:
                if (strcmp(::optarg, "sw") != 0 && strcmp(::optarg, "haps") != 0) {
                    MY_ERROR << " --mode, mode '" << ::optarg << "' is unknown" << endl;
                    return false;
                }
                opt["mode"][0].set(::optarg);
                break;

            case 'f':
                opt["force"][0].set("true");
                break;

            case 0x70000001:
                opt["asic"][0].set(::optarg);
                break;

            case 0x70000002:
                opt["show_ports"].push_back(option_val(::optarg));
                break;
            
            case 0x70000003:
                opt["clean_console"][0].set("true");
                break;

            case 0x70000004:
                m_lkup["error2warning"][::optarg] = false;
                break;
            
            case 0x70000005:
                m_lkup["suppress_warning"][::optarg] = false;
                break;
            
            case 0x70000006:
                opt["dry_run"][0].set("true");
                break;
            
            case 0x70000007:
                opt["clean"][0].set("true");
                break;
            
            case 0x7000000b:
                opt["job"].push_back(option_val(::optarg));
                break;
            
            case 0x7000000c:
                opt["restore"][0].set("false");
                break;

            case 0x7000000d:
                opt["reverse_bytes"][0].set("true");
                break;

            case 0x7000000e:
                m_lkup["suppress_log"][::optarg] = true;
                break;
            
            case '?': return false;
        }
    }

    return true;
}


option::~option()
{
}


option* option::instancep() 
{
    return option_ptr.get();
}


option& option::instance()
{
    return *instancep();
}


map<string, vector<option_val> >& option::inst()
{
    return instancep()->opt;
}


option_val::option_val(const char* v)
    : m_val(v)
{
}


void option_val::set(const char* v)
{
    m_val = string(v);
}


template<> bool option_val::get<bool>() const
{
    if (m_val == "true" || m_val == "1") return true;
    else return false;
}


template<> const string& option_val::get<const string&>() const
{
    return m_val;
}


bool option::has(const string& kw0, const string& kw1) const
{
    if (m_lkup.find(kw0) == m_lkup.end()) return false;
    
    bool regmatch = false;

    for (map<string, bool>::const_iterator it = m_lkup.at(kw0).begin(); it != m_lkup.at(kw0).end(); it++) {
        if (it->second) {
            regmatch = true;
            break;
        }
    }

    
    if (!regmatch) {
        if (m_lkup.at(kw0).find(kw1) == m_lkup.at(kw0).end()) return false;
        return true;
    }
    else {
        for (map<string, bool>::const_iterator it = m_lkup.at(kw0).begin(); it != m_lkup.at(kw0).end(); it++) {
            std::regex re(it->first);
            if (std::regex_match(kw1, re)) return true;
        }
        return false;
    }
}




} // namespace axitrace
