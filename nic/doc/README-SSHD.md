System boot up behavior customization:
======================================
The file '/nic/conf/system_boot_config' that is bundled in with Naples Distributed Services Card (DSC) firmware allows customization of:
1. SSHD behavior on boot up and customzation of sshd configuration.
2. SSH keys persistence across reloads of DSC.
3. root login through password authentication.

Content of this file is processed at the boot uptime of DSC and sets the system behavior accordingly.

This is a build time defined file and is not expected to be edited after.
However, if the adminstrator of the system needs to alter the customisation, it can be done by placing override file '/sysconfig/config0/system_boot_config'. This override configuration file will be copied over as 'nic/conf/system_boot_config' during boot up and processed.

1. SSHD behavior on boot up and customzation of it:
---------------------------------------------------
There are three knobs provided that dictate whether to bring up SSHD on boot up, persist or not SSHD configuration across reloads and to allow or not clear text
passwords based authentication with SSH.
    1.    SSHD_BOOT_BEHAVIOR={on|off}
    2.    SSHD_PERSIST={on|off}
    3.    SSHD_PASSWORD_AUTHENTICATION={yes|no}

    SSHD_BOOT_BEHAVIOR
    ------------------
    Setting of 'on' indicates that sshd be started on system boot up, with default
    sshd_config file content.

    This setting solely controls start of sshd in any of the below three cases:
    1. If the system is being booted up for the first of its life.
    2. If 'SSHD_PERSIST' setting is not turned 'on'.
    3. If 'SSHD_PERSIST' is turned 'on' but there is no prior boot up time
       cached sshd state on the system.

    Refer to description of 'SSHD_PERSIST' setting for explanation of sshd 
    configuration cache.

    SSHD_PERSIST
    ------------
    Setting this to 'on' means sshd should retain its running configuration
    ('sshd_config' file content, and the state of sshd being UP or not) across
    reloads of the system.

    Default setting is OFF.
    
    Enabling this setting gets sshd start with default configuration content,
    when system comes up for the first time(assuming SSHD_BOOT_BEHAVIOR is set
    to 'on' as well), caches it as the file '/sysconfig/config0/ssh/sshd_config'
    and uses it across reloads. Seperately sshd run state (i.e sshd is UP or
    stopped) is cached as well.
    On subsequent reloads whole of this cached sshd state is restored.

    With persistence enabled, any customization that system administrator
    needs to make to sshd configuration itself should be done by editing
    this cached file and should re-start sshd or reload the system.

    sshd can be stopped and started or just re-started, on a running system, by:
        /etc/init.d/S50sshd {stop|start|restart}

    SSHD_PASSWORD_AUTHENTICATION
    ----------------------------
    Setting this to 'no' disables clear text passwords acceptance with SSH.
    Set this to 'yes' to allow clear text passwords with SSH.
    This settting maps to the 'sshd_config' file key 'PasswordAuthentication'.

    Note:
    ------
    1. Disabling clear text password through this knob will not allow
    password based authenticaton, even if password based authentication for
    root user is enabled through ROOT_PASSWORD setting.
    2. If ROOT_PASSWORD setting disabled password based authentication, this
    setting (SSHD_PASSWORD_AUTHENTICATION) automatically is interpreted as
    'no' i.e. no clear text password accepted through SSH.


2. SSH keys persistence across reloads of DSC.
----------------------------------------------
This setting determines whether to reuse or not, across DSC reload, SSH public
keys that were loaded into system. The knob is SSH_KEYS_PERSIST.
    SSH_KEYS_PERSIST
    ----------------
    Set this to 'on' if SSH start after system reload should use previously
    uploaded keys.

Content of this file is processed at the boot uptime of DSC and controls the system behavior accordingly.

3. root login through password authentication.
----------------------------------------------
Set the knob ROOT_PASSWORD to 'on' if DSC should allow password based
authentication into root account of the system.


Enabling SSH:
=============
If SSH is disabled on a DSC, it can be enabled through the below procedure.

1. Create customization file to enable SSH on reboot of DSC.
    # cp /nic/conf/system_boot_config /sysconfig/config0/

2. Edit the file contents to look as below.

    # cat /sysconfig/config0/system_boot_config
    SSHD_BOOT_BEHAVIOR=on
    SSHD_PERSIST=off
    SSHD_PASSWORD_AUTHENTICATION=yes
    ROOT_PASSWORD=on
    SSH_KEYS_PERSIST=on
    #

    Note that, with the above configuration example password based SSH authentication is allowed ('SSHD_PASSWORD_AUTHENTICATION=yes') and persistence is turned 'off'. 
    i. If password based authentication is to be turned off, then one has to
       load public key of ssh user into the DSC either through 'penctl'
       command interface or by editing the public key into the file:
       /sysconfig/config0/.authorized_keys
   ii. If persistence is to be turned 'on', then user has to clear off
       any previous cached state, by the below command, before reboot (step 3)
       of this procedure:
       # mv /sysconfig/config0/ssh/sshd_config /sysconfig/config0/ssh/sshd_config.`date+"%Y%m%d%H%M%S"`
3. Reboot the system as below. DSC should come up with SSH enabled.
   # sysreset.sh
