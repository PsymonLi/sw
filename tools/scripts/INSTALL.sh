#!/bin/sh

# this script is run by customer after extracting the image.
# this is supposed to install all the venice components in the system

# cleanup any installation on this node as if we are gonna install fresh again
function cleanupNode() {
        for srv in "pen-cmd" "pen-etcd" "pen-kube-controller-manager" "pen-kube-scheduler" "pen-kube-apiserver"
        do
            ${SUDO} systemctl stop  ${srv}
            ${SUDO} systemctl disable  ${srv}
        done
        ${SUDO} systemctl stop pen-kubelet
        for i in $(/usr/bin/systemctl list-unit-files --no-legend --no-pager -l | grep --color=never -o kube.*\.slice )
        do
            ${SUDO} systemctl stop $i
        done
        if [ "$(docker ps -qa)" != "" ]
        then
            docker stop -t 2 $(docker ps -qa)
            docker rm $(docker ps -qa)
        fi
        for i in $(cat /proc/mounts | grep kubelet | cut -d " " -f 2)
        do
            ${SUDO} umount $i
        done
        ${SUDO} rm -fr /etc/pensando/* /etc/kubernetes/* /usr/pensando/bin/* /var/lib/pensando/* /var/log/pensando/*  /var/lib/cni/ /var/lib/kubelet/* /etc/cni/
        ${SUDO} ip addr flush label *pens
        if [ "$(docker ps -qa)" != "" ]
        then
            docker stop -t 2 $(docker ps -qa)
            docker rm -f $(docker ps -qa)
        fi
}

function elasticSysctl() {
    TARGET_KEY="vm.max_map_count"
    CONFIG_FILE="/etc/sysctl.conf"
    REPLACEMENT_VALUE=262144

    if ${SUDO} grep -q "^[ ^I]*$TARGET_KEY[ ^I]*=" "$CONFIG_FILE"; then
        ${SUDO} sed -i -e "s^A^\\([ ^I]*$TARGET_KEY[ ^I]*=[ ^I]*\\).*$^A\\1$REPLACEMENT_VALUE^A" "$CONFIG_FILE"
    else
        ${SUDO} echo "$TARGET_KEY = $REPLACEMENT_VALUE" >> "$CONFIG_FILE"
    fi
    ${SUDO} sysctl -p
}

if [ "$(id -u)" != "0" ]
then
    SUDO="sudo"
fi

if [ "$1" == "--clean" ]
then
    cleanupNode
fi

if [ "$1" == "--clean-only" ]
then
    cleanupNode
    exit 0
fi

for i in tars/pen* ; do docker load -i  $i; done
docker run --rm --name pen-install -v /var/log/pensando:/host/var/log/pensando -v /var/lib/pensando:/host/var/lib/pensando -v /usr/pensando/bin:/host/usr/pensando/bin -v /usr/lib/systemd/system:/host/usr/lib/systemd/system -v /etc/pensando:/host/etc/pensando pen-install -c /initscript

elasticSysctl

${SUDO} systemctl stop chronyd || echo
${SUDO} systemctl disable chronyd || echo
${SUDO} systemctl daemon-reload
${SUDO} systemctl enable pensando.target
${SUDO} systemctl start pensando.target
${SUDO} systemctl enable pen-cmd
${SUDO} systemctl start pen-cmd
