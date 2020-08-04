#!/usr/bin/python3

import argparse
import subprocess
import os
import sys

parser = argparse.ArgumentParser(description='Generate release ready drop to customers')
parser.add_argument('--rel-drop-dir', dest='rel_drop_dir',
                    default=None, help='Directory for storing the release drop')

parser.add_argument('--drop-version', dest='drop_version',
                    default=None, help='Which version of ESXi release drop to be generated')

GlobalOptions = parser.parse_args()

if GlobalOptions.drop_version == '67':
    bundle_name = "VMW-ESX-6.7.0-"
    untar_dir_name = "drivers-esx-eth"
elif GlobalOptions.drop_version == '65':
    bundle_name = "VMW-ESX-6.5.0-"
    untar_dir_name = "drivers-esx-eth-65"
elif GlobalOptions.drop_version == "70":
    # 7.0 does not use bundle anymore
    bundle_name = ""
    untar_dir_name = "drivers-esx-eth-70"
else:
    print ("Unknown version of release drop that requested to be generated %s" % GlobalOptions.drop_version)
    sys.exit(1)

# By looking into the name of vib file to figure out the driver name and version
def get_drv_name_ver():
    cmd = "ls %s/%s/platform/drivers/esxi/ionic_en/build/vib" % (GlobalOptions.rel_drop_dir, untar_dir_name)
    print(cmd)
    result = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    # vib_name will be something like ionic-en-1.4-1OEM.650.0.0.4598673.x86_64.vib, we just want to get ionic-en-1.4
    vib_name = result.communicate()[0]
    return vib_name.decode().split("-1OEM")[0]

def get_offline_bundle():
    # Rename offline-bundle.zip
    cmd = "mv %s/platform/drivers/esxi/ionic_en/build/bundle/offline-bundle.zip %s.zip" % (untar_dir_name, bundle_name)
    print(cmd)
    ret = os.system(cmd)
    assert(ret == 0)
    return 0

def get_doc():
    cmd = "mkdir -p %s/doc && mv %s/platform/drivers/esxi/README.md %s/doc" % (GlobalOptions.rel_drop_dir, untar_dir_name, GlobalOptions.rel_drop_dir)
    print(cmd)
    ret = os.system(cmd)
    assert(ret == 0)
    return 0

def copy_vib():
    cmd = "mv %s/%s/platform/drivers/esxi/ionic_en/build/vib/*.vib %s" % (GlobalOptions.rel_drop_dir, untar_dir_name, GlobalOptions.rel_drop_dir)
    print(cmd)
    ret = os.system(cmd)
    assert(ret == 0)
    return 0

# Only needed for vSphere 7.0
def copy_component():
    cmd = "mv %s/%s/platform/drivers/esxi/ionic_en/build/component/VMW*.zip %s" % (GlobalOptions.rel_drop_dir, untar_dir_name, GlobalOptions.rel_drop_dir)
    print(cmd)
    ret = os.system(cmd)
    assert(ret == 0)
    return 0

def get_pencli():
    cmd = "mv %s/%s/platform/drivers/esxi/esx-pencli.pyc %s" % (GlobalOptions.rel_drop_dir, untar_dir_name, GlobalOptions.rel_drop_dir)
    print(cmd)
    ret = os.system(cmd)
    assert(ret == 0)
    return 0

def get_bulletin_xml():
    cmd = "mv %s/%s/platform/drivers/esxi/ionic_en/build/bulletin.xml %s" % (GlobalOptions.rel_drop_dir, untar_dir_name, GlobalOptions.rel_drop_dir)
    print(cmd)
    ret = os.system(cmd)
    assert(ret == 0)
    return 0

def get_penutil_vib():
    cmd = "mv %s/%s/platform/penutil/build/vib/*.vib %s" % (GlobalOptions.rel_drop_dir, untar_dir_name, GlobalOptions.rel_drop_dir)
    print(cmd)
    ret = os.system(cmd)
    assert(ret == 0)
    return 0
 
def get_penutil_bundle():
    cmd = "mv %s/%s/platform/penutil/build/bundle/PEN-*.zip %s" % (GlobalOptions.rel_drop_dir, untar_dir_name, GlobalOptions.rel_drop_dir)
    print(cmd)
    ret = os.system(cmd)
    assert(ret == 0)
    return 0
 
def get_penutil_component():
    cmd = "mv %s/%s/platform/penutil/build/component/*.zip %s" % (GlobalOptions.rel_drop_dir, untar_dir_name, GlobalOptions.rel_drop_dir)
    print(cmd)
    ret = os.system(cmd)
    assert(ret == 0)
    return 0
  
def del_untar_dir():
    cmd = "rm %s/%s/platform/ -rf" % (GlobalOptions.rel_drop_dir, untar_dir_name)
    print(cmd)
    ret = os.system(cmd)
    assert(ret == 0)

    return 0

if GlobalOptions.drop_version == "65" or GlobalOptions.drop_version == "67":
    drv_name_ver = get_drv_name_ver()
    bundle_name += drv_name_ver + "-offline_bundle"
    get_offline_bundle()

copy_vib()

# Only needed for vSphere 7.0
if GlobalOptions.drop_version == "70":
    copy_component()

get_doc()
get_pencli()
get_bulletin_xml()

if GlobalOptions.drop_version == "67":
    get_penutil_vib()
    get_penutil_bundle()
if GlobalOptions.drop_version == "70":
    get_penutil_vib()
    get_penutil_component()

del_untar_dir()

