/*
* Copyright (c) 2019, Pensando Systems Inc.
*/

#ifndef __MCTP_TRANSPORT_H__
#define __MCTP_TRANSPORT_H__

#include <string>
#include <fcntl.h>
#include "libmctp.h"
#include "libmctp-i2c.h"

namespace sdk {
namespace platform {
namespace ncsi {

class mctp_transport : public transport {

private:
    int mctp_fd;
    int local_eid_default;
    struct mctp *mctp;
    struct mctp_binding_i2c *i2c;
    uint8_t last_src_eid;
    uint8_t last_tag;
    ssize_t last_len;
    void *user_buf;
public:
    mctp_transport()
    {
        xport_name = "MCTP";
    };
    int Init()
    {
        int rc;
        const char *path = "/dev/ncsi";
        local_eid_default = 0;

        printf("In MCTP transport Init() API\n");

        mctp = mctp_init();
        i2c = mctp_i2c_init();
        if (!i2c)
            return -1;

        rc = mctp_i2c_open_path(i2c, path);
        if (rc)
            return -1;

        mctp_register_bus(mctp, mctp_binding_i2c_core(i2c), local_eid_default);
        mctp_fd = mctp_i2c_get_fd(i2c);
        mctp_set_rx_all(mctp, capture_rx_msg, this);

        return 0;
    };

    ssize_t SendPkt(const void *buf, size_t len)
    {
        ssize_t ret;
        uint8_t ncsi_mctp_hdr[] = {0x02};
        uint8_t *ncsi_mctp_buf = ((uint8_t *)buf - (sizeof(struct mctp_header)));

        memcpy(ncsi_mctp_buf, ncsi_mctp_hdr, sizeof(ncsi_mctp_hdr));
        ret = mctp_message_tx(mctp, last_src_eid, ncsi_mctp_buf, len + sizeof(struct mctp_header), last_tag);
        if (ret != 0) {
            printf("Error in sending packet out over MCTP interface: error: %ld",
                    ret);
            return 0;
        }

        free(ncsi_mctp_buf);
        return (len + sizeof(struct mctp_header));
    };

    static void capture_rx_msg(uint8_t src_eid, void *data, void *msg, size_t len, uint8_t tag)
    {
        mctp_transport *obj = (mctp_transport *)data;

        memcpy(obj->user_buf, msg, len);
        obj->last_len = len;
        obj->last_tag = tag;
        obj->last_src_eid = src_eid;
    };

    ssize_t RecvPkt(void *buf, size_t len, size_t& ncsi_hdr_offset)
    {
        int rc;
        user_buf = buf;
        rc = mctp_i2c_read(i2c);

        if (rc)
            printf("Error in mctp_i2c_read\n");

        ncsi_hdr_offset = sizeof(struct mctp_header);

#if 0
        uint8_t *ptr = (uint8_t *)buf;
        printf("\n pkt received on MCTP interface: sz: %ld bytes\n", last_len);
        for (uint32_t i = 0; i < last_len; i++)
        {
            printf("%02x ", ptr[i]);
            if (!((i+1) % 16))
                printf("\n");
        }
        printf("\n");
#endif
        return last_len;
    }

    int GetFd() { return mctp_fd; };
    void *GetNcsiRspPkt(ssize_t sz)
    {
        void *ptr = calloc(sizeof(struct mctp_header) + sz, sizeof(uint8_t));
        return ((uint8_t *)ptr + sizeof(struct mctp_header));
    }

};

} // namespace ncsi
} // namespace platform
} // namespace sdk

#endif //__MCTP_TRANSPORT_H__

