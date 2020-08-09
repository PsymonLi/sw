set -x

os_version=`awk -F= '$1=="VERSION_ID" { print $2 ;}' /etc/os-release | sed -e 's/\([678]\)\../\1/'`


if [ "$1" = "register" ]; then
	# register and add repos
	/usr/sbin/subscription-manager register --username rhel@pensando.io --password N0isystem$
	/usr/sbin/subscription-manager attach --pool=8a85f99c707807c80170843dd71c7775
	
	# for RHEL 7	
        if [[ $os_version == *"7"* ]]; then
		subscription-manager repos --enable rhel-7-server-optional-rpms 
		subscription-manager repos --enable rhel-server-rhscl-7-rpms 
		subscription-manager repos --enable rhel-7-server-extras-rpms
		yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo

		yum -y install bind-utils net-tools wireshark

		yum -y install iperf3 vim sshpass sysfsutils net-tools libnl3-devel valgrind-devel hping3 lshw python-setuptools
		yum -y install elfutils-libelf-devel nmap
		yum -y install docker-ce container-selinux
                yum -y install libtool-ltdl
                yum -y install kernel-abi-whitelists

		easy_install pip
		pip install pyyaml

		yum install -y python3
		pip3 install --upgrade pip
		pip3 install pyyaml

		# iperf
		rpm -ivh https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
		yum -y install iperf

        elif [[ $os_version == *"8"* ]]; then
		yum -y install libtool-ltdl
		yum -y install kernel-abi-whitelists
		yum -y install kernel-rpm-macros
	fi
elif [ "$1" = "unregister" ]; then
	sudo subscription-manager remove --all
	sudo subscription-manager unregister
	sudo subscription-manager clean
fi
