package equinix_test

import (
	"fmt"
	"time"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)

var _ = Describe("events tests", func() {
	var startTime time.Time
	BeforeEach(func() {
		// verify cluster is in good health
		startTime = time.Now().UTC()
		Eventually(func() error {
			return ts.model.VerifyClusterStatus()
		}).Should(Succeed())
	})
	AfterEach(func() {
		//Expect No Service is stopped
		Expect(ts.model.ServiceStoppedEvents(startTime, ts.model.Naples()).Len(0))
	})

	Context("Basic events tests", func() {
		It("Link flap should trigger an event from hal/linkmgr", func() {
			startTime := time.Now().UTC()
			startTime = startTime.Add(-3 * time.Minute)

			// get all naples and flap the port
			nc := ts.model.Naples()
			Expect(ts.model.PortFlap(nc)).Should(Succeed())

			// ensures the link events are triggered and available in venice
			Eventually(func() error {
				return ts.model.VerifyPortFlapEvents(startTime, nc)
			}, 10*time.Minute, 1*time.Minute).Should(Succeed())

			// verify ping is successful across all workloads after the port flap
			Eventually(func() error {
				return ts.model.PingPairs(ts.model.WorkloadPairs().WithinNetwork())
			}).Should(Succeed())
		})
		It("Events generation on sim", func() {
			// get all naples
			nc := ts.model.Naples()
			nodeCount := len(nc.Names())

			// generate 10 SYSTEM_COLDBOOT events
			// Note: only 1 event with count (10) will be exported to venice
			Expect(ts.model.StartEventsGenOnNaples(nc, "10", "10")).Should(Succeed())

			// verify the events made it to Venice
			Eventually(func() error {
				ec := ts.model.SystemBootEvents(nc)
				if !ec.LenGreaterThanEqualTo(nodeCount) {
					return fmt.Errorf("got less than %v system boot events", nodeCount)
				}
				return nil
			}, 10*time.Minute, 1*time.Minute).Should(Succeed())
		})
	})
})
