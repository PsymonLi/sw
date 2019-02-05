// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

#include <algorithm>
#include <chrono>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <unistd.h>

#include "nic/utils/penlog/lib/penlog.hpp"
#include "nic/utils/penlog/lib/null_logger.hpp"

#include "delphi_messages.hpp"
#include "pipe_t.hpp"
#include "scheduler.hpp"
#include "sysmgr_service.hpp"

using namespace std;

#define MAX_EVENTS 10

struct died_pid_t {
    pid_t pid;
    int   status;
};

penlog::LoggerPtr logger = make_shared<penlog::NullLogger>();

static auto died_pids = make_shared<Pipe<died_pid_t>>();
static auto started_pids = make_shared<Pipe<pid_t>>();
static auto delphi_messages = make_shared<Pipe<int32_t>>();
static auto heartbeats = make_shared<Pipe<pid_t>>();
static auto quit_pipe = make_shared<Pipe<int>>();
static auto reload_pipe = make_shared<Pipe<int>>();

const pid_t mypid = getpid();
string log_location = "/var/log/sysmgr";

void exists_or_mkdir(const char *dir) {
    struct stat sb;
    if (stat(dir, &sb) == 0) {
        if (!S_ISDIR(sb.st_mode)) {
            logger->critical("%s is not a directory");
            exit(-1);
        }
        return;
    }
    
    mkdir(dir, S_IRWXU);
    logger->debug("Creating directory {}", dir);
}

void mkdirs(const char *dir) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;
    struct stat sb;

    // if file exists bail out
    if (stat(dir, &sb) == 0) {
        if (!S_ISDIR(sb.st_mode)) {
            logger->error("%s is not a directory");
            exit(-1);
        }
        return;
    }
    
    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);

    if(tmp[len - 1] == '/')
    {
        tmp[len - 1] = 0;
    }
                
    for(p = tmp + 1; *p; p++)
    {
        if(*p == '/')
        {
            *p = 0;
            exists_or_mkdir(tmp);
            *p = '/';
        }
    }
    exists_or_mkdir(tmp);
}

void redirect(const string &filename, int fd)
{
    int file_fd = open(filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
   
    close(fd);

    dup2(file_fd, fd);

    close(file_fd);
}

// Redirect stdout and stderr
void redirect_stds(const string &name, pid_t pid)
{
    redirect(log_location + "/" + name + "." + to_string(pid) + ".out" + ".log", 1);
    redirect(log_location + "/" + name + "." + to_string(pid) + ".err" + ".log", 2);
}

void exec_command(const string &command)
{
    // split the command to an array of string(tokens)
    istringstream iss(command);
    vector<string> tokens;
    copy(istream_iterator<string>(iss),
         istream_iterator<string>(),
         back_inserter(tokens));

    // convert them to c_strings
    vector<const char *> c_strings;
    for (auto &&s : tokens)
    {
        c_strings.push_back(s.c_str());
    }
    c_strings.push_back(NULL);

    execvp(c_strings[0], (char *const *)&c_strings[0]);
    fprintf(stderr, "exec(%s) failed: %s", c_strings[0], strerror(errno));
    exit(-1); // the only way to get here is if exec failed, exit
}

pid_t launch(const string &name, const string &command)
{
    pid_t pid = fork();

    if (pid == -1)
    {
        logger->error("Fork failed: {}", strerror(errno));
        exit(1);
    }
    else if (pid == 0)
    {
        redirect_stds(name, getpid());
        exec_command(command);
    }

    logger->info("Fork success. Child pid: {}", pid);

    return pid;
}

void switch_root()
{
    kill(-mypid, SIGTERM);
    execlp("/nic/tools/switch_rootfs.sh", "altfw", NULL);
    fprintf(stderr, "Switch root failed");
    exit(-1);
}

void sigchld_handler(int sig)
{
    pid_t pid;
    int status;

    while ((pid = waitpid((pid_t)(-1), &status, WNOHANG)) > 0)
    {
        struct died_pid_t dp = { .pid = pid, .status = status };
        died_pids->pipe_write(&dp);
    }
}

void sig_handler(int sig)
{
    quit_pipe->pipe_write(&sig);
}

void install_sigchld_handler()
{
    struct sigaction sa;
    sa.sa_handler = &sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1)
    {
        logger->error("sigaction(): {}", strerror(errno));
        exit(1);
    }
}

void wait_for_events(Scheduler &scheduler, int epollfd, int sleep)
{
    struct epoll_event events[MAX_EVENTS];

    int fds = epoll_wait(epollfd, events, MAX_EVENTS, sleep * 1000);
    if (fds == -1) {
        logger->error("epoll_wait() error: {}", strerror(errno));
    }
    logger->debug("Number of events: {}", fds);
    for (int i = 0; i < fds; i++)
    {
	if (events[i].data.fd == reload_pipe->raw_fd())
	{
	    logger->info("Reloading...");
	    switch_root();
	}
        else if (events[i].data.fd == died_pids->raw_fd())
        {
            logger->info("died_pids event");
            died_pid_t dp;
            while (died_pids->pipe_read(&dp) == 0)
            {
                logger->info("died_pids: pipe_read(): {}", dp.pid);
                scheduler.service_died(dp.pid, dp.status);
            }
        }
        else if (events[i].data.fd == started_pids->raw_fd())
        {
            logger->info("started_pids event");
            pid_t pid;
            while (started_pids->pipe_read(&pid) == 0)
            {
                logger->info("started_pids: pipe_read(): {}", pid);
                scheduler.service_started(pid);
            }
        }
        else if (events[i].data.fd == delphi_messages->raw_fd())
        {
            logger->info("delphi_messages event");
            int32_t msg;
            while (delphi_messages->pipe_read(&msg) == 0)
            {
                logger->info("delphi_messages: pipe_read(): {}", msg);
                if (msg == DELPHI_UP)
                {
                    scheduler.service_started("delphi");
                }
                else
                {
                    logger->info("UKNOWN delphi_messages message");
                }
            }
        }
        else if (events[i].data.fd == heartbeats->raw_fd())
        {
            logger->debug("heartbeat event");
            pid_t pid;
            while(heartbeats->pipe_read(&pid) == 0)
            {
                logger->debug("heartbeat: pipe_read(): {}", pid);
                scheduler.heartbeat(pid);
            }
        }
        else if (events[i].data.fd == quit_pipe->raw_fd())
        {
            int sig;
            while(quit_pipe->pipe_read(&sig) == 0)
            {
                logger->info("Signal {} received. Exiting", sig);
                kill(-mypid, SIGTERM);
                exit(-1);
            }
        }
        else
        {
            logger->info("UNKOWN EVENT");
        }
    }
}

void epollfd_register(int epollfd, int fd)
{
    struct epoll_event ev = {0};
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        logger->error("epoll_ctl: {}", strerror(errno));
        exit(1);
    }
}

void loop(Scheduler &scheduler, shared_ptr<SysmgrService> delphi_svc)
{
    int epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        logger->error("epoll_create1: {}", strerror(errno));
        exit(1);
    }

    epollfd_register(epollfd, died_pids->raw_fd());
    epollfd_register(epollfd, started_pids->raw_fd());
    epollfd_register(epollfd, delphi_messages->raw_fd());
    epollfd_register(epollfd, heartbeats->raw_fd());
    epollfd_register(epollfd, quit_pipe->raw_fd());
    epollfd_register(epollfd, reload_pipe->raw_fd());

    for (;;)
    {
        auto action = scheduler.next_action();
        logger->debug("Next action: {}", action->type);
        if (action->type == LAUNCH)
        {
            logger->info("Launching services");
            for (auto service : action->launch_list)
            {
                logger->info("Launching {}: {}", service->name, service->command);
                pid_t pid = launch(service->name, service->command);
                scheduler.service_launched(service, pid);
            }
        }
        else if (action->type == WAIT)
        {
            logger->debug("Waiting for events");
            wait_for_events(scheduler, epollfd, action->sleep);
        }
        else if (action->type == REBOOT)
        {
            logger->info("Fault");
            delphi_svc->set_system_fault();
            wait_for_events(scheduler, epollfd, -1);
        }
    }
}

void *delphi_thread_run(void *ctx)
{
    delphi::SdkPtr sdk = *reinterpret_cast<delphi::SdkPtr*>(ctx);

    logger->info("Starting Delphi MainLoop()");
    sdk->MainLoop();
    logger->info("Delphi main loop exited");

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
       fprintf(stderr, "Please use %s <CONFIG_FILE>\n`", argv[0]);
       return -1;
    }
    if (argc == 3) {
        log_location = argv[2];
    }
    
    mkdirs(log_location.c_str());
   
    int rc = setpgid(0, 0);
    if (rc == -1) 
    {
        perror("setpgid");
        exit(-1);
    }
    redirect_stds("sysmgr", getpid());

    // The delphi client
    delphi::SdkPtr sdk(make_shared<delphi::Sdk>());
    shared_ptr<SysmgrService> svc = make_shared<SysmgrService>(sdk,
        "SystemManager", started_pids, delphi_messages, heartbeats,
	reload_pipe);
    sdk->RegisterService(svc);
    logger = penlog::logger_init(sdk, "sysmgr");

    pthread_t delphi_thread;
    pthread_create(&delphi_thread, NULL, delphi_thread_run,
        reinterpret_cast<void *>(&sdk));

    // The scheduler
    auto spec = specs_from_json(argv[1]);
    Scheduler scheduler(spec, logger);
    scheduler.set_service_status_watcher(svc);
    
    install_sigchld_handler();
    signal(SIGQUIT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGINT, sig_handler);

    loop(scheduler, svc);

    return 0;
}
