.. _penctl_show_metrics_sonic_hw_ring:

penctl show metrics sonic hw_ring
---------------------------------

Metrics for hardware rings
 Key indices - RId: ring ID, SubRId: sub-ring ID


Ring ID:

  cp     : compression
  cp_hot : compression (priority)
  dc     : decompression
  dc_hot : decompression (priority)
  xts_enc: XTS encryption
  xts_dec: XTS decryption
  gcm_enc: GCM encryption
  gcm_dec: GCM decryption


Value Description:

PIndex     : ring producer index
CIndex     : ring consumer index
InputBytes : total input bytes (not available for cp_hot, dc_hot, xts_enc/dec, gcm_enc/dec)
OutputBytes: total output bytes (not available for cp_hot, dc_hot, xts_enc/dec, gcm_enc/dec)
SoftResets : number of soft resets executed

Synopsis
~~~~~~~~



---------------------------------
 Metrics for hardware rings
 Key indices - RId: ring ID, SubRId: sub-ring ID


Ring ID:

  cp     : compression
  cp_hot : compression (priority)
  dc     : decompression
  dc_hot : decompression (priority)
  xts_enc: XTS encryption
  xts_dec: XTS decryption
  gcm_enc: GCM encryption
  gcm_dec: GCM decryption


Value Description:

PIndex     : ring producer index
CIndex     : ring consumer index
InputBytes : total input bytes (not available for cp_hot, dc_hot, xts_enc/dec, gcm_enc/dec)
OutputBytes: total output bytes (not available for cp_hot, dc_hot, xts_enc/dec, gcm_enc/dec)
SoftResets : number of soft resets executed
---------------------------------


::

  penctl show metrics sonic hw_ring [flags]

Options
~~~~~~~

::

  -h, --help   help for hw_ring

Options inherited from parent commands
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

  -j, --json      display in json format (default true)
      --verbose   display penctl debug log
  -v, --version   display version of penctl
  -y, --yaml      display in yaml format

SEE ALSO
~~~~~~~~

* `penctl show metrics sonic <penctl_show_metrics_sonic.rst>`_ 	 - Metrics for Storage Offload NIC

