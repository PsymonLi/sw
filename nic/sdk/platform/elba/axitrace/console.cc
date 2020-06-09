#include <cassert>
#include <memory>
#include <ctime>
#include "console.h"
#include "msg_undef.inc"
#include "msg_redef.inc"


namespace axitrace {

static unique_ptr<console> console_ptr = unique_ptr<console>(new console());


console* console::instancep() 
{
    return console_ptr.get();
}


console& console::instance()
{
    return *instancep();
}


console::control::control(const string& nm)
    : name(nm)
{
}


const string& console::control::operator()(const string&) const
{
    return name;
}


string console::control::operator()(string) const
{
    return string(name);
}


bool console::control::operator==(const string& o) const
{
    return name == o;
}


bool console::control::operator==(const console::control& o) const
{
    return name == o.name;
}


console::stream_control::stream_control(const string& nm, const string& fname, ios_base::openmode mode)
    : console::control(nm)
{
    active = false;
    m_filename = fname;
    m_mode = mode;
    m_of = 0;
}


console::stream_control::stream_control(const console::stream_control& s)
    : console::control(s.name)
{
    active = false;
    m_of = 0;
    m_filename = s.m_filename;
    m_mode = s.m_mode;
}


console::stream_control::~stream_control()
{
    if (name != "cout" && name != "cerr" && m_of != 0) {
        assert(dynamic_cast<ofstream*>(m_of));
        dynamic_cast<ofstream*>(m_of)->close();
        delete m_of;
    }
}


bool console::stream_control::open()
{
    if (m_of) return false;
    if (name == "cout") {
        m_of = &std::cout;
        return true;
    }
    if (name == "cerr") {
        m_of = &std::cerr;
        return true;
    }

    ofstream* of = new ofstream;
    m_of = of;
    of->open(m_filename, ios_base::out | m_mode);
    if (!of->is_open()) {
        MY_ERROR << " file '" << m_filename << "' can not open for write, exit" << endl;
        delete of;
        m_of = 0;
        return false;
    }
    return true;
}


console::console()
{
}


console::~console()
{
    for (vector<stream_control*>::iterator it = streams.begin(); it != streams.end(); it++) {
        delete *it;
    }
}


console& console::operator<<(const console::control& ctrl)
{
    if (ctrl == "act-all") {
        for (unsigned i = 0; i < streams.size(); ++i) streams[i]->active = true;
    }
    else if (ctrl == "deact-all") {
        for (unsigned i = 0; i < streams.size(); ++i) streams[i]->active = false;
    }
    else if (ctrl == "timestamp") {
        time_t rawtime;
        struct tm * timeinfo;
        char buffer[80];

        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, sizeof(buffer), "%m-%d-%Y %H:%M:%S", timeinfo);
        for (unsigned i = 0; i < streams.size(); ++i) {
            if (!streams[i]->active) continue;
            streams[i]->get() << "##===== TM =====  " << buffer << "  ====##" << endl << endl;
        }
    }

    return *this;
}


console& console::operator<<(const stream_control& s)
{
    if (map_stream_name2p.find(s.name) == map_stream_name2p.end()) {
        stream_control* ns = new stream_control(s);
        if (!ns->open()) exit(1);
        ns->active = true;
        streams.push_back(ns);
        map_stream_name2p[ns->name] = ns;
    }
    else {
        map_stream_name2p[s.name]->active = true;
    }
    return *this;
}


console& console::operator<<(endl_func_t f)
{
    for (unsigned i = 0; i < streams.size(); ++i) {
        if (!streams[i]->active) continue;
        streams[i]->get() << f;
    }
    return *this;
}


console& console::operator<<(hex_func_t f)
{
    for (unsigned i = 0; i < streams.size(); ++i) {
        if (!streams[i]->active) continue;
        streams[i]->get() << f;
    }
    return *this;
}


console& console::fill(char c)
{
    for (unsigned i = 0; i < streams.size(); ++i) {
        if (!streams[i]->active) continue;
        streams[i]->get().fill(c);
    }
    return *this;
}



} ///namespace axitrace
