#include <memory>
#include <iostream>
#include "generic.h"
#include "pio.h"
#include "rand.h"
#include "lib/pal/pal.hpp"
#include "option.h"
#include "config.h"
#include "asset.h"



#ifdef MY_MSG
#undef MY_MSG
#undef MY_INFO
#undef MY_ERROR
#undef MY_WARNING
#undef MY_DEBUG
#endif
#define MY_MSG  " PIO "
#define MY_INFO cout << TOKEN_APP << TOKEN_INFO << MY_MSG
#define MY_ERROR cout << TOKEN_APP << TOKEN_ERROR << MY_MSG
#define MY_WARNING cout << TOKEN_APP << TOKEN_WARNING << MY_MSG
#define MY_DEBUG if (DEBUG_MODE) cout << TOKEN_APP << TOKEN_DEBUG << MY_MSG


using namespace sdk::lib;

namespace axitrace {

static unique_ptr<pio> pio_ptr = unique_ptr<pio>(new pio());


#if PIO_REGMODEL_MODE
static map<uint64_t, uint32_t> regmodel;
#endif




pio::~pio()
{
}


pio* pio::instancep()
{
    return pio_ptr.get();
}


pio& pio::instance()
{
    return *instancep();
}


bool pio::setup()
{
#if PIO_PAL_MODE
    if (pal_ret_t::PAL_RET_OK != pal_init(platform_type_t::PLATFORM_TYPE_HW)) {
        return false;
    }
#endif
#if PIO_REGMODEL_MODE
    BOOST_FOREACH(const ptree::value_type& p, config::instance()->get_child(asset::instance().chip() + ".reg-init")) {
        uint64_t addr = config::instance().to_int<uint64_t>(p.second.get<string>("addr"));
        uint32_t data = config::instance().to_int<uint32_t>(p.second.get<string>("data"));
        if (!reg_write(addr, data)) return false;
    }
#endif

    return true;
}


bool pio::reg_write(uint64_t addr, uint32_t data, bool secure) const
{
    MY_DEBUG << "reg write : @0x" << hex << addr << " <=== 0x" << data << dec << endl;
#if PIO_PAL_MODE
    if (pal_ret_t::PAL_RET_OK != pal_reg_write(addr, &data, 1)) {
        MY_ERROR << "[PIO] register write FAILED at 0x" << hex << addr << dec << endl;
        return false;   
    }
#endif
#if PIO_REGMODEL_MODE
    regmodel[addr & 0xfffffffffffffffc] = data;
#endif
    return true;
}


bool pio::reg_read(uint64_t addr, uint32_t& data, bool secure) const
{
    MY_DEBUG << "reg read: @0x" << hex << addr << dec << " ---- " << endl;
#if PIO_PAL_MODE
    if (pal_ret_t::PAL_RET_OK != pal_reg_read(addr, &data, 1)) {
        MY_ERROR << "[PIO] register read FAILED at 0x" << hex << addr << dec << endl;
        return false;
    }
#elif PIO_REGMODEL_MODE
    if (regmodel.find(addr & 0xfffffffffffffffc) != regmodel.end()) {
        data = regmodel[addr & 0xfffffffffffffffc];
    }
    else {
        data = axitrace::rand::instance().get<int>();
    }
#else
    data = axitrace::rand::instance().get<int>();
#endif
    MY_DEBUG << "reg read: @0x" << hex << addr << " ===> 0x" << data << dec << endl;
    return true;
}


bool pio::mem_write(uint64_t addr, const uint8_t* data, uint32_t bytes, bool secure) const
{
    MY_DEBUG << "mem write : @0x" << hex << addr << " <=== 0x" << bytes << " (" << dec << bytes << ") bytes" << endl;
    if (DEBUG_MODE) {
        for (uint32_t i = 0; i < bytes; i++) {
            printf("%02x ", data[i]);
            if ((i % 16) == 15) printf("\n");
        }
        printf("\n");
    }
#if PIO_PAL_MODE
    if (pal_ret_t::PAL_RET_OK != pal_mem_write(addr, const_cast<uint8_t*>(data), bytes)) {
        MY_ERROR << "[PIO] memory write FAILED at 0x" << hex << addr << dec << endl;
        return false;
    }
#endif
    return true;
}


bool pio::mem_read(uint64_t addr, uint32_t bytes, uint8_t* data, bool secure) const
{

    MY_DEBUG << "mem read : @0x" << hex << addr << " ===> 0x" << bytes << " (" << dec << bytes << ") bytes" << endl;

#if PIO_PAL_MODE
    if (pal_ret_t::PAL_RET_OK != pal_mem_read(addr, data, bytes)) {
        MY_ERROR << "[PIO] memory read FAILED at 0x" << hex << addr << dec << endl;
        return false;
    }
#else
    for (unsigned i = 0; i < bytes; i++) data[i] = (uint8_t)axitrace::rand::instance().get<int>();
#endif

    if (DEBUG_MODE) {
        for (uint32_t i = 0; i < bytes; i++) {
            printf("%02x ", data[i]);
            if ((i % 16) == 15) printf("\n");
        }
        printf("\n");
    }
    return true;
}



} ///namespace axitrace
