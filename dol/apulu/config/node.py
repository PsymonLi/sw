#! /usr/bin/python3
import pdb

import apollo.config.utils as utils
import apollo.config.node as node 

from apollo.config.node import client as NodeClient

def GetMatchingObjects(selectors):
    return node.GetMatchingObjects(selectors)