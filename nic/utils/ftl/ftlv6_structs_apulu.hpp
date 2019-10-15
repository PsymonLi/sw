//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------
#define FTLV6_APOLLO_ENTRY_NUM_HINTS 4

struct __attribute__((__packed__)) ftlv6_entry_t {
    // data after key
    uint32_t __pad_to_512b : 10;
    uint32_t entry_valid : 1;
    uint32_t nexthop_id : 16;
    uint32_t more_hints : 18;
    uint32_t more_hashes : 1;
    uint32_t hint4 : 18;
    uint32_t hash4 : 12;
    uint32_t hint3 : 18;
    uint32_t hash3 : 12;
    uint32_t hint2 : 18;
    uint32_t hash2 : 12;
    uint32_t hint1 : 18;
    uint32_t hash1 : 12;

    // key
    uint32_t ktype : 2;
    uint8_t  src[16];
    uint32_t bd_id : 16;
    uint8_t dst[16];
    uint32_t proto : 8;
    uint32_t sport : 16;
    uint32_t dport : 16;

    // data before key
    uint32_t nexthop_type : 2;
    uint32_t nexthop_valid : 1;
    uint32_t flow_role : 1;
    uint32_t session_id : 20;
    uint32_t epoch : 8;

#ifdef __cplusplus

public:
    void set_nhgroup_index(uint32_t index) {
        nexthop_id = index & 0xffffffff;
    }

    void tostr(char *buff, uint32_t len) {
        snprintf(buff, len,
                 "[#/msb/hint: 1/%d/%d 2/%d/%d 3/%d%d 4/%d/%d M/%d/%d] "
                 "[key type:%d,bd_id:%d,proto:%d,"
                 "src:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x,"
                 "dst:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x,"
                 "sport:%d,dport:%d] [data role:%d,session:%d,nhgroup:%d,valid:%d,epoch:%d,nh_type:%d, nh_valid:%d]",
                 hint1, hash1, hint2, hash2, hint3, hash3,
                 hint4, hash4, more_hashes, more_hints,
                 ktype, bd_id, proto,
                 src[0], src[1], src[2], src[3],
                 src[4], src[5], src[6], src[7],
                 src[8], src[9], src[10], src[11],
                 src[12], src[13], src[14], src[15],
                 dst[0], dst[1], dst[2], dst[3],
                 dst[4], dst[5], dst[6], dst[7],
                 dst[8], dst[9], dst[10], dst[11],
                 dst[12], dst[13], dst[14], dst[15],
                 sport, dport, flow_role, session_id, nexthop_id,
                 entry_valid, epoch, nexthop_type, nexthop_valid);
    }

    static void keyheader2str(char *buff, uint32_t len) {
        char *cur = buff;
        cur += snprintf(cur, len, "%16s\t%16s\t%5s\t%5s\t%3s\t%4s\t%8s\n",
                        "SrcIP", "DstIP", "SrcPort", "DstPort", "Proto", "BdId", "Session");
        snprintf(cur, buff + len - cur, "%16s\t%16s\t%5s\t%5s\t%3s\t%4s\t%8s\n",
                 "-----", "-----", "-------", "-------", "-----", "----", "----");
        return;
    }

    void key2str(char *buff, uint32_t len) {
        char srcstr[INET6_ADDRSTRLEN], dststr[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, src, srcstr, INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, dst, dststr, INET6_ADDRSTRLEN);
        snprintf(buff, len, "%16s\t%16s\t%5u\t%5u\t%3u\t%4u%8u\n",
                 srcstr, dststr, sport, dport,
                 proto, bd_id, session_id);
    }

    void clear_hints() {
        __pad_to_512b = 0;
        more_hints = 0;
        more_hashes = 0;
        hash4 = 0;
        hint4 = 0;
        hash3 = 0;
        hint3 = 0;
        hash2 = 0;
        hint2 = 0;
        hint1 = 0;
        hash1 = 0;
    }

    void clear_key() {
        ftlv6_entry_t s;

        memset(&s, 0, sizeof(ftlv6_entry_t));
        copy_key(&s);
    }

    void clear_data() {
        ftlv6_entry_t s;

        memset(&s, 0, sizeof(ftlv6_entry_t));
        copy_data(&s);
    }

    void clear_key_data() {
        clear_key();
        clear_data();
    }

    void clear() {
        clear_key();
        clear_data();
        clear_hints();
    }

    void copy_key(ftlv6_entry_t *s) {
        ktype = s->ktype;
        memcpy(src, s->src, 16);
        bd_id = s->bd_id;
        memcpy(dst, s->dst, 16);
        proto = s->proto;
        sport = s->sport;
        dport = s->dport;
    }

    void copy_data(ftlv6_entry_t *s) {
        nexthop_type = s->nexthop_type;
        nexthop_valid = s->nexthop_valid;
        flow_role = s->flow_role;
        session_id = s->session_id;
        epoch = s->epoch;
        nexthop_id = s->nexthop_id;
    }

    void copy_key_data(ftlv6_entry_t *s) {
        copy_key(s);
        copy_data(s);
    }

    void build_key(ftlv6_entry_t *s) {
        copy_key(s);
        clear_data();
        clear_hints();
        entry_valid = 0;
    }

    bool compare_key(ftlv6_entry_t *s) {
        if (s->ktype != ktype) return false;
        if (memcmp(s->src, src, 16)) return false;
        if (s->bd_id != bd_id) return false;
        if (memcmp(s->dst, dst, 16)) return false;
        if (s->proto != proto) return false;
        if (s->sport != sport) return false;
        if (s->dport != dport) return false;
        return true;
    }

    void set_hint_hash(uint32_t slot, uint32_t hint, uint32_t hash) {
        assert(slot);
        switch (slot) {
        case 1: hint1 = hint; hash1 = hash; break;
        case 2: hint2 = hint; hash2 = hash; break;
        case 3: hint3 = hint; hash3 = hash; break;
        case 4: hint4 = hint; hash4 = hash; break;
        default: more_hashes = hash; more_hints = hint; break;
        }
    }

    void get_hint_hash(uint32_t slot, uint32_t &hint, uint16_t &hash) {
        assert(slot);
        switch (slot) {
        case 1: hint = hint1; hash = hash1; break;
        case 2: hint = hint2; hash = hash2; break;
        case 3: hint = hint3; hash = hash3; break;
        case 4: hint = hint4; hash = hash4; break;
        default: hint = more_hints; hash = more_hashes; break;
        }
    }

    void get_hint(uint32_t slot, uint32_t &hint) {
        assert(slot);
        switch (slot) {
        case 1: hint = hint1; break;
        case 2: hint = hint2; break;
        case 3: hint = hint3; break;
        case 4: hint = hint4; break;
        default: hint = more_hints; break;
        }
    }

    uint32_t get_num_hints() {
        return FTLV6_APOLLO_ENTRY_NUM_HINTS;
    }

    uint32_t get_more_hint_slot() {
        return get_num_hints() + 1;
    }

    bool is_hint_slot_valid(uint32_t slot) {
        if (slot && slot <= get_more_hint_slot()) {
            return true;
        }
        return false;
    }

    uint32_t find_last_hint() {
        if (more_hints && more_hashes) {
            return get_more_hint_slot();
        } else if (hint4) {
            return 4;
        } else if (hint3) {
            return 3;
        } else if (hint2) {
            return 2;
        } else if (hint1) {
            return 1;
        }
        return 0;
    }
#endif

};
