## penctl create qos-class

qos-class object

### Synopsis


qos-class object

```
penctl create qos-class [flags]
```

### Options

```
      --dot1q-pcp uint32              Specify pcp value 0-7
      --dscp string                   Specify dscp values 0-63 as --dscp 10,20,30 (default "0")
      --dwrr-bw uint32                Specify DWRR BW percentage (0-100)
  -h, --help                          help for qos-class
      --mtu uint32                    Specify MTU (default 1500)
      --pfc-cos uint32                Specify COS value for PFC
      --pfc-enable                    Enable PFC with default values
      --qosgroup string               Specify qos group. Valid groups: user-defined-1,user-defined-2,user-defined-3,user-defined-4,user-defined-5,user-defined-6
      --strict-priority-rate uint32   Specify strict priority rate in bps
      --xoff-threshold uint32         Specify xoff threshold (2 * mtu to 8 * mtu)
      --xon-threshold uint32          Specify xon threshold (2 * mtu to 4 * mtu)
```

### Options inherited from parent commands

```
  -i, --interface string   ethernet device of naples
  -j, --json               display in json format
  -t, --tabular            display in tabular format (default true)
  -v, --version            display version of penctl
  -y, --yaml               display in yaml format
```

### SEE ALSO
* [penctl create](penctl_create.md)	 - Create Object

