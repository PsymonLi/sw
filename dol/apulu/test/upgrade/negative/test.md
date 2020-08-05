# Testcases for Upgrade failure functionality

Initial setup:
    a) Configuration:
        On the DUT VPCs, subnets, VNICs, local mappings, remote mappings, route
        tables, tunnels, nexthops, security groups and policies are configured.

## Upgrade Test
    Trigger:
        a) Send a packet

    Expectation:
        a) Check packet is received

    Trigger:
        a) Set Upgrade mode to hitless
        b) Send Upgrade request via gRPC
        c) fail the upgrade at different stages

    Expectation:
        a) Read upgrade status and verify it is FAILED
        b) make sure new instances got deleted

    Trigger:
        a) Send a packet and check it is received as expected

    Expectation:
        a) Check packet is received
