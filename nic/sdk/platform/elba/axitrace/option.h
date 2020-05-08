#ifndef __AXITRACE_OPTION_HH__
#define __AXITRACE_OPTION_HH__

#include <map>
#include <vector>


using namespace std;

namespace axitrace {

class option_val {
public:
    option_val(const char* v);
    void                                    set(const char* v);
    template<typename T = const string&> T  get() const;
private:
    string                                  m_val;
};


class option {
public:
    option() {}
    virtual ~option();
    static option*                              instancep();
    static option&                              instance();
    static map<string, vector<option_val> >&    inst(); 
    bool                                        parse(int argc, char* const* argv);
    map<string, vector<option_val> >*           operator->() { return &opt; }
    map<string, vector<option_val> >            opt;
    bool                                        has(const string& kw0, const string& kw1) const; 

private:
    void                                        usage();
    map<string, map<string, bool> >             m_lkup;
};



} //namespace axitrace


#endif //__AXITRACE_OPTION_HH__
