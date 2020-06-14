---
id: firewallprofiles
---
## Firewall Profiles

The distributed firewall uses user-imposed session limits and timeouts, to guard against brute-force TCP/UDP/ICMP flooding attacks or DDOS attacks. When the open/active session limit is exceeded, a syslog alert is generated, and any subsequent session establishment requests are dropped in the data-plane thus implicitly protecting the workload and host from flooding attacks. 

The Firewall profile comprise of three sections -

1. Details

1. Specifications (Specs)

1. Status

###Details section:

<load-table group:security obj:ApiObjectMeta
            include:name >
<load-table group:security obj:ApiObjectMeta
            include:tenant omitHeader:true>
<load-table group:security obj:ApiObjectMeta
            include:mod-time omitHeader:true>
<load-table group:security obj:ApiObjectMeta
            include:creation-time omitHeader:true>


### Specifications or Specs Section:

| Timeouts and Limits | Default value |
| ------------- | ------------- |
| Session Idle Timeout | 90s |
| TCP Connection Setup Timeout | 30s |
| TCP Close Timeout | 15s |
| TCP Half Closed Timeout | 120s |
| TCP Drop Timeout | 90s |
| UDP Drop Timeout | 60s |
| ICMP Drop Timeout | 60s |
| Drop Timeout | 60s |
| TCP Timeout | 3600s |
| UDP Timeout | 30s |
| ICMP Timeout | 6h |
| TCP Half Open Session Limit | disabled |
| UDP Active Session Limit | disabled |
| ICMP Active Session Limit | disabled |

<load-table group:security obj:SecurityFirewallProfileSpec
            include:session-idle-timeout >
<load-table group:security obj:SecurityFirewallProfileSpec
            include:tcp-connection-setup-timeout omitHeader:true>
<load-table group:security obj:SecurityFirewallProfileSpec
            include:tcp-close-timeout omitHeader:true>
<load-table group:security obj:SecurityFirewallProfileSpec
            include:tcp-half-closed-timeout omitHeader:true>
<load-table group:security obj:SecurityFirewallProfileSpec
            include:tcp-drop-timeout omitHeader:true>
<load-table group:security obj:SecurityFirewallProfileSpec
            include:udp-drop-timeout omitHeader:true>
<load-table group:security obj:SecurityFirewallProfileSpec
            include:icmp-drop-timeout omitHeader:true>
<load-table group:security obj:SecurityFirewallProfileSpec
            include:drop-timeout omitHeader:true>
<load-table group:security obj:SecurityFirewallProfileSpec
            include:tcp-timeout omitHeader:true>
<load-table group:security obj:SecurityFirewallProfileSpec
            include:udp-timeout omitHeader:true>
<load-table group:security obj:SecurityFirewallProfileSpec
            include:icmp-timeout omitHeader:true>
<load-table group:security obj:SecurityFirewallProfileSpec
            include:tcp-half-open-session-limit omitHeader:true>
<load-table group:security obj:SecurityFirewallProfileSpec
            include:udp-active-session-limit omitHeader:true>
<load-table group:security obj:SecurityFirewallProfileSpec
            include:icmp-active-session-limit omitHeader:true>


### Status section:

Propagation section:
Complete or Pending


The Update button on right hand top corner in the firewall profile provides the flexibility to adjust the timeouts and limits to values that are based upon individual customers' security requirements and environment. After modification, click on "Save Firewall profile" to save the settings. Use caution to change the default values.


