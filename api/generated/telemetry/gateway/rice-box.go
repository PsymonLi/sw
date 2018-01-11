package telemetryGwService

import (
	"github.com/GeertJohan/go.rice/embedded"
	"time"
)

func init() {

	// define files
	file2 := &embedded.EmbeddedFile{
		Filename:    "telemetry.swagger.json",
		FileModTime: time.Unix(1515703237, 0),
		Content:     string("{\n  \"swagger\": \"2.0\",\n  \"info\": {\n    \"title\": \"Service name\",\n    \"version\": \"version not set\"\n  },\n  \"schemes\": [\n    \"http\",\n    \"https\"\n  ],\n  \"consumes\": [\n    \"application/json\"\n  ],\n  \"produces\": [\n    \"application/json\"\n  ],\n  \"paths\": {\n    \"/{O.Tenant}/monitoringPolicy\": {\n      \"post\": {\n        \"operationId\": \"AutoAddMonitoringPolicy\",\n        \"responses\": {\n          \"200\": {\n            \"description\": \"\",\n            \"schema\": {\n              \"$ref\": \"#/definitions/telemetryMonitoringPolicy\"\n            }\n          }\n        },\n        \"parameters\": [\n          {\n            \"name\": \"O.Tenant\",\n            \"in\": \"path\",\n            \"required\": true,\n            \"type\": \"string\"\n          },\n          {\n            \"name\": \"body\",\n            \"in\": \"body\",\n            \"required\": true,\n            \"schema\": {\n              \"$ref\": \"#/definitions/telemetryMonitoringPolicy\"\n            }\n          }\n        ],\n        \"tags\": [\n          \"MonitoringPolicyV1\"\n        ]\n      }\n    },\n    \"/{O.Tenant}/monitoringPolicy/{O.Name}\": {\n      \"get\": {\n        \"operationId\": \"AutoGetMonitoringPolicy\",\n        \"responses\": {\n          \"200\": {\n            \"description\": \"\",\n            \"schema\": {\n              \"$ref\": \"#/definitions/telemetryMonitoringPolicy\"\n            }\n          }\n        },\n        \"parameters\": [\n          {\n            \"name\": \"O.Tenant\",\n            \"in\": \"path\",\n            \"required\": true,\n            \"type\": \"string\"\n          },\n          {\n            \"name\": \"O.Name\",\n            \"in\": \"path\",\n            \"required\": true,\n            \"type\": \"string\"\n          },\n          {\n            \"name\": \"T.Kind\",\n            \"description\": \"Kind represents the type of the API object.\",\n            \"in\": \"query\",\n            \"required\": false,\n            \"type\": \"string\"\n          },\n          {\n            \"name\": \"T.APIVersion\",\n            \"description\": \"APIVersion defines the version of the API object.\",\n            \"in\": \"query\",\n            \"required\": false,\n            \"type\": \"string\"\n          },\n          {\n            \"name\": \"O.Namespace\",\n            \"description\": \"Namespace of the object, for scoped objects.\",\n            \"in\": \"query\",\n            \"required\": false,\n            \"type\": \"string\"\n          },\n          {\n            \"name\": \"O.ResourceVersion\",\n            \"description\": \"Resource version in the object store. This can only be set by the server.\",\n            \"in\": \"query\",\n            \"required\": false,\n            \"type\": \"string\"\n          },\n          {\n            \"name\": \"O.UUID\",\n            \"description\": \"UUID is the unique identifier for the object. This can only be set by the server.\",\n            \"in\": \"query\",\n            \"required\": false,\n            \"type\": \"string\"\n          },\n          {\n            \"name\": \"O.CreationTime.time\",\n            \"in\": \"query\",\n            \"required\": false,\n            \"type\": \"string\",\n            \"format\": \"date-time\"\n          },\n          {\n            \"name\": \"O.ModTime.time\",\n            \"in\": \"query\",\n            \"required\": false,\n            \"type\": \"string\",\n            \"format\": \"date-time\"\n          },\n          {\n            \"name\": \"Spec.Area\",\n            \"in\": \"query\",\n            \"required\": false,\n            \"type\": \"array\",\n            \"items\": {\n              \"type\": \"string\"\n            }\n          },\n          {\n            \"name\": \"Spec.CollectionPolicy\",\n            \"in\": \"query\",\n            \"required\": false,\n            \"type\": \"string\"\n          },\n          {\n            \"name\": \"Spec.RetentionPolicy\",\n            \"in\": \"query\",\n            \"required\": false,\n            \"type\": \"string\"\n          },\n          {\n            \"name\": \"Spec.ExportPolicies\",\n            \"in\": \"query\",\n            \"required\": false,\n            \"type\": \"array\",\n            \"items\": {\n              \"type\": \"string\"\n            }\n          },\n          {\n            \"name\": \"Status.Workloads\",\n            \"in\": \"query\",\n            \"required\": false,\n            \"type\": \"array\",\n            \"items\": {\n              \"type\": \"string\"\n            }\n          }\n        ],\n        \"tags\": [\n          \"MonitoringPolicyV1\"\n        ]\n      },\n      \"delete\": {\n        \"operationId\": \"AutoDeleteMonitoringPolicy\",\n        \"responses\": {\n          \"200\": {\n            \"description\": \"\",\n            \"schema\": {\n              \"$ref\": \"#/definitions/telemetryMonitoringPolicy\"\n            }\n          }\n        },\n        \"parameters\": [\n          {\n            \"name\": \"O.Tenant\",\n            \"in\": \"path\",\n            \"required\": true,\n            \"type\": \"string\"\n          },\n          {\n            \"name\": \"O.Name\",\n            \"in\": \"path\",\n            \"required\": true,\n            \"type\": \"string\"\n          }\n        ],\n        \"tags\": [\n          \"MonitoringPolicyV1\"\n        ]\n      },\n      \"put\": {\n        \"operationId\": \"AutoUpdateMonitoringPolicy\",\n        \"responses\": {\n          \"200\": {\n            \"description\": \"\",\n            \"schema\": {\n              \"$ref\": \"#/definitions/telemetryMonitoringPolicy\"\n            }\n          }\n        },\n        \"parameters\": [\n          {\n            \"name\": \"O.Tenant\",\n            \"in\": \"path\",\n            \"required\": true,\n            \"type\": \"string\"\n          },\n          {\n            \"name\": \"O.Name\",\n            \"in\": \"path\",\n            \"required\": true,\n            \"type\": \"string\"\n          },\n          {\n            \"name\": \"body\",\n            \"in\": \"body\",\n            \"required\": true,\n            \"schema\": {\n              \"$ref\": \"#/definitions/telemetryMonitoringPolicy\"\n            }\n          }\n        ],\n        \"tags\": [\n          \"MonitoringPolicyV1\"\n        ]\n      }\n    }\n  },\n  \"definitions\": {\n    \"apiListMeta\": {\n      \"type\": \"object\",\n      \"properties\": {\n        \"ResourceVersion\": {\n          \"type\": \"string\",\n          \"description\": \"Resource version of object store at the time of list generation.\"\n        }\n      },\n      \"description\": \"ListMeta contains the metadata for list of objects.\"\n    },\n    \"apiListWatchOptions\": {\n      \"type\": \"object\",\n      \"properties\": {\n        \"O\": {\n          \"$ref\": \"#/definitions/apiObjectMeta\"\n        },\n        \"LabelSelector\": {\n          \"type\": \"string\"\n        },\n        \"FieldSelector\": {\n          \"type\": \"string\"\n        },\n        \"PrefixWatch\": {\n          \"type\": \"boolean\",\n          \"format\": \"boolean\"\n        }\n      }\n    },\n    \"apiObjectMeta\": {\n      \"type\": \"object\",\n      \"properties\": {\n        \"Name\": {\n          \"type\": \"string\",\n          \"description\": \"Name of the object, unique within a Namespace for scoped objects.\"\n        },\n        \"Tenant\": {\n          \"type\": \"string\",\n          \"description\": \"Tenant is global namespace isolation for various objects. This can be automatically\\nfilled in many cases based on the tenant a user, who created the object, belongs go.\"\n        },\n        \"Namespace\": {\n          \"type\": \"string\",\n          \"description\": \"Namespace of the object, for scoped objects.\"\n        },\n        \"ResourceVersion\": {\n          \"type\": \"string\",\n          \"description\": \"Resource version in the object store. This can only be set by the server.\"\n        },\n        \"UUID\": {\n          \"type\": \"string\",\n          \"description\": \"UUID is the unique identifier for the object. This can only be set by the server.\"\n        },\n        \"Labels\": {\n          \"type\": \"object\",\n          \"additionalProperties\": {\n            \"type\": \"string\"\n          },\n          \"description\": \"Labels are arbitrary (key,value) pairs associated with any object.\"\n        },\n        \"CreationTime\": {\n          \"$ref\": \"#/definitions/apiTimestamp\",\n          \"title\": \"CreationTime is the creation time of Object\"\n        },\n        \"ModTime\": {\n          \"$ref\": \"#/definitions/apiTimestamp\",\n          \"title\": \"ModTime is the Last Modification time of Object\"\n        }\n      },\n      \"description\": \"ObjectMeta contains metadata that all objects stored in kvstore must have.\"\n    },\n    \"apiTimestamp\": {\n      \"type\": \"object\",\n      \"properties\": {\n        \"time\": {\n          \"type\": \"string\",\n          \"format\": \"date-time\"\n        }\n      }\n    },\n    \"apiTypeMeta\": {\n      \"type\": \"object\",\n      \"properties\": {\n        \"Kind\": {\n          \"type\": \"string\",\n          \"description\": \"Kind represents the type of the API object.\"\n        },\n        \"APIVersion\": {\n          \"type\": \"string\",\n          \"description\": \"APIVersion defines the version of the API object.\"\n        }\n      },\n      \"description\": \"TypeMeta contains the metadata about kind and version for all API objects.\"\n    },\n    \"telemetryAutoMsgMonitoringPolicyWatchHelper\": {\n      \"type\": \"object\",\n      \"properties\": {\n        \"Type\": {\n          \"type\": \"string\",\n          \"title\": \"Area describes an area for which the monitoring policy is specified\"\n        },\n        \"Object\": {\n          \"$ref\": \"#/definitions/telemetryMonitoringPolicy\",\n          \"title\": \"object selector for the service (list of object kind/namespace/instance to match)\\nTBD: this would need to be replaced by a generic definition of an object selector\"\n        }\n      }\n    },\n    \"telemetryMonitoringPolicy\": {\n      \"type\": \"object\",\n      \"properties\": {\n        \"T\": {\n          \"$ref\": \"#/definitions/apiTypeMeta\",\n          \"title\": \"list of workloads to be monitored for the policy\"\n        },\n        \"O\": {\n          \"$ref\": \"#/definitions/apiObjectMeta\"\n        },\n        \"Spec\": {\n          \"$ref\": \"#/definitions/telemetryMonitoringPolicySpec\"\n        },\n        \"Status\": {\n          \"$ref\": \"#/definitions/telemetryMonitoringPolicyStatus\"\n        }\n      }\n    },\n    \"telemetryMonitoringPolicyList\": {\n      \"type\": \"object\",\n      \"properties\": {\n        \"T\": {\n          \"$ref\": \"#/definitions/apiTypeMeta\"\n        },\n        \"ListMeta\": {\n          \"$ref\": \"#/definitions/apiListMeta\"\n        },\n        \"Items\": {\n          \"type\": \"array\",\n          \"items\": {\n            \"$ref\": \"#/definitions/telemetryMonitoringPolicy\"\n          },\n          \"description\": \"Spec contains the configuration of the monitoring policy.\"\n        }\n      }\n    },\n    \"telemetryMonitoringPolicySpec\": {\n      \"type\": \"object\",\n      \"properties\": {\n        \"Area\": {\n          \"type\": \"array\",\n          \"items\": {\n            \"type\": \"string\"\n          }\n        },\n        \"ObjectSelector\": {\n          \"type\": \"object\",\n          \"additionalProperties\": {\n            \"type\": \"string\"\n          }\n        },\n        \"CollectionPolicy\": {\n          \"type\": \"string\"\n        },\n        \"RetentionPolicy\": {\n          \"type\": \"string\"\n        },\n        \"ExportPolicies\": {\n          \"type\": \"array\",\n          \"items\": {\n            \"type\": \"string\"\n          }\n        }\n      }\n    },\n    \"telemetryMonitoringPolicyStatus\": {\n      \"type\": \"object\",\n      \"properties\": {\n        \"Workloads\": {\n          \"type\": \"array\",\n          \"items\": {\n            \"type\": \"string\"\n          }\n        }\n      }\n    }\n  }\n}\n"),
	}

	// define dirs
	dir1 := &embedded.EmbeddedDir{
		Filename:   "",
		DirModTime: time.Unix(1514930677, 0),
		ChildFiles: []*embedded.EmbeddedFile{
			file2, // "telemetry.swagger.json"

		},
	}

	// link ChildDirs
	dir1.ChildDirs = []*embedded.EmbeddedDir{}

	// register embeddedBox
	embedded.RegisterEmbeddedBox(`../../../../../sw/api/generated/telemetry/swagger`, &embedded.EmbeddedBox{
		Name: `../../../../../sw/api/generated/telemetry/swagger`,
		Time: time.Unix(1514930677, 0),
		Dirs: map[string]*embedded.EmbeddedDir{
			"": dir1,
		},
		Files: map[string]*embedded.EmbeddedFile{
			"telemetry.swagger.json": file2,
		},
	})
}
