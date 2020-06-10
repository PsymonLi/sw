package channelqueue

import (
	"context"
	"testing"
	"time"
)

func TestChQueue(t *testing.T) {
	chQ := NewChQueue()
	chQ.Start(context.Background())
	// Writing more events than the channel size shouldn't block
	doneCh := make(chan bool, 6)
	for i := 0; i < 6; i++ {
		go func() {
			for i := 0; i < 10; i++ {
				chQ.Send(Item{})
			}
			doneCh <- true
		}()
	}

	count := 0
	for count < 6 {
		select {
		case <-doneCh:
			count++
		case <-time.After(1 * time.Second):
			t.Fatalf("Failed to write events without blocking")
		}
	}

	// Perform writes and reads at the same time.
	count = 0
	for i := 0; i < 4; i++ {
		go func() {
			for i := 0; i < 10; i++ {
				chQ.Send(Item{})
			}
		}()
	}

	go func() {
		count := 0
		for count < 100 {
			select {
			case <-chQ.ReadCh():
				count++
			}
		}
		doneCh <- true
	}()

	select {
	case <-doneCh:
	case <-time.After(1 * time.Second):
		t.Fatalf("Failed to write events without blocking")
	}

	// Queue should be empty now
	select {
	case <-chQ.ReadCh():
		t.Fatalf("Queue should have been empty")
	default:
	}

	// shutdown queue
	chQ.Stop()

	// Sending items should not block
	doneCh = make(chan bool, 6)
	for i := 0; i < 6; i++ {
		go func() {
			for i := 0; i < 10; i++ {
				chQ.Send(Item{})
			}
			doneCh <- true
		}()
	}

	count = 0
	for count < 6 {
		select {
		case <-doneCh:
			count++
		case <-time.After(1 * time.Second):
			t.Fatalf("Failed to write events without blocking")
		}
	}

	select {
	case <-chQ.ReadCh():
		t.Fatalf("Shouldn't have been able to read value since run thread is dead")
	default:
	}

}
