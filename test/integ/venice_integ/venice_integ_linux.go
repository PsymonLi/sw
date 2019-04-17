// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

package veniceinteg

import uuid "github.com/satori/go.uuid"

// GetDefaultSuiteConfig returns default test suite config
func GetDefaultSuiteConfig() SuiteConfig {
	return SuiteConfig{
		APIGatewayPort:       "9443",
		APIGatewaySkipAuth:   false,
		StartEventsAndSearch: true,
		ElasticSearchName:    uuid.NewV4().String(),
		NumHosts:             numIntegTestAgents,
		NumWorkloadsPerHost:  10,
		NumPolicies:          3,
		NumRulesPerPolicy:    10,
		NumSecurityGroups:    10,
		DatapathKind:         "mock",
		NetAgentRestPort:     10001,
		NmdRestPort:          12001,
		TmAgentRestPort:      14001,
		TsAgentRestPort:      16001,
	}
}
