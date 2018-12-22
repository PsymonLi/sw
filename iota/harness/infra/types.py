#! /usr/bin/python3
import iota.harness.infra.utils.utils   as utils

__status = [
    'SUCCESS',
    'FAILURE',
    'INVALID_ARG',
    'RETRY',
    'UNAVAIL',
    'IGNORED',
    'ABORTED',
    'ERROR',
    'DISABLED',
]
status = utils.List2Enum(__status)

__level = {
    0: 'NONE',
    1: 'CRITICAL',
    2: 'ERROR',
    3: 'WARNING',
    4: 'INFO',
    5: 'DEBUG',
    6: 'VERBOSE',
    7: 'MAX'
}
loglevel = utils.Dict2Enum(__level)

__result = [
    'PASS',
    'FAIL',
]
result = utils.List2Enum(__result)

__tbtype = [
    'ANY',
    'SIMULATION',
    'HARDWARE',
    'HYBRID'
]
tbtype = utils.List2Enum(__tbtype)


HEADER_SUMMARY = "-" * 140
FORMAT_TESTCASE_SUMMARY = "%-15s %-70s %-32s %-7s %-8s"
HEADER_SHORT_SUMMARY = "-" * 104

FORMAT_ALL_TESTSUITE_SUMMARY = "%-32s %-5s %-5s %-6s %-8s %-5s %-6s %-5s %-8s %-8s"
FORMAT_ALL_TESTBUNDLE_SUMMARY = "%-32s %-5s %-5s %-6s %-8s %-5s %-6s %-5s %-8s %-8s"


HOST_TOOLS_DIR = "/pensando/iota/tools"
