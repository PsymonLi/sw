import { MetricMeasurement } from './metadata';
import { Telemetry_queryMetricsQuerySpec_function } from "@sdk/v1/models/generated/telemetry_query";

export const MetricsMetadataIris: { [key: string]: MetricMeasurement } = {
  AccelHwRingMetrics: {
  "name": "AccelHwRingMetrics",
  "description": "Key indices - RId: ring ID, SubRId: sub-ring ID",
  "displayName": "Metrics for hardware rings",
  "fields": [
    {
      "name": "PIndex",
      "displayName": "P Index",
      "description": "ring producer index",
      "units": "ID",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "CIndex",
      "displayName": "C Index",
      "description": "ring consumer index",
      "units": "ID",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "InputBytes",
      "displayName": "Input Bytes",
      "description": "total input bytes (not available for cp_hot, dc_hot, xts_enc/dec, gcm_enc/dec)",
      "units": "Bytes",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "OutputBytes",
      "displayName": "Output Bytes",
      "description": "total output bytes (not available for cp_hot, dc_hot, xts_enc/dec, gcm_enc/dec)",
      "units": "Bytes",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "SoftResets",
      "displayName": "Soft Resets",
      "description": "number of soft resets executed",
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
  "scope": "PerRingPerSubRing",
  "objectKind": "DistributedServiceCard",
  "uiGroup": "AccelHwRingMetrics"
},
  AccelPfInfo: {
  "name": "AccelPfInfo",
  "description": "Key index - logical interface ID",
  "displayName": "Device information",
  "fields": [
    {
      "name": "HwLifId",
      "displayName": "Hw Lif Id",
      "description": "hardware logical interface ID",
      "units": "ID",
      "baseType": "uint64",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NumSeqQueues",
      "displayName": "Num Seq Queues",
      "description": "number of sequencer queues available",
      "units": "Count",
      "baseType": "uint32",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "CryptoKeyIdxBase",
      "displayName": "Crypto Key Idx Base",
      "description": "crypto key base index",
      "units": "Count",
      "baseType": "uint32",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NumCryptoKeysMax",
      "displayName": "Num Crypto Keys Max",
      "description": "maximum number of crypto keys allowed",
      "units": "Count",
      "baseType": "uint32",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IntrBase",
      "displayName": "Intr Base",
      "description": "CPU interrupt base",
      "units": "Count",
      "baseType": "uint32",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IntrCount",
      "displayName": "Intr Count",
      "description": "CPU interrupt vectors available",
      "units": "Count",
      "baseType": "uint32",
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
  "scope": "PerLIF",
  "objectKind": "NetworkInterface",
  "interfaceType": "host-pf",
  "uiGroup": "AccelPfInfo"
},
  AccelSeqQueueInfoMetrics: {
  "name": "AccelSeqQueueInfoMetrics",
  "description": "Key indices are - LifID and QId",
  "displayName": "Sequencer queues information",
  "fields": [
    {
      "name": "QStateAddr",
      "displayName": "Q State Add",
      "description": "queue state memory address",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "QGroup",
      "displayName": "Q Group",
      "description": "queue group\\n           : 0 - compress/decompress\\n           : 1 - compress/decompress status\\n           : 2 - crypto\\n           : 3 - crypto status\\n",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "CoreId",
      "displayName": "Core Id",
      "description": "CPU core ID (not available currently",
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
  "scope": "PerLIFPerQ",
  "objectKind": "DistributedServiceCard",
  "uiGroup": "AccelSeqQueueInfoMetrics"
},
  AccelSeqQueueMetrics: {
  "name": "AccelSeqQueueMetrics",
  "description": "Key indices are - LifID and QId",
  "displayName": "Metrics for sequencer queues",
  "fields": [
    {
      "name": "InterruptsRaised",
      "displayName": "Interrupts Raised",
      "description": "CPU interrupts raised",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NextDBsRung",
      "displayName": "Next DBs Rung",
      "description": "chaining doorbells rung",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "SeqDescsProcessed",
      "displayName": "Seq Descs Processed",
      "description": "sequencer descriptors processed",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "SeqDescsAborted",
      "displayName": "Seq Descs Aborted",
      "description": "sequencer descriptors aborted (due to reset)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "StatusPdmaXfers",
      "displayName": "Status Pdma Xfers",
      "description": "status descriptors copied",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "HwDescXfers",
      "displayName": "Hw Desc Xfers",
      "description": "descriptors transferred to hardware",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "HwBatchErrors",
      "displayName": "Hw Batch Errors",
      "description": "hardware batch (length) errors",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "HwOpErrors",
      "displayName": "Hw Op Errors",
      "description": "hardware operation errors",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "AolUpdateReqs",
      "displayName": "Aol Update Reqs",
      "description": "AOL list updates requested",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "SglUpdateReqs",
      "displayName": "Sgl Update Reqs",
      "description": "scatter/gather list updates requested",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "SglPdmaXfers",
      "displayName": "Sgl Pdma Xfers",
      "description": "payload DMA transfers executed",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "SglPdmaErrors",
      "displayName": "Sgl Pdma Errors",
      "description": "payload DMA errors encountered",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "SglPadOnlyXfers",
      "displayName": "Sgl Pad Only Xfers",
      "description": "pad-data-only DMA transfers executed",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "SglPadOnlyErrors",
      "displayName": "Sgl Pad Only Errors",
      "description": "pad-data-only DMA errors encountered",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "AltDescsTaken",
      "displayName": "Alt Descs Taken",
      "description": "alternate (bypass-onfail) descriptors executed",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "AltBufsTaken",
      "displayName": "Alt Bufs Taken",
      "description": "alternate buffers taken",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "LenUpdateReqs",
      "displayName": "Len Update Reqs",
      "description": "length updates requested",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "CpHeaderUpdates",
      "displayName": "Cp Header Updates",
      "description": "compression header updates requested",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "SeqHwBytes",
      "displayName": "Seq Hw Bytes",
      "description": "bytes processed",
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
  "scope": "PerLIFPerQ",
  "objectKind": "DistributedServiceCard",
  "uiGroup": "AccelSeqQueueMetrics"
},
  DropMetrics: {
  "name": "DropMetrics",
  "description": "Asic Forwarding Drops Statistics",
  "displayName": "Forwarding Drops",
  "fields": [
    {
      "name": "DropParseLenError",
      "displayName": "Packet Length Errors",
      "description": "number of packets dropped due to parse length errors",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropHardwareError",
      "displayName": "Hardware Errors",
      "description": "number of packets dropped due to hardware errors seen",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropInputMapping",
      "displayName": "Input Mapping Table",
      "description": "number of packets dropped due to missing lookup in input mapping table",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropMultiDestNotPinnedUplink",
      "displayName": "Multi-Dest Not Pinned Uplink",
      "description": "number of multi-destination (multicast) packets dropped because they were not seen on right pinned uplink",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropFlowHit",
      "displayName": "Drop Flow Hit",
      "description": "number of packets dropped due to hitting drop flows",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropFlowMiss",
      "displayName": "Flow Miss",
      "description": "number of packets dropped due to missing a flow-hit",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropNacl",
      "displayName": "Drop NACL Hit",
      "description": "number of packets dropped due to drop-nacl hit",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropIpsg",
      "displayName": "Drop IPSG",
      "description": "number of packets dropped due to drop-ipsg hit",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropIpNormalization",
      "displayName": "IP Normalization",
      "description": "number of packets dropped due to IP packet normalization",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropTcpNormalization",
      "displayName": "TCP Normalization",
      "description": "number of TCP packets dropped due to TCP normalization",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropTcpRstWithInvalidAckNum",
      "displayName": "TCP RST Invalid ACK",
      "description": "number of TCP RST packets dropped due to invalid ACK number",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropTcpNonSynFirstPkt",
      "displayName": "TCP Non-SYN First Packet",
      "description": "number of TCP non-SYN first packets dropped",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropIcmpNormalization",
      "displayName": "ICMP Normalization",
      "description": "number of packets dropped due to ICMP packet normalization",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropInputPropertiesMiss",
      "displayName": "Input Properties Miss",
      "description": "number of packets dropped due to input properties miss",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropTcpOutOfWindow",
      "displayName": "TCP Out Of Window",
      "description": "number of TCP packets dropped due to out-of-window",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropTcpSplitHandshake",
      "displayName": "TCP Split Handshake",
      "description": "number of TCP packets dropped due to split handshake",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropTcpWinZeroDrop",
      "displayName": "TCP Zero Window",
      "description": "number of TCP packets dropped due to window size being zero",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropTcpDataAfterFin",
      "displayName": "TCP Data After FIN",
      "description": "number of TCP packets dropped due to data received after FIN was seen",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropTcpNonRstPktAfterRst",
      "displayName": "TCP Non-RST After RST",
      "description": "number of TCP packets dropped due to non-RST seen after RST",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropTcpInvalidResponderFirstPkt",
      "displayName": "TCP Responder First Packet",
      "description": "number of TCP packets dropped due to invalid first packet seen from responder",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropTcpUnexpectedPkt",
      "displayName": "TCP Unexpected Packet",
      "description": "number of TCP packets dropped due to unexpected packet seen",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropSrcLifMismatch",
      "displayName": "Source LIF Mismatch",
      "description": "number of packets dropped due to packets received on unexpected source LIF",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropIcmpFragPkt",
      "displayName": "ICMP/ICMPv6 Fragment",
      "description": "ICMP/ICMPv6 fragmented packet drops",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "DropIpFragPkt",
      "displayName": "IP Fragment",
      "description": "IP fragmented packet drops",
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
  "tags": [
    "Level2"
  ],
  "scope": "PerASIC",
  "objectKind": "DistributedServiceCard",
  "uiGroup": "DropMetrics"
},
  FteCPSMetrics: {
  "name": "FteCPSMetrics",
  "description": "Connections per second related statistics",
  "displayName": "CPS Statistics",
  "fields": [
    {
      "name": "ConnectionsPerSecond",
      "displayName": "Connections Per Second",
      "description": "The number of connections established per second on DSCs",
      "units": "Gauge",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "MaxConnectionsPerSecond",
      "displayName": "Max-CPS",
      "description": "Max Connections per second",
      "units": "Gauge",
      "baseType": "Counter",
      "tags": [
        "Level7"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "PacketsPerSecond",
      "displayName": "PPS",
      "description": "Packets per second",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level7"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "MaxPacketsPerSecond",
      "displayName": "Max-PPS",
      "description": "Max packets per second",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level7"
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
  "tags": [
    "Level4"
  ],
  "scope": "PerFTE",
  "features": [
    "FLOWAWARE",
    "FLOWAWARE_FIREWALL"
  ],
  "objectKind": "DistributedServiceCard",
  "uiGroup": "SessionSummaryMetrics"
},
  FteLifQMetrics: {
  "name": "FteLifQMetrics",
  "description": "Key index - FTE ID",
  "displayName": "per-FTE Queue Statistics",
  "fields": [
    {
      "name": "FlowMissPackets",
      "displayName": "Flow-miss Packets",
      "description": "Number of flow miss packets processed by this FTE",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FlowRetransmitPackets",
      "displayName": "Flow-retransmit Packets",
      "description": "Number of flow retransmits seen",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "L4RedirectPackets",
      "displayName": "L4-redirect Packets",
      "description": "Number of packets that hit the L4 redirect queue",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "AlgControlFlowPackets",
      "displayName": "ALG-control-flow Packets",
      "description": "Number of packets that hit the ALG control flow queue",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TcpClosePackets",
      "displayName": "TCP-Close Packets",
      "description": "Number of packets that hit the TCP close queue",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TlsProxyPackets",
      "displayName": "TLS-proxy Packets",
      "description": "Number of packets that hit the TLS Proxy queue",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FteSpanPackets",
      "displayName": "FTE-Span Packets",
      "description": "Number of packets that hit the FTE SPAN queue",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "SoftwareQueuePackets",
      "displayName": "Software-config-Q Requests",
      "description": "Number of packets that hit the FTE config path",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "QueuedTxPackets",
      "displayName": "Queued-Tx Packets",
      "description": "Number of packets enqueue in the FTE TX queue",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "FreedTxPackets",
      "displayName": "Freed-Tx Packets",
      "description": "Number of dropped or non-flowmiss packets for which the CPU resources are freed",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "MaxSessionThresholdDrops",
      "displayName": "Max Session Threshold Drops",
      "description": "Number of flowmiss packets dropped due to max session limit being hit",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "SessionCreatesIgnored",
      "displayName": "Session creates ignored",
      "description": "Number of flow miss packets ignored for session creation",
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
  "tags": [
    "Level7"
  ],
  "scope": "PerFTE",
  "features": [
    "FLOWAWARE",
    "FLOWAWARE_FIREWALL"
  ],
  "objectKind": "DistributedServiceCard",
  "uiGroup": "FteLifQMetrics"
},
  SessionSummaryMetrics: {
  "name": "SessionSummaryMetrics",
  "description": "Session Summary Related Statistics",
  "displayName": "Session",
  "fields": [
    {
      "name": "TotalActiveSessions",
      "displayName": "Total Active Sessions",
      "description": "Total Number of active sessions",
      "units": "Gauge",
      "baseType": "Counter",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.max,
      "tags": [
        "Level4"
      ],
      "jsType": "number"
    },
    {
      "name": "NumL2Sessions",
      "displayName": "L2 Sessions",
      "description": "Total Number of L2 Sessions",
      "units": "Count",
      "baseType": "Counter",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.max,
      "tags": [
        "Level4"
      ],
      "jsType": "number"
    },
    {
      "name": "NumTcpSessions",
      "displayName": "TCP Sessions",
      "description": "Total Number of TCP sessions",
      "units": "Count",
      "baseType": "Counter",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.max,
      "tags": [
        "Level4"
      ],
      "jsType": "number"
    },
    {
      "name": "NumUdpSessions",
      "displayName": "UDP Sessions",
      "description": "Total Number of UDP sessions",
      "units": "Count",
      "baseType": "Counter",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.max,
      "tags": [
        "Level4"
      ],
      "jsType": "number"
    },
    {
      "name": "NumIcmpSessions",
      "displayName": "ICMP Sessions",
      "description": "Total Number of ICMP sessions",
      "units": "Count",
      "baseType": "Counter",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.max,
      "tags": [
        "Level4"
      ],
      "jsType": "number"
    },
    {
      "name": "NumDropSessions",
      "displayName": "Security Policy Drops",
      "description": "Total Number of sessions dropped by Security Policy",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NumAgedSessions",
      "displayName": "Aged Sessions",
      "description": "Total Number of Aged sessions",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NumTcpResets",
      "displayName": "TCP RST Sent",
      "description": "Total Number of TCP Resets sent as a result of SFW Reject",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NumIcmpErrors",
      "displayName": "ICMP Error Sent",
      "description": "Total Number of ICMP Errors sent as a result of SFW Reject",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NumTcpCxnsetupTimeouts",
      "displayName": "Connection Timeout Sessions",
      "description": "Total Number of sessions that timed out at connection setup",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NumSessionCreateErrors",
      "displayName": "Session Create Errors",
      "description": "Total Number of sessions that errored out during creation",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NumTcpHalfOpenSessions",
      "displayName": "TCP Half Open Sessions",
      "description": "Total Number of TCP Half Open sessions",
      "units": "Count",
      "baseType": "Counter",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.max,
      "tags": [
        "Level4"
      ],
      "jsType": "number"
    },
    {
      "name": "NumOtherActiveSessions",
      "displayName": "Other Active Sessions",
      "description": "Total Number of sessions other than TCP/UDP/ICMP",
      "units": "Count",
      "baseType": "Counter",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.max,
      "tags": [
        "Level4"
      ],
      "jsType": "number"
    },
    {
      "name": "NumTcpSessionLimitDrops",
      "displayName": "TCP Half Open Session Limit Exceed Drops",
      "description": "Total Number of sessions dropped due to TCP half open session limit",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NumUdpSessionLimitDrops",
      "displayName": "UDP Session Limit Exceed Drops",
      "description": "Total Number of sessions dropped due to UDP session limit",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NumIcmpSessionLimitDrops",
      "displayName": "ICMP Session Limit Exceed Drops",
      "description": "Total Number of sessions dropped due to ICMP session limit",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "NumDscSessionLimitDrops",
      "displayName": "DSC Session Limit Exceed Drops",
      "description": "Total Number of sessions dropped due to DSC session limit",
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
  "tags": [
    "Level1"
  ],
  "scope": "PerFTE",
  "features": [
    "FLOWAWARE",
    "FLOWAWARE_FIREWALL"
  ],
  "objectKind": "DistributedServiceCard",
  "uiGroup": "SessionSummaryMetrics"
},
  MacMetrics: {
  "name": "MacMetrics",
  "description": "Uplink Interface Packet Statistics",
  "displayName": "Uplink Interface",
  "fields": [
    {
      "name": "FramesRxOk",
      "displayName": "Rx OK Frames",
      "description": "Good Frames received",
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
      "description": "All Frames received (Good and Bad)",
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
      "description": "Frames received with bad FCS",
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
      "description": "Frames received with any bad (CRC, Length or Align)",
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
      "description": "Octets received in Good Frames",
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
      "description": "Octets received in all frames (Good and Bad)",
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
      "description": "Frames received with Unicast Address",
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
      "description": "Frames received with Multicast Address",
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
      "description": "Frames received with Broadcast Address",
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
      "description": "Frames received with lenth error. EtherType field < 1536, and frame size > received length value.",
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
      "description": "Frames received with frame size < min frame size(64 Octets)",
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
      "description": "Frames received with frame size > max frame size(9216 Octets) but < jabber size(9232 Octets)",
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
      "description": "Fragment frames received",
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
      "displayName": "Rx PFC Frames",
      "description": "Frames received of type PFC",
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
      "description": "Frames received with frame size > max frame size(9216 Octets)",
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
      "description": "Received frames dropped (Buffer Full)",
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
      "description": "Frames received of length < 64 octets",
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
      "description": "Frames received of length 64 octets",
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
      "description": "Frames received of length 65~127 octets",
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
      "description": "Frames received of length 128~255 octets",
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
      "description": "Frames received of length 256~511 octets",
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
      "description": "Frames received of length 512~1023 octets",
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
      "description": "Frames received of length 1024~1518 octets",
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
      "description": "Frames received of length 1519~2047 octets",
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
      "description": "Frames received of length 2048~4095 octets",
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
      "description": "Frames received of length 4096~8191 octets",
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
      "description": "Frames received of length 8192~9215 octets",
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
      "description": "Frames received of length > 9215 octets",
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
      "description": "Good frames transmitted",
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
      "description": "All frames transmitted (Good and Bad)",
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
      "description": "Frames transmitted with bad (CRC, Length or Align)",
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
      "description": "Octets transmitted in good frames",
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
      "description": "Octets transmitted in all frames (Good and Bad)",
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
      "description": "Frames transmitted with Unicast Address",
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
      "description": "Frames transmitted with Multicast Address",
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
      "description": "Frames transmitted with Broadcast Address",
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
      "displayName": "Tx PFC Frames",
      "description": "Frames transmitted of type PFC",
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
      "description": "Frames transmitted with vlan header",
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
      "description": "Frames transmitted of length < 64 octets",
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
      "description": "Frames transmitted of length 64 octets",
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
      "description": "Frames transmitted of length 65~127 octets",
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
      "description": "Frames transmitted of length 128~255 octets",
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
      "description": "Frames transmitted of length 256~511 octets",
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
      "description": "Frames transmitted of length 512~1023 octets",
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
      "description": "Frames transmitted of length 1024~1518 octets",
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
      "description": "Frames transmitted of length 1519~2047 octets",
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
      "description": "Frames transmitted of length 2048~4095 octets",
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
      "description": "Frames transmitted of length 4096~8191 octets",
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
      "description": "Frames transmitted of length 8192~9215 octets",
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
      "description": "Frames transmitted of length > 9215 octets",
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
      "displayName": "Tx PFC Pri0 Frames",
      "description": "Frames transmitted of PFC priority 0",
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
      "displayName": "Tx PFC Pri1 Frames",
      "description": "Frames transmitted of PFC priority 1",
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
      "displayName": "Tx PFC Pri2 Frames",
      "description": "Frames transmitted of PFC priority 2",
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
      "displayName": "Tx PFC Pri3 Frames",
      "description": "Frames transmitted of PFC priority 3",
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
      "displayName": "Tx PFC Pri4 Frames",
      "description": "Frames transmitted of PFC priority 4",
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
      "displayName": "Tx PFC Pri5 Frames",
      "description": "Frames transmitted of PFC priority 5",
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
      "displayName": "Tx PFC Pri6 Frames",
      "description": "Frames transmitted of PFC priority 6",
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
      "displayName": "Tx PFC Pri7 Frames",
      "description": "Frames transmitted of PFC priority 7",
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
      "displayName": "Rx PFC Pri0 Frames",
      "description": "Frames received of PFC priority 0",
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
      "displayName": "Rx PFC Pri1 Frames",
      "description": "Frames received of PFC priority 1",
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
      "displayName": "Rx PFC Pri2 Frames",
      "description": "Frames received of PFC priority 2",
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
      "displayName": "Rx PFC Pri3 Frames",
      "description": "Frames received of PFC priority 3",
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
      "displayName": "Rx PFC Pri4 Frames",
      "description": "Frames received of PFC priority 4",
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
      "displayName": "Rx PFC Pri5 Frames",
      "description": "Frames received of PFC priority 5",
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
      "displayName": "Rx PFC Pri6 Frames",
      "description": "Frames received of PFC priority 6",
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
      "displayName": "Rx PFC Pri7 Frames",
      "description": "Frames received of PFC priority 7",
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
      "description": "Frames transmitted with truncation",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxPps",
      "displayName": "Tx Packets/sec",
      "description": "Transmit BW Packets per second(PPS)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxBytesps",
      "displayName": "Tx Bytes/sec",
      "description": "Transmit BW Bytes per second(BPS)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxPps",
      "displayName": "Rx Packets/sec",
      "description": "Receive BW Packets per second(PPS)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxBytesps",
      "displayName": "Rx Bytes/sec",
      "description": "Receive BW Bytes per second(BPS)",
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
  "tags": [
    "Level3"
  ],
  "scope": "PerEthPort",
  "objectKind": "NetworkInterface",
  "interfaceType": "uplink-eth",
  "uiGroup": "MacMetrics"
},
  MgmtMacMetrics: {
  "name": "MgmtMacMetrics",
  "description": "Management Interface Packet Statistics",
  "displayName": "Management Interface",
  "fields": [
    {
      "name": "FramesRxOk",
      "displayName": "Rx OK Frames",
      "description": "Good Frames received",
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
      "description": "All Frames received (Good and Bad)",
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
      "description": "Frames received with bad FCS",
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
      "description": "Frames with any bad (CRC, Length or Align)",
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
      "description": "Octets received in Good Frames",
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
      "description": "Octets received in all frames (Good and Bad)",
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
      "description": "Frames received with Unicast Address",
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
      "description": "Frames received with Multicast Address",
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
      "description": "Frames received with Broadcast Address",
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
      "description": "Frames received with lenth error. EtherType field < 1536, and frame size > received length value.",
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
      "description": "Frames received with frame size < min frame size(64 Octets)",
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
      "description": "Frames received with frame size > max frame size(9216 Octets) but < jabber size(9232 Octets)",
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
      "description": "Fragment frames received",
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
      "description": "Frames received of length 64 octets",
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
      "description": "Frames received of length 65~127 octets",
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
      "description": "Frames received of length 128~255 octets",
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
      "description": "Frames received of length 256~511 octets",
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
      "description": "Frames received of length 512~1023 octets",
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
      "description": "Frames received of length 1024~1518 octets",
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
      "description": "Frames received of length > 1518 octets",
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
      "description": "Good frames transmitted",
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
      "description": "All frames transmitted (Good and Bad)",
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
      "description": "Frames transmitted with bad (CRC, Length or Align)",
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
      "description": "Octets transmitted in good frames",
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
      "description": "Octets transmitted in all frames (Good and Bad)",
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
      "description": "Frames transmitted with Unicast Address",
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
      "description": "Frames transmitted with Multicast Address",
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
      "description": "Frames transmitted with Broadcast Address",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxPps",
      "displayName": "Tx Packets/sec",
      "description": "Transmit bandwidth in  Packets per second(PPS)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxBytesps",
      "displayName": "Tx Bytes/sec",
      "description": "Transmit bandwidth in Bytes per second(BPS)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxPps",
      "displayName": "Rx Packets/sec",
      "description": "Receive bandwidth in Packets per second(PPS)",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxBytesps",
      "displayName": "Rx Bytes/sec",
      "description": "Receive bandwidth in Bytes per second(BPS)",
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
  "tags": [
    "Level3"
  ],
  "scope": "PerMgmtPort",
  "objectKind": "NetworkInterface",
  "interfaceType": "uplink-mgmt",
  "uiGroup": "MgmtMacMetrics"
},
  LifMetrics: {
  "name": "LifMetrics",
  "description": "Logical Interface Statistics",
  "displayName": "Logical Interface",
  "fields": [
    {
      "name": "RxUnicastBytes",
      "displayName": "Rx Unicast Bytes",
      "description": "Bytes received in packets with unicast destination",
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
      "description": "Packets received with unicast destination",
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
      "description": "Bytes received in packets with multicast destination",
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
      "description": "Packets received with multicast destination",
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
      "description": "Bytes received in packets with broadcast destination",
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
      "description": "Packets received with broadcast destination",
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
      "description": "Bytes of received packets dropped with unicast destination",
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
      "description": "Received packets dropped with unicast destination",
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
      "description": "Bytes of received packets dropped with multicast destination",
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
      "description": "Received packets dropped with multicast destination",
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
      "description": "Bytes of received packets dropped with broadcast destination",
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
      "description": "Received packets dropped with broadcast destination",
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
      "description": "Bytes transmitted in packets with unicast destination",
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
      "description": "Packets transmitted with unicast destination",
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
      "description": "Bytes transmitted in packets with multicast destination",
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
      "description": "Packets transmitted with multicast destination",
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
      "description": "Bytes transmitted in packets with broadcast destination",
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
      "description": "Packets transmitted with broadcast destination",
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
      "description": "Bytes of transmit packets dropped with unicast destination",
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
      "description": "Transmit packets dropped with unicast destination",
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
      "description": "Bytes of transmit packets dropped with multicast destination",
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
      "description": "Transmit packets dropped with multicast destination",
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
      "description": "Bytes of transmit packets dropped with broadcast destination",
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
      "description": "Transmit packets dropped with broadcast destination",
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
      "displayName": "Tx Packets",
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
      "displayName": "Rx Packets",
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
      "name": "TxPps",
      "displayName": "Tx Packets/sec",
      "description": "Transmit bandwidth in Packets per second",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TxBytesps",
      "displayName": "Tx Bytes/sec",
      "description": "Transmit bandwidth in Bytes per second",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxPps",
      "displayName": "Rx Packets/sec",
      "description": "Receive bandwidth in Packets per second",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "RxBytesps",
      "displayName": "Rx Bytes/sec",
      "description": "Receive bandwidth in Bytes per second",
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
  "tags": [
    "Level3"
  ],
  "scope": "PerLIF",
  "objectKind": "NetworkInterface",
  "interfaceType": "host-pf",
  "uiGroup": "LifMetrics"
},
  NMDMetrics: {
  "name": "NMDMetrics",
  "description": "Statistics related to NMD",
  "displayName": "NMD Metrics",
  "fields": [
    {
      "name": "GetCalls",
      "displayName": "GET calls",
      "description": "Number of GET calls to NMD",
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
  "scope": "PerNode",
  "objectKind": "DistributedServiceCard",
  "uiGroup": "NMDMetrics"
},
  RuleMetrics: {
  "name": "RuleMetrics",
  "description": "Key index - Rule ID",
  "displayName": "per-Rule Statistics",
  "fields": [
    {
      "name": "TcpHits",
      "displayName": "TCP Hits",
      "description": "Number of TCP packets that hit the rule",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "UdpHits",
      "displayName": "UDP Hits",
      "description": "Number of UDP packets that hit the rule",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "IcmpHits",
      "displayName": "ICMP Hits",
      "description": "Number of ICMP packets that hit the rule",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "EspHits",
      "displayName": "ESP Hits",
      "description": "Number of ESP packets that hit the rule",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "OtherHits",
      "displayName": "Other Hits",
      "description": "Number of non-TCP/UDP/ICMP/ESP packets that hit the rule",
      "units": "Count",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "TotalHits",
      "displayName": "Total Hits",
      "description": "Number of total packets that hit the rule",
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
  "tags": [
    "Level7"
  ],
  "scope": "PerFwRule",
  "objectKind": "DistributedServiceCard",
  "uiGroup": "RuleMetrics"
},
  AsicMemoryMetrics: {
  "name": "AsicMemoryMetrics",
  "description": "System Memory",
  "displayName": "System Memory",
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
  "tags": [
    "Level7"
  ],
  "scope": "UnknownScope",
  "objectKind": "DistributedServiceCard",
  "uiGroup": "AsicMemoryMetrics"
},
  AsicPowerMetrics: {
  "name": "AsicPowerMetrics",
  "description": "Asic Power",
  "displayName": "Asic Power",
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
  "tags": [
    "Level7"
  ],
  "scope": "PerASIC",
  "objectKind": "DistributedServiceCard",
  "uiGroup": "AsicPowerMetrics"
},
  AsicTemperatureMetrics: {
  "name": "AsicTemperatureMetrics",
  "description": "Asic Temperature",
  "displayName": "Asic Temperature",
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
      "name": "QsfpPort1Temperature",
      "displayName": "QSFP port1 temperature",
      "description": "QSFP port 1 temperature in celsius",
      "units": "Celsius",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "QsfpPort2Temperature",
      "displayName": "QSFP port2 temperature",
      "description": "QSFP port 2 temperature in celsius",
      "units": "Celsius",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "QsfpPort1WarningTemperature",
      "displayName": "QSFP port1 warning temperature",
      "description": "QSFP port 1 warning temperature in celsius",
      "units": "Celsius",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "QsfpPort2WarningTemperature",
      "displayName": "QSFP port2 warning temperature",
      "description": "QSFP port 2 warning temperature in celsius",
      "units": "Celsius",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "QsfpPort1AlarmTemperature",
      "displayName": "QSFP port1 alarm temperature",
      "description": "QSFP port 1 alarm temperature in celsius",
      "units": "Celsius",
      "baseType": "Counter",
      "tags": [
        "Level4"
      ],
      "jsType": "number",
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.last
    },
    {
      "name": "QsfpPort2AlarmTemperature",
      "displayName": "QSFP port2 alarm temperature",
      "description": "QSFP port 2 alarm temperature in celsius",
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
  "tags": [
    "Level7"
  ],
  "scope": "PerASIC",
  "objectKind": "DistributedServiceCard",
  "uiGroup": "AsicTemperatureMetrics"
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
  ],
  "uiGroup": "Node"
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
  ],
  "uiGroup": "DistributedServiceCard"
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
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.max
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
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.max
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
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.max
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
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.max
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
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.max
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
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.max
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
      ],
      "aggregationFunc": Telemetry_queryMetricsQuerySpec_function.max
    }
  ],
  "uiGroup": "Cluster"
},
}