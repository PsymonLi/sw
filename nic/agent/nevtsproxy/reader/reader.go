package reader

import (
	"io/ioutil"
	"os"
	"path/filepath"
	"regexp"
	"sync"
	"time"

	"github.com/fsnotify/fsnotify"

	"github.com/pensando/sw/venice/utils/events"
	"github.com/pensando/sw/venice/utils/log"
)

// Reader watches the directory `shm.GetSharedMemoryDirectory()` and
// spins up events readers (evtReader) on any file (shm) create event.
// And, these events readers in turn starts receiving events from those shared memory files.
// Also, this reader acts as a central entity to manage all the event readers
// that were spun up.

var retryCount = 10

// Reader represents the events shared memory reader which reads events
// dir `shm.GetSharedMemoryDirectory()`
type Reader struct {
	dir            string                // dir to watch, e.g. /dev/shm/pen-events
	pollDelay      time.Duration         // poll interval
	evtReaders     map[string]*EvtReader // list of event readers that were spun up; one reader per shm file
	fileWatcher    *fsnotify.Watcher     // file watcher that watches file create events on `dir`
	evtsDispatcher events.Dispatcher     // events dispatcher to be used by the readers to dispatch events
	stop           sync.Once             // to stop the reader
	logger         log.Logger            // logger
	wg             sync.WaitGroup        // for the file watchers
}

// NewReader creates a new shm events reader
// - it creates a file watcher on the given dir to watch file(shm) create events/
func NewReader(dir string, pollDelay time.Duration, evtsDispatcher events.Dispatcher, logger log.Logger) *Reader {
	rdr := &Reader{
		dir:            dir,
		pollDelay:      pollDelay,
		evtReaders:     map[string]*EvtReader{},
		evtsDispatcher: evtsDispatcher,
		logger:         logger,
	}

	return rdr
}

// Start starts the file watcher to watch shm create events
func (r *Reader) Start() error {
	var err error
	var fileWatcher *fsnotify.Watcher

	for i := 0; i < retryCount; i++ {
		if fileWatcher, err = fsnotify.NewWatcher(); err != nil {
			r.logger.Debugf("failed to create file watcher, err: %v, retrying...", err)
			time.Sleep(1 * time.Second)
			continue
		}

		if err = fileWatcher.Add(r.dir); err != nil {
			r.logger.Debugf("failed to add {%s} to the file watcher, err: %v, retrying...", r.dir, err)
			fileWatcher.Close()
			time.Sleep(1 * time.Second)
			continue
		}

		r.fileWatcher = fileWatcher
	}

	// start reader on the existing "*.events" files
	r.startReaderOnExistingFiles()

	r.wg.Add(1)
	if r.fileWatcher != nil {
		go r.watchFileEvents()
	}

	return err
}

// Stop stops the reader by stopping the file watcher and all the events readers that were spun
func (r *Reader) Stop() {
	r.stop.Do(func() {
		r.fileWatcher.Close() // watchFileEvents will be stopped

		// close all readers
		for _, eRdr := range r.evtReaders {
			eRdr.Stop()
		}

		r.wg.Wait()
	})
}

// GetEventReaders returns the list of readers created so far
func (r *Reader) GetEventReaders() map[string]*EvtReader {
	return r.evtReaders
}

// watchFileEvents helper function to watch file events
func (r *Reader) watchFileEvents() {
	defer r.wg.Done()
	for {
		select {
		case event, ok := <-r.fileWatcher.Events:
			if !ok { // when the watcher is closed (during Stop())
				r.logger.Debug("exiting file watcher...")
				return
			}

			// on file create event, start reader on those file that end with ".events"
			if event.Op == fsnotify.Create {
				fs, err := os.Stat(event.Name)
				if err != nil {
					r.logger.Errorf("failed to get stats for %s, err: %v", event.Name, err)
					continue
				}

				if fs.Mode().IsRegular() && r.isEventsFile(fs.Name()) {
					r.startEvtsReader(event.Name)
				}
			}
		case err, ok := <-r.fileWatcher.Errors:
			if !ok { // when the watcher is closed (during Stop())
				return
			}

			r.logger.Errorf("received error from file watcher, err: %v", err)
		}
	}
}

// startReaderOnExistingFiles helper function to start readers on the existing event files that
// were created before initializing the file watcher.
func (r *Reader) startReaderOnExistingFiles() {
	r.logger.Infof("starting reader on existing event files on dir {%s}", r.dir)

	files, err := ioutil.ReadDir(r.dir)
	if err != nil {
		r.logger.Errorf("failed to read list of files from directory {%s}, err: %v", r.dir, err)
		return
	}

	for _, f := range files {
		if f.Mode().IsRegular() && r.isEventsFile(f.Name()) {
			r.startEvtsReader(filepath.Join(r.dir, f.Name()))
		}
	}
}

// startEvtsReader helper function to start events reader to reader events
// from the given shared memory file (shmPath)
func (r *Reader) startEvtsReader(shmPath string) {
	if existingRdr, ok := r.evtReaders[shmPath]; ok {
		r.logger.Debugf("reader already exists for {%s}; restarting the reader..", shmPath)
		if numPendingEvts := existingRdr.NumPendingEvents(); numPendingEvts != 0 {
			r.logger.Debugf("pending events to be read from the existing reader for {%s}: %v", shmPath, numPendingEvts)
			time.Sleep(2 * time.Second)
		}
		existingRdr.Stop()
	}

	eRdr, err := NewEventReader(shmPath, r.pollDelay, r.logger,
		WithEventsDispatcher(r.evtsDispatcher))
	if err != nil {
		r.logger.Errorf("failed to create reader for shm: %s, err: %v", shmPath, err)
		return
	}

	eRdr.Start()

	r.evtReaders[shmPath] = eRdr
	r.logger.Infof("successfully created reader for %s", shmPath)
}

// return true if the filename ends with ".events". Otherwise, false.
func (r *Reader) isEventsFile(filename string) bool {
	ok, err := regexp.MatchString("^*\\.events$", filename)
	if err != nil {
		r.logger.Errorf("failed to match the filename with pattern {^*\\.events$}, err: %v", err)
		return false
	}

	return ok
}
