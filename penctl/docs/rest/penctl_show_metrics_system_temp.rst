.. _penctl_show_metrics_system_temp:

penctl show metrics system temp
-------------------------------

System temperature information:


Value Description:

local_temperature: Temperature of the board.
die_temperature: Temperature of the die.
hbm_temperature: Temperature of the hbm.
The temperature is degree Celcius


Synopsis
~~~~~~~~



---------------------------------
 System temperature information:


Value Description:

local_temperature: Temperature of the board.
die_temperature: Temperature of the die.
hbm_temperature: Temperature of the hbm.
The temperature is degree Celcius

---------------------------------


::

  penctl show metrics system temp [flags]

Options
~~~~~~~

::

  -h, --help   help for temp

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

* `penctl show metrics system <penctl_show_metrics_system.rst>`_ 	 - Metrics for system monitors

