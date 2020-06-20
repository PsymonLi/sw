#!/bin/bash


# Running build.sh is equivalent to running make 
# in the drivers directory of this package.

DIR=$(dirname "$0")
DIR=$(readlink -f "$DIR")

make -j -C "$DIR/drivers" || exit
