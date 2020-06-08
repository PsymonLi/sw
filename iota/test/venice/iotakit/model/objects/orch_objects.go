package objects

import (
	"strings"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/api/generated/orchestration"
	"github.com/pensando/sw/iota/test/venice/iotakit/cfg/objClient"
)

//Orchestrator Return orchestrator
type Orchestrator struct {
	Name     string
	IP       string
	Username string
	Password string
	License  string
	DC       string
	orch     *orchestration.Orchestrator
	client   objClient.ObjClient
}

// NetworkCollection is a list of subnets
type OrchestratorCollection struct {
	CollectionCommon
	err           error
	Orchestrators []*Orchestrator
}

func createOrchestrator(name, ip, port, user, password string) *orchestration.Orchestrator {

	if port != "" {
		ip = ip + ":" + port
	}
	return &orchestration.Orchestrator{
		ObjectMeta: api.ObjectMeta{
			Name: name,
		},
		TypeMeta: api.TypeMeta{
			Kind: "Orchestrator",
		},
		Spec: orchestration.OrchestratorSpec{
			Type: "vcenter",
			URI:  ip,
			Credentials: &monitoring.ExternalCred{
				AuthType:                    "username-password",
				UserName:                    user,
				Password:                    password,
				DisableServerAuthentication: true,
			},
		},
		Status: orchestration.OrchestratorStatus{
			Status: "unknown",
		},
	}
}

//NewOrchestrator create orchestrator.
func NewOrchestrator(client objClient.ObjClient, dcname, name, ip, port, user, password string) *OrchestratorCollection {

	orch := createOrchestrator(name, ip, port, user, password)

	orchObj := &Orchestrator{
		Name:     name,
		IP:       ip,
		Username: user,
		Password: password,
		DC:       dcname,
		orch:     orch,
		client:   client,
	}

	return &OrchestratorCollection{CollectionCommon: CollectionCommon{Client: client}, Orchestrators: []*Orchestrator{orchObj}}

}

//Merge multiple orchestration Collectors
func (orchCol *OrchestratorCollection) Merge(otherOrchCol *OrchestratorCollection) *OrchestratorCollection {

	for _, orch := range otherOrchCol.Orchestrators {
		orchCol.Orchestrators = append(orchCol.Orchestrators, orch)
	}

	return orchCol
}

//Commit  commit multiple orchestration
func (orchCol *OrchestratorCollection) Commit() error {

	for _, orch := range orchCol.Orchestrators {
		if err := orch.Commit(); err != nil {
			return err
		}
	}

	return nil
}

//Delete  commit multiple orchestration
func (orchCol *OrchestratorCollection) Delete() error {

	for _, orch := range orchCol.Orchestrators {
		if err := orch.Delete(); err != nil {
			return err
		}
	}

	return nil
}

//Connected  commit multiple orchestration
func (orchCol *OrchestratorCollection) Connected() (bool, error) {

	for _, orch := range orchCol.Orchestrators {
		if connected, err := orch.Connected(); err != nil || !connected {
			return false, err
		}
	}

	return true, nil
}

//Commit commit the orchestration object
func (orch *Orchestrator) Commit() error {
	//orch.client.

	orch.orch.Spec.ManageNamespaces = strings.Split(orch.DC, ",")
	return orch.client.CreateOrchestration(orch.orch)
}

//Delete deletes orch config
func (orch *Orchestrator) Delete() error {

	return orch.client.DeleteOrchestration(orch.orch)

}

//Connected checks if it is connected
func (orch *Orchestrator) Connected() (bool, error) {

	orchObj, err := orch.client.GetOrchestration(orch.orch)
	if err == nil {
		return orchObj.Status.Status == "success", nil
	}

	return false, err
}
