#include <memory>

#include <spdlog/spdlog.h>

#include "../events_api.hpp"
#include "nic/infra/operd/event/event.hpp"

class DummyLogger : public SysmgrEvents {
public:
    void SystemBooted(void) override;
    void ServiceStartedEvent(std::string name) override;
    void ServiceStoppedEvent(std::string name) override;
    DummyLogger();
private:
    operd::event::event_recorder_ptr g_event;

};
typedef std::shared_ptr<DummyLogger> DummyLoggerPtr;

SysmgrEventsPtr
init_events (std::shared_ptr<spdlog::logger> logger)
{
    return std::make_shared<DummyLogger>();
}

DummyLogger::DummyLogger() {
    g_event = operd::event::event_recorder::get();
}

void
DummyLogger::SystemBooted(void) {
    printf("ooops\n");
    fflush(stdout);
    g_event->event(operd::event::SYSTEM_COLDBOOT, "System booted");
}

void
DummyLogger::ServiceStartedEvent(std::string svc) {
    printf("ooops\n");
    fflush(stdout);
    g_event->event(operd::event::DSC_SERVICE_STARTED, "Service: %s",
                   svc.c_str());
}

void
DummyLogger::ServiceStoppedEvent(std::string svc) {
    g_event->event(operd::event::DSC_SERVICE_STOPPED, "Service: %s",
                   svc.c_str());
}
