package cluster

import (
	"context"
	"fmt"

	es "github.com/olivere/elastic"
	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"

	"github.com/pensando/sw/api"
	cmd "github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/elastic"
	"github.com/pensando/sw/venice/utils/log"
)

var _ = Describe("events test", func() {
	var (
		esClient       elastic.ESClient
		from           = int32(0)
		maxResults     = int32(10)
		sortByField    = ""
		sortAsc        = true
		veniceServices = []string{globals.Cmd, globals.APIGw, globals.APIServer, globals.Npm,
			globals.EvtsMgr, globals.Tpm, globals.Tsm}
	)

	BeforeEach(func() {
		esAddr := fmt.Sprintf("%s:%s", ts.tu.FirstVeniceIP, globals.ElasticsearchRESTPort)
		logConfig := log.GetDefaultConfig("e2e_events_test")
		Eventually(func() error {
			var err error
			esClient, err = elastic.NewAuthenticatedClient(esAddr, nil, log.GetNewLogger(logConfig))
			return err
		}, 30, 1).Should(BeNil(), "failed to initialize elastic client")
	})

	It("evtsproxy should be running on all the venice nodes", func() {
		nodesList, err := ts.tu.APIClient.ClusterV1().Node().List(context.Background(), &api.ListWatchOptions{ObjectMeta: api.ObjectMeta{Tenant: globals.DefaultTenant}})
		Expect(err).Should(BeNil())
		for _, node := range nodesList {
			nodeName := node.GetName()
			Eventually(func() string {
				return ts.tu.CommandOutput(ts.tu.NameToIPMap[nodeName], fmt.Sprintf("docker ps -q -f Name=%s", globals.EvtsProxy))
			}, 10, 1).ShouldNot(BeEmpty(), fmt.Sprintf("%s container should be running on %s", globals.EvtsProxy, ts.tu.NameToIPMap[nodeName]))
		}
	})

	It("evtmgr should be running on all the venice nodes", func() {
		nodesList, err := ts.tu.APIClient.ClusterV1().Node().List(context.Background(), &api.ListWatchOptions{ObjectMeta: api.ObjectMeta{Tenant: globals.DefaultTenant}})
		Expect(err).Should(BeNil())
		for _, node := range nodesList {
			nodeName := node.GetName()
			Eventually(func() string {
				return ts.tu.CommandOutput(ts.tu.NameToIPMap[nodeName], fmt.Sprintf("docker ps -q -f Name=%s", globals.EvtsMgr))
			}, 10, 1).ShouldNot(BeEmpty(), fmt.Sprintf("%s container should be running on %s", globals.EvtsMgr, ts.tu.NameToIPMap[nodeName]))
		}
	})

	It("Venice services should start recording events once the cluster is up", func() {
		for _, service := range veniceServices {
			queries := []*es.BoolQuery{}
			if ts.tu.VeniceModules[service].DaemonSet { // make sure the events are recorded from all the nodes (quorum + non-quorum)
				for _, node := range ts.tu.QuorumNodes {
					queries = append(queries, es.NewBoolQuery().Must(
						es.NewTermQuery("source.component.keyword", service),
						es.NewMatchQuery("source.node-name", node)))
				}

				// TODO: e2e tests are not bringing up all the non-quorum nodes. any reason why this behavior?
			} else {
				queries = append(queries, es.NewBoolQuery().Must(
					es.NewTermQuery("source.component.keyword", service)))
			}

			// execute all the queries
			for _, query := range queries {
				Eventually(func() error {
					res, err := esClient.Search(context.Background(),
						elastic.GetIndex(globals.Events, globals.DefaultTenant),
						elastic.GetDocType(globals.Events),
						query, nil, from, maxResults, sortByField, sortAsc)

					if err != nil {
						return err
					}

					if res.TotalHits() == 0 {
						return fmt.Errorf("could not find any %s event", service)
					}
					return nil
				}, 30, 1).Should(BeNil(), "could not find requested event(s) in elasticsearch")
			}
		}

		// check for `LeaderElected` event
		Eventually(func() error {
			query := es.NewBoolQuery().Must(es.NewTermQuery("source.component.keyword", globals.Cmd),
				es.NewTermQuery("type.keyword", cmd.LeaderElected))
			res, err := esClient.Search(context.Background(),
				elastic.GetIndex(globals.Events, globals.DefaultTenant),
				elastic.GetDocType(globals.Events),
				query, nil, from, maxResults, sortByField, sortAsc)

			if err != nil {
				return err
			}

			if res.TotalHits() == 0 {
				return fmt.Errorf("could not find `LeaderElected` event")
			}
			return nil
		}, 30, 1).Should(BeNil(), "could not find `LeaderElected` event in elasticsearch")
	})

	It("NMD should start recording events once NAPLES node is added to the cluster", func() {
		if ts.tu.NumNaplesHosts == 0 {
			Skip("No NAPLES node to be added to the cluster. Skipping NMD events test")
		}

		// check for `NICAdmitted` event
		// NOTE: cluster objects are not allowed to have tenant, so, NICAdmitted event will be in a index (e.g. venice.external.events.2018-09-12)
		// different than other events (e.g.venice.external.<tenant>.events.2018-09-12).
		Eventually(func() error {
			query := es.NewBoolQuery().Must(es.NewTermQuery("source.component.keyword", globals.Nmd),
				es.NewTermQuery("type.keyword", cmd.NICAdmitted))
			res, err := esClient.Search(context.Background(),
				elastic.GetIndex(globals.Events, globals.DefaultTenant), // empty tenant
				elastic.GetDocType(globals.Events),
				query, nil, from, maxResults, sortByField, sortAsc)

			if err != nil {
				return err
			}

			if ts.tu.NumNaplesHosts != int(res.TotalHits()) {
				return fmt.Errorf("could not find `NICAdmitted` events")
			}
			return nil
		}, 100, 1).Should(BeNil(), "could not find `NICAdmitted` events in elasticsearch")
	})

	AfterEach(func() {
		esClient.Close()
	})
})
