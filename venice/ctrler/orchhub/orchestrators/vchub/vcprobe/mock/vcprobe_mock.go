// Code generated by MockGen. DO NOT EDIT.
// Source: ../vcprobe.go

// Package mock is a generated GoMock package.
package mock

import (
	reflect "reflect"

	gomock "github.com/golang/mock/gomock"
	object "github.com/vmware/govmomi/object"
	mo "github.com/vmware/govmomi/vim25/mo"
	types "github.com/vmware/govmomi/vim25/types"

	vcprobe "github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe"
)

// MockProbeInf is a mock of ProbeInf interface
type MockProbeInf struct {
	ctrl     *gomock.Controller
	recorder *MockProbeInfMockRecorder
}

// MockProbeInfMockRecorder is the mock recorder for MockProbeInf
type MockProbeInfMockRecorder struct {
	mock *MockProbeInf
}

// NewMockProbeInf creates a new mock instance
func NewMockProbeInf(ctrl *gomock.Controller) *MockProbeInf {
	mock := &MockProbeInf{ctrl: ctrl}
	mock.recorder = &MockProbeInfMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use
func (m *MockProbeInf) EXPECT() *MockProbeInfMockRecorder {
	return m.recorder
}

// Start mocks base method
func (m *MockProbeInf) Start(vcWriteOnly bool) error {
	ret := m.ctrl.Call(m, "Start", vcWriteOnly)
	ret0, _ := ret[0].(error)
	return ret0
}

// Start indicates an expected call of Start
func (mr *MockProbeInfMockRecorder) Start(vcWriteOnly interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Start", reflect.TypeOf((*MockProbeInf)(nil).Start), vcWriteOnly)
}

// IsSessionReady mocks base method
func (m *MockProbeInf) IsSessionReady() bool {
	ret := m.ctrl.Call(m, "IsSessionReady")
	ret0, _ := ret[0].(bool)
	return ret0
}

// IsSessionReady indicates an expected call of IsSessionReady
func (mr *MockProbeInfMockRecorder) IsSessionReady() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "IsSessionReady", reflect.TypeOf((*MockProbeInf)(nil).IsSessionReady))
}

// ClearState mocks base method
func (m *MockProbeInf) ClearState() {
	m.ctrl.Call(m, "ClearState")
}

// ClearState indicates an expected call of ClearState
func (mr *MockProbeInfMockRecorder) ClearState() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "ClearState", reflect.TypeOf((*MockProbeInf)(nil).ClearState))
}

// StartWatchers mocks base method
func (m *MockProbeInf) StartWatchers() {
	m.ctrl.Call(m, "StartWatchers")
}

// StartWatchers indicates an expected call of StartWatchers
func (mr *MockProbeInfMockRecorder) StartWatchers() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "StartWatchers", reflect.TypeOf((*MockProbeInf)(nil).StartWatchers))
}

// ListVM mocks base method
func (m *MockProbeInf) ListVM(dcRef *types.ManagedObjectReference) []mo.VirtualMachine {
	ret := m.ctrl.Call(m, "ListVM", dcRef)
	ret0, _ := ret[0].([]mo.VirtualMachine)
	return ret0
}

// ListVM indicates an expected call of ListVM
func (mr *MockProbeInfMockRecorder) ListVM(dcRef interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "ListVM", reflect.TypeOf((*MockProbeInf)(nil).ListVM), dcRef)
}

// ListDC mocks base method
func (m *MockProbeInf) ListDC() []mo.Datacenter {
	ret := m.ctrl.Call(m, "ListDC")
	ret0, _ := ret[0].([]mo.Datacenter)
	return ret0
}

// ListDC indicates an expected call of ListDC
func (mr *MockProbeInfMockRecorder) ListDC() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "ListDC", reflect.TypeOf((*MockProbeInf)(nil).ListDC))
}

// ListDVS mocks base method
func (m *MockProbeInf) ListDVS(dcRef *types.ManagedObjectReference) []mo.VmwareDistributedVirtualSwitch {
	ret := m.ctrl.Call(m, "ListDVS", dcRef)
	ret0, _ := ret[0].([]mo.VmwareDistributedVirtualSwitch)
	return ret0
}

// ListDVS indicates an expected call of ListDVS
func (mr *MockProbeInfMockRecorder) ListDVS(dcRef interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "ListDVS", reflect.TypeOf((*MockProbeInf)(nil).ListDVS), dcRef)
}

// ListPG mocks base method
func (m *MockProbeInf) ListPG(dcRef *types.ManagedObjectReference) []mo.DistributedVirtualPortgroup {
	ret := m.ctrl.Call(m, "ListPG", dcRef)
	ret0, _ := ret[0].([]mo.DistributedVirtualPortgroup)
	return ret0
}

// ListPG indicates an expected call of ListPG
func (mr *MockProbeInfMockRecorder) ListPG(dcRef interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "ListPG", reflect.TypeOf((*MockProbeInf)(nil).ListPG), dcRef)
}

// ListHosts mocks base method
func (m *MockProbeInf) ListHosts(dcRef *types.ManagedObjectReference) []mo.HostSystem {
	ret := m.ctrl.Call(m, "ListHosts", dcRef)
	ret0, _ := ret[0].([]mo.HostSystem)
	return ret0
}

// ListHosts indicates an expected call of ListHosts
func (mr *MockProbeInfMockRecorder) ListHosts(dcRef interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "ListHosts", reflect.TypeOf((*MockProbeInf)(nil).ListHosts), dcRef)
}

// StopWatchForDC mocks base method
func (m *MockProbeInf) StopWatchForDC(dcName, dcID string) {
	m.ctrl.Call(m, "StopWatchForDC", dcName, dcID)
}

// StopWatchForDC indicates an expected call of StopWatchForDC
func (mr *MockProbeInfMockRecorder) StopWatchForDC(dcName, dcID interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "StopWatchForDC", reflect.TypeOf((*MockProbeInf)(nil).StopWatchForDC), dcName, dcID)
}

// StartWatchForDC mocks base method
func (m *MockProbeInf) StartWatchForDC(dcName, dcID string) {
	m.ctrl.Call(m, "StartWatchForDC", dcName, dcID)
}

// StartWatchForDC indicates an expected call of StartWatchForDC
func (mr *MockProbeInfMockRecorder) StartWatchForDC(dcName, dcID interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "StartWatchForDC", reflect.TypeOf((*MockProbeInf)(nil).StartWatchForDC), dcName, dcID)
}

// GetVM mocks base method
func (m *MockProbeInf) GetVM(vmID string) (mo.VirtualMachine, error) {
	ret := m.ctrl.Call(m, "GetVM", vmID)
	ret0, _ := ret[0].(mo.VirtualMachine)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetVM indicates an expected call of GetVM
func (mr *MockProbeInfMockRecorder) GetVM(vmID interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetVM", reflect.TypeOf((*MockProbeInf)(nil).GetVM), vmID)
}

// AddPenDC mocks base method
func (m *MockProbeInf) AddPenDC(dcName string, retry int) error {
	ret := m.ctrl.Call(m, "AddPenDC", dcName, retry)
	ret0, _ := ret[0].(error)
	return ret0
}

// AddPenDC indicates an expected call of AddPenDC
func (mr *MockProbeInfMockRecorder) AddPenDC(dcName, retry interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AddPenDC", reflect.TypeOf((*MockProbeInf)(nil).AddPenDC), dcName, retry)
}

// RemovePenDC mocks base method
func (m *MockProbeInf) RemovePenDC(dcName string, retry int) error {
	ret := m.ctrl.Call(m, "RemovePenDC", dcName, retry)
	ret0, _ := ret[0].(error)
	return ret0
}

// RemovePenDC indicates an expected call of RemovePenDC
func (mr *MockProbeInfMockRecorder) RemovePenDC(dcName, retry interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "RemovePenDC", reflect.TypeOf((*MockProbeInf)(nil).RemovePenDC), dcName, retry)
}

// AddPenPG mocks base method
func (m *MockProbeInf) AddPenPG(dcName, dvsName string, pgConfigSpec *types.DVPortgroupConfigSpec, retry int) error {
	ret := m.ctrl.Call(m, "AddPenPG", dcName, dvsName, pgConfigSpec, retry)
	ret0, _ := ret[0].(error)
	return ret0
}

// AddPenPG indicates an expected call of AddPenPG
func (mr *MockProbeInfMockRecorder) AddPenPG(dcName, dvsName, pgConfigSpec, retry interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AddPenPG", reflect.TypeOf((*MockProbeInf)(nil).AddPenPG), dcName, dvsName, pgConfigSpec, retry)
}

// GetPenPG mocks base method
func (m *MockProbeInf) GetPenPG(dcName, pgName string, retry int) (*object.DistributedVirtualPortgroup, error) {
	ret := m.ctrl.Call(m, "GetPenPG", dcName, pgName, retry)
	ret0, _ := ret[0].(*object.DistributedVirtualPortgroup)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetPenPG indicates an expected call of GetPenPG
func (mr *MockProbeInfMockRecorder) GetPenPG(dcName, pgName, retry interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetPenPG", reflect.TypeOf((*MockProbeInf)(nil).GetPenPG), dcName, pgName, retry)
}

// GetPGConfig mocks base method
func (m *MockProbeInf) GetPGConfig(dcName, pgName string, ps []string, retry int) (*mo.DistributedVirtualPortgroup, error) {
	ret := m.ctrl.Call(m, "GetPGConfig", dcName, pgName, ps, retry)
	ret0, _ := ret[0].(*mo.DistributedVirtualPortgroup)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetPGConfig indicates an expected call of GetPGConfig
func (mr *MockProbeInfMockRecorder) GetPGConfig(dcName, pgName, ps, retry interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetPGConfig", reflect.TypeOf((*MockProbeInf)(nil).GetPGConfig), dcName, pgName, ps, retry)
}

// RenamePG mocks base method
func (m *MockProbeInf) RenamePG(dcName, oldName, newName string, retry int) error {
	ret := m.ctrl.Call(m, "RenamePG", dcName, oldName, newName, retry)
	ret0, _ := ret[0].(error)
	return ret0
}

// RenamePG indicates an expected call of RenamePG
func (mr *MockProbeInfMockRecorder) RenamePG(dcName, oldName, newName, retry interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "RenamePG", reflect.TypeOf((*MockProbeInf)(nil).RenamePG), dcName, oldName, newName, retry)
}

// RemovePenPG mocks base method
func (m *MockProbeInf) RemovePenPG(dcName, pgName string, retry int) error {
	ret := m.ctrl.Call(m, "RemovePenPG", dcName, pgName, retry)
	ret0, _ := ret[0].(error)
	return ret0
}

// RemovePenPG indicates an expected call of RemovePenPG
func (mr *MockProbeInfMockRecorder) RemovePenPG(dcName, pgName, retry interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "RemovePenPG", reflect.TypeOf((*MockProbeInf)(nil).RemovePenPG), dcName, pgName, retry)
}

// AddPenDVS mocks base method
func (m *MockProbeInf) AddPenDVS(dcName string, dvsCreateSpec *types.DVSCreateSpec, retry int) error {
	ret := m.ctrl.Call(m, "AddPenDVS", dcName, dvsCreateSpec, retry)
	ret0, _ := ret[0].(error)
	return ret0
}

// AddPenDVS indicates an expected call of AddPenDVS
func (mr *MockProbeInfMockRecorder) AddPenDVS(dcName, dvsCreateSpec, retry interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AddPenDVS", reflect.TypeOf((*MockProbeInf)(nil).AddPenDVS), dcName, dvsCreateSpec, retry)
}

// GetPenDVS mocks base method
func (m *MockProbeInf) GetPenDVS(dcName, dvsName string, retry int) (*object.DistributedVirtualSwitch, error) {
	ret := m.ctrl.Call(m, "GetPenDVS", dcName, dvsName, retry)
	ret0, _ := ret[0].(*object.DistributedVirtualSwitch)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetPenDVS indicates an expected call of GetPenDVS
func (mr *MockProbeInfMockRecorder) GetPenDVS(dcName, dvsName, retry interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetPenDVS", reflect.TypeOf((*MockProbeInf)(nil).GetPenDVS), dcName, dvsName, retry)
}

// UpdateDVSPortsVlan mocks base method
func (m *MockProbeInf) UpdateDVSPortsVlan(dcName, dvsName string, portsSetting vcprobe.PenDVSPortSettings, retry int) error {
	ret := m.ctrl.Call(m, "UpdateDVSPortsVlan", dcName, dvsName, portsSetting, retry)
	ret0, _ := ret[0].(error)
	return ret0
}

// UpdateDVSPortsVlan indicates an expected call of UpdateDVSPortsVlan
func (mr *MockProbeInfMockRecorder) UpdateDVSPortsVlan(dcName, dvsName, portsSetting, retry interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "UpdateDVSPortsVlan", reflect.TypeOf((*MockProbeInf)(nil).UpdateDVSPortsVlan), dcName, dvsName, portsSetting, retry)
}

// GetPenDVSPorts mocks base method
func (m *MockProbeInf) GetPenDVSPorts(dcName, dvsName string, criteria *types.DistributedVirtualSwitchPortCriteria, retry int) ([]types.DistributedVirtualPort, error) {
	ret := m.ctrl.Call(m, "GetPenDVSPorts", dcName, dvsName, criteria, retry)
	ret0, _ := ret[0].([]types.DistributedVirtualPort)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetPenDVSPorts indicates an expected call of GetPenDVSPorts
func (mr *MockProbeInfMockRecorder) GetPenDVSPorts(dcName, dvsName, criteria, retry interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetPenDVSPorts", reflect.TypeOf((*MockProbeInf)(nil).GetPenDVSPorts), dcName, dvsName, criteria, retry)
}

// RemovePenDVS mocks base method
func (m *MockProbeInf) RemovePenDVS(dcName, dvsName string, retry int) error {
	ret := m.ctrl.Call(m, "RemovePenDVS", dcName, dvsName, retry)
	ret0, _ := ret[0].(error)
	return ret0
}

// RemovePenDVS indicates an expected call of RemovePenDVS
func (mr *MockProbeInfMockRecorder) RemovePenDVS(dcName, dvsName, retry interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "RemovePenDVS", reflect.TypeOf((*MockProbeInf)(nil).RemovePenDVS), dcName, dvsName, retry)
}

// TagObjAsManaged mocks base method
func (m *MockProbeInf) TagObjAsManaged(ref types.ManagedObjectReference) error {
	ret := m.ctrl.Call(m, "TagObjAsManaged", ref)
	ret0, _ := ret[0].(error)
	return ret0
}

// TagObjAsManaged indicates an expected call of TagObjAsManaged
func (mr *MockProbeInfMockRecorder) TagObjAsManaged(ref interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "TagObjAsManaged", reflect.TypeOf((*MockProbeInf)(nil).TagObjAsManaged), ref)
}

// RemoveTagObjManaged mocks base method
func (m *MockProbeInf) RemoveTagObjManaged(ref types.ManagedObjectReference) error {
	ret := m.ctrl.Call(m, "RemoveTagObjManaged", ref)
	ret0, _ := ret[0].(error)
	return ret0
}

// RemoveTagObjManaged indicates an expected call of RemoveTagObjManaged
func (mr *MockProbeInfMockRecorder) RemoveTagObjManaged(ref interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "RemoveTagObjManaged", reflect.TypeOf((*MockProbeInf)(nil).RemoveTagObjManaged), ref)
}

// TagObjWithVlan mocks base method
func (m *MockProbeInf) TagObjWithVlan(ref types.ManagedObjectReference, vlan int) error {
	ret := m.ctrl.Call(m, "TagObjWithVlan", ref, vlan)
	ret0, _ := ret[0].(error)
	return ret0
}

// TagObjWithVlan indicates an expected call of TagObjWithVlan
func (mr *MockProbeInfMockRecorder) TagObjWithVlan(ref, vlan interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "TagObjWithVlan", reflect.TypeOf((*MockProbeInf)(nil).TagObjWithVlan), ref, vlan)
}

// RemoveTagObjVlan mocks base method
func (m *MockProbeInf) RemoveTagObjVlan(ref types.ManagedObjectReference) error {
	ret := m.ctrl.Call(m, "RemoveTagObjVlan", ref)
	ret0, _ := ret[0].(error)
	return ret0
}

// RemoveTagObjVlan indicates an expected call of RemoveTagObjVlan
func (mr *MockProbeInfMockRecorder) RemoveTagObjVlan(ref interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "RemoveTagObjVlan", reflect.TypeOf((*MockProbeInf)(nil).RemoveTagObjVlan), ref)
}

// RemovePensandoTags mocks base method
func (m *MockProbeInf) RemovePensandoTags(ref types.ManagedObjectReference) []error {
	ret := m.ctrl.Call(m, "RemovePensandoTags", ref)
	ret0, _ := ret[0].([]error)
	return ret0
}

// RemovePensandoTags indicates an expected call of RemovePensandoTags
func (mr *MockProbeInfMockRecorder) RemovePensandoTags(ref interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "RemovePensandoTags", reflect.TypeOf((*MockProbeInf)(nil).RemovePensandoTags), ref)
}
