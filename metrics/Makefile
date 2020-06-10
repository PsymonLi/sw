# Generate metrics manifest.
# Each path where protos for metrics exist is listed below as a tuple of prefix and the path seperated by a :
IRIS_PATHS =\
	nicmgr:nic/proto/nicmgr                         \
	linkmgr:nic/proto/linkmgr                       \
	dropstats:nic/proto/dropstats                   \
	ftestats:nic/proto/ftestats                     \
	rulestats:nic/proto/rulestats                   \
	flowstats:nic/proto/flowstats                   \
	sysmond:nic/proto/sysmond                       \
	nmdmetrics:nic/agent/protos/nmd/nmdmetrics      \
	pciemgr:platform/src/app/pciemgrd/proto

# Apulu metric jsons are hand maintained in nic/operd/metrics.
APULU_PATHS =\
	nic/operd/metrics/venice \
	nic/operd/metrics/common

# all generated files are in the <pipeline>/generated/ path
all:
	$(MAKE) apulu-metrics
	$(MAKE) iris-metrics

apulu-metrics:
	@mkdir -p apulu/generated
	@- $(foreach PATH,$(APULU_PATHS), \
		find "${GOPATH}/src/github.com/pensando/sw/$(PATH)" -name "*_metrics.json" | xargs -I{} cp {} apulu/generated; \
	)
	# adding in pipeline info
	@for filename in apulu/generated/*_metrics.json; do\
		sed -i '/^.*Package.*$$/a \ \ "Pipeline": "apulu",' $$filename;\
	done

iris-metrics:
	@mkdir -p iris/generated iris/genfields
	@- $(foreach PATH,$(IRIS_PATHS), \
        $(eval PREFIX = $(word 1,$(subst :, ,$(PATH)))) \
        $(eval PROTOPATH = $(word 2,$(subst :, ,$(PATH)))) \
   		\
   		${GOPATH}/src/github.com/pensando/sw/tools/scripts/genmetrics.sh iris ${PREFIX} ${PROTOPATH}; \
		)

	# Auto generate field map assignment
	@- $(foreach PATH,$(IRIS_PATHS), \
        $(eval PREFIX = $(word 1,$(subst :, ,$(PATH)))) \
        $(eval PROTOPATH = $(word 2,$(subst :, ,$(PATH)))) \
		$(foreach protofile,$(shell find ${GOPATH}/src/github.com/pensando/sw/${PROTOPATH}/*.proto), \
			protoc -I. \
				-I${GOPATH}/src \
				-I${GOPATH}/src/github.com/pensando/sw/api \
				-I${GOPATH}/src/github.com/pensando/sw/api/fields \
				-I${GOPATH}/src/github.com/pensando/sw/api/labels \
				-I${GOPATH}/src/github.com/pensando/sw/api/protos \
				-I${GOPATH}/src/github.com/pensando/sw/events/protos \
				-I${GOPATH}/src/github.com/pensando/sw/vendor/github.com/pensando/grpc-gateway/third_party/googleapis \
				-I${GOPATH}/src/github.com/pensando/sw/vendor/github.com/pensando/grpc-gateway/third_party \
				-I${GOPATH}/src/github.com/pensando/sw/vendor/github.com/gogo/protobuf/protobuf \
				-I${GOPATH}/src/github.com/pensando/sw/vendor \
				-I${GOPATH}/src/github.com/pensando/sw/nic/delphi/proto/delphi \
				-I${GOPATH}/src/github.com/pensando/sw/venice/utils/apigen/annotations \
				-I${GOPATH}/src/github.com/pensando/sw/${PROTOPATH} \
				--grpc-gateway_out=S_prefix=${PREFIX},S_path=${GOPATH}/src/github.com/pensando/sw/metrics,logtostderr=false,v=7,gengw=false,templates=github.com/pensando/sw/metrics/templates/msgfieldmap.yaml,log_dir=${GOPATH}/src/github.com/pensando/sw/metrics/tmp:${GOPATH}/src/github.com/pensando/sw/metrics/iris/genfields \
				${protofile}; \
			) \
		)

	# goimports generated go files
	@goimports -local "github.com/pensando/sw" -w ${GOPATH}/src/github.com/pensando/sw/metrics/iris/genfields/*.go
	
clean:
	@rm -rf apulu/generated/*
	@rm -rf iris/generated/*
	@rm -rf iris/genfields/*map.go

gen-clean: clean
