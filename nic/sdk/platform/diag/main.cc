// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#include <time.h>
#include <string.h>

#include "lib/pal/pal.hpp"
#include "platform/diag/rtc_test.h"
#include "platform/diag/cpld_test.h"
#include "platform/diag/temp_sensor_test.h"
#include "platform/diag/diag_utils.h"
#include "lib/logger/logger.hpp"
#include "include/sdk/timestamp.hpp"

#define ARRAY_LEN(var)   (int)((sizeof(var)/sizeof(var[0])))

using namespace sdk::platform::diag;

FILE *g_log_fp;
sdk_trace_level_e g_trace_level;

struct test_info
{
    const char *test_name;
    const char *test_banner;
    diag_ret_e (*func)(test_mode_e mode, int argc, char *argv[]);
    const char *desc;
};

struct test_info tests_list[] = 
{
    {"cpld", "CPLD Test", cpld_test, "Run the CPLD Test"},
    {"temp-sensor", "Temperature Sensor Test", temp_sensor_test, "Run the Temperature Sensor Test"},
    {"rtc", "RTC Test", rtc_test, "Run the RTC Test"},
};

static int
usage (int argc, char* argv[])
{
    int i;

    printf("Usage: %s <mode> [test] [test args]\n",argv[0]);
    printf("Possible modes are:\n");
    printf("   %-16s\n","offline");
    printf("   %-16s\n","online");
    printf("   %-16s\n","post");

    printf("Available tests are: \n");
    for( i = 0; i < ARRAY_LEN(tests_list); i++ )
        if( tests_list[i].desc )
            printf("   %-16s %s\n",tests_list[i].test_name,tests_list[i].desc);

    printf("\nRun '%s <mode> <test_name> -help' for more information on a specific test.\n", argv[0]);
    return 1;
}

static int
diag_logger (uint32_t mod_id, sdk_trace_level_e trace_level,
             const char *format, ...)
{
    char       logbuf[1024];
    va_list    args;
    timespec_t ts;

    assert(g_log_fp);
    if (trace_level > g_trace_level) {
        return 0;
    }

    clock_gettime(CLOCK_REALTIME, &ts);
    sdk::timestamp_str(logbuf, sizeof(logbuf), &ts);
    fprintf(g_log_fp, "%s", logbuf);

    switch (trace_level) {
    case sdk::lib::SDK_TRACE_LEVEL_ERR:
        fprintf(g_log_fp, " E ");
        break;
    case sdk::lib::SDK_TRACE_LEVEL_WARN:
        fprintf(g_log_fp, " W ");
        break;
    case sdk::lib::SDK_TRACE_LEVEL_INFO:
        fprintf(g_log_fp, " I ");
        break;
    case sdk::lib::SDK_TRACE_LEVEL_DEBUG:
        fprintf(g_log_fp, " D ");
        break;
    case sdk::lib::SDK_TRACE_LEVEL_VERBOSE:
        fprintf(g_log_fp, " V ");
        break;
    default:
        return 0;
    }

    va_start(args, format);
    vfprintf(g_log_fp, format, args);
    va_end(args);

    fprintf(g_log_fp, "\n");
    fflush(g_log_fp);
    return 0;
}

static sdk_ret_t
init_logger (const char* log_name)
{
    char log_path[64] = { 0 };

    snprintf(log_path, sizeof(log_path), "/var/log/pensando/%s", log_name);
    g_log_fp = fopen(log_path, "w");
    if (g_log_fp == NULL) {
        fprintf(stderr, "Failed to open log file %s", log_path);
        return SDK_RET_ERR;
    }
    sdk::lib::logger::init(diag_logger);
#ifdef DEBUG_ENABLE
    g_trace_level = sdk::lib::SDK_TRACE_LEVEL_VERBOSE;
#else
    g_trace_level = sdk::lib::SDK_TRACE_LEVEL_INFO;
#endif
    return SDK_RET_OK;
}

int
main (int argc, char* argv[])
{
    int i;
    test_mode_e mode = TEST_MODE_MAX;
    char diag_banner[64] = {0};
    char logname[128] = {0};
    char log_timestamp[64] = {0};
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    if (argc < 2)
        return usage(argc, argv);

    if (!strcmp(argv[1], "offline"))
        mode = OFFLINE_DIAG;
    else if (!strcmp(argv[1], "online"))
        mode = ONLINE_DIAG;
    else if (!strcmp(argv[1], "post"))
        mode = POST;
    else
    {
        printf("Invalid test mode\n");
        return usage(argc, argv);
    }

    sprintf(log_timestamp, "%d%d%d-%d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    //Create the logger
    sprintf(logname, "%s%s.%s.log", argv[1], (strcmp(argv[1], "post") ? "-diags" : ""), log_timestamp);
    assert(init_logger(logname) == SDK_RET_OK);

    assert(sdk::lib::pal_init(platform_type_t::PLATFORM_TYPE_HAPS) == sdk::lib::PAL_RET_OK);
  
    sprintf(diag_banner, "%s%s result", argv[1], (strcmp(argv[1], "post") ? "-diags" : ""));

    if ((argc == 2) || ((argc > 2) && (!strcmp(argv[2], "all"))))
    {
        //Print the Banner for diag test results
        PRINT_TEST_REPORT_BANNER(diag_banner);

        //Run all the tests in the tests_list
        for (i = 0; i < ARRAY_LEN(tests_list); i++)
        {
            tests_list[i].func(mode, argc-2, argv+2);
        }
    }
    else //argc > 2 && user selected test
    {
        //Run the specific test user has provided
        for (i = 0; i < ARRAY_LEN(tests_list); i++)
        {
            if (!strcmp(argv[2], tests_list[i].test_name))
            {
                tests_list[i].func(mode, argc-2, argv+2);
            }
        }
    }

    return 0;
}
