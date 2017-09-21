#!/usr/bin/python
#
# Capri-Non-Compiler-Compiler (capri-ncc)
# Parag Bhide (Pensando Systems)

import argparse
import os
import sys
import pdb
import logging
from collections import OrderedDict
from enum import IntEnum

# insert the p4-hlir submodule path
_top_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
sys.path.insert(1, _top_dir + '/p4-hlir')

from p4_hlir.main import HLIR
import p4_hlir.hlir.p4 as p4
import p4_hlir.hlir.table_dependency as table_dependency
import capri_logging, capri_tables, capri_parser, capri_pa, capri_deparser
from capri_utils import *
from capri_model import capri_model as capri_model
from capri_output import capri_model_dbg_output as capri_model_dbg_output
from capri_p4pd import capri_p4pd_code_generate as capri_p4pd_code_generate

def get_parser():
    parser = argparse.ArgumentParser(description='p4 compiler for Capri')
    parser.add_argument('sources', metavar='source', type=str, nargs='*',
                        help='P4 source file'
                        'P4 program')
    parser.add_argument('--log', dest='loglevel', type=str,
                        help='Set console logging level (DEBUG, INFO, WARNING, ERROR, CRITICAL',
                        default='INFO', required=False)
    parser.add_argument('--flog', dest='floglevel', type=str,
                        help='Set File logging level (DEBUG, INFO, WARNING, ERROR, CRITICAL',
                        default='DEBUG', required=False)
    parser.add_argument('--no-ohi', dest='no_ohi', action='store_true',
                        help='Do not use OHI - everything in phvs (for testing)',
                        default=False, required=False)
    parser.add_argument('--asm-out', dest='asm_out', action='store_true',
                        help='Generate header files for capri-assembler',
                        default=False, required=False)
    parser.add_argument('--post-extract', dest='post_extract', action='store_true',
                        help='Depricated - Extract headers in a state while entering the state',
                        default=False, required=False)
    parser.add_argument('--p4-plus', dest='p4_plus', action='store_true',
                        help='Compile P4+ program',
                        default=False, required=False)
    parser.add_argument('--p4-plus-module', dest='p4_plus_module', action='store',
                        help='Generate configuration for P4+ program',
                        choices=['rxdma', 'txdma'],
                        default=None, required=False)
    parser.add_argument('--i2e-user', dest='i2e_user', action='store_true',
                        help='Use i2e metadata specified by the user',
                        default=False, required=False)
    parser.add_argument('--old-phv-allocation', dest='old_phv_allocation', action='store_true',
                        help='Use old phv allocation logic (Will be removed)',
                        default=False, required=False)
    parser.add_argument('--pd-gen', dest='pd_gen', action='store_true',
                        help='Generate P4-PD code',
                        default=False, required=False)
    parser.add_argument('--no-te-bit2byte', dest='no_te_bit2byte', action='store_true',
                        help='Do not convert K and I bit extractions to byte',
                        default=False, required=False)
    parser.add_argument('--two-byte-profile', dest='two_byte_profile', action='store_true',
                        help='Use Two-Byte-Mode for km_profiles',
                        default=False, required=False)
    parser.add_argument('--gen-dir', dest='gen_dir', action='store',
                        help='Directory for all Compiler generated code',
                        default='.', required=False)
    parser.add_argument('--cfg-dir', dest='cfg_dir', action='store',
                        help='Directory for compiler generated configuration',
                        default=None, required=False)
    return parser

# Main back-end class that holds everything needed by the backend
# provides a easy way to get to required information
# keeps a back reference to hlir
class capri_backend:
    def __init__(self, hlir=None, hw_model=None, args=None):
        self.h= hlir
        self.logger = logging.getLogger('BE')
        self.hw_model = hw_model
        self.args = args
        self.pa = capri_pa.capri_pa(self)
        self.ma = None
        self.tables = capri_tables.capri_table_manager(self)
        self.parsers = [capri_parser.capri_parser(self, d) for d in xgress]
        self.deparsers = [capri_deparser.capri_deparser(self, d) for d in xgress]
        prog_name = os.path.split(args.sources[0])
        self.prog_name = prog_name[1].replace('.p4', '')

    def initialize(self):
        # collect information from hlir into various objects
        self.pa.initialize()
        self.tables.initialize_tables()
        for p in self.parsers:
            p.initialize()
        for dp in self.deparsers:
            dp.initialize()
        self.pa.initialize_unions()

    def model_dbg_output(self):
        pa_dbg = self.pa.phv_dbg_output()
        parser_dbg = OrderedDict()
        for d in xgress:
            parser_dbg[d.name] = self.parsers[d].parser_dbg_output()
        te_dbg = self.tables.tm_dbg_output()

        dbg_info = OrderedDict()
        dbg_info["PHV"] = pa_dbg
        dbg_info["Parser"] = parser_dbg
        dbg_info["TableEngine"] = te_dbg
        # add deparser...

        capri_model_dbg_output(self, dbg_info)

def setup_p4_plus_hw_parameters(capri_model):
    capri_model['phv']['num_flits'] = 12
    max_phv_bits = 12*512
    capri_model['phv']['max_size'] = max_phv_bits
    max_phv_bytes = max_phv_bits / 8
    capri_model['phv']['containers'] = {8: max_phv_bytes} # {size:num} all 8 bit containers
    capri_model['match_action']['num_stages'] = 8
    # set up parser params as well..code uses parser flits for phv allocation
    capri_model['parser']['parser_num_flits'] = capri_model['phv']['num_flits'] / 2
    

def main():
    args = get_parser().parse_args()
    prog_name = os.path.split(args.sources[0])
    prog_name = prog_name[1].replace('.p4', '')
    capri_logging.logger_init(log_dir=args.gen_dir, prog_name=prog_name, 
                                loglevel=args.loglevel, floglevel=args.floglevel)

    # TBD - Insert toplevel try-except block
    h = HLIR(*args.sources)
    if not h.build():
        sys.exit(1)

    if args.p4_plus:
        setup_p4_plus_hw_parameters(capri_model)

    capri_be = capri_backend(h, capri_model, args)

    capri_be.initialize()

    # run passes from here
    # create initial field ordering based on parser extraction order and table usage
    capri_be.pa.init_field_ordering()
    re_init = False

    # assign ohi slots
    for d in xgress:
        # make sure each state uses max ohi slots allowed by the hw
        capri_be.parsers[d].update_ohi_per_state()
        max_ohi, max_ohi_path  = capri_be.parsers[d].assign_ohi_slots()
        ohi_threshold = capri_be.hw_model['parser']['ohi_threshold']
        # This is just to test multiple passes thru' ohi allocation
        # converting more ohis to phv will increase phv requirement and may exceed phv limits
        max_retry = 4
        while max_ohi > ohi_threshold and max_retry:
            # convert N ohis along longest path to phvs and try again
            capri_be.parsers[d].ohi_to_phv(max_ohi_path, max_ohi - ohi_threshold)
            # check again
            max_ohi, max_ohi_path  = capri_be.parsers[d].assign_ohi_slots()
            max_retry -= 1
            re_init = True
        assert max_ohi <= ohi_threshold, "Cannot bring down the ohi count"

    if re_init:
        # re-init the field order after ohi fields have changed
        capri_be.pa.init_field_ordering()

    capri_be.pa.create_flits()

    for d in xgress:
        capri_be.parsers[d].assign_hv_bits()
        capri_be.parsers[d].program_capri_states()
        capri_be.deparsers[d].build_field_dictionary()

    capri_be.tables.update_table_config()
    capri_be.tables.program_tables()

    # Generate various outputs
    for d in xgress:
        if not capri_be.args.p4_plus:
            capri_be.parsers[d].generate_output()
            #capri_be.parsers[d].print_long_paths(10)
            #capri_be.parsers[d].print_short_paths(10)
            #capri_be.parsers[d].print_path_histogram(5)
        capri_be.pa.gress_pa[d].update_phc_map()
        capri_be.pa.gress_pa[d].print_field_order_info("PHV order(Final)")
        if not capri_be.args.p4_plus:
            capri_be.deparsers[d].print_deparser_info()
            capri_be.deparsers[d].generate_output()

    capri_be.tables.generate_output()

    # generate debug information for model
    # output into a JSON file for model debug logs
    capri_be.model_dbg_output()    

    if args.asm_out:
        capri_be.pa.capri_asm_output()
        capri_be.tables.capri_asm_output()

    if args.pd_gen or args.asm_out:
        capri_p4pd_code_generate(capri_be)

main()
