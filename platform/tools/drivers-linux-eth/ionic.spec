
# Disable the building of the debug package(s).
%define debug_package %{nil}

%define kmod_name	ionic
%define kmod_basever	@KMOD_BASEVER@
%define kmod_release	@KMOD_RELEASE@
%define kmod_version	@KMOD_VERSION@

%define distro	@DISTRO@

%define kernel_version %(uname -r)
%define kernel_basever %(echo %{kernel_version} | cut -d- -f1)
%define kernel_release %(echo %{kernel_version} | cut -d- -f2- | rev | cut -d. -f2- | rev | tr - .)

Name:		%{kmod_name}
Vendor:		Pensando Systems
Version:	%{kmod_basever}
Release:	%{kmod_release}.%{distro}
Summary:	Pensando Systems %{kmod_name} Driver
URL:		https://pensando.io

Group:		System Environment/Kernel
License:	GPLv2
Source0:	%{kmod_name}-%{kmod_version}.tar.gz
Source1:	%{kmod_name}.files
Source2:	kmod-%{kmod_name}.conf
Source3:	%{kmod_name}.conf

BuildRequires:	%{kernel_module_package_buildreqs}
ExclusiveArch:	x86_64

# Macro defined in /usr/lib/rpm/redhat/macros
%kernel_module_package @DASHB@ -f %{SOURCE1}

%description
This package provides the %{vendor} %{kmod_name} kernel driver.

%prep
%setup -q

%build
sh build.sh

%install

%{__install} -d %{buildroot}/lib/modules/%{kernel_version}/extra/%{kmod_name}/
%{__install} -m 755 drivers/eth/%{kmod_name}/%{kmod_name}.ko %{buildroot}/lib/modules/%{kernel_version}/extra/ionic/
%{__install} -d %{buildroot}%{_sysconfdir}/depmod.d/
%{__install} -m 644 kmod-%{kmod_name}.conf %{buildroot}%{_sysconfdir}/depmod.d/
%{__install} -d %{buildroot}%{_sysconfdir}/dracut.conf.d/
%{__install} -m 644 %{kmod_name}.conf %{buildroot}%{_sysconfdir}/dracut.conf.d/%{kmod_name}.conf
@UDEV_RULE@

%clean
%{__rm} -rf %{buildroot}

%changelog
* Mon Aug 10 2020 Shannon Nelson <snelson@pensando.io>
- fix up kernel_module_package use for sles
* Tue Jul 21 2020 Neel Patel <neel@pensando.io>
- Use kernel module package rpm macros
* Tue Jun 30 2020 Shannon Nelson <snelson@pensando.io>
- add bits to save off inbox driver and update initramfs
* Thu Apr 16 2020 Drew Michaels <dmichaels@pensando.io>
- First package release.
