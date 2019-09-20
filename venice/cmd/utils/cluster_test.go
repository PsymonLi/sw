package utils

import (
	"encoding/json"
	"io/ioutil"
	"os"
	"reflect"
	"testing"

	"github.com/pensando/sw/venice/cmd/env"
	"github.com/pensando/sw/venice/cmd/server/options"
)

func TestClusterConfigFile(t *testing.T) {
	env.Options = options.NewServerRunOptions()
	env.Options.ConfigDir = "/tmp"

	cluster := Cluster{
		Name:      "testCluster",
		UUID:      "AAAA-BBBB-CCCC-DDDD",
		VirtualIP: "192.168.30.10",
	}
	if err := SaveCluster(&cluster); err != nil {
		t.Fatalf("Saving cluster failed with error: %v", err)
	}
	if newCluster, err := GetCluster(); err != nil {
		t.Fatalf("Getting cluster failed with error: %v", err)
	} else if !reflect.DeepEqual(cluster, *newCluster) {
		t.Fatalf("Got %v, expected %v", *newCluster, cluster)
	}
	if err := DeleteCluster(); err != nil {
		t.Fatalf("Deleting cluster failed with error: %v", err)
	}
	if newCluster, err := GetCluster(); err != nil {
		t.Fatalf("Getting cluster failed with error: %v", err)
	} else if newCluster != nil {
		t.Fatalf("Got %v, expected nil", *newCluster)
	}
}
func TestClusterConfigFileErrorCases(t *testing.T) {
	env.Options = options.NewServerRunOptions()

	// invalid file name
	env.Options.ConfigDir = "/"
	env.Options.ClusterConfigFile = "tmp"

	cluster := Cluster{
		Name:      "testCluster",
		UUID:      "AAAA-BBBB-CCCC-DDDD",
		VirtualIP: "192.168.30.10",
	}
	if err := SaveCluster(&cluster); err == nil {
		// cannot write a config to a directory (/tmp)
		t.Fatalf("Saving cluster succeeded writing to invalid config file")
	}
	if _, err := GetCluster(); err == nil {
		t.Fatalf("Getting cluster succeeded with invalid config file")
	}

	// file with invalid contents
	env.Options.ConfigDir = "/dev"
	env.Options.ClusterConfigFile = "null"
	if _, err := GetCluster(); err == nil {
		t.Fatalf("Getting cluster succeeded with invalid config file")
	}
}

func TestContainerConfigFile(t *testing.T) {

	env.Options = options.NewServerRunOptions()
	env.Options.CommonConfigDir = "/"

	tmpfile, err := ioutil.TempFile(os.TempDir(), "cconfig")
	if err != nil {
		t.Fatalf("Error creating temp file %#v", err)
	}
	env.Options.ContainerConfigFile = tmpfile.Name()
	defer os.Remove(tmpfile.Name())

	cInfo := GetContainerInfo()
	if len(cInfo) != 0 {
		t.Errorf("Got non-empty info when config file is not present")
	}

	imageConfig := ImageConfig{
		ImageMap:                map[string]string{"testName": "testImage"},
		UpgradeOrder:            []string{"testName", "testName2"},
		SupportedNaplesVersions: map[string][]string{"naples1": {"ver1", "ver2", "ver3"}},
		GitVersion:              map[string]string{"image_version_2": "cmd_version_3"},
	}
	var content []byte
	if content, err = json.Marshal(imageConfig); err != nil {
		t.Fatalf("unable to json.Marshall %v error: %v", imageConfig, err)
	}

	if _, err := tmpfile.Write(content); err != nil {
		t.Fatal(err)
	}
	if err := tmpfile.Close(); err != nil {
		t.Fatal(err)
	}

	cInfo = GetContainerInfo()
	for k, v := range cInfo {
		if v.ImageName != imageConfig.ImageMap[k] {
			t.Fatalf("got %v instead of %v ", cInfo, imageConfig.ImageMap)
		}
	}

	upgOrder := GetUpgradeOrder()
	if reflect.DeepEqual(upgOrder, imageConfig.UpgradeOrder) != true {
		t.Fatalf("got %v instead of %v ", upgOrder, imageConfig.UpgradeOrder)
	}

	naplesVer := GetSupportedNaplesVersions()
	if naplesVer["naples1"][0] != "ver1" {
		t.Fatalf("got %v instead of ver1 ", naplesVer["naples1"][0])
	}

	cmdVer := GetGitVersion()
	if cmdVer["image_version_2"] != "cmd_version_3" {
		t.Fatalf("got %v instead of cmd_version_3 ", cmdVer["image_version_2"])
	}

}

func TestGenerateNodeQuorumList(t *testing.T) {
	tests := []struct {
		name           string
		envQuorumNodes []string
		nodeID         string
		out            []string
	}{
		{
			name:           "node ID in quorum nodes",
			envQuorumNodes: []string{"1", "2", "3"},
			nodeID:         "2",
			out:            []string{"1", "3"},
		},
		{
			name:           "no node ID in quorum nodes",
			envQuorumNodes: []string{"1", "2", "3"},
			nodeID:         "5",
			out:            []string{"1", "2", "3"},
		},
	}
	for _, test := range tests {
		qnodes := GetOtherQuorumNodes(test.envQuorumNodes, test.nodeID)
		if !reflect.DeepEqual(qnodes, test.out) {
			t.Errorf("%s test failed, expected quorum nodes %v, got %v", test.name, test.out, qnodes)
		}
	}
}
