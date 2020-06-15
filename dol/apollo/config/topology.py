#! /usr/bin/python3
import infra.common.parser as parser
import apollo.config.resmgr as resmgr
import apollo.config.utils as utils
import pdb

from apollo.config.node import client as NodeClient
from infra.common.logging import logger as logger
from infra.common.glopts import GlobalOptions
from apollo.config.store import EzAccessStore

def __get_topo_file():
    topo_file = '%s.topo' % GlobalOptions.topology
    return topo_file

def __get_topo_path(default=False):
    pipeline = utils.GetPipelineName()
    topo_file = f'{pipeline}/config/topology/'
    return topo_file

def __get_topo_spec():
    topofile = __get_topo_file()
    path = __get_topo_path()
    topospec = parser.ParseFile(path, topofile)
    if not topospec:
        logger.error("Invalid topofile %s" % (topofile))
        assert(0)
        return None
    return topospec

def Main():
    topospec = __get_topo_spec()
    # TODO: Fix topos to have node name
    # Add node class
    dutnode = "Node%d" % topospec.dutnode
    EzAccessStore.SetDUTNode(dutnode)
    
    for nodespec in topospec.node:
        nodename = "Node%d" % nodespec.id
        NodeClient.CreateNode(nodename, nodespec)
    return