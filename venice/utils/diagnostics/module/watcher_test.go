package module

import (
	"context"
	"fmt"
	"testing"

	diagapi "github.com/pensando/sw/api/generated/diagnostics"

	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestWatcher(t *testing.T) {
	ti := &tInfo{}
	ti.setup()
	defer ti.shutdown()

	m := &diagapi.Module{}
	m.Defaults("All")
	m.Name = t.Name()
	m.Kind = string(diagapi.KindModule)
	m.Spec.LogLevel = diagapi.ModuleSpec_Info.String()

	AssertEventually(t, func() (bool, interface{}) {
		_, err := ti.apicl.DiagnosticsV1().Module().Create(context.TODO(), m)
		if err != nil {
			return false, err
		}
		return true, nil
	}, "unable to create diagnostic module")

	mockFn := func(module *diagapi.Module) {
		Assert(t, module.Name == t.Name(), fmt.Sprintf("unexpected module %v", module.Name))
	}
	w := GetWatcher(t.Name(), ti.apiSrvAddr, nil, ti.logger, mockFn)
	AssertEventually(t, func() (bool, interface{}) {
		return w.Module().Name == t.Name(), nil
	}, fmt.Sprintf("unexpected module %#v", w.Module()))
	w.Stop()
	w.Start()
	AssertEventually(t, func() (bool, interface{}) {
		return w.Module().Name == t.Name(), nil
	}, fmt.Sprintf("unexpected module %#v", w.Module()))
	ti.apicl.DiagnosticsV1().Module().Delete(context.TODO(), m.GetObjectMeta())
	w.Stop()
}
