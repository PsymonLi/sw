// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

#include <assert.h>
#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "htable.hpp"
#include "kvstore.hpp"
#include "metrics.hpp"
#include "lib/pal/pal.hpp"
#include "shm.hpp"

namespace pt = boost::property_tree;

namespace sdk {
namespace metrics {

#define METRICS_RESERVED_COUNTER "RESERVED"

static const int SDK_METRICS_DEFAULT_TBL_SIZE = 1000;

typedef enum metrics_counter_type_ {
    METRICS_COUNTER_VALUE64   = 1, // A 64bit value
    METRICS_COUNTER_POINTER64 = 2, // A pointer to a 64bit value
    METRICS_COUNTER_RSVD64    = 3, // A reserved counter (to be ignored)
} metrics_counter_type_t;

typedef struct metrics_counter_ {
    const char *name; // Name of the metric. e.g. InboundPackets
    metrics_counter_type_t type;
} metrics_counter_t;

struct counter_ {
    std::string name;
    metrics_counter_type_t type;
};

typedef struct metrics_table_ {
    std::string name;
    TableMgrUptr tbl;
    metrics_type_t type;
    size_t row_size;
    std::vector<struct counter_> counters;
} metrics_table_t;

typedef struct buffer_ {
    int length;
    char data[];
} __attribute__((packed)) buffer_t;

typedef struct serialized_spec_ {
    unsigned int name_length;
    metrics_type_t type;
    unsigned int counter_count;
    char         name[];
} serialized_spec_t;

typedef struct serialized_counter_spec_ {
    unsigned int           name_length;
    metrics_counter_type_t type;
    char                   name[];
} serialized_counter_spec_t;

static ShmPtr
get_shm (void)
{
    static ShmPtr shm = nullptr;

    if (shm == nullptr) {
        shm = std::make_shared<Shm>();
        error err = shm->MemMap(SDK_METRICS_SHM_NAME, SDK_METRICS_SHM_SIZE);
        assert(err == error::OK());
    }

    return shm;
}

static TableMgrUptr
get_meta_table (void)
{
    TableMgrUptr tbl = nullptr;

    if (tbl == nullptr) {
        tbl = get_shm()->Kvstore()->Table("__metrics_meta__");
        if (tbl == nullptr) {
            tbl = get_shm()->Kvstore()->CreateTable(
                "__metrics_meta__", SDK_METRICS_DEFAULT_TBL_SIZE);
            assert(tbl != nullptr);
        }
    }

    return tbl;
}

static inline std::string
get_schema_file (std::string name)
{
    std::string cfg_path;

    cfg_path = std::string(std::getenv("CONFIG_PATH"));
    if (cfg_path.empty()) {
        cfg_path = std::string("./");
    } else {
        cfg_path += "/";
    }

    return cfg_path + name;
}

static void
save_schema_ (metrics_table_t *tbl, schema_t *schema)
{
    pt::ptree root;
    std::string schema_file;
    serialized_spec_t *srlzd;
    int16_t srlzd_len;
    int cnt = 0;

    // todo
    // check if it already exists and crash if mismatch

    schema_file = get_schema_file(schema->filename);
    if (access(schema_file.c_str(), R_OK) < 0) {
        fprintf(stderr, "%s doesn't exist or not accessible\n", schema_file.c_str());
        assert(0);
        return;
    }

    read_json(schema_file, root);
    BOOST_FOREACH(boost::property_tree::ptree::value_type &msgs,
                  root.get_child("Messages")) {
        std::string msg_name = msgs.second.get<std::string>("Name");
        if (msg_name != schema->name) {
            continue;
        }
        BOOST_FOREACH(boost::property_tree::ptree::value_type &fields,
                      msgs.second.get_child("Fields")) {
            std::string key = tbl->name + ":" + std::to_string(cnt++);
            // get the counter name
            std::string cntr_name = fields.second.get<std::string>("Name");
            serialized_counter_spec_t *srlzd_counter;
            int16_t srlzd_counter_len =  sizeof(serialized_counter_spec_t *) +
                                         cntr_name.size() + 1;
            metrics_counter_type_t type;

            if (tbl->type == SW) {
                type = METRICS_COUNTER_VALUE64;
            } else {
                if (cntr_name.compare(METRICS_RESERVED_COUNTER) == 0) {
                    type = METRICS_COUNTER_RSVD64;
                } else {
                    type = METRICS_COUNTER_POINTER64;
                }
            }

            srlzd_counter = (serialized_counter_spec_t *)
                            malloc(srlzd_counter_len);
            srlzd_counter->name_length = cntr_name.size();
            memcpy(srlzd_counter->name, cntr_name.c_str(),
                   srlzd_counter->name_length + 1);
            srlzd_counter->type = type;

            error err = get_meta_table()->Publish(key.c_str(), key.size(),
                                                  (char *)srlzd_counter,
                                                  srlzd_counter_len);
            assert(err == error::OK());

            free(srlzd_counter);

            tbl->counters.push_back({name: cntr_name.c_str(), type: type,});
        }
        break;
    }

    tbl->row_size = tbl->counters.size() * sizeof(uint64_t);
    srlzd_len = sizeof(*srlzd) + schema->name.size() + 1;
    srlzd = (serialized_spec_t *)malloc(srlzd_len);
    srlzd->name_length = schema->name.size();
    srlzd->type = tbl->type;
    memcpy(&srlzd->name, schema->name.c_str(), srlzd->name_length + 1);
    srlzd->counter_count = cnt;

    error err = get_meta_table()->Publish(
        tbl->name.c_str(), tbl->name.size(), (char *)srlzd, srlzd_len);
    assert(err == error::OK());

    free(srlzd);
}

static metrics_table_t *
load_table_ (const char *name)
{
    metrics_table_t *tbl;
    serialized_spec_t *srlzd;

    tbl = new metrics_table_t();
    tbl->name = name;

    srlzd = (serialized_spec_t *)get_meta_table()->Find(
        tbl->name.c_str(), tbl->name.size());
    if (srlzd == NULL) {
        delete tbl;
        return NULL;
    }

    tbl->type = srlzd->type;

    for (unsigned int i = 0; i < srlzd->counter_count; i++) {
        serialized_counter_spec_t *srlzd_counter;
        std::string key = tbl->name + ":" + std::to_string(i);

        srlzd_counter = (serialized_counter_spec_t *)get_meta_table()->Find(
            key.c_str(), key.size());
        assert(srlzd_counter != NULL);

        tbl->counters.push_back({
            name: srlzd_counter->name,
            type: srlzd_counter->type,
            });
    }
    tbl->row_size = tbl->counters.size() * sizeof(uint64_t);

    return tbl;
}

void *
create (schema_t *schema, bool counter_block)
{
    metrics_table_t *tbl;
    TableMgrUptr tbmgr;


    tbmgr = get_shm()->Kvstore()->Table(schema->name.c_str());
    if (tbmgr == nullptr) {
        tbl = new metrics_table_t();
        tbl->tbl = get_shm()->Kvstore()->CreateTable(
            schema->name.c_str(), SDK_METRICS_DEFAULT_TBL_SIZE);
        assert(tbl->tbl != nullptr);
        tbl->name = schema->name;
        tbl->type = schema->type;
        save_schema_(tbl, schema);
    } else {
        tbl = load_table_(schema->name.c_str());
        assert(tbl != NULL);
        tbl->tbl = std::move(tbmgr);
    }

    return tbl;
}

void
row_address(void *handle, key_t key, void *address,
            uint32_t counter_block_size) {
    metrics_table_t *tbl = (metrics_table_t *)handle;
    assert(tbl->type == HBM);

    error err = tbl->tbl->Publish((char *)&key, sizeof(key), (char *)&address,
                                  sizeof(address));

    assert(err == error::OK());
}

void
metrics_update (void *handle, key_t key, uint64_t *values)
{
    metrics_table_t *tbl = (metrics_table_t *)handle;

    error err = tbl->tbl->Publish((char *)&key, sizeof(key), (char *)values,
                                  tbl->row_size);
    assert(err == error::OK());
}

void *
metrics_open (const char *name)
{
    metrics_table_t *tbl = load_table_(name);

    if (tbl == NULL) {
        return NULL;
    }
    tbl->tbl = get_shm()->Kvstore()->Table(name);
    if (tbl->tbl == nullptr) {
        delete tbl;
        return NULL;
    }

    return tbl;
}

static counters_t
metrics_read_values (void *handle, key_t key)
{
    metrics_table_t *tbl;
    counters_t counters;
    uint64_t *values;

    tbl = (metrics_table_t *)handle;

    values = (uint64_t *)tbl->tbl->Find((char *)&key, sizeof(key));

    for (unsigned int i = 0; i < tbl->counters.size(); i++) {
        metrics_counter_pair_t pair;

        pair.first = tbl->counters[i].name;
        if (values) {
            pair.second = values[i];
        } else {
            pair.second = 0;
        }
        counters.push_back(pair);
    }

    return counters;
}

static uint64_t
read_value (uint64_t base, unsigned int offset)
{
    uint64_t value;
    int rc;

    rc = sdk::lib::pal_reg_read(base + (offset * sizeof(value)),
                                (uint32_t *)&value, 2);
    assert(rc == sdk::lib::PAL_RET_OK);

    return value;
}

static counters_t
metrics_read_pointers (void *handle, key_t key)
{
    metrics_table_t *tbl;
    counters_t counters;
    uint64_t *base;

    tbl = (metrics_table_t *)handle;

    base = (uint64_t *)tbl->tbl->Find((char *)&key, sizeof(key));
    if (base == NULL) {
        return counters;
    }

    for (unsigned int i = 0; i < tbl->counters.size(); i++) {
        if (tbl->counters[i].type != METRICS_COUNTER_RSVD64) {
            metrics_counter_pair_t pair;

            pair.first = tbl->counters[i].name;
            pair.second = read_value(*base, i);
            counters.push_back(pair);
        }
    }

    return counters;
}

counters_t
metrics_read (void *handle, key_t key)
{
    metrics_table_t *tbl;

    tbl = (metrics_table_t *)handle;

    if (tbl->type == SW) {
        return metrics_read_values(handle, key);
    } else {
        return metrics_read_pointers(handle, key);
    }
}

} // namespace sdk
} // namespace metrics
