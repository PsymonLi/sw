
#include <string>

#include "device.hpp"


std::string
oprom_type_to_str(OpromType type) {
    std::string oprom = "";

    switch (type) {
    case (OPROM_LEGACY):
        oprom = "legacy";
        break;
    case (OPROM_UEFI):
        oprom = "uefi";
        break;
    case (OPROM_UNIFIED):
        oprom = "unified";
        break;
    default:
        oprom = "unknown";
    }

    return oprom;
}

OpromType
str_to_oprom_type(std::string const& s) {
    if (s == "legacy") {
        return OPROM_LEGACY;
    } else if (s == "uefi") {
        return OPROM_UEFI;
    } else if (s == "unified") {
        return OPROM_UNIFIED;
    } else {
        NIC_LOG_ERR("Unknown OPROM type: {}", s);
        return OPROM_UNKNOWN;
    }
}
