// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __AXITRACE_CONSOLE_HH__
#define __AXITRACE_CONSOLE_HH__

#include <iostream>
#include <fstream>
#include <vector>
#include <map>



using namespace std;

namespace axitrace {


class console
{
public:
class control
{
public:
    control(const string& nm);
    virtual ~control() {}
    const string                            name;
    virtual bool                            is_stream() const { return false; }
    const string&                           operator()(const string&) const;
    string                                  operator()(string) const;
    bool                                    operator==(const string&) const;                                   
    bool                                    operator==(const console::control&) const;                                   
};

class stream_control : public control
{
public:
    stream_control(const string& nm, const string& fname = "", ios_base::openmode mode = ios_base::trunc);
    stream_control(const stream_control& s);
    virtual ~stream_control();
    virtual bool                            is_stream() const { return true; }
    ostream&                                get() { return *m_of; }
    bool                                    open();
    bool                                    active;

private:
    string                                  m_filename;
    ios_base::openmode                      m_mode;    
    ostream*                                m_of;
};

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
public:
    typedef ostream&  (* endl_func_t)(ostream& os);
    typedef ios_base& (* hex_func_t)(ios_base& str);
    console();
    virtual ~console();
    static console*                         instancep();
    static console&                         instance();
    console&                                operator<<(const control&);
    console&                                operator<<(const stream_control&);
    console&                                operator<<(endl_func_t f);
    console&                                operator<<(hex_func_t f);
    template<typename T> console&           operator<<(T);
    console&                                fill(char c);

protected:
    vector<stream_control*>                 streams;
    map<string, stream_control*>            map_stream_name2p; 
};


template<typename T> console& console::operator<<(T v)
{
    for (unsigned i = 0; i < streams.size(); ++i) {
        if (!streams[i]->active) continue;
        streams[i]->get() << v;
    }
    return *this;
}


} /// namespace axitrace


#endif //__AXITRACE_CONSOLE_HH__
