import { MetricMeasurement } from './metadata';

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
      "name": "TCP sessions over IPv4",
      "displayName": "TCP sessions over IPv4",
      "description": "TCP sessions over IPv4",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number"
    },
    {
      "name": "UDP sessions over IPv4",
      "displayName": "UDP sessions over IPv4",
      "description": "UDP sessions over IPv4",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number"
    },
    {
      "name": "ICMP sessions over IPv4",
      "displayName": "ICMP sessions over IPv4",
      "description": "ICMP sessions over IPv4",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number"
    },
    {
      "name": "Other sessions over IPv4",
      "displayName": "Other sessions over IPv4",
      "description": "Other sessions over IPv4",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number"
    },
    {
      "name": "TCP sessions over IPv6",
      "displayName": "TCP sessions over IPv6",
      "description": "TCP sessions over IPv6",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number"
    },
    {
      "name": "UDP sessions over IPv6",
      "displayName": "UDP sessions over IPv6",
      "description": "UDP sessions over IPv6",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number"
    },
    {
      "name": "ICMP sessions over IPv6",
      "displayName": "ICMP sessions over IPv6",
      "description": "ICMP sessions over IPv6",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number"
    },
    {
      "name": "Other sessions over IPv6",
      "displayName": "Other sessions over IPv6",
      "description": "Other sessions over IPv6",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number"
    },
    {
      "name": "L2 sessions",
      "displayName": "L2 sessions",
      "description": "L2 sessions",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number"
    },
    {
      "name": "Session create errors",
      "displayName": "Session create errors",
      "description": "Session create errors",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number"
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
      ]
    }
  ],
  "objectKind": "DistributedServiceCard"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      ]
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      ]
    }
  ],
  "objectKind": "NetworkInterface",
  "interfaceType": "uplink-mgmt"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      ]
    }
  ],
  "objectKind": "NetworkInterface",
  "interfaceType": "host-pf"
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      ]
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      ]
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      ]
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
      "jsType": "number"
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
      "jsType": "number"
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
      "jsType": "number"
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
      ]
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