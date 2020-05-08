#include <memory>
#include <stdio.h> 
#include <dirent.h> 
#include <fstream>
#include "generic.h"
#include "pio.h"
#include "asset.h"
#include "buffer_elba_axi.h"
#include "trace_elba_axi.h"
#include "asset_elba.h"
#include "console.h"
#include "msg_undef.inc"
#include "msg_redef.inc"
#include "setup.h"
#include "console.h"
#include "option.h"

namespace axitrace {


static unique_ptr<buffer::man> buffer_man_ptr = unique_ptr<buffer::man>(new buffer::man());



buffer::man* buffer::instancep()
{
    return buffer_man_ptr.get();
}


buffer::man& buffer::instance()
{
    return *instancep();
}


buffer::man::man()
{
    cur_idx = 0;
    cur_gen = 0;
}



buffer::man::~man()
{
}



bool buffer::man::setup()
{
    for (unsigned i = 0; i < asset::instance().ports.size(); ++i) {
        string& port = asset::instance().ports[i];
        if (!asset::instance().active(port)) continue;

        addr[port] = asset::instance().get<asset::action*>(port, "trace")->get("base_addr") << 6;
        indices[port] = 0;
        generations[port] = 0;
        stopped[port] = false;
    }
    return true;
}



bool buffer::man::halt() 
{
    for (map<string, bool>::iterator it = stopped.begin(); it != stopped.end(); it++) {
        if (!it->second) return false;
    }
    return true;
}



buffer::buffer(const string& p)
    : port(p)
{
    finish = false;
    complete = false;
    error = 0;

    start_index = 0;
    start_generation = 0;
    start_addr = 0;

    index = 0;
    generation = 0;
    addr = 0;
    size = 0;

    rawdata = 0;
    rawsize = 0;
    cursor = 0;
}



buffer::~buffer()
{
    for (unsigned i = 0; i < fields.size(); ++i) {
        fields[i]->destroy();
        delete fields[i];
    }
    if (rawdata) delete [] rawdata;
}



buffer::field::field(buffer* b, const string& n)
    : buf(b), name(n)
{
    addr = 0;
    mvals = 0;
    msize = 0;
    loaded = false;
}



buffer::field::~field()
{
    if (mvals) delete [] mvals;
}



buffer::field* buffer::get(unsigned idx) 
{
    assert(idx < fields.size());
    return fields[idx];
}



buffer::field* buffer::get(const string& name)
{
    assert(map_field_name2idx.find(name) != map_field_name2idx.end());
    return get(map_field_name2idx[name]);
}


const buffer::field* buffer::get(unsigned idx) const
{
    assert(idx < fields.size());
    return fields.at(idx);
}



const buffer::field* buffer::get(const string& name) const
{
    assert(map_field_name2idx.find(name) != map_field_name2idx.end());
    return get(map_field_name2idx.at(name));
}



bool buffer::field::streamin()
{
    if (buf->finish) return false;
    buf->current = this;
    return true;
}



bool buffer::streamin()
{
    uint64_t port_start_addr = asset::instance().get<asset::action*>(port, "trace")->get("base_addr") << 6;
    uint32_t port_buf_size = (1ull << asset::instance().get<asset::action*>(port, "trace")->get("buf_size"));
    uint64_t port_end_addr = port_start_addr + (port_buf_size << 6) - 1;
    bool wrap = (bool)asset::instance().get<asset::action*>(port, "trace")->get("wrap");

    if ((generation == buffer::instance().cur_gen) && (index == buffer::instance().cur_idx)) {
        if (index == port_buf_size) {
            assert(!wrap);
            assert(addr == port_end_addr + 1);
            assert(generation == 0);
            finish = true;
            buffer::instance().stopped[port] = true;
        }
        return false; ///full
    }


#if DEBUG_CREATE_BUFFER
#else
    uint8_t* p = rawdata;
    if (cursor > 0) p += rawsize;
    if (!pio::instance().mem_read(addr, rawsize, p)) {
        finish = true;
        error = true;
        return false;
    }
#endif

    forward();
    return true;
}



void buffer::forward()
{
    uint64_t port_start_addr = asset::instance().get<asset::action*>(port, "trace")->get("base_addr") << 6;
    uint32_t port_buf_size = (1ull << asset::instance().get<asset::action*>(port, "trace")->get("buf_size"));
    uint64_t port_end_addr = port_start_addr + (port_buf_size << 6) - 1;
    bool wrap = (bool)asset::instance().get<asset::action*>(port, "trace")->get("wrap");
    
    addr += rawsize;
    size += rawsize;
    ++index;
    
    if (index >= port_buf_size) {
        assert(addr > port_end_addr);
        if (wrap) {
            index = 0;
            addr = port_start_addr;
            generation = 1 - generation;
        }
    }
}


void buffer::rewind()
{
    uint64_t port_start_addr = asset::instance().get<asset::action*>(port, "trace")->get("base_addr") << 6;
    uint32_t port_buf_size = (1ull << asset::instance().get<asset::action*>(port, "trace")->get("buf_size"));
    uint64_t port_end_addr = start_addr + (port_buf_size << 6) - 1;
    bool wrap = (bool)asset::instance().get<asset::action*>(port, "trace")->get("wrap");

    if (wrap) {
        if (index == 0) {
            assert(addr == port_start_addr);
            index = port_buf_size;
            addr = port_end_addr + 1;
            generation = 1 - generation;
        }
    }

    addr -= rawsize;
    size -= rawsize;
    --index;
}



buffer* buffer::man::create(const string& port)
{
    if (asset::instance().chip() == "elba") {
        buffer* b = buffer_elba_axi::create(port);
        b->build();
        return b;
    }
    else {
        assert(0);
        return 0;
    }
}



void buffer::man::push_ar(const string& port, uint64_t id, uint64_t arlen, uint64_t arsize, uint64_t araddr)
{
    if (ar_ids[port][id].size() >  1024) pop_ar(port, id);

    ar_info_t  r;
    r.id = id;
    r.arlen = arlen;
    r.arsize = arsize;
    r.araddr = araddr;
    ar_ids[port][id].push(r);
}



void buffer::man::pop_ar(const string& port, uint64_t id)
{
    assert(ar_ids.find(port) != ar_ids.end());
    assert(ar_ids[port].find(id) != ar_ids[port].end());
    assert(ar_ids[port][id].size() > 0);
    ar_ids[port][id].pop();
}


#ifdef SAVE_FORMAT
#undef DEF_FORMAT
#undef SAVE_FORMAT
#undef RESTORE_FORMAT
#endif
#define DEF_FORMAT      char saved_fill; streamsize saved_w;
#define SAVE_FORMAT     { saved_fill = std::cout.fill(); saved_w = std::cout.width(); }
#define RESTORE_FORMAT  { console::instance().fill(saved_fill); console::instance() << setw(saved_w); }


void buffer::field::show(unsigned indent, unsigned tab) const
{
    unsigned mytab;
    DEF_FORMAT;

    for (unsigned i = 0; i < indent; ++i) console::instance() << "    ";
    mytab = tab - 4 * indent;

    console::instance() << name;
    mytab -= name.length();

    for (unsigned i = 0; i < mytab; ++i) console::instance() << " ";
    console::instance() << ": ";

    if (mvals) {
        SAVE_FORMAT;
        console::instance().fill('0') << "-- @0x" << hex << setw(16) << addr << dec;
        RESTORE_FORMAT;
        console::instance() << " size 0x" << hex << msize << " (" << dec << msize << ") bytes ---- ----" << endl;
        console::instance().fill('0') << hex;
        for (unsigned i = 0; i < msize; ++i) {
            if ((i % 16) == 0) {
                for (unsigned j = 0; j < indent; ++j) console::instance() << "    ";
                if (!option::inst()["reverse_bytes"][0].get<bool>()) {
                    console::instance() << "0x" << setw(16) << (addr + i) << " : ";
                } 
                else {
                    console::instance() << "0x" << setw(16) << (addr + msize - i - 1) << " : ";
                }
            }

            if (!option::inst()["reverse_bytes"][0].get<bool>()) {
                console::instance() << setw(2) << (int)mvals[i];
            }
            else {
                console::instance() << setw(2) << (int)mvals[msize - i - 1];
            }

            if ((i % 16) == 15) {
                console::instance() << endl;
            }
            else if ((i % 8) == 7) {
                console::instance() << "    ";
            }
            else {
                console::instance() << " ";
            }
        }
        if ((msize % 16) != 0) console::instance() << endl;
        RESTORE_FORMAT;
    } 
    else if (svals.size() > 0) {
        for (unsigned i = 0; i < svals.size(); ++i) {
            console::instance() << svals[i] << " ";
        }
        console::instance() << endl;
    }
    else {
        for (unsigned i = 0; i < ivals.size(); ++i) {
            console::instance() << "0x" << hex << ivals[i] << dec << "  (" << ivals[i] << ")  ";
        }
        console::instance() << endl;
    }
}



void  buffer::show(unsigned indent, unsigned tab) const
{
    unsigned mytab, mytab2;
    DEF_FORMAT;
    SAVE_FORMAT;

    for (unsigned i = 0; i < indent; ++i) console::instance() << "====";
    mytab = tab - 4 * indent;
    uint64_t tm = get("timestamp")->get();
    console::instance().fill('0') << " [T:" << setw(15) << tm << "] [A:0x" << setw(16) << hex << start_addr << dec << "] [" << port << "]";
    RESTORE_FORMAT;
    console::instance() << " size 0x" << hex << size << dec << " (" << size << ") bytes ====" << endl;
    
    mytab -= 4;

    for (unsigned i = 0; i < indent + 1; ++i) console::instance() << "    ";
    string s = "generation";
    console::instance() << s;
    mytab2 = mytab - s.length();
    for (unsigned i = 0; i < mytab2; ++i) console::instance() << " ";
    console::instance() << ": ";
    console::instance() << (int)start_generation << endl;

    for (unsigned i = 0; i < indent + 1; ++i) console::instance() << "    ";
    s = "index";
    console::instance() << s;
    mytab2 = mytab - s.length();
    for (unsigned i = 0; i < mytab2; ++i) console::instance() << " ";
    console::instance() << ": ";
    console::instance() << start_index << endl;

    for (unsigned i = 0; i < indent + 1; ++i) console::instance() << "    ";
    s = "complete";
    console::instance() << s;
    mytab2 = mytab - s.length();
    for (unsigned i = 0; i < mytab2; ++i) console::instance() << " ";
    console::instance() << ": ";
    console::instance() << complete << endl;

    for (unsigned i = 0; i < indent + 1; ++i) console::instance() << "    ";
    s = "error";
    console::instance() << s;
    mytab2 = mytab - s.length();
    for (unsigned i = 0; i < mytab2; ++i) console::instance() << " ";
    console::instance() << ": ";
    console::instance() << error << endl;

    for (unsigned i = 0; i < indent + 1; ++i) console::instance() << "    ";
    console::instance() << "------------------------------------------------" << endl;

    for (unsigned i = 0; i < loads.size(); ++i) {
        assert(loads[i]->loaded);
        loads[i]->show(indent + 1, tab);
    }

    console::instance() << endl;
}



void buffer::man::poll(const string& port)
{
    cur_port = port;
    if (asset::instance().chip() == "elba") {
        buffer_elba_axi::poll(port);
    }
    else {
        assert(0);
    }    
}



bool buffer::man::main_loop()
{
    for (unsigned i = 0; i < asset::instance().ports.size(); ++i) {
        string&  port = asset::instance().ports[i];
        if (!asset::instance().active(port)) continue;
        poll(port);
        bool wrap = (bool)asset::instance().get<asset::action*>(port, "trace")->get("wrap");
        if (cur_gen == generations[port] && cur_idx == indices[port]) {
            uint32_t bufsize = (1ul << asset::instance().get<asset::action*>(port, "trace")->get("buf_size"));
            if (!wrap && (cur_idx >= bufsize)) {
                buffer::instance().stopped[port] = true;
            }
            continue;
        }

        if (cur_gen != 0 && !wrap) {
            MY_ERROR << "[" << port << "] generation = " << cur_gen << " but wrap = 0, sanity check FAILED" << endl;
            continue;
        }
        if (cur_gen != generations[port] && cur_idx >= indices[port]) {
            MY_ERROR << "[" << port << "] buffer read too slow or buffer write too fast or buffer too shallow, PI = " << cur_idx << ", CI = " << indices[port] << endl;
            continue;
        }

        if (buffers[port] == 0) {
            buffer* b  = create(port);
            b->start_generation = generations[port];
            b->start_index = indices[port];
            b->start_addr = asset::instance().get<asset::action*>(port, "trace")->get("base_addr") << 6;
            b->start_addr += (b->start_index << 6);
            buffers[port] = b;
        }
        bool ret = buffers[port]->current->streamin();
        generations[port] = buffers[port]->generation;
        indices[port] = buffers[port]->index;
        if (!buffers[port]->finish) continue;

        
        ////// setup log && reports 
        ////////////////////////////
        bool exceed_reports_size = false;
        uint64_t sz = buffer::get_reports_size();
        sz >>= 20;
        MY_DEBUG << ".......... " << sz << " MB reports have been generated already" << endl;
        if (sz >= config::instance()->get<uint64_t>(asset::instance().chip() + ".max-reports-size", 1024)) {
            exceed_reports_size = true;
        }


        console::instance() << console::control("deact-all");

        if (!(option::instance().has("suppress_log", "stdout"))) {
            console::instance() << console::stream_control("cout"); 
        }
        if (!(option::instance().has("suppress_log", "all") || option::instance().has("suppress_log", "console")) && !exceed_reports_size) {
            console::instance() << console::stream_control("console"); 
        }

        string logname;
        string logfile;

        logname = "trace_" + port;
        logfile = option::inst()["odir"][0].get() + "/" + logname + ".log";
        if (!(option::instance().has("suppress_log", "all") || option::instance().has("suppress_log", logname)) && !exceed_reports_size) {
            console::instance() << console::stream_control(logname, logfile, ios_base::trunc); 
        }


        if (port == "TD" || port == "RD" || port == "XD") {
            logname = "trace_" + port + "_" + buffers[port]->get_src();
            logfile = option::inst()["odir"][0].get() + "/" + logname + ".log";
            if (!(option::instance().has("suppress_log", "all") || option::instance().has("suppress_log", logname)) && !exceed_reports_size) {
                console::instance() << console::stream_control(logname, logfile, ios_base::trunc); 
            }

            logname = "trace_" + port + "_" + buffers[port]->get_src() + "_" + buffers[port]->get_tag();
            logfile = option::inst()["odir"][0].get() + "/" + logname + ".log";
            if (!(option::instance().has("suppress_log", "all") || option::instance().has("suppress_log", logname)) && !exceed_reports_size) {
                console::instance() << console::stream_control(logname, logfile, ios_base::trunc); 
            }

            logname = "trace_" + port + "_" + buffers[port]->get_src() + "_" + buffers[port]->get_tag() + "_aid" + config::instance().to_dec(buffers[port]->get_aid());
            logfile = option::inst()["odir"][0].get() + "/" + logname + ".log";
            if (!(option::instance().has("suppress_log", "all") || option::instance().has("suppress_log", logname)) && !exceed_reports_size) {
                console::instance() << console::stream_control(logname, logfile, ios_base::trunc); 
            }
        }
        else {
            logname = "trace_" + port + "_" + buffers[port]->get_tag();
            logfile = option::inst()["odir"][0].get() + "/" + logname + ".log";
            if (!(option::instance().has("suppress_log", "all") || option::instance().has("suppress_log", logname)) && !exceed_reports_size) {
                console::instance() << console::stream_control(logname, logfile, ios_base::trunc); 
            }

            logname = "trace_" + port + "_" + buffers[port]->get_tag() + "_id" + config::instance().to_dec(buffers[port]->get_id());
            logfile = option::inst()["odir"][0].get() + "/" + logname + ".log";
            if (!(option::instance().has("suppress_log", "all") || option::instance().has("suppress_log", logname)) && !exceed_reports_size) {
                console::instance() << console::stream_control(logname, logfile, ios_base::trunc); 
            }
        }

        //////

        if (ret) {
            assert(buffers[port]->finish);
            assert(buffers[port]->complete);
            MY_DEBUG << "[" << port << "] fully retrieve a buffer successfully" << endl;
            buffers[port]->show(2, 40);
            delete buffers[port];
            buffers[port] = 0;
        }
        else if (buffers[port]->finish) {
            MY_DEBUG << "[" << port << "] retrieve a buffer, complete = " << buffers[port]->complete << ", error = " << buffers[port]->error << endl;
            buffers[port]->show(2, 40);
            delete buffers[port];
            buffers[port] = 0;
        }
    }

    return true;
}



uint64_t buffer::get_reports_size() 
{
    uint64_t sz = 0;

    struct dirent *de;  
      
    DIR *dr = opendir(option::inst()["odir"][0].get().c_str()); 
          
    if (dr) { 
        while ((de = readdir(dr)) != 0) {
            string fname = de->d_name;
            if (fname == "." || fname == "..") continue;
            std::ifstream in(option::inst()["odir"][0].get() + "/" + fname, ifstream::ate);
            sz += (uint64_t)in.tellg();
            //cout << "  xxxxxxxxx " << fname << ",  " << in.tellg() << endl;
        }
        closedir(dr);   
    } 

    return sz;
}



} //// namespace axitrace
