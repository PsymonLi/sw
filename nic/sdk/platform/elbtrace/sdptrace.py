
from ctypes import *
from pprint import pformat
import os.path
from enum import Enum
from collections import defaultdict
from common import get_bits, get_bit, reverse_bytes

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
        ("local_timestamp", c_uint64, 48),
    ]


#17 Bytes
class IntGlobalHeader(BigEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("tm_iport", c_uint8, 4),
        ("tm_oport", c_uint8, 4),
        
        ("tm_iq", c_uint8, 5),
        ("lif", c_uint16, 11),
        ("timestamp", c_uint64, 48),

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

#todo: check the values
class PIPELINE(Enum):
    SXDMA_XG  = 4
    TXDMA_PCT = 3
    RXDMA_PCR = 2
    P4EG_SGE  = 1
    P4IG_SGI  = 0

def decode_sdp_trace_file(bytez, sort_type, print_type):
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

    sdp_trace = defaultdict(list)
        
    while s < len(bytez):
        if (len(bytez) - s) < sizeof(SdpTraceFileHeader):
            break

        # Read the file header
        fhdr = SdpTraceFileHeader.from_buffer_copy(bytez[s: s + sizeof(SdpTraceFileHeader)])
        # print(ctypes_pformat(fhdr))
        s += sizeof(SdpTraceFileHeader)
        
            
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
        sdp_trace = decode_rings(b_ctl, b_phv, fhdr, sort_type, sdp_trace, print_type)
        #print_trace(sdp_trace)
        
    #print_trace(sdp_trace)
    return sdp_trace
    
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
    
def decode_rings(ctl, phv, fhdr, sort_type, sdp_trace, print_type):
    #assert (isinstance(ctl, bytes))
    s = 0
    k = 0
    pipeline = fhdr.pipeline_num
    if print_type:
        print_fhdr(pipeline, fhdr)
    #print(" ")
    
    #print ("len of ctl {}".format(len(ctl)))
    #print ("len of phv {}".format(len(phv)))
    
    check_ctl_phv_len(ctl, phv)
    mpuDict = {}
    mpuDict["pipeline_num"] = fhdr.pipeline_num
    mpuDict["stage_num"]    = fhdr.stage_num

    pipeline_num = fhdr.pipeline_num
    stage_num = fhdr.stage_num
    
    full_list = []

    while s < len(ctl):

        Cinfo = SdpCtlHeader.from_buffer_copy(ctl[s: s + sizeof(SdpCtlHeader)])
        Cbytes = int.from_bytes(ctl[s: s + sizeof(SdpCtlHeader)], byteorder='big')
        s += sizeof(SdpCtlHeader)

        entryDict = {}
        entryDict["cinfo"] = Cbytes
        for fld in (Cinfo._fields_):
            if not fld[0].startswith('_'):
                #print("{:50} {:#x}".format(fld[0], getattr(Cinfo, fld[0])))
                entryDict[fld[0]] = getattr(Cinfo, fld[0])
        #print(" ")
        
        Ginfo = IntGlobalHeader.from_buffer_copy(phv[k: k + sizeof(IntGlobalHeader)])
        #k += sizeof(IntGlobalHeader)
        q = k + sizeof(IntGlobalHeader)
        #print ("jumping k to {}".format(k))
        if pipeline is PIPELINE.TXDMA_PCT.value:
            #print("pipeline is TX")
            Pinfo = IntTxHeader.from_buffer_copy(phv[q: q + sizeof(IntTxHeader)])
        elif pipeline is PIPELINE.RXDMA_PCR.value:
            #print("pipeline is RX")
            Pinfo = IntRxHeader.from_buffer_copy(phv[q: q + sizeof(IntRxHeader)])
        else:
            #print("pipeline is P4")
            Pinfo = IntP4Header.from_buffer_copy(phv[q: q + sizeof(IntP4Header)])

            
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

        #k += sizeof(IntP4Header)
        p_list = ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"]            
        num_phv_lines = Cinfo.phv_len
        num_bytes = Cinfo.phv_len * 64
        sdp_phv = int.from_bytes(phv[k: k + num_bytes], byteorder='big')
        entryDict["full_phv"] = sdp_phv
        mpuDict["full_phv"]  = sdp_phv

        timestamp = Ginfo.timestamp
        
        jkDict = {}
        jkDict["pipeline_num"] = fhdr.pipeline_num
        jkDict["stage_num"]    = fhdr.stage_num
        jkDict["sdp_phv"]      = sdp_phv
        #sdp_trace[timestamp].append((pipeline_num, stage_num, sdp_phv))
        sdp_trace[timestamp].append((jkDict))
        #sdp_trace[1918281453].append((jkDict))
        
        #print("k--")
        #print(k)
        jk = k
        for i in range(num_phv_lines):
           Pdata = phv[jk: jk + 64]
           #print(int.from_bytes(Pdata, byteorder='big'))
           fld_name = "phv_line" + p_list[i]
           #print(fld_name)
           entryDict[fld_name] = int.from_bytes(Pdata, byteorder='big')
           jk += 64
           
        #if not sort_type:
        #    sortedList = entryDict
        #else:
        #    sortedList = entryDict
            #print("sort_type")
            #print(sort_type)
            #print(entryDict)
            #sortedList = sorted(entryDict, key=lambda x: x[sort_type])

        #if print_type:
        #    print_phv(sortedList)

        k += Cinfo.phv_len * 64
        #print ("jumping k to {}".format(k))
        full_list.append(entryDict)

    #print("mpuDict")
    #for key in (mpuDict):
        #print("{:50} {:#x}".format(key, mpuDict[key]))
    
    if not sort_type:    
        sortedList = full_list
    else:
        sortedList = sorted(full_list, key=lambda x: x[sort_type])
    if print_type:
        for s in sortedList:
            print_phv(s)

    return sdp_trace
    
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
        ctl_list.append(int.from_bytes(Cdata, byteorder='little'))
        s += 64
        #print("\n>>> Cdata : 0x{:0128x}\n".format(int.from_bytes(Cdata, byteorder='little')))

    while k < len(phv):
        Pdata = phv[k: k + 64]
        phv_list.append(int.from_bytes(Pdata, byteorder='little'))
        k += 64
        #print("\n>>> Pdata : 0x{:0128x}\n".format(int.from_bytes(Pdata, byteorder='little')))

    #print("length of ctl_list is {}".format(len(ctl_list)))
    #print("length of phv_list is {}".format(len(phv_list)))
    #print(ctl_ptr)
    #print(phv_ptr)

    r_ctl_list = rotate(ctl_list, ctl_ptr)
    r_phv_list = rotate(phv_list, phv_ptr)

    # SDP doesn't swizzle bytes when writing on AXI bus. So swizzle the init_val here to compensate for it
    init_val = reverse_bytes(0x000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f) << 8
    #print("init_val")
    #print(hex(init_val))
    f_ctl_list = []
    for s in r_ctl_list:
        if not (s == init_val):
            f_ctl_list.append(s)
            #print("\n>>> Cdata_rotated : 0x{:0128x}\n".format(s))

    f_phv_list = []
    for s in r_phv_list:
        if not (s == init_val):
            f_phv_list.append(s)
            #print("\n>>> Pdata_rotated : 0x{:0128x}\n".format(s))

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

    #print("c_bytes")
    #print(hex(int.from_bytes(c_bytes, byteorder='big') ) )

    p_bytes = bytearray()
    p_len = len(p_list)
    for i in range(p_len):
        p_bytes += p_list[i].to_bytes(64, 'big')

    #print("p_bytes")
    #print(hex(int.from_bytes(p_bytes, byteorder='big') ) )

    return (c_bytes, p_bytes)


def check_ctl_phv_len (ctl, phv):
    
    phv_len = 0
    s = 0
    while s < len(ctl):

        Cinfo = SdpCtlHeader.from_buffer_copy(ctl[s: s + sizeof(SdpCtlHeader)])
        phv_len += Cinfo.phv_len
        s += sizeof(SdpCtlHeader)
        
    #print("ctl_len")
    #print(phv_len)
    phv_size = len(phv) / 64
    #print("phv_size")
    #print(phv_size)
    assert phv_size >= phv_len, "PHV buffer size and Control ring size valid entries mismatch"

    return



def print_fhdr(pipeline, fhdr):

    #print ("\n Size of SDP TraceFileHeader is {}".format(sizeof(SdpTraceFileHeader) ) )
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
                    print_intrinsic(pipeline, trigger_val)
                    trigger_val = 0
            elif fld[0].startswith('pipeline'):
                #print pipeline as RXDMA/TXDMA instead of 2/3
                print("{:50} {:#x} ({})".format(fld[0], getattr(fhdr, fld[0]), PIPELINE(getattr(fhdr, fld[0])).name))
            else:
                print("{:50} {:#x}".format(fld[0], getattr(fhdr, fld[0])))

    return


def print_phv(sortedList):

    for key in (sortedList):
        
        if key.startswith('full_phv'):
            print(" ")
        elif key.startswith('cinfo'):
            print("\n>>> SDP ctl ring data : 0x{:0128x}\n".format(sortedList[key]))
        elif key.startswith('phv_line'):
            if key.startswith('phv_line0'):
                print(" ")
                print("PHV (64B in each flit including SOP intrinsic):")
                print("===============================================")
            print("0x{:0128x}".format(sortedList[key]))
            #print("{:50} 0x{:0128x}".format(key, sortedList[key]))
        else:
            if key.startswith('tm_iport'):
                print(" ")
                print("PHV intrinsic fields (SOP flit):")
                print("===============================")
            print("{:50} {:#x}".format(key, sortedList[key]))

    return


def print_trace(sdp_trace):
    
    print("printing sdp_trace")
    for ts in sdp_trace:
        print("timestamp is {}".format(hex(ts)))
        #print(sdp_trace[ts])
        print("individual fields")
        for entry in sdp_trace[ts]:
            print("entry")
            #print(entry)
            for key in (entry):
                print("{:50} {:#x}".format(key, entry[key]))
        #for pipeline_num, stage_num, sdp_phv in sdp_trace[ts]:
        #for stage_num in sdp_trace[ts]:
            #if pipeline_num:
                #print("pipeline")
                #print(pipeline_num)
            #elif stage_num:
            #print("stage")
            #print(stage_num)
            #elif sdp_phv:
                #print("phv")
                #print(sdp_phv)

    return

def print_intrinsic(pipeline, phv_sop_flit):

    #print("\n VAL: \n 0x{:0128x}".format(phv_sop_flit))
    #num_str = "{0:b}".format(phv_sop_flit)
    #print("\n size: \n {}".format(len(num_str)))

    #a = 1 << 511 | phv_sop_flit
    #a = phv_sop_flit
    #num_str = "{0:b}".format(a)
    #print("\n size: \n {}".format(len(num_str)))
    #b = get_bits(phv_sop_flit,0,135)
    #print(b)
    
    c_bytes = bytearray()
    c_bytes += phv_sop_flit.to_bytes(64, 'big')
    #print(c_bytes)
    
    k = 0
    Ginfo = IntGlobalHeader.from_buffer_copy(c_bytes[k: k + sizeof(IntGlobalHeader)])
    k += sizeof(IntGlobalHeader)
    #print(Ginfo)
    if pipeline is PIPELINE.TXDMA_PCT.value:
        #print("pipeline is TX")
        Pinfo = IntTxHeader.from_buffer_copy(c_bytes[k: k + sizeof(IntTxHeader)])
    elif pipeline is PIPELINE.RXDMA_PCR.value:
        #print("pipeline is RX")
        Pinfo = IntRxHeader.from_buffer_copy(c_bytes[k: k + sizeof(IntRxHeader)])
    else:
        #print("pipeline is P4")
        Pinfo = IntP4Header.from_buffer_copy(c_bytes[k: k + sizeof(IntP4Header)])

    for fld in (Ginfo._fields_):
        if not fld[0].startswith('_'):
            if not (getattr(Ginfo, fld[0]) == 0):
                print("{:50} {:#x}".format(fld[0], getattr(Ginfo, fld[0])))

    for fld in (Pinfo._fields_):
        if not fld[0].startswith('_'):
            if not fld[0].startswith('a__'):
                if not (getattr(Pinfo, fld[0]) == 0):
                    print("{:50} {:#x}".format(fld[0], getattr(Pinfo, fld[0])))
                    print("test")
            else:    
                q = fld[0].split('__')
                tt = getattr(Pinfo, fld[0])
                if not (int.from_bytes(tt, byteorder='big') == 0):
                    print("{:50} {:#x}".format(q[1], int.from_bytes(tt, byteorder='big') ) )
                    
    return
