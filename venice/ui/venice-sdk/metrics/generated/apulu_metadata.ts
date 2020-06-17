import { MetricMeasurement } from './metadata';
import { Telemetry_queryMetricsQuerySpec_function } from "@sdk/v1/models/generated/telemetry_query";

export const MetricsMetadataApulu: { [key: string]: MetricMeasurement } = {
  FlowStatsSummary: {
  "name": "FlowStatsSummary",
  "description": "Session Summary",
  "displayName": "Flow Stats Summary",
  "tags": [
    "Level4"
  ],
  "scope": "PerASIC",
  "fields": [
    {
      "name": "TCPSessionsOverIPv4",
      "displayName": "TCP sessions over IPv4",
      "description": "TCP sessions over IPv4",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "UDPSessionsOverIPv4",
      "displayName": "UDP sessions over IPv4",
      "description": "UDP sessions over IPv4",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "ICMPSessionsOverIPv4",
      "displayName": "ICMP sessions over IPv4",
      "description": "ICMP sessions over IPv4",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "OtherSessionsOverIPv4",
      "displayName": "Other sessions over IPv4",
      "description": "Other sessions over IPv4",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TCPSessionsOverIPv6",
      "displayName": "TCP sessions over IPv6",
      "description": "TCP sessions over IPv6",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "UDPSessionsOverIPv6",
      "displayName": "UDP sessions over IPv6",
      "description": "UDP sessions over IPv6",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "ICMPSessionsOverIPv6",
      "displayName": "ICMP sessions over IPv6",
      "description": "ICMP sessions over IPv6",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "OtherSessionsOverIPv6",
      "displayName": "Other sessions over IPv6",
      "description": "Other sessions over IPv6",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "L2Sessions",
      "displayName": "L2 sessions",
      "description": "L2 sessions",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "SessionCreateErrors",
      "displayName": "Session create errors",
      "description": "Session create errors",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "reporterID",
      "description": "Name of reporting object",
      "baseType": "string",
      "jsType": "string",
      "isTag": true,
      "displayName": "reporterID",
      "tags": [
        "Level4"
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    }
  ],
  "objectKind": "DistributedServiceCard"
},
  LifMetrics: {
  "name": "LifMetrics",
  "description": "Logical Interface Metrics",
  "displayName": "Logical Interface Statistics",
  "tags": [
    "Level4"
  ],
  "scope": "PerLIF",
  "fields": [
    {
      "name": "RxUnicastBytes",
      "displayName": "Rx Unicast Bytes",
      "description": "Rx Unicast Bytes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxUnicastPackets",
      "displayName": "Rx Unicast Packets",
      "description": "Rx Unicast Packets",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxMulticastBytes",
      "displayName": "Rx Multicast Bytes",
      "description": "Rx Multicast Bytes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxMulticastPackets",
      "displayName": "Rx Multicast Packets",
      "description": "Rx Multicast Packets",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxBroadcastBytes",
      "displayName": "Rx Broadcast Bytes",
      "description": "Rx Broadcast Bytes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxBroadcastPackets",
      "displayName": "Rx Broadcast Packets",
      "description": "Rx Broadcast Packets",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxDropUnicastBytes",
      "displayName": "Rx Drop Unicast Bytes",
      "description": "Rx Drop Unicast Bytes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxDropUnicastPackets",
      "displayName": "Rx Drop Unicast Packets",
      "description": "Rx Drop Unicast Packets",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxDropMulticastBytes",
      "displayName": "Rx Drop Multicast Bytes",
      "description": "Rx Drop Multicast Bytes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxDropMulticastPackets",
      "displayName": "Rx Drop Multicast Packets",
      "description": "Rx Drop Multicast Packets",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxDropBroadcastBytes",
      "displayName": "Rx Drop Broadcast Bytes",
      "description": "Rx Drop Broadcast Bytes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxDropBroadcastPackets",
      "displayName": "Rx Drop Broadcast Packets",
      "description": "Rx Drop Broadcast Packets",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxUnicastBytes",
      "displayName": "Tx Unicast Bytes",
      "description": "Tx Unicast Bytes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxUnicastPackets",
      "displayName": "Tx Unicast Packets",
      "description": "Tx Unicast Packets",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxMulticastBytes",
      "displayName": "Tx Multicast Bytes",
      "description": "Tx Multicast Bytes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxMulticastPackets",
      "displayName": "Tx Multicast Packets",
      "description": "Tx Multicast Packets",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxBroadcastBytes",
      "displayName": "Tx Broadcast Bytes",
      "description": "Tx Broadcast Bytes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxBroadcastPackets",
      "displayName": "Tx Broadcast Packets",
      "description": "Tx Broadcast Packets",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxDropUnicastBytes",
      "displayName": "Tx Drop Unicast Bytes",
      "description": "Tx Drop Unicast Bytes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxDropUnicastPackets",
      "displayName": "Tx Drop Unicast Packets",
      "description": "Tx Drop Unicast Packets",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxDropMulticastBytes",
      "displayName": "Tx Drop Multicast Bytes",
      "description": "Tx Drop Multicast Bytes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxDropMulticastPackets",
      "displayName": "Tx Drop Multicast Packets",
      "description": "Tx Drop Multicast Packets",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxDropBroadcastBytes",
      "displayName": "Tx Drop Broadcast Bytes",
      "description": "Tx Drop Broadcast Bytes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxDropBroadcastPackets",
      "displayName": "Tx Drop Broadcast Packets",
      "description": "Tx Drop Broadcast Packets",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxPkts",
      "displayName": "Tx Pkts",
      "description": "Total Transmit Packets",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxBytes",
      "displayName": "Tx Bytes",
      "description": "Total Transmit Bytes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxPkts",
      "displayName": "Rx Pkts",
      "description": "Total Receive Packets",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxBytes",
      "displayName": "Rx Bytes",
      "description": "Total Receive Bytes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "reporterID",
      "description": "Name of reporting object",
      "baseType": "string",
      "jsType": "string",
      "isTag": true,
      "displayName": "reporterID",
      "tags": [
        "Level4"
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    }
  ],
  "objectKind": "NetworkInterface",
  "interfaceType": "host-pf"
},
  MacMetrics: {
  "name": "MacMetrics",
  "description": "Uplink Metrics",
  "displayName": "Uplink Interface Packet Statistics",
  "tags": [
    "Level4"
  ],
  "scope": "PerEthPort",
  "fields": [
    {
      "name": "FramesRxOk",
      "displayName": "Rx OK Frames",
      "description": "Frames received OK",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxAll",
      "displayName": "Rx All Frames",
      "description": "Frames Received All (Good and Bad Frames)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxBadFcs",
      "displayName": "Rx Bad FCS Frames",
      "description": "Frames Received with Bad FCS",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxBadAll",
      "displayName": "Rx All Bad Frames",
      "description": "Frames with any bad (CRC, Length, Align)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "OctetsRxOk",
      "displayName": "Rx OK Octets",
      "description": "Octets Received in Good Frames",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "OctetsRxAll",
      "displayName": "Rx All Octets",
      "description": "Octets Received (Good/Bad Frames)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxUnicast",
      "displayName": "Rx Unicast Frames",
      "description": "Frames Received with Unicast Address",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxMulticast",
      "displayName": "Rx Multicast Frames",
      "description": "Frames Received with Multicast Address",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxBroadcast",
      "displayName": "Rx Broadcast Frames",
      "description": "Frames Received with Broadcast Address",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxPause",
      "displayName": "Rx Pause Frames",
      "description": "Frames Received of type PAUSE",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxBadLength",
      "displayName": "Rx Bad Length Frames",
      "description": "Frames Received with Bad Length",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxUndersized",
      "displayName": "Rx Undersized Frames",
      "description": "Frames Received Undersized",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxOversized",
      "displayName": "Rx Oversized Frames",
      "description": "Frames Received Oversized",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxFragments",
      "displayName": "Rx Fragment Frames",
      "description": "Fragments Received",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxJabber",
      "displayName": "Rx Jabber Frames",
      "description": "Jabber Received",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxPripause",
      "displayName": "Rx Priority Pause Frames",
      "description": "Priority Pause Frames Received",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxStompedCrc",
      "displayName": "Rx Stomped CRC Frames",
      "description": "Stomped CRC Frames Received",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxTooLong",
      "displayName": "Rx Too long Frames",
      "description": "Received Frames Too Long",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxVlanGood",
      "displayName": "Rx Good VLAN Frames",
      "description": "Received VLAN Frames (Good)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxDropped",
      "displayName": "Rx Dropped Frames",
      "description": "Received Frames Dropped (Buffer Full)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxLessThan_64B",
      "displayName": "Rx Less than 64b Frames",
      "description": "Frames Received Length less than 64",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRx_64B",
      "displayName": "Rx 64b Frames",
      "description": "Frames Received Length=64",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRx_65B_127B",
      "displayName": "Rx 65b_127b Frames",
      "description": "Frames Received Length=65~127",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRx_128B_255B",
      "displayName": "Rx 128b_255b Frames",
      "description": "Frames Received Length=128~255",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRx_256B_511B",
      "displayName": "Rx 256b_511b Frames",
      "description": "Frames Received Length=256~511",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRx_512B_1023B",
      "displayName": "Rx 512b_1023b Frames",
      "description": "Frames Received Length=512~1023",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRx_1024B_1518B",
      "displayName": "Rx 1024b_1518b Frames",
      "description": "Frames Received Length=1024~1518",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRx_1519B_2047B",
      "displayName": "Rx 1519b_2047b Frames",
      "description": "Frames Received Length=1519~2047",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRx_2048B_4095B",
      "displayName": "Rx 2048b_4095b Frames",
      "description": "Frames Received Length=2048~4095",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRx_4096B_8191B",
      "displayName": "Rx 4096b_8191b Frames",
      "description": "Frames Received Length=4096~8191",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRx_8192B_9215B",
      "displayName": "Rx 8192b_9215b Frames",
      "description": "Frames Received Length=8192~9215",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxOther",
      "displayName": "Rx Other Frames",
      "description": "Frames Received Length greater than 9215",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxOk",
      "displayName": "Tx OK Frames",
      "description": "Frames Transmitted OK",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxAll",
      "displayName": "Tx All Frames",
      "description": "Frames Transmitted All (Good/Bad Frames)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxBad",
      "displayName": "Tx Bad Frames",
      "description": "Frames Transmitted Bad",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "OctetsTxOk",
      "displayName": "Tx OK Octets",
      "description": "Octets Transmitted Good",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "OctetsTxTotal",
      "displayName": "Tx All Octets",
      "description": "Octets Transmitted Total (Good/Bad)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxUnicast",
      "displayName": "Tx Unicast Frames",
      "description": "Frames Transmitted with Unicast Address",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxMulticast",
      "displayName": "Tx Multicast Frames",
      "description": "Frames Transmitted with Multicast Address",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxBroadcast",
      "displayName": "Tx Broadcast Frames",
      "description": "Frames Transmitted with Broadcast Address",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxPause",
      "displayName": "Tx Pause Frames",
      "description": "Frames Transmitted of type PAUSE",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxPripause",
      "displayName": "Tx Priority Pause Frames",
      "description": "Frames Transmitted of type PriPAUSE",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxVlan",
      "displayName": "Tx VLAN Frames",
      "description": "Frames Transmitted VLAN",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxLessThan_64B",
      "displayName": "Tx Less than 64b Frames",
      "description": "Frames Transmitted Length<64",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTx_64B",
      "displayName": "Tx 64b Frames",
      "description": "Frames Transmitted Length=64",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTx_65B_127B",
      "displayName": "Tx 65b_127b Frames",
      "description": "Frames Transmitted Length=65~127",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTx_128B_255B",
      "displayName": "Tx 128b_255b Frames",
      "description": "Frames Transmitted Length=128~255",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTx_256B_511B",
      "displayName": "Tx 256b_511b Frames",
      "description": "Frames Transmitted Length=256~511",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTx_512B_1023B",
      "displayName": "Tx 512b_1023b Frames",
      "description": "Frames Transmitted Length=512~1023",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTx_1024B_1518B",
      "displayName": "Tx 1024b_1518b Frames",
      "description": "Frames Transmitted Length=1024~1518",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTx_1519B_2047B",
      "displayName": "Tx 1519b_2047b Frames",
      "description": "Frames Transmitted Length=1519~2047",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTx_2048B_4095B",
      "displayName": "Tx 2048b_4095b Frames",
      "description": "Frames Transmitted Length=2048~4095",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTx_4096B_8191B",
      "displayName": "Tx 4096b_8191b Frames",
      "description": "Frames Transmitted Length=4096~8191",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTx_8192B_9215B",
      "displayName": "Tx 8192b_9215b Frames",
      "description": "Frames Transmitted Length=8192~9215",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxOther",
      "displayName": "Tx Other Frames",
      "description": "Frames Transmitted Length greater than 9215",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxPri_0",
      "displayName": "Tx Pri0 Frames",
      "description": "Pri#0 Frames Transmitted",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxPri_1",
      "displayName": "Tx Pri1 Frames",
      "description": "Pri#1 Frames Transmitted",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxPri_2",
      "displayName": "Tx Pri2 Frames",
      "description": "Pri#2 Frames Transmitted",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxPri_3",
      "displayName": "Tx Pri3 Frames",
      "description": "Pri#3 Frames Transmitted",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxPri_4",
      "displayName": "Tx Pri4 Frames",
      "description": "Pri#4 Frames Transmitted",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxPri_5",
      "displayName": "Tx Pri5 Frames",
      "description": "Pri#5 Frames Transmitted",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxPri_6",
      "displayName": "Tx Pri6 Frames",
      "description": "Pri#6 Frames Transmitted",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxPri_7",
      "displayName": "Tx Pri7 Frames",
      "description": "Pri#7 Frames Transmitted",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxPri_0",
      "displayName": "Rx Pri0 Frames",
      "description": "Pri#0 Frames Received",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxPri_1",
      "displayName": "Rx Pri1 Frames",
      "description": "Pri#1 Frames Received",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxPri_2",
      "displayName": "Rx Pri2 Frames",
      "description": "Pri#2 Frames Received",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxPri_3",
      "displayName": "Rx Pri3 Frames",
      "description": "Pri#3 Frames Received",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxPri_4",
      "displayName": "Rx Pri4 Frames",
      "description": "Pri#4 Frames Received",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxPri_5",
      "displayName": "Rx Pri5 Frames",
      "description": "Pri#5 Frames Received",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxPri_6",
      "displayName": "Rx Pri6 Frames",
      "description": "Pri#6 Frames Received",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxPri_7",
      "displayName": "Rx Pri7 Frames",
      "description": "Pri#7 Frames Received",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxPripause_0_1UsCount",
      "displayName": "Tx Pri0Pause1US Count",
      "description": "Transmit Pri#0 Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxPripause_1_1UsCount",
      "displayName": "Tx Pri1Pause1US Count",
      "description": "Transmit Pri#1 Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxPripause_2_1UsCount",
      "displayName": "Tx Pri2Pause1US Count",
      "description": "Transmit Pri#2 Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxPripause_3_1UsCount",
      "displayName": "Tx Pri3Pause1US Count",
      "description": "Transmit Pri#3 Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxPripause_4_1UsCount",
      "displayName": "Tx Pri4Pause1US Count",
      "description": "Transmit Pri#4 Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxPripause_5_1UsCount",
      "displayName": "Tx Pri5Pause1US Count",
      "description": "Transmit Pri#5 Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxPripause_6_1UsCount",
      "displayName": "Tx Pri6Pause1US Count",
      "description": "Transmit Pri#6 Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxPripause_7_1UsCount",
      "displayName": "Tx Pri7Pause1US Count",
      "description": "Transmit Pri#7 Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxPripause_0_1UsCount",
      "displayName": "Rx Pri0Pause1US Count",
      "description": "Receive Pri#0 Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxPripause_1_1UsCount",
      "displayName": "Rx Pri1Pause1US Count",
      "description": "Receive Pri#1 Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxPripause_2_1UsCount",
      "displayName": "Rx Pri2Pause1US Count",
      "description": "Receive Pri#2 Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxPripause_3_1UsCount",
      "displayName": "Rx Pri3Pause1US Count",
      "description": "Receive Pri#3 Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxPripause_4_1UsCount",
      "displayName": "Rx Pri4Pause1US Count",
      "description": "Receive Pri#4 Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxPripause_5_1UsCount",
      "displayName": "Rx Pri5Pause1US Count",
      "description": "Receive Pri#5 Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxPripause_6_1UsCount",
      "displayName": "Rx Pri6Pause1US Count",
      "description": "Receive Pri#6 Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxPripause_7_1UsCount",
      "displayName": "Rx Pri7Pause1US Count",
      "description": "Receive Pri#7 Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxPause_1UsCount",
      "displayName": "Rx Pause1US Count",
      "description": "Receive Standard Pause 1US Count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxTruncated",
      "displayName": "Tx Truncated Frames",
      "description": "Frames Truncated",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "reporterID",
      "description": "Name of reporting object",
      "baseType": "string",
      "jsType": "string",
      "isTag": true,
      "displayName": "reporterID",
      "tags": [
        "Level4"
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    }
  ],
  "objectKind": "NetworkInterface",
  "interfaceType": "uplink-eth"
},
  MgmtMacMetrics: {
  "name": "MgmtMacMetrics",
  "description": "Management Interface Metrics",
  "displayName": "Management Interface Packet Statistics",
  "tags": [
    "Level4"
  ],
  "scope": "PerMgmtPort",
  "fields": [
    {
      "name": "FramesRxOk",
      "displayName": "Rx OK Frames",
      "description": "Frames received OK",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxAll",
      "displayName": "Rx All Frames",
      "description": "Frames Received All (Good and Bad Frames)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxBadFcs",
      "displayName": "Rx Bad FCS Frames",
      "description": "Frames Received with Bad FCS",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxBadAll",
      "displayName": "Rx All Bad Frames",
      "description": "Frames with any bad (CRC, Length, Align)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "OctetsRxOk",
      "displayName": "Rx OK Octets",
      "description": "Octets Received in Good Frames",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "OctetsRxAll",
      "displayName": "Rx All Octets",
      "description": "Octets Received (Good/Bad Frames)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxUnicast",
      "displayName": "Rx Unicast Frames",
      "description": "Frames Received with Unicast Address",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxMulticast",
      "displayName": "Rx Multicast Frames",
      "description": "Frames Received with Multicast Address",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxBroadcast",
      "displayName": "Rx Broadcast Frames",
      "description": "Frames Received with Broadcast Address",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxPause",
      "displayName": "Rx Pause Frames",
      "description": "Frames Received of type PAUSE",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxBadLength",
      "displayName": "Rx Bad Length Frames",
      "description": "Frames Received with Bad Length",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxUndersized",
      "displayName": "Rx Undersized Frames",
      "description": "Frames Received Undersized",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxOversized",
      "displayName": "Rx Oversized Frames",
      "description": "Frames Received Oversized",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxFragments",
      "displayName": "Rx Fragment Frames",
      "description": "Fragments Received",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxJabber",
      "displayName": "Rx Jabber Frames",
      "description": "Jabber Received",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRx_64B",
      "displayName": "Rx 64b Frames",
      "description": "Frames Received Length=64",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRx_65B_127B",
      "displayName": "Rx 65b_127b Frames",
      "description": "Frames Received Length=65~127",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRx_128B_255B",
      "displayName": "Rx 128b_255b Frames",
      "description": "Frames Received Length=128~255",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRx_256B_511B",
      "displayName": "Rx 256b_511b Frames",
      "description": "Frames Received Length=256~511",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRx_512B_1023B",
      "displayName": "Rx 512b_1023b Frames",
      "description": "Frames Received Length=512~1023",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRx_1024B_1518B",
      "displayName": "Rx 1024b_1518b Frames",
      "description": "Frames Received Length=1024~1518",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxGt_1518B",
      "displayName": "Rx Greater than 1518b Frames",
      "description": "Frames Received Length greater than 1518",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesRxFifoFull",
      "displayName": "Rx FIFO Full Frames",
      "description": "Frames Received FIFO Full",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxOk",
      "displayName": "Tx OK Frames",
      "description": "Frames Transmitted OK",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxAll",
      "displayName": "Tx All Frames",
      "description": "Frames Transmitted All (Good/Bad Frames)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxBad",
      "displayName": "Tx Bad Frames",
      "description": "Frames Transmitted Bad",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "OctetsTxOk",
      "displayName": "Tx OK Octets",
      "description": "Octets Transmitted Good",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "OctetsTxTotal",
      "displayName": "Tx All Octets",
      "description": "Octets Transmitted Total (Good/Bad)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxUnicast",
      "displayName": "Tx Unicast Frames",
      "description": "Frames Transmitted with Unicast Address",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxMulticast",
      "displayName": "Tx Multicast Frames",
      "description": "Frames Transmitted with Multicast Address",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxBroadcast",
      "displayName": "Tx Broadcast Frames",
      "description": "Frames Transmitted with Broadcast Address",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FramesTxPause",
      "displayName": "Tx Pause Frames",
      "description": "Frames Transmitted of type PAUSE",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "reporterID",
      "description": "Name of reporting object",
      "baseType": "string",
      "jsType": "string",
      "isTag": true,
      "displayName": "reporterID",
      "tags": [
        "Level4"
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    }
  ],
  "objectKind": "NetworkInterface",
  "interfaceType": "uplink-mgmt"
},
  PcieMgrMetrics: {
  "name": "PcieMgrMetrics",
  "description": "pcie port metrics",
  "displayName": "PCIe Manager information",
  "tags": [
    "Level7"
  ],
  "scope": "PerPciePort",
  "fields": [
    {
      "name": "NotIntr",
      "displayName": "not_intr",
      "description": "notify total intrs",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotSpurious",
      "displayName": "not_spurious",
      "description": "notify spurious intrs",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotCnt",
      "displayName": "not_cnt",
      "description": "notify total txns",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotMax",
      "displayName": "not_max",
      "description": "notify max txns per intr",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotCfgrd",
      "displayName": "not_cfgrd",
      "description": "notify config reads",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotCfgwr",
      "displayName": "not_cfgwr",
      "description": "notify config writes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotMemrd",
      "displayName": "not_memrd",
      "description": "notify memory reads",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotMemwr",
      "displayName": "not_memwr",
      "description": "notify memory writes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotIord",
      "displayName": "not_iord",
      "description": "notify io reads",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotIowr",
      "displayName": "not_iowr",
      "description": "notify io writes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotUnknown",
      "displayName": "not_unknown",
      "description": "notify unknown type",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotRsrv0",
      "displayName": "not_rsrv0",
      "description": "notify rsrv0",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotRsrv1",
      "displayName": "not_rsrv1",
      "description": "notify rsrv1",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotMsg",
      "displayName": "not_msg",
      "description": "notify pcie message",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotUnsupported",
      "displayName": "not_unsupported",
      "description": "notify unsupported",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotPmv",
      "displayName": "not_pmv",
      "description": "notify pgm model violation",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotDbpmv",
      "displayName": "not_dbpmv",
      "description": "notify doorbell pmv",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotAtomic",
      "displayName": "not_atomic",
      "description": "notify atomic trans",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotPmtmiss",
      "displayName": "not_pmtmiss",
      "description": "notify PMT miss",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotPmrmiss",
      "displayName": "not_pmrmiss",
      "description": "notify PMR miss",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotPrtmiss",
      "displayName": "not_prtmiss",
      "description": "notify PRT miss",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotBdf2Vfidmiss",
      "displayName": "not_bdf2vfidmiss",
      "description": "notify bdf2vfid table miss",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotPrtoor",
      "displayName": "not_prtoor",
      "description": "notify PRT out-of-range",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotVfidoor",
      "displayName": "not_vfidoor",
      "description": "notify vfid out-of-range",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotBdfoor",
      "displayName": "not_bdfoor",
      "description": "notify bdf out-of-range",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotPmrind",
      "displayName": "not_pmrind",
      "description": "notify PMR force indirect",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotPrtind",
      "displayName": "not_prtind",
      "description": "notify PRT force indirect",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotPmrecc",
      "displayName": "not_pmrecc",
      "description": "notify PMR ECC error",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NotPrtecc",
      "displayName": "not_prtecc",
      "description": "notify PRT ECC error",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IndIntr",
      "displayName": "ind_intr",
      "description": "indirect total intrs",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IndSpurious",
      "displayName": "ind_spurious",
      "description": "indirect spurious intrs",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IndCfgrd",
      "displayName": "ind_cfgrd",
      "description": "indirect config reads",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IndCfgwr",
      "displayName": "ind_cfgwr",
      "description": "indirect config writes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IndMemrd",
      "displayName": "ind_memrd",
      "description": "indirect memory reads",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IndMemwr",
      "displayName": "ind_memwr",
      "description": "indirect memory writes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IndIord",
      "displayName": "ind_iord",
      "description": "indirect io reads",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IndIowr",
      "displayName": "ind_iowr",
      "description": "indirect io writes",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IndUnknown",
      "displayName": "ind_unknown",
      "description": "indirect unknown type",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Healthlog",
      "displayName": "healthlog",
      "description": "health log events",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "reporterID",
      "description": "Name of reporting object",
      "baseType": "string",
      "jsType": "string",
      "isTag": true,
      "displayName": "reporterID",
      "tags": [
        "Level4"
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    }
  ],
  "objectKind": "DistributedServiceCard"
},
  PciePortMetrics: {
  "name": "PciePortMetrics",
  "description": "Key index - pcie port",
  "displayName": "PCIe port information",
  "tags": [
    "Level7"
  ],
  "scope": "PerPciePort",
  "fields": [
    {
      "name": "IntrTotal",
      "displayName": "intr_total",
      "description": "total port intrs",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IntrPolled",
      "displayName": "intr_polled",
      "description": "total port intrs polled",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IntrPerstn",
      "displayName": "intr_perstn",
      "description": "pcie out of reset",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IntrLtssmstEarly",
      "displayName": "intr_ltssmst_early",
      "description": "link train before linkup",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IntrLtssmst",
      "displayName": "intr_ltssmst",
      "description": "link train after  linkup",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IntrLinkup2Dn",
      "displayName": "intr_linkup2dn",
      "description": "link down",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IntrLinkdn2Up",
      "displayName": "intr_linkdn2up",
      "description": "link up",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IntrRstup2Dn",
      "displayName": "intr_rstup2dn",
      "description": "mac up",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IntrRstdn2Up",
      "displayName": "intr_rstdn2up",
      "description": "mac down",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IntrSecbus",
      "displayName": "intr_secbus",
      "description": "secondary bus set",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Linkup",
      "displayName": "linkup",
      "description": "link is up",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Hostup",
      "displayName": "hostup",
      "description": "host is up (secbus)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Phypolllast",
      "displayName": "phypolllast",
      "description": "phy poll count (last)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Phypollmax",
      "displayName": "phypollmax",
      "description": "phy poll count (max)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Phypollperstn",
      "displayName": "phypollperstn",
      "description": "phy poll lost perstn",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Phypollfail",
      "displayName": "phypollfail",
      "description": "phy poll failed",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Gatepolllast",
      "displayName": "gatepolllast",
      "description": "gate poll count (last)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Gatepollmax",
      "displayName": "gatepollmax",
      "description": "gate poll count (max)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Markerpolllast",
      "displayName": "markerpolllast",
      "description": "marker poll count (last)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Markerpollmax",
      "displayName": "markerpollmax",
      "description": "marker poll count (max)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Axipendpolllast",
      "displayName": "axipendpolllast",
      "description": "axipend poll count (last)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Axipendpollmax",
      "displayName": "axipendpollmax",
      "description": "axipend poll count (max)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Faults",
      "displayName": "faults",
      "description": "link faults",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Powerdown",
      "displayName": "powerdown",
      "description": "powerdown count",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "LinkDn2UpInt",
      "displayName": "link_dn2up_int",
      "description": "link_dn2up_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "LinkUp2DnInt",
      "displayName": "link_up2dn_int",
      "description": "link_up2dn_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "SecBusRstInt",
      "displayName": "sec_bus_rst_int",
      "description": "sec_bus_rst_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RstUp2DnInt",
      "displayName": "rst_up2dn_int",
      "description": "rst_up2dn_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RstDn2UpInt",
      "displayName": "rst_dn2up_int",
      "description": "rst_dn2up_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "PortgateOpen2CloseInt",
      "displayName": "portgate_open2close_int",
      "description": "portgate_open2close_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "LtssmStChangedInt",
      "displayName": "ltssm_st_changed_int",
      "description": "ltssm_st_changed_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "SecBusnumChangedInt",
      "displayName": "sec_busnum_changed_int",
      "description": "sec_busnum_changed_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RcPmeInt",
      "displayName": "rc_pme_int",
      "description": "rc_pme_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RcAerrInt",
      "displayName": "rc_aerr_int",
      "description": "rc_aerr_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RcSerrInt",
      "displayName": "rc_serr_int",
      "description": "rc_serr_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RcHpeInt",
      "displayName": "rc_hpe_int",
      "description": "rc_hpe_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RcEqReqInt",
      "displayName": "rc_eq_req_int",
      "description": "rc_eq_req_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RcDpcInt",
      "displayName": "rc_dpc_int",
      "description": "rc_dpc_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "PmTurnoffInt",
      "displayName": "pm_turnoff_int",
      "description": "pm_turnoff_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxbfrOverflowInt",
      "displayName": "txbfr_overflow_int",
      "description": "txbfr_overflow_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxtlpErrInt",
      "displayName": "rxtlp_err_int",
      "description": "rxtlp_err_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TlFlrReqInt",
      "displayName": "tl_flr_req_int",
      "description": "tl_flr_req_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RcLegacyIntpinChangedInt",
      "displayName": "rc_legacy_intpin_changed_int",
      "description": "rc_legacy_intpin_changed_int",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "PpsdSbeInterrupt",
      "displayName": "ppsd_sbe_interrupt",
      "description": "ppsd_sbe_interrupt",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "PpsdDbeInterrupt",
      "displayName": "ppsd_dbe_interrupt",
      "description": "ppsd_dbe_interrupt",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "SbusErrInterrupt",
      "displayName": "sbus_err_interrupt",
      "description": "sbus_err_interrupt",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "PoweronRetries",
      "displayName": "poweron_retries",
      "description": "poweron_retries",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "reporterID",
      "description": "Name of reporting object",
      "baseType": "string",
      "jsType": "string",
      "isTag": true,
      "displayName": "reporterID",
      "tags": [
        "Level4"
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    }
  ],
  "objectKind": "DistributedServiceCard"
},
  MemoryMetrics: {
  "name": "MemoryMetrics",
  "description": "System Memory",
  "displayName": "System Memory",
  "tags": [
    "Level7"
  ],
  "scope": "PerASIC",
  "fields": [
    {
      "name": "Totalmemory",
      "displayName": "Total memory",
      "description": "Total memory of the system",
      "units": "KB",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Availablememory",
      "displayName": "Available memory",
      "description": "Available memory of the system",
      "units": "KB",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Freememory",
      "displayName": "Free memory",
      "description": "Free memory of the system",
      "units": "KB",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "reporterID",
      "description": "Name of reporting object",
      "baseType": "string",
      "jsType": "string",
      "isTag": true,
      "displayName": "reporterID",
      "tags": [
        "Level4"
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    }
  ],
  "objectKind": "DistributedServiceCard"
},
  PowerMetrics: {
  "name": "PowerMetrics",
  "description": "Asic Power",
  "displayName": "Asic Power",
  "tags": [
    "Level7"
  ],
  "scope": "PerASIC",
  "fields": [
    {
      "name": "Pin",
      "displayName": "Input Power",
      "description": "Input power to the system",
      "units": "MilliWatts",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Pout1",
      "displayName": "Core Output Power",
      "description": "Core output power",
      "units": "MilliWatts",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "Pout2",
      "displayName": "ARM Output Power",
      "description": "ARM output power",
      "units": "MilliWatts",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "reporterID",
      "description": "Name of reporting object",
      "baseType": "string",
      "jsType": "string",
      "isTag": true,
      "displayName": "reporterID",
      "tags": [
        "Level4"
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    }
  ],
  "objectKind": "DistributedServiceCard"
},
  AsicTemperatureMetrics: {
  "name": "AsicTemperatureMetrics",
  "description": "Asic Temperature",
  "displayName": "Asic Temperature",
  "tags": [
    "Level7"
  ],
  "scope": "PerASIC",
  "fields": [
    {
      "name": "LocalTemperature",
      "displayName": "Local Temperature",
      "description": "Temperature of the board in celsius",
      "units": "Celsius",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DieTemperature",
      "displayName": "Die Temperature",
      "description": "Temperature of the die in celsius",
      "units": "Celsius",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "HbmTemperature",
      "displayName": "HBM Temperature",
      "description": "Temperature of the HBM in celsius",
      "units": "Celsius",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "reporterID",
      "description": "Name of reporting object",
      "baseType": "string",
      "jsType": "string",
      "isTag": true,
      "displayName": "reporterID",
      "tags": [
        "Level4"
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    }
  ],
  "objectKind": "DistributedServiceCard"
},
  PortTemperatureMetrics: {
  "name": "PortTemperatureMetrics",
  "description": "Port Transceiver Temperature",
  "displayName": "Port Transceiver Temperature",
  "tags": [
    "Level7"
  ],
  "scope": "PerPort",
  "fields": [
    {
      "name": "Temperature",
      "displayName": "Transceiver temperature",
      "description": "Transceiver temperature in celsius",
      "units": "Celsius",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "WarningTemperature",
      "displayName": "Transceiver warning temperature",
      "description": "Transceiver warning temperature in celsius",
      "units": "Celsius",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "AlarmTemperature",
      "displayName": "Transceiver alarm temperature",
      "description": "Transceiver alarm temperature in celsius",
      "units": "Celsius",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "reporterID",
      "description": "Name of reporting object",
      "baseType": "string",
      "jsType": "string",
      "isTag": true,
      "displayName": "reporterID",
      "tags": [
        "Level4"
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    }
  ],
  "objectKind": "DistributedServiceCard"
},
  Node: {
  "name": "Node",
  "objectKind": "Node",
  "displayName": "Node",
  "description": "Contains metrics reported from the Venice Nodes",
  "tags": [
    "Level7"
  ],
  "fields": [
    {
      "name": "CPUUsedPercent",
      "displayName": "Percent CPU Used",
      "description": "CPU usage (percent) ",
      "units": "Percent",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0,
      "scaleMax": 100
    },
    {
      "name": "DiskFree",
      "displayName": "Disk Free",
      "description": "Disk Free in bytes",
      "units": "Bytes",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0
    },
    {
      "name": "DiskTotal",
      "displayName": "Total Disk Space",
      "description": "Total Disk Space in bytes",
      "units": "Bytes",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0
    },
    {
      "name": "DiskUsed",
      "displayName": "Disk Used",
      "description": "Disk Used in bytes",
      "units": "Bytes",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0
    },
    {
      "name": "DiskUsedPercent",
      "displayName": "Percent disk Used",
      "description": "Disk usage (percent) ",
      "units": "Percent",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0,
      "scaleMax": 100
    },
    {
      "name": "InterfaceRxBytes",
      "displayName": "Interface Rx",
      "description": "Interface Rx in bytes",
      "units": "Bytes",
      "baseType": "counter",
      "jsType": "number",
      "scaleMin": 0
    },
    {
      "name": "InterfaceTxBytes",
      "displayName": "Interface Tx",
      "description": "Interface Tx in bytes",
      "units": "Bytes",
      "baseType": "counter",
      "jsType": "number",
      "scaleMin": 0
    },
    {
      "name": "MemFree",
      "displayName": "Memory Free",
      "description": "Memory Free in bytes",
      "units": "Bytes",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0
    },
    {
      "name": "MemTotal",
      "displayName": "Total Memory Space",
      "description": "Total Memory Space in bytes",
      "units": "Bytes",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0
    },
    {
      "name": "MemUsed",
      "displayName": "Memory Used",
      "description": "Memory Used in bytes",
      "units": "Bytes",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0
    },
    {
      "name": "MemUsedPercent",
      "displayName": "Percent Memory Used",
      "description": "Memory usage (percent) ",
      "units": "Percent",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0,
      "scaleMax": 100
    },
    {
      "name": "reporterID",
      "displayName": "Node",
      "description": "Name of reporting node",
      "baseType": "string",
      "jsType": "string",
      "isTag": true
    }
  ]
},
  DistributedServiceCard: {
  "name": "DistributedServiceCard",
  "objectKind": "DistributedServiceCard",
  "displayName": "DSC",
  "description": "Contains metrics reported from the DSC",
  "tags": [
    "Level7"
  ],
  "fields": [
    {
      "name": "CPUUsedPercent",
      "displayName": "Percent CPU Used",
      "description": "CPU usage (percent) ",
      "units": "Percent",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0,
      "scaleMax": 100
    },
    {
      "name": "DiskFree",
      "displayName": "Disk Free",
      "description": "Disk Free in bytes",
      "units": "Bytes",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0
    },
    {
      "name": "DiskTotal",
      "displayName": "Total Disk Space",
      "description": "Total Disk Space in bytes",
      "units": "Bytes",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0
    },
    {
      "name": "DiskUsed",
      "displayName": "Disk Used",
      "description": "Disk Used in bytes",
      "units": "Bytes",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0
    },
    {
      "name": "DiskUsedPercent",
      "displayName": "Percent Disk Used",
      "description": "Disk usage (percent) ",
      "units": "Percent",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0,
      "scaleMax": 100
    },
    {
      "name": "InterfaceRxBytes",
      "displayName": "Interface Rx",
      "description": "Interface Rx in bytes",
      "units": "Bytes",
      "baseType": "counter",
      "jsType": "number",
      "scaleMin": 0
    },
    {
      "name": "InterfaceTxBytes",
      "displayName": "Interface Tx",
      "description": "Interface Tx in bytes",
      "units": "Bytes",
      "baseType": "counter",
      "jsType": "number",
      "scaleMin": 0
    },
    {
      "name": "MemFree",
      "displayName": "Memory Free",
      "description": "Memory Free in bytes",
      "units": "Bytes",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0
    },
    {
      "name": "MemTotal",
      "displayName": "Total Memory Space",
      "description": "Total Memory Space in bytes",
      "units": "Bytes",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0
    },
    {
      "name": "MemUsed",
      "displayName": "Memory Used",
      "description": "Memory Used in bytes",
      "units": "Bytes",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0
    },
    {
      "name": "MemUsedPercent",
      "displayName": "Percent Memory Used",
      "units": "Percent",
      "description": "Memory usage (percent) ",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0,
      "scaleMax": 100
    },
    {
      "name": "reporterID",
      "displayName": "DSC",
      "description": "Name of reporting DSC",
      "baseType": "string",
      "jsType": "string",
      "isTag": true
    }
  ]
},
  Cluster: {
  "name": "Cluster",
  "objectKind": "Cluster",
  "displayName": "Cluster",
  "description": "Contains metrics related to the cluster",
  "tags": [
    "Level6",
    "SingleReporter"
  ],
  "fields": [
    {
      "name": "AdmittedNICs",
      "displayName": "Admitted DSCs",
      "description": "Number of admitted DSCs",
      "units": "count",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0,
      "tags": [
        "Level1"
      ]
    },
    {
      "name": "DecommissionedNICs",
      "displayName": "Decomissioned DSCs",
      "description": "Number of decommissioned DSCs",
      "units": "count",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0,
      "tags": [
        "Level1"
      ]
    },
    {
      "name": "DisconnectedNICs",
      "displayName": "Disconnected DSCs",
      "description": "Number of disconnected DSCs",
      "units": "count",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0,
      "tags": [
        "Level2"
      ]
    },
    {
      "name": "HealthyNICs",
      "displayName": "Healthy DSCs",
      "description": "Number of healthy DSCs",
      "units": "count",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0,
      "tags": [
        "Level2"
      ]
    },
    {
      "name": "PendingNICs",
      "displayName": "Pending DSCs",
      "description": "Number of pending DSCs",
      "units": "count",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0,
      "tags": [
        "Level1"
      ]
    },
    {
      "name": "RejectedNICs",
      "displayName": "Rejected DSCs",
      "description": "Number of rejected DSCs",
      "units": "count",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0,
      "tags": [
        "Level1"
      ]
    },
    {
      "name": "UnhealthyNICs",
      "displayName": "Unhealthy DSCs",
      "description": "Number of unhealthy DSCs",
      "units": "count",
      "baseType": "gauge",
      "jsType": "number",
      "scaleMin": 0,
      "tags": [
        "Level2"
      ]
    }
  ]
},
}