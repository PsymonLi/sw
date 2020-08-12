// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <time.h>

#include "nic/sdk/platform/evutils/include/evutils.h"
//#include "nic/sdk/platform/ncsi/ncsi_mgr.h"
#include "nic/sdk/platform/ncsi/cmd_hndlr.h"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/sdk/platform/pal/include/pal.h"
#include "nic/sdk/lib/logger/logger.hpp"
#include "nic/sdk/lib/runenv/runenv.h"
#include "nic/sdk/lib/utils/path_utils.hpp"
#include "grpc_ipc.h"
#include "nic/include/hal_cfg.hpp"
#include "nic/utils/device/device.hpp"

#define ARRAY_LEN(var)   (int)((sizeof(var)/sizeof(var[0])))

#define NCSI_TRACE_ERR(fmt...)             \
    if (GetCurrentLogger()) {              \
        GetCurrentLogger()->error(fmt);    \
    }

#define NCSI_TRACE_INFO(fmt...)            \
    if (GetCurrentLogger()) {              \
        GetCurrentLogger()->info(fmt);     \
    }

typedef std::shared_ptr<spdlog::logger> Logger;
static Logger current_logger;
static EV_P;

using namespace std;
using namespace sdk::platform::ncsi;
using namespace sdk::lib;
using namespace hal::utils;

int usage(int argc, char* argv[]);
void ipc_init();

Logger CreateLogger(const char* log_name) {

    char log_path[64] = {0};
    snprintf(log_path, sizeof(log_path), "/obfl/%s", log_name);

    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>
        (log_path, 1*1024*1024, 3);

    Logger _logger = std::make_shared<spdlog::logger>(log_name, rotating_sink);

    _logger->set_pattern("[%Y-%m-%d_%H:%M:%S.%e] %L %v");
    _logger->flush_on(spdlog::level::info);

#ifdef DEBUG_ENABLE
    spdlog::set_level(spdlog::level::debug);
#endif

    return _logger;
}

void SetCurrentLogger(Logger logger) {
    current_logger = logger;

    return;
}

Logger GetCurrentLogger() {
    return current_logger;
}

int ncsi_logger (uint32_t mod_id, trace_level_e trace_level,
                 const char *format, ...)
{
    char       logbuf[1024];
    va_list    args;

    va_start(args, format);
    vsnprintf(logbuf, sizeof(logbuf), format, args);
    va_end(args);

    switch (trace_level) {
    case sdk::types::trace_err:
        NCSI_TRACE_ERR("{}", logbuf);
        break;
    case sdk::types::trace_info:
        NCSI_TRACE_INFO("{}", logbuf);
        break;
    default:
        break;
    }
    return 0;
}
shared_ptr<grpc_ipc> grpc_ipc_svc;

//NcsiMgr *ncsimgr;
std::vector<CmdHndler *> cmd_hndlr_objs;
std::vector<std::string> transport_modes = {"MCTP:i2c", "RBT:oob_mnic0"};
transport * xport_obj;

bool is_interface_online(const char* interface) {
    struct ifreq ifr;
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    char buf[64] = {0};

    if (sock == -1) {
        perror("SOCKET OPEN");
        return false;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, interface);

    ifr.ifr_flags = (IFF_UP | IFF_BROADCAST | IFF_RUNNING | IFF_MULTICAST);

    if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
        snprintf(buf, sizeof(buf), "%s: SIOCSIFFLAGS", interface);
        perror(buf);
        close(sock);
        return false;
    }
    
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, interface);
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        snprintf(buf, sizeof(buf), "%s: SIOCSIFFLAGS", interface);
        perror(buf);
        close(sock);
        return false;
    }

    close(sock);
    return !!(ifr.ifr_flags & IFF_UP);
}

void InitNcsiMgr(EV_P)
{
    string xport_mode;
    string iface_name;
    CmdHndler * cmdhndlr;
    hal::utils::device *device =
        hal::utils::device::factory(sdk::lib::get_device_conf_path());
    string dev_feature_profile =
        dev_feature_profile_to_string(device->get_feature_profile());

    for (size_t xport=0; xport < transport_modes.size(); xport++)
    {
        stringstream ss(transport_modes[xport]);

        std::getline(ss, xport_mode, ':');
        printf("transport_modes.size() = %ld, xport_mode: %s\n", transport_modes.size(), xport_mode.c_str());

        if (!(xport_mode.compare("RBT")))
        {
            std::getline(ss, iface_name);

            if (!iface_name.compare("oob_mnic0")) {

                feature_query_ret_e feature_query_ret = runenv::is_feature_enabled(NCSI_FEATURE);
                if (feature_query_ret != FEATURE_ENABLED) {
                    SDK_TRACE_INFO("NCSI feature is not enabled on %s interface. Feature query response: %d", iface_name.c_str(), feature_query_ret);
                    //SDK_TRACE_INFO("Exiting ncsid app !");
                    //return 0;
                    continue;
                }
            }

            SDK_TRACE_INFO("Initializing ncsi transport in %s mode over %s interface", 
                    xport_mode.c_str(), iface_name.c_str());
            xport_obj = new rbt_transport(iface_name.c_str());

            while(!is_interface_online(iface_name.c_str())) {
                //SDK_TRACE_INFO("Interface is not online, waiting...\n");
                usleep(10000);
            }
            cmdhndlr = CmdHndler::factory(grpc_ipc_svc, xport_obj,
                                          dev_feature_profile, EV_A);
            cmd_hndlr_objs.push_back(cmdhndlr);
        }
        else if (!xport_mode.compare("MCTP"))
        {
            SDK_TRACE_INFO("Initializing ncsi transport in %s mode", 
                    xport_mode.c_str());
            xport_obj = new mctp_transport();
            cmdhndlr = CmdHndler::factory(grpc_ipc_svc, xport_obj,
                                          dev_feature_profile, EV_A);
            cmd_hndlr_objs.push_back(cmdhndlr );
        }
        else
        {
            SDK_TRACE_INFO("Invalid transport mode: %s", transport_modes[xport].c_str());
            return;
        }

    }// for

    SDK_TRACE_INFO("NCSI interface is UP !"); 

    return;
}

static void
ncsid_event_thread_start(void *ctx)
{
    SDK_TRACE_INFO("Inside thread func. calling InitNcsiMgr() now");
    //sdk::event_thread::event_thread *curr_thread = (sdk::event_thread::event_thread *)ctx;
    
    InitNcsiMgr(EV_A);
}

static void
ncsid_event_thread_exit(void *ctx)
{
        SDK_TRACE_INFO("ncsid event thread exiting ..");
}

sdk::event_thread::event_thread *ncsid_event_thread;

int init_ncsid_event_thread()
{

    ncsid_event_thread = sdk::event_thread::event_thread::factory(
                "ncsid_ev", hal::HAL_THREAD_ID_NCSID /*thread_id*/,
                sdk::lib::THREAD_ROLE_CONTROL,
                0x0,    // use all control cores
                ncsid_event_thread_start,  // entry function
                ncsid_event_thread_exit,  // exit function
                NULL,  // thread event callback
                sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
                sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
                THREAD_FLAGS_NONE);
    
    SDK_TRACE_INFO("ncsid_ev thread created");
    SDK_ASSERT_TRACE_RETURN((ncsid_event_thread != NULL), -1,
            "Failed to spawn ncsid event thread");
    EV_A = ncsid_event_thread->ev_loop();
    ncsid_event_thread->start(ncsid_event_thread);
    
    return 0;
}

void ncsid_wait()
{
	int         rv;

	rv = pthread_join(ncsid_event_thread->pthread_id(), NULL);
	if (rv != 0) {
		SDK_TRACE_ERR("pthread_join failure, thread %s, err : %d",
				ncsid_event_thread->name(), rv);
	}

	return;
}

int main(int argc, char* argv[])
{
    //ncsimgr = new NcsiMgr();
    grpc_ipc_svc = make_shared<grpc_ipc>();
    Logger logger_obj;
    int ret;

    //Create the logger
    logger_obj = CreateLogger("ncsi.log");
    SetCurrentLogger(logger_obj);

    sdk::lib::logger::init(ncsi_logger);

    SDK_TRACE_INFO("======== starting ncsid app =========");
    if (argc > 1) {
        stringstream user_transport(argv[1]);
        string item;

        // if user has provided transport, use that instead of static one
        transport_modes.clear();

        while(getline(user_transport, item, ','))
        {
            SDK_TRACE_INFO("user provided trasport: %s", item.c_str());
            transport_modes.push_back(item);
        }
    }

    grpc_ipc_svc->connect_hal();
    ret = init_ncsid_event_thread();
    if (ret)
    {
        SDK_TRACE_ERR("ncsid thread create failed, exiting...");
        return ret;
    }

	ncsid_wait();

    //evutil_run(EV_A);

    //Should never reach here
    SDK_TRACE_INFO("should never reach here. Exiting ncsid app !");

    return 0;
}

int usage(int argc, char* argv[])
{
    printf("Usage: %s [transport mode]\n",argv[0]);
    printf("Possible transport modes are:\n");
    printf("   %-16s\n","RBT (default)");
    printf("   %-16s\n","MCTP");

    return 1;
}

