//:: import os, pdb, re, itertools, functools
//:: from ftl_utils import *
//:: from collections import OrderedDict
//:: pddict = _context['pddict']
//:: #pdb.set_trace()
/*
 * ftl.h
 * {C} Copyright 2019 Pensando Systems Inc. All rights reserved
 *
 * This file is generated from P4 program. Any changes made to this file will
 * be lost.
 */

#ifndef __FTL_H__
#define __FTL_H__

#include "include/sdk/base_table_entry.hpp"
#include "nic/sdk/include/sdk/mem.hpp"

//::
//::     k_d_action_data_json = {}
//::     k_d_action_data_json['INGRESS_KD'] = {}
//::     for table in pddict['tables']:
//::        if not is_table_ftl_gen(table):
//::            continue
//::        #endif
//::        if pddict['tables'][table]['direction'] == "EGRESS":
//::            continue
//::        #endif
//::        if pddict['tables'][table]['hash_overflow'] and not pddict['tables'][table]['otcam']:
//::            # Skip generating KI and KD for hash overflow table.
//::            # Overflow table will use KI and KD of regular table.
//::            continue
//::        #endif
//::
//::     ########## K + I fields ##########
//::
//::        key_fields_list = []
//::        key_fields_dict = {}
//::        if len(pddict['tables'][table]['asm_ki_fields']):
//::            pad_to_512 = 0
//::
//::            for fields in pddict['tables'][table]['asm_ki_fields']:
//::                if fields[0] == 'unionized':
//::                    ustr, uflds = fields
//::                    all_fields_of_header_in_same_byte = {}
//::                    for fields in uflds:
//::                        (multip4fldname, p4fldname, p4fldwidth, phvbit, \
//::                         flit, flitoffset, typestr, hdrname) = fields
//::                        if hdrname in all_fields_of_header_in_same_byte.keys():
//::                            all_fields_of_header_in_same_byte[hdrname] += p4fldwidth 
//::                        else:
//::                            all_fields_of_header_in_same_byte[hdrname] = p4fldwidth 
//::                        #endif
//::                    #endfor
//::                    max_fld_union_key_len = max(x for x in all_fields_of_header_in_same_byte.values())
//::                    pad_to_512 += max_fld_union_key_len 
//::
//::                    all_fields_of_header_in_same_byte = []
//::                    for fields in uflds:
//::                        (multip4fldname, p4fldname, p4fldwidth, phvbit, \
//::                         flit, flitoffset, typestr, hdrname) = fields
//::                        if p4fldwidth == max_fld_union_key_len:
//::
//::                            if typestr == 'K':
//::                                ftl_store_field(key_fields_dict, key_fields_list, multip4fldname, p4fldwidth, '')
//::                            #endif
//::                        else:
//::                            if hdrname not in all_fields_of_header_in_same_byte:
//::
//::                                all_fields_of_header_in_same_byte.append(hdrname)
//::                                _total_p4fldwidth = 0
//::                                for _fields in uflds:
//::                                    (_multip4fldname, _p4fldname, _p4fldwidth, _phvbit, \
//::                                    _flit, _flitoffset, _typestr, _hdrname) = _fields
//::                                    if _hdrname == hdrname:
//::                                        if _typestr == 'K':
//::                                            ftl_store_field(key_fields_dict, key_fields_list, _multip4fldname, _p4fldwidth, '')
//::                                        #endif
//::                                        _total_p4fldwidth += _p4fldwidth
//::                                    #endif
//::                                #endfor
//::                                padlen = max_fld_union_key_len - _total_p4fldwidth
//::                                if padlen:
//::
//::                                    pass
//::                                #endif
//::
//::                            #endif
//::                        #endif
//::                    #endfor
//::
//::                else:
//::                    (multip4fldname, p4fldname, p4fldwidth, phvbit, \
//::                    flit, flitoffset, typestr, hdrname) = fields
//::
//::                    if typestr == 'K':
//::                        ftl_store_field(key_fields_dict, key_fields_list, multip4fldname, p4fldwidth, '')
//::                    #endif
//::                    pad_to_512 += p4fldwidth
//::                #endif
//::            #endfor
//::            if pad_to_512 < 512:
//::                pad_to_512 = 512 - pad_to_512
//::
//::            #endif
//::
//::        #endif # if k+i len > 0
//::
//::     ########## K + D fields ##########
//::
//::        k_d_action_data_json['INGRESS_KD'][table] = {}
//::        _start_kd_bit = 0
//::        if len(pddict['tables'][table]['actions']) > 1:
//::            if not (pddict['tables'][table]['is_raw']):
//::                _start_kd_bit = 8
//::            #endif
//::        #endif
//::        for action in pddict['tables'][table]['actions']:
//::            data_fields_list = []
//::            data_fields_dict = {}
//::            pad_to_512 = 0
//::            (actionname, actionflddict, _) = action
//::            kd_size = 0
//::            pad_data_bits = 0
//::            totaladatabits = 0
//::            kd_json = OrderedDict()
//::            _kdbit = _start_kd_bit
//::            if len(actionflddict):
//::                totaladatabits = 0
//::                for actionfld in actionflddict:
//::                    actionfldname  = actionfld['asm_name']
//::                    actionfldwidth = actionfld['len']
//::                    totaladatabits += actionfldwidth
//::                #endfor
//::
//::                # Action Data before Key bits in case of Hash table.
//::                adata_bits_before_key = 0
//::                if not pddict['tables'][table]['is_toeplitz'] and pddict['tables'][table]['type'] == 'Hash' or pddict['tables'][table]['type'] == 'Hash_OTcam':
//::                    mat_key_start_bit = pddict['tables'][table]['match_key_start_bit']
//::                    if len(pddict['tables'][table]['actions']) > 1:
//::                        actionpc_bits = 8
//::                    else:
//::                        actionpc_bits = 0
//::                    #endif
//::                    assert(mat_key_start_bit >= actionpc_bits)
//::                    adata_bits_before_key = mat_key_start_bit - actionpc_bits
//::                    axi_pad_bits = 0
//::                    spilled_adata_bits = 0
//::                    max_adata_bits_before_key = min(totaladatabits, adata_bits_before_key)
//::                    if pddict['tables'][table]['location'] != 'HBM' and totaladatabits < mat_key_start_bit and (mat_key_start_bit - totaladatabits) > 16:
//::                        spilled_adata_bits = totaladatabits % 16
//::                        max_adata_bits_before_key = totaladatabits - spilled_adata_bits if totaladatabits > spilled_adata_bits else totaladatabits
//::                    #endif
//::                    if pddict['tables'][table]['location'] != 'HBM' and adata_bits_before_key > max_adata_bits_before_key:
//::                        # Pad axi shift amount of bits
//::                        axi_pad_bits = ((adata_bits_before_key - max_adata_bits_before_key) >> 4) << 4
//::                        if axi_pad_bits:
//::
//::                            kd_json[_kdbit] = {'bit': _kdbit, 'width': axi_pad_bits, 'field': '__pad_axi_shift_bits'}
//::                            _kdbit += axi_pad_bits
//::                            kd_size += axi_pad_bits
//::                        #endif
//::                    #endif
//::                    last_actionfld_bits = 0
//::                    if adata_bits_before_key:
//::                        total_adatabits_beforekey = 0
//::                        fill_adata = max_adata_bits_before_key
//::                        for actionfld in actionflddict:
//::                            actionfldname  = actionfld['asm_name']
//::                            actionfldwidth = actionfld['len']
//::                            dvec_start = actionfld['dvec_start']
//::                            fle_start = 511-dvec_start
//::                            little_str = ''
//::                            if actionname in pddict['tables'][table]['le_action_params'].keys():
//::                                if actionfldname in pddict['tables'][table]['le_action_params'][actionname]:
//::                                    little_str = ' (little)'
//::                                #endif
//::                            #endif
//::                            if actionfldwidth <= fill_adata:
//::                                ftl_store_field(data_fields_dict, data_fields_list, actionfldname, actionfldwidth, little_str)
//::                                kd_json[_kdbit] = {'bit': _kdbit, 'width': actionfldwidth, 'field': actionfldname}
//::                                _kdbit += actionfldwidth
//::                                fill_adata -= actionfldwidth
//::                                last_actionfld_bits = actionfldwidth
//::                                total_adatabits_beforekey += actionfldwidth
//::                            else:
//::                                ftl_actionfldname = actionfldname + "_sbit0_ebit" + str(fill_adata-1)
//::                                ftl_store_field(data_fields_dict, data_fields_list, ftl_actionfldname, fill_adata, little_str)
//::                                kd_json[_kdbit] = {'bit': _kdbit, 'width': fill_adata, 'field': actionfldname + '_sbit0_ebit' + str(fill_adata-1)}
//::                                _kdbit += fill_adata
//::                                last_actionfld_bits = fill_adata
//::                                total_adatabits_beforekey += fill_adata
//::                                fill_adata = 0
//::                            #endif
//::                            if not fill_adata:
//::                                last_actionfldname = actionfldname
//::                                break
//::                            #endif
//::                        #endfor
//::                        # Pad when adata size/bits < match-key-start-bit
//::                        if (axi_pad_bits + total_adatabits_beforekey + actionpc_bits) < mat_key_start_bit:
//::                            pad_data_bits = mat_key_start_bit - (axi_pad_bits + total_adatabits_beforekey + actionpc_bits)
//::
//::                            kd_json[_kdbit] = {'bit': _kdbit, 'width': pad_data_bits, 'field': '__pad_adata_key_gap_bits'}
//::                            _kdbit += pad_data_bits
//::                            kd_size += pad_data_bits
//::                        #endif
//::                    #endif
//::                    if pddict['tables'][table]['is_wide_key']:
//::                        _match_key_bit_length = pddict['tables'][table]['wide_key_len']
//::                    else:
//::                        _match_key_bit_length = pddict['tables'][table]['match_key_bit_length']
//::                    #endif
//::                    if pddict['tables'][table]['include_k_in_d']:
//::                        startindex = 0
//::                        endindex = len(pddict['tables'][table]['asm_ki_fields'])
//::                        keyencountered = False
//::                        for fields in pddict['tables'][table]['asm_ki_fields']:
//::                            if fields[0] == 'unionized':
//::                                ustr, uflds = fields
//::                                increment_index = False
//::                                for fields in uflds:
//::                                     (multip4fldname, p4fldname, p4fldwidth, phvbit, \
//::                                     flit, flitoffset, typestr, hdrname) = fields
//::                                     if not keyencountered:
//::                                         if typestr != 'K':
//::                                             increment_index = True
//::                                         else:
//::                                             keyencountered = True
//::                                         #endif
//::                                     #endif
//::                                #endfor
//::                                if increment_index:
//::                                    startindex += 1
//::                                #endif
//::                            else:
//::                                (multip4fldname, p4fldname, p4fldwidth, phvbit, \
//::                                flit, flitoffset, typestr, hdrname) = fields
//::                                if not keyencountered:
//::                                    if typestr != 'K':
//::                                        startindex += 1
//::                                    else:
//::                                        keyencountered = True
//::                                    #endif
//::                                #endif
//::                            #endif
//::                        #endfor
//::                        keyencountered = False
//::                        for fields in reversed(pddict['tables'][table]['asm_ki_fields']):
//::                            if fields[0] == 'unionized':
//::                                ustr, uflds = fields
//::                                decrement_index = False
//::                                for fields in uflds:
//::                                    (multip4fldname, p4fldname, p4fldwidth, phvbit, \
//::                                    flit, flitoffset, typestr, hdrname) = fields
//::                                    if not keyencountered:
//::                                        if typestr != 'K':
//::                                            decrement_index = True 
//::                                        else:
//::                                            keyencountered = True
//::                                            break
//::                                        #endif
//::                                    #endif
//::                                #endfor
//::                                if decrement_index:
//::                                    endindex -= 1
//::                                #endif
//::                            else:
//::                                (multip4fldname, p4fldname, p4fldwidth, phvbit, \
//::                                flit, flitoffset, typestr, hdrname) = fields
//::                                if not keyencountered:
//::                                    if typestr != 'K':
//::                                        endindex -= 1
//::                                    else:
//::                                        keyencountered = True
//::                                        break
//::                                    #endif
//::                                #endif
//::                            #endif
//::                        #endfor
//::                        for fields in pddict['tables'][table]['asm_ki_fields'][startindex:endindex]:
//::                            if fields[0] == 'unionized':
//::                                ustr, uflds = fields
//::                                all_fields_of_header_in_same_byte = {}
//::                                for fields in uflds:
//::                                    (multip4fldname, p4fldname, p4fldwidth, phvbit, \
//::                                     flit, flitoffset, typestr, hdrname) = fields
//::                                    if hdrname in all_fields_of_header_in_same_byte.keys():
//::                                        all_fields_of_header_in_same_byte[hdrname] += p4fldwidth 
//::                                    else:
//::                                        all_fields_of_header_in_same_byte[hdrname] = p4fldwidth 
//::                                    #endif
//::                                #endfor
//::                                max_fld_union_key_len = max(x for x in all_fields_of_header_in_same_byte.values())
//::
//::                                all_fields_of_header_in_same_byte = []
//::                                for fields in uflds:
//::                                    (multip4fldname, p4fldname, p4fldwidth, phvbit, \
//::                                     flit, flitoffset, typestr, hdrname) = fields
//::                                    if p4fldwidth == max_fld_union_key_len:
//::
//::                                        if typestr == 'K':
//::                                            ftl_store_field(data_fields_dict, data_fields_list, multip4fldname, p4fldwidth, '')
//::                                        #endif
//::                                    else:
//::                                        if hdrname not in all_fields_of_header_in_same_byte:
//::
//::                                            all_fields_of_header_in_same_byte.append(hdrname)
//::                                            _total_p4fldwidth = 0
//::                                            for _fields in uflds:
//::                                                (_multip4fldname, _p4fldname, _p4fldwidth, _phvbit, \
//::                                                _flit, _flitoffset, _typestr, _hdrname) = _fields
//::                                                if _hdrname == hdrname:
//::
//::                                                    if _typestr == 'K':
//::                                                        ftl_store_field(data_fields_dict, data_fields_list, _multip4fldname, _p4fldwidth, '')
//::                                                    #endif
//::                                                    _total_p4fldwidth += _p4fldwidth
//::                                                #endif
//::                                            #endfor
//::                                            padlen = max_fld_union_key_len - _total_p4fldwidth
//::                                            if padlen:
//::
//::                                                pass
//::                                            #endif
//::
//::                                        #endif
//::                                    #endif
//::                                #endfor
//::
//::                            else:
//::                                (multip4fldname, p4fldname, p4fldwidth, phvbit, \
//::                                flit, flitoffset, typestr, hdrname) = fields
//::
//::                                if typestr == 'K':
//::                                    ftl_store_field(data_fields_dict, data_fields_list, multip4fldname, p4fldwidth, '')
//::                                #endif
//::                            #endif
//::                        #endfor
//::                    else:
//::                        # Table Key bits -- Pad it here
//::                        ftl_store_field(data_fields_dict, data_fields_list, '__pad_key_bits', 0, '')
//::                        kd_json[_kdbit] = {'bit': _kdbit, 'width': _match_key_bit_length, 'field': '__pad_key_bits'}
//::                        _kdbit += _match_key_bit_length
//::                    #endif
//::                    kd_size += _match_key_bit_length
//::                #endif
//::                if adata_bits_before_key:
//::                    # Pack remaining Action Data bits after Key
//::                    skip_adatafld  = True
//::                    for actionfld in actionflddict:
//::                        actionfldname  = actionfld['asm_name']
//::                        actionfldwidth = actionfld['len']
//::                        dvec_start = actionfld['dvec_start']
//::                        fle_start = 511-dvec_start
//::                        little_str = ''
//::                        if actionname in pddict['tables'][table]['le_action_params'].keys():
//::                            if actionfldname in pddict['tables'][table]['le_action_params'][actionname]:
//::                                little_str = ' (little)'
//::                            #endif
//::                        #endif
//::                        if skip_adatafld and actionfldname != last_actionfldname:
//::                            continue
//::                        elif skip_adatafld:
//::                            skip_adatafld  = False
//::                            if actionfldwidth > last_actionfld_bits:
//::                                ftl_actionfldname = actionfldname + "_sbit" + str(last_actionfld_bits) + "_ebit" + str(actionfldwidth-1)
//::                                ftl_store_field(data_fields_dict, data_fields_list, ftl_actionfldname, actionfldwidth - last_actionfld_bits, little_str)
//::                                kd_json[_kdbit] = {'bit': _kdbit, 'width': actionfldwidth - last_actionfld_bits, 'field': actionfldname + '_sbit' + str(last_actionfld_bits) + '_ebit' + str(actionfldwidth - 1)}
//::                                _kdbit += (actionfldwidth - last_actionfld_bits)
//::                            #endif
//::                            continue
//::                        #endif
//::                        ftl_store_field(data_fields_dict, data_fields_list, actionfldname, actionfldwidth, '')
//::                            kd_json[_kdbit] = {'bit': _kdbit, 'width': actionfldwidth, 'field': actionfldname}
//::                            _kdbit += actionfldwidth
//::                    #endfor
//::                else:
//::                    for actionfld in actionflddict:
//::                        actionfldname  = actionfld['p4_name']
//::                        asmfldname  = actionfld['asm_name']
//::                        actionfldwidth = actionfld['len']
//::                        dvec_start = actionfld['dvec_start']
//::                        fle_start = 511-dvec_start
//::                        little_str = ''
//::                        if actionname in pddict['tables'][table]['le_action_params'].keys():
//::                            if actionfldname in pddict['tables'][table]['le_action_params'][actionname]:
//::                                little_str = ' (little)'
//::                            #endif
//::                        #endif
//::                        ftl_store_field(data_fields_dict, data_fields_list, actionfldname, actionfldwidth, little_str)
//::                        kd_json[_kdbit] = {'bit': _kdbit, 'width': actionfldwidth, 'field': actionfldname}
//::                        _kdbit += actionfldwidth
//::                    #endfor
//::                #endif
//::                kd_size += totaladatabits
//::                if len(pddict['tables'][table]['actions']) > 1:
//::                    if not (pddict['tables'][table]['is_raw']):
//::                        pad_to_512 = 512 - (8 + kd_size)
//::                    else:
//::                        pad_to_512 = 512 - (kd_size)
//::                    #endif
//::                else:
//::                    pad_to_512 = 512 - (kd_size)
//::                #endif
//::                if (pad_to_512):
//::                    if is_table_pad_256(table):
//::                        ftl_store_field(data_fields_dict, data_fields_list, '__pad_to_512b', pad_to_512 % 256, '')
//::                    else:
//::                        ftl_store_field(data_fields_dict, data_fields_list, '__pad_to_512b', pad_to_512, '')
//::                    #endif
//::                    kd_json[_kdbit] = {'bit': _kdbit, 'width': pad_to_512, 'field': '__pad_to_512b'}
//::                    _kdbit += pad_to_512
//::                #endif
//::
//::                # construct the struct names
//::                struct_name = actionname
//::                struct_full_name = struct_name + '_entry_t'

//::
//::                ######################################
//::                # KEY STRUCT
//::                ######################################
//::
//::                if is_table_gen_key(table):
struct __attribute__((__packed__)) ${struct_name}_key_entry_t {
//::                    for key_field in reversed(key_fields_list):
//::                        ftl_field_str = ftl_process_field(key_field)
    ${ftl_field_str}
//::                    #endfor
};
//::                #endif

//::                ######################################
//::                # DATA STRUCT
//::                ######################################
//::
struct __attribute__((__packed__)) ${struct_full_name} : base_table_entry_t {
//::                for data_field in reversed(data_fields_list):
//::                    # TODO remove once key is expanded for Apollo
//::                    if data_field.name() == '__pad_key_bits':
//::                        for key_field in reversed(key_fields_list):
//::                            ftl_field_str = ftl_process_field(key_field)
    ${ftl_field_str}
//::                        #endfor
//::                    else:
//::                        ftl_field_str = ftl_process_field(data_field)
    ${ftl_field_str}
//::                    #endif
//::                #endfor

#ifdef __cplusplus

public:
//::                ######################################
//::                # DATA METHODS
//::                ######################################
//::
    void copy_data(void *_s) {
        ${struct_full_name} *s = (${struct_full_name} *)_s;

//::                for data_field in reversed(data_fields_list):
//::                    # skip over key fields if part of data fields
//::                    if find_field(data_field, key_fields_list):
//::                        continue
//::                    #endif
//::                    field_name = data_field.name()
//::                    field_width = data_field.width()
//::                    # skip over non-user data
//::                    if data_field.is_key_appdata_field():
//::                        field_set_str_list = data_field.set_field_str('s->' + field_name, False)
//::                        for field_set_str in field_set_str_list:
        ${field_set_str}
//::                        #endfor
//::                    #endif
//::                #endfor
    }

    void clear_data(void) {
//::                for data_field in reversed(data_fields_list):
//::                    # skip over key fields if part of data fields
//::                    if find_field(data_field, key_fields_list):
//::                        continue
//::                    #endif
//::                    field_name = data_field.name()
//::                    field_width = data_field.width()
//::                    # skip over non-user data
//::                    if data_field.is_key_appdata_field():
//::                        field_set_str_list = data_field.set_field_str('0', False)
//::                        for field_set_str in field_set_str_list:
        ${field_set_str}
//::                        #endfor
//::                    #endif
//::                #endfor
    }

    int data2str(char *buff, uint32_t len) {
//::                format_list, args_list = ftl_field_print(data_fields_list)
        return snprintf(buff, len, "data: "
//::                if len(format_list) == 1:
//::                    format_str = format_list[0]
//::                else:
//::                    format_str = ', '.join(map(lambda format: format, format_list))
//::                #endif
                 "${format_str}",
//::                if len(args_list) == 1:
//::                    args = args_list[0]
//::                else:
//::                    args = ', '.join(map(lambda arg: arg, args_list))
//::                #endif
                 ${args});
    }

//::                ######################################
//::                # KEY METHODS
//::                ######################################
//::
//::                if is_table_gen_key(table):
    void copy_key(${struct_name}_entry_t *s) {
//::                    for key_field in reversed(key_fields_list):
//::                        field_name = key_field.name()
//::                        field_width = key_field.width()
//::                        field_set_str_list = key_field.set_field_str('s->' + field_name, False)
//::                        for field_set_str in field_set_str_list:
        ${field_set_str}
//::                        #endfor
//::                    #endfor
    }

    uint8_t get_entry_valid (void) {
        return entry_valid;
    }

    void set_entry_valid (uint8_t _entry_valid) {
        base_table_entry_t::entry_valid = entry_valid = _entry_valid;
    }

    void build_key(void *_s) {
        ${struct_full_name} *s = (${struct_full_name} *)_s;

        copy_key(s);
        clear_data();
        clear_hints();
        set_entry_valid(0);
    }

    static uint32_t entry_size(void) {
        return sizeof(${struct_full_name}) - sizeof(base_table_entry_t);
    }

    static base_table_entry_t *alloc(uint32_t size = 0) {
        void *mem;
        uint32_t _size;

        if (size == 0) {
            _size = sizeof(${struct_full_name});
        } else {
            _size = size;
        }

        mem = SDK_CALLOC(sdk::SDK_MEM_ALLOC_FTL, _size);
        if (mem == NULL) {
            return NULL;
        }

        return new (mem) ${struct_full_name}();
    }

    base_table_entry_t *construct(uint32_t size = 0) {
        return alloc(size);
    }

    void clear_key(void) {
//::                    for key_field in reversed(key_fields_list):
//::                        field_name = key_field.name()
//::                        field_width = key_field.width()
//::                        field_set_str_list = key_field.set_field_str('0', False)
//::                        for field_set_str in field_set_str_list:
        ${field_set_str}
//::                        #endfor
//::                    #endfor
    }

    void copy_key_data(void *_s) {
        ${struct_full_name} *s = (${struct_full_name} *)_s;

        copy_key(s);
        copy_data(s);
    }

    void clear_key_data(void) {
        clear_key();
        clear_data();
    }

    bool compare_key(void *_s) {
        ${struct_full_name} *s = (${struct_full_name} *)_s;

//::                    for key_field in reversed(key_fields_list):
//::                        field_width = key_field.width()
//::                        field_name = key_field.name()
//::                        if field_width > get_field_bit_unit():
//::                            arr_len = get_bit_arr_length(field_width)
        if (memcmp(${field_name}, s->${field_name}, ${arr_len})) return false;
//::                        else:
        if (${field_name} != s->${field_name}) return false;
//::                        #endif
//::                    #endfor
        return true;
    }

    int key2str(char *buff, uint32_t len) {
//::                    format_list, args_list = ftl_field_print(key_fields_list)
        return snprintf(buff, len, "key: "
//::                    if len(format_list) == 1:
//::                        format_str = format_list[0]
//::                    else:
//::                        format_str = ', '.join(map(lambda format: format, format_list))
//::                    #endif
                 "${format_str}",
//::                    if len(args_list) == 1:
//::                        args = args_list[0]
//::                    else:
//::                        args = ', '.join(map(lambda arg: arg, args_list))
//::                    #endif
                 ${args});
    }
//::                # if is_table_gen_key end
//::                #endif
//::
//::                ######################################
//::                # HASH/HINT METHODS
//::                ######################################
//::
//::                if is_table_gen_hints(table):

    void clear_hints(void) {
//::                    # TODO use setters
//::                    for data_field in reversed(data_fields_list):
//::                        field_name = data_field.name()
//::                        field_width = data_field.width()
//::                        if field_name == '__pad_to_512b':
//::                            if field_width > get_field_bit_unit():
//::                                arr_len = get_bit_arr_length(field_width)
        memset(${field_name}, 0, ${arr_len});
//::                            else:
        ${field_name} = 0;
//::                            #endif
//::                        #endif
//::                        if data_field.is_hash_hint_field():
        ${field_name} = 0;
//::                        #endif
//::                    #endfor
    }

//::                    hint_field_size = 0
//::                    hash_field_size = 0
    void set_hint_hash(uint32_t slot, uint32_t hint, uint32_t hash) {
        assert(slot);
        switch(slot) {
//::                    hash_field_cnt = ftl_hash_field_cnt()
//::                    for hash_field in range(1, hash_field_cnt):
//::                        fields = get_hash_hint_fields_db(hash_field)
//::                        if fields is None:
//::                            continue
//::                        #endif
            case ${hash_field}:
//::                        split_field_dict = {}
//::                        for field in fields:
//::                            if field.is_field_split():
//::                                split_field_name = field.split_name()
//::                                # ignore if method is already generated
//::                                if split_field_name in split_field_dict:
//::                                    continue
//::                                #endif
//::                                split_field_dict[split_field_name] = 1
//::                            #endif
//::                            if field.is_hash_field():
//::                                if field.width() > hash_field_size:
//::                                    hash_field_size = field.width()
//::                                #endif
//::                                field_set_str_list = field.set_field_str('hash')
//::                            else:
//::                                if field.width() > hint_field_size:
//::                                    hint_field_size = field.width()
//::                                #endif
//::                                field_set_str_list = field.set_field_str('hint')
//::                            #endif
//::                            for field_set_str in field_set_str_list:
                ${field_set_str}
//::                            #endfor
//::                        #endfor
                break;
//::                    #endfor
            default: more_hints = hint; more_hashes = hash; break;
        }
    }

//::                    _hash_field_size = next_pow_2(hash_field_size)
//::                    _hint_field_size = next_pow_2(hint_field_size)
    void get_hint_hash(uint32_t slot, uint32_t &hint, uint16_t &hash) {
        assert(slot);
        switch(slot) {
//::                    hash_field_cnt = ftl_hash_field_cnt()
//::                    for hash_field in range(1, hash_field_cnt):
//::                        fields = get_hash_hint_fields_db(hash_field)
//::                        if fields is None:
//::                            continue
//::                        #endif
            case ${hash_field}:
//::                        split_field_dict = {}
//::                        for field in fields:
//::                            if field.is_field_split():
//::                                split_field_name = field.split_name()
//::                                # ignore if method is already generated
//::                                if split_field_name in split_field_dict:
//::                                    continue
//::                                #endif
//::                                split_field_dict[split_field_name] = 1
//::                            #endif
//::                            if field.is_hash_field():
//::                                field_get_str_list = field.get_field_str('hash')
//::                            else:
//::                                field_get_str_list = field.get_field_str('hint')
//::                            #endif
//::                            for field_get_str in field_get_str_list:
                ${field_get_str}
//::                            #endfor
//::                        #endfor
                break;
//::                    #endfor
            default: hint = more_hints; hash = more_hashes; break;
        }
    }

    void get_hint(uint32_t slot, uint32_t &hint) {
        assert(slot);
        switch(slot) {
//::                    hash_field_cnt = ftl_hash_field_cnt()
//::                    for hash_field in range(1, hash_field_cnt):
//::                        fields = get_hash_hint_fields_db(hash_field)
//::                        if fields is None:
//::                            continue
//::                        #endif
            case ${hash_field}:
//::                        split_field_dict = {}
//::                        for field in fields:
//::                            if field.is_hash_field():
//::                                continue
//::                            else:
//::                                if field.is_field_split():
//::                                    split_field_name = field.split_name()
//::                                    # ignore if method is already generated
//::                                    if split_field_name in split_field_dict:
//::                                        continue
//::                                    #endif
//::                                    split_field_dict[split_field_name] = 1
//::                                #endif
//::                                field_get_str_list = field.get_field_str('hint')
//::                            #endif
//::                            for field_get_str in field_get_str_list:
                ${field_get_str}
//::                            #endfor
//::                        #endfor
                break;
//::                    #endfor
            default: hint = more_hints; break;
        }
    }

    uint32_t get_num_hints(void) {
//::                    hash_field_cnt = ftl_hash_field_cnt()
        return ${hash_field_cnt-1};
    }

    uint32_t get_more_hint_slot(void) {
        return get_num_hints() + 1;
    }

    bool is_hint_slot_valid(uint32_t slot) {
        if (slot && slot <= get_more_hint_slot()) {
            return true;
        }
        return false;
    }

    uint32_t find_last_hint(void) {
        if (more_hints && more_hashes) {
            return get_more_hint_slot();
//::                    hash_field_cnt = ftl_hash_field_cnt()
//::                    for hash_field in range(hash_field_cnt-1, 0, -1):
//::                        fields = get_hash_hint_fields_db(hash_field)
//::                        if fields is None:
//::                            continue
//::                        #endif
//::                        if len(fields) == 1:
//::                            field_name = fields[0].name()
//::                        else:
//::                            for field in fields:
//::                                if field.is_hint_field():
//::                                    field_name = field.name()
//::                                #endif
//::                            #endfor
//::                        #endif
        } else if (${field_name}) {
            return ${hash_field};
//::                    #endfor
        }
        return 0;
    }
//::                # if is_table_gen_hints(table):
//::                #endif

    int tostr(char *buff, uint32_t len) {
        int offset = 0;

//::                if is_table_gen_key(table):
        offset = key2str(buff, len);
        // delimiter b/w key and data
        snprintf(buff + offset, len-offset, ", ");
        offset += 3;
//::                #endif
        return data2str(buff + offset, len - offset);
    }

    void clear(void) {
//::                if is_table_gen_key(table):
        clear_key();
//::                #endif
//::                if is_table_gen_hints(table):
        clear_hints();
//::                #endif
        clear_data();
        set_entry_valid(0);
    }

//::                ######################################
//::                # SETTER METHODS
//::                ######################################
//::
//::                # To set fields if they are split
//::                split_field_dict = {}
//::                if is_table_gen_key(table) and pddict['pipeline'] != 'iris':
//::                    key_data_chain = itertools.chain(key_fields_list, data_fields_list)
//::                else:
//::                    key_data_chain = itertools.chain(data_fields_list)
//::                #endif
//::
//::                for key_data_field in key_data_chain:
//::                    # ignore if field is hash/hint/padding
//::                    if not key_data_field.is_key_appdata_field():
//::                        continue
//::                    #endif
//::                    field_name = key_data_field.name()
//::                    field_width = key_data_field.width()
//::                    field_type_str = key_data_field.field_type_str()
//::                    if key_data_field.is_field_split():
//::                        split_field_name = key_data_field.split_name()
//::
//::                        # ignore if method is already generated
//::                        if split_field_name in split_field_dict:
//::                            continue
//::                        #endif
//::
//::                        split_field_dict[split_field_name] = 1
//::                        field_set_str_list = key_data_field.set_field_str('_' + split_field_name)
    void set_${split_field_name}(${field_type_str} _${split_field_name}) {
//::                    else:
//::                        field_set_str_list = key_data_field.set_field_str('_' + field_name)
    void set_${field_name}(${field_type_str} _${field_name}) {
//::                    #endif
//::                    for field_set_str in field_set_str_list:
        ${field_set_str}
//::                    #endfor
    }

//::                #endfor
//::
//::                ######################################
//::                # GETTER METHODS
//::                ######################################
//::
//::                # To get fields if they are split
//::                split_field_dict = {}
//::                if is_table_gen_key(table) and pddict['pipeline'] != 'iris':
//::                    key_data_chain = itertools.chain(key_fields_list, data_fields_list)
//::                else:
//::                    key_data_chain = itertools.chain(data_fields_list)
//::                #endif
//
//::                for key_data_field in key_data_chain:
//::                    # ignore if field is hash/hint/padding
//::                    if not key_data_field.is_key_appdata_field():
//::                        continue
//::                    #endif
//::                    field_name = key_data_field.name()
//::                    field_width = key_data_field.width()
//::                    field_type_str = key_data_field.field_type_str()
//::                    if key_data_field.is_field_split():
//::                        split_field_name = key_data_field.split_name()
//::
//::                        # ignore if method is already generated
//::                        if split_field_name in split_field_dict:
//::                            continue
//::                        #endif
//::
//::                        split_field_dict[split_field_name] = 1
//::                        split_fields_list = get_split_field(split_field_name)
    ${field_type_str} get_${split_field_name}(void) {
        ${field_type_str} ${split_field_name} = 0x0;
//::                        for split_field in split_fields_list:
//::                            field_name = split_field.name()
//::                            sbit = split_field.sbit()
//::                            ebit = split_field.ebit()
//::                            mask = 0
//::                            for i in range(ebit-sbit+1):
//::                                mask |= (1 << i)
//::                            #endfor
//::                            mask_str = str(hex(mask))
        ${split_field_name} |= (${field_name} << ${sbit}) & ${mask_str};
//::                        #endfor
        return ${split_field_name};
//::                    else:
//::                        if field_width > get_field_bit_unit():
//::                            arr_len = get_bit_arr_length(field_width)
    void get_${field_name}(uint8_t *_${field_name}) {
        memcpy(_${field_name}, ${field_name}, ${arr_len});
        return;
//::                        else:
    ${field_type_str} get_${field_name}(void) {
        return ${field_name};
//::                        #endif
//::                    #endif
    }

//::                #endfor
#endif
};
//::
//::                # reset the global state since this in invoked multiple times
//::                ftl_hash_field_cnt_reset()
//::                split_fields_dict_reset()
//::                hash_hint_fields_dict_reset()
//::
//::            #endif
//::        k_d_action_data_json['INGRESS_KD'][table][actionname] = kd_json
//::        #endfor
//::
//::        add_kd_action_bits = False
//::        if len(pddict['tables'][table]['actions']) > 1:
//::
//::            if not (pddict['tables'][table]['is_raw']):
//::
//::                add_kd_action_bits = True
//::                k_d_action_data_json['INGRESS_KD'][table][actionname][0] = {'bit': 0, 'width': 8, 'field': "__action_pc"}
//::            #endif
//::            empty_action = True
//::            for action in pddict['tables'][table]['actions']:
//::                (actionname, actionflddict, _) = action
//::                if len(actionflddict):
//::                    empty_action = False
//::                    break
//::                #endif
//::            #endfor
//::            if not empty_action:
//::
//::                for action in pddict['tables'][table]['actions']:
//::                    (actionname, actionflddict, _) = action
//::                    if len(actionflddict):
//::                        if add_kd_action_bits:
//::                            k_d_action_data_json['INGRESS_KD'][table][actionname][0] = {'bit': 0, 'width': 8, 'field': "__action_pc"}
//::                        #endif
//::
//::                    #endif
//::                #endfor
//::
//::            #endif
//::
//::        elif len(pddict['tables'][table]['actions']) == 1:
//::            (actionname, actionflddict, _) = pddict['tables'][table]['actions'][0]
//::            if len(actionflddict):
//::                pass
//::            #endif
//::        #endif
//::     #endfor
#endif    // __FTL_H__
