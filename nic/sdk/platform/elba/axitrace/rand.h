// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __AXITRACE_RAND_HH__
#define __AXITRACE_RAND_HH__


using namespace std;

namespace axitrace {

class rand 
{
public:
    rand();
    virtual ~rand();
    static rand*                    instancep();
    static rand&                    instance();
    template<typename T = int> T    get() const;
    int                             seed() const;
    int                             seed(int s);
    int                             max() const;

private:
    const int                       m_max;
    int                             m_seed;
};


} //namespace axitrace 

#endif //__AXITRACE_RAND_HH__
