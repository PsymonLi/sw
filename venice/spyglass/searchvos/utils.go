package searchvos

import "context"

func isContextDone(ctx context.Context) bool {
	// Return if the query context is cancelled
	select {
	case <-ctx.Done():
		return true
	default:
	}

	return false
}
