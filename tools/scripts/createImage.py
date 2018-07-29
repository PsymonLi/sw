#!/usr/bin/python

import os
import datetime
import json
import errno
import subprocess

''' This program creates a tar file of all the necessary containers for running venice.
In addition it also creates a venice.json which has the version info that will be read by
cluster-management-daemon at runtime to determine the list of services that need to be started
in the system '''


# this will be tag for all the dynamic containers generated by this script. Depending on versioning
#  scheme we may take this from some environment variable in the future.
container_tag = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")

def ExecuteCommand(cmd):
    print cmd
    return os.system(cmd)

# Each InstallationItem is looked into during the installation time. Each entry has a name, type
#   (thus implying pre-defined actions to be taken by installer and (optionally) data
# currently supported installType are: container, inline-script
# As requirements evolve, in the future, we might support other installTypes like: rpm, deb, script-file etc
class InstallationItem:
    def __init__(self, data, install_type='container', comment=None):
        self.comment = comment
        self.install_type = install_type
        self.data = data

class MyEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, InstallationItem):
            retval = {
                "installType" : obj.install_type,
                "data" : obj.data
            }
            if obj.comment is not None:
                retval['comment'] = obj.comment
            return retval
        return super(MyEncoder, self).default(obj)


# images which are not compiled every day
static_images = {
    'pen-kube-controller-manager': 'registry.test.pensando.io:5000/google_containers/kube-controller-manager-amd64:v1.7.14',
    'pen-kube-scheduler' : 'registry.test.pensando.io:5000/google_containers/kube-scheduler-amd64:v1.7.14',
    'pen-kube-apiserver' : 'registry.test.pensando.io:5000/google_containers/kube-apiserver-amd64:v1.7.14',
    'pen-etcd' : 'registry.test.pensando.io:5000/coreos/etcd:v3.3.2',
    'pen-filebeat' : 'registry.test.pensando.io:5000/beats/filebeat:6.3.0',
    'pen-ntp' : 'registry.test.pensando.io:5000/pens-ntp:v0.4',
    'pen-influx' : 'registry.test.pensando.io:5000/influxdb:1.4.2',
    'pen-elastic'  : 'registry.test.pensando.io:5000/elasticsearch-cluster:v0.3',
    'pen-pause' : 'gcr.io/google_containers/pause-amd64:3.0',
}

# images which are compiled every time
dynamic_images = [
    "cmd", "apiserver", "apigw", "vchub", "npm", "vcsim", "netagent", "nmd", "collector", "tpm", "spyglass", "evtsmgr", "tsm", "evtsproxy", "aggregator", "vos",
]

# dictionary of module name(specified in venice/globals/modules.go )  to containerImage
imageMap = {}

# dictionary of the names used in systemd service files to the moduleNames
# systemd variables have some idiosyncracies. they should not contain '-'.
systemdNameMap = {
    'PEN_CMD' : 'pen-cmd',
    'PEN_KUBE_SCHEDULER' : 'pen-kube-scheduler',
    'PEN_KUBE_APISERVER' : 'pen-kube-apiserver',
    'PEN_KUBE_CONTROLLERMGR' : 'pen-kube-controller-manager',
    'PEN_ETCD': 'pen-etcd'
}

try:
    os.makedirs("bin/tars")
except EnvironmentError, e:
    if e.errno != errno.EEXIST:
        raise

for k, v in static_images.items():
    if subprocess.check_output(["/bin/sh", "-c", "docker images -q " + v]) == "":
        ExecuteCommand("docker pull " + v)
    ExecuteCommand("docker save -o bin/tars/{}.tar {}".format(k, v))
    imageMap[k] = v


for i in dynamic_images:
    ExecuteCommand("docker tag pen-{}:latest pen-{}:{}".format(i, i, container_tag))
    ExecuteCommand("docker save -o bin/tars/pen-{}.tar pen-{}:{}".format(i, i, container_tag))
    imageMap["""pen-{}""".format(i)] = "pen-{}:{}".format(i, container_tag)
    # remove old images to conserve disk space on dev machines
    old_images = subprocess.check_output(["/bin/sh", "-c", '''docker images pen-{} --filter "before=pen-{}:latest" --format {} | sort | uniq '''.format(i, i, '{{.ID}}')])
    if old_images != "":
        ExecuteCommand('''docker rmi -f ''' + old_images)

# the datastructure that will be written to the file
imageConfig = {}
imageConfig['imageMap'] = imageMap

# the order in which the services get upgraded. For now fill up with some random order.
imageConfig['upgradeOrder'] = ['pen-cmd', 'pen-apiserver', 'pen-apigw', 'pen-vchub', 'pen-npm', 'pen-collector', 'pen-tpm', 'pen-spyglass', 'pen-evtsmgr', 'pen-tsm', 'pen-evtsproxy',
                               'pen-kube-controller-manager', 'pen-kube-scheduler', 'pen-kube-apiserver', 'pen-etcd', 'pen-filebeat', 'pen-ntp', 'pen-influx', 'pen-elastic', 'pen-aggregator', "pen-vos"]

# installInfo is used by the installer during installation of this image.
# This has 2 steps. Preload and LoadAndInstall.
#   Preload is part of Precheck phase and is not supposed to change system state.
#   LoadAndInstall is run after Preload
#   to start with, all this does is do 'docker load -i <x>.tar' for each image
#   This can be used to orchestrate each step of execution, load container images, execute post-installation scripts etc
installInfo = {}
installInfo['Preload'] = [InstallationItem(k + ".tar") for k in imageMap]
installInfo['Preload'].append(InstallationItem("pen-install.tar"))
installInfo['LoadAndInstall'] = [InstallationItem(install_type="inline-script", data="echo starting install", comment="start")]
installInfo['LoadAndInstall'].append(InstallationItem(install_type="inline-script",
    data="docker run --rm --name pen-install -v /usr/pensando/bin:/host/usr/pensando/bin -v /usr/lib/systemd/system:/host/usr/lib/systemd/system -v /etc/pensando:/host/etc/pensando pen-install -c /initscript",
    comment="run initscript"))
installInfo['LoadAndInstall'].append(InstallationItem(install_type="systemctl-daemon-reload", data="", comment="systemctl daemon reload"))
installInfo['LoadAndInstall'].append(InstallationItem(install_type="systemctl-reload-running", data="pen-etcd.service", comment="restart pen-etcd"))
installInfo['LoadAndInstall'].append(InstallationItem(install_type="systemctl-reload-running", data="pen-kubelet.service", comment="restart pen-kubelet if running"))
installInfo['LoadAndInstall'].append(InstallationItem(install_type="systemctl-reload-running", data="pen-kube-controller-manager.service", comment="restart pen-kube-controller-manager if running"))
installInfo['LoadAndInstall'].append(InstallationItem(install_type="systemctl-reload-running", data="pen-kube-apiserver.service", comment="restart pen-kube-apiserver if running"))
installInfo['LoadAndInstall'].append(InstallationItem(install_type="systemctl-reload-running", data="pen-kube-scheduler.service", comment="restart pen-kube-scheduler if running"))
installInfo['LoadAndInstall'].append(InstallationItem(install_type="inline-script", data="echo done", comment="done"))

with open("bin/venice-install.json", 'w') as f:
    json.dump(installInfo, f, indent=True, sort_keys=True, cls=MyEncoder)
with open("tools/docker-files/install/target/etc/pensando/shared/common/venice.json", 'w') as f:
    json.dump(imageConfig, f, indent=True, sort_keys=True)
with open("tools/docker-files/install/target/etc/pensando/shared/common/venice.conf", 'w') as f:
    for  k, v in systemdNameMap.items():
        f.write("{}='{}'\n".format(k, imageMap[v]))
