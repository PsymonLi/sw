#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "flow_decoder.h"
#include "lib/operd/decoder.h"

static size_t
vpp_text_decoder (uint8_t encoder, const char *data, size_t data_length,
             char *output, size_t output_size)
{
    char *buf = output;
    size_t buf_len = output_size;
    size_t n;
    const operd_flow_t *flow = (operd_flow_t *)data;

    assert(sizeof(*flow) == data_length);
    
    switch (flow->action) {
    case OPERD_FLOW_ACTION_ALLOW:
        n = snprintf(buf, buf_len, "Allow, ");
        buf_len -= n;
        buf += n;
        break;
    case OPERD_FLOW_ACTION_DENY:
        n = snprintf(buf, buf_len, "Deny, ");
        buf_len -= n;
        buf += n;
        break;
    default:
        assert(0);
    }

    switch (flow->logtype) {
    case OPERD_FLOW_LOGTYPE_ADD:
        n = snprintf(buf, buf_len, "Open, ");
        buf_len -= n;
        buf += n;
        break;
    case OPERD_FLOW_LOGTYPE_DEL:
        n = snprintf(buf, buf_len, "Close, ");
        buf_len -= n;
        buf += n;
        break;
    case OPERD_FLOW_LOGTYPE_ACTIVE:
        n = snprintf(buf, buf_len, "Active, ");
        buf_len -= n;
        buf += n;
        break;
    default:
        assert(0);
    }

    
    n = snprintf(buf, buf_len, "Session: %" PRIu32 ", ", flow->session_id);
    buf_len -= n;
    buf += n;

    n = snprintf(buf, buf_len, "I bytes: %" PRIu64 ", ",
                 flow->stats.iflow_bytes_count);
    buf_len -= n;
    buf += n;

    n = snprintf(buf, buf_len, "I packets: %" PRIu64 ", ",
                 flow->stats.iflow_packets_count);
    buf_len -= n;
    buf += n;

    n = snprintf(buf, buf_len, "R bytes: %" PRIu64 ", ",
                 flow->stats.rflow_bytes_count);
    buf_len -= n;
    buf += n;

    n = snprintf(buf, buf_len, "R packets: %" PRIu64 ", ",
                 flow->stats.rflow_packets_count);
    buf_len -= n;
    buf += n;

    switch (flow->type) {
    case OPERD_FLOW_TYPE_IP4:
    {
        struct in_addr addr;

        n = snprintf(buf, buf_len, "IPV4: lookup-id %" PRIu16 ", ",
                     flow->v4.lookup_id);
        buf_len -= n;
        buf += n;

        addr.s_addr = flow->v4.src;
        n = snprintf(buf, buf_len, "src %s:%" PRIu16 ", ",
                     inet_ntoa(addr), flow->v4.sport);
        buf_len -= n;
        buf += n;

        addr.s_addr = flow->v4.dst;
        n = snprintf(buf, buf_len, "dst %s:%" PRIu16 ", ",
                     inet_ntoa(addr), flow->v4.dport);
        buf_len -= n;
        buf += n;

        
        n = snprintf(buf, buf_len, "proto %" PRIu8,
                     flow->v4.proto);
        buf_len -= n;
        buf += n;
    }
    break;
    case OPERD_FLOW_TYPE_IP6:
    {
        n = snprintf(buf, buf_len, "IPv6, ");
        buf_len -= n;
        buf += n;
    }
    break;
    case OPERD_FLOW_TYPE_L2:
    {
        n = snprintf(buf, buf_len, "L2, ");
        buf_len -= n;
        buf += n;
    }
    break;
    default:
        assert(0);
        break;
    }
    
    assert(buf_len < output_size);

    fprintf(stdout, "Decoded: %s", buf);
    fflush(stdout);

    return output_size - buf_len;
}

extern "C" void
decoder_lib_init(register_decoder_fn register_decoder)
{
    register_decoder(OPERD_DECODER_VPP, vpp_text_decoder);
}
