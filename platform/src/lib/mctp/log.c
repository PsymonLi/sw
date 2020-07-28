#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>

#include "libmctp.h"
#include "libmctp-log.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

static int log_level = MCTP_LOG_INFO;

void
set_log_level(int log_level)
{
    log_level = log_level;
}

int
get_log_level(void)
{
    return log_level;
}

void mctp_log(int level, const char *pname, const char *fmt, ...)
{
    va_list ap;
    struct timeval tv;
    struct tm tmtime;
    char buf[20];
    char lvl;

    switch (level) {
    case MCTP_LOG_ERR:
        lvl = 'E';
        break;
    case MCTP_LOG_WARNING:
        lvl = 'W';
        break;
    case MCTP_LOG_NOTICE:
        lvl = 'N';
        break;
    case MCTP_LOG_INFO:
        lvl = 'I';
        break;
    case MCTP_LOG_DEBUG:
        lvl = 'D';
        break;
    default:
        lvl = 'I';
    }

    va_start(ap, fmt);
    if (level <= log_level) {
        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &tmtime);
        strftime(buf, 20, "%Y-%m-%d %H:%M:%S", &tmtime);

        fprintf(stdout, "%c [%s.%06u] %u/%s: ", lvl, buf,
                (unsigned) (tv.tv_usec / 1000), getpid(), pname);

        vfprintf(stdout, fmt, ap);
        fputs("\n", stdout);
    }
    va_end(ap);
}

void mctp_prlog(int level, const char *fmt, ...)
{
    va_list ap;
    struct timeval tv;
    struct tm tmtime;
    char buf[20];
    char lvl;

    switch (level) {
    case MCTP_LOG_ERR:
        lvl = 'E';
        break;
    case MCTP_LOG_WARNING:
        lvl = 'W';
        break;
    case MCTP_LOG_NOTICE:
        lvl = 'N';
        break;
    case MCTP_LOG_INFO:
        lvl = 'I';
        break;
    case MCTP_LOG_DEBUG:
        lvl = 'D';
        break;
    default:
        lvl = 'I';
    }

    va_start(ap, fmt);
    if (level <= log_level) {
        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &tmtime);
        strftime(buf, 20, "%Y-%m-%d %H:%M:%S", &tmtime);

        fprintf(stdout, "%c [%s.%06u] %u/libmctp: ", lvl, buf,
                (unsigned) (tv.tv_usec / 1000), getppid());

        vfprintf(stdout, fmt, ap);
        fputs("\n", stdout);
    }
    va_end(ap);
}
