#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "nic/sdk/platform/pal/include/pal.h"
#include "watchdog.hpp"
#include "fault_watcher.hpp"
#include "log.hpp"
#include "timer_watcher.hpp"
#include "nic/sysmgr/penvisor/penvisor.h"

class PALWatchdog : public AbstractWatchdog {
public:
    static std::shared_ptr<PALWatchdog> create();
    PALWatchdog();
    virtual void kick();
};
typedef std::shared_ptr<PALWatchdog> PALWatchdogPtr;

class PenvisorWatchdog : public AbstractWatchdog {
public:
    static std::shared_ptr<PenvisorWatchdog> create();
    PenvisorWatchdog();
    virtual void kick();

private:
    int fd_;
    struct sockaddr_un addr_;
    penvisor_command_t command_;
};
typedef std::shared_ptr<PenvisorWatchdog> PenvisorWatchdogPtr;

class SimulationWatchdog : public AbstractWatchdog,
                           public TimerReactor
{
private:
    TimerWatcherPtr timer_watcher;
public:
    static std::shared_ptr<SimulationWatchdog> create();
    virtual void kick();
    virtual void on_timer();
};
typedef std::shared_ptr<SimulationWatchdog> SimulationWatchdogPtr;

PALWatchdogPtr PALWatchdog::create()
{
    PALWatchdogPtr watchdog = std::make_shared<PALWatchdog>();
    return watchdog;
}

PALWatchdog::PALWatchdog()
{
    int rc = pal_watchdog_init(PANIC_WDT);
    g_log->info("pal_watchdog_init() rc = %i", rc);
}

void PALWatchdog::kick()
{
    pal_watchdog_kick();
}

PenvisorWatchdogPtr
PenvisorWatchdog::create() {
    PenvisorWatchdogPtr watchdog = std::make_shared<PenvisorWatchdog>();
    return watchdog;
}

PenvisorWatchdog::PenvisorWatchdog() {
    this->fd_ = socket(AF_UNIX, SOCK_DGRAM, 0);

    memset(&this->addr_, 0, sizeof(this->addr_));
    this->addr_.sun_family = AF_UNIX;
    strncpy(this->addr_.sun_path, PENVISOR_PATH, sizeof(this->addr_.sun_path));
    this->addr_.sun_path[sizeof(this->addr_.sun_path) - 1] = '\0';

    memset(&this->command_, 0, sizeof(this->command_));
    this->command_.action = PENVISOR_HEARTBEAT;
}

void
PenvisorWatchdog::kick() {
    int rc;
    
    rc = sendto(this->fd_, &this->command_, sizeof(this->command_), 0,
                (struct sockaddr *)&this->addr_, sizeof(this->addr_));
    if (rc == -1) {
        g_log->err("sendto failed: %s(%d)", strerror(errno), errno);
    }
}

SimulationWatchdogPtr SimulationWatchdog::create()
{
    SimulationWatchdogPtr watchdog = std::make_shared<SimulationWatchdog>();
    watchdog->timer_watcher = TimerWatcher::create(60.0, 60.0, watchdog);

    return watchdog;
}

void SimulationWatchdog::kick()
{
    this->timer_watcher->repeat();
}

void SimulationWatchdog::on_timer()
{
    ev::default_loop loop;

    g_log->err("Simulation Watchdog Expired");
    // Kill all children
    kill(-getpid(), SIGTERM);
    loop.break_loop(ev::ALL);
}

WatchdogPtr Watchdog::create()
{
    WatchdogPtr wchdg = std::make_shared<Watchdog>();

    wchdg->kick_it = true;
    
    wchdg->timer_watcher = TimerWatcher::create(1.0, 1.0, wchdg);
    FaultLoop::getInstance()->register_fault_reactor(wchdg);
    SwitchrootLoop::getInstance()->register_switchroot_reactor(wchdg);

    g_log->debug("Creating watchdog");

    if ((access("/.instance_a", F_OK) != -1) ||
        (access("/.instance_b", F_OK) != -1)) {
        wchdg->watchdog = PenvisorWatchdog::create();
        g_log->info("Using Penvisor watchdog");
    } else if ((access("/data/no_watchdog", F_OK) != -1) ||
               (std::getenv("NO_WATCHDOG"))) {
        wchdg->watchdog = SimulationWatchdog::create();
        g_log->info("Using Simulation watchdog");
    } else {
        wchdg->watchdog = PALWatchdog::create();
        g_log->info("Using PAL watchdog");
    }
    
    wchdg->timer_watcher->repeat();
    
    return wchdg;
}

void Watchdog::on_fault(std::string reason)
{
    this->kick_it = false;
}

void Watchdog::on_timer()
{
    if (this->kick_it == false)
    {
        g_log->trace("Not kicking watchdog");
        return;
    }
    g_log->trace("Kicking watchdog");
    this->watchdog->kick();
}

void Watchdog::on_switchroot()
{
    g_log->debug("Watchdog on_switchroot()");
    this->stop();
}

void Watchdog::stop()
{
    g_log->debug("Watchdog stop()");
    this->timer_watcher->stop();
    pal_watchdog_stop();
}
