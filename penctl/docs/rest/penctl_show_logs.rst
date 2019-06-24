.. _penctl_show_logs:

penctl show logs
----------------

Show logs from Naples

Synopsis
~~~~~~~~



------------------------------
 Show Module Logs From Naples 
------------------------------


::

  penctl show logs [flags]

Options
~~~~~~~

::

  -h, --help            help for logs
  -m, --module string   Module to show logs for
			Valid modules are:
				nmd
				netagent
				tmagent
				pciemgrd


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

* `penctl show <penctl_show.rst>`_ 	 - Show Object and Information

