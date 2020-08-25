#!/bin/bash

#
# Do not run this script directly: use ../setup.sh instead
#

DUT=$1
TESTER=$2

APTLIBS=" \
tcl \
python3 \
python3-pexpect \
python3-numpy \
python3-docutils \
python3-xlrd \
python3-pip \
python3-git \
libnuma-dev \
libpcap-dev \
lldpad \
"
PIPLIBS=" \
xlwt \
jinja2 \
"
RMLIBS=" \
python-scapy \
python-ipython \
python3-scapy \
python3-ipython \
"

# Configure the Tester
echo
echo "[>>] Configuring packages on $TESTER"
apt-get update
apt install -y $APTLIBS
apt remove -y $RMLIBS
apt -y autoremove
pip3 install $PIPLIBS

# Set python3 as the default
update-alternatives --install /usr/bin/python python /usr/bin/python2 0
update-alternatives --install /usr/bin/python python /usr/bin/python3 1

# Install Scapy on the Tester
cd scapy
sed -i "s/if not os.path.isdir(os.path.join/if not os.path.exists(os.path.join/" scapy/__init__.py
python3 setup.py install > scapy.log 2>&1
cd -

# Configure the DUT
echo
echo "[>>] Installing packages on $DUT"
ssh root@$DUT "apt-get update && apt install -y $APTLIBS && apt remove -y $RMLIBS && apt -y autoremove && pip3 install $PIPLIBS"
ssh root@$DUT "update-alternatives --install /usr/bin/python python /usr/bin/python2 0"
ssh root@$DUT "update-alternatives --install /usr/bin/python python /usr/bin/python3 1"

# Install Scapy on the DUT (required by some tests)
ssh root@$DUT "rm -rf scapy"
scp -r scapy root@$DUT:
ssh root@$DUT "cd scapy ; python3 setup.py install > scapy.log 2>&1"

# Hush up the Scapy splash screen
echo
echo "[>>] Configuring Scapy..."
echo conf.fancy_prompt=False > ~/.scapy_startup.py
ssh root@$DUT "echo conf.fancy_prompt=False > ~/.scapy_startup.py"

readarray -t INTEL_PORTS <<<`lspci | grep Ethernet | tail -n 2 | cut -d ' ' -f 1`
readarray -t NAPLES_PORTS <<<`ssh root@$DUT "lspci | grep 1dd8:1002 | cut -d ' ' -f 1"`

echo
echo "[>>] Configuring init_dut.sh on $DUT..."
PCIBUS=`echo ${NAPLES_PORTS[0]} | cut -d ':' -f 1`
PCIBUS=$(( 16#$PCIBUS ))
sed -i "s/XXX_BUS0/${PCIBUS}/" scripts/init_dut.sh
PCIBUS=$((PCIBUS + 1))
sed -i "s/XXX_BUS1/${PCIBUS}/" scripts/init_dut.sh
PCIBUS=$((PCIBUS + 1))
sed -i "s/XXX_BUS2/${PCIBUS}/" scripts/init_dut.sh
scp scripts/init_dut.sh root@$DUT:
ssh root@$DUT "./init_dut.sh"

# Let config_tester.sh finish
./scripts/config_tester.sh $@
