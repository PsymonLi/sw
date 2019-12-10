// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// Purpose: Common helper APIs for metaswitch stub programming 

#include "nic/metaswitch/stubs/mgmt/pdsa_mgmt_utils.hpp"

#define SHARED_DATA_TYPE CSS_LOCAL
using namespace std;


void
ip_addr_to_spec (const ip_addr_t *ip_addr,
                 types::IPAddress *ip_addr_spec)
{
    if (ip_addr->af == IP_AF_IPV4) {
        ip_addr_spec->set_af(types::IP_AF_INET);
        ip_addr_spec->set_v4addr(ip_addr->addr.v4_addr);
    } else {
        ip_addr_spec->set_af(types::IP_AF_INET6);
        ip_addr_spec->set_v6addr(ip_addr->addr.v6_addr.addr8, IP6_ADDR8_LEN);
    }

    return;
}

bool
ip_addr_spec_to_ip_addr (const types::IPAddress& in_ipaddr,
                         ip_addr_t *out_ipaddr)
{
    memset(out_ipaddr, 0, sizeof(ip_addr_t));
    if (in_ipaddr.af() == types::IP_AF_INET) {
        out_ipaddr->af = IP_AF_IPV4;
        out_ipaddr->addr.v4_addr = in_ipaddr.v4addr();
    } else if (in_ipaddr.af() == types::IP_AF_INET6) {
        out_ipaddr->af = IP_AF_IPV6;
        memcpy(out_ipaddr->addr.v6_addr.addr8,
               in_ipaddr.v6addr().c_str(),
               IP6_ADDR8_LEN);
    } else {
        return false;
    }

    return true;
}

NBB_VOID 
pdsa_convert_ip_addr_to_amb_ip_addr (ip_addr_t     pdsa_ip_addr, 
                                     NBB_LONG      *type, 
                                     NBB_ULONG     *len, 
                                     NBB_BYTE      *amb_ip_addr)
{
    switch (pdsa_ip_addr.af)
    {
        case IP_AF_IPV4:
            *type = AMB_INETWK_ADDR_TYPE_IPV4;
            *len = AMB_MAX_IPV4_ADDR_LEN;
            break;

        case IP_AF_IPV6:
            *type = AMB_INETWK_ADDR_TYPE_IPV6;
            *len = AMB_MAX_IPV6_ADDR_LEN;
            break;

        default:
            *type = *len = 0;
            return;
    }

    NBB_MEMCPY (amb_ip_addr, &pdsa_ip_addr.addr, *len);
    return;
}

NBB_VOID
pdsa_convert_amb_ip_addr_to_ip_addr (NBB_BYTE      *amb_ip_addr,
                                     NBB_LONG      type,
                                     NBB_ULONG     len,
                                     ip_addr_t     *pdsa_ip_addr)
{
    switch (type)
    {
        case AMB_INETWK_ADDR_TYPE_IPV4:
            pdsa_ip_addr->af = IP_AF_IPV4;
            break;

        case AMB_INETWK_ADDR_TYPE_IPV6:
            pdsa_ip_addr->af = IP_AF_IPV6;
            break;

        default:
            assert(0);
    }

    NBB_MEMCPY (&pdsa_ip_addr->addr, amb_ip_addr, len);
    return;
}

NBB_VOID
pdsa_convert_long_to_pdsa_ipv4_addr (NBB_ULONG ip, ip_addr_t *pdsa_ip_addr)
{
    pdsa_ip_addr->af            = IP_AF_IPV4;
    pdsa_ip_addr->addr.v4_addr  = htonl(ip);
}


NBB_VOID
pdsa_set_address_oid(NBB_ULONG *oid,
                     const NBB_CHAR  *tableName,
                     const NBB_CHAR  *fieldName,
                     const types::IPAddress &addr)
{
    NBB_ULONG       oidAddrTypeIdx;
    NBB_ULONG       oidAddrIdx;
    NBB_LONG        ambAddrType;
    NBB_ULONG       ambAddrLen;
    NBB_BYTE        ambAddr[AMB_BGP_MAX_IP_PREFIX_LEN];
    ip_addr_t       outAddr;
    NBB_ULONG       ii = 0;

    if (strcmp(fieldName, "remote_addr") == 0) {
        if (strcmp(tableName, "bgpPeerAfiSafiTable") == 0) {
            oidAddrTypeIdx = AMB_BGP_PAS_REMOTE_ADD_TYP_IX;
            oidAddrIdx = AMB_BGP_PAS_REMOTE_ADDR_INDEX;
        } else if (strcmp(tableName, "bgpPeerTable") == 0) {
            oidAddrTypeIdx = AMB_BGP_PER_REMOTE_ADD_TYP_IX;
            oidAddrIdx = AMB_BGP_PER_REMOTE_ADDR_INDEX;
        } else if (strcmp(tableName, "bgpPeerAfiSafiStatusTable") == 0) {
            oidAddrTypeIdx = AMB_BGP_PAST_REMOTE_ADDR_TYP_IX;
            oidAddrIdx = AMB_BGP_PAST_REMOTE_ADDR_INDEX;
        } else if (strcmp(tableName, "bgpPeerStatusTable") == 0) {
            oidAddrTypeIdx = AMB_BGP_PRST_LOCAL_ADDR_TYP_IX;
            oidAddrIdx = AMB_BGP_PRST_LOCAL_ADDR_INDEX;
        } else {
            assert(0);
        }
    } else if (strcmp(fieldName, "local_addr") == 0) {
        if (strcmp(tableName, "bgpPeerAfiSafiTable") == 0) {
            oidAddrTypeIdx = AMB_BGP_PAS_LOCAL_ADD_TYP_INDEX;
            oidAddrIdx = AMB_BGP_PAS_LOCAL_ADDR_INDEX;
        } else if (strcmp(tableName, "bgpPeerTable") == 0) {
            oidAddrTypeIdx = AMB_BGP_PER_LOCAL_ADD_TYP_INDEX;
            oidAddrIdx = AMB_BGP_PER_LOCAL_ADDR_INDEX;
        } else if (strcmp(tableName, "bgpPeerAfiSafiStatusTable") == 0) {
            oidAddrTypeIdx = AMB_BGP_PAST_LOCAL_ADDR_TYP_IX;
            oidAddrIdx = AMB_BGP_PAST_LOCAL_ADDR_INDEX;
        } else if (strcmp(tableName, "bgpPeerStatusTable") == 0) {
            oidAddrTypeIdx = AMB_BGP_PRST_REMOTE_ADDR_TYP_IX;
            oidAddrIdx = AMB_BGP_PRST_REMOTE_ADDR_INDEX;
        } else {
            assert(0);
        }
    } else if (strcmp(fieldName, "ipaddress")  == 0) {
        if (strcmp(tableName, "limL3InterfaceAddressTable") == 0) {
            oidAddrTypeIdx = AMB_LIM_L3_ADDR_IPDDR_TYP_INDEX;
            oidAddrIdx = AMB_LIM_L3_ADDR_IPADDR_INDEX;
        }
    } else {
        assert(0);
    }

    ip_addr_spec_to_ip_addr(addr, &outAddr);
    pdsa_convert_ip_addr_to_amb_ip_addr(outAddr, &ambAddrType, &ambAddrLen, ambAddr);

    // Fill oid now
    oid[oidAddrTypeIdx]   = ambAddrType;
    oid[oidAddrIdx]       = ambAddrLen;
    for (ii = 0; ii < ambAddrLen; ii++)
    {
        oid[oidAddrIdx + 1 + ii] =
            (NBB_ULONG) ambAddr[ii];
    }
}

NBB_VOID
pdsa_set_address_field(AMB_GEN_IPS *mib_msg,
                       const NBB_CHAR  *tableName,
                       const NBB_CHAR  *fieldName,
                       NBB_VOID        *dest,
                       const types::IPAddress &addr)
{
    NBB_ULONG       addrIdx;
    NBB_ULONG       addrType;
    ip_addr_t       outAddr;

    ip_addr_spec_to_ip_addr(addr, &outAddr);

    if (strcmp(fieldName, "remote_addr") == 0) {
        if (strcmp(tableName, "bgpPeerAfiSafiTable") == 0) {
            AMB_BGP_PEER_AFI_SAFI *data = (AMB_BGP_PEER_AFI_SAFI *)dest;
            pdsa_convert_ip_addr_to_amb_ip_addr(outAddr,
                                                &data->remote_addr_type,
                                                &data->remote_addr_len,
                                                data->remote_addr);
            addrIdx = AMB_OID_BGP_PAS_REMOTE_ADDR;
            addrType = AMB_OID_BGP_PAS_REMOTE_ADDR_TYP;
        } else if (strcmp(tableName, "bgpPeerTable") == 0) {
            AMB_BGP_PEER *data = (AMB_BGP_PEER *)dest;
            pdsa_convert_ip_addr_to_amb_ip_addr(outAddr,
                                                &data->remote_addr_type,
                                                &data->remote_addr_len,
                                                data->remote_addr);
            addrIdx = AMB_OID_BGP_PER_REMOTE_ADDR;
            addrType = AMB_OID_BGP_PER_REMOTE_DDR_TYP;
        } else {
            assert(0);
        }
    } else if (strcmp(fieldName, "local_addr") == 0) {
        if (strcmp(tableName, "bgpPeerAfiSafiTable") == 0) {
            AMB_BGP_PEER_AFI_SAFI *data = (AMB_BGP_PEER_AFI_SAFI *)dest;
            pdsa_convert_ip_addr_to_amb_ip_addr(outAddr,
                                                &data->local_addr_type,
                                                &data->local_addr_len,
                                                data->local_addr);
            addrIdx = AMB_OID_BGP_PAS_LOCAL_ADDR;
            addrType = AMB_OID_BGP_PAS_LOCAL_ADDR_TYP;
        } else if (strcmp(tableName, "bgpPeerTable") == 0) {
            AMB_BGP_PEER *data = (AMB_BGP_PEER *)dest;
            pdsa_convert_ip_addr_to_amb_ip_addr(outAddr,
                                                &data->local_addr_type,
                                                &data->local_addr_len,
                                                data->local_addr);
            addrIdx = AMB_OID_BGP_PER_LOCAL_ADDR;
            addrType = AMB_OID_BGP_PER_LOCAL_ADDR_TYP;
        } else {
            assert(0);
        }
    } else if (strcmp(fieldName, "ipaddress")  == 0) {
        if (strcmp(tableName, "limL3InterfaceAddressTable") == 0) {
            AMB_LIM_L3_IF_ADDR *data = (AMB_LIM_L3_IF_ADDR *)dest;
            pdsa_convert_ip_addr_to_amb_ip_addr (outAddr,
                                                 &data->ipaddr_type, 
                                                 &data->ipaddress_len,
                                                 data->ipaddress);
            addrIdx  = AMB_OID_LIM_L3_ADDR_IPADDR;
            addrType = AMB_OID_LIM_L3_ADDR_TYPE; 
        }
    } else {
        assert(0);
    }
    AMB_SET_FIELD_PRESENT (mib_msg, addrIdx);
    AMB_SET_FIELD_PRESENT (mib_msg, addrType);
}

NBB_LONG
pdsa_nbb_get_long(NBB_BYTE *byteVal)
{
    NBB_LONG val;
    NBB_GET_LONG(val, byteVal);
    return val;
}

types::IPAddress   res;

types::IPAddress*
pdsa_get_address(const NBB_CHAR  *tableName,
                 const NBB_CHAR  *fieldName,
                 NBB_VOID        *src)
{
    ip_addr_t          pdsa_ip_addr;

    if (strcmp(fieldName, "remote_addr") == 0) {
        if (strcmp(tableName, "bgpPeerAfiSafiStatusTable") == 0) {
            AMB_BGP_PEER_AFI_SAFI_STAT *data = (AMB_BGP_PEER_AFI_SAFI_STAT *)src;
            pdsa_convert_amb_ip_addr_to_ip_addr(data->remote_addr,
                                                data->remote_addr_type,
                                                data->remote_addr_len,
                                                &pdsa_ip_addr);
        } else if (strcmp(tableName, "bgpPeerStatusTable") == 0) {
            AMB_BGP_PEER_STATUS *data = (AMB_BGP_PEER_STATUS *)src;
            pdsa_convert_amb_ip_addr_to_ip_addr(data->remote_addr,
                                                data->remote_addr_type,
                                                data->remote_addr_len,
                                                &pdsa_ip_addr);
        } else {
            assert(0);
        }
    } else if (strcmp(fieldName, "local_addr") == 0) {
        if (strcmp(tableName, "bgpPeerAfiSafiStatusTable") == 0) {
            AMB_BGP_PEER_AFI_SAFI_STAT *data = (AMB_BGP_PEER_AFI_SAFI_STAT *)src;
            pdsa_convert_amb_ip_addr_to_ip_addr(data->local_addr,
                                                data->local_addr_type,
                                                data->local_addr_len,
                                                &pdsa_ip_addr);
        } else if (strcmp(tableName, "bgpPeerStatusTable") == 0) {
            AMB_BGP_PEER_STATUS *data = (AMB_BGP_PEER_STATUS *)src;
            pdsa_convert_amb_ip_addr_to_ip_addr(data->local_addr,
                                                data->local_addr_type,
                                                data->local_addr_len,
                                                &pdsa_ip_addr);
        } else {
            assert(0);
        }
    } else {
        assert(0);
    }
    ip_addr_to_spec(&pdsa_ip_addr, &res);
    return &res;
}

// TODO: Temporary APIs to fill bytes fields of internal.proto. 
// These APIs will be auto-generated and removed later

namespace pds {
NBB_VOID
pdsa_fill_lim_vrf_name_field (LimVrfSpec req, AMB_GEN_IPS *mib_msg)
{
    AMB_LIM_VRF *data = NULL;

    data = (AMB_LIM_VRF *)((NBB_BYTE *)mib_msg + mib_msg->data_offset); 

    data->vrf_name_len = (ulong)req.vrfnamelen();
    NBB_MEMCPY (data->vrf_name, req.vrfname().c_str(), data->vrf_name_len); 

    AMB_SET_FIELD_PRESENT (mib_msg, AMB_OID_LIM_VRF_NAME);
    AMB_SET_FIELD_PRESENT (mib_msg, AMB_OID_LIM_VRF_NAME_LEN);
}

NBB_VOID
pdsa_fill_lim_vrf_name_oid (LimVrfSpec& req, NBB_ULONG *oid)
{
    NBB_ULONG   ii = 0;
    NBB_ULONG   vrf_name_len = 0;
    NBB_BYTE    vrf_name[AMB_VRF_NAME_MAX_LEN] = {0};

    vrf_name_len = (ulong)req.vrfnamelen();
    NBB_MEMCPY (vrf_name, req.vrfname().c_str(), vrf_name_len); 
    for (ii = 0; ii < vrf_name_len; ii++)
    {
        oid[AMB_LIM_VRF_NAME_INDEX + ii] = (NBB_ULONG)vrf_name[ii];
    }
    oid[AMB_LIM_VRF_NAME_LEN_INDEX] = vrf_name_len;
}

NBB_VOID
pdsa_fill_evpn_vrf_name_field (EvpnIpVrfSpec req, AMB_GEN_IPS *mib_msg)
{
    AMB_EVPN_IP_VRF *data = NULL;

    data = (AMB_EVPN_IP_VRF *)((NBB_BYTE *)mib_msg + mib_msg->data_offset); 

    data->vrf_name_len = (ulong)req.vrfnamelen();
    NBB_MEMCPY (data->vrf_name, req.vrfname().c_str(), (ulong)req.vrfnamelen()); 

    AMB_SET_FIELD_PRESENT (mib_msg, AMB_OID_EVPN_IP_VRF_NAME);
}

NBB_VOID
pdsa_fill_evpn_vrf_name_oid (EvpnIpVrfSpec& req, NBB_ULONG *oid)
{
    NBB_ULONG ii = 0;
    NBB_ULONG vrf_name_len = 0;
    NBB_BYTE  vrf_name[AMB_VRF_NAME_MAX_LEN] = {0};      

    vrf_name_len = (ulong)req.vrfnamelen();
    NBB_MEMCPY (vrf_name, req.vrfname().c_str(), vrf_name_len); 
    for (ii = 0; ii < vrf_name_len; ii++)
    {
        oid[AMB_EVPN_IP_VRF_NAME_INDEX + ii] = (NBB_ULONG)vrf_name[ii];
    }
    oid[AMB_EVPN_IP_VRF_NAME_LEN_INDEX] = vrf_name_len;
}

NBB_VOID
pdsa_fill_evpn_evi_rd_field (EvpnEviSpec req, AMB_GEN_IPS *mib_msg)
{
    AMB_EVPN_EVI *data = NULL;

    data = (AMB_EVPN_EVI *)((NBB_BYTE *)mib_msg + mib_msg->data_offset); 

    if (req.autord() == AMB_EVPN_CONFIGURED) {
        NBB_MEMCPY (data->cfg_rd, req.cfgrd().c_str(), AMB_EVPN_EXT_COMM_LENGTH); 
        AMB_SET_FIELD_PRESENT (mib_msg, AMB_OID_EVPN_EVI_CFG_RD);
    }
}

NBB_VOID
pdsa_fill_evpn_vrf_rd_field (EvpnIpVrfSpec req, AMB_GEN_IPS *mib_msg)
{
    AMB_EVPN_IP_VRF *data = NULL;

    data = (AMB_EVPN_IP_VRF *)((NBB_BYTE *)mib_msg + mib_msg->data_offset); 

    // if not default RD then set it
    if (req.defaultrd() != AMB_TRUE)  {
        NBB_MEMCPY (data->route_distinguisher, req.rd().c_str(), AMB_EVPN_EXT_COMM_LENGTH); 
        AMB_SET_FIELD_PRESENT (mib_msg, AMB_OID_EVPN_IP_VRF_NAME);
    }
}

NBB_VOID
pdsa_fill_lim_if_cfg_vrf_name_field (LimInterfaceCfgSpec req, AMB_GEN_IPS *mib_msg)
{
    AMB_LIM_IF_CFG *data = NULL;

    data = (AMB_LIM_IF_CFG *)((NBB_BYTE *)mib_msg + mib_msg->data_offset); 

    
    data->bind_vrf_name_len= (ulong)req.vrfnamelen();
    if (data->bind_vrf_name_len) {
        NBB_MEMCPY (data->bind_vrf_name, req.vrfname().c_str(), (ulong)req.vrfnamelen()); 
        AMB_SET_FIELD_PRESENT (mib_msg, AMB_OID_LIM_IF_CFG_BIND_VRFNAME);
    }
}
} // namespace pds
// End of Temporary APIs
