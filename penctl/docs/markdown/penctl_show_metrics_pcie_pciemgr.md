## penctl show metrics pcie pciemgr



### Synopsis



---------------------------------
 Pcie Mgr Metrics information:
not_intr : notify total intrs.
not_spurious : notify spurious intrs.
not_cnt : notify total txns.
not_max : notify max txns per intr.
not_cfgrd : notify config reads.
not_cfgwr : notify config writes.
not_memrd : notify memory reads.
not_memwr : notify memory writes.
not_iord : notify io reads.
not_iowr : notify io writes.
not_unknown : notify unknown type.
not_rsrv0 : notify rsrv0.
not_rsrv1 : notify rsrv1.
not_msg : notify pcie message.
not_unsupported : notify unsupported.
not_pmv : notify pgm model violation.
not_dbpmv : notify doorbell pmv.
not_atomic : notify atomic trans.
not_pmtmiss : notify PMT miss.
not_pmrmiss : notify PMR miss.
not_prtmiss : notify PRT miss.
not_bdf2vfidmiss : notify bdf2vfid table miss.
not_prtoor : notify PRT out-of-range.
not_vfidoor : notify vfid out-of-range.
not_bdfoor : notify bdf out-of-range.
not_pmrind : notify PMR force indirect.
not_prtind : notify PRT force indirect.
not_pmrecc : notify PMR ECC error.
not_prtecc : notify PRT ECC error.
ind_intr : indirect total intrs.
ind_spurious : indirect spurious intrs.
ind_cfgrd : indirect config reads.
ind_cfgwr : indirect config writes.
ind_memrd : indirect memory reads.
ind_memwr : indirect memory writes.
ind_iord : indirect io reads.
ind_iowr : indirect io writes.
ind_unknown : indirect unknown type.


---------------------------------


```
penctl show metrics pcie pciemgr [flags]
```

### Options

```
  -h, --help   help for pciemgr
```

### Options inherited from parent commands

```
  -a, --authtoken string   path to file containing authorization token
  -j, --json               display in json format (default true)
      --verbose            display penctl debug log
  -v, --version            display version of penctl
  -y, --yaml               display in yaml format
```

### SEE ALSO
* [penctl show metrics pcie](penctl_show_metrics_pcie.md)	 - 

