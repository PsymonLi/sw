// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __AXITRACE_PIO_HH__
#define __AXITRACE_PIO_HH__

using namespace std;

namespace axitrace {

class pio
{
public:
    pio() {}
    virtual ~pio();
    static pio*                 instancep();
    static pio&                 instance();
    bool                        setup();
    bool                        reg_write(uint64_t addr, uint32_t data, bool secure = false) const;
    bool                        reg_read(uint64_t addr, uint32_t& data, bool secure = false) const;
    bool                        mem_write(uint64_t addr, const uint8_t* data, uint32_t bytes, bool secure = false) const;
    bool                        mem_read(uint64_t addr, uint32_t bytes, uint8_t* data, bool secure = false) const;
};


} //namespace axitrace


#endif //__AXITRACE_PIO_HH__
