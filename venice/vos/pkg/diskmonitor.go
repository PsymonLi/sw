package vospkg

import (
	"context"
	"fmt"
	"os"
	"sync"
	"time"

	syscall "golang.org/x/sys/unix"

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
)

func isThresholdReached(currPath string, size uint64, thresholdPercent float64) (bool, uint64) {
	var used uint64

	dir, err := os.Open(currPath)
	if err != nil {
		return false, used
	}
	defer dir.Close()

	files, err := dir.Readdir(-1)
	if err != nil {
		log.Errorf("monitor disk usage path %s, err %+v", currPath, err)
		return false, 0
	}

	for _, file := range files {
		if file.IsDir() {
			reached, tmp := isThresholdReached(fmt.Sprintf("%s/%s", currPath, file.Name()), size, thresholdPercent)
			if reached {
				return true, tmp
			}
			used += tmp
		} else {
			used += uint64(file.Size())
		}

		p := (float64(used) / float64(size)) * 100
		if p >= thresholdPercent {
			log.Infof("disk threshold reached for namespace %s", currPath)
			return true, used
		}
	}

	return false, used
}

// disk usage of path/disk
func diskUsage(path string, thresholdPercent float64) (bool, uint64, uint64, error) {
	fs := syscall.Statfs_t{}
	err := syscall.Statfs(path, &fs)
	if err != nil {
		log.Errorf("monitor disk usage path %s, err %+v", path, err)
		return false, 0, 0, fmt.Errorf("monitor disk usage err:  %+v", err)
	}
	all := fs.Blocks * uint64(fs.Bsize)
	allInGB := all / bytesToGBConversionFactor
	th := thresholdPercent
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

	reached, used := isThresholdReached(path, all, th)
	return reached, all, used, nil
}

func (w *storeWatcher) monitorDisks(ctx context.Context,
	monitorTimeout time.Duration, wg *sync.WaitGroup, paths *sync.Map) {
	defer wg.Done()

	for {
		select {
		case <-ctx.Done():
			return
		case <-time.After(monitorTimeout):
			w.statDisk(paths)
		}
	}
}

func (w *storeWatcher) statDisk(paths *sync.Map) {
	paths.Range(func(k interface{}, v interface{}) bool {
		p := k.(string)
		t := v.(float64)
		reached, all, used, err := diskUsage(p, t)
		if err != nil {
			return false
		}

		// Generate notification only if the disk threshold is reached
		if reached {
			obj := makeEvent(p, all, used)
			evType := kvstore.Created
			log.Debugf("sending watch event [%v][%v][%+v]", diskUpdateWatchPath, evType, obj)
			qs := w.watchPrefixes.Get(diskUpdateWatchPath)
			for j := range qs {
				err := qs[j].Enqueue(evType, obj, nil)
				if err != nil {
					log.Errorf("unable to enqueue the event (%s)", err)
				}
			}
		}
		return true
	})
}

func makeEvent(path string, all, used uint64) runtime.Object {
	obj := &protos.DiskUpdate{}
	obj.Status.Path = path
	obj.Status.Size_ = all
	obj.Status.UsedByNamespace = used
	obj.ResourceVersion = fmt.Sprintf("%d", time.Now().UnixNano())
	return obj
}
