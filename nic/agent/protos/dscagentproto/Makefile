EXTRA_INCLUDES =  -I${GOPATH}/src/github.com/pensando/sw/api/protos
EXTRA_GOFAST_OUT = Mgithub.com/pensando/sw/api/protos/smartnic.proto=github.com/pensando/sw/api/generated/cluster,
SERVICE_PROTOS := $(shell ls *.proto)
include ../../../../venice/Makefile.proto

default: $(SERVICE_PROTOS:.proto=.pb.go)
%.pb.go: %.proto Makefile
	protoc -I/usr/local/include -I. -I../../../../vendor -I${GOPATH}/src \
	-I../../../../vendor/github.com/pensando/grpc-gateway/third_party \
	-I../../../../vendor/github.com/pensando/grpc-gateway/third_party/googleapis \
	-I../../../../vendor/github.com/gogo/protobuf/protobuf \
        --gofast_out=plugins=grpc:. *.proto

clean:
	rm -f *.pb.go

gen-clean: clean