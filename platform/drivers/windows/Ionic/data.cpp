
#define NO_EXTERNS
#include "common.h"

extern "C" {
//
// Version info
//
IONIC_VERSION_INFO IonicVersionInfo;

PDRIVER_OBJECT IonicDriverObject;

NDIS_HANDLE IonicDriver = NULL;

OSVERSIONINFOEXW	IonicOSVersion;

ULONG	StateFlags = 0;

ULONG	DriverFlags = 0;

unsigned int devcmd_timeout = IONIC_DEVCMD_WAIT_TIME_SEC;

unsigned int max_slaves = 0;

unsigned int rx_copybreak = IONIC_RX_COPYBREAK_DEFAULT;

NDIS_HANDLE AdapterControl = NULL;

DEVICE_OBJECT *AdapterCntrlDevObj = NULL;

UNICODE_STRING AdapterRegistryPath = {0};

LIST_ENTRY AdapterList;

NDIS_MUTEX AdapterListLock;

LONG port_count = 0;

KEVENT  perfmon_event;

LIST_ENTRY memory_block_list;

NDIS_SPIN_LOCK memory_block_lock;

struct _PERF_MON_CB *ionic_perfmon_stats = NULL;

ULONG ionic_stats_len = 0;

NDIS_HANDLE perfmon_timer = NULL;

LONG	perfmon_counter_cnt = 0;

//
// Trace variables
//

ULONG TraceFlags = 0;

ULONG TraceComponent = 0;

ULONG TraceLevel = 0;

NDIS_SPIN_LOCK TraceLock;

ULONG TraceRemainingLength = 0;

char *TraceCurrentBuffer = NULL;

char *TraceBuffer = NULL;

ULONG TraceCounter = 0;

ULONG TraceBufferLength = 0;

PTraceLogMsg TraceMsgFnc = NULL;
};
