
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
        ctlList = decode_ctlRing(bytez[s: s + (fhdr.ring_size * 64)])
        s += (fhdr.ring_size * 64)
        #phvList = decode_phvRing(bytez[s: s + (fhdr.ring_size * 1024)])
        s += (fhdr.ring_size * 1024)

    return
    
def decode_ctlRing(bytez):
    assert (isinstance(bytez, bytes))
    s = 0
        
    #this is a list of data structures
    trace_entries = []
    #this is a list of dictionaries
    trace_dict_entries = []
    i = 0
    entryDict = {}
    while s < len(bytez):

        Cinfo = (bytez[s: s + 64])
        s += 64
        print("\n>>> SDP ctl buf data : 0x{:0128x}\n".format(int.from_bytes(Cinfo, byteorder='big')))

    return
    
