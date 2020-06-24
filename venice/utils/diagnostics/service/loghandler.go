package service

import (
	"bufio"
	"context"
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"strconv"

	"github.com/gogo/protobuf/types"

	"github.com/pensando/sw/api"
	diagapi "github.com/pensando/sw/api/generated/diagnostics"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/diagnostics"
	"github.com/pensando/sw/venice/utils/diagnostics/protos"
	"github.com/pensando/sw/venice/utils/log"
)

const (
	// log parameters

	// Lines is name of the parameter containing number of lines of log to fetch from log file
	Lines = "lines"

	defaultLines    = 1000
	defaultLineSize = 150
)

// FileLogOption fills the optional parameters for file log handler
type FileLogOption func(*fileLogRetriever)

// WithFile passes a log file. Used for testing
func WithFile(file *os.File) FileLogOption {
	return func(f *fileLogRetriever) {
		f.file = file
	}
}

type fileLogRetriever struct {
	module   string
	node     string
	category diagapi.ModuleStatus_CategoryType
	logger   log.Logger
	file     *os.File
}

func (f *fileLogRetriever) Start() error {
	return nil
}

func (f *fileLogRetriever) Stop() {}

func (f *fileLogRetriever) HandleRequest(ctx context.Context, req *diagapi.DiagnosticsRequest) (*api.Any, error) {
	var err error
	logfile := f.file
	if logfile == nil { // not nil for testing
		module := f.module
		if module == globals.Rollout {
			module = "rollout"
		}
		fname := filepath.Join(globals.LogDir, fmt.Sprintf("%s.%s", module, "log"))
		logfile, err = os.Open(fname)
		if err != nil {
			f.logger.ErrorLog("method", "HandleRequest", "msg", "error opening log file", "error", err)
			return nil, err
		}
	}
	defer func() {
		logfile.Close()
	}()
	fileinfo, err := os.Stat(logfile.Name())
	if err != nil {
		f.logger.ErrorLog("method", "HandleRequest", "msg", "error in stat file", "error", err)
		return nil, err
	}
	lines := defaultLines
	val, ok := req.Parameters[Lines]
	if ok {
		lines, err = strconv.Atoi(val)
		if err != nil {
			f.logger.ErrorLog("method", "HandleRequest", "msg", "illegal lines parameter", "error", err)
			return nil, err
		}
	}

	var offset int64
	if fileinfo.Size() < int64(lines*defaultLineSize) {
		offset = fileinfo.Size()
	} else {
		offset = int64(lines * defaultLineSize)
	}
	_, err = logfile.Seek(-offset, 2)
	if err != nil {
		f.logger.ErrorLog("method", "HandleRequest", "msg", "error in seek file", "error", err)
		return nil, err
	}

	r := bufio.NewReader(logfile) // assumes each line in log file is < 4096 bytes
	var data []byte
	if offset != fileinfo.Size() {
		// ignore first line read if not reading from the beginning of the file
		data, err = r.ReadBytes('\n')
	}
	logs := &protos.Logs{}
	for i := 0; i < lines; i++ {
		var logMsg protos.Log
		if data, err = r.ReadBytes('\n'); err == nil {
			json.Unmarshal(data, &logMsg.Log)
			logs.Logs = append(logs.Logs, &logMsg)
		} else {
			break
		}
	}
	anyObj, err := types.MarshalAny(logs)
	if err != nil {
		f.logger.ErrorLog("method", "HandleRequest", "msg", "unable to marshal logs {%+v} to Any object", logs, "err", err)
		return nil, err
	}
	return &api.Any{Any: *anyObj}, nil
}

// NewFileLogHandler returns handler to fetch logs from file
func NewFileLogHandler(module, node string, category diagapi.ModuleStatus_CategoryType, logger log.Logger, opts ...FileLogOption) diagnostics.Handler {
	svc := &fileLogRetriever{
		module:   module,
		node:     node,
		category: category,
		logger:   logger,
	}
	for _, f := range opts {
		f(svc)
	}
	return svc
}
