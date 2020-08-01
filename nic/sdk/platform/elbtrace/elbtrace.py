#!/usr/bin/python3

import os
import re
import argparse
from enum import Enum
from trace import decode_mpu_trace_file, decode_instruction, MpuTraceHeader
from dmatrace import decode_dma_trace_file
from sdptrace import decode_sdp_trace_file
from syms import *
from itertools import product
from common import get_bits, reverse_bytes
from collections import defaultdict


parser = argparse.ArgumentParser(prog='elbtrace')

subparsers = parser.add_subparsers(dest='command')

syms_parser = subparsers.add_parser('gen_syms', help='Generate symbol file for pipeline type mentioned with --pipeline')
syms_parser.add_argument('--sym_file', default='elbtrace.syms', help='Output symbol filepath')
syms_parser.add_argument('--pipeline', help='Pipeline',
    default='iris',
    choices=['iris', 'apollo', 'apulu', 'artemis', 'athena', 'gft'])
syms_parser.add_argument('--asic', help='Asic',
    default='elba',
    choices=['capri', 'elba'])

phv_parser = subparsers.add_parser('phv', help='Parse P-vector')
phv_parser.add_argument('program', help='Program name')
phv_parser.add_argument('vec', type=lambda x: int(x, 16), help='P-vector')
phv_parser.add_argument('--swizzle', action="store_true")
phv_parser.add_argument('--sym', default='elbtrace.syms', help="Symbol file")
phv_parser.add_argument('--load', default='mpu_prog_info.json', help="Loader file")

key_parser = subparsers.add_parser('key', help='Parse K-vector')
key_parser.add_argument('program', help='Program name')
key_parser.add_argument('vec', type=lambda x: int(x, 16), help='K-vector')
key_parser.add_argument('--sym', default='elbtrace.syms', help="Symbol file")
key_parser.add_argument('--load', default='mpu_prog_info.json', help="Loader file")

data_parser = subparsers.add_parser('data', help='Parse D-vector')
data_parser.add_argument('program', help='Program name')
data_parser.add_argument('vec', type=lambda x: int(x, 16), help='D-vector')
data_parser.add_argument('--swizzle', action="store_true")
data_parser.add_argument('--sym', default='elbtrace.syms', help="Symbol file")
data_parser.add_argument('--load', default='mpu_prog_info.json', help="Loader file")

m_file_parser = subparsers.add_parser('decode_mpu', help='Parse MPU Trace file')
m_file_parser.add_argument('mpu_file', help='MPU Trace file path')
m_file_parser.add_argument('--decode_sdp', help="Parse SDP Trace file")
m_file_parser.add_argument('--sym', default='elbtrace.syms', help="Symbol file")
m_file_parser.add_argument('--load', default='mpu_prog_info.json', help="Loader file")
m_file_parser.add_argument('--fltr', nargs='+', default=list(), help='Header filters')

s_file_parser = subparsers.add_parser('decode_sdp', help='Parse SDP Trace file')
s_file_parser.add_argument('sdp_file', help='SDP Trace file path')
s_file_parser.add_argument('--sort', default="", help='Sort by fieldname')

d_file_parser = subparsers.add_parser('decode_dma', help='Parse DMA Trace file')
d_file_parser.add_argument('dma_file', help='DMA Trace file path')
d_file_parser.add_argument('--print', default="def", help='short, all, def',
                           choices=['short', 'all', 'def'])
d_file_parser.add_argument('--sort', default="", help='Sort by fieldname')

args = parser.parse_args()

class INST_SEL(Enum):
    ALU    = 3
    PHVWR  = 2
    LD_ST  = 1
    BRANCH = 0

if args.command == "gen_syms":
    sym_dir = 'build/aarch64/{}/{}/out/'.format(args.pipeline, args.asic)
    if os.path.exists(sym_dir):
        create_symbol_file(sym_dir, sym_file=args.sym_file)
    else:
        raise IOError('Build directory does not exist! {}'.format(sym_dir))
elif args.command == "phv":
    if args.sym:
        load_symbol_file(args.sym)
    else:
        create_symbol_file()

    if args.load:
        create_loader_db(args.load)
    else:
        create_loader_db()

    if args.swizzle:
        pvec = reverse_bytes(args.vec)
    else:
        pvec = args.vec

    pgm = get_program_by_name(args.program)
    for name, fld in sorted(pgm["p_struct"].items(), key=lambda f: (f[1]["end"], f[1]["name"])):
        print("{:50}\t{:#x}".format(fld["name"], get_bits(pvec, fld["start"], fld["end"])))
elif args.command == "key":
    if args.sym:
        load_symbol_file(args.sym)
    else:
        create_symbol_file()

    if args.load:
        create_loader_db(args.load)
    else:
        create_loader_db()

    pgm = get_program_by_name(args.program)
    for name, fld in sorted(pgm["k_struct"].items(), key=lambda f: (f[1]["end"], f[1]["name"])):
        print("{:50}\t{:#x}".format(fld["name"], get_bits(args.vec, fld["start"], fld["end"])))
elif args.command == "data":
    if args.sym:
        load_symbol_file(args.sym)
    else:
        create_symbol_file()

    if args.load:
        create_loader_db(args.load)
    else:
        create_loader_db()

    if args.swizzle:
        dvec = reverse_bytes(args.vec)
    else:
        dvec = args.vec

    pgm = get_program_by_name(args.program)
    for name, fld in sorted(pgm["d_struct"].items(), key=lambda f: (f[1]["end"], f[1]["name"])):
        print("{:50}\td[{:3d}:{:3d}]\t{:#x}".format(fld["name"], fld["end"], fld["start"], get_bits(dvec, fld["start"], fld["end"])))
elif args.command == "decode_mpu":
    if args.sym:
        load_symbol_file(args.sym)
    else:
        create_symbol_file()

    if args.load:
        create_loader_db(args.load)
    else:
        create_loader_db()


    def print_trace(sdp_trace):
    
        print("printing sdp_trace")
        for ts in sdp_trace:
            print("timestamp is {}".format(hex(ts)))
            print("timestamp is {}".format(ts))
            #print(sdp_trace[ts])
            print("individual fields")
            for entry in sdp_trace[ts]:
                print("entry")
                #print(entry)
                for key in (entry):
                    print("{:50} {:#x}".format(key, entry[key]))
        return



    if args.decode_sdp:
        print("decode_sdp detected")
        sort_type=""
        k = open(args.decode_sdp, "rb")
        sdp_list = decode_sdp_trace_file(k.read(), sort_type, 0)
        #print("sdp_list")
        #print(sdp_list)
        #print_trace(sdp_list)
        #print(sdp_list[1918281453])
        #print_trace(sdp_list)
####
    def hdr_sort_key(hdr):
        return hdr.sdp_pkt_id, hdr.stg, hdr.timestamp


    with open(args.mpu_file, "rb") as f:

        trace = defaultdict(list)
        
        j = 0x0
        # Decode and filter the trace
        for fhdr, hdr, key, data, instructions in decode_mpu_trace_file(f.read()):

            # Apply filters
            if not all(getattr(hdr, k) == int(v, 16) for k, v in [tuple(x.split('=')) for x in args.fltr]):
                continue

            # Save trace entry
            trace[hdr.phv_timestamp_capture].append((fhdr, hdr, key, data, instructions))


        # Print the trace
        for ts in sorted(trace):

            print()
            print("=== PHV ===")
            print()

            stg_phv = defaultdict(list)
            #check if ts entry is available in sdp_list
            if ts in sdp_list:
                #print("Timestamp match found")
                for entry in sdp_list[ts]:
                    #print("entry")
                    #for key in (entry):
                        #print("{:50} {:#x}".format(key, entry[key]))
                    #create per stage entries
                    stg_num = entry["stage_num"]
                    #print(stg_num)
                    stg_phv[stg_num].append(entry["sdp_phv"])
                    #print(entry["sdp_phv"])
                #del sdp_list[ts]

            for fhdr, hdr, key, data, instructions in sorted(trace[ts], key=lambda x: hdr_sort_key(x[1])):
                
                if hdr.stg in stg_phv:
                    print("=== STG PHV ===")
                    print("Stage Num: {}".format(hdr.stg))
                    #print(stg_phv[hdr.stg])
                    for entry in stg_phv[hdr.stg]:
                        for name, val in decode_phv(hdr.entry_pc << 6, entry):
                            print("{:50} {:#x}".format(name, val))
                        print("")
                    #delete per-stage entries after printing to avoid printing again
                    del stg_phv[hdr.stg]
                #print(stg_phv)


                # Header
                print("\n>>> HDR : 0x{:0128x}\n".format(int.from_bytes(hdr, byteorder='big')))
                for fld in sorted(hdr._fields_, key=lambda x: x[0]):
                    if not fld[0].startswith('_'):
                        print("{:50} {:#x}".format(fld[0], getattr(hdr, fld[0])))

                pgm = get_program(hdr.entry_pc << 6)
                label = get_label(hdr.entry_pc << 6)

                pgm = pgm["name"]
                label = label["name"]
                print("\n>>> PROGRAM : pc 0x{:010x} program '{:}' label '{:}'\n".format(
                    hdr.entry_pc << 6, pgm, label))
                prev_time = hdr.timestamp


                if key:
                    # Decode key
                    print("\n>>> KEY : 0x{:0128x}\n".format(key))
                    for name, val in decode_key(hdr.entry_pc << 6, key):
                        print("{:50} {:#x}".format(name, val))

                if data:
                    # Decode data
                    print("\n>>> DATA : 0x{:0128x}\n".format(data))
                    for name, val in decode_data(hdr.entry_pc << 6, data):
                        print("{:50} {:#x}".format(name, val))

                if instructions:
                    end = max([x.timestamp for x in instructions])

                    for instr in instructions:
                        instr_pgm = get_program(instr.pc << 3)
                        instr_label = get_label(instr.pc << 3)

                        # Did the program or label change?
                        if instr_pgm != pgm or instr_label != label:
                            pgm_changed = instr_pgm != pgm
                            pgm = instr_pgm
                            label = instr_label
                            print("\n>>> BRANCH : pc 0x{:010x} program '{:}' label '{:}'\n".format
                                  (instr.pc << 3, pgm["name"], label["name"]))
                            # Did the program change?
                            if pgm_changed:
                                # Decode key
                                print("\n>>> KEY : 0x{:0128x}\n".format(key))
                                for name, val in decode_key(instr.pc << 3, key):
                                    print("{:50} {:#x}".format(name, val))
                                # Decode data
                                print("\n>>> DATA : 0x{:0128x}\n".format(data))
                                for name, val in decode_data(instr.pc << 3, data):
                                    print("{:50} {:#x}".format(name, val))

                        instr_info = "[{:3d}]: {:3d}: {:+3d}: {:09x}: {:016x}  {:}".format(
                            instr.inst_count,
                            instr.timestamp - hdr.timestamp,
                            instr.timestamp - prev_time,
                            instr.pc << 3,
                            instr.opcode,
                            "X" if instr.predicate == 0 else " ")

                        instr_txt = decode_instruction(instr.pc << 3, instr.opcode)
                        instr_variants = {instr_txt}

                        p_xfrm = get_phv_field_names(instr.pc << 3, instr_txt)
                        k_xfrm = get_key_field_names(instr.pc << 3, instr_txt)
                        d_xfrm = get_data_field_names(instr.pc << 3, instr_txt)

                        d_xfrms = product(*(d_xfrm[x] for x in d_xfrm))
                        k_xfrms = product(*(k_xfrm[x] for x in k_xfrm))
                        p_xfrms = product(*(p_xfrm[x] for x in p_xfrm))

                        for xfrms_p, xfrms_k, xfrms_d in product(p_xfrms, k_xfrms, d_xfrms):
                            instr_var = instr_txt
                            for d, xfrm_d in xfrms_d:
                                instr_var = instr_var.replace(d, xfrm_d)
                            for k, xfrm_k in xfrms_k:
                                instr_var = instr_var.replace(k, xfrm_k)
                            for p, xfrm_p in xfrms_p:
                                instr_var = instr_var.replace(p, xfrm_p)
                            instr_variants.add(instr_var)

                        for instr_inst in instr_variants:
                            print("{} {}".format(
                                instr_info,
                                instr_inst,
                            ))

                        print("{}:".format(INST_SEL(instr.inst_sel).name))
                        if (INST_SEL(instr.inst_sel).name == "ALU"):
                            print("rsrcB      \t\t alu_result \t\t tsrc_src2b \t\t src1b") 
                        elif (INST_SEL(instr.inst_sel).name == "BRANCH"):
                            print("rsrc       \t\t rdst       \t\t -- \t\t\t --") 
                        elif (INST_SEL(instr.inst_sel).name == "PHVWR"):
                            print("phvwr_data \t\t phvwr_mask \t\t -- \t\t\t --") 
                        elif (INST_SEL(instr.inst_sel).name == "LD_ST"):
                            print("ld_addr    \t\t ld_data    \t\t -- \t\t\t --") 

                        print("0x{:<15x} \t 0x{:<15x} \t 0x{:<15x} \t 0x{:<15x}".format(
                            instr.rsrcB,
                            instr.alu_result,
                            instr.tsrc_src2b,
                            instr.src1b,
                        ))
                        
                        print()
                        print("=== MPU Instruction Header === ")
                        for fld in (instr._fields_):
                            if not fld[0].startswith('_'):
                                print("{:50} {:#x}".format(fld[0], getattr(instr, fld[0])))
                        print()

                        prev_time = instr.timestamp
        
elif args.command == "decode_dma":

    with open(args.dma_file, "rb") as f:

        trace = defaultdict(list)
        print_type = args.print
        sort_type = args.sort

        decode_dma_trace_file(f.read(), print_type, sort_type)

        print("decode_dma done")

elif args.command == "decode_sdp":

    with open(args.sdp_file, "rb") as f:

        trace = defaultdict(list)
        sort_type = args.sort

        sdp_list = decode_sdp_trace_file(f.read(), sort_type, 1)
        #print_sdp(sdp_list, sort_type)

        print("decode_sdp done")



