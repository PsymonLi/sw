
export HAL_CONFIG_PATH=/nic/conf/
export LD_LIBRARY_PATH=/nic/lib:/nic/conf/sdk:/nic/conf/linkmgr/:/nic/conf/sdk/external:/usr/local/lib:/usr/lib/aarch64-linux-gnu:$LD_LIBRARY_PATH
export HAL_PBC_INIT_CONFIG="2x100_hbm"

ulimit -c unlimited

$GDB /nic/bin/hal -c hal_hostpin.json
