package indexer

import (
	"github.com/pensando/sw/api/generated/search"
	"github.com/pensando/sw/venice/globals"
	elastic "github.com/pensando/sw/venice/utils/elastic"
	mapper "github.com/pensando/sw/venice/utils/elastic/mapper"
	"github.com/pensando/sw/venice/utils/log"
)

/*
	*************************************************************************************
	** Venice object in JSON form and index mapping need to match JSON names precisely **
	** TODO: The json field names in Type and Object meta are still in camelCase and   **
	**       needs to be converted lower case hypenated form as in spec and status     **
	*************************************************************************************

	{

		// Note the following Meta fields use static schema i.e index mapping
		// as defined in configMapping. Since these are commonly used and
		// least prone to changes, having a pre-defined schema helps with
		// optimized searching, sorting and aggregations on these fields.

		"kind": "SecurityGroup",
		"api-version": "v1",
		"meta": {
			"name": "sg03",
			"tenant": "tesla",
			"namespace": "production",
			"resource-version": "13",
			"uuid": "e13cdc39-0d7c-454b-b0b9-18845030f88b",
			"labels": {
				"Application": "SAP-HANA",
				"Location": "us-west-zone2"
			},
			"creation-time": "2018-02-22T02:11:29.059791305Z",
			"mod-time": "2018-02-22T02:11:29.059792348Z",
			"self-link": "/venice/security-groups/security-groups/tesla/sg03"
		},

		// The spec and status fields will in general be schema-less
		// and will become dynamic mappings auto-created by elastic with
		// text as well as keyword mappings to allow text-search and
		// sorting/aggregations on these attributes as well.
		//
		// If there are overlapping names with different data-types
		// we need to seggregate the objects into different doc-type
		// and use copy_to mapping to create unique attribute names
		// prefixed by Kind to disambiguate the overlapping names.

		"spec": {
			"workload-selector": {
				"requirements": [
					{
						"key": "app",
						"operator": "equals",
						"values": [
							"procurement"
						]
					},
					{
						"key": "env",
						"operator": "equals",
						"values": [
							"production"
						]
					}
				]
			}
		},
		"status": {}
	}

The auto-generated elastic config mappings for indexing config/policy
objects would be as follows:

{
    "settings": {
        "number_of_shards": 1,
        "number_of_replicas": 0
    },
    "mappings": {
        "configs": {
            "properties": {
                "api-version": {
                    "type": "text"
                },
                "kind": {
                    "type": "keyword"
                },
                "meta": {
                    "properties": {
                        "creation-time": {
                            "type": "date"
                        },
                        "labels": {
                            "properties": {
                                "key": {
                                    "type": "text"
                                },
                                "value": {
                                    "type": "text"
                                }
                            }
                        },
                        "mod-time": {
                            "type": "date"
                        },
                        "name": {
                            "type": "text"
                        },
                        "namespace": {
                            "type": "text"
                        },
                        "resource-version": {
                            "type": "text"
                        },
                        "self-link": {
                            "type": "text"
                        },
                        "tenant": {
                            "type": "keyword"
                        },
                        "uuid": {
                            "type": "text"
                        }
                    }
                }
            }
        }
    }
}
*/

// Get index Mapping (schema) based on data type
func (idr *Indexer) getIndexMapping(dType globals.DataType) (string, error) {

	switch dType {
	case globals.Configs:

		// get the elastic mapping config
		config, err := mapper.ElasticMapper(search.Entry{},
			elastic.GetDocType(globals.Configs),
			mapper.WithShardCount(3),
			mapper.WithReplicaCount(2))
		if err != nil {
			log.Errorf("Failed to generate elastic mapping for docType: %d, err: %v", dType, err)
			return "", err
		}

		// get the json string for the mapping
		str, err := config.JSONString()
		return str, err

	case globals.Alerts:
		return "", nil
	}

	return "", nil
}
