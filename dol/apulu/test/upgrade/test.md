# Testcases for Upgrade functionality

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

    Expectation:
        a) Read upgrade status and verify it is SUCCESS
        b) Wait for config replay notification

    Trigger:
        a) Reconfigure the DUT configurations VPCs, subnets, VNICs, etc...,

    Expectation:
        a) Validate the DUT configurations after config replay

    Trigger:
        a) Send a packet and check it is received as expected

    Expectation:
        a) Check packet is received
