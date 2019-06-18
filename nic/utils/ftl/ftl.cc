//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------
#include "ftl_base.hpp"

sdk::table::properties_t * FTL_AFPFX()::props_ = NULL;
void * FTL_AFPFX()::main_table_                = NULL;
sdk::utils::crcFast * FTL_AFPFX()::crc32gen_   = NULL;
uint32_t hw_entry_count_                       = 0;

#define FTL_API_BEGIN(_name) {\
    FTL_TRACE_VERBOSE("%s %s begin: %s %s", \
                      "--", FTL_AFPFX_STR(), _name, "--");\
}

#define FTL_API_END(_name, _status) {\
   FTL_TRACE_VERBOSE("%s %s end: %s (r:%d) %s", \
                     "--", FTL_AFPFX_STR(), _name, _status, "--");\
}

#define FTL_API_BEGIN_() {\
        FTL_API_BEGIN(props_->name.c_str());\
}

#define FTL_API_END_(_status) {\
        FTL_API_END(props_->name.c_str(), (_status));\
}

#ifndef SIM
#define CRC32X(crc, value) __asm__("crc32x %w[c], %w[c], %x[v]":[c]"+r"(crc):[v]"r"(value))
#define RBITX(value) __asm__("rbit %x0, %x1": "=r"(value) : "r"(value))
#define RBITW(value) __asm__("rbit %w0, %w1": "=r"(value) : "r"(value))
#define REVX(value) __asm__("rev %x0, %x1": "=r"(value) : "r"(value))
uint32_t
crc32_aarch64(const uint64_t *p) {
    uint32_t crc = 0;
    for (auto i = 0; i < 8; i++) {
        auto v = p[i];
        RBITX(v);
        REVX(v);
        CRC32X(crc, v);
    }
    RBITW(crc);
    return crc;
}
#endif

//---------------------------------------------------------------------------
// Factory method to instantiate the class
//---------------------------------------------------------------------------
FTL_AFPFX() *
FTL_AFPFX()::factory(sdk_table_factory_params_t *params) {
    void *mem = NULL;
    FTL_AFPFX() *f = NULL;
    sdk_ret_t ret = SDK_RET_OK;

    //FTL_API_BEGIN("NewTable");
    mem = (FTL_AFPFX() *) SDK_CALLOC(SDK_MEM_ALLOC_FTL, sizeof(FTL_AFPFX()));
    if (mem) {
        f = new (mem) FTL_AFPFX()();
        ret = f->init_(params);
        if (ret != SDK_RET_OK) {
            f->~FTL_AFPFX()();
            SDK_FREE(SDK_MEM_ALLOC_FTL, mem);
            f = NULL;
        }
    } else {
        ret = SDK_RET_OOM;
    }

    FTL_API_END("NewTable", ret);
    return f;
}

sdk_ret_t
FTL_AFPFX()::init_(sdk_table_factory_params_t *params) {
    p4pd_error_t p4pdret;
    p4pd_table_properties_t tinfo, ctinfo;
    thread_id_ = params->thread_id;

    if (props_) {
        goto skip_props;
    }

    props_ = (sdk::table::properties_t *)SDK_CALLOC(SDK_MEM_ALLOC_FTL_PROPERTIES,
                                                 sizeof(sdk::table::properties_t));
    if (props_ == NULL) {
        return SDK_RET_OOM;
    }

    props_->ptable_id = params->table_id;
    props_->num_hints = params->num_hints;
    props_->max_recircs = params->max_recircs;
    //props_->health_monitor_func = params->health_monitor_func;
    props_->key2str = params->key2str;
    props_->data2str = params->appdata2str;
    props_->entry_trace_en = params->entry_trace_en;

    p4pdret = p4pd_table_properties_get(props_->ptable_id, &tinfo);
    SDK_ASSERT_RETURN(p4pdret == P4PD_SUCCESS, SDK_RET_ERR);

    props_->name = tinfo.tablename;

    props_->ptable_size = tinfo.tabledepth;
    SDK_ASSERT(props_->ptable_size);

    props_->hash_poly = tinfo.hash_type;

    props_->ptable_base_mem_pa = tinfo.base_mem_pa;
    props_->ptable_base_mem_va = tinfo.base_mem_va;

    props_->stable_id = tinfo.oflow_table_id;
    SDK_ASSERT(props_->stable_id);

    // Assumption: All ftl tables will have a HINT table
    SDK_ASSERT(tinfo.has_oflow_table);

    p4pdret = p4pd_table_properties_get(props_->stable_id, &ctinfo);
    SDK_ASSERT_RETURN(p4pdret == P4PD_SUCCESS, SDK_RET_ERR);

    props_->stable_base_mem_pa = ctinfo.base_mem_pa;
    props_->stable_base_mem_va = ctinfo.base_mem_va;

    props_->stable_size = ctinfo.tabledepth;
    SDK_ASSERT(props_->stable_size);

skip_props:
    if (main_table_) {
        goto skip_tables;
    }

    main_table_ = FTL_MAKE_AFTYPE(main_table)::factory(props_);
    SDK_ASSERT_RETURN(main_table_, SDK_RET_OOM);

    FTL_TRACE_INFO("Creating Flow table.");
    FTL_TRACE_INFO("- ptable_id:%d ptable_size:%d ",
                   props_->ptable_id, props_->ptable_size);
    FTL_TRACE_INFO("- stable_id:%d stable_size:%d ",
                   props_->stable_id, props_->stable_size);
    FTL_TRACE_INFO("- num_hints:%d max_recircs:%d hash_poly:%d",
                   props_->num_hints, props_->max_recircs, props_->hash_poly);
    FTL_TRACE_INFO("- ptable base_mem_pa:%#lx base_mem_va:%#lx",
                   props_->ptable_base_mem_pa, props_->ptable_base_mem_va);
    FTL_TRACE_INFO("- stable base_mem_pa:%#lx base_mem_va:%#lx",
                   props_->stable_base_mem_pa, props_->stable_base_mem_va);

skip_tables:
    // Initialize CRC Fast
    crc32gen_ = crcFast::factory();
    SDK_ASSERT(crc32gen_);

    return SDK_RET_OK;
}

//---------------------------------------------------------------------------
// ftl Destructor
//---------------------------------------------------------------------------
void
FTL_AFPFX()::destroy(FTL_AFPFX() *t) {
    FTL_API_BEGIN("DestroyTable");
    FTL_TRACE_VERBOSE("%p", t);
    FTL_MAKE_AFTYPE(main_table)::destroy_(static_cast<FTL_MAKE_AFTYPE(main_table)*>(t->main_table_));
    FTL_API_END("DestroyTable", SDK_RET_OK);
    t->main_table_ = NULL;
    t->props_ = NULL;
    t->crc32gen_ = NULL;
}

//---------------------------------------------------------------------------
// ftl Insert entry with hash value
//---------------------------------------------------------------------------
// TODO
sdk_ret_t
FTL_AFPFX()::genhash_(sdk_table_api_params_t *params) {
    static thread_local uint8_t hashkey[64] = {0};
    
    ((FTL_MAKE_AFTYPE(entry_t)*)hashkey)->build_key((FTL_MAKE_AFTYPE(entry_t)*)params->entry);
    sdk::table::FTL_MAKE_AFTYPE(internal)::FTL_MAKE_AFTYPE(swizzle)((FTL_MAKE_AFTYPE(entry_t)*)hashkey);

    if (!params->hash_valid) {
#ifdef SIM
        static thread_local char buff[256];
        ((FTL_MAKE_AFTYPE(entry_t)*)params->entry)->tostr(buff, sizeof(buff));
        FTL_TRACE_VERBOSE("Input Entry = [%s]", buff);
        params->hash_32b = crc32gen_->compute_crc((uint8_t *)&hashkey,
                                                  sizeof(ftlv6_entry_t),
                                                  props_->hash_poly);
#else
        params->hash_32b = crc32_aarch64((uint64_t *)&hashkey);
#endif
        params->hash_valid = true;
    }

    FTL_TRACE_VERBOSE("[%s] => H:%#x", ftlu_rawstr(hashkey, 64),
                      params->hash_32b);
    return SDK_RET_OK;
}

//---------------------------------------------------------------------------
// ftl: Create API context. This is used by all APIs
//---------------------------------------------------------------------------
sdk_ret_t
FTL_AFPFX()::ctxinit_(sdk_table_api_op_t op,
              sdk_table_api_params_t *params) {
    FTL_TRACE_VERBOSE("op:%d", op);
    if (SDK_TABLE_API_OP_IS_CRUD(op)) {
        auto ret = genhash_(params);
        if (ret != SDK_RET_OK) {
            FTL_TRACE_ERR("failed to generate hash, ret:%d", ret);
            return ret;
        }
    }

    apictx_[0].init(op, params, props_, &tstats_, thread_id_);
    return SDK_RET_OK;
}

//---------------------------------------------------------------------------
// ftl Insert entry to ftl table
//---------------------------------------------------------------------------
sdk_ret_t
FTL_AFPFX()::insert(sdk_table_api_params_t *params) {
__label__ done;
    sdk_ret_t ret = SDK_RET_OK;

    FTL_API_BEGIN_();
    SDK_ASSERT(params->entry);

    time_profile_begin(sdk::utils::time_profile::TABLE_LIB_FTL_INSERT);

    ret = ctxinit_(sdk::table::SDK_TABLE_API_INSERT, params);
    FTL_RET_CHECK_AND_GOTO(ret, done, "ctxinit r:%d", ret);

    ret = static_cast<FTL_MAKE_AFTYPE(main_table)*>(main_table_)->insert_(apictx_);
    FTL_RET_CHECK_AND_GOTO(ret, done, "main table insert r:%d", ret);

done:
    time_profile_end(sdk::utils::time_profile::TABLE_LIB_FTL_INSERT);
    api_stats_.insert(ret);
    FTL_API_END_(ret);
    return ret;
}

//---------------------------------------------------------------------------
// ftl Update entry to ftl table
//---------------------------------------------------------------------------
sdk_ret_t
FTL_AFPFX()::update(sdk_table_api_params_t *params) {
    sdk_ret_t ret = SDK_RET_OK;

    FTL_API_BEGIN_();
    SDK_ASSERT(params->key);

    ret = ctxinit_(sdk::table::SDK_TABLE_API_UPDATE, params);
    if (ret != SDK_RET_OK) {
        FTL_TRACE_ERR("failed to create api context. ret:%d", ret);
        goto update_return;
    }

    ret = static_cast<FTL_MAKE_AFTYPE(main_table)*>(main_table_)->update_(apictx_);
    if (ret != SDK_RET_OK) {
        FTL_TRACE_ERR("update_ failed. ret:%d", ret);
        goto update_return;
    }

update_return:
    api_stats_.update(ret);
    FTL_API_END_(ret);
    return ret;
}

//---------------------------------------------------------------------------
// ftl Remove entry from ftl table
//---------------------------------------------------------------------------
sdk_ret_t
FTL_AFPFX()::remove(sdk_table_api_params_t *params) {
    sdk_ret_t ret = SDK_RET_OK;

    FTL_API_BEGIN_();
    SDK_ASSERT(params->key);

    ret = ctxinit_(sdk::table::SDK_TABLE_API_REMOVE, params);
    if (ret != SDK_RET_OK) {
        FTL_TRACE_ERR("failed to create api context. ret:%d", ret);
        goto remove_return;
    }

    ret = static_cast<FTL_MAKE_AFTYPE(main_table)*>(main_table_)->remove_(apictx_);
    if (ret != SDK_RET_OK) {
        FTL_TRACE_ERR("remove_ failed. ret:%d", ret);
        goto remove_return;
    }

remove_return:
    api_stats_.remove(ret);
    FTL_API_END_(ret);
    return ret;
}

//---------------------------------------------------------------------------
// ftl Get entry from ftl table
//---------------------------------------------------------------------------
sdk_ret_t
FTL_AFPFX()::get(sdk_table_api_params_t *params) {
    sdk_ret_t ret = SDK_RET_OK;

    FTL_API_BEGIN_();
    SDK_ASSERT(params->key);

    ret = ctxinit_(sdk::table::SDK_TABLE_API_GET, params);
    if (ret != SDK_RET_OK) {
        FTL_TRACE_ERR("failed to create api context. ret:%d", ret);
        goto get_return;
    }

    ret = static_cast<FTL_MAKE_AFTYPE(main_table)*>(main_table_)->get_(apictx_);
    if (ret != SDK_RET_OK) {
        FTL_TRACE_ERR("remove_ failed. ret:%d", ret);
        goto get_return;
    }

get_return:
    api_stats_.get(ret);
    FTL_API_END_(ret);
    return ret;
}

static inline void
ftl_get_hw_entry_count_iter_cb (sdk_table_api_params_t *params)
{
    FTL_MAKE_AFTYPE(entry_t) *hwentry =
            (FTL_MAKE_AFTYPE(entry_t) *) params->entry;

    if (hwentry->entry_valid) {
        hw_entry_count_++;
    }
}

static inline sdk_ret_t
ftl_get_hw_entry_count (FTL_AFPFX() *table)
{
    sdk_ret_t ret = SDK_RET_OK;
    sdk_table_api_params_t params = {0};

    params.itercb = ftl_get_hw_entry_count_iter_cb;
    ret = table->iterate(&params, TRUE);
    if (ret != SDK_RET_OK) {
        FTL_TRACE_ERR("get_hw_enttry_count iterate failed. ret:%d", ret);
    }
    FTL_TRACE_VERBOSE("ftl_get_hw_entry_count %u\n", hw_entry_count_);

    return ret;
}

//---------------------------------------------------------------------------
// ftl Get Stats from ftl table
// As stats are maintained per thread, needs to call from each thread.
//---------------------------------------------------------------------------
sdk_ret_t
FTL_AFPFX()::stats_get(sdk_table_api_stats_t *api_stats,
                       sdk_table_stats_t *table_stats, bool force_hwread) {
    FTL_API_BEGIN_();
    // reset hw_entry_count_
    hw_entry_count_ = 0;
    api_stats_.get(api_stats);
    if (force_hwread) {
        ftl_get_hw_entry_count(this);
    }
    tstats_.set_entries(hw_entry_count_);
    tstats_.get(table_stats);
    FTL_API_END_(SDK_RET_OK);

    return SDK_RET_OK;
}

static inline void
ftl_dump_hw_entry_iter_cb (sdk_table_api_params_t *params)
{
    FTL_MAKE_AFTYPE(entry_t) *hwentry =
            (FTL_MAKE_AFTYPE(entry_t) *)params->entry;
    FILE *fp = (FILE *)params->cbdata;

    if (hwentry->entry_valid) {
        hw_entry_count_++;
        hwentry->tofile(fp, hw_entry_count_);
    }
}

static inline void
ftl_dump_hw_entry_header (FILE *fp)
{
    fprintf(fp, "%8s\t%16s\t%16s\t%5s\t%5s\t%3s\t%4s\n",
            "Entry", "SrcIP", "DstIP", "SrcPort", "DstPort", "Proto", "Vnic");
    fprintf(fp, "%8s\t%16s\t%16s\t%5s\t%5s\t%3s\t%4s\n",
            "-----", "-----", "-----", "-------", "-------", "-----", "----");
    return;
}

//---------------------------------------------------------------------------
// ftl Dump all HW entries to file
//---------------------------------------------------------------------------
int
FTL_AFPFX()::hwentries_dump(char *fname) {
    sdk_ret_t ret;
    sdk_table_api_params_t params = {0};
    FILE *logfp = fopen(fname, "a");
    int retcode = -1;

    FTL_API_BEGIN("dump_hwentries");
    if (logfp == NULL) {
        FTL_TRACE_ERR("dump_hwentries failed to open %s", fname);
        goto dump_hwentries_return;
    }
    params.itercb = ftl_dump_hw_entry_iter_cb;
    params.cbdata = logfp;
    // reset hw_entry_count_
    hw_entry_count_ = 0;
    ftl_dump_hw_entry_header(logfp);
    ret = iterate(&params, TRUE);
    if (ret != SDK_RET_OK) {
        FTL_TRACE_ERR("dump_hwentries iterate failed. ret:%d", ret);
    } else {
        retcode = hw_entry_count_;
    }
    fclose(logfp);

dump_hwentries_return:
    FTL_API_END("dump_hwentries", retcode);
    return retcode;
}

//---------------------------------------------------------------------------
// ftl Dump Stats from ftl table
// As stats are maintained per thread, needs to call from each thread.
//---------------------------------------------------------------------------
void
FTL_AFPFX()::stats_dump(void) {
    api_stats_.dump();
    tstats_.dump();
}
sdk_ret_t
FTL_AFPFX()::iterate(sdk_table_api_params_t *params, bool force_hwread) {
__label__ done;
    sdk_ret_t ret = SDK_RET_OK;

    FTL_API_BEGIN_();
    SDK_ASSERT(params->itercb);

    ret = ctxinit_(sdk::table::SDK_TABLE_API_ITERATE, params);
    FTL_RET_CHECK_AND_GOTO(ret, done, "ctxinit r:%d", ret);

    ret = static_cast<FTL_MAKE_AFTYPE(main_table)*>(main_table_)->iterate_(apictx_, force_hwread);
    FTL_RET_CHECK_AND_GOTO(ret, done, "iterate r:%d", ret);

done:
    FTL_API_END_(ret);
    return ret;
}

sdk_ret_t
FTL_AFPFX()::clear(bool clear_global_state,
           bool clear_thread_local_state) {
__label__ done;
    sdk_ret_t ret = SDK_RET_OK;
    FTL_API_BEGIN_();

    ret = ctxinit_(sdk::table::SDK_TABLE_API_CLEAR, NULL);
    FTL_RET_CHECK_AND_GOTO(ret, done, "ctxinit r:%d", ret);

    apictx_[0].clear_global_state = clear_global_state;
    apictx_[0].clear_thread_local_state = clear_thread_local_state;
    ret = static_cast<FTL_MAKE_AFTYPE(main_table)*>(main_table_)->clear_(apictx_);
    FTL_RET_CHECK_AND_GOTO(ret, done, "clear r:%d", ret);
    
    if (clear_thread_local_state) {
        api_stats_.clear();
        tstats_.clear();
    }

done:
    FTL_API_END_(ret);
    return ret;
}
