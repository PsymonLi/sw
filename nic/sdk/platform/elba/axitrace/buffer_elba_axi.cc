// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#include <cstring>
#include <iomanip>
#include "buffer_elba_axi.h"
#include "msg_undef.inc"
#include "msg_redef.inc"
#include "rand.h"
#include "asset_elba.h"
#include "console.h"
#include "config.h"



namespace axitrace {


buffer_elba_axi::buffer_elba_axi(const string& port)
   : buffer(port) 
{
    rawsize = 64;
    rawdata = new uint8_t[rawsize * 2];
}



buffer_elba_axi::~buffer_elba_axi()
{
}



buffer* buffer_elba_axi::create(const string& port)
{
    return new buffer_elba_axi(port);
}



bool buffer_elba_axi::build()
{
#ifdef BUILD_FIELD
#undef BUILD_FIELD
#endif
#define BUILD_FIELD(X, N) \
    f = new buffer_elba_axi::field_##X(this, #N); \
    f->init(); \
    assert(map_field_name2idx.find(f->name) == map_field_name2idx.end()); \
    map_field_name2idx[f->name] = fields.size(); \
    fields.push_back(f);

    buffer::field* f;

    BUILD_FIELD(tag, tag);

    BUILD_FIELD(axid, axi_arid);
    BUILD_FIELD(filter_id, axi_arfilter_id);
    BUILD_FIELD(axlen, axi_arlen);
    BUILD_FIELD(axsize, axi_arsize);
    BUILD_FIELD(axcache, axi_arcache);
    BUILD_FIELD(sta_pending, reads_pending);
    BUILD_FIELD(use_cache, filter_read_use_cache);
    BUILD_FIELD(log_resp, filter_log_rresp);
    BUILD_FIELD(axaddr, axi_araddr);
    BUILD_FIELD(generation, filter_generation);
    BUILD_FIELD(pic, pic_addr);
    BUILD_FIELD(timestamp, timestamp);

    BUILD_FIELD(axid, axi_awid);
    BUILD_FIELD(filter_id, axi_awfilter_id);
    BUILD_FIELD(axlen, axi_awlen);
    BUILD_FIELD(axsize, axi_awsize);
    BUILD_FIELD(axcache, axi_awcache);
    BUILD_FIELD(sta_pending, writes_pending);
    BUILD_FIELD(use_cache, filter_write_use_cache);
    BUILD_FIELD(log_resp, filter_log_bresp);
    BUILD_FIELD(write_inval_send, filter_write_inval_send);
    BUILD_FIELD(write_local_ack, filter_write_local_ack);
    BUILD_FIELD(wstrb, axi_wstrb);
    BUILD_FIELD(axaddr, axi_awaddr);

    BUILD_FIELD(wdata, axi_wdata);
    BUILD_FIELD(rdata, axi_rdata);
    
    BUILD_FIELD(axid, axi_rid);
    BUILD_FIELD(axresp, axi_rresp);
    BUILD_FIELD(axid, axi_bid);
    BUILD_FIELD(axresp, axi_bresp);

    current = get("tag");
    assert(current);
    tag = dynamic_cast<buffer_elba_axi::field_tag*>(current);
    assert(tag);
    return true;
}



bool buffer_elba_axi::field_tag::streamin()
{
    if (!buffer::field::streamin()) return false;
    buf->addr = buf->start_addr;
    buf->index = buf->start_index;
    buf->generation = buf->start_generation;
    buf->size = 0;
    buf->cursor = 0;
    if (!buf->streamin()) return false;
    buf->rewind();
    addr = buf->addr;
    index = buf->index;
    buf->forward();

    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = 0xC0DE411C0DE41100 + (axitrace::rand::instance().get() % 4);
#else
    d |= buf->rawdata[63];
    d <<= 8;
    d |= buf->rawdata[62];
    d <<= 8;
    d |= buf->rawdata[61];
    d <<= 8;
    d |= buf->rawdata[60];
    d <<= 8;
    d |= buf->rawdata[59];
    d <<= 8;
    d |= buf->rawdata[58];
    d <<= 8;
    d |= buf->rawdata[57];
    d <<= 8;
    d |= buf->rawdata[56];
#endif

    switch (d) {
        case 0xC0DE411C0DE41100: svals[0] = "ar"; tag = ar; break;
        case 0xC0DE411C0DE41101: svals[0] = "r"; tag = r; break;
        case 0xC0DE411C0DE41102: svals[0] = "aww"; tag = aww; break;
        case 0xC0DE411C0DE41103: svals[0] = "b"; tag = b; break;
        default: 
            MY_ERROR << " tag 0x" << hex << d << " is unknown at address = 0x" << addr << dec << endl;
            return false;
    }

    loaded = true;
    buf->loads.push_back(this);

    switch (tag) {
        case ar:
            {    
            if (!buf->get("axi_arid")->streamin()) return false; 
            }
            break;
        case r:     
            {
            if (!buf->get("axi_rid")->streamin()) return false; 
            }
            break;
        case aww:   
            {
            if (!buf->get("axi_awid")->streamin()) return false; 
            }
            break;
        case b:     
            {
            if (!buf->get("axi_bid")->streamin()) return false;
            }
            break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_axid::streamin()
{
    if (!buffer::field::streamin()) return false;
    addr = mbuf->tag->addr;
    index = mbuf->tag->index;

    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = (axitrace::rand::instance().get() & 0x1ffful);
    //if (axitrace::rand::instance().get() & 0x1ul) d |= (1ull << 10);
    if (axitrace::rand::instance().get() & 0x1ul) d |= (1ull << 11);
    //if (axitrace::rand::instance().get() & 0x1ul) d |= (1ull << 13);
#else
    d |= buf->rawdata[55];
    d <<= 8;
    d |= buf->rawdata[54];
#endif

    ivals[0] = d;
    loaded = true;
    buf->loads.push_back(this);

    if (buf->port == "TD") {
        if (d & 0x400ull) {
            src = "pdma";
        }
        else {
            src = "stg";
            src += config::instance().to_dec((d >> 7) & 0x7ull);
        }
        aid = (d & 0x7f);
    }
    else if (buf->port == "RD") {
        if (d & 0x1000ull) {
            src = "pdma";
            aid = (d & 0x1ff);
        }
        else {
            src = "stg";
            src += config::instance().to_dec((d >> 9) & 0x7ull);
            aid = (d & 0x7f);
        }
    }
    else if (buf->port == "XD") {
        if (d & 0x200ull) {
            src = "pdma";
        }
        else {
            src = "stg";
            src += config::instance().to_dec((d >> 7) & 0x3ull);
        }
        aid = (d & 0x7f);
    }

    buffer_elba_axi::field_tag* t = dynamic_cast<buffer_elba_axi::field_tag*>(buf->get("tag"));
    assert(t);
    switch (t->tag) {
        case buffer_elba_axi::field_tag::ar:    if (!buf->get("axi_arfilter_id")->streamin()) return false; break;
        case buffer_elba_axi::field_tag::r:     if (!buf->get("axi_rresp")->streamin()) return false; break;
        case buffer_elba_axi::field_tag::aww:   if (!buf->get("axi_awfilter_id")->streamin()) return false; break;
        case buffer_elba_axi::field_tag::b:     if (!buf->get("axi_bresp")->streamin()) return false; break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_filter_id::streamin()
{
    if (!buffer::field::streamin()) return false;
    addr = mbuf->tag->addr;
    index = mbuf->tag->index;


    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = (axitrace::rand::instance().get() & 0x3ful);
#else
    d |= buf->rawdata[53];
#endif

    ivals[0] = d;
    loaded = true;
    buf->loads.push_back(this);

    buffer_elba_axi::field_tag* t = dynamic_cast<buffer_elba_axi::field_tag*>(buf->get("tag"));
    assert(t);
    switch (t->tag) {
        case buffer_elba_axi::field_tag::ar:    if (!buf->get("axi_arlen")->streamin()) return false; break;
        case buffer_elba_axi::field_tag::aww:   if (!buf->get("axi_awlen")->streamin()) return false; break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_axlen::streamin()
{
    if (!buffer::field::streamin()) return false;
    addr = mbuf->tag->addr;
    index = mbuf->tag->index;


    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = (axitrace::rand::instance().get() & 0xful);
#else
    d |= ((buf->rawdata[52] >> 4) & 0xf);
#endif

    ivals[0] = d;
    loaded = true;
    buf->loads.push_back(this);

    buffer_elba_axi::field_tag* t = dynamic_cast<buffer_elba_axi::field_tag*>(buf->get("tag"));
    assert(t);
    switch (t->tag) {
        case buffer_elba_axi::field_tag::ar:    if (!buf->get("axi_arsize")->streamin()) return false; break;
        case buffer_elba_axi::field_tag::aww:   if (!buf->get("axi_awsize")->streamin()) return false; break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_axsize::streamin()
{
    if (!buffer::field::streamin()) return false;
    addr = mbuf->tag->addr;
    index = mbuf->tag->index;

    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = 6;
#else
    d |= ((buf->rawdata[52] >> 0) & 0x7);
#endif

    ivals[0] = d;
    loaded = true;
    buf->loads.push_back(this);

    buffer_elba_axi::field_tag* t = dynamic_cast<buffer_elba_axi::field_tag*>(buf->get("tag"));
    assert(t);
    switch (t->tag) {
        case buffer_elba_axi::field_tag::ar:    
            if (!buf->get("axi_arcache")->streamin()) return false; 
            break;
        case buffer_elba_axi::field_tag::aww:   
            if (!buf->get("axi_awcache")->streamin()) return false; 
            break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_axcache::streamin()
{
    if (!buffer::field::streamin()) return false;
    addr = mbuf->tag->addr;
    index = mbuf->tag->index;


    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = (axitrace::rand::instance().get() & 0xful);
#else
    d |= ((buf->rawdata[51] >> 4) & 0xf);
#endif

    ivals[0] = d;
    loaded = true;
    buf->loads.push_back(this);

    buffer_elba_axi::field_tag* t = dynamic_cast<buffer_elba_axi::field_tag*>(buf->get("tag"));
    assert(t);
    switch (t->tag) {
        case buffer_elba_axi::field_tag::ar:    if (!buf->get("reads_pending")->streamin()) return false; break;
        case buffer_elba_axi::field_tag::aww:   if (!buf->get("writes_pending")->streamin()) return false; break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_sta_pending::streamin()
{
    if (!buffer::field::streamin()) return false;
    addr = mbuf->tag->addr;
    index = mbuf->tag->index;


    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = (axitrace::rand::instance().get() & 0xfful);
#else
    d |= buf->rawdata[50];
#endif

    ivals[0] = d;
    loaded = true;
    buf->loads.push_back(this);

    buffer_elba_axi::field_tag* t = dynamic_cast<buffer_elba_axi::field_tag*>(buf->get("tag"));
    assert(t);
    switch (t->tag) {
        case buffer_elba_axi::field_tag::ar:    if (!buf->get("filter_read_use_cache")->streamin()) return false; break;
        case buffer_elba_axi::field_tag::aww:   if (!buf->get("filter_write_use_cache")->streamin()) return false; break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_use_cache::streamin()
{
    if (!buffer::field::streamin()) return false;
    addr = mbuf->tag->addr;
    index = mbuf->tag->index;


    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = (axitrace::rand::instance().get() & 0x1ul);
#else
    switch (mbuf->tag->tag) {
        case buffer_elba_axi::field_tag::ar:    d = ((buf->rawdata[49] >> 1) & 0x1); break;
        case buffer_elba_axi::field_tag::aww:   d = ((buf->rawdata[49] >> 3) & 0x1); break;
        default: assert(0);
    }
#endif

    ivals[0] = d;
    loaded = true;
    buf->loads.push_back(this);

    switch (mbuf->tag->tag) {
        case buffer_elba_axi::field_tag::ar:    if (!buf->get("filter_log_rresp")->streamin()) return false; break;
        case buffer_elba_axi::field_tag::aww:   if (!buf->get("filter_log_bresp")->streamin()) return false; break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_log_resp::streamin()
{
    if (!buffer::field::streamin()) return false;
    addr = mbuf->tag->addr;
    index = mbuf->tag->index;


    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = (axitrace::rand::instance().get() & 0x1ul);
#else
    buffer_elba_axi::field_tag* t = dynamic_cast<buffer_elba_axi::field_tag*>(buf->get("tag"));
    assert(t);
    switch (t->tag) {
        case buffer_elba_axi::field_tag::ar:    d = ((buf->rawdata[49] >> 0) & 0x1); break;
        case buffer_elba_axi::field_tag::aww:   d = ((buf->rawdata[49] >> 2) & 0x1); break;
        default: assert(0);
    }
#endif

    ivals[0] = d;
    loaded = true;
    buf->loads.push_back(this);

    switch (mbuf->tag->tag) {
        case buffer_elba_axi::field_tag::ar:    if (!buf->get("axi_araddr")->streamin()) return false; break;
        case buffer_elba_axi::field_tag::aww:   if (!buf->get("filter_write_inval_send")->streamin()) return false; break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_axaddr::streamin()
{
    if (!buffer::field::streamin()) return false;
    addr = mbuf->tag->addr;
    index = mbuf->tag->index;


    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = ((uint64_t)axitrace::rand::instance().get()) & 0xffffffffull;
    d <<= 32;
    d |= ((uint64_t)axitrace::rand::instance().get()) & 0xffffffffull;
#else
    d |= buf->rawdata[15];
    d <<= 8;
    d |= buf->rawdata[14];
    d <<= 8;
    d |= buf->rawdata[13];
    d <<= 8;
    d |= buf->rawdata[12];
    d <<= 8;
    d |= buf->rawdata[11];
    d <<= 8;
    d |= buf->rawdata[10];
    d <<= 8;
    d |= buf->rawdata[9];
    d <<= 8;
    d |= buf->rawdata[8];
#endif

    ivals[0] = d;
    loaded = true;
    buf->loads.push_back(this);

    buffer_elba_axi::field_tag* t = dynamic_cast<buffer_elba_axi::field_tag*>(buf->get("tag"));
    assert(t);
    switch (t->tag) {
        case buffer_elba_axi::field_tag::ar:    
            buffer::instance().push_ar(buf->port, buf->get("axi_arid")->get_int(), buf->get("axi_arlen")->get_int(), buf->get("axi_arsize")->get_int(), d);
            if (!buf->get("filter_generation")->streamin()) return false; 
            break;
        case buffer_elba_axi::field_tag::aww: 
            if (!buf->get("filter_generation")->streamin()) return false; 
            break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_generation::streamin()
{
    if (!buffer::field::streamin()) return false;
    addr = mbuf->tag->addr;
    index = mbuf->tag->index;


    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = axitrace::rand::instance().get() & 0x1ul;
#else
    d = (buf->rawdata[7] >> 7) & 0x1;
#endif

    ivals[0] = d;
    loaded = true;
    buf->loads.push_back(this);

    buffer_elba_axi::field_tag* t = dynamic_cast<buffer_elba_axi::field_tag*>(buf->get("tag"));
    assert(t);
    switch (t->tag) {
        case buffer_elba_axi::field_tag::ar:    
        case buffer_elba_axi::field_tag::aww:   
            if (!buf->get("pic_addr")->streamin()) return false; 
            break;
        case buffer_elba_axi::field_tag::r:
        case buffer_elba_axi::field_tag::b:
            if (!buf->get("timestamp")->streamin()) return false; 
            break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_pic::streamin()
{
    if (!buffer::field::streamin()) return false;
    addr = mbuf->tag->addr;
    index = mbuf->tag->index;


    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = axitrace::rand::instance().get() & 0x1ul;
#else
    d = (buf->rawdata[7] >> 6) & 0x1;
#endif

    ivals[0] = d;
    loaded = true;
    buf->loads.push_back(this);

    buffer_elba_axi::field_tag* t = dynamic_cast<buffer_elba_axi::field_tag*>(buf->get("tag"));
    assert(t);
    switch (t->tag) {
        case buffer_elba_axi::field_tag::ar:    
        case buffer_elba_axi::field_tag::aww:   
            if (!buf->get("timestamp")->streamin()) return false; 
            break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_timestamp::streamin()
{
    if (!buffer::field::streamin()) return false;
    addr = mbuf->tag->addr;
    index = mbuf->tag->index;


    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = ((uint64_t)axitrace::rand::instance().get()) & 0xffffull;
    d <<= 32;
    d |= ((uint64_t)axitrace::rand::instance().get()) & 0xffffffffull;
#else
    d |= buf->rawdata[5];
    d <<= 8;
    d |= buf->rawdata[4];
    d <<= 8;
    d |= buf->rawdata[3];
    d <<= 8;
    d |= buf->rawdata[2];
    d <<= 8;
    d |= buf->rawdata[1];
    d <<= 8;
    d |= buf->rawdata[0];
#endif

    ivals[0] = d;
    loaded = true;
    buf->loads.push_back(this);

    buffer_elba_axi::field_tag* t = dynamic_cast<buffer_elba_axi::field_tag*>(buf->get("tag"));
    assert(t);
    switch (t->tag) {
        case buffer_elba_axi::field_tag::ar:    
        case buffer_elba_axi::field_tag::b:    
            buf->finish = true;
            buf->complete = true;
            break;
        case buffer_elba_axi::field_tag::aww:   
            if (!buf->get("axi_wdata")->streamin()) return false; 
            break;
        case buffer_elba_axi::field_tag::r:   
            if (!buf->get("axi_rdata")->streamin()) return false; 
            break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_write_inval_send::streamin()
{
    if (!buffer::field::streamin()) return false;
    addr = mbuf->tag->addr;
    index = mbuf->tag->index;


    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = axitrace::rand::instance().get() & 0x1ul;
#else
    d = (buf->rawdata[49] >> 1) & 0x1;
#endif

    ivals[0] = d;
    loaded = true;
    buf->loads.push_back(this);

    buffer_elba_axi::field_tag* t = dynamic_cast<buffer_elba_axi::field_tag*>(buf->get("tag"));
    assert(t);
    switch (t->tag) {
        case buffer_elba_axi::field_tag::aww:   
            if (!buf->get("filter_write_local_ack")->streamin()) return false; 
            break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_write_local_ack::streamin()
{
    if (!buffer::field::streamin()) return false;
    addr = mbuf->tag->addr;
    index = mbuf->tag->index;


    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = axitrace::rand::instance().get() & 0x1ul;
#else
    d = (buf->rawdata[49] >> 0) & 0x1;
#endif

    ivals[0] = d;
    loaded = true;
    buf->loads.push_back(this);

    buffer_elba_axi::field_tag* t = dynamic_cast<buffer_elba_axi::field_tag*>(buf->get("tag"));
    assert(t);
    switch (t->tag) {
        case buffer_elba_axi::field_tag::aww:   
            if (!buf->get("axi_wstrb")->streamin()) return false; 
            break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_wstrb::streamin()
{
    if (!buffer::field::streamin()) return false;
    addr = mbuf->tag->addr;
    index = mbuf->tag->index;


    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = ((uint64_t)axitrace::rand::instance().get()) & 0xffffffffull;
    d <<= 32;
    d |= ((uint64_t)axitrace::rand::instance().get()) & 0xffffffffull;
#else
    d |= buf->rawdata[23];
    d <<= 8;
    d |= buf->rawdata[22];
    d <<= 8;
    d |= buf->rawdata[21];
    d <<= 8;
    d |= buf->rawdata[20];
    d <<= 8;
    d |= buf->rawdata[19];
    d <<= 8;
    d |= buf->rawdata[18];
    d <<= 8;
    d |= buf->rawdata[17];
    d <<= 8;
    d |= buf->rawdata[16];
#endif

    ivals[0] = d;
    loaded = true;
    buf->loads.push_back(this);

    buffer_elba_axi::field_tag* t = dynamic_cast<buffer_elba_axi::field_tag*>(buf->get("tag"));
    assert(t);
    switch (t->tag) {
        case buffer_elba_axi::field_tag::aww:   
            if (!buf->get("axi_awaddr")->streamin()) return false; 
            break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_axresp::streamin()
{
    if (!buffer::field::streamin()) return false;
    addr = mbuf->tag->addr;
    index = mbuf->tag->index;


    uint64_t d = 0;
#if DEBUG_CREATE_BUFFER
    d = (axitrace::rand::instance().get() & 0x3ul);
#else
    d |= buf->rawdata[53] & 0x3;
#endif

    ivals[0] = d;
    loaded = true;
    buf->loads.push_back(this);
    if (d != 0) ++buf->error;

    buffer_elba_axi::field_tag* t = dynamic_cast<buffer_elba_axi::field_tag*>(buf->get("tag"));
    assert(t);
    switch (t->tag) {
        case buffer_elba_axi::field_tag::r:   
        case buffer_elba_axi::field_tag::b:   
            if (!buf->get("filter_generation")->streamin()) 
                return false; 
            break;
        default: assert(0);
    }

    return true;
}



bool buffer_elba_axi::field_wdata::streamin()
{

    if (!buffer::field::streamin()) return false;
   

    if (msize == 0) {
        msize = (1ul << buf->get("axi_awsize")->get_int()) * (buf->get("axi_awlen")->get_int() + 1);
        mvals = new uint8_t[msize];
        already_read = 0;
        addr = buf->addr;
        index = buf->index;
    }
    
    while (already_read < msize) {
        buf->cursor = 0;
        if (!buf->streamin()) return false;

#if DEBUG_CREATE_BUFFER
        for (unsigned i = already_read; i < already_read + buf->rawsize; ++i) {
            mvals[i] = (uint8_t)axitrace::rand::instance().get();
        }
#else
        memcpy(mvals + already_read, buf->rawdata, buf->rawsize);
#endif
        already_read += buf->rawsize;
    }


    loaded = true;
    buf->loads.push_back(this);
    buf->finish = true;
    buf->complete = true;

    return true;
}



void buffer_elba_axi::field_rdata::init()
{
    search_code411 = false;
}



bool buffer_elba_axi::field_rdata::streamin()
{
    if (!buffer::field::streamin()) return false;
    
    if (msize == 0) {
        already_read = 0;
        addr = buf->addr;
        index = buf->index;
        uint64_t id = buf->get("axi_rid")->get();
        if (buffer::instance().ar_ids[buf->port][id].size() > 0) {
            buffer::man::ar_info_t r = buffer::instance().ar_ids[buf->port][id].front();
            buffer::instance().pop_ar(buf->port, id);
            assert(r.id == id);
            msize = (1ul << r.arsize) * (r.arlen + 1);
            msize = (msize + 63) / 64 * 64; ////round to 64
            search_code411 = false;
            araddr = r.araddr;
        }
        else {
            msize = 64 * 16;
            search_code411 = true;
            araddr = 0;
        }
        mvals = new uint8_t[msize];
    }


    bool marker = false;
    while ((already_read < msize) && (!search_code411 || !marker)) {
        buf->cursor = 0;
        if (!buf->streamin()) return false;

#if DEBUG_CREATE_BUFFER
        for (unsigned i = already_read; i < already_read + buf->rawsize; ++i) {
            mvals[i] = (uint8_t)axitrace::rand::instance().get();
        }
        if (search_code411) {
            if (axitrace::rand::instance().get() & 0x1) {
                marker = true;
                buf->rewind();
            }
            else {
                already_read += buf->rawsize;
            }
        }
        else {
            already_read += buf->rawsize;
        }
#else
        if (search_code411) {
            uint64_t d = 0;
            d |= buf->rawdata[63];
            d <<= 8;
            d |= buf->rawdata[62];
            d <<= 8;
            d |= buf->rawdata[61];
            d <<= 8;
            d |= buf->rawdata[60];
            d <<= 8;
            d |= buf->rawdata[59];
            d <<= 8;
            d |= buf->rawdata[58];
            d <<= 8;
            d |= buf->rawdata[57];
            if (d == 0xC0DE411C0DE411) {
                marker = true;
                buf->rewind();
            }
            else {
                memcpy(mvals + already_read, buf->rawdata, buf->rawsize);
                already_read += buf->rawsize;
            }
        }
        else {
            memcpy(mvals + already_read, buf->rawdata, buf->rawsize);
            already_read += buf->rawsize;
        }
#endif
    }
   
    
    if (search_code411 && !marker) {
        MY_WARNING << "[" << buf->port << "] Can not find CODE411 marker starting from the address = 0x" << hex << addr << dec << endl;
    }

    msize = already_read;
    loaded = true;
    buf->loads.push_back(this);
    buf->finish = true;
    buf->complete = true;

    return true;
}



void buffer_elba_axi::poll(const string& port)
{
#if DEBUG_CREATE_BUFFER
    uint32_t port_buf_size = (1ull << asset::instance().get<asset::action*>(port, "trace")->get("buf_size"));
    bool wrap = (bool)asset::instance().get<asset::action*>(port, "trace")->get("wrap");
    
    uint32_t idx = buffer::instance().indices[port] + (axitrace::rand::instance().get() & 0xf);
    if (idx >= port_buf_size) {
        if (wrap) {
            buffer::instance().cur_gen = 1 - buffer::instance().generations[port]; 
            buffer::instance().cur_idx = idx - port_buf_size;
        } 
        else {
            buffer::instance().cur_gen = buffer::instance().generations[port]; 
            assert(buffer::instance().cur_gen == 0);
            buffer::instance().cur_idx = port_buf_size;
        }
    } 
    else {
        buffer::instance().cur_gen = buffer::instance().generations[port]; 
        buffer::instance().cur_idx = idx;
    }
#else 
    if (!asset::instance().poll(port)) {
        MY_ERROR << "[" << port << "] Polling buffer status FAILED" << endl;
        return;
    }
    buffer::instance().cur_idx = asset::instance().get<asset::condition*>(port, "trace_status")->get("index");
    buffer::instance().cur_gen = asset::instance().get<asset::condition*>(port, "trace_status")->get("generation");
#endif
}



const string& buffer_elba_axi::get_tag() const
{
    return tag->svals[0];
}



uint32_t buffer_elba_axi::get_id() const
{
    return (uint32_t)loads[1]->ivals[0];
}


#ifdef SAVE_FORMAT
#undef DEF_FORMAT
#undef SAVE_FORMAT
#undef RESTORE_FORMAT
#endif
#define DEF_FORMAT      char saved_fill; streamsize saved_w;
#define SAVE_FORMAT     { saved_fill = std::cout.fill(); saved_w = std::cout.width(); }
#define RESTORE_FORMAT  { console::instance().fill(saved_fill); console::instance() << setw(saved_w); }


void buffer_elba_axi::field_wdata::show(unsigned indent, unsigned tab) const
{
    unsigned mytab;
    DEF_FORMAT;
    SAVE_FORMAT;
    uint64_t axaddr = buf->get("axi_awaddr")->get_int();

    for (unsigned i = 0; i < indent; ++i) console::instance() << "    ";
    mytab = tab - 4 * indent;

    console::instance() << name;
    mytab -= name.length();

    for (unsigned i = 0; i < mytab; ++i) console::instance() << " ";
    console::instance() << ": ";

    assert(mvals);
    console::instance().fill('0') << "-- @ 0x" << hex << setw(16) << addr << " (axi: 0x" << axaddr << ")" << dec;
    RESTORE_FORMAT;
    console::instance() << " size 0x" << hex << msize << " (" << dec << msize << ") bytes ---- ----" << endl;
    axaddr &= 0xffffffffffffffc0;
    console::instance().fill('0') << hex;
    for (unsigned i = 0; i < msize; ++i) {
        if ((i % 16) == 0) {
            for (unsigned j = 0; j < indent; ++j) console::instance() << "    ";
            if (!option::inst()["reverse_bytes"][0].get<bool>()) {
                console::instance() << "0x" << setw(16) << (addr + i) << " (axi: 0x" << setw(16) << (axaddr + i) << ") : ";
            } 
            else {
                console::instance() << "0x" << setw(16) << (addr + msize - i - 1) << " (axi: 0x" << setw(16) << (axaddr + msize - i - 1) << ") : ";
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
    console::instance() << dec;
    RESTORE_FORMAT;
}



void buffer_elba_axi::field_rdata::show(unsigned indent, unsigned tab) const
{
    unsigned mytab;
    DEF_FORMAT;
    SAVE_FORMAT;
    uint64_t axaddr = araddr;

    for (unsigned i = 0; i < indent; ++i) console::instance() << "    ";
    mytab = tab - 4 * indent;

    console::instance() << name;
    mytab -= name.length();

    for (unsigned i = 0; i < mytab; ++i) console::instance() << " ";
    console::instance() << ": ";

    assert(mvals);
    console::instance().fill('0') << "-- @ 0x" << hex << setw(16) << addr << " (axi: 0x" << axaddr << ")" << dec;
    RESTORE_FORMAT;
    console::instance() << " size 0x" << hex << msize << " (" << dec << msize << ") bytes ---- ----" << endl;
    axaddr &= 0xffffffffffffffc0;
    console::instance().fill('0') << hex;
    for (unsigned i = 0; i < msize; ++i) {
        if ((i % 16) == 0) {
            for (unsigned j = 0; j < indent; ++j) console::instance() << "    ";
            if (!option::inst()["reverse_bytes"][0].get<bool>()) {
                console::instance() << "0x" << setw(16) << (addr + i) << " (axi: 0x" << setw(16) << (axaddr == 0? 0: (axaddr + i)) << ") : ";
            } 
            else {
                console::instance() << "0x" << setw(16) << (addr + msize - i - 1) << " (axi: 0x" << setw(16) << (axaddr == 0? 0: (axaddr + msize - i - 1)) << ") : ";
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
    console::instance() << dec;
    RESTORE_FORMAT;
}



void buffer_elba_axi::field_axid::show(unsigned indent, unsigned tab) const
{
    if (!(buf->port == "RD" || buf->port == "TD" || buf->port == "XD")) {
        buffer::field::show(indent, tab);
        return;
    }

    unsigned mytab;

    for (unsigned i = 0; i < indent; ++i) console::instance() << "    ";
    mytab = tab - 4 * indent;

    console::instance() << name;
    mytab -= name.length();

    for (unsigned i = 0; i < mytab; ++i) console::instance() << " ";
    console::instance() << ": ";

    console::instance() << "0x" << hex << ivals[0] << dec;
    console::instance() << " (aid: 0x" << hex << aid << dec << " (" << aid << "), source: " << src  << ")";
    console::instance() << endl;
}



const string& buffer_elba_axi::get_src() const
{
    buffer_elba_axi::field_axid* f = dynamic_cast<buffer_elba_axi::field_axid*>(loads[1]);
    assert(f);
    return f->src;
}



uint32_t buffer_elba_axi::get_aid() const
{
    buffer_elba_axi::field_axid* f = dynamic_cast<buffer_elba_axi::field_axid*>(loads[1]);
    assert(f);
    return f->aid;
}




} //// namespace axitrace
