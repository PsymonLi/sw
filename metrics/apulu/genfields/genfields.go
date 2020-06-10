package genfields

import (
	"io/ioutil"
	"log"
	"path"
	"path/filepath"
	"runtime"

	plugin "github.com/pensando/sw/venice/utils/apigen/plugins/gateway"
)

// mapping of class to list of fields
var kindToFieldNameMap = map[string][]string{}

// GetFieldNamesFromKind returns a list of all fields belonging to the given class
func GetFieldNamesFromKind(k string) []string {
	return kindToFieldNameMap[k]
}

func init() {
	_, currPath, _, ok := runtime.Caller(0)
	if !ok {
		log.Fatalf("failed to get apulu genfields abs path")
	}
	generatedFolderPath := path.Join(filepath.Dir(currPath), "/../generated")
	files, err := ioutil.ReadDir(generatedFolderPath)
	if err != nil {
		log.Fatalf("Failed to read dir %v Err: %v", generatedFolderPath, err)
		return
	}
	for _, f := range files {
		filePath := path.Join(generatedFolderPath, f.Name())
		bytes, err := ioutil.ReadFile(filePath)
		if err != nil {
			log.Fatalf("Failed to read file %v Err: %v", filePath, err)
			return
		}
		fieldMap, err := plugin.GetMetricsFieldMapFromJSON(bytes)
		for metric, fields := range fieldMap {
			kindToFieldNameMap[metric] = fields
		}
	}
}
