package agent

import (
	"fmt"
	"os"
	"strings"
	"time"

	"github.com/pkg/errors"

	iota "github.com/pensando/sw/iota/protos/gogen"
	utils "github.com/pensando/sw/iota/svcs/agent/utils"
	Common "github.com/pensando/sw/iota/svcs/common"
)

type kubeMasterNode struct {
	commandNode
}

func (kube *kubeMasterNode) bringUpKube(ip string, hostname string, reload bool, token string) error {

	// if this is a reload, we are done
	if reload {
		return nil
	}

	curDir, _ := os.Getwd()
	defer os.Chdir(curDir)

	for i := 0; true; i++ {
		setHostname := []string{"hostnamectl", "set-hostname", "--static", hostname}
		if stderr, stdout, err := utils.Run(setHostname, 0, false, false, nil); err != nil {
			kube.logger.Printf("Setting hostname failed %v %v %v", stderr, stdout, err.Error())
			if i == 5 {
				return errors.Wrap(err, stdout)
			}
			time.Sleep(1 * time.Second)
			continue
		}
		break
	}

	// check etc/hosts exists, centos doesn't have it but k8s needs it
	if _, err := os.Stat("/etc/hosts"); err != nil {
		// create this file
		f, err2 := os.Create("/etc/hosts")
		if err2 != nil {
			kube.logger.Printf("Failed to create /etc/hosts: %v", err2.Error())
			return err2
		}
		output := fmt.Sprintf("%s %s\n", ip, hostname)
		_, err2 = f.WriteString(output)
		if err2 != nil {
			kube.logger.Printf("Failed to write to /etc/hosts: %v", err2.Error())
			return err2
		}
		f.Sync()
		f.Close()
	}
/*
	copy := []string{"cp", "/etc/kubernetes/ca.*", "/etc/kubernetes/pki/"}
        if stderr, stdout, err := utils.Run(copy, 0, false, false, nil); err != nil {
                kube.logger.Printf("Running kube ca key copy failed: %v %v %v", stderr, stdout, err.Error())
                return errors.Wrap(err, stdout)
        }
*/
	kube.logger.Println("initializing k8s master...")
	install := []string{"kubeadm", "init", "--token", token}
	if stderr, stdout, err := utils.Run(install, 0, false, false, nil); err != nil {
		kube.logger.Printf("Running kubeadm init failed: %v %v %v", stderr, stdout, err.Error())
		return errors.Wrap(err, stdout)
	}

	kube.logger.Println("adding k8s network component...")
	cni := []string{"kubectl", "--kubeconfig", "/etc/kubernetes/admin.conf", "apply", "-f", "/etc/kubernetes/calico.yaml"}
	if stderr, stdout, err := utils.Run(cni, 0, false, false, nil); err != nil {
		kube.logger.Printf("Running k8s adding cni failed: %v %v %v", stderr, stdout, err.Error())
		return errors.Wrap(err, stdout)
	}

	kube.logger.Println("waiting for service up...")
	status := []string{"kubectl", "--kubeconfig", "/etc/kubernetes/admin.conf", "get", "node", hostname}
	for i := 0; true; i++ {
		if i == 60 {
			return errors.New("k8s timeout waiting for master ready")
		}
		stderr, stdout, err := utils.Run(status, 0, false, false, nil)
		if err != nil {
			kube.logger.Printf("Running k8s status failed: %v %v %v", stderr, stdout, err.Error())
			time.Sleep(1 * time.Second)
			continue
		}
		if !strings.Contains(stdout, "Ready") {
			kube.logger.Println("k8s master node not up")
			time.Sleep(1 * time.Second)
			continue
		}
		break
	}

	return nil
}

//Init initialize node type
func (kube *kubeMasterNode) Init(in *iota.Node) (*iota.Node, error) {
	kube.commandNode.Init(in)
	kube.iotaNode.name = in.GetName()
	kube.iotaNode.nodeMsg = in
	kube.logger.Printf("Bring up request received for : %v. Req: %+v", in.GetName(), in)

	if err := kube.bringUpKube(in.IpAddress, in.GetName(), in.GetReload(), "f0c861.753c505740ecde4c"); err != nil {
		kube.logger.Println("K8s master node bring up failed.")
		msg := fmt.Sprintf("k8s master node bring up failed: %v", err.Error())
		kube.logger.Println(msg)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR,
			ErrorMsg: msg,
		}}, err
	}

	kube.logger.Println("K8s node bring script up successfully.")

	dir := Common.DstIotaEntitiesDir + "/" + in.GetName()
	os.Mkdir(dir, 0765)
	os.Chmod(dir, 0777)

	return &iota.Node{Name: in.Name, IpAddress: in.IpAddress, Type: in.GetType(),
		NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_STATUS_OK}}, nil
}

// AddWorkload brings up a workload type on a given node
func (kube *kubeMasterNode) AddWorkloads(*iota.WorkloadMsg) (*iota.WorkloadMsg, error) {
	kube.logger.Println("Add workload on venice not supported.")
	return &iota.WorkloadMsg{ApiResponse: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_BAD_REQUEST}}, nil
}

// DeleteWorkloads deletes a given workloads
func (kube *kubeMasterNode) DeleteWorkloads(*iota.WorkloadMsg) (*iota.WorkloadMsg, error) {
	kube.logger.Println("Delete workload on venice not supported.")
	return &iota.WorkloadMsg{ApiResponse: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_BAD_REQUEST}}, nil
}
