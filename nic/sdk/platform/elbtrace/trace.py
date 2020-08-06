
from ctypes import *
import _ctypes
from pprint import pformat
import os.path
from enum import Enum
from collections import defaultdict
from common import get_bits, get_bit

###################
##### MPU Trace 
###################

def libelbisa_so_load():
    # so_container path will be used if customers run captrace in a container
    so_container_path_elb = "/sw/nic/sdk/third-party/asic/ip/verif/pensim/gen/x86_64/lib/libcelbisa.so"
    so_rel_path_elb = "/../../third-party/asic/ip/verif/pensim/gen/x86_64/lib/libcelbisa.so"

    if os.path.exists(so_container_path_elb):
        libelbisa = cdll.LoadLibrary(so_container_path_elb)
    else:
        dir_path = os.path.dirname(os.path.realpath(__file__))
        so_path_elb = dir_path + so_rel_path_elb
        libelbisa = cdll.LoadLibrary(so_path_elb)

    return libelbisa

libelbisa = libelbisa_so_load()

libelbisa.c_libelbisa_init()

def to_dict(obj):
    if isinstance(obj, Array):
        return [to_dict(x) for x in obj]
    elif isinstance(obj, Structure) or isinstance(obj, Union):
        return {x[0]: to_dict(getattr(obj, x[0])) for x in obj._fields_ if not x[0].startswith('__')}
    elif isinstance(obj, int):
        return "0x%x" % obj
    else:
        return obj


def ctypes_pformat(cstruct):
    return pformat(to_dict(cstruct))


class MpuTraceHeader(BigEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("magic", c_uint64, 56),
        ("__pad0", c_uint64, 1),
        ("stg", c_uint64, 5),       # sge = 0-5, sgi = 6-11, txdma = 16-23, rxdma = 23-31
        ("mpu", c_uint64, 2),

        ("phv_timestamp_capture", c_uint64, 48),
        ("__pad1", c_uint64, 2),
        ("pkt_size", c_uint64, 14),

        ("mpu_processing_table_addr", c_uint64),

        ("__pad2", c_uint64, 1),
        ("entry_pc", c_uint64, 31),
        ("hash", c_uint64, 32),

        ("mpu_processing_table_latency", c_uint64, 16),
        ("mpu_processing_table_id", c_uint64, 4),

        ("sdp_pkt_id", c_uint64, 8),
        ("ring_nonempty", c_uint64, 8),
        ("table_hit", c_uint64, 1),
        ("table_error", c_uint64, 1),
        ("phv_error", c_uint64, 1),
        ("__pad3", c_uint64, 9),
        ("__pad4", c_uint64, 16),

        ("__pad5", c_uint64),

        ("__pad6", c_uint64),

        ("trace_debug_generation", c_uint64, 1),
        ("trace_table_and_key", c_uint64, 1),
        ("__pad9", c_uint64, 14),
        ("timestamp", c_uint64, 48),
    ]


class MpuTraceKD(BigEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("key_data", c_uint8 * 64),
        ("table_data", c_uint8 * 64),
    ]


class MpuTraceInstructionEntry(BigEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("end_DS", c_uint64, 1),
        ("predicate", c_uint64, 1),
        ("pc", c_uint64, 34),
        ("C", c_uint64, 7),
        ("c_result", c_uint64, 1),
        ("__pad0", c_uint64, 3),
        ("dual_issue", c_uint64, 1),
        ("inst_count", c_uint64, 16),
        ("rsrcB", c_uint64),
        ("alu_result", c_uint64),
        ("opcode", c_uint64),

        ("tsrc_src2b", c_uint64),
        ("src1b", c_uint64),

        ("__pad1", c_uint64, 48),
        ("__pad2", c_uint64, 13),
        ("inst_sel", c_uint64, 2),
        ("exception_level", c_uint64, 1),
        ("trace_debug_generation", c_uint64, 1),
        ("__pad3", c_uint64, 7),
        ("sdp_pkt_id", c_uint64, 8),
        ("timestamp", c_uint64, 48)
    ]

class PIPELINE(Enum):
    SXDMA_XG  = 4
    TXDMA_PCT = 3
    RXDMA_PCR = 2
    PGEG_SGE  = 1
    PGIG_SGI  = 0

class TraceFileHeader(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("pipeline_num", c_uint8),
        ("stage_num", c_uint32),
        ("mpu_num", c_uint32),
        ("enable", c_uint8),
        ("trace_enable", c_uint8),
        ("phv_debug", c_uint8),
        ("phv_error", c_uint8),
        ("trace_addr", c_uint64),
        ("table_key", c_uint8),
        ("instructions", c_uint8),
        ("wrap", c_uint8),
        ("reset", c_uint8),
        ("trace_size", c_uint32),
        ("wpc_trace", c_uint8),
        ("wpc_count", c_uint8),
        ("wpc_intr", c_uint8),
        ("wpc_stop", c_uint8),
        ("wpc_exception", c_uint8),
        ("wpc_addr_lo", c_uint64),
        ("wpc_addr_hi", c_uint64),
        ("wdata_rtrace", c_uint8),
        ("wdata_wtrace", c_uint8),
        ("wdata_rintr", c_uint8),
        ("wdata_wintr", c_uint8),
        ("wdata_rstop", c_uint8),
        ("wdata_wstop", c_uint8),
        ("wdata_rexception", c_uint8),
        ("wdata_wexception", c_uint8),
        ("wdata_addr_lo", c_uint64),
        ("wdata_addr_hi", c_uint64),
        ("debug_index", c_uint32),
        ("debug_generation", c_uint8),
        ("__pad", c_int8 * 49)
]
        


def decode_mpu_trace_file(bytez):
    assert (isinstance(bytez, bytes))
    s = 0

    while s < len(bytez):
        if (len(bytez) - s) < sizeof(TraceFileHeader):
            break

        # Read the file header
        fhdr = TraceFileHeader.from_buffer_copy(bytez[s: s + sizeof(TraceFileHeader)])
        # print(ctypes_pformat(fhdr))
        s += sizeof(TraceFileHeader)

        #debug info
        #print(sizeof(TraceFileHeader))
        #print("\n>>> Trace config HDR : 0x{:0128x}\n".format(int.from_bytes(fhdr, byteorder='big')))
        #for fld in (fhdr._fields_):
        #    if not fld[0].startswith('_'):
        #        if fld[0].startswith('pipeline'):
        #            print("{:50} {:#x} ({})".format(fld[0], getattr(fhdr, fld[0]), PIPELINE(getattr(fhdr, fld[0])).name))
        #        else:
        #            print("{:50} {:#x}".format(fld[0], getattr(fhdr, fld[0])))

        assert(fhdr.trace_size != 0)
        # Read Trace
        for thdr, key, data, instructions in decode_mpu_trace_kd(bytez[s: s + (fhdr.trace_size * 64)]):
            yield fhdr, thdr, key, data, instructions

        s += (fhdr.trace_size * 64)


def decode_mpu_trace_kd(bytez):
    assert (isinstance(bytez, bytes))

    magic = [0xc0, 0xde, 0x41, 0x1c, 0x0d, 0xe4, 0x11]
    for i in range(len(bytez)):
        for j in range(len(magic)):
            if bytez[i + j] != magic[j]:
                break
        else:
            break
    else:
        # Empty trace
        print("\n Empty trace \n")
        return

    # print(','.join(hex(x) for x in bytez[:64]))

    #print("Found magic at offset ", i)
    from collections import deque
    bytez = deque(bytez)
    bytez.rotate(-i)
    bytez = bytes(bytez)

    # print(','.join(hex(x) for x in bytez[:64]))

    s = 0
    while s < len(bytez):

        if (len(bytez) - s) < sizeof(MpuTraceHeader):
            break

        # Read Header
        thdr = MpuTraceHeader.from_buffer_copy(bytez[s: s + sizeof(MpuTraceHeader)])
        # print(ctypes_pformat(thdr))

        if thdr.magic == 0xc0de411c0de411:
            s += sizeof(MpuTraceHeader)

            if (len(bytez) - s) < sizeof(MpuTraceKD):
                break

            if thdr.trace_table_and_key:
                # Read KD
                kd = MpuTraceKD.from_buffer_copy(bytez[s: s + sizeof(MpuTraceKD)])
                # print(ctypes_pformat(kd))
                s += sizeof(MpuTraceKD)

                instructions = []
                # We need to figure out if there are instruction trace entries
                while s < len(bytez):
                    # Probe the next entry to see if there is valid header there.
                    ent = MpuTraceHeader.from_buffer_copy(bytez[s: s + sizeof(MpuTraceHeader)])
                    if ent.magic == 0xc0de411c0de411:
                        break
                    # It is either an instruction entry or uninitialized memory.
                    ent = MpuTraceInstructionEntry.from_buffer_copy(bytez[s: s + sizeof(MpuTraceInstructionEntry)])
                    # An entry is a valid instruction entry if it has the following relations
                    # with the header entry.
                    if (ent.timestamp >= thdr.timestamp and
                        thdr.trace_debug_generation == ent.trace_debug_generation and
                        thdr.sdp_pkt_id == ent.sdp_pkt_id):
                        instructions.append(ent)
                    s += sizeof(MpuTraceInstructionEntry)

                # Generate
                yield (
                        thdr,
                        int.from_bytes(kd.key_data, byteorder='big'),
                        int.from_bytes(kd.table_data, byteorder='big'),
                        instructions
                       )
            else:
                yield (
                    thdr,
                    None,
                    None,
                    []
                )
        else:
            s += 1

    else:
        if s != len(bytez):
            raise ValueError("Invalid Trace!")

def decode_instruction(pc, opcode):
    out = libelbisa.c_libelb_dasm(c_uint64(pc), c_uint64(opcode))
    s = c_char_p(out).value.decode('utf-8')
    libelbisa.c_libelb_dasm_free(out)
    return s
