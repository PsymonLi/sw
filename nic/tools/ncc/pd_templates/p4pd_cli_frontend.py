//:: import os, pdb
//:: from collections import OrderedDict
//:: pddict = _context['pddict']
//:: #pdb.set_trace()
//::
//:: if pddict['p4plus']:
//::    api_prefix = 'p4pd_' + pddict['p4program']
//:: else:
//::    api_prefix = 'p4pd'
//:: #endif
//::
#!/usr/bin/python
# This file is auto-generated. Changes will be overwritten!
#
# Debug CLI
#

//:: register_list =  ["ppa","sgi","rpc","intr","pxb","sge","pr","pp","pt","tsi","pcr","txs","tse","pct","pb","pm","db","ssi","sse","bx","md","tpc","dpr","mc","dpp","sema","mp", "ms","mx"]
//::
import click
from click_repl import register_repl
import ${pddict['cli-name']}_backend
from cli_frontend import *

array_cols = 16

${api_prefix}_table_types_enum = [
    'HASH',
    'HASH_OTCAM',
    'TERNARY',
    'INDEX',
    'MPU',
]

//::    for table in pddict['tables']:
@read.group()
@click.pass_context
def ${table}(ctx):
    ctx.obj['table_name'] ='${table}'
    pass

@${table}.group(invoke_without_command=True)
@click.argument('ids', metavar='${table}_id')
@click.pass_context
def ${table}_index(ctx, ids):
    config = {}
    config['table_name'] = ctx.obj['table_name']
    config['action_name'] = ''
    config['opn'] = ctx.obj['opn']
    config['index'] = int(ids,0)
    config['actionid_vld'] = False
    config['actionfld'] = {}
    config['swkey']={}
    config['swkey_mask']={}
    ${pddict['cli-name']}_backend.populate_table(config)
    pass

@${table}.group(invoke_without_command=True)
@click.pass_context
def all(ctx):
    config = {}
    config['table_name'] = ctx.obj['table_name']
    config['action_name'] = ''
    config['opn'] = ctx.obj['opn']
    config['index'] = 0xffffffff
    config['actionid_vld'] = False
    config['actionfld'] = {}
    config['swkey']={}
    config['swkey_mask']={}
    ${pddict['cli-name']}_backend.populate_table(config)
    pass

//::    #endfor
//::    for table in pddict['tables']:
@write.group()
@click.pass_context
def ${table}(ctx):
    ctx.obj['table_name']='${table}'
    pass

@${table}.group(invoke_without_command=True)
@click.argument('ids', metavar='${table}_id')
@click.pass_context
def ${table}_index(ctx, ids):
    pass

@${table}_index.resultcallback()
@click.pass_context
def process_results(ctx, rslt, ids):
    config = {}
    config['action_name'] = ''
    config['table_name'] = ctx.obj['table_name']
    config['opn'] = ctx.obj['opn']
    config['index'] = int(ids,0)
    config['action_name'] = ctx.obj['action_name']
    config['actionid_vld'] = False
    config['actionfld'] = {}
    config['swkey'] = {}
    config['swkey_mask'] = {}
    for arg, value in rslt:
        config['actionfld'][arg] = value
    ${pddict['cli-name']}_backend.populate_table(config)

//::        for action in pddict['tables'][table]['actions']:
//::            (actionname, actionfldlist) = action
//::            fld_len = len(actionfldlist)
//::            if fld_len == 0:
@${table}_index.group(invoke_without_command=True, chain=True)
//::            else:
@${table}_index.group(chain=True)
//::            #endif
@click.pass_context
def ${actionname}(ctx):
    ctx.obj['action_name'] =  '${actionname}'
    pass

//::            if (len(actionfldlist) > 0):
//::                for actionfld in actionfldlist:
//::                    actionfldname,actionfldwidth = actionfld
@${actionname}.command(name='${actionfldname}')
@click.argument('arg_${actionfldname}', type=click.STRING)
def ${actionfldname}_cmd(arg_${actionfldname}):
    return ('${actionfldname}', arg_${actionfldname})

//::                #endfor
//::            #endif
//::        #endfor
//::    #endfor
