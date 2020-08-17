package mirrorstats

import (
	"fmt"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/netutils"
	"github.com/pensando/sw/venice/utils/ntranslate"
)

const maxRetry = 5

// Agenturl exports the rest endpoint for netagent
var Agenturl = globals.Localhost + ":" + globals.AgentRESTPort

type mirrorMetricsXlate struct{}

func init() {
	tstr := ntranslate.MustGetTranslator()
	tstr.Register("MirrorMetricsKey", &mirrorMetricsXlate{})
}

// GetMirrorInfo converts mirrorKey to mirror session name and collector ip address by netagent api
func GetMirrorInfo(mirrorKey uint64) (string, string) {
	// ToDo Make this into function call once tmagent integrates with netagent
	var mirrorSession netproto.MirrorSession
	reqURL := fmt.Sprintf("http://%s/api/mapping/mirror-collectors/%v", Agenturl, mirrorKey)
	var err error
	for i := 0; i < maxRetry; i++ {
		err = netutils.HTTPGet(reqURL, &mirrorSession)
		if err != nil {
			log.Errorf("Failed to get and parse mirror session from %v. Err: %v", reqURL, err)
			time.Sleep(time.Millisecond * 100)
			continue
		}

		mirrorSessionName := mirrorSession.Name
		collectorIP, err := getCollectorIP(mirrorKey, mirrorSession)
		if err != nil {
			log.Errorf("Failed to get collector IP. Err: %v", err)
			time.Sleep(time.Millisecond * 100)
			continue
		}

		return mirrorSessionName, collectorIP
	}
	log.Errorf("Failed to get mirror session name and collector ip from %s. Err: %v", reqURL, err)
	return "", ""
}

func (n *mirrorMetricsXlate) KeyToMeta(key interface{}) *api.ObjectMeta {
	if mirrorID, ok := key.(uint64); ok {
		mirrorName, collectorIP := GetMirrorInfo(mirrorID)
		if mirrorName != "" && collectorIP != "" {
			return &api.ObjectMeta{
				Tenant:    "default",
				Namespace: "default",
				Name:      mirrorName,
				Labels: map[string]string{
					"destination": collectorIP,
				},
			}
		}
	}
	return nil
}

// MetaToKey converts meta to network key
func (n *mirrorMetricsXlate) MetaToKey(meta *api.ObjectMeta) interface{} {
	return meta.Name
}

// getCollectorIP get collector ip based on collector id
func getCollectorIP(key uint64, ms netproto.MirrorSession) (string, error) {
	for idx, msID := range ms.Status.MirrorSessionIDs {
		if key == msID {
			if idx < len(ms.Spec.Collectors) {
				return ms.Spec.Collectors[idx].ExportCfg.Destination, nil
			}
			return "", fmt.Errorf("Mirror session ID index out of bound for collectors: %+v", ms)
		}
	}
	return "", fmt.Errorf("Failed to find collector %v inside %+v", key, ms)
}
