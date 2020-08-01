// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#include <memory>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <locale>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include "generic.h"
#include "task.h"
#include "config.h"
#include "setup.h"
#include "pio.h"
#include "rand.h"
#include "option.h"
#include "asset.h"
#include "msg_undef.inc"
#include "msg_redef.inc"
#include "trace.h"
#include "console.h"
#include "buffer_elba_axi.h"



using namespace std;

namespace axitrace {

static unique_ptr<task> task_ptr = unique_ptr<task>(new task());

void task::setup()
{
#ifdef ADD_JOB
#undef ADD_JOB
#endif
#define ADD_JOB(X) \
    job = new job_##X(#X); \
    m_jobs[job->name] = job;

    job_base* job;

    ADD_JOB(driver_calibrate);
    ADD_JOB(show_filters);
    ADD_JOB(trace);
    ADD_JOB(write_test_buffer);
}


task::task()
{
    current = 0;
    setup();
}


task::~task()
{
    for(map<string, job_base*>::iterator it = m_jobs.begin(); it != m_jobs.end(); it++) {
        delete it->second;
    }
}


task* task::instancep()
{
    return task_ptr.get();
}


task& task::instance()
{
    return *instancep();
}


bool task::run(const string& jobname)
{
    if (jobname != "") {
        if (m_jobs.find(jobname) == m_jobs.end()) {
            MY_ERROR << " Job '" << jobname << "' not found" << endl;
            return false;
        }
        current = m_jobs[jobname];
        bool ret = current->run();
        current = 0;
        return ret;
    } 
    else {
        if (axitrace::option::inst()["show_ports"].size() > 0) {
            MY_INFO << " Start polling axi filter registers ... " << endl;
            if (!run("show_filters")) {
                MY_ERROR << " Polling axi filter registers FAILED " << endl;
                return false;
            }
            MY_INFO << " Polling axi filter registers done successfully" << endl;
        }
        else if (option::inst()["job"].size() > 0) {
            string jobname = option::inst()["job"][0].get();
            MY_INFO << " Start job '" << jobname << "' ... " << endl;
            if (!run(jobname)) {
                MY_ERROR << " Job '" << jobname << "' FAILED" << endl;
                return false;
            }
            MY_INFO << " Job '" << jobname << "' done successfully" << endl;
        }
        else {
            MY_INFO << " Start tracing ... " << endl;
            if (!run("trace")) {
                MY_ERROR << " Tracing FAILED " << endl;
                return false;
            }
            MY_INFO << " Tracing done successfully" << endl;
        }
    }
    return true;
}


bool job_base::run() 
{
    if (!pre_main()) return false;
    if (!main()) return false;
    if (!post_main()) return false;
    return true;
}



bool job_driver_calibrate::main()
{
#include "msg_undef.inc"
#define MY_MSG  "[driver-calibrate] reg.seq." << op << " @[0x" << hex << addr << dec << "]: "
#include "msg_redef.inc"

#ifdef INC_PASS
#undef INC_PASS
#undef INC_FAIL
#endif
#define INC_PASS {++total_test_num; ++total_test_pass; ++reg_test_num; ++reg_test_pass;}
#define INC_FAIL {++total_test_num; ++total_test_fail; ++reg_test_num; ++reg_test_fail;}

    int total_test_num = 0;
    int total_test_pass = 0;
    int total_test_fail = 0;

    ////registers
    uint64_t reg_stash = -1ull;
    uint64_t reg_stash_addr = 0;
    int reg_test_num = 0;
    int reg_test_pass = 0;
    int reg_test_fail = 0;
    BOOST_FOREACH(const pt::ptree::value_type& vt, config::instance().tree.get_child("driver-calibrate.reg.seq")) {
        const pt::ptree& seqitem = vt.second;
        string op = seqitem.get<string>("op", "");
        uint64_t addr = (uint64_t)config::instance().to_int<long long>(seqitem.get<string>("addr", "0"));
        uint32_t data;

        if (op == "read") {
            if (!pio::instance().reg_read(addr, data)) {
                MY_ERROR << "pio reg_read failed" << endl;
                INC_FAIL;
                continue;
            }
            boost::optional<string> strv;
            if ((strv = seqitem.get_optional<string>("mask"))) {
                uint32_t intv = (uint32_t)config::instance().to_int<long long>(*strv);
                data &= intv;
            }
            if ((strv = seqitem.get_optional<string>("shift"))) {
                uint32_t intv = (uint32_t)config::instance().to_int<long long>(*strv);
                data >>= intv;
            }

            MY_INFO << "retrieved 0x" << hex << data << dec << endl;
        }
        else if (op == "read-compare") {
            if (!pio::instance().reg_read(addr, data)) {
                MY_ERROR << "pio reg_read failed" << endl;
                INC_FAIL;
                continue;
            }
            boost::optional<string> strv;
            if ((strv = seqitem.get_optional<string>("mask"))) {
                uint32_t intv = (uint32_t)config::instance().to_int<long long>(*strv);
                data &= intv;
            }
            if ((strv = seqitem.get_optional<string>("shift"))) {
                uint32_t intv = (uint32_t)config::instance().to_int<long long>(*strv);
                data >>= intv;
            }
            string verify_type = seqitem.get<string>("verify.type");
            bool verify_ok = false;
            if (verify_type == "list") {
                int num = 0;
                BOOST_FOREACH(const pt::ptree::value_type& vlist, seqitem.get_child("verify.val")) {
                    num++;
                    uint32_t compare = config::instance().to_int<uint32_t>(vlist.second.get<string>("val", ""));
                    if (data == compare) {
                        verify_ok = true;
                        break;
                    }
                }
                if (verify_ok) {
                    MY_INFO << "retrieved 0x" << hex << data << dec << ", compared against " << num << " values, verified OK" << endl;
                } 
                else {
                    MY_ERROR << "retrieved 0x" << hex << data << dec << ", compared against " << num << " values, verification FAILED" << endl;
                    INC_FAIL;
                    continue;
                }
            }
            else if (verify_type == "direct") {
                uint32_t compare = config::instance().to_int<uint32_t>(seqitem.get<string>("verify.val"));
                if (data == compare) verify_ok = true;
                if (verify_ok) {
                    MY_INFO << "retrieved 0x" << hex << data << dec << ", compared against 0x" << hex << compare << dec << ", verified OK" << endl;
                } 
                else {
                    MY_ERROR << "retrieved 0x" << hex << data << dec << ", compared against 0x" << hex << compare << dec << ", verification FAILED" << endl;
                    INC_FAIL;
                    continue;
                }
            }
        }
        else if (op == "write") {
            string str_val = seqitem.get<string>("val");
            if (str_val == "random") {
                data = (uint32_t)axitrace::rand::instance().get<int>();
            }
            else {
                data = config::instance().to_int<uint32_t>(str_val);
            }
            if (!pio::instance().reg_write(addr, data)) {
                MY_ERROR << "pio reg_write failed" << endl;
                INC_FAIL;
                continue;
            }
            MY_INFO << "deposit 0x" << hex << data << dec << endl;
        }
        else if (op == "write-read") {
            string str_val = seqitem.get<string>("val");
            if (str_val == "random") {
                data = (uint32_t)axitrace::rand::instance().get<int>();
            }
            else {
                data = config::instance().to_int<uint32_t>(str_val);
            }
            if (!pio::instance().reg_write(addr, data)) {
                MY_ERROR << "pio reg_write failed" << endl;
                INC_FAIL;
                continue;
            }
            uint32_t rdata;
            if (!pio::instance().reg_read(addr, rdata)) {
                MY_ERROR << "pio reg_read failed" << endl;
                INC_FAIL;
                continue;
            }
            if (rdata == data) {
                MY_INFO << "deposit 0x" << hex << data << ", retrieved 0x" << rdata << ", verified OK" << endl;
            } 
            else {
                MY_ERROR << "deposit 0x" << hex << data << ", retrieved 0x" << rdata << ", verification FAILED" << endl;
                INC_FAIL;
                continue;
            }
        }
        else if (op == "save") {
            if (reg_stash != -1ull) {
                MY_ERROR << "only 1 save can be supported at a time" << endl;
                INC_FAIL;
                continue;
            }
            if (!pio::instance().reg_read(addr, data)) {
                MY_ERROR << "pio reg_read failed" << endl;
                INC_FAIL;
                continue;
            }
            reg_stash = (uint64_t)data;
            reg_stash_addr = addr;
            MY_INFO << "save register data 0x" << hex << reg_stash << dec << endl;
        }
        else if (op == "restore") {
            if (reg_stash == -1ull) {
                MY_ERROR << "save needs to be done before restore" << endl;
                INC_FAIL;
                continue;
            }
            addr = reg_stash_addr;
            data = (uint32_t)reg_stash;
            if (!pio::instance().reg_write(addr, data)) {
                MY_ERROR << "pio reg_write failed" << endl;
                reg_stash = -1ull;
                INC_FAIL;
                continue;
            }
            MY_INFO << "restore register data 0x" << hex << data << dec << endl;
            reg_stash = -1ull;
        }
        else {
            MY_ERROR << "unknown op" << endl;
            INC_FAIL;
            continue;
        }
        INC_PASS;
    }


#include "msg_undef.inc"
#define MY_MSG  "[driver-calibrate] mem.seq." << op << " @[0x" << hex << addr << dec << " - 0x" << hex << size << dec << "(" << size << ")]: "
#include "msg_redef.inc"

#ifdef INC_PASS
#undef INC_PASS
#undef INC_FAIL
#endif
#define INC_PASS {++total_test_num; ++total_test_pass; ++mem_test_num; ++mem_test_pass;}
#define INC_FAIL {++total_test_num; ++total_test_fail; ++mem_test_num; ++mem_test_fail;}

    ////memory
    int mem_test_num = 0;
    int mem_test_pass = 0;
    int mem_test_fail = 0;

    uint8_t* mem_stash = 0;
    uint64_t mem_stash_addr = 0;
    uint32_t mem_stash_size = 0;

    uint8_t* mem_write_buff = 0;
    uint32_t mem_write_size = 0;

    BOOST_FOREACH(const pt::ptree::value_type& vt, config::instance().tree.get_child("driver-calibrate.mem.seq")) {
        const pt::ptree& seqitem = vt.second;
        string op = seqitem.get<string>("op", "");
        uint64_t addr = config::instance().to_int<uint64_t>(seqitem.get<string>("addr", "0"));
        uint32_t size = config::instance().to_int<uint32_t>(seqitem.get<string>("bytes", "0"));
        uint8_t* data = 0;
        if (op == "read") {
            data = new uint8_t[size];
            if (!pio::instance().mem_read(addr, size, data)) {
                MY_ERROR << "pio mem read failed" << endl;
                delete [] data;
                INC_FAIL;
                continue;
            }
            MY_INFO << "retrieved ======== " << endl;
            for (uint32_t i = 0; i < size; i++) {
                printf("%02x ", data[i]);
                if ((i % 8) == 7) printf("    ");
                if ((i % 16) == 15) printf("\n");
            }
            printf("\n");
            delete [] data;
        }
        else if (op == "save") {
            if (mem_stash != 0) {
                MY_ERROR << "only 1 save can be supported before restore" << endl;
                INC_FAIL;
                continue;
            }
            data = new uint8_t[size];
            if (!pio::instance().mem_read(addr, size, data)) {
                MY_ERROR << "pio mem read failed" << endl;
                delete [] data;
                INC_FAIL;
                continue;
            }
            mem_stash = data;
            mem_stash_addr = addr;
            mem_stash_size = size;
            MY_INFO << "saved ======== " << endl;
            for (uint32_t i = 0; i < mem_stash_size; i++) {
                printf("%02x ", mem_stash[i]);
                if ((i % 8) == 7) printf("    ");
                if ((i % 16) == 15) printf("\n");
            }
            printf("\n");
        }
        else if (op == "restore") {
            if (mem_stash == 0) {
                MY_ERROR << "no save found for restore" << endl;
                INC_FAIL;
                continue;
            }
            addr = mem_stash_addr;
            size = mem_stash_size;
            data = mem_stash;
            if (!pio::instance().mem_write(addr, data, size)) {
                MY_ERROR << "pio mem write failed" << endl;
                delete [] data;
                mem_stash = 0;
                INC_FAIL;
                continue;
            }
            MY_INFO << "restored ======== " << endl;
            for (uint32_t i = 0; i < mem_stash_size; i++) {
                printf("%02x ", mem_stash[i]);
                if ((i % 8) == 7) printf("    ");
                if ((i % 16) == 15) printf("\n");
            }
            printf("\n");
            delete [] data;
            mem_stash = 0;
        }
        else if (op == "write") {
            string strv = seqitem.get<string>("val", "");
            data = new uint8_t[size];
            if (strv == "random") {
                for (unsigned i = 0; i < size; i++) data[i] = (uint8_t)axitrace::rand::instance().get<int>();
            }
            else {
                config::instance().to_int(strv, data, size);
            }
            if (!pio::instance().mem_write(addr, data, size)) {
                MY_ERROR << "pio mem write failed" << endl;
                delete [] data;
                INC_FAIL;
                continue;
            }
            MY_INFO << " deposited ======== " << endl;
            for (uint32_t i = 0; i < size; i++) {
                printf("%02x ", data[i]);
                if ((i % 8) == 7) printf("    ");
                if ((i % 16) == 15) printf("\n");
            }
            printf("\n");
            delete [] mem_write_buff;
            mem_write_buff = data;
            mem_write_size = size;
        }
        else if (op == "read-compare") {
            data = new uint8_t[size];
            if (!pio::instance().mem_read(addr, size, data)) {
                MY_ERROR << "pio mem read failed" << endl;
                delete [] data;
                INC_FAIL;
                continue;
            }
            bool verify_ok = false;
            if (mem_write_buff != 0 && size <= mem_write_size) {
                if (memcmp(data, mem_write_buff, size) == 0) {
                    verify_ok = true;
                }
            }
            if (verify_ok) {
                MY_INFO << "retrieved ======== " << endl;
            } 
            else {
                MY_ERROR << "retrieved ======== " << endl;
            }
            for (uint32_t i = 0; i < size; i++) {
                printf("%02x ", data[i]);
                if ((i % 8) == 7) printf("    ");
                if ((i % 16) == 15) printf("\n");
            }
            printf("\n");
            if (mem_write_buff != 0) {
                if (verify_ok) {
                    MY_INFO << "compared against 0x" << hex << mem_write_size << dec << "(" << mem_write_size << ") ======== " << endl;
                } 
                else {
                    MY_DEBUG << "compared against 0x" << hex << mem_write_size << dec << "(" << mem_write_size << ") ======== " << endl;
                }
                for (uint32_t i = 0; i < mem_write_size; i++) {
                    printf("%02x ", mem_write_buff[i]);
                    if ((i % 8) == 7) printf("    ");
                    if ((i % 16) == 15) printf("\n");
                }
                printf("\n");
            }
            if (verify_ok) {
                MY_INFO << "verified OK" << endl;
            }
            else {
                MY_ERROR << "verification FAILED" << endl;
            }
            
            delete [] data;
            if (!verify_ok) {
                INC_FAIL;
                continue;
            }
        }
        else {
            MY_ERROR << "unknown op" << endl;
            delete [] data;
            INC_FAIL;
            continue;
        }
        INC_PASS;
    }



    delete [] mem_write_buff;
    delete [] mem_stash;
    ///////////////////////////////summary

    cout << TOKEN_APP << TOKEN_INFO << "[driver-calibrate] total register access tested : " << reg_test_num << ", passed: " << reg_test_pass << ", failed: " << reg_test_fail << endl;
    cout << TOKEN_APP << TOKEN_INFO << "[driver-calibrate] total memory access tested   : " << mem_test_num << ", passed: " << mem_test_pass << ", failed: " << mem_test_fail << endl;
    cout << TOKEN_APP << TOKEN_INFO << "[driver-calibrate] total tested                 : " << total_test_num << ", passed: " << total_test_pass << ", failed: " << total_test_fail << endl;

    if (reg_test_fail != 0 || reg_test_num != (reg_test_pass + reg_test_fail) ||
        mem_test_fail != 0 || mem_test_num != (mem_test_pass + mem_test_fail) ||
        total_test_fail != 0 || total_test_num != (total_test_pass + total_test_fail)
       ) {
        return false;
    }

    return true;
}



bool job_show_filters::main()
{
    console::instance() << console::control("deact-all");
    console::instance() << console::stream_control("cout");
    console::instance() << console::stream_control("console");
    console::instance() << console::control("timestamp");
    console::instance() << "##==========================================================================" << endl;
    console::instance() << "##   " << asset::instance().chip() << " axi-filter " << endl;
    console::instance() << "##==========================================================================" << endl;
    console::instance() << endl;
    
    asset::instance().show();
    return true;
}


bool job_show_filters::post_main()
{
    if (!DEBUG_MODE || !DEBUG_WRITE_FILTER_CFG) return true;

    for (unsigned i = 0; i < asset::instance().ports.size(); ++i) {
        uint64_t addr = 0x80000000;
        uint64_t size =  0x1000000;
        for (unsigned j = 0; j < asset::instance().filters[asset::instance().ports[i]].size(); ++j) {
            asset::instance().filters[asset::instance().ports[i]][j]->get<asset::condition*>("addr_range")->set("addr_lo", addr + size * j);
            asset::instance().filters[asset::instance().ports[i]][j]->get<asset::condition*>("addr_range")->set("addr_hi", addr + size * j + size - 1);
            //asset::instance().filters[asset::instance().ports[i]][j]->get<asset::condition*>("addr_range")->set("addr_lo", addr + size * j * 3 / 4);
            //asset::instance().filters[asset::instance().ports[i]][j]->get<asset::condition*>("addr_range")->set("addr_hi", addr + size * j * 3 / 4 + size - 1);
            asset::instance().filters[asset::instance().ports[i]][j]->update(true);
            asset::instance().filters[asset::instance().ports[i]][j]->get<asset::action*>("control")->set("log_req", 0);
            asset::instance().filters[asset::instance().ports[i]][j]->get<asset::action*>("control")->set("log_resp", 0);
            asset::instance().filters[asset::instance().ports[i]][j]->get<asset::action*>("control")->set("read_freeze", 0);
            asset::instance().filters[asset::instance().ports[i]][j]->get<asset::action*>("control")->set("write_freeze", 0);
        }
        asset::instance().get<asset::action*>(asset::instance().ports[i], "trace")->set("rst", 0);
        asset::instance().get<asset::action*>(asset::instance().ports[i], "trace")->enabled = false;
    }




    bool ret = asset::instance().program();
    pt::json_parser::write_json(axitrace::option::inst()["odir"][0].get() + "/regtest.cfg", config::instance()->get_child(asset::instance().chip() + ".debug.write-4-reg-init"));
    if (!ret) return false;

    return true;
}



#include "msg_undef.inc"
#include "msg_redef.inc"



void job_show_filters::collect_active_ports() const
{
    for (unsigned i = 0; i < axitrace::option::inst()["show_ports"].size(); ++i) {
        string p = axitrace::option::inst()["show_ports"][i].get();
        boost::to_lower(p);
        if (p == "all") {
            asset::instance().active_ports.clear();
            return;
        }
        vector<string> pl = config::instance().split_list(p);
        for (unsigned j = 0; j < pl.size(); ++j) {
            boost::to_upper(pl[j]);
            asset::instance().active_ports.insert(pl[j]);
        }
    }
}


void task::collect_active_ports() const
{
    if (axitrace::option::inst()["show_ports"].size() > 0) {
        m_jobs.at("show_filters")->collect_active_ports();
    } 
    else {
        m_jobs.at("trace")->collect_active_ports();
    }
}



void job_trace::collect_active_ports() const
{
    BOOST_FOREACH(const ptree::value_type& p, config::instance()->get_child(asset::instance().chip() + ".axi-trace.traces")) {
        vector<string> ports = config::instance().split_list(p.second.get<string>("ports", ""));
        for (vector<string>::iterator it = ports.begin(); it != ports.end(); it++) {
            string p = *it;
            boost::to_lower(p);
            if (p == "all") {
                asset::instance().active_ports.clear();
                return;
            }
            boost::to_upper(p);
            asset::instance().active_ports.insert(p);
        }
    }
}



bool job_trace::pre_main()
{
    console::instance() << console::control("deact-all");
    console::instance() << console::stream_control("cout");
    console::instance() << console::stream_control("console");
    if (!trace::instance().setup()) return false;
    return true;
}



static void task_ctrl_c_handler(int s) 
{
    MY_INFO << " 'ctrl-c' received, process terminated" << endl;
    if (task::instance().current) task::instance().current->terminate = true;
}


bool job_trace::main()
{
    if (option::inst()["dry_run"][0].get<bool>() && !option::inst()["force"][0].get<bool>()) {
        MY_WARNING << " '--dry-run' turned on, we stop here. To continue, try '--force' " << endl << endl << endl;
        return true;
    }
    
    if (!buffer::instance().setup()) return false;

#if DEBUG_CREATE_BUFFER
#else
    for (unsigned i = 0; i < asset::instance().ports.size(); ++i) {
        string&  port = asset::instance().ports[i];
        if (!asset::instance().active(port)) continue;
        asset::instance().reset(port);
    } 
#endif
    
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = task_ctrl_c_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    ////////
    restore_needed = true;
    asset::instance().program();
   
    if (option::instance().has("suppress_log", "stdout") && !(option::instance().has("suppress_log", "all") || option::instance().has("suppress_log", "console"))) {
        MY_INFO << "======================================================================================================" << endl;
        MY_INFO << " stdout has been suppressed, you can use 'ctrl-z' and 'bg' to send the running process in background " << endl;
        MY_INFO << " use the following command to poll the status ... " << endl;
        MY_INFO << "      tail -f  " << option::inst()["odir"][0].get() << "/console.log" << endl;
        MY_INFO << endl;
        MY_INFO << " To quit, run 'fg' to bring the process back to foreground, and followed by 'ctrl-c' " << endl;
        MY_INFO << "==============================================================================================" << endl;
    }

    while (!terminate) {
        bool ret = buffer::instance().main_loop();
        if (!ret) return false; 
        if (buffer::instance().halt()) terminate = true;   
    }

    return true;
}


bool job_trace::post_main()
{
    if (restore_needed && option::inst()["restore"][0].get<bool>()) {
        MY_INFO << " Restoring all filter config registers ..." << endl;
        asset::instance().restore();
        asset::instance().program();
        //asset::instance().load();
        //asset::instance().show();
        MY_INFO << " Restoring all filter config registers done successfully" << endl;
    }
    return true;
}



bool job_write_test_buffer::main()
{
    return true;
}



} ///namespace axitrace
