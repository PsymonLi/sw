//------------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//
// base SDK header file that goes into rest of the SDK
//------------------------------------------------------------------------------

#ifndef __SDK_PLATFORM_HPP__
#define __SDK_PLATFORM_HPP__

#include "include/sdk/base.hpp"

#define SDK_INVALID_HBM_ADDRESS    ((uint64_t) 0xFFFFFFFFFFFFFFFF)

#define CACHE_LINE_SIZE                       64
#define CACHE_LINE_SIZE_SHIFT                  6
#define CACHE_LINE_SIZE_MASK                   (CACHE_LINE_SIZE - 1)

namespace sdk {
namespace platform {

#define PLATFORM_TYPE(ENTRY)                                                \
    ENTRY(PLATFORM_TYPE_NONE,       0, "PLATFORM_TYPE_NONE")                \
    ENTRY(PLATFORM_TYPE_SIM,        1, "PLATFORM_TYPE_SIM")                 \
    ENTRY(PLATFORM_TYPE_HAPS,       2, "PLATFORM_TYPE_HAPS")                \
    ENTRY(PLATFORM_TYPE_HW,         3, "PLATFORM_TYPE_HW")                  \
    ENTRY(PLATFORM_TYPE_MOCK,       4, "PLATFORM_TYPE_MOCK")                \
    ENTRY(PLATFORM_TYPE_ZEBU,       5, "PLATFORM_TYPE_ZEBU")                \
    ENTRY(PLATFORM_TYPE_RTL,        6, "PLATFORM_TYPE_RTL")

SDK_DEFINE_ENUM(platform_type_t, PLATFORM_TYPE)
SDK_DEFINE_ENUM_TO_STR(platform_type_t, PLATFORM_TYPE)
SDK_DEFINE_MAP_EXTERN(platform_type_t, PLATFORM_TYPE)
// #undef PLATFORM_TYPE

#define ASIC_TYPE(ENTRY)                                                    \
    ENTRY(SDK_ASIC_TYPE_NONE,       0, "SDK_ASIC_TYPE_NONE")                \
    ENTRY(SDK_ASIC_TYPE_CAPRI,      1, "SDK_ASIC_TYPE_CAPRI")               \
    ENTRY(SDK_ASIC_TYPE_ELBA,       2, "SDK_ASIC_TYPE_ELBA")

SDK_DEFINE_ENUM(asic_type_t, ASIC_TYPE)
SDK_DEFINE_ENUM_TO_STR(asic_type_t, ASIC_TYPE)
SDK_DEFINE_MAP_EXTERN(asic_type_t, ASIC_TYPE)
// #undef ASIC_TYPE

#define SYSINIT_MODE(ENTRY)                                       \
    ENTRY(SYSINIT_MODE_DEFAULT,    0, "SYSINIT_MODE_DEFAULT")     \
    ENTRY(SYSINIT_MODE_GRACEFUL,   1, "SYSINIT_MODE_GRACEFUL")    \
    ENTRY(SYSINIT_MODE_HITLESS,    2, "SYSINIT_MODE_HITLESS")     \

SDK_DEFINE_ENUM(sysinit_mode_t,        SYSINIT_MODE)
SDK_DEFINE_ENUM_TO_STR(sysinit_mode_t, SYSINIT_MODE)
SDK_DEFINE_MAP_EXTERN(sysinit_mode_t,  SYSINIT_MODE)
// #undef SYSINIT_MODE

static inline bool
sysinit_mode_default (sysinit_mode_t type)
{
    return type == SYSINIT_MODE_DEFAULT;
}

static inline bool
sysinit_mode_hitless (sysinit_mode_t type)
{
    return type == SYSINIT_MODE_HITLESS;
}

static inline bool
sysinit_mode_graceful (sysinit_mode_t type)
{
    return type == SYSINIT_MODE_GRACEFUL;
}

// used in hitless upgrade
#define SYSINIT_DOMAIN(ENTRY)                              \
    ENTRY(SYSINIT_DOMAIN_A,   0, "SYSINIT_DOMAIN_A")       \
    ENTRY(SYSINIT_DOMAIN_B,   1, "SYSINIT_DOMAIN_B")       \

SDK_DEFINE_ENUM(sysinit_dom_t,        SYSINIT_DOMAIN)
SDK_DEFINE_ENUM_TO_STR(sysinit_dom_t, SYSINIT_DOMAIN)
SDK_DEFINE_MAP_EXTERN(sysinit_dom_t,  SYSINIT_DOMAIN)
// #undef SYSINIT_DOMAIN

static inline bool
sysinit_domain_a (sysinit_dom_t dom)
{
    return dom == SYSINIT_DOMAIN_A;
}

static inline bool
sysinit_domain_b (sysinit_dom_t dom)
{
    return dom == SYSINIT_DOMAIN_B;
}

}    // namespace platform
}    // namespace sdk

using sdk::platform::platform_type_t;
using sdk::platform::asic_type_t;
using sdk::platform::sysinit_mode_t;
using sdk::platform::sysinit_dom_t;

#endif    // __SDK_PLATFORM_HPP__

