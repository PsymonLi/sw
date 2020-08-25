package alertengine

import (
	"encoding/json"
	"io/ioutil"
	"path/filepath"

	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/venice/utils/log"
)

// GetDefaultAlertPolicies reads default alert policies from json files.
//   paths: Absolute paths where the json files are stored
//   skipFiles: Files that should be skipped (for eg., example.json)
func GetDefaultAlertPolicies(paths []string, skipFiles []string) ([]monitoring.AlertPolicy, error) {
	skipFile := func(f string) bool {
		for _, sf := range skipFiles {
			if sf == f {
				return true
			}
		}

		return false
	}

	var pols []monitoring.AlertPolicy
	for _, p := range paths {
		if p == "" {
			continue
		}

		dir, err := filepath.Abs(p)
		if err != nil {
			log.Errorf("Invalid path %v, err: %v", p, err)
			continue
		}

		files, err := ioutil.ReadDir(dir)
		if err != nil {
			log.Errorf("Failed to read default policy dir %v, err: %v", dir, err)
			return nil, err
		}

		for _, f := range files {
			if filepath.Ext(f.Name()) != ".json" {
				continue
			}

			if skipFile(f.Name()) {
				continue
			}

			file, err := ioutil.ReadFile(dir + "/" + f.Name())
			if err != nil {
				log.Errorf("Failed to read file %v, err: %v", f.Name(), err)
				return nil, err
			}

			pol := monitoring.AlertPolicy{}
			pol.Defaults("all")
			err = json.Unmarshal(file, &pol)
			if err != nil {
				log.Errorf("Failed to unmarshal %v, err: %v", f.Name(), err)
				return nil, err
			}

			pols = append(pols, pol)
		}
	}

	return pols, nil
}
