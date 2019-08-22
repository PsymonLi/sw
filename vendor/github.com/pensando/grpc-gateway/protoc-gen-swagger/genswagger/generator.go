package genswagger

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"path/filepath"
	"strings"

	"github.com/gogo/protobuf/proto"
	plugin "github.com/gogo/protobuf/protoc-gen-gogo/plugin"
	"github.com/golang/glog"
	"github.com/pensando/grpc-gateway/protoc-gen-grpc-gateway/descriptor"
	gen "github.com/pensando/grpc-gateway/protoc-gen-grpc-gateway/generator"
)

var (
	errNoTargetService = errors.New("no target service defined in the file")
)

type generator struct {
	reg  *descriptor.Registry
	opts Opts
}

// New returns a new generator which generates grpc gateway files.
func New(reg *descriptor.Registry, opts Opts) gen.Generator {
	return &generator{reg: reg, opts: opts}
}

var Finalizers []Finalizer

func (g *generator) Generate(targets []*descriptor.File) ([]*plugin.CodeGeneratorResponse_File, error) {
	var files []*plugin.CodeGeneratorResponse_File
	for _, file := range targets {
		glog.V(1).Infof("Processing %s", file.GetName())
		code, err := applyTemplate(param{File: file, reg: g.reg, opts: g.opts})
		if err == errNoTargetService {
			glog.V(1).Infof("%s: %v", file.GetName(), err)
			continue
		}
		if err != nil {
			return nil, err
		}

		var formatted bytes.Buffer
		json.Indent(&formatted, []byte(code), "", "  ")

		name := file.GetName()
		ext := filepath.Ext(name)
		base := strings.TrimSuffix(name, ext)
		output := fmt.Sprintf("%s.swagger.json", base)
		files = append(files, &plugin.CodeGeneratorResponse_File{
			Name:    proto.String(output),
			Content: proto.String(formatted.String()),
		})
		glog.V(1).Infof("Will emit %s", output)
	}
	return files, nil
}

func (g *generator) GenerateFromTemplates(targets *descriptor.File, paths []gen.TemplateDef, index int) ([]*plugin.CodeGeneratorResponse_File, error) {
	var ret []*plugin.CodeGeneratorResponse_File
	return ret, nil
}
