#ifndef __AXITRACE_CONFIG_HH__
#define __AXITRACE_CONFIG_HH__

#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/detail/json_parser_error.hpp>
#include <string>
#include <set>
#include <exception>
#include <iostream>
#include "asset.h"

namespace pt = boost::property_tree;

using namespace std;
using namespace boost;
using namespace pt;

namespace axitrace {

class config {
public:
    config() {}
    virtual ~config();
    static config*                              instancep();
    static config&                              instance();
    bool                                        setup();
    bool                                        parse(const string& filename, pt::ptree* this_tree = 0, bool merge2trunk = true);
    bool                                        add_options();
    pt::ptree*                                  operator->() { return &tree; }
    
    template<typename T = string> vector<T>     split_list(const string& v) const;
    template<typename T = string> string        join_list(const vector<T>& l, const string& delimiter = ", ") const;
    template<typename T = uint64_t> string      join_list_hex(const vector<T>& l, const string& delimiter = ", ") const;                                    
    template<typename T = uint64_t> string      join_list_dec(const vector<T>& l, const string& delimiter = ", ") const;                                    

    template<typename T = long long> T          to_int(const string& v) const;
    bool                                        to_bool(const string& v) const;
    void                                        to_int(const string& v, uint8_t* d, uint32_t size) const;
    string                                      to_hex(uint64_t v) const;
    string                                      to_dec(uint64_t v) const;
    bool                                        test_json();
    void                                        debug(const string& fname) const;

public:
    pt::ptree                                   tree;


private:
    void                                        travel2display(const pt::ptree& subtree, const string& pathstr);
    void                                        travel2merge(pt::ptree& subtree, const string& pathstr);
};


template<> vector<string> config::split_list<string>(const string& v) const;
template<> string config::join_list<string>(const vector<string>& l, const string& delimiter) const;
template<> string config::join_list_hex(const vector<uint64_t>& l, const string& delimiter) const;
template<> string config::join_list_hex(const vector<unsigned>& l, const string& delimiter) const;
template<> string config::join_list_dec(const vector<uint64_t>& l, const string& delimiter) const;
template<> string config::join_list_dec(const vector<unsigned>& l, const string& delimiter) const;
template<> string config::join_list_dec(const vector<asset::filter*>& l, const string& delimiter) const;

}

#endif //__AXITRACE_CONFIG_HH__
