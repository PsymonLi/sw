#! /usr/bin/python3
import atexit
import json
import os
import pdb
import re
import time
import traceback

import iota.harness.infra.store as store
import iota.harness.infra.types as types
import iota.harness.infra.utils.parser as parser
import iota.harness.infra.utils.loader as loader
import iota.harness.api as api
import iota.harness.infra.testbundle as testbundle
import iota.harness.infra.testcase as testcase
import iota.harness.infra.topology as topology
import iota.harness.infra.logcollector as logcollector
import iota.harness.infra.core_collector as core_collector
import iota.harness.infra.utils.utils as utils
import iota.harness.infra.utils.timeprofiler as timeprofiler

from iota.harness.infra.utils.logger import Logger as Logger
from iota.harness.infra.glopts import GlobalOptions as GlobalOptions
import iota.test.iris.testcases.server.verify_pci as verify_pci
from iota.harness.infra.exceptions import *
import iota.protos.pygen.topo_svc_pb2 as topo_pb2


class TestSuiteResults(object):
    def __init__(self, testbed=None, repo=None, sha=None, shaTitle=None, targetId=None):
        #these var names map directly to test results viewer schema
        #do not change.
        if testbed:
            self.Testbed = testbed
        else:
            self.Testbed = os.getenv("TESTBED_ID","jobd")
        if repo:
            self.Repository = repo
        else:
            self.Repository = os.getenv("JOB_REPOSITORY")
        if repo:
            self.SHA = sha
        else:
            self.SHA = os.getenv("")
        if shaTitle:
            self.SHATitle = shaTitle
        else:
            self.SHATitle = os.getenv("")
        try: 
          if targetId:
              self.TargetID = int(targetId)
          else:
              self.TargetID = int(os.getenv("TARGET_ID", "0"))
        except ValueError:
            print("targetID is not an integer")
        self.Testcases = []

    def addTestcase(self, tc):
        self.Testcases.append(tc)

    def getTestCaseResults(self):
        return self.Testcases


class TestSuite:
    def __init__(self, spec):
        self.__spec = spec
        self.__testbundles = []
        self.__testbunfiles = []
        self.__verifs = []
        self.__debugs = []
        self.__setups = []
        self.__triggers = []
        self.__teardowns = []
        self.__common_args = parser.Dict2Object({})
        self.__setup_complete = False
        self.__release_versions = {}

        self.__aborted = False
        self.__attrs = {}
        self.__timer = timeprofiler.TimeProfiler()

        self.__stats_pass = 0
        self.__stats_fail = 0
        self.__stats_ignored = 0
        self.__stats_error = 0
        self.__stats_total = 0
        self.__stats_target = 0
        self.result = types.status.FAILURE
        self.__skip = self.__apply_skip_filters()
        self.__ignoreList = getattr(spec.meta, "ignore_list", [])
        self.__images = self.__load_image_manifest()
        self.__process_provision_spec()
        self.__defaultNicMode = getattr(spec.meta, "nicmode", "classic")
        self.__defaultNicPipeline = GlobalOptions.pipeline
        self.__portspeed = self.__getPortSpeedFromTestsuite(spec.meta)
        return

    def Abort(self):
        self.__aborted = True
        self.__curr_tbun.Abort()
        return

    def __getPortSpeedFromTestsuite(self, meta):
        speed = getattr(meta, "portspeed", "auto").lower()
        if speed == "auto":
            return topo_pb2.DataSwitch.Speed_auto
        elif speed == "100g":
            return topo_pb2.DataSwitch.Speed_100G
        elif speed == "50g":
            return topo_pb2.DataSwitch.Speed_50G
        else:
            raise ValueError("speed value must be auto, 100g, or 50g. user entered {0}".format(speed))

    def GetPortSpeed(self, node=None, nic=None):
        if not node:
            return self.__portspeed
        pipeline = self.GetPipelineFromProvisionInfo(node, nic)
        if pipeline == "athena":
            speed = self.__portspeed
        else:
            speed = topo_pb2.DataSwitch.Speed_auto
        api.Logger.debug("{0}:{1} is {2}. using port speed {3}".format(node, nic, pipeline, speed))
        return speed 

    def GetTestbedType(self):
        if self.__spec.meta.mode.lower() == 'hardware':
            return types.tbtype.HARDWARE
        elif self.__spec.meta.mode.lower() == 'simulation':
            return types.tbtype.SIMULATION
        elif self.__spec.meta.mode.lower() == 'hybrid':
            return types.tbtype.HYBRID
        return types.tbtype.ANY

    def GetFirmwareType(self):
        if hasattr(self._TestSuite__spec.meta,"firmware"):
            fmw=self._TestSuite__spec.meta.firmware.lower()
            if fmw == "gold":
                return types.firmware.GOLD
            elif fmw == "main":
                return types.firmware.MAIN
            else:
                raise ValueError("firmware must be gold or main. user specified: {0}".format(fmw))

    def GetDefaultNicMode(self):
        return self.__defaultNicMode

    def GetDefaultNicPipeline(self):
        return self.__defaultNicPipeline

    def GetPackages(self):
        return self.__spec.packages

    def SetupComplete(self):
        return self.__setup_complete

    def GetImages(self):
        return self.__images

    def GetFirmwareVersion(self):
        return self.__release_versions.get('Firmware', 'latest')

    def GetDriverVersion(self):
        return self.__release_versions.get('Driver', 'latest')

    def GetImageManifestFile(self):
        if GlobalOptions.compat_test:
            Logger.debug("Compat-testing for driver: %s and firmware: %s" % (GlobalOptions.driver_version, GlobalOptions.fw_version))
            # assert(GlobalOptions.driver_version != GlobalOptions.fw_version)
            return self.__build_new_image_manifest()
        else:
            Logger.debug("Using testsuite spec for image-manifest")
            if hasattr(self.__spec, 'image_manifest'):
                path = getattr(self.__spec.image_manifest, 'file', 'images/latest.json')
            else:
                path = 'images/latest.json'
            return os.path.join(GlobalOptions.topdir, path)

    def GetTopology(self):
        return self.__topology

    def GetAllowDuplicateBundle(self):
        return getattr(self.__spec.meta, "allowduplicatebundle", "")

    def SetDefaultNicMode(self, mode):
        self.__defaultNicMode = mode

    def GetNaplesPipelines(self):
        return self.__pipelines

    def GetVerifs(self):
        return self.__verifs

    def GetDebugs(self):
        return self.__debugs

    def GetSetups(self):
        return self.__setups

    def GetTriggers(self):
        return self.__triggers

    def GetTeardowns(self):
        return self.__teardowns

    def GetPacakges(self):
        return self.__spec.packages

    def IsConfigOnly(self):
       return getattr(self.__spec.meta, "cfgonly", False)

    def IsBitw(self):
       return getattr(self.__spec.meta, "nicmode", None) == "bitw"

    def GetFwdMode(self):
       return getattr(self.__spec.meta, "fwdmode", "")

    def GetPolicyMode(self):
       return getattr(self.__spec.meta, "policymode", "")

    def DoConfig(self):
        return self.__setup_config()

    def GetCommonArgs(self):
        return self.__common_args

    def __build_new_image_manifest(self):
        # Pick up latest.json (from testsuite spec)
        if hasattr(self.__spec, 'image_manifest'):
            path = getattr(self.__spec.image_manifest, 'file', 'images/latest.json')
        else:
            path = 'images/latest.json'

        with open(os.path.join(GlobalOptions.topdir, path), 'r') as fh:
            new_img_manifest = json.loads(fh.read())

        # pick the non-latest versions
        if GlobalOptions.driver_version != 'latest':
            # driver image to be changed
            self.__release_versions['Driver'] = GlobalOptions.driver_version
            new_img_manifest["Drivers"] = None
            dr_img_manifest_file = os.path.join(GlobalOptions.topdir, 
                                                "images", GlobalOptions.driver_version + ".json")
            with open(dr_img_manifest_file, "r") as fh:
                dr_img_manifest = json.loads(fh.read())
            new_img_manifest["Drivers"] = dr_img_manifest["Drivers"]

        if GlobalOptions.fw_version != 'latest':
            # Firmware image to be changed
            self.__release_versions['Firmware'] = GlobalOptions.fw_version
            new_img_manifest["Firmwares"] = None
            fw_img_manifest_file = os.path.join(GlobalOptions.topdir, 
                                                "images", GlobalOptions.fw_version + ".json")
            with open(fw_img_manifest_file, "r") as fh:
                fw_img_manifest = json.loads(fh.read())
            new_img_manifest["Firmwares"] = fw_img_manifest["Firmwares"]

        new_img_manifest["Version"] = "DriverVersion:{0},FwVersion:{1}".format(GlobalOptions.driver_version, GlobalOptions.fw_version)

        # Create {GlobalOptions.topdir}/images if not exists
        folder = os.path.join(GlobalOptions.topdir, "images")
        if not os.path.isdir(folder):
            os.mkdir(folder)
        manifest_file = os.path.join(folder, self.Name() + ".json")
        with open(manifest_file, "w") as fh:
            fh.write(json.dumps(new_img_manifest, indent=2))
        return manifest_file

    def __process_provision_spec(self):
        if not self.ProvisionInfo():
            return

        prov_spec = parser.YmlParse(self.ProvisionInfo())
        for rel_ver in getattr(prov_spec.spec, 'images', []):
            self.__release_versions['Firmware'] = rel_ver

        return

    def __load_image_manifest(self):
        manifest_file = self.GetImageManifestFile()
        image_info = parser.JsonParse(manifest_file)
        images = parser.Dict2Object({})

        naples_type = 'capri'
        if hasattr(self.__spec, 'image_manifest') and hasattr(self.__spec.image_manifest, 'naples'): 
            naples_type = self.__spec.image_manifest.naples 
        setattr(images, 'naples_type', naples_type)

        image_tag = GlobalOptions.pipeline
        if hasattr(self.__spec, 'image_manifest') and hasattr(self.__spec.image_manifest, 'build'): 
            setattr(images, 'build', self.__spec.image_manifest.build)
            image_tag += "-" + images.build

        all_fw_images = list(filter(lambda x: x.naples_type == naples_type, image_info.Firmwares))
        if all_fw_images and len(all_fw_images) == 1:
            fw_images = list(filter(lambda x: x.tag == image_tag, all_fw_images[0].Images))
            if fw_images and len(fw_images) == 1:
                setattr(images, 'naples', fw_images[0].image)

        # Derive venice-image
        venice = False
        if hasattr(self.__spec, 'image_manifest') and hasattr(self.__spec.image_manifest, 'venice'): 
            venice = self.__spec.image_manifest.venice 
        if venice and image_info.Venice:
            setattr(images, 'venice', image_info.Venice[0].Image)

        # Derive driver-images
        driver = False
        if hasattr(self.__spec, 'image_manifest') and hasattr(self.__spec.image_manifest, 'driver'):
            driver = self.__spec.image_manifest.driver
        if driver:
            setattr(images, 'driver', image_info.Drivers)

        naples_sim = False
        if hasattr(self.__spec, 'image_manifest') and hasattr(self.__spec.image_manifest, 'naples_sim'):
            naples_sim = self.__spec.image_manifest.naples_sim
        if naples_sim:
            setattr(images, 'naples_sim', image_info.Simulation[0].Image)

        setattr(images, 'penctl', image_info.PenCtl[0].Image)
        return images

    def __resolve_calls(self):
        if getattr(self.__spec, 'common', None) and getattr(self.__spec.common, 'verifs', None):
            self.__verifs = self.__spec.common.verifs
        if getattr(self.__spec, 'common', None) and getattr(self.__spec.common, 'debugs', None):
            self.__debugs = self.__spec.common.debugs
        if getattr(self.__spec, 'common', None) and getattr(self.__spec.common, 'setups', None):
            self.__setups = self.__spec.common.setups
        if getattr(self.__spec, 'common', None) and getattr(self.__spec.common, 'triggers', None):
            self.__triggers = self.__spec.common.triggers
        if getattr(self.__spec, 'common', None) and getattr(self.__spec.common, 'teardowns', None):
            self.__teardowns = self.__spec.common.triggers
        if getattr(self.__spec, 'common', None) and getattr(self.__spec.common, 'args', None):
            self.__common_args = self.__spec.common.args

    def __append_testbundle(self, bunfile):
        if self.GetAllowDuplicateBundle() == True or not bunfile in self.__testbunfiles:
            self.__testbunfiles.append(bunfile)

            tbun = testbundle.TestBundle(bunfile, self)
            self.__testbundles.append(tbun)

    def __import_testbundles(self):
        if not GlobalOptions.skip_sanity:
            for bunfile in self.__spec.testbundles.sanity:
                self.__append_testbundle(bunfile)

        if GlobalOptions.extended:
            for bunfile in getattr(self.__spec.testbundles, 'extended', []):
                self.__append_testbundle(bunfile)

        if GlobalOptions.regression:
            for bunfile in getattr(self.__spec.testbundles, 'regression', []):
                self.__append_testbundle(bunfile)

        if GlobalOptions.precheckin:
            for bunfile in getattr(self.__spec.testbundles, 'precheckin', []):
                self.__append_testbundle(bunfile)

        if GlobalOptions.compat_test:
            Logger.debug("For compat-test with fw-version %s and driver-version: %s" % 
                         (self.GetFirmwareVersion(), self.GetDriverVersion()))
            if hasattr(self.__spec.testbundles, 'compat'):
                # first load "common" testbundles for compat-regression
                for bunfile in getattr(self.__spec.testbundles.compat, 'common', []):
                    self.__append_testbundle(bunfile)

                # Append with fw-version specific bundles to be included
                fw_section = 'Fw-' + self.GetFirmwareVersion()
                for bunfile in getattr(self.__spec.testbundles.compat, self.GetFirmwareVersion(), []):
                    self.__append_testbundle(bunfile)

                # Append with driver-version specific bundles to be included
                fw_section = 'Dr-' + self.GetFirmwareVersion()
                for bunfile in getattr(self.__spec.testbundles.compat, self.GetDriverVersion(), []):
                    self.__append_testbundle(bunfile)
            else:
                Logger.error('Missing compat section')

    def __resolve_teardown(self):
        teardown_spec = getattr(self.__spec, 'teardown', [])
        if teardown_spec is None:
            return types.status.SUCCESS
        for s in self.__spec.teardown:
            Logger.debug("Resolving teardown module: %s" % s.step)
            s.step = loader.Import(s.step, self.__spec.packages)
        return types.status.SUCCESS

    def __expand_iterators(self):
        return

    def __parse_setup_topology(self):
        topospec = getattr(self.__spec.setup, 'topology', None)
        if not topospec:
            Logger.error("Error: No topology specified in the testsuite.")
            assert(0)
        self.__topology = topology.Topology(topospec, self.ProvisionInfo())
        store.SetTopology(self.__topology)
        return types.status.SUCCESS

    def __resolve_setup_config(self):
        cfgspec = getattr(self.__spec.setup, 'config', None)
        if not cfgspec:
            return types.status.SUCCESS
        for s in self.__spec.setup.config:
            Logger.debug("Resolving config step: %s" % s.step)
            s.name = s.step
            args = getattr(s, "args", None)
            s.step = loader.Import(s.step, self.__spec.packages)
            s.args = getattr(s, "args", None)
        return types.status.SUCCESS

    def __parse_setup(self):
        ret = self.__parse_setup_topology()
        if ret != types.status.SUCCESS:
            return ret

        ret = self.__resolve_setup_config()
        if ret != types.status.SUCCESS:
            return ret
        return types.status.SUCCESS

    def __setup_config(self):
        for s in self.__spec.setup.config:
            # Reset the running directory before every step
            Logger.info("Starting Config Step: ", s.step)
            api.ChangeDirectory(None)
            status = loader.RunCallback(s.step, 'Main', True, getattr(s, "args", None))
            if status != types.status.SUCCESS:
                Logger.error("ERROR: Failed to run config step", s.step)
                return status
        return types.status.SUCCESS


    def __setup(self):
        ret = None

        if GlobalOptions.skip_setup:
            ret = self.__topology.Build(self)
            if ret != types.status.SUCCESS:
                return ret
        else:
            ret = self.__topology.Setup(self)
            if ret != types.status.SUCCESS:
                return ret
        if not GlobalOptions.skip_switch_init:
            store.GetTestbed().SetupVlanGroups()
        ret = self.__setup_config()
        if ret != types.status.SUCCESS:
            return ret
        self.__setup_complete = True
        return types.status.SUCCESS

    def __invoke_teardown(self):
        teardown_spec = getattr(self.__spec, 'teardown', [])
        if teardown_spec is None:
            return types.status.SUCCESS
        for s in teardown_spec:
            Logger.info("Starting Teardown Step: ", s.step)
            status = loader.RunCallback(s.step, 'Main', True, getattr(s, "args", None))
            if status != types.status.SUCCESS:
                Logger.error("ERROR: Failed to run teardown step", s.step)
                return status
        return types.status.SUCCESS

    def __teardown(self):
        return self.__invoke_teardown()

    def __update_stats(self):
        for tbun in self.__testbundles:
            p,f,i,e,t = tbun.GetStats()
            self.__stats_pass += p
            self.__stats_fail += f
            self.__stats_ignored += i
            self.__stats_error += e
            self.__stats_target += t

        self.__stats_total = (self.__stats_pass + self.__stats_fail +\
                              self.__stats_ignored + self.__stats_error)
        if self.__stats_target != 0:
            self.__stats_target = max(self.__stats_target, self.__stats_total)
        return

    def __execute_testbundles(self):
        result = types.status.SUCCESS
        for tbun in self.__testbundles:
            tbun.setIgnoreList(self.__ignoreList)
            self.__curr_tbun = tbun
            ret = tbun.Main()
            #Don't propogate disabled status up.
            if ret == types.status.DISABLED:
                ret = types.status.SUCCESS
            elif ret != types.status.SUCCESS:
                result = ret
                if result == types.status.CRITICAL and GlobalOptions.stop_on_critical:
                    return ret
                if GlobalOptions.no_keep_going:
                    return ret
            if self.__aborted:
                return types.status.FAILURE
        return result

    def Name(self):
        return self.__spec.meta.name

    def Mode(self):
        return self.__spec.meta.mode

    def ProvisionInfo(self):
        if hasattr(self.__spec.meta, 'provision'):
            return self.__spec.meta.provision
        return None

    def GetPipelineFromProvisionInfo(self, nodeName, nicName):
        try:
            prov_spec = parser.YmlParse(self.ProvisionInfo())
            nodes = getattr(prov_spec, 'nodes', [])
            for node in nodes:
                if node.node.name == nodeName:
                    for nic in node.node.nics:
                        if nic.nic.name == nicName:
                            return nic.nic.pipeline
        except:
            Logger.debug("failed to find pipeline for {0}:{1}".format(nodeName, nicName))

    def LogsDir(self):
        return "%s/iota/logs/%s" % (api.GetTopDir(), self.Name())

    def __get_oss(self):
        return getattr(self.__spec.meta, 'os', ['linux'])

    def __apply_skip_filters(self):
        if GlobalOptions.testsuites and self.Name() not in GlobalOptions.testsuites:
            Logger.debug("Skipping Testsuite: %s because of command-line filters." % self.Name())
            return True

        #if self.GetTestbedType() != types.tbtype.ANY and\
        #   self.GetTestbedType() != store.GetTestbed().GetTestbedType()\
        #   and not GlobalOptions.dryrun:
        #    Logger.debug("Skipping Testsuite: %s due to testbed type mismatch." % self.Name())
        #   return True

        if not store.GetTestbed().IsSimulation() and  not store.GetTestbed().GetOs().intersection(self.__get_oss()) and not GlobalOptions.dryrun:
            Logger.debug("Skipping Testsuite: %s due to OS mismatch." % self.Name())
            return True

        enable = getattr(self.__spec.meta, 'enable', True)
        return not enable

    def IsSkipped(self):
        return self.__skip

    def CollectCores(self):
        try:
            destCoreDir = "corefiles/{0}".format(re.sub('[\W]+','_',self.Name()))
            print("Searching for corefiles.....")
            core_collector.CollectCores(GlobalOptions.testbed_json, destCoreDir, 
                                        store.GetTestbed().GetProvisionUsername(),
                                        store.GetTestbed().GetProvisionPassword(), Logger)
            
        except:
            Logger.debug("failed to collect cores. error was {0}".format(traceback.format_exc()))

    def checkPci(self):
        result = types.status.SUCCESS
        if GlobalOptions.dryrun:
            return result
        for node in self.GetTopology().GetNodes():
            msg = "calling verify_pci.verify_error_lspci() for node {0}".format(node.Name())
            Logger.debug(msg)
            result = verify_pci.verify_errors_lspci(node.Name(), node.GetOs())
            if result != types.status.SUCCESS:
                msg = "PCIe Failure detected on node {0} with OS {1}".format(node.Name(), node.GetOs())
                print(msg)
                Logger.error(msg)
                return result
                # raise OfflineTestbedException
        return result

    def __getNodeByName(self, nodeName):
        node = [node for node in store.GetTopology().GetNodes() if node._Node__name == nodeName]
        if node == []:
            return None
        return node[0]

    def UpdateNaplesPipelines(self, pipelines=[]):
        if GlobalOptions.skip_firmware_upgrade:
            Logger.debug("user requested to skip firmware upgrade so skipping naples pipeline install")
            return
        if not pipelines:
            pipelines = self.GetNaplesPipelines()
            if not pipelines:
                Logger.debug("no pipelines found")
                return
        nwarmd = "{0}/iota/warmd.json".format(api.GetTopDir())
        with open(GlobalOptions.testbed_json, "r") as warmdFile:
            updated = False
            alreadyDownloaded = []
            warmd = json.load(warmdFile)
            for pl in pipelines:
                if not types.nicModes.valid(pl.mode.upper()):
                    raise ValueError("nic mode {0} is not valid. must be one of: {1}".format(pl.mode, types.nicModes.str_enums.values()))
                if not types.pipelines.valid(pl.pipeline.upper()):
                    raise ValueError("nic pipeline {0} is not valid. must be one of: {1}".format(pl.pipeline, types.pipelines.str_enums.values()))
                if pl.nicNumber < 1:
                    raise ValueError("nic number must be >= 1. value from testsuite files was {0}".format(pl.nicNumber))
                Logger.debug("checking pipeline info for {0}".format(pl))
                topoNode = self.__getNodeByName(pl.node)
                if not topoNode:
                    Logger.warn("failed to find node {0} in topology node list".format(node.name))
                    continue
                instId = topoNode.GetNodeInfo()["InstanceID"]
                for node in warmd['Instances']:
                    if instId == node.get('ID',None):
                        device = topoNode.GetDeviceByNicNumber(pl.nicNumber)
                        device.SetMode(pl.mode)
                        device.SetNaplesPipeline(pl.pipeline)
                        nic = node['Nics'][pl.nicNumber-1]
                        nic['version'] = pl.version
                        nic['pipeline'] = pl.pipeline
                        nic['mode'] = pl.mode
                        updated = True
                        if pl.version not in alreadyDownloaded:
                            api.DownloadAssets(pl.version)
                            alreadyDownloaded.append(pl.version)
                        Logger.info("upgrading node:nic {0}:{1}".format(topoNode.MgmtIpAddress(),pl.nicNumber))
                        devices = {instId : { "nics":[pl.nicNumber], "nodeName":pl.node, "pipeline":pl.pipeline} }
                        Logger.debug("writing updated warmd.json to {0}".format(nwarmd))
                        with open(nwarmd,'w') as outfile:
                            json.dump(warmd,outfile,indent=4)
                        resp = api.ReInstallImage(fw_version=pl.version, dr_version=pl.version, devices=devices)
                        if resp != api.types.status.SUCCESS:
                            Logger.error(f"Failed to install images on the node:nic {0}:{1}".format(topoNode.MgmtIpAddress(),pl.nicNumber))
                        break
                else:
                    Logger.warn("failed to find node {0} / id {1} in warmd".format(topoNode.MgmtIpAddress(),instId))


    def writeTestResults(self):
        filename = "testsuite_{0}_results.json".format(self.Name())
        try:
            tsr = TestSuiteResults()
            for tbun in self.__testbundles:
                if tbun.getTestBundleResults():
                    tsr.Testcases.extend(tbun.getTestBundleResults())
            with open(filename,'w') as outfile:
                json.dump(tsr, outfile, default=lambda x: x.__dict__, sort_keys=True)
        except:
            Logger.debug("failed to save test results to file {0}. error was: {1}".format(filename,traceback.format_exc()))

    def ExitHandler(self):
        if GlobalOptions.dryrun:
            return
        status = self.__teardown()
        Logger.info(f"Testsuite {self.Name()} teardown STATUS = {status}")
        api.UnsetBreakoutInterfaces()
        logcollector.CollectLogs()
        logcollector.CollectTechSupport(self.Name())
        self.CollectCores()
        self.writeTestResults()
        if store.GetTestbed().IsUsingProdVCenter():
            store.GetTestbed().CleanupTestbed()
        store.GetTopology().CleanupTopology()
        return

    def Main(self):
        if self.__skip:
            Logger.debug("Skipping Testsuite: %s due to filters." % self.Name())
            return types.status.SUCCESS

        atexit.register(self.ExitHandler)

        # Start the testsuite timer
        self.__timer.Start()

        # Update logger
        Logger.SetTestsuite(self.Name())
        Logger.info("Starting Testsuite: %s" % self.Name())
        Logger.info("Testsuite {0} timestamp: {1}".format(self.Name(), time.asctime()))

        if self.GetFirmwareType() == types.firmware.GOLD:
            Logger.debug("setting global firmware type to gold")
            GlobalOptions.use_gold_firmware = True

        if self.__release_versions:
            # Download Assets
            for _, rel in self.__release_versions.items():
                if rel != 'latest':
                    api.DownloadAssets(rel)

        # Initialize Testbed for this testsuite
        status = store.GetTestbed().InitForTestsuite(self)
        if status != types.status.SUCCESS:
            self.__timer.Stop()
            return status

        status = self.__parse_setup()
        if status != types.status.SUCCESS:
            self.__timer.Stop()
            return status

        self.__import_testbundles()
        self.__resolve_teardown()
        self.__expand_iterators()
        self.__resolve_calls()

        status = self.__setup()
        if status != types.status.SUCCESS:
            self.__timer.Stop()
            return status

        status = self.checkPci()
        if status != types.status.SUCCESS:
            self.__timer.Stop()
            return status

        self.result = self.__execute_testbundles()
        self.__update_stats()
        Logger.info("Testsuite %s FINAL STATUS = %d" % (self.Name(), self.result))

        self.__timer.Stop()
        return self.result

    def PrintReport(self):
        if self.__skip:
           return types.status.SUCCESS
        print("\nTestSuite: %s" % self.__spec.meta.name)
        print(types.HEADER_SUMMARY)
        print(types.FORMAT_TESTCASE_SUMMARY %\
              ("Testbundle", "Testcase", "Owner", "Result", "Duration"))
        print(types.HEADER_SUMMARY)
        for tbun in self.__testbundles:
            tbun.PrintReport()
        print(types.HEADER_SUMMARY)
        return

    def PrintBundleSummary(self):
        if self.__skip:
           return types.status.SUCCESS
        print("\nTestBundle Summary for TestSuite: %s" % self.__spec.meta.name)
        print(types.HEADER_SHORT_SUMMARY)
        print(types.FORMAT_ALL_TESTSUITE_SUMMARY %\
              ("Testbundle", "Pass", "Fail", "Ignore", "Dis/Err", "Total", "Target", "%Done", "Result", "Duration"))
        print(types.HEADER_SHORT_SUMMARY)
        for tbun in self.__testbundles:
            tbun.PrintSummary()
        print(types.HEADER_SHORT_SUMMARY)
        return

    def PrintSummary(self):
        if self.__skip:
           return types.status.SUCCESS
        if self.__stats_target:
            pc_complete = "%02.02f" % (float(self.__stats_total)/float(self.__stats_target)*100)
        else:
            pc_complete = 'NA'
        print(types.FORMAT_ALL_TESTSUITE_SUMMARY %\
              (self.__spec.meta.name, self.__stats_pass, self.__stats_fail, self.__stats_ignored,
               self.__stats_error, self.__stats_total, self.__stats_target, pc_complete,
               types.status.str(self.result).title(), self.__timer.TotalTime()))
        return types.status.SUCCESS

    def SetAttr(self, attr, value):
        Logger.info("Adding new Testsuite attribute: %s = " % attr, value)
        self.__attrs[attr] = value
        return

    def GetAttr(self, attr):
        if attr in self.__attrs:
            return self.__attrs[attr]
        return None
