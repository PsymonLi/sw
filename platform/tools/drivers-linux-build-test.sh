#!/bin/bash

# Build test linux drivers with various kernel versions.
#
# usage: linux-drivers-build-test.sh [ what ... ]
#
# If option 'what' is not specified, build test kernel space drivers with all
# kernel ofed headers found under /usr/src, and the user space driver.
#
#   linux-drivers-build-test.sh
#
# Search for headers in a location besides /usr/src by overriding SRC.
#
#   SRC=/local/allenbh/docker/drivers-linux/sysroot/usr/src \
#       linux-drivers-build-test.sh
#
# If option 'what' is 'rdma-core', only build test the user space driver.
#
#   linux-drivers-build-test.sh rdma-core
#
# If option 'what' is a path to kernel or ofed headers, only build test kernel
# space drivers with those headers.
#
#   linux-drivers-build-test.sh /usr/src/linux-headers-4.15.0-46-generic
#
#   linux-drivers-build-test.sh /usr/src/ofa_kernel/4.9.0-3-amd64
#
#   linux-drivers-build-test.sh /local/allenbh/linux
#
# More than one 'what' can be specified.
#
#   linux-drivers-build-test.sh rdma-core \
#       /usr/src/linux-headers-4.15.0-46-generic \
#       /usr/src/ofa_kernel/4.9.0-3-amd64
#

TOP=$(readlink -f "$(dirname "$0")/../..")
MARK='################################'

echo -e "\n$MARK\ndrivers package\n$MARK\n"
"$TOP/platform/tools/drivers-linux.sh" || exit
cd "$TOP/platform/gen/drivers-linux" || exit

fail() { FAILURES="$FAILURES\n  $*" ; }

: ${SRC:=/usr/src}

export KSRC=
export OFA_KSRC=

is_linux() {
    test -e "$KSRC/include/generated/autoconf.h" -a \
        -e "$KSRC/include/config/auto.conf"
}

is_ofa() {
    KSRC="$SRC/linux-headers-$(basename "$OFA_KSRC")"
    is_linux
}

test_linux() {
    OFA_KSRC=
    echo -e "\n$MARK\n$KSRC\n$MARK\n"
    cd drivers
    make clean && make -k || fail "$KSRC"
    cd ..
}

test_ofa() {
    echo -e "\n$MARK\n$OFA_KSRC\n$MARK\n"
    cd drivers
    make clean && make -k || fail "$OFA_KSRC"
    cd ..
}

test_rdmacore() {
    echo -e "\n$MARK\nrdma-core\n$MARK\n"
    cd rdma-core
    ./build.sh || fail "rdma-core"
    cd ..
}

if [ $# -eq 0 ] ; then
    # build-test for each kernel version that has installed headers
    for KSRC in ${@:-$SRC/linux*/ $SRC/kernels/*/} ; do
        is_linux && test_linux
    done

    # build-test for each ofed rdma stack
    for OFA_KSRC in ${@:-$SRC/ofa_kernel-*/*} ; do
        is_ofa && test_ofa
    done

    # build-test the user space driver
    test_rdmacore
else
    # build-test each user supplied parameter
    for WHAT in "$@" ; do

        # build-test the user space driver
        if [ "x$WHAT" = "xrdma-core" ] ; then
            test_rdmacore
            continue
        fi

        # build-test user supplied kernel version
        KSRC="$WHAT"
        if is_linux ; then
            test_linux
            continue
        fi

        # build-test user supplied ofed rdma stack
        OFA_KSRC="$WHAT"
        if is_ofa ; then
            test_ofa
            continue
        fi

        # not found something to be tested
        fail "$WHAT (not found)"
    done
fi

echo -e "\n$MARK\nsummary\n$MARK\n"

if [ -v FAILURES ] ; then
echo -e "failures:$FAILURES"
exit 1
fi

echo "success"
