#! /usr/bin/python3

import infra.common.parser      as parser
import infra.common.objects     as objects

from infra.common.logging       import logger
from infra.common.glopts        import GlobalOptions
from infra.engine.modmgr        import ModuleStore

FeatureStore = objects.ObjectDatabase(logger)
BaseTopoExcludeFeatureList = [
    'dolut',
    'fte',
    'norm',
    'eth' ,
    'acl',
    'networking',
    'vxlan',
    'ipsg',
    'firewall',
    'proxy',
    'ipsec',
    'hostpin',
    'l2mc'
]

class FeatureObject:
    def __init__(self, spec):
        self.spec       = spec
        self.id         = spec.feature.id
        self.package    = spec.feature.package
        self.module     = spec.feature.module
        self.enable     = spec.feature.enable
        self.ignore     = spec.feature.ignore
        self.sub        = getattr(spec.feature, 'sub', None)
        self.testspec   = getattr(spec.feature, 'spec', None)
        self.args       = getattr(spec.feature, 'args', None)
        return

    def LoadModules(self):
        for m in self.spec.modules:
            mspec = m.module
            if self.sub is None:
                mspec.feature = self.id
            else:
                mspec.feature = "%s/%s" %(self.id, self.sub)
            mspec.enable    = getattr(mspec, 'enable', self.enable)
            mspec.ignore    = getattr(mspec, 'ignore', self.ignore)
            mspec.module    = getattr(mspec, 'module', self.module)
            mspec.package   = getattr(mspec, 'package', self.package)
            mspec.spec      = getattr(mspec, 'spec', self.testspec)
            mspec.args      = getattr(mspec, 'args', self.args)
            ModuleStore.Add(mspec)
        return

class FeatureObjectHelper(parser.ParserBase):
    def __init__(self):
        self.features = []
        return

    def __is_match(self, feature):
        if GlobalOptions.feature is None:
            if feature.id in BaseTopoExcludeFeatureList:
                return False
            return True
        return feature.id in GlobalOptions.feature

    def __is_subfeature_match(self, feature):
        if GlobalOptions.subfeature is None:
            return True
        return GlobalOptions.subfeature == feature.sub

    def __is_enabled(self, feature):
        if feature.enable is False:
            return False

        if self.__is_subfeature_match(feature) is False:
            logger.info("  - Subfeature Mismatch....Skipping")
            return False

        return self.__is_match(feature)

    def Parse(self):
        ftlist = super().Parse('test/', '*.mlist')
        for fspec in ftlist:
            logger.info("Loading Feature Test Module List: %s" % fspec.feature.id)
            feature = FeatureObject(fspec)
            if self.__is_enabled(feature) is False:
                logger.info("  - Disabled....Skipping")
                continue
            feature.LoadModules()
            self.features.append(feature)
        return

def Init():
    if GlobalOptions.feature is not None:
        GlobalOptions.feature = GlobalOptions.feature.split(',')
    FtlHelper = FeatureObjectHelper()
    FtlHelper.Parse()
    return
