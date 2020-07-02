package module

import (
	"context"
	"errors"
	"fmt"
	"reflect"
	"strings"
	"testing"

	diagapi "github.com/pensando/sw/api/generated/diagnostics"

	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestWriter(t *testing.T) {
	ti := &tInfo{}
	ti.setup()
	defer ti.shutdown()

	w := GetUpdater(clientName, ti.apiSrvAddr, nil, ti.logger)
	m := &diagapi.Module{}
	m.Defaults("All")
	m.Name = t.Name()
	m.Kind = string(diagapi.KindModule)
	m.Spec.LogLevel = diagapi.ModuleSpec_Info.String()
	err := w.Enqueue(m, Create)
	AssertOk(t, err, "unexpected error enqueuing diagnostic module for creation")
	AssertEventually(t, func() (bool, interface{}) {
		_, err := ti.apicl.DiagnosticsV1().Module().Get(context.TODO(), m.GetObjectMeta())
		if err != nil {
			return false, err
		}
		return true, nil
	}, fmt.Sprintf("module %#v not created", *m))
	m.Spec.LogLevel = diagapi.ModuleSpec_Debug.String()
	err = w.Enqueue(m, Update)
	AssertOk(t, err, "unexpected error enqueuing diagnostic module for update")
	AssertEventually(t, func() (bool, interface{}) {
		cm, err := ti.apicl.DiagnosticsV1().Module().Get(context.TODO(), m.GetObjectMeta())
		if err != nil {
			return false, err
		}
		if cm.Spec.LogLevel != diagapi.ModuleSpec_Debug.String() {
			return false, fmt.Errorf("unexpected log level: %v", cm.Spec.LogLevel)
		}
		return true, nil
	}, fmt.Sprintf("module %#v not created", *m))
	w.Stop()
	err = w.Enqueue(m, Delete)
	Assert(t, reflect.DeepEqual(err, errors.New("failed to enqueue diagnostic module")), fmt.Sprintf("unexpected error: %v", err))
	w.Start()
	err = w.Enqueue(m, Delete)
	AssertOk(t, err, "unexpected error enqueuing diagnostic module for deletion")
	AssertEventually(t, func() (bool, interface{}) {
		_, err := ti.apicl.DiagnosticsV1().Module().Get(context.TODO(), m.GetObjectMeta())
		if err != nil && strings.Contains(err.Error(), "NotFound") {
			return true, nil
		}
		return false, err
	}, fmt.Sprintf("module %#v not deleted", *m))
	w = GetUpdater(clientName, ti.apiSrvAddr, nil, ti.logger)
	w.Stop()
}
