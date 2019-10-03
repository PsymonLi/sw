package monitor_test

import (
	"time"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"

	"github.com/pensando/sw/iota/test/venice/iotakit"
)

var _ = Describe("events tests", func() {
	var startTime time.Time
	BeforeEach(func() {
		// verify cluster is in good health
		startTime = time.Now().UTC()
		Eventually(func() error {
			return ts.model.Action().VerifyClusterStatus()
		}).Should(Succeed())
	})
	AfterEach(func() {
		ts.tb.AfterTestCommon()
		//Expect No Service is stopped
		Expect(ts.model.ServiceStoppedEvents(startTime, ts.model.Naples()).Len(0))
	})

	Context("tags:type=basic;datapath=true;duration=short Basic events tests", func() {
		It("tags:sanity=true nevtsproxy should be running all the naples nodes", func() {
			ts.model.ForEachNaples(func(nc *iotakit.NaplesCollection) error {
				out, err := ts.model.Action().RunNaplesCommand(nc, "ps aux | grep [n]evtsproxy")
				Expect(err).ShouldNot(HaveOccurred())
				Expect(len(out) == 1).Should(BeTrue())
				Expect(out[0]).ShouldNot(BeEmpty())
				return nil
			})
		})

		It("tags:sanity=true Link flap should trigger an event from hal/linkmgr", func() {
			Skip("link flap cannot be run on NAPLES sim")
			if ts.tb.HasNaplesSim() {
				Skip("link flap cannot be run on NAPLES sim")
			}
			//Naples time in UTC
			startTime := time.Now().UTC()
			startTime = startTime.Add(-3 * time.Minute) // TODO: remove this; there is ~2 to ~3 minute delay between naples sim and rund VM

			// get a random naples and flap the port
			npc := ts.model.Naples()
			nc := npc.Any(1)
			Expect(nc.Error()).ShouldNot(HaveOccurred())
			Expect(ts.model.Action().PortFlap(nc)).Should(Succeed())
			time.Sleep(60 * time.Second) // wait for the event to reach venice

			// ensures the link events are triggered and available in venice
			ec := ts.model.LinkUpEventsSince(startTime, npc)
			Expect(ec.Error()).ShouldNot(HaveOccurred())
			Expect(ec.LenGreaterThanEqualTo(1)).Should(BeTrue())

			ec = ts.model.LinkDownEventsSince(startTime, npc)
			Expect(ec.Error()).ShouldNot(HaveOccurred())
			Expect(ec.LenGreaterThanEqualTo(1)).Should(BeTrue())

			// verify ping is successful across all workloads after the port flap
			Eventually(func() error {
				return ts.model.Action().PingPairs(ts.model.WorkloadPairs().WithinNetwork())
			}).Should(Succeed())
		})
	})
})
