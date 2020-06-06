#include <memory>
#include "gen/proto/events.pb.h"
#include "gen/proto/eventtypes.pb.h"
#include "gen/proto/attributes.pb.h"
#include "sysmond_eventrecorder.hpp"
#include "nic/utils/events/recorder/recorder.hpp"

EventLoggerPtr EventLogger::instance = nullptr;

EventLoggerPtr EventLogger::getInstance()
{
    if (instance == nullptr)
    {
        instance = std::make_shared<EventLogger>();

        instance->recorder = events_recorder::init(
            "sysmond", GetLogger());
    }
    return instance;
}

void EventLogger::LogCriticalTempEvent(const char *description)
{
    this->recorder->event(eventtypes::NAPLES_CATTRIP_INTERRUPT, description);
}

void EventLogger::LogOverTempAlarmEvent(const char *description, int temperature)
{
    TRACE_ERR(GetLogger(), "LogOverTempAlarmEvent::Over temp alarm.");
    this->recorder->event(eventtypes::NAPLES_OVER_TEMP, description, temperature);
}

void EventLogger::LogOverTempExitAlarmEvent(const char *description, int temperature)
{
    this->recorder->event(eventtypes::NAPLES_OVER_TEMP_EXIT, description, temperature);
}

void EventLogger::LogFatalInterruptEvent(const char *desc) {
    this->recorder->event(eventtypes::NAPLES_FATAL_INTERRUPT, desc);
}

void EventLogger::LogPanicEvent(const char *description)
{
    this->recorder->event(eventtypes::NAPLES_PANIC_EVENT, description);
}

void EventLogger::LogPostdiagEvent(const char *description)
{
    this->recorder->event(eventtypes::NAPLES_POST_DIAG_FAILURE_EVENT, description);
}

void EventLogger::LogInfoPcieHealthEvent(const char *description, const char *desc) {
    this->recorder->event(eventtypes::NAPLES_INFO_PCIEHEALTH_EVENT,
                          description, desc);
}

void EventLogger::LogWarnPcieHealthEvent(const char *description, const char *desc) {
    this->recorder->event(eventtypes::NAPLES_WARN_PCIEHEALTH_EVENT,
                          description, desc);
}

void EventLogger::LogErrorPcieHealthEvent(const char *description, const char *desc) {
    this->recorder->event(eventtypes::NAPLES_ERR_PCIEHEALTH_EVENT,
                          description, desc);
}
