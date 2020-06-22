#!/bin/bash

if grep -q ubuntu /etc/os-release
then
  wget -c http://pm.test.pensando.io/tools/kubernetes-ubuntu.tar.gz
  tar xvzf kubernetes-ubuntu.tar.gz -C /
else
  wget -c http://pm.test.pensando.io/tools/kubernetes-centos7.tar.gz
  tar xvzf kubernetes-centos7.tar.gz -C /
cat <<EOF > /etc/sysctl.d/89-k8s.conf
net.bridge.bridge-nf-call-ip6tables = 1
net.bridge.bridge-nf-call-iptables = 1
net.ipv4.ip_forward = 1
EOF
  sysctl --system
fi

if command -v conntrack
then
  echo "conntrack exists"
else
  if grep -q redhat /etc/os-release
  then
    wget -c http://pm.test.pensando.io/tools/libnetfilter_cthelper-1.0.0-11.el7.x86_64.rpm
    wget -c http://pm.test.pensando.io/tools/libnetfilter_cttimeout-1.0.0-7.el7.x86_64.rpm
    wget -c http://pm.test.pensando.io/tools/libnetfilter_queue-1.0.2-2.el7_2.x86_64.rpm
    wget -c http://pm.test.pensando.io/tools/conntrack-tools-1.4.4-7.el7.x86_64.rpm
    rpm -i libnetfilter_cthelper-1.0.0-11.el7.x86_64.rpm
    rpm -i libnetfilter_cttimeout-1.0.0-7.el7.x86_64.rpm
    rpm -i libnetfilter_queue-1.0.2-2.el7_2.x86_64.rpm
    rpm -i conntrack-tools-1.4.4-7.el7.x86_64.rpm
  else
    if grep -q ubuntu /etc/os-release
    then
      apt-get install -y conntrack
    else
      yum install -y conntrack-tools
    fi
  fi
fi

if command -v socat
then
  echo "socat exists"
else
  if grep -q ubuntu /etc/os-release
  then
    apt-get install -y socat
  else
    yum install -y socat
  fi
fi

if command -v tc
then
  echo "tc exists"
else
  yum install -y iproute
fi

systemctl enable kubelet
systemctl start kubelet

sed -i '/swap/d' /etc/fstab
swapoff -a
