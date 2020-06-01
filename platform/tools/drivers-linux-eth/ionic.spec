%define kernel KERNEL
%define kernel_release %(uname -r)
%define ionic_version 0.0.0

%define debug_package %{nil}

%define release RELEASE
%define distro DISTRO

Name:		RPM
Vendor:		Pensando Systems
Version:	%{ionic_version}
Release:	%{release}.%{distro}
Summary:	Pensando Systems IONIC Driver
URL:		https://pensando.io

Group:		System Environment/Kernel
License:	GPLv2
Source:		RPM-%{ionic_version}.tar.gz

BuildRequires:	REQUIRES-devel = %{kernel}
Requires:	REQUIRES = %{kernel}

%description
This package provides the Pensando Systems IONIC kernel driver (ionic.ko).

%prep

%setup -q

%build
sh build.sh
echo "ionic" > ionic.conf

%install
mkdir -p %{buildroot}/etc/modules-load.d
mkdir -p %{buildroot}/lib/modules/%{kernel_release}/kernel/drivers/net
install -m 644 ionic.conf %{buildroot}/etc/modules-load.d
install -m 644 drivers/eth/ionic/ionic.ko %{buildroot}/lib/modules/%{kernel_release}/kernel/drivers/net

%files
%defattr(0644, root, root)
/etc/modules-load.d/ionic.conf
/lib/modules/%{kernel_release}/kernel/drivers/net/ionic.ko

%post
echo 'SUBSYSTEM=="net", ENV{ID_VENDOR_ID}=="0x1dd8", NAME="$env{ID_NET_NAME_PATH}"' > /etc/udev/rules.d/81-pensando-net.rules
depmod -a
modprobe ionic

%postun
rmmod ionic

%changelog
* Thu Apr 16 2020 Drew Michaels <dmichaels@pensando.io>
- First package release.
