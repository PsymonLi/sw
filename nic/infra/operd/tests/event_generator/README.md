# Event Generator

    Test binary used to generate all types of supported event along with
    the following options

         -r event generation rate per second [default: 1 event per sec]
         -n total number of event to be generated  [default: 1 event]
         -t type of event to be generated [default: -1 (random type)]
         -p prefix for event messages [default: 'TEST_ALERT_GEN']

## How to run

### On DOL

    OPERD_REGIONS=/sw/nic/conf/apulu/operd-regions.json /sw/nic/build/x86_64/apulu/capri/bin/event_gen -t 3 -n 10 -r 10

### On SIM

    OPERD_REGIONS=/naples/nic/conf/operd-regions.json /naples/nic/bin/event_gen -t 3 -n 10 -r 10

### On HW

    Being a testapp, It is not packaged in HW but it should work
    <copied_path>/event_gen -t 3 -n 10 -r 10

