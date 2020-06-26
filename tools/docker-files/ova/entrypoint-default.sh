#!/bin/bash

set -x
# set -e
umask 000
echo | ./VMware-ovftool-4.3.0-13981069-lin.x86_64.bundle --eulas-agreed
# Build the OVA and download it using buildit tool. Try a few times.
for i in {1..10}; do
    output=`/t/buildit b -i /t/pen-install.iso pensando/e2e-images/venice/venice.yaml`
    if [[ $? != 0 ]]; then
        echo "attemp: $i to run buildit failed"
        echo -e "buildit output = $output"
    else
	break
    fi
    if [[ $i < 10 ]]; then
	echo "retrying again in 60 seconds"
	sleep 60
    else
	echo "giving up.."
	exit 1
    fi
done
# Parse build id
build_id=`echo -e "$output" | grep 'Build ID: ' | uniq | sed 's/Build ID: //'`
echo "Build ID = $build_id"
if [[ $build_id != build-+([0-9]) ]]; then 
    echo "cannot determine the build id"
    exit 1
fi
# Directory to keep the output of the build
if [[ $# == 1 && $1 == "apulu" ]]; then
    echo "building ova and qcow2 for apulu venice"
    dir="output-apulu"
else
    echo "building ova and qcow2 for iris venice"
    dir="output"
fi
mkdir $dir
./buildit i p -o $dir/venice.ova $build_id
# Untar the OVA so we can modify the .ovf file and repackage it
chmod -R 777 $dir
file_list=`tar -C $dir -xvf $dir/venice.ova | xargs`
rm -f $dir/venice.ova
# We need to do two things
# 1. For OVF, insert the OVA properties and repackage the OVA
# 2. For vmdk, create qcow2
for file in $file_list; do
    if [[ $file == *".ovf" ]]; then
        # Run python script to insert the OVA properties and repackage the OVA
        python venice_ova_insert_ova_properties.py $dir/$file
    elif [[ $file == *".vmdk" ]]; then
        # Run qemu-img convert here
	qemu-img convert -f vmdk -O qcow2 $dir/$file $dir/venice.qcow2
    fi
done
chmod -R 777 $dir 
for file in $file_list; do
    if [[ $file != "." ]]; then
        rm -f $dir/$file
    fi
done
