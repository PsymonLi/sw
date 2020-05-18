package kvstore

import (
	"github.com/pensando/sw/venice/utils/runtime"
)

// WatchEventType defines possible types of events for a watch.
type WatchEventType string

const (
	// Created is an event to indicate an object is created
	Created WatchEventType = "Created"
	// Updated is an event to indicate an object is updated.
	Updated WatchEventType = "Updated"
	// Deleted is an event to indicate an object is deleted.
	Deleted WatchEventType = "Deleted"
	// WatcherError is an event to indicate an error with watch. Watch must
	// be re-established when this happens.
	WatcherError WatchEventType = "Error"
	// WatcherControl is an event for control messages in the watch channel. Handling is optional.
	WatcherControl WatchEventType = "Control"
)

func (w WatchEventType) String() string {
	return string(w)
}

// WatchEvent contains information about a single event on watched object(s)
type WatchEvent struct {
	// Type of the watch event.
	Type WatchEventType

	// Key is the Kvstore key for the object.
	Key string

	// For a Deleted event, this is previous version of the object. For an
	// Error event, it is undefined. For all other events, it is the latest
	// version of the object.
	Object runtime.Object

	// Control is only set when Type is WatcherControl
	Control *WatchControl
}

// WatchControl holds control messages on the watch channel
type WatchControl struct {
	Code    uint32
	Message string
}

// Watcher is an interface that can be implemented to keep track of changes to
// objects in a key value store. Any watcher that implements this interface must
// start the watch when the watcher is created.
type Watcher interface {
	// EventChan returns the channel to receive events on. If there is an
	// error with the watch or when Stop is called, this channel will be
	// closed.
	EventChan() <-chan *WatchEvent

	// Stop stops the watch and closes the channel returned by EventChan().
	Stop()
}
