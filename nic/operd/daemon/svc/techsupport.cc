//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//------------------------------------------------------------------------------

#include <cstdlib>
#include <string>
#include <time.h>
#include "gen/proto/techsupport.grpc.pb.h"
#include "gen/proto/types.pb.h"
#include "techsupport.hpp"

static inline std::string
get_techsupport_filename (void)
{
    char timestring[PATH_MAX];
    char filename[PATH_MAX];
    time_t current_time = time(NULL);

    strftime(timestring, PATH_MAX, "%Y%m%d%H%M%S", gmtime(&current_time));
    snprintf(filename, PATH_MAX, "dsc-tech-support-%s.tar.gz", timestring);

    return std::string(filename);
}

static inline std::string
get_techsupport_binary (void)
{
    const char* nic_dir;
    std::string ts_bin_path;
    std::string ts_bin = "/bin/techsupport";

    nic_dir = std::getenv("PDSPKG_TOPDIR");
    if (nic_dir == NULL) {
        ts_bin_path = std::string("/nic/");
    } else {
        ts_bin_path.assign(nic_dir);
    }
    ts_bin_path += ts_bin;

    return ts_bin_path;
}

static inline std::string
get_techsupport_config (void)
{
    const char* nic_conf_dir;
    std::string ts_cfg_path;
    std::string ts_cfg = "/techsupport.json";

    nic_conf_dir = std::getenv("CONFIG_PATH");
    if (nic_conf_dir == NULL) {
        ts_cfg_path = std::string("/nic/conf/");
    } else {
        ts_cfg_path.assign(nic_conf_dir);
    }
    ts_cfg_path += ts_cfg;

    return ts_cfg_path;
}

static inline std::string
get_techsupport_cmd (std::string ts_dir, std::string ts_file, bool skipcores)
{
    char ts_cmd[PATH_MAX];
    std::string bin_path, ts_bin_path;
    std::string ts_bin, ts_task;

    ts_bin = get_techsupport_binary();
    ts_task = get_techsupport_config();

    snprintf(ts_cmd, PATH_MAX, "%s -c %s -d %s -o %s %s", ts_bin.c_str(),
             ts_task.c_str(), ts_dir.c_str(), ts_file.c_str(),
             skipcores ? "-s" : "");

    return std::string(ts_cmd);
}

static inline int
get_exit_status (int rc)
{
    int ret = rc;

    if (WIFEXITED(rc)) {
        ret = WEXITSTATUS(rc);
    }
    return ret;
}

Status
TechSupportSvcImpl::TechSupportCollect(ServerContext *context,
                                       const TechSupportRequest *req,
                                       TechSupportResponse *rsp) {
    int rc;
    std::string tsdir = "/data/techsupport/";
    auto tsrsp = rsp->mutable_response();
    auto tsfile = get_techsupport_filename();
    auto tscmd = get_techsupport_cmd(tsdir, tsfile, req->request().skipcores());

    rc = get_exit_status(system(tscmd.c_str()));
    fprintf(stdout, "Techsupport request %s, rc %d\n", tsfile.c_str(), rc);
    if (rc != 0) {
        rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
    } else {
        tsrsp->set_filepath(tsdir + tsfile);
        rsp->set_apistatus(types::ApiStatus::API_STATUS_OK);
    }
    return Status::OK;
}
