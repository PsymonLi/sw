// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __AXITRACE_BUFFER_ELBA_AXI_HH__
#define __AXITRACE_BUFFER_ELBA_AXI_HH__

#include "buffer.h"


#define DEFINE_FIELD_CLASS(X) \
class field_##X : public buffer::field \
{ \
public: \
    buffer_elba_axi* mbuf; \
    field_##X(buffer* b, const string& n) : buffer::field(b, n) { mbuf = dynamic_cast<buffer_elba_axi*>(b); assert(mbuf); } \
    virtual ~field_##X() { } \
    virtual bool streamin(); 

#define ENDDEF_FIELD_CLASS };



namespace axitrace {


class buffer_elba_axi : public buffer 
{
/////
DEFINE_FIELD_CLASS(tag)
    virtual void                                init() { svals.push_back(""); }
    typedef enum {
        ar, aww, r, b
    } tag_t;
    tag_t                                       tag;
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(axid)
    uint32_t                                    aid;
    string                                      src;
    virtual void                                init() { ivals.push_back(0); }
    virtual void                                show(unsigned indent, unsigned tab) const;
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(filter_id)
    virtual void                                init() { ivals.push_back(0); }
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(axlen)
    virtual void                                init() { ivals.push_back(0); }
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(axsize)
    virtual void                                init() { ivals.push_back(0); }
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(axcache)
    virtual void                                init() { ivals.push_back(0); }
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(sta_pending)
    virtual void                                init() { ivals.push_back(0); }
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(use_cache)
    virtual void                                init() { ivals.push_back(0); }
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(log_resp)
    virtual void                                init() { ivals.push_back(0); }
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(axaddr)
    virtual void                                init() { ivals.push_back(0); }
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(generation)
    virtual void                                init() { ivals.push_back(0); }
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(pic)
    virtual void                                init() { ivals.push_back(0); }
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(timestamp)
    virtual void                                init() { ivals.push_back(0); }
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(write_inval_send)
    virtual void                                init() { ivals.push_back(0); }
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(write_local_ack)
    virtual void                                init() { ivals.push_back(0); }
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(wstrb)
    virtual void                                init() { ivals.push_back(0); }
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(axresp)
    virtual void                                init() { ivals.push_back(0); }
ENDDEF_FIELD_CLASS

/////
DEFINE_FIELD_CLASS(wdata)
    uint32_t                                    already_read;
    uint64_t                                    awaddr;
    virtual void                                show(unsigned indent, unsigned tab) const;
ENDDEF_FIELD_CLASS

DEFINE_FIELD_CLASS(rdata)
    virtual void                                init();
    virtual void                                show(unsigned indent, unsigned tab) const;
    uint32_t                                    already_read;
    bool                                        search_code411;
    uint64_t                                    araddr;
ENDDEF_FIELD_CLASS


public:
    buffer_elba_axi(const string& port);
    virtual ~buffer_elba_axi();
    static buffer*                              create(const string& port);
    virtual bool                                build();
    static void                                 poll(const string& port);
    buffer_elba_axi::field_tag*                 tag;
    virtual const string&                       get_tag() const;
    virtual uint32_t                            get_id() const;
    virtual const string&                       get_src() const;
    virtual uint32_t                            get_aid() const;
};



} /// namespace axitrace


#endif //__AXITRACE_BUFFER_ELBA_AXI_HH__
