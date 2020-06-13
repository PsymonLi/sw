# No '#!/bin/sh'. Meant to run directly from sysint.sh.
# Don't want the cost of additional shell process invokation
# during system init.

SYSTEM_CONFIG_FILE=/sysconfig/config0/system-config.json

# Check if SSH is enabled through engineering knob.
if [ -f $SYSTEM_CONFIG_FILE ]; then
    ssh_eng_knob=$(jq -r '.ssh' $SYSTEM_CONFIG_FILE 2> /dev/null)
else
    ssh_eng_knob=""
fi

# Reload the keys if asked to keep them persistent across reloads.
# Or if SSH is enabled through engineering knob.
source /nic/conf/system_boot_config
if [ "${SSH_KEYS_PERSIST}" = "on" ] || [ "${ssh_eng_knob}" = "enable" ]; then
    # Restore "authorized_keys"
    SAVED_AUTHORIZED_KEYS=/sysconfig/config0/.authorized_keys
    AUTHORIZED_KEYS=/root/.ssh/authorized_keys
    if [ -f ${SAVED_AUTHORIZED_KEYS} ]; then
        mkdir -p $(dirname ${AUTHORIZED_KEYS})
        cp ${SAVED_AUTHORIZED_KEYS} ${AUTHORIZED_KEYS}
        chmod 0600 ${AUTHORIZED_KEYS}
    fi
fi
