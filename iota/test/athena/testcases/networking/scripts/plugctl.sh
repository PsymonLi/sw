#!/bin/sh

# For Naples software development it is sometimes useful to
# isolate the server from Naples to allow Naples to restart
# after a firmware update without needing to restart the server.
# Make use of the Linux pci "hotplug" system to manually remove
# a Naples from the pci bus before firmware update, then rescan
# the pci bus after Naples is running updated firmware.
#
# If the ionic driver is loaded, it will unbind with the devices
# during the unplug operation and rebind during rescan.
#
# Also, for development it is sometimes useful to disable pci error
# reporting on the server to avoid server crashes due to pci bus
# errors while testing Naples firmware.  Run errdisable to disable
# error reporting by Naples hw, and mask errors on the root port.
# This should NOT be used as a production mode.

usage()
{
cat <<EOF >&2
usage: plugctl in [slot-root-bdf ...]
       plugctl out [naples-bdf ...]
       plugctl reset [naples-bdf ...]
EOF
}

plugin()
{
    # prefer cmdline
    dbdf_list="$*"
    # check $envar
    if [ -z "$dbdf_list" ]; then
	dbdf_list="$PLUGCTL_ROOT_PORTS"
    fi
    if [ -z "$dbdf_list" ]; then
	rescan=/sys/bus/pci/rescan
	echo "rescanning $rescan"
	echo 1 > $rescan
    else
	for dbdf in $dbdf_list; do
	    rescan=/sys/bus/pci/devices/$dbdf/rescan
	    if [ -f $rescan ]; then
		echo "rescanning $dbdf"
		echo 1 > $rescan
	    else
		echo "$dbdf not found" >&2
	    fi
	done
    fi
}

plugout()
{
    # prefer cmdline
    dbdf_list="$*"
    # if none given, plugout all Naples devices we find
    if [ -z "$dbdf_list" ]; then
	dbdf_list=`lspci -Dnd 1dd8:1000 | awk '{ print $1; }'`
    fi
    for dbdf in $dbdf_list; do
        remove=/sys/bus/pci/devices/$dbdf/remove
        if [ -f $remove ]; then
	    echo "removing $dbdf"
	    echo 1 > /sys/bus/pci/devices/$dbdf/remove
	else
	    echo "$dbdf not found" >&2
	fi
    done
}

errdisable()
{
    # prefer cmdline
    dbdf_list="$*"
    # if none given, config all Naples devices we find
    if [ -z "$dbdf_list" ]; then
	dbdf_list=`lspci -Dnd 1dd8:1000 | awk '{ print $1; }'`
    fi
    for dbdf in $dbdf_list; do
	# mask uncorrectable errors in root port AER cap
	port=$(basename $(dirname $(readlink "/sys/bus/pci/devices/$dbdf")))
	uemsk=`setpci -s $port ECAP_AER+8.l 2>/dev/null`
	if [ ! -z "$uemsk" ]; then
	    # mask: SurpriseDown, CompletionTimeout
	    uemsk=$(printf "%08x" $((0x$uemsk | 0x00004020)))
	    setpci -s $port ECAP_AER+8.l=$uemsk
	fi

	# disable SERR in command
	cmd=`setpci -s $dbdf COMMAND`
	cmd=$(printf "%04x" $((0x$cmd & ~0x0100)))
	setpci -s $dbdf COMMAND=$cmd

	# disable fatal error reporting in pci express cap devctl
	devctl=`setpci -s $dbdf CAP_EXP+8.w`
	devctl=$(printf "%04x" $((0x$devctl & ~0x0004)))
	setpci -s $dbdf CAP_EXP+8.w=$devctl
    done
}

reset()
{
    # prefer cmdline
    dbdf_list="$*"
    # if none given, reset all Naples devices we find
    if [ -z "$dbdf_list" ]; then
	dbdf_list=`lspci -Dnd 1dd8:1000 | awk '{ print $1; }'`
    fi
    for dbdf in $dbdf_list; do
	port=$(basename $(dirname $(readlink "/sys/bus/pci/devices/$dbdf")))
	echo "resetting $dbdf"
	brctl_orig=`setpci -s $port BRIDGE_CONTROL`
	# set SecondaryBusReset
	brctl_rst=$(printf "%04x" $((0x$brctl_orig | 0x0040)))
	setpci -s $port BRIDGE_CONTROL=$brctl_rst
	setpci -s $port BRIDGE_CONTROL=$brctl_orig
    done
}

case "$1" in
    in)    shift; plugin $*; errdisable;;
    out)   shift; errdisable; plugout $*;;
    reset) shift; errdisable; reset $*;;
    *)     usage;;
esac

