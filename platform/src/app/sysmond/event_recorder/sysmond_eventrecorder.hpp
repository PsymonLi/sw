#ifndef __SYSYMOND_EVENTRECORDER_HPP__
#define __SYSYMOND_EVENTRECORDER_HPP__

#include <memory>
#include "gen/proto/events.pb.h"
#include "gen/proto/eventtypes.pb.h"
#include "platform/src/app/sysmond/logger.h"
#include "nic/utils/events/recorder/recorder.hpp"

class EventLogger {
    static std::shared_ptr<EventLogger> instance;
    events_recorder* recorder;
public:
    static std::shared_ptr<EventLogger> getInstance();
    void LogCriticalTempEvent(const char *description);
    void LogOverTempAlarmEvent(const char *description, int temperature);
    void LogOverTempExitAlarmEvent(const char *description, int temperature);
    void LogFatalInterruptEvent(const char *desc);
    void LogPanicEvent(const char *description);
    void LogPostdiagEvent(const char *description);
    void LogInfoPcieHealthEvent(const char *description, const char *desc);
    void LogWarnPcieHealthEvent(const char *description, const char *desc);
    void LogErrorPcieHealthEvent(const char *description, const char *desc);
};
typedef std::shared_ptr<EventLogger> EventLoggerPtr;

#endif // __SYSYMOND_EVENTRECORDER_HPP__
