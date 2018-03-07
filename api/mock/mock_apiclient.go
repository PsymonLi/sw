// Code generated by MockGen. DO NOT EDIT.
// Source: ../generated/apiclient/client.go

package mock

import (
	reflect "reflect"

	gomock "github.com/golang/mock/gomock"

	alerts "github.com/pensando/sw/api/generated/alerts"
	app "github.com/pensando/sw/api/generated/app"
	auth "github.com/pensando/sw/api/generated/auth"
	bookstore "github.com/pensando/sw/api/generated/bookstore"
	cmd "github.com/pensando/sw/api/generated/cmd"
	events "github.com/pensando/sw/api/generated/events"
	network "github.com/pensando/sw/api/generated/network"
	networkencryption "github.com/pensando/sw/api/generated/networkencryption"
	telemetry "github.com/pensando/sw/api/generated/telemetry"
	x509 "github.com/pensando/sw/api/generated/x509"
)

// MockServices is a mock of Services interface
type MockServices struct {
	ctrl     *gomock.Controller
	recorder *MockServicesMockRecorder
}

// MockServicesMockRecorder is the mock recorder for MockServices
type MockServicesMockRecorder struct {
	mock *MockServices
}

// NewMockServices creates a new mock instance
func NewMockServices(ctrl *gomock.Controller) *MockServices {
	mock := &MockServices{ctrl: ctrl}
	mock.recorder = &MockServicesMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use
func (_m *MockServices) EXPECT() *MockServicesMockRecorder {
	return _m.recorder
}

// Close mocks base method
func (_m *MockServices) Close() error {
	ret := _m.ctrl.Call(_m, "Close")
	ret0, _ := ret[0].(error)
	return ret0
}

// Close indicates an expected call of Close
func (_mr *MockServicesMockRecorder) Close() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "Close", reflect.TypeOf((*MockServices)(nil).Close))
}

// AlertDestinationV1 mocks base method
func (_m *MockServices) AlertDestinationV1() alerts.AlertDestinationV1Interface {
	ret := _m.ctrl.Call(_m, "AlertDestinationV1")
	ret0, _ := ret[0].(alerts.AlertDestinationV1Interface)
	return ret0
}

// AlertDestinationV1 indicates an expected call of AlertDestinationV1
func (_mr *MockServicesMockRecorder) AlertDestinationV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "AlertDestinationV1", reflect.TypeOf((*MockServices)(nil).AlertDestinationV1))
}

// AlertPolicyV1 mocks base method
func (_m *MockServices) AlertPolicyV1() alerts.AlertPolicyV1Interface {
	ret := _m.ctrl.Call(_m, "AlertPolicyV1")
	ret0, _ := ret[0].(alerts.AlertPolicyV1Interface)
	return ret0
}

// AlertPolicyV1 indicates an expected call of AlertPolicyV1
func (_mr *MockServicesMockRecorder) AlertPolicyV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "AlertPolicyV1", reflect.TypeOf((*MockServices)(nil).AlertPolicyV1))
}

// AppV1 mocks base method
func (_m *MockServices) AppV1() app.AppV1Interface {
	ret := _m.ctrl.Call(_m, "AppV1")
	ret0, _ := ret[0].(app.AppV1Interface)
	return ret0
}

// AppV1 indicates an expected call of AppV1
func (_mr *MockServicesMockRecorder) AppV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "AppV1", reflect.TypeOf((*MockServices)(nil).AppV1))
}

// AuthV1 mocks base method
func (_m *MockServices) AuthV1() auth.AuthV1Interface {
	ret := _m.ctrl.Call(_m, "AuthV1")
	ret0, _ := ret[0].(auth.AuthV1Interface)
	return ret0
}

// AuthV1 indicates an expected call of AuthV1
func (_mr *MockServicesMockRecorder) AuthV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "AuthV1", reflect.TypeOf((*MockServices)(nil).AuthV1))
}

// BookstoreV1 mocks base method
func (_m *MockServices) BookstoreV1() bookstore.BookstoreV1Interface {
	ret := _m.ctrl.Call(_m, "BookstoreV1")
	ret0, _ := ret[0].(bookstore.BookstoreV1Interface)
	return ret0
}

// BookstoreV1 indicates an expected call of BookstoreV1
func (_mr *MockServicesMockRecorder) BookstoreV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "BookstoreV1", reflect.TypeOf((*MockServices)(nil).BookstoreV1))
}

// CmdV1 mocks base method
func (_m *MockServices) CmdV1() cmd.CmdV1Interface {
	ret := _m.ctrl.Call(_m, "CmdV1")
	ret0, _ := ret[0].(cmd.CmdV1Interface)
	return ret0
}

// CmdV1 indicates an expected call of CmdV1
func (_mr *MockServicesMockRecorder) CmdV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "CmdV1", reflect.TypeOf((*MockServices)(nil).CmdV1))
}

// EventPolicyV1 mocks base method
func (_m *MockServices) EventPolicyV1() events.EventPolicyV1Interface {
	ret := _m.ctrl.Call(_m, "EventPolicyV1")
	ret0, _ := ret[0].(events.EventPolicyV1Interface)
	return ret0
}

// EventPolicyV1 indicates an expected call of EventPolicyV1
func (_mr *MockServicesMockRecorder) EventPolicyV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "EventPolicyV1", reflect.TypeOf((*MockServices)(nil).EventPolicyV1))
}

// EndpointV1 mocks base method
func (_m *MockServices) EndpointV1() network.EndpointV1Interface {
	ret := _m.ctrl.Call(_m, "EndpointV1")
	ret0, _ := ret[0].(network.EndpointV1Interface)
	return ret0
}

// EndpointV1 indicates an expected call of EndpointV1
func (_mr *MockServicesMockRecorder) EndpointV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "EndpointV1", reflect.TypeOf((*MockServices)(nil).EndpointV1))
}

// LbPolicyV1 mocks base method
func (_m *MockServices) LbPolicyV1() network.LbPolicyV1Interface {
	ret := _m.ctrl.Call(_m, "LbPolicyV1")
	ret0, _ := ret[0].(network.LbPolicyV1Interface)
	return ret0
}

// LbPolicyV1 indicates an expected call of LbPolicyV1
func (_mr *MockServicesMockRecorder) LbPolicyV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "LbPolicyV1", reflect.TypeOf((*MockServices)(nil).LbPolicyV1))
}

// NetworkV1 mocks base method
func (_m *MockServices) NetworkV1() network.NetworkV1Interface {
	ret := _m.ctrl.Call(_m, "NetworkV1")
	ret0, _ := ret[0].(network.NetworkV1Interface)
	return ret0
}

// NetworkV1 indicates an expected call of NetworkV1
func (_mr *MockServicesMockRecorder) NetworkV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "NetworkV1", reflect.TypeOf((*MockServices)(nil).NetworkV1))
}

// SecurityGroupV1 mocks base method
func (_m *MockServices) SecurityGroupV1() network.SecurityGroupV1Interface {
	ret := _m.ctrl.Call(_m, "SecurityGroupV1")
	ret0, _ := ret[0].(network.SecurityGroupV1Interface)
	return ret0
}

// SecurityGroupV1 indicates an expected call of SecurityGroupV1
func (_mr *MockServicesMockRecorder) SecurityGroupV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "SecurityGroupV1", reflect.TypeOf((*MockServices)(nil).SecurityGroupV1))
}

// ServiceV1 mocks base method
func (_m *MockServices) ServiceV1() network.ServiceV1Interface {
	ret := _m.ctrl.Call(_m, "ServiceV1")
	ret0, _ := ret[0].(network.ServiceV1Interface)
	return ret0
}

// ServiceV1 indicates an expected call of ServiceV1
func (_mr *MockServicesMockRecorder) ServiceV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "ServiceV1", reflect.TypeOf((*MockServices)(nil).ServiceV1))
}

// SgpolicyV1 mocks base method
func (_m *MockServices) SgpolicyV1() network.SgpolicyV1Interface {
	ret := _m.ctrl.Call(_m, "SgpolicyV1")
	ret0, _ := ret[0].(network.SgpolicyV1Interface)
	return ret0
}

// SgpolicyV1 indicates an expected call of SgpolicyV1
func (_mr *MockServicesMockRecorder) SgpolicyV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "SgpolicyV1", reflect.TypeOf((*MockServices)(nil).SgpolicyV1))
}

// TenantV1 mocks base method
func (_m *MockServices) TenantV1() network.TenantV1Interface {
	ret := _m.ctrl.Call(_m, "TenantV1")
	ret0, _ := ret[0].(network.TenantV1Interface)
	return ret0
}

// TenantV1 indicates an expected call of TenantV1
func (_mr *MockServicesMockRecorder) TenantV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "TenantV1", reflect.TypeOf((*MockServices)(nil).TenantV1))
}

// TrafficEncryptionPolicyV1 mocks base method
func (_m *MockServices) TrafficEncryptionPolicyV1() networkencryption.TrafficEncryptionPolicyV1Interface {
	ret := _m.ctrl.Call(_m, "TrafficEncryptionPolicyV1")
	ret0, _ := ret[0].(networkencryption.TrafficEncryptionPolicyV1Interface)
	return ret0
}

// TrafficEncryptionPolicyV1 indicates an expected call of TrafficEncryptionPolicyV1
func (_mr *MockServicesMockRecorder) TrafficEncryptionPolicyV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "TrafficEncryptionPolicyV1", reflect.TypeOf((*MockServices)(nil).TrafficEncryptionPolicyV1))
}

// FlowExportPolicyV1 mocks base method
func (_m *MockServices) FlowExportPolicyV1() telemetry.FlowExportPolicyV1Interface {
	ret := _m.ctrl.Call(_m, "FlowExportPolicyV1")
	ret0, _ := ret[0].(telemetry.FlowExportPolicyV1Interface)
	return ret0
}

// FlowExportPolicyV1 indicates an expected call of FlowExportPolicyV1
func (_mr *MockServicesMockRecorder) FlowExportPolicyV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "FlowExportPolicyV1", reflect.TypeOf((*MockServices)(nil).FlowExportPolicyV1))
}

// FwlogPolicyV1 mocks base method
func (_m *MockServices) FwlogPolicyV1() telemetry.FwlogPolicyV1Interface {
	ret := _m.ctrl.Call(_m, "FwlogPolicyV1")
	ret0, _ := ret[0].(telemetry.FwlogPolicyV1Interface)
	return ret0
}

// FwlogPolicyV1 indicates an expected call of FwlogPolicyV1
func (_mr *MockServicesMockRecorder) FwlogPolicyV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "FwlogPolicyV1", reflect.TypeOf((*MockServices)(nil).FwlogPolicyV1))
}

// StatsPolicyV1 mocks base method
func (_m *MockServices) StatsPolicyV1() telemetry.StatsPolicyV1Interface {
	ret := _m.ctrl.Call(_m, "StatsPolicyV1")
	ret0, _ := ret[0].(telemetry.StatsPolicyV1Interface)
	return ret0
}

// StatsPolicyV1 indicates an expected call of StatsPolicyV1
func (_mr *MockServicesMockRecorder) StatsPolicyV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "StatsPolicyV1", reflect.TypeOf((*MockServices)(nil).StatsPolicyV1))
}

// CertificateV1 mocks base method
func (_m *MockServices) CertificateV1() x509.CertificateV1Interface {
	ret := _m.ctrl.Call(_m, "CertificateV1")
	ret0, _ := ret[0].(x509.CertificateV1Interface)
	return ret0
}

// CertificateV1 indicates an expected call of CertificateV1
func (_mr *MockServicesMockRecorder) CertificateV1() *gomock.Call {
	return _mr.mock.ctrl.RecordCallWithMethodType(_mr.mock, "CertificateV1", reflect.TypeOf((*MockServices)(nil).CertificateV1))
}
