# Flow logs generator

    Test binary used to generate flow logs along with
    the following options

         -r flow log generation rate per second [default: 100 per sec]
         -n total number of flow logs to be generated  [default: 1000]

## How to run

### On DOL

    OPERD_REGIONS=/sw/nic/conf/apulu/operd-regions.json /sw/nic/build/x86_64/apulu/capri/bin/flowlog_gen -r 1 -n 5

### On SIM

    OPERD_REGIONS=/naples/nic/conf/operd-regions.json /naples/nic/bin/flowlog_gen  -r 100 -n 500

### On HW

    Being a testapp, It is not packaged in HW but it should work
    <copied_path>/flowlog_gen -r 100 -n 1000

