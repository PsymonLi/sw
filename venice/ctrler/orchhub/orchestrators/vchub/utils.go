package vchub

import (
	"fmt"
	"strings"

	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/globals"
)

var (
	// VcLabelPrefix is the prefix applied to tags picked up from vcenter
	VcLabelPrefix = fmt.Sprintf("%s%s", globals.SystemLabelPrefix, "vcenter.")
	// NameKey is the display name of the vm in vcenter
	NameKey = createLabelKey("display-name")
)

func createLabelKey(tag string) string {
	return fmt.Sprintf("%s%s", VcLabelPrefix, tag)
}

func generateLabelsFromTags(existingLabels map[string]string, tagMsg defs.TagMsg) map[string]string {
	labels := map[string]string{}
	for _, tagEntry := range tagMsg.Tags {
		key := createLabelKey(tagEntry.Category)
		if len(labels[key]) == 0 {
			labels[key] = tagEntry.Name
		} else {
			labels[key] = fmt.Sprintf("%s:%s", labels[key], tagEntry.Name)
		}
	}
	retainSpecialLabels(existingLabels, labels)
	return labels
}

func retainSpecialLabels(existingLabels, newLabels map[string]string) {
	// Add old values of orch-name, name, name-space etc
	// TODO: vm-name and orch-name could technically conflict
	// with cateory names. Is this ok?
	// In case of conflict, we overwrite with vm/orch name
	if existingLabels == nil {
		existingLabels = map[string]string{}
	}
	if newLabels == nil {
		existingLabels = map[string]string{}
	}
	if v, ok := existingLabels[utils.OrchNameKey]; ok {
		newLabels[utils.OrchNameKey] = v
	}
	if v, ok := existingLabels[NameKey]; ok {
		newLabels[NameKey] = v
	}
	if v, ok := existingLabels[utils.NamespaceKey]; ok {
		newLabels[utils.NamespaceKey] = v
	}
}

func addNameLabel(labels map[string]string, name string) {
	labels[NameKey] = name
}

func getNameLabel(labels map[string]string) (string, bool) {
	name, ok := labels[NameKey]
	return name, ok
}

// CreatePGName creates the PG name
func CreatePGName(networkName string) string {
	return fmt.Sprintf("%s%s", defs.DefaultPGPrefix, networkName)
}

func createNetworkName(name string) string {
	return strings.TrimPrefix(name, defs.DefaultPGPrefix)
}

func isPensandoPG(name string) bool {
	return strings.HasPrefix(name, defs.DefaultPGPrefix)
}

func isPensandoDVS(name, dcName string) bool {
	return CreateDVSName(dcName) == name
}

// CreateDVSName creates the DVS name
func CreateDVSName(dcName string) string {
	return fmt.Sprintf("%s%s", defs.DefaultDVSPrefix, dcName)
}

func createVmkWorkloadNameFromHostName(hostName string) string {
	// uses apiserver host object.Name
	return fmt.Sprintf("%s%s%s", defs.VmkWorkloadPrefix, utils.Delim, hostName)
}
