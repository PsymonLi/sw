#ifndef __AXITRACE_BUFFER_HH__
#define __AXITRACE_BUFFER_HH__

#include <list>
#include <map>
#include <vector>
#include <cassert>
#include <queue>


using namespace std;


namespace axitrace {



class buffer 
{
public:

class man
{
public:
    man();
    ~man();
    typedef struct {
        uint64_t                                        id;
        uint64_t                                        arlen;
        uint64_t                                        arsize;
        uint64_t                                        araddr;   
    } ar_info_t;
    typedef map<uint64_t, queue<ar_info_t> >             ar_id_q_t;

    map<string, ar_id_q_t>                              ar_ids;
    map<string, uint64_t>                               addr;
    map<string, uint32_t>                               indices;
    map<string, uint8_t>                                generations;
    map<string, bool>                                   stopped;
    map<string, buffer*>                                buffers;
 
    bool                                                setup();
    bool                                                main_loop();
    void                                                push_ar(const string& port, uint64_t id, uint64_t arlen, uint64_t arsize, uint64_t araddr);
    void                                                pop_ar(const string& port, uint64_t id);
    bool                                                halt();   
    buffer*                                             create(const string& port);

    uint32_t                                            cur_idx;
    uint8_t                                             cur_gen;
    string                                              cur_port;

protected:
    void                                                poll(const string& port);
};



class field 
{
public:
    field(buffer* b, const string& n);
    virtual ~field();
    axitrace::buffer* const                             buf;
    const string                                        name;

    virtual void                                        init() { }
    virtual void                                        destroy() { }
    uint64_t                                            addr;
    uint32_t                                            index;
    uint64_t                                            get(unsigned idx = 0) const { return get_int(idx); }
    virtual uint64_t                                    get_int(unsigned idx = 0) const { assert(ivals.size() > 0 && idx < ivals.size()); return ivals[idx]; }
    virtual string&                                     get_str(unsigned idx = 0) { assert(svals.size() > 0 && idx < svals.size()); return svals[idx]; }
    vector<uint64_t>                                    ivals;
    vector<string>                                      svals;
    uint8_t*                                            mvals;
    uint32_t                                            msize; 
    bool                                                loaded;
    virtual bool                                        streamin();
    virtual void                                        show(unsigned indent, unsigned tab) const;
};


//////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////// class buffer /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
public:
    buffer(const string& p);
    virtual ~buffer();
    static man*                                         instancep();
    static man&                                         instance();

    const string                                        port;
    vector<field*>                                      fields;
    vector<field*>                                      loads;
    field*                                              current;

    field*                                              get(unsigned idx);
    field*                                              get(const string& name);
    const field*                                        get(unsigned idx) const;
    const field*                                        get(const string& name) const;
    virtual bool                                        build()  { return false; }
    virtual bool                                        streamin(); 
    virtual void                                        show(unsigned indent, unsigned tab) const;
    virtual const string&                               get_tag() const { assert(0); return port; }
    virtual uint32_t                                    get_id() const { assert(0); return 0; }
    virtual const string&                               get_src() const { assert(0); return port; }
    virtual uint32_t                                    get_aid() const { assert(0); return 0; }
    
    void                                                forward();
    void                                                rewind();

    bool                                                finish; ///done reading
    bool                                                complete; ///no buffer truncate
    int                                                 error; 
    
    uint8_t                                             start_generation;
    uint32_t                                            start_index;
    uint64_t                                            start_addr;
    
    uint8_t                                             generation;
    uint32_t                                            index;
    uint64_t                                            addr;
    uint32_t                                            size;
    uint32_t                                            cursor;

    uint8_t*                                            rawdata;
    uint32_t                                            rawsize;

    static uint64_t                                     get_reports_size();

protected:
    map<string, unsigned>                               map_field_name2idx;
};





} // namespace axitrace


#endif //__AXITRACE_BUFFER_HH__
