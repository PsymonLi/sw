.. _penctl_update_dsc-profile:

penctl update dsc-profile
-------------------------

Distributed Service Card profile object

Synopsis
~~~~~~~~



----------------------------
 Update Distributed Service Card Profiles 
----------------------------


::

  penctl update dsc-profile [flags]

Options
~~~~~~~

::

  -h, --help                  help for dsc-profile
  -n, --name string           Name of the Distributed Service Card profile to be created
  -i, --num-lifs int32        Maximum number of LIFs on the eth device. 1 or 16 (default 1)
  -p, --port-default string   Set default port admin state for next reboot. (enable | disable) (default "enable")

Options inherited from parent commands
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

  -a, --authtoken string   path to file containing authorization token
  -j, --json               display in json format (default true)
      --verbose            display penctl debug log
  -v, --version            display version of penctl
  -y, --yaml               display in yaml format

SEE ALSO
~~~~~~~~

* `penctl update <penctl_update.rst>`_ 	 - Update Object

