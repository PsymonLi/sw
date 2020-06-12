package objects

import (
	evtsapi "github.com/pensando/sw/api/generated/events"
)

// EventsColection is the collection of events
type EventsCollection struct {
	CollectionCommon
	List evtsapi.EventList
	err  error
}

// Count return the number of events in collection
func (ec *EventsCollection) Count() int {
	return len(ec.List.Items)
}

// LenGreaterThanEqualTo return true when the events collection/list matches the expected len
func (ec *EventsCollection) LenGreaterThanEqualTo(expectedLen int) bool {
	return len(ec.List.Items) >= expectedLen
}

// Len returns true when the events collection/list matches the expected len
func (ec *EventsCollection) Len(expectedLen int) bool {
	return len(ec.List.Items) == expectedLen
}

// Error returns the error in collection
func (ec *EventsCollection) Error() error {
	return ec.err
}
