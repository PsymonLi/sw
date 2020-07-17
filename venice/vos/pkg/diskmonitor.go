package vospkg

import (
	"context"
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"time"

	syscall "golang.org/x/sys/unix"

	"github.com/pensando/sw/events/generated/eventattrs"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/runtime"
	"github.com/pensando/sw/venice/vos/protos"
)

const (
	bytesToGBConversionFactor = 1024 * 1024 * 1024

	// Out of the base disk of 250GB, 230GB is given to data partition and 20GB to config partition
	// Other contributors of /data partition are Citadel, Techsupport, Images, Docker, Archieved logs and events etc.
	baseInGBReservedForOtherContributors = 230

	// This threshold is not for entire partition but only for the space that is used between minio and elastic for fwlogs
	// The remaining 10% we are leaving for Elastic, which may be much more then what Elastic needs, but lets not set a very
	// high threshold for objstore, 90% is already very high.
	objstoreThreshold = 90

	diskThresholdCriticialEventMessage = "Flow logs disk usage threshold exceeded on %s, current threshold %e, partition name %s, partition size %d bytes"

	flowlogsDiskThresholdCriticalEventReportingPeriod = 60 * time.Minute

	// Minio (+ other components like Citadel, Elastic etc.) data partition
	minioDataPartition = "/data"
)

// DiskMonitorConfig represents Disk monitor configuration used by Vos for monitoring disk usage
type DiskMonitorConfig struct {
	// If tenant name is empty it will apply to all tenants
	TenantName               string   `json:"tenant"`
	CombinedBuckets          []string `json:"buckets"`
	CombinedThresholdPercent float64  `json:"threshold"`
}

func newDiskMonitorConfig(tenantName string,
	combinedThreshold float64, combinedBuckets ...string) DiskMonitorConfig {
	return DiskMonitorConfig{
		TenantName:               tenantName,
		CombinedThresholdPercent: combinedThreshold,
		CombinedBuckets:          combinedBuckets,
	}
}

func isThresholdReached(basePath string,
	c DiskMonitorConfig, size uint64, thresholdPercent float64, paths []string) (bool, uint64, error) {
	var used uint64
	tenantNames := map[string]struct{}{}
	if c.TenantName != "" {
		tenantNames[c.TenantName] = struct{}{}
	} else {
		// List all the dirs under the basePath and extract tenant names from them.
		dir, err := os.Open(basePath)
		if err != nil {
			return false, used, err
		}
		defer dir.Close()

		files, err := dir.Readdir(-1)
		if err != nil {
			log.Errorf("monitor disk usage path %s, err %+v", basePath, err)
			return false, 0, err
		}

		for _, file := range files {
			if !file.IsDir() {
				continue
			}

			for _, bucket := range c.CombinedBuckets {
				if strings.Contains(file.Name(), bucket) {
					tokens := strings.Split(file.Name(), ".")
					if len(tokens) != 2 {
						continue
					}
					tenantNames[tokens[0]] = struct{}{}
				}
			}
		}
	}

	var helper func(currPath string, c DiskMonitorConfig, size uint64, thresholdPercent float64) (bool, uint64, error)
	helper = func(currPath string, c DiskMonitorConfig, size uint64, thresholdPercent float64) (bool, uint64, error) {
		dir, err := os.Open(currPath)
		if err != nil {
			return false, used, err
		}
		defer dir.Close()

		files, err := dir.Readdir(-1)
		if err != nil {
			log.Errorf("monitor disk usage path %s, err %+v", currPath, err)
			return false, 0, err
		}

		for _, file := range files {
			if file.IsDir() {
				used += uint64(file.Size())
				reached, tmp, err := helper(fmt.Sprintf("%s/%s", currPath, file.Name()), c, size, thresholdPercent)
				if err != nil {
					log.Errorf("error encountered while disk crawling %+v", err)
					continue
				}
				if reached {
					return true, tmp, nil
				}
			} else {
				used += uint64(file.Size())
			}

			// Multiple the used size with total number of diskpaths, since we are going over only 1 disk.
			temp := used * uint64(len(paths))

			p := (float64(temp) / float64(size)) * 100
			if p >= thresholdPercent {
				log.Infof("disk threshold reached for namespace %s, used %+v, th %+v, usedSize %+v", currPath, p, thresholdPercent, temp)
				return true, temp, nil
			}
		}
		return false, used, nil
	}

	for tn := range tenantNames {
		for _, bucket := range c.CombinedBuckets {
			currPath := filepath.Join(basePath, tn+"."+bucket)
			reached, tmp, err := helper(currPath, c, size, thresholdPercent)
			if err != nil {
				log.Errorf("error encountered while disk crawling %+v", err)
				continue
			}
			if reached {
				return reached, tmp, nil
			}
		}
	}

	return false, used, nil
}

// disk usage of path/disk
func diskUsage(tenant string, c DiskMonitorConfig, paths []string) (bool, uint64, uint64, string, float64, error) {
	// No need to go over all the disks as they are replication of each other.
	basePath := paths[0]
	fs := syscall.Statfs_t{}
	err := syscall.Statfs(basePath, &fs)
	if err != nil {
		log.Errorf("monitor disk usage path %s, err %+v", basePath, err)
		return false, 0, 0, basePath, c.CombinedThresholdPercent, fmt.Errorf("monitor disk usage err:  %+v", err)
	}
	all := fs.Blocks * uint64(fs.Bsize)
	allInGB := all / bytesToGBConversionFactor
	th := c.CombinedThresholdPercent
	if th == -1 {
		// calculate threshold percent dynamically based on the current disk size.
		// This calculation assumes that disk is expanded in chunks of 500GB.
		switch {
		case allInGB <= 250:
			// Ideally fwlogs should not run when /data partition size is less then 250GB.
			// But still putting a very low threshold becuase we don't stop user from enabling
			// fwlogs when partition size is less then 250GB.
			// If we set threhold lower then 25% then it may affect systest.
			th = 25
		default:
			left := allInGB - baseInGBReservedForOtherContributors
			leftForObjstore := (left * objstoreThreshold) / 100
			th = (float64(leftForObjstore) / float64(allInGB)) * 100
		}
	}

	reached, used, err := isThresholdReached(basePath, c, all, th, paths)
	return reached, all, used, basePath, th, err
}

func (w *storeWatcher) monitorDisks(ctx context.Context,
	monitorTimeout time.Duration, wg *sync.WaitGroup, monitorConfig *sync.Map, paths []string) {
	defer wg.Done()

	for {
		select {
		case <-ctx.Done():
			return
		case <-time.After(monitorTimeout):
			w.statDisk(monitorConfig, paths)
		}
	}
}

func (w *storeWatcher) statDisk(monitorConfig *sync.Map, paths []string) {
	monitorConfig.Range(func(k interface{}, v interface{}) bool {
		tenant := k.(string)
		t := v.(DiskMonitorConfig)
		reached, all, used, p, th, err := diskUsage(tenant, t, paths)
		if err != nil {
			return false
		}

		// Generate notification only if the disk threshold is reached
		if reached {
			obj := makeEvent(p, all, used, th)
			evType := kvstore.Created
			qs := w.watchPrefixes.Get(diskUpdateWatchPath)
			for j := range qs {
				err := qs[j].Enqueue(evType, obj, nil)
				if err != nil {
					log.Errorf("unable to enqueue the event (%s)", err)
				}
			}

			if w.shouldRaiseEvent(eventattrs.Severity_CRITICAL) {
				descr := fmt.Sprintf(diskThresholdCriticialEventMessage, utils.GetHostname(), th, minioDataPartition, all)
				recorder.Event(eventtypes.FLOWLOGS_DISK_THRESHOLD_EXCEEDED, descr, nil)
			}
		}
		return true
	})
}

func (w *storeWatcher) shouldRaiseEvent(eventSev eventattrs.Severity) bool {
	switch eventSev {
	case eventattrs.Severity_CRITICAL:
		if time.Now().Sub(w.lastFlowlogsCriticalEventRaisedTime).Minutes() >=
			w.flowlogsdiskThresholdCriticialEventDuration.Minutes() {
			w.lastFlowlogsCriticalEventRaisedTime = time.Now()
			return true
		}
	}

	return false
}

func makeEvent(path string, all, used uint64, th float64) runtime.Object {
	log.Infof("sending disk monitor watch event diskUpdateWatchPath %+v, path %+v, size %+v, used %+v, th %+v",
		diskUpdateWatchPath, path, all, used, th)
	obj := &protos.DiskUpdate{}
	obj.Status.Path = path
	obj.Status.Size_ = all
	obj.Status.UsedByNamespace = used
	obj.ResourceVersion = fmt.Sprintf("%d", time.Now().UnixNano())
	return obj
}
