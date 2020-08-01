#!/usr/bin/python
# {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

import json
import os
import argparse

# compare manifest from the upgrade image and current running image


def upgrade_to_same_image_allowed():
    upgrade_to_same_check_file = "/data/upgrade_to_same_firmware_allowed"
    return os.path.isfile(upgrade_to_same_check_file)


def is_same_image(mode, rdata, udata, ndata):
    fields_build = ["build_date", "base_version", "software_version"]
    for field in fields_build:
        print(
            "Field %s run-version %s new-version %s"
            % (field, rdata[field], ndata[field])
        )
        if rdata[field] != ndata[field]:
            return False

    udata_keys = {x + "_hitless_compat_version" for x in udata["modules"].keys()}
    ndata_keys = set(ndata["upgrade"][mode].keys())
    ndata_keys.remove("firmware_hitless_compat_version")

    if udata_keys != ndata_keys:
        print("Different sets of version numbers present in the images")
        print(", ".join(udata_keys))
        print(", ".join(ndata_keys))
        return False

    return True


def is_upgrade_ok(mode, rdata, udata, ndata):
    firmware_ver_str = "firmware_" + mode + "_compat_version"

    # compare the pipeline
    if rdata["software_pipeline"] != ndata["software_pipeline"]:
        print(
            "Compatibility check failed, pipeline mismatch( running=%s, new=%s )"
            % (rdata["software_pipeline"], ndata["software_pipeline"])
        )
        return False

    # compare firmware upgrade major version. This keeps track of any upgrade
    # module version numbers added or removed.
    firmware_major_version = ndata["upgrade"][mode][firmware_ver_str]["major"]
    if udata["firmware"]["major"] != firmware_major_version:
        print(
            "Compatibility check failed, firmware major version "
            "mismatch( running %s new %s)"
            % (udata["firmware"]["major"], firmware_major_version)
        )
        return False

    # compare all upgrade module version numbers
    for ukey, uver in udata["modules"].items():
        cc_attr = ukey + "_" + mode + "_compat_version"
        nver = ndata["upgrade"][mode].get(cc_attr)

        if nver is not None and uver["major"] != nver["major"]:
            print(
                "Compatibility check failed, %s doesn't match"
                "( current version=%s, version in the new image=%s )"
                % (ukey, uver["major"], nver["major"])
            )
            return False

    # check if the new image is the same as the running image and if
    # yes, upgrade to the same image is allowed.
    if not upgrade_to_same_image_allowed() and is_same_image(mode, rdata, udata, ndata):
        print(
            "Compatibility check failed, upgrading to the same " "firmware not allowed"
        )
        return False

    # all checks passed.
    print("Compatibility check successful")
    return True


def main():

    helpstr = (
        "Check if the firmware provided is compatible with "
        "the running version for ISSU upgrade"
    )
    parser = argparse.ArgumentParser(description=helpstr)
    parser.add_argument("running_meta", help="Meta info for the running image")
    parser.add_argument("running_img", help="mainfwa|mainfwb")
    parser.add_argument("new_meta", help="New image meta")
    parser.add_argument("mode", help="Upgrade mode(hitless|graceful)")
    args = parser.parse_args()

    upgrade_cc_data_file = "/nic/upgrade/etc/upgrade_cc_" + args.mode + "_version.json"

    try:
        with open(args.running_meta, "r") as f:
            rdata = json.load(f)

        with open(upgrade_cc_data_file, "r") as f:
            udata = json.load(f)

        with open(args.new_meta, "r") as f:
            ndata = json.load(f)

        meta_running = rdata[args.running_img]["system_image"]
        if is_upgrade_ok(args.mode, meta_running, udata, ndata):
            return 0
        else:
            return -1

    except Exception as e:
        print("Compatibility check failed, due to exception")
        print(e)
        return -1


if __name__ == "__main__":
    exit(main())
