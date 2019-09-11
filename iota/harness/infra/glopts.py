#! /usr/bin/python3
import argparse
import pdb
import sys
import os

__valid = False

parser = argparse.ArgumentParser(description='IOTA Harness')
parser.add_argument('--debug', dest='debug',
                    action='store_true', help='Enable Debug Mode')
parser.add_argument('--verbose', dest='verbose',
                    action='store_true', help='Enable Verbose Mode')
parser.add_argument('--dryrun', dest='dryrun', action='store_true',
                    help='Dry-Run mode. (No communication with HAL & Model)')
parser.add_argument('--testsuite', dest='testsuite', default=None,
                    help='Run single Testsuite.')
parser.add_argument('--testbundle', dest='testbundle', default=None,
                    help='Run single Testbundle.')
parser.add_argument('--testcase', dest='testcase', default=None,
                    help='Run single Testcase.')
parser.add_argument('--coverage', dest='coverage',
                    action='store_true', help='Collect code coverage data.')
parser.add_argument('--coverage-file', dest='coverage_file',
                    help='Naples Coverage file.')
parser.add_argument('--coverage-dir', dest='coverage_dir',
                    help='Directory to copy coverage file')

#parser.add_argument('--svcport', dest='svcport', default=60000,
#                    help='IOTA Service Port.')
parser.add_argument('--testbed', dest='testbed_json', default="/warmd.json",
                    help='Testbed JSON file')
parser.add_argument('--no-keep-going', dest='no_keep_going',
                    action='store_true', help='Stop on first error.')
parser.add_argument('--stop-on-critical', dest='stop_on_critical',
                    action='store_true', default=True, help='Stop on critical errors')
#parser.add_argument('--mode', dest='mode', default='hardware',
#                    choices=["hardware", "simulation", "mixed"],
#                    help='Testbed Mode: Hardware / Simulation / Mixed.')
parser.add_argument('--pipeline', dest='pipeline', default="iris",
                    help='Filter tests by Pipeline')
parser.add_argument('--rerun', dest='rerun',
                    action='store_true', help='Re-run mode, Skips copy and few init steps.')
parser.add_argument('--skip-firmware-upgrade', dest='skip_firmware_upgrade',
                    action='store_true', help='Skip Firmware Upgrade and Start Tests.')
parser.add_argument('--only-firmware-upgrade', dest='only_firmware_upgrade',
                    action='store_true', help='Only Do Firmware Upgrade and Stop.')
parser.add_argument('--only-reboot', dest='only_reboot',
                    action='store_true', help='Only reboot the nodes and start tests.')
parser.add_argument('--regression', dest='regression',
                    action='store_true', help='Only run regression Testsuites.')
parser.add_argument('--skip-sanity', dest='skip_sanity',
                    action='store_true', help='Skip sanity testbundles.')
parser.add_argument('--stress', dest='stress',
                    action='store_true', help='Enable stress test mode.')
parser.add_argument('--stress-factor', dest='stress_factor', type=int, default=1,
                    help='Stress factor - Control # of iterations in stress test.')
parser.add_argument('--logdir', dest='logdir', default=None,
                    help='Save logs in a specific directory. (Relative to IOTA directory)')
parser.add_argument('--skip-logs', dest='skip_logs', help='Skip log collection.',
                    action='store_true')
GlobalOptions = parser.parse_args()

GlobalOptions.default_username = "vm"
GlobalOptions.default_password = "vm"

GlobalOptions.testsuites = None
if GlobalOptions.testsuite != None:
    GlobalOptions.testsuites = GlobalOptions.testsuite.split(',')

GlobalOptions.testbundles = None
if GlobalOptions.testbundle != None:
    GlobalOptions.testbundles = GlobalOptions.testbundle.split(',')

GlobalOptions.beg_testcase = None
GlobalOptions.end_testcase = None
GlobalOptions.testcases = None
GlobalOptions.markers_present = False
GlobalOptions.inb_markers = False
if GlobalOptions.testcase != None:
    if "..." in GlobalOptions.testcase:
        markers = GlobalOptions.testcase.split("...")
        GlobalOptions.testcase_begin = markers[0]
        GlobalOptions.testcase_end = markers[1]
        GlobalOptions.markers_present = True
    else:
        GlobalOptions.testcases = GlobalOptions.testcase.split(',')

def __validate():
    if GlobalOptions.debug:
        print("========================================================")
        print("Global Command Line Options")
        for k,v in GlobalOptions.__dict__.items():
            print ("  %s = " % k, v)
        print("========================================================")

    global __valid
    __valid = True

if not __valid:
    __validate()
