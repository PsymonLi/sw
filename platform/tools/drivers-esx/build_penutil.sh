#!/bin/sh

set -o pipefail
set -ex

DIR=$(dirname "$0")
DIR=$(readlink -f "$DIR")

echo $PWD

if [ $1 -eq 67 ]
then
    cd platform/penutil/ && cp Makefile_ESXI_67 Makefile && make
    rm -rf .build
elif [ $1 -eq 65 ]
then
    cd platform/penutil/ && cp Makefile_ESXI_65 Makefile && make
    rm -rf .build
else
    cd platform/penutil/ && cp Makefile_ESXI_70 Makefile && make
    rm -rf .build
fi
# TODO: Add ESXi version as a part of the final print
echo "Building ESXi version of penutil successfull...."
