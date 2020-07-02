package module

import (
	"context"
	"testing"

	diagapi "github.com/pensando/sw/api/generated/diagnostics"

	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestGetter(t *testing.T) {
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

	g := NewGetter(ti.apiSrvAddr, nil, ti.logger)
	AssertEventually(t, func() (bool, interface{}) {
		_, err := g.GetModule(t.Name())
		if err != nil {
			return false, err
		}
		return true, nil
	}, "unable to get module")
	ti.apicl.DiagnosticsV1().Module().Delete(context.TODO(), m.GetObjectMeta())
}
