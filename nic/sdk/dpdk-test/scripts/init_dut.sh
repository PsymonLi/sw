#!/bin/bash

BUS0=XXX_BUS0
BUS1=XXX_BUS1
BUS2=XXX_BUS2

rmmod ionic > /dev/null 2>&1
insmod ~/drivers-linux-eth/drivers/eth/ionic/ionic.ko

modprobe msr

sleep 2

ip link set enp${BUS0}s0 up
ip link set enp${BUS1}s0 up

ip addr add 169.254.${BUS2}.2/24 dev enp${BUS2}s0
ip link set enp${BUS2}s0 up

sleep 2

# Enable SSH
./penctl.linux -a penctl.token update ssh-pub-key -f ~/.ssh/id_rsa.pub

sleep 1

# Hush up DHCP broadcasts
ssh -o StrictHostKeyChecking=accept-new root@169.254.${BUS2}.1 ifconfig inb_mnic0 down
ssh root@169.254.${BUS2}.1 ifconfig inb_mnic1 down

# Hush up LLDP nonsense
lldptool -L -i eno1 adminStatus=disabled
