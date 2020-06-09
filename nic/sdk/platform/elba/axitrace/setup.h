#ifndef __AXITRACE_SETUP_HH__
#define __AXITRACE_SETUP_HH__


using namespace std;

namespace axitrace {

class setup {
public:
    setup();
    virtual ~setup();

    static setup*               instancep();
    static setup&               instance();
    void                        show();
    bool                        check();

public:
    string                      bin;
    string                      pwd;
    string                      root;
    string                      cfg;
};

}


#endif //__AXITRACE_SETUP_HH__
