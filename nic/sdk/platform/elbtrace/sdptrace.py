
from ctypes import *
from pprint import pformat
import os.path
from enum import Enum
from collections import defaultdict
from common import get_bits, get_bit

###################
##### SDP Trace 
###################

#shd be 64B aligned

class SdpTraceFileHeader(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("pipeline_num",               c_uint8), 
        ("stage_num",                  c_uint32),
        ("enable",                     c_uint8),
        ("trace_trigger_enable",       c_uint8), 
        ("sw_reset_enable",            c_uint8), 
        
        ("__pad",            c_uint16), 
        ("no_trace_when_full",       c_uint8), 
        ("stop_when_full",           c_uint8), 
        ("ctl_base_addr",            c_uint32),
        
        ("phv_base_addr",  c_uint32),
        ("ring_size",      c_uint32),
        
        ("ctl_ring_wr_ptr", c_uint32),
        ("phv_ring_wr_ptr", c_uint32),
        
        ("trigger_data_0",   c_uint64),
        ("trigger_data_1",   c_uint64),
        ("trigger_data_2",   c_uint64),
        ("trigger_data_3",   c_uint64),
        ("trigger_data_4",   c_uint64),
        ("trigger_data_5",   c_uint64),
        ("trigger_data_6",   c_uint64),
        ("trigger_data_7",   c_uint64),
        
        ("trigger_mask_0",   c_uint64),
        ("trigger_mask_1",   c_uint64),
        ("trigger_mask_2",   c_uint64),
        ("trigger_mask_3",   c_uint64),
        ("trigger_mask_4",   c_uint64),
        ("trigger_mask_5",   c_uint64),
        ("trigger_mask_6",   c_uint64),
        ("trigger_mask_7",   c_uint64),
        
]


class SdpCtlHeader(BigEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("generation", c_uint8, 1),
        ("__pad0", c_uint8, 7),
        ("__pad1", c_int8 * 55),

        ("__pad2", c_uint16, 12),
        ("phv_len", c_uint8, 4),
        ("timestamp", c_uint64, 48),
    ]


#17 Bytes
class IntGlobalHeader(BigEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("tm_iport", c_uint8, 4),
        ("tm_oport", c_uint8, 4),
        
        ("tm_iq", c_uint8, 5),
        ("lif", c_uint16, 11),
        ("timestamp_upr", c_uint64, 48),

        ("tm_span_session", c_uint8, 8),
        ("tm_replicate_ptr", c_uint16, 16),
        ("tm_replicate_en", c_uint8, 1),
        ("tm_cpu", c_uint8, 1),
        ("tm_q_depth", c_uint16, 14),
        ("drop", c_uint8, 1),
        ("bypass", c_uint8, 1),
        ("hw_error", c_uint8, 1),
        ("tm_oq", c_uint8, 5),
        ("debug_trace", c_uint8, 1),
        ("csum_err", c_uint8, 5),
        ("error_bits_upr", c_uint8, 2),
        
        ("error_bits_lwr", c_uint8, 4),
        ("tm_instance_type", c_uint8, 4),
    ]

class IntP4Header(BigEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("crc_err", c_uint8, 1),
        ("len_err", c_uint8, 4),
        ("recirc_count", c_uint8, 3),

        ("parser_err", c_uint8, 1),
        ("crypto_hdr", c_uint8, 1),
        ("frame_size", c_uint16, 14),

        ("no_data", c_uint8, 1),
        ("recirc", c_uint8, 1),
        ("packet_len_upr", c_uint16, 14),

        ("csum_error_hi", c_uint8, 2),
        ("deparser_csum_disable", c_uint8, 6),

        ("deparser_crc_disable", c_uint8, 2),
        ("roce_table_ptr", c_uint16, 9),

        ("roce_csum_table_upr", c_uint8, 5),


        ("roce_csum_table_lwr", c_uint8, 1),
        ("roce_enable", c_uint8, 1),
        ("payload_start", c_uint16, 14),
        ("payload_size", c_uint16, 14),
        ("auth_result", c_uint8, 1),
        ("u_elb_align_pad", c_uint8, 1),
        ("l_elb_align_pad", c_uint32, 24),
        ("stage_skip", c_uint8, 8),

        ("phv_id", c_uint16, 10),
        ("app_type", c_uint8, 4),
        ("elb_p4_rsv", c_uint8, 2),
        ("__padding", c_int8 * 13),

        ("a__hdr_vld", c_int8 * 16),

    ]


class IntTxHeader(BigEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("crc_err", c_uint8, 1),
        ("len_err", c_uint8, 4),
        ("recirc_count", c_uint8, 3),
        
        ("parser_err", c_uint8, 1),
        ("crypto_hdr", c_uint8, 1),
        ("frame_size", c_uint16, 14),
        
        ("no_data", c_uint8, 1),
        ("recirc", c_uint8, 1),
        ("packet_len", c_uint16, 14),
        
        ("a__qid", c_int8 * 3),
        
        ("dma_cmd_ptr", c_uint8, 6),
        ("qstate_addr", c_uint64, 34),
        ("qtype", c_uint8, 3),
        ("txdma_rsv", c_uint8, 5),
        ("elb_align_pad", c_uint8, 8),
        ("stage_skip", c_uint8, 8),

        ("phv_id", c_uint16, 10),
        ("app_type", c_uint8, 4),
        ("elb_txdma_rsv", c_uint8, 2),
        ("__padding", c_int8 * 29),
    ]


class IntRxHeader(BigEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("crc_err", c_uint8, 1),
        ("len_err", c_uint8, 4),
        ("recirc_count", c_uint8, 3),
        ("parser_err", c_uint8, 1),
        ("crypto_hdr", c_uint8, 1),
        ("frame_size", c_uint16, 14),

        ("no_data", c_uint8, 1),
        ("recirc", c_uint8, 1),
        ("packet_len", c_uint16, 14),

        ("a__qid", c_int8 * 3),

        ("dma_cmd_ptr", c_uint8, 6),
        ("qstate_addr", c_uint64, 34),
        ("qtype", c_uint8, 3),
        ("rx_splitter_offset", c_uint16, 10),
        ("rxdma_rsv", c_uint8, 3),
        ("stage_skip", c_uint8, 8),

        ("phv_id", c_uint16, 10),
        ("app_type", c_uint8, 4),
        ("elb_rxdma_rsv", c_uint8, 2),
        ("__padding", c_int8 * 29),
    ]


class PIPELINE(Enum):
    TXDMA_PCT = 3
    RXDMA_PCR = 2
    PGEG_SGE  = 1
    PGIG_SGI  = 0

def decode_sdp_trace_file(bytez):
    #print ("test_def\n")
    #print("\n>>> justina SDP Trace : numbytes in file : {}\n".format(len(bytez)))
    assert (isinstance(bytez, bytes))
    s = 0

    if (len(bytez) - s) < sizeof(SdpTraceFileHeader):
        print("\n>>> justina SDP Trace. Not enough bytes for SDP Trace Header.\n")

    #print("size of ctl hdr is {}".format(sizeof(SdpCtlHeader)) )   
    #print("size of Intrinsic Global hdr is {}".format(sizeof(IntGlobalHeader)) )   
    #print("size of Intrinsic P4 hdr is {}".format(sizeof(IntP4Header)) )   
    #print("size of Intrinsic TX hdr is {}".format(sizeof(IntTxHeader)) )   
    #print("size of Intrinsic RX hdr is {}".format(sizeof(IntRxHeader)) )   
        
    #Create 2 fhdr for PRD and PTD. Each fhdr has a list of trace_dict_entries    
    while s < len(bytez):
        if (len(bytez) - s) < sizeof(SdpTraceFileHeader):
            break

        # Read the file header
        fhdr = SdpTraceFileHeader.from_buffer_copy(bytez[s: s + sizeof(SdpTraceFileHeader)])
        # print(ctypes_pformat(fhdr))
        s += sizeof(SdpTraceFileHeader)
        
        print ("\n Size of SDP TraceFileHeader is {}".format(sizeof(SdpTraceFileHeader) ) )
        print("\n>>> SDP Trace HDR : 0x{:0128x}\n".format(int.from_bytes(fhdr, byteorder='big')))
        trigger_val = 0
        for fld in (fhdr._fields_):
            if not fld[0].startswith('_'):
                if fld[0].startswith('trigger'):
                    #print("{:50} 0x{:016x}".format(fld[0], getattr(fhdr, fld[0])))
                    trigger_val = trigger_val << 64 | getattr(fhdr, fld[0])
                    if fld[0].endswith('_7'):
                        k = fld[0].split('_7')
                        print("\n{}: \n 0x{:0128x}".format(k[0], trigger_val))
                        trigger_val = 0
                elif fld[0].startswith('pipeline'):
                    #print pipeline as RXDMA/TXDMA instead of 2/3
                    print("{:50} {:#x} ({})".format(fld[0], getattr(fhdr, fld[0]), PIPELINE(getattr(fhdr, fld[0])).name))
                else:
                    print("{:50} {:#x}".format(fld[0], getattr(fhdr, fld[0])))
            
        assert(fhdr.ring_size != 0)

        #first ring_size*64 has control ring. Each control ring entry is 64B
        #next ring_size*1024 has PHV ring. Each PHV in PHV ring can be upto 16 * 64B
        #ctlList = decode_ctlRing(bytez[s: s + (fhdr.ring_size * 64)])
        #decode_ctlRing(bytez[s: s + (fhdr.ring_size * 64)])
        ("ctl_ring_wr_ptr", c_uint32),
        ("phv_ring_wr_ptr", c_uint32),
        
        ctl_bytes = bytez[s: s + (fhdr.ring_size * 64)]
        s += (fhdr.ring_size * 64)
        phv_bytes = bytez[s: s + (fhdr.ring_size * 1024)]
        s += (fhdr.ring_size * 1024)

        Clist, Plist = filter_rings(ctl_bytes, phv_bytes, fhdr.ctl_ring_wr_ptr, fhdr.phv_ring_wr_ptr)
        #print(Clist)
        #print(Plist)
        b_ctl, b_phv = list_to_bytes(Clist, Plist)
        decode_rings(b_ctl, b_phv, fhdr.pipeline_num)

    return
    
def decode_ctlRing(bytez):
    assert (isinstance(bytez, bytes))
    s = 0
    print(" ")
        
    #this is a list of data structures
    trace_entries = []
    #this is a list of dictionaries
    trace_dict_entries = []
    i = 0
    entryDict = {}
    while s < len(bytez):

        Cinfo = SdpCtlHeader.from_buffer_copy(bytez[s: s + sizeof(SdpCtlHeader)])
        s += 64
        print("\n>>> SDP ctl ring data : 0x{:0128x}\n".format(int.from_bytes(Cinfo, byteorder='big')))

        for fld in (Cinfo._fields_):
            if not fld[0].startswith('_'):
                print("{:50} {:#x}".format(fld[0], getattr(Cinfo, fld[0])))
        
        print(" ")

    return
    
def decode_rings(ctl, phv, pipeline):
    #assert (isinstance(ctl, bytes))
    s = 0
    k = 0
    print(" ")
    
    #print ("len of ctl {}".format(len(ctl)))
    #print ("len of phv {}".format(len(phv)))
    
    check_ctl_phv_len(ctl, phv)

    while s < len(ctl):

        Cinfo = SdpCtlHeader.from_buffer_copy(ctl[s: s + sizeof(SdpCtlHeader)])
        s += sizeof(SdpCtlHeader)
        print("\n>>> SDP ctl ring data : 0x{:0128x}\n".format(int.from_bytes(Cinfo, byteorder='big')))

        entryDict = {}
        for fld in (Cinfo._fields_):
            if not fld[0].startswith('_'):
                #print("{:50} {:#x}".format(fld[0], getattr(Cinfo, fld[0])))
                entryDict[fld[0]] = getattr(Cinfo, fld[0])
        print(" ")
        
        Ginfo = IntGlobalHeader.from_buffer_copy(phv[k: k + sizeof(IntGlobalHeader)])
        k += sizeof(IntGlobalHeader)
        #print ("jumping k to {}".format(k))
        if pipeline is PIPELINE.TXDMA_PCT.value:
            #print("pipeline is TX")
            Pinfo = IntTxHeader.from_buffer_copy(phv[k: k + sizeof(IntTxHeader)])
        elif pipeline is PIPELINE.RXDMA_PCR.value:
            #print("pipeline is RX")
            Pinfo = IntRxHeader.from_buffer_copy(phv[k: k + sizeof(IntRxHeader)])
        else:
            #print("pipeline is P4")
            Pinfo = IntP4Header.from_buffer_copy(phv[k: k + sizeof(IntP4Header)])

        k += sizeof(IntP4Header)
        #print ("jumping k to {}".format(k))
        for fld in (Ginfo._fields_):
            if not fld[0].startswith('_'):
                #print("{:50} {:#x}".format(fld[0], getattr(Ginfo, fld[0])))
                entryDict[fld[0]] = getattr(Ginfo, fld[0])

        for fld in (Pinfo._fields_):
            if not fld[0].startswith('_'):
                if not fld[0].startswith('a__'):
                    #print("{:50} {:#x}".format(fld[0], getattr(Pinfo, fld[0])))
                    entryDict[fld[0]] = getattr(Pinfo, fld[0])
                else:    
                    q = fld[0].split('__')
                    tt = getattr(Pinfo, fld[0])
                    #print("{:50} {:#x}".format(q[1], int.from_bytes(tt, byteorder='big') ) )
                    entryDict[q[1]] = int.from_bytes(tt, byteorder='big')

        p_list = ["1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"]            
        num_phv_lines = Cinfo.phv_len-1
        for i in range(num_phv_lines):
           Pdata = phv[k: k + 64]
           #print(int.from_bytes(Pdata, byteorder='big'))
           fld_name = "phv_line" + p_list[i]
           #print(fld_name)
           entryDict[fld_name] = int.from_bytes(Pdata, byteorder='big')
           

        #print(entryDict)
        for key in (entryDict):
            if key.startswith('phv_line'):
                if key.startswith('phv_line1'):
                    print(" ")
                    print("PHV (64B in each flit excluding SOP intrinsic):")
                    print("===============================================")
                print("0x{:0128x}".format(entryDict[key]))
            else:
                if key.startswith('tm_iport'):
                    print(" ")
                    print("PHV intrinsic fields (SOP flit):")
                    print("===============================")
                print("{:50} {:#x}".format(key, entryDict[key]))

        k += (Cinfo.phv_len-1) * 64
        #print ("jumping k to {}".format(k))

    return
    
def rotate(l, n):
    return l[n:] + l[:n]


def filter_rings(ctl, phv, ctl_ptr, phv_ptr):
    assert (isinstance(ctl, bytes))
    assert (isinstance(phv, bytes))
    s = 0
    k = 0
    
    ctl_list = []
    phv_list = []

    while s < len(ctl):
        Cdata = ctl[s: s + 64]
        ctl_list.append(int.from_bytes(Cdata, byteorder='big'))
        s += 64
        #print("\n>>> Cdata : 0x{:0128x}\n".format(int.from_bytes(Cdata, byteorder='big')))

    while k < len(phv):
        Pdata = phv[k: k + 64]
        phv_list.append(int.from_bytes(Pdata, byteorder='big'))
        k += 64
        #print("\n>>> Pdata : 0x{:0128x}\n".format(int.from_bytes(Pdata, byteorder='big')))

    #print("length of ctl_list is {}".format(len(ctl_list)))
    #print("length of phv_list is {}".format(len(phv_list)))
    #print(ctl_ptr)
    #print(phv_ptr)

    r_ctl_list = rotate(ctl_list, ctl_ptr)
    r_phv_list = rotate(phv_list, phv_ptr)

    magic = 0x000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f
    f_ctl_list = []
    for s in r_ctl_list:
        if not (s == magic):
            f_ctl_list.append(s)

    f_phv_list = []
    for s in r_phv_list:
        if not (s == magic):
            f_phv_list.append(s)

    if (len(f_ctl_list) == 0):
        print("Control buffer is empty")

    if (len(f_phv_list) == 0):
        print("PHV buffer is empty")

    return (f_ctl_list, f_phv_list)
    

def list_to_bytes (c_list, p_list):
    
    #print(c_list[0])
    #print(p_list[0])

    c_bytes = bytearray()
    c_len = len(c_list)
    for i in range(c_len):
        c_bytes += c_list[i].to_bytes(64, 'big')

    #print(hex(int.from_bytes(c_bytes, byteorder='big') ) )

    p_bytes = bytearray()
    p_len = len(p_list)
    for i in range(p_len):
        p_bytes += p_list[i].to_bytes(64, 'big')

    #print(hex(int.from_bytes(p_bytes, byteorder='big') ) )

    return (c_bytes, p_bytes)


def check_ctl_phv_len (ctl, phv):
    
    phv_len = 0
    s = 0
    while s < len(ctl):

        Cinfo = SdpCtlHeader.from_buffer_copy(ctl[s: s + sizeof(SdpCtlHeader)])
        phv_len += Cinfo.phv_len
        s += sizeof(SdpCtlHeader)
        
    #print(phv_len)
    phv_size = len(phv) / 64
    #print(phv_size)
    assert phv_size == phv_len, "PHV buffer size and Control ring size valid entries mismatch"

    return
