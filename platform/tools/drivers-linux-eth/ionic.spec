%define kernel KERNEL
%define kernel_release %(uname -r)
%define ionic_version 0.0.0
%define dpath kernel/drivers/net/ethernet/pensando/ionic
%define udev_file /etc/udev/rules.d/81-pensando-net.rules

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
mkdir -p %{buildroot}/lib/modules/%{kernel_release}/%{dpath}
install -m 644 ionic.conf %{buildroot}/etc/modules-load.d
install -m 644 drivers/eth/ionic/ionic.ko %{buildroot}/lib/modules/%{kernel_release}/%{dpath}

%files
%defattr(0644, root, root)
/etc/modules-load.d/ionic.conf
/lib/modules/%{kernel_release}/%{dpath}/ionic.ko

%pre
# save off original kernel driver
for f in ionic.ko ionic.ko.gz ionic.ko.xz ; do
	n=/lib/modules/%{kernel_release}/%{dpath}/$f
	m=/lib/modules/%{kernel_release}/%{dpath}/${f}.orig
	if [ -e $n -a ! -e $m ] ; then
		mv $n $m
	fi
done

%post
echo 'SUBSYSTEM=="net", ENV{ID_VENDOR_ID}=="0x1dd8", NAME="$env{ID_NET_NAME_PATH}"' > %{udev_file}
echo Updating module files...
depmod -a
echo Updating initramfs...
dracut --force

%postun
# restore original kernel driver
for f in ionic.ko ionic.ko.gz ionic.ko.xz ; do
	n=/lib/modules/%{kernel_release}/%{dpath}/$f
	m=/lib/modules/%{kernel_release}/%{dpath}/${f}.orig
	if [ -e $m ] ; then
		mv $m $n
	fi
done
rm %{udev_file}
echo Updating module files...
depmod -a
echo Updating initramfs...
dracut --force


%changelog
* Fri Jun 19 2020 Shannon Nelson <snelson@pensando.io>
- add bits to save off inbox driver and update initramfs
* Thu Apr 16 2020 Drew Michaels <dmichaels@pensando.io>
- First package release.
