/*
* Copyright (c) 2020, Pensando Systems Inc.
*/

#ifndef __DELL_PKT_DEFS_H__
#define __DELL_PKT_DEFS_H__
#include <linux/types.h>


#define DELL_NCSI_MFG_ID                0x02A2
#define DELL_NCSI_MSG_PAYLOAD_VER       0x2
#define MAX_DELL_NCSI_CMDS              0x64

#define DELL_NCSI_CMD_OPCODE_OFFSET      21

/* data structures for requests */
struct DellNcsiFixedCmdPkt {
    struct NcsiCmdPktHdr    cmd;
    __be32                  mfg_id;
    uint8_t                 payload_ver;
    uint8_t                 cmd_id;
    uint8_t                 pad[2];
    __be32                  csum;
} __attribute__((packed));

struct DellNcsiSendEthFrameCmdPkt {
    struct NcsiCmdPktHdr    cmd;
    __be32                  mfg_id;
    uint8_t                 payload_ver;
    uint8_t                 cmd_id;
    uint8_t                 rsvd;
    /* eth frame provided by BMC is not part of struct due to variable size */
    __be32                  csum;
} __attribute__((packed));

struct DellNcsiSendLLDPCmdPkt {
    struct NcsiCmdPktHdr    cmd;
    __be32                  mfg_id;
    uint8_t                 payload_ver;
    uint8_t                 cmd_id;
    uint8_t                 rsvd;
    uint8_t                 num_tlvs;
    /* tlv fields before csum is skipped in struct definition */
    __be32                  csum;
} __attribute__((packed));

/* data structures for responses */
struct DellNcsiFixedResp {
    struct NcsiPktHdr       NcsiHdr;
    __be16                  code;
    __be16                  reason;
    __be32                  mfg_id;
    uint8_t                 payload_ver;
    uint8_t                 cmd_id;
} __attribute__((packed));

struct DellNcsiMinResp {
    struct DellNcsiFixedResp    rsp;
    __be16                      pad;
    __be32                      csum;
} __attribute__((packed));

struct DellNcsiGetInvRespPkt {
    struct DellNcsiFixedResp    rsp;
    __be16                      media_type;
    __be32                      family_fw_ver;
    __be32                      rsvd_ff;
    /* tlv fields before csum is skipped in struct definition */
    __be32                      csum;
} __attribute__((packed));

struct DellNcsiGetExtCapRespPkt {
    struct DellNcsiFixedResp    rsp;
    __be32                      capabilities;
    uint8_t                     rsvd;
    uint8_t                     dcb_cap;
    uint8_t                     nic_part_cap;
    uint8_t                     eswitch_cap;
    uint8_t                     num_pci_pf;
    uint8_t                     num_pci_vf;
    __be32                      csum;
} __attribute__((packed));

struct DellNcsiGetPartitionInfoRespPkt {
    struct DellNcsiFixedResp    rsp;
    uint8_t                     num_pci_pf_enabled;
    uint8_t                     partition_id;
    __be16                      partition_status;
    /* tlv fields before csum is skipped in struct definition */
    __be32                      csum;
} __attribute__((packed));

struct DellNcsiGetTempRespPkt {
    struct DellNcsiFixedResp    rsp;
    uint8_t                     max_temperature;
    uint8_t                     cur_temperature;
    __be32                      csum;
} __attribute__((packed));

struct DellNcsiGetSupPayloadVerRespPkt {
    struct DellNcsiFixedResp    rsp;
    __be16                      supported_ver;
    __be32                      rsvd;
    __be32                      csum;
} __attribute__((packed));

struct DellNcsiGetOSDrvVersionRespPkt {
    struct DellNcsiFixedResp    rsp;
    uint8_t                     partition_id;
    uint8_t                     num_actv_drvs_in_tlvs;
    /* tlv fields before csum is skipped in struct definition */
    __be32                      csum;
} __attribute__((packed));

struct DellNcsiGetLLDPRespPkt {
    struct DellNcsiFixedResp    rsp;
    uint8_t                     rsvd;
    uint8_t                     num_tlvs;
    /* tlv fields before csum is skipped in struct definition */
    __be32                      csum;
} __attribute__((packed));

struct DellNcsiGetInterfaceInfoRespPkt {
    struct DellNcsiFixedResp    rsp;
    uint8_t                     rsvd;
    uint8_t                     intf_type;
    /* tlv fields before csum is skipped in struct definition */
    __be32                      csum;
} __attribute__((packed));

struct DellNcsiGetInterfaceSensorRespPkt {
    struct DellNcsiFixedResp    rsp;
    uint8_t                     status;
    uint8_t                     identifier;
    /* tlv fields before csum is skipped in struct definition */
    __be32                      csum;
} __attribute__((packed));

struct SfpSesnorInfo {
    __be16 temp_high_alarm_threshold;
    __be16 temp_high_warn_threshold;
    __be16 temperature;
    __be16 voltage;
    __be16 tx_bias_cur;
    __be16 tx_out_pwr;
    __be16 rx_in_power;
    uint8_t flag1; 
    uint8_t flag2; 
    uint8_t flag3; 
    uint8_t flag4;
    __be16 pad;
} __attribute__((packed));

class DellTLVFields {
public:
    uint8_t     type;
    uint8_t     len;
    uint8_t     val[128]; //Dell TLV can take 128 bytes max
    ssize_t     sizeof_tlv() 
    {
        return (sizeof(type) + sizeof(len) + len);
    };

    DellTLVFields(uint8_t tlv_type, uint8_t tlv_len, const char* tlv_val)
    {
        type = tlv_type;
        len = tlv_len;
        memcpy(val, tlv_val, tlv_len);
    }
    ssize_t copy_tlv(uint8_t *dst)
    {
        *dst = this->type;
        *(dst + sizeof(type)) = this->len;
        memcpy((dst + sizeof(type) + sizeof(len)), this->val, this->len);
        return this->sizeof_tlv();
    }
};

/* Dell NCSI Commands Opcodes */
typedef enum {
    CMD_GET_INVENTORY            = 0x00,
    CMD_GET_EXT_CAPABILITIES     = 0x01,
    CMD_GET_PARTITION_INFO       = 0x02,
    CMD_GET_TEMPERATURE          = 0x13,
    CMD_GET_SUP_PAYLOAD_VER      = 0x1A,
    CMD_GET_OS_DRIVER_VER        = 0x1C,
    CMD_GET_LLDP                 = 0x28,
    CMD_GET_INTF_INFO            = 0x29,
    CMD_GET_INTF_SENSOR          = 0x2A,
    CMD_SEND_ETH_FRAME           = 0x2B,
    CMD_SEND_LLDP                = 0x62,
} DellNcsiCmd;

#endif //__DELL_PKT_DEFS_H__
