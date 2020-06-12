#!/usr/bin/python

# {C} Copyright 2020 Pensando Systems Inc. All rights reserved\n

import sys
import json
import os

# compare manifest from the upgrade image and current
# running image

if len(sys.argv) != 5:
    print("Usage : <command> <running_meta.json> <running_img> <new_meta.json> <mode>")
    exit -1

running_meta = sys.argv[1]
running_img = sys.argv[2]
new_meta = sys.argv[3]
mode = sys.argv[4]

fields_build = ["build_date", "base_version", "software_version"]
fields_cc = ["nicmgr", "kernel", "pcie", "pdsagent", "p4", "p4plus", "linkmgr"]

upgrade_to_same_check_file = '/data/upgrade_to_same_firmware_allowed'
upgrade_cc_data_file = '/nic/upgrade/etc/upgrade_cc_' + mode + '_version.json'

def is_upgrade_image_same(rdata, udata, ndata):
    match = 0
    cc_attr = None
    fields = fields_build + fields_cc

    for e in fields_build:
        print("Field %s run-version %s new-version %s" \
              %(e, rdata[running_img]['system_image'][e], ndata[e]))
        if rdata[running_img]['system_image'][e] == ndata[e]:
            match = match + 1

    for e in fields_cc:
        cc_attr = e + '_' + mode + '_compat_version'
        if udata['versions'][e] == ndata['upgrade'][mode][cc_attr]:
            match = match + 1

    if match == len(fields_build) + len(fields_cc):
        print("Compat check, trying to upgrade to same")
        return True

    return False

def is_upgrade_to_same_allowed():
    if not os.path.isfile(upgrade_to_same_check_file):
        print("Compat check failed, upgrading to same image not allowed")
        return False
    return True

def is_upgrade_ok(udata, ndata):
    match = 0
    cc_attr = None

    for e in fields_cc:
        cc_attr = e + '_' + mode + '_compat_version'
        if udata['versions'][e] == ndata['upgrade'][mode][cc_attr]:
            match = match + 1
        else:
            break

    if match != len(fields_cc):
        print("Compat check failed, version mismatch in %s" %(cc_attr))
        return False

    return True

def main():
    try:
        with open(running_meta, 'r') as f:
            rdata = json.load(f)

        with open(new_meta, 'r') as f:
            ndata = json.load(f)

        with open(upgrade_cc_data_file, 'r') as f:
            udata = json.load(f)

        if is_upgrade_image_same(rdata, udata, ndata):
            if is_upgrade_to_same_allowed():
                print("Compat check successful")
                return 0

        elif is_upgrade_ok(udata, ndata):
            print("Compat check successful")
            return 0

        print("Compat check failed")
        return -1

    except Exception as e:
        print("Compat check failed, due to exception")
        print(e)
        return -1

if __name__ == '__main__':
    exit(main())
