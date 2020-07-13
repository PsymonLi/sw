#! /usr/bin/python3
import pdb

from infra.common.logging import logger as logger

def Setup(infra, module):
    iterelem = module.iterator.Get()

    logger.info("Iterator Selectors")
    if iterelem:
        if 'flow' in iterelem.__dict__:
            logger.info("Extending flow selector to : %s" % iterelem.flow)
            module.testspec.selectors.flow.Extend(iterelem.flow)
        if 'vnic' in iterelem.__dict__:
            logger.info("Extending vnic selector to : %s" % iterelem.vnic)
            if not hasattr(module.testspec.selectors, 'vnic'):
                module.testspec.selectors.vnic = iterelem.vnic
            else:
                module.testspec.selectors.vnic.Extend(iterelem.vnic)
        if 'route' in iterelem.__dict__:
            logger.info("Extending route selector to : %s" % iterelem.route)
            module.testspec.selectors.route.Extend(iterelem.route)
        if 'policy' in iterelem.__dict__:
            logger.info("Extending policy selector to : %s" % iterelem.policy)
            module.testspec.selectors.policy.Extend(iterelem.policy)
        if 'node' in iterelem.__dict__:
            logger.info("Extending node selector to : %s" % iterelem.node)
            module.testspec.selectors.node.Extend(iterelem.node)