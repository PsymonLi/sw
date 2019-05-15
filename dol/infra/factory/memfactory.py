#! /usr/bin/python3
import pdb
import copy
import infra.factory.template   as template
import infra.common.loader      as loader
import infra.common.objects     as objects

from infra.factory.store    import FactoryStore
from infra.common.logging   import logger

def init():
    logger.info("Loading Descriptor templates.")
    objlist = template.ParseDescriptorTemplates()
    FactoryStore.templates.SetAll(objlist)

    logger.info("Loading Buffer templates.")
    objlist = template.ParseBufferTemplates()
    FactoryStore.templates.SetAll(objlist)

def __resolve_refs_list(v, tc):
    for e in v:
        # List must be of objects with more refs.
        assert(objects.IsFrameworkTemplateObject(e))
        __resolve_refs_obj(e, tc)
    return

def __resolve_refs_obj(obj, tc):
    if obj is None:
        return
    for a, v in obj.__dict__.items():
        if objects.IsReference(v):
            val = v.Get(tc)
            logger.info("Resolving spec.fields.%s = " % a, val)
            obj.__dict__[a] = val
        elif objects.IsCallback(v):
            val = v.call(tc, obj)
            logger.info("Resolving spec.fields.%s = " % a, val)
            obj.__dict__[a] = val
        elif isinstance(v, list):
            __resolve_refs_list(v, tc)
        elif objects.IsFrameworkTemplateObject(v):
            __resolve_refs_obj(v, tc)
        else:
            # Native types
            continue
    return

def __generate_common(tc, spec, write = True):
    spec = copy.deepcopy(spec)
    template = spec.template.Get(FactoryStore)
    obj = template.CreateObjectInstance()
    obj.GID(spec.id)
    
    logger.info("Created MemFactoryObject: %s" % obj.GID())
    # Resolve all the references.
    __resolve_refs_obj(spec.fields, tc)

    obj.Init(spec)
    if write:
        obj.Write()
    return obj

def __generate_descriptors_from_callback(tc, descr_entry):
    cb = getattr(descr_entry, 'callback', None)
    if cb is None:
        return
    descrs = cb.call(tc)
    if descrs:
        tc.descriptors.SetAll(descrs)
    return

def __generate_descriptors_from_spec(tc, desc_entry):
    dspec = getattr(desc_entry, 'descriptor', None)
    if dspec is None:
        return
    if dspec.id is None:
        return
    obj = __generate_common(tc, desc_entry.descriptor, write = False)
    tc.descriptors.Add(obj)

def GenerateDescriptors(tc):
    logger.info("Generating Descriptors")
    for desc_entry in tc.testspec.descriptors:
        __generate_descriptors_from_callback(tc, desc_entry)
        __generate_descriptors_from_spec(tc, desc_entry)
    return

def __generate_buffers_from_spec(tc, buff_entry):
    bspec = getattr(buff_entry, 'buffer', None)
    if bspec is None:
        return
    if bspec.id == None:
        return
    obj = __generate_common(tc, buff_entry.buffer)
    tc.buffers.Add(obj)
    return
   
def __generate_buffers_from_callback(tc, buff_entry):
    cb = getattr(buff_entry, 'callback', None)
    if cb is None:
        return
    bufs = cb.call(tc)
    if bufs:
        tc.buffers.SetAll(bufs)
    return

def GenerateBuffers(tc):
    logger.info("Generating Buffers")
    for buff_entry in tc.testspec.buffers:
        __generate_buffers_from_spec(tc, buff_entry)
        __generate_buffers_from_callback(tc, buff_entry)
    return
