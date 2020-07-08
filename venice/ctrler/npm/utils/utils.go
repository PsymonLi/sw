package utils

import (
	"fmt"

	"github.com/pensando/sw/venice/globals"
)

var (
	// NpmNameKey is the key used to label npm created objects
	NpmNameKey = fmt.Sprintf("%s%s", globals.SystemLabelPrefix, "psm-internal")
)

// IsObjInternal check for venice auto created object label
func IsObjInternal(labels map[string]string) bool {
	if labels == nil {
		return false
	}
	if val, ok := labels[NpmNameKey]; ok {
		if val == "Auto" {
			return true
		}
	}
	return false
}
