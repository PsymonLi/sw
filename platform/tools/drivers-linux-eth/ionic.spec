%define kernel KERNEL
%define kernel_release %(uname -r)
%define kver %(uname -r | sed -e 's/.x86_64//')
%define krev %(uname -r | tr - . | sed -e 's/.x86_64//')
%define ionic_version 0.0.0
%define dpath kernel/drivers/net/ethernet/pensando/ionic
%define udev_file /etc/udev/rules.d/81-pensando-net.rules

%define debug_package %{nil}

%define release RELEASE
%define distro DISTRO
%define iver %{ionic_version}.RELEASE

Name:		RPM
Vendor:		Pensando Systems
Version:	%{ionic_version}
Release:	%{release}.%{distro}.%{krev}
Summary:	Pensando Systems IONIC Driver
URL:		https://pensando.io

Group:		System Environment/Kernel
License:	GPLv2
Source:		RPM-%{ionic_version}.tar.gz

Requires:	REQUIRES = %{kver}

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
install -m 644 drivers/eth/ionic/ionic.ko %{buildroot}/lib/modules/%{kernel_release}/%{dpath}/ionic.ko-%{iver}

%files
%defattr(0644, root, root)
/etc/modules-load.d/ionic.conf
/lib/modules/%{kernel_release}/%{dpath}/ionic.ko-%{iver}

%pre
# Before we install the driver, check to see if there is an existing one,
# and if so, try to save it if there isn't a file that it is already
# linked to.
for drv in ionic.ko ionic.ko.xz ionic.ko.gz ; do
	f=/lib/modules/%{kernel_release}/%{dpath}/${drv}
	if [ -e ${f} ] ; then
		inodea=`ls -i ${f} | awk '{print $1}'`
		inodeb=0
		ver=`modinfo -F version ${f} | tr - .`
		if [ -e ${f}-${ver} ] ; then
			inodeb=`ls -i ${f}-${ver} | awk '{print $1}'`
		fi

		if [ ${inodea} -eq ${inodeb} ] ; then
			rm ${f}
		else
			mv ${f} ${f}-${ver}
		fi
	fi
done

%post
# Link the version we just installed to ionic.ko
ln -f /lib/modules/%{kernel_release}/%{dpath}/ionic.ko-%{iver} /lib/modules/%{kernel_release}/%{dpath}/ionic.ko

# RHEL 8.0, 8.1, and 8.2 need this udev hack for correct interface names
if [ %{distro} == rhel8u0 -o %{distro} == rhel8u1 -o %{distro} == rhel8u2 ] ; then
	echo 'SUBSYSTEM=="net", ENV{ID_VENDOR_ID}=="0x1dd8", NAME="$env{ID_NET_NAME_PATH}"' > %{udev_file}
fi

depmod -a %{kernel_release}
dracut --force --kver %{kernel_release}

%preun
# In this pre-uninstall section, we know that ionic.ko-iver exists,
# and usually it is linked to the same inode as ionic.ko, but it is
# possible that another RPM has been installed and changed that link,
# so remove ionic.ko only if they point to the same inode.
f=/lib/modules/%{kernel_release}/%{dpath}/ionic.ko
inodea=`ls -i ${f} | awk '{print $1}'`
inodeb=`ls -i ${f}-%{iver} | awk '{print $1}'`
if [ ${inodea} -eq ${inodeb} ] ; then
	rm ${f}
fi

%postun
# Don't remove the udev rule we added in the broken rhel8.x so that we don't
# lose it when the user does an "rpm -U" update.

# If there is no ionic.ko left, but there is an ionic.ko-,
# put it back where it can be used.
f=/lib/modules/%{kernel_release}/%{dpath}/ionic.ko
g=/lib/modules/%{kernel_release}/%{dpath}/ionic.ko-
if [ ! -e ${f} -a -e ${g} ] ; then
	mv ${g} ${f}
fi

depmod -a %{kernel_release}
dracut --force --kver %{kernel_release}


%changelog
* Tue Jun 30 2020 Shannon Nelson <snelson@pensando.io>
- add bits to save off inbox driver and update initramfs
* Thu Apr 16 2020 Drew Michaels <dmichaels@pensando.io>
- First package release.
