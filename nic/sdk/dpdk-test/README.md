# Test Suite

## DPDK Test Suite

This guide contains instructions for running the Data Plane Development Kit Test Suite (DTS).

The Tester machine is equipped with an adapter (Intel 10G) that will be used for running the test. The DUT machine is equipped with the target adapter (Pensando DSC). The Intel adapter has 2 interfaces directly connected to the 2 interfaces of the Pensando DSC.

## Long Story Short

1. Acquire a Tester and DUT, directly connected. DTS will not function if there is a switch between the tester and DUT. You have been warned.
2. Disable IOMMU on both hosts if using igb_uio (igb_uio does not support IOMMU), OR enable IOMMU on the DUT if using vfio-pci.
3. Copy penctl.linux and penctl.token to the DUT.

4. From a dev server:
```
sw-dev1:~# cd dpdk-test
sw-dev1:~/dpdk-test# ./setup.sh <dut-hostname> <tester-hostname>
```

5. On the Tester:
```
tester:~# cd ~/dpdk-test
tester:~/dpdk-test# emacs pen-conf/execution.cfg   # to configure which driver and tests to run
tester:~/dpdk-test# sudo ./run.sh
```

## Full Instructions

### System Tuning

For best performance:

- Enable HPET on both Tester and DUT (Optional) from BIOS -> Advanced -> PCH-IO Configuration -> High Precision Timer
- Disable C3/C6 on the DUT for best performance (Optional) from
	BIOS -> Advanced -> Processor Configuration -> Processor C3
	BIOS -> Advanced -> Processor Configuration -> Processor C6

It is not clear if these suggestions apply to the Cisco UCS testbeds we are using.

### DTS Configuration

The configuration files are in pen-conf/. These files are copied to the dts/ folder by run.sh every time DTS is run, so don't edit the versions in dts/ directly.

### Run the Test Suite

```
tester:~# cd ~/dpdk-test
tester:~/dpdk-test# ./run.sh
```

## Common errors

If you enabled dpdk on Tester interfaces, make sure that the interfaces on Tester are using the kernel driver before running DTS:

```
tester:~# cd ~/dpdk-test/dpdk
tester:~/dpdk-test/dpdk# ./usertools/dpdk-devbind.py --bind=ixgbe 0000:01:00.0
tester:~/dpdk-test/dpdk# ./usertools/dpdk-devbind.py --bind=ixgbe 0000:01:00.1
```

If you want to run the DPDK example apps outside of DTS on the DUT, make sure to bind the DSC devices to igb_uio.

```
dut:~# cd ~/dpdk
dut:~/dpdk# ./usertools/dpdk-devbind.py --force --bind=igb_uio 0000:b5:00.0 0000:b6:00.0
```
