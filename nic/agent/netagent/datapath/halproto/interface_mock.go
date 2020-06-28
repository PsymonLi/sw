// Code generated by MockGen. DO NOT EDIT.
// Source: interface.pb.go

// Package halproto is a generated GoMock package.
package halproto

import (
	reflect "reflect"

	gomock "github.com/golang/mock/gomock"
	context "golang.org/x/net/context"
	grpc "google.golang.org/grpc"
)

// MockisLifStats_Lifs is a mock of isLifStats_Lifs interface
type MockisLifStats_Lifs struct {
	ctrl     *gomock.Controller
	recorder *MockisLifStats_LifsMockRecorder
}

// MockisLifStats_LifsMockRecorder is the mock recorder for MockisLifStats_Lifs
type MockisLifStats_LifsMockRecorder struct {
	mock *MockisLifStats_Lifs
}

// NewMockisLifStats_Lifs creates a new mock instance
func NewMockisLifStats_Lifs(ctrl *gomock.Controller) *MockisLifStats_Lifs {
	mock := &MockisLifStats_Lifs{ctrl: ctrl}
	mock.recorder = &MockisLifStats_LifsMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use
func (m *MockisLifStats_Lifs) EXPECT() *MockisLifStats_LifsMockRecorder {
	return m.recorder
}

// isLifStats_Lifs mocks base method
func (m *MockisLifStats_Lifs) isLifStats_Lifs() {
	m.ctrl.Call(m, "isLifStats_Lifs")
}

// isLifStats_Lifs indicates an expected call of isLifStats_Lifs
func (mr *MockisLifStats_LifsMockRecorder) isLifStats_Lifs() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "isLifStats_Lifs", reflect.TypeOf((*MockisLifStats_Lifs)(nil).isLifStats_Lifs))
}

// MarshalTo mocks base method
func (m *MockisLifStats_Lifs) MarshalTo(arg0 []byte) (int, error) {
	ret := m.ctrl.Call(m, "MarshalTo", arg0)
	ret0, _ := ret[0].(int)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// MarshalTo indicates an expected call of MarshalTo
func (mr *MockisLifStats_LifsMockRecorder) MarshalTo(arg0 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "MarshalTo", reflect.TypeOf((*MockisLifStats_Lifs)(nil).MarshalTo), arg0)
}

// Size mocks base method
func (m *MockisLifStats_Lifs) Size() int {
	ret := m.ctrl.Call(m, "Size")
	ret0, _ := ret[0].(int)
	return ret0
}

// Size indicates an expected call of Size
func (mr *MockisLifStats_LifsMockRecorder) Size() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Size", reflect.TypeOf((*MockisLifStats_Lifs)(nil).Size))
}

// MockisIfEnicInfo_EnicTypeInfo is a mock of isIfEnicInfo_EnicTypeInfo interface
type MockisIfEnicInfo_EnicTypeInfo struct {
	ctrl     *gomock.Controller
	recorder *MockisIfEnicInfo_EnicTypeInfoMockRecorder
}

// MockisIfEnicInfo_EnicTypeInfoMockRecorder is the mock recorder for MockisIfEnicInfo_EnicTypeInfo
type MockisIfEnicInfo_EnicTypeInfoMockRecorder struct {
	mock *MockisIfEnicInfo_EnicTypeInfo
}

// NewMockisIfEnicInfo_EnicTypeInfo creates a new mock instance
func NewMockisIfEnicInfo_EnicTypeInfo(ctrl *gomock.Controller) *MockisIfEnicInfo_EnicTypeInfo {
	mock := &MockisIfEnicInfo_EnicTypeInfo{ctrl: ctrl}
	mock.recorder = &MockisIfEnicInfo_EnicTypeInfoMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use
func (m *MockisIfEnicInfo_EnicTypeInfo) EXPECT() *MockisIfEnicInfo_EnicTypeInfoMockRecorder {
	return m.recorder
}

// isIfEnicInfo_EnicTypeInfo mocks base method
func (m *MockisIfEnicInfo_EnicTypeInfo) isIfEnicInfo_EnicTypeInfo() {
	m.ctrl.Call(m, "isIfEnicInfo_EnicTypeInfo")
}

// isIfEnicInfo_EnicTypeInfo indicates an expected call of isIfEnicInfo_EnicTypeInfo
func (mr *MockisIfEnicInfo_EnicTypeInfoMockRecorder) isIfEnicInfo_EnicTypeInfo() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "isIfEnicInfo_EnicTypeInfo", reflect.TypeOf((*MockisIfEnicInfo_EnicTypeInfo)(nil).isIfEnicInfo_EnicTypeInfo))
}

// MarshalTo mocks base method
func (m *MockisIfEnicInfo_EnicTypeInfo) MarshalTo(arg0 []byte) (int, error) {
	ret := m.ctrl.Call(m, "MarshalTo", arg0)
	ret0, _ := ret[0].(int)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// MarshalTo indicates an expected call of MarshalTo
func (mr *MockisIfEnicInfo_EnicTypeInfoMockRecorder) MarshalTo(arg0 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "MarshalTo", reflect.TypeOf((*MockisIfEnicInfo_EnicTypeInfo)(nil).MarshalTo), arg0)
}

// Size mocks base method
func (m *MockisIfEnicInfo_EnicTypeInfo) Size() int {
	ret := m.ctrl.Call(m, "Size")
	ret0, _ := ret[0].(int)
	return ret0
}

// Size indicates an expected call of Size
func (mr *MockisIfEnicInfo_EnicTypeInfoMockRecorder) Size() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Size", reflect.TypeOf((*MockisIfEnicInfo_EnicTypeInfo)(nil).Size))
}

// MockisIfTunnelInfo_EncapInfo is a mock of isIfTunnelInfo_EncapInfo interface
type MockisIfTunnelInfo_EncapInfo struct {
	ctrl     *gomock.Controller
	recorder *MockisIfTunnelInfo_EncapInfoMockRecorder
}

// MockisIfTunnelInfo_EncapInfoMockRecorder is the mock recorder for MockisIfTunnelInfo_EncapInfo
type MockisIfTunnelInfo_EncapInfoMockRecorder struct {
	mock *MockisIfTunnelInfo_EncapInfo
}

// NewMockisIfTunnelInfo_EncapInfo creates a new mock instance
func NewMockisIfTunnelInfo_EncapInfo(ctrl *gomock.Controller) *MockisIfTunnelInfo_EncapInfo {
	mock := &MockisIfTunnelInfo_EncapInfo{ctrl: ctrl}
	mock.recorder = &MockisIfTunnelInfo_EncapInfoMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use
func (m *MockisIfTunnelInfo_EncapInfo) EXPECT() *MockisIfTunnelInfo_EncapInfoMockRecorder {
	return m.recorder
}

// isIfTunnelInfo_EncapInfo mocks base method
func (m *MockisIfTunnelInfo_EncapInfo) isIfTunnelInfo_EncapInfo() {
	m.ctrl.Call(m, "isIfTunnelInfo_EncapInfo")
}

// isIfTunnelInfo_EncapInfo indicates an expected call of isIfTunnelInfo_EncapInfo
func (mr *MockisIfTunnelInfo_EncapInfoMockRecorder) isIfTunnelInfo_EncapInfo() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "isIfTunnelInfo_EncapInfo", reflect.TypeOf((*MockisIfTunnelInfo_EncapInfo)(nil).isIfTunnelInfo_EncapInfo))
}

// MarshalTo mocks base method
func (m *MockisIfTunnelInfo_EncapInfo) MarshalTo(arg0 []byte) (int, error) {
	ret := m.ctrl.Call(m, "MarshalTo", arg0)
	ret0, _ := ret[0].(int)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// MarshalTo indicates an expected call of MarshalTo
func (mr *MockisIfTunnelInfo_EncapInfoMockRecorder) MarshalTo(arg0 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "MarshalTo", reflect.TypeOf((*MockisIfTunnelInfo_EncapInfo)(nil).MarshalTo), arg0)
}

// Size mocks base method
func (m *MockisIfTunnelInfo_EncapInfo) Size() int {
	ret := m.ctrl.Call(m, "Size")
	ret0, _ := ret[0].(int)
	return ret0
}

// Size indicates an expected call of Size
func (mr *MockisIfTunnelInfo_EncapInfoMockRecorder) Size() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Size", reflect.TypeOf((*MockisIfTunnelInfo_EncapInfo)(nil).Size))
}

// MockisInterfaceSpec_IfInfo is a mock of isInterfaceSpec_IfInfo interface
type MockisInterfaceSpec_IfInfo struct {
	ctrl     *gomock.Controller
	recorder *MockisInterfaceSpec_IfInfoMockRecorder
}

// MockisInterfaceSpec_IfInfoMockRecorder is the mock recorder for MockisInterfaceSpec_IfInfo
type MockisInterfaceSpec_IfInfoMockRecorder struct {
	mock *MockisInterfaceSpec_IfInfo
}

// NewMockisInterfaceSpec_IfInfo creates a new mock instance
func NewMockisInterfaceSpec_IfInfo(ctrl *gomock.Controller) *MockisInterfaceSpec_IfInfo {
	mock := &MockisInterfaceSpec_IfInfo{ctrl: ctrl}
	mock.recorder = &MockisInterfaceSpec_IfInfoMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use
func (m *MockisInterfaceSpec_IfInfo) EXPECT() *MockisInterfaceSpec_IfInfoMockRecorder {
	return m.recorder
}

// isInterfaceSpec_IfInfo mocks base method
func (m *MockisInterfaceSpec_IfInfo) isInterfaceSpec_IfInfo() {
	m.ctrl.Call(m, "isInterfaceSpec_IfInfo")
}

// isInterfaceSpec_IfInfo indicates an expected call of isInterfaceSpec_IfInfo
func (mr *MockisInterfaceSpec_IfInfoMockRecorder) isInterfaceSpec_IfInfo() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "isInterfaceSpec_IfInfo", reflect.TypeOf((*MockisInterfaceSpec_IfInfo)(nil).isInterfaceSpec_IfInfo))
}

// MarshalTo mocks base method
func (m *MockisInterfaceSpec_IfInfo) MarshalTo(arg0 []byte) (int, error) {
	ret := m.ctrl.Call(m, "MarshalTo", arg0)
	ret0, _ := ret[0].(int)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// MarshalTo indicates an expected call of MarshalTo
func (mr *MockisInterfaceSpec_IfInfoMockRecorder) MarshalTo(arg0 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "MarshalTo", reflect.TypeOf((*MockisInterfaceSpec_IfInfo)(nil).MarshalTo), arg0)
}

// Size mocks base method
func (m *MockisInterfaceSpec_IfInfo) Size() int {
	ret := m.ctrl.Call(m, "Size")
	ret0, _ := ret[0].(int)
	return ret0
}

// Size indicates an expected call of Size
func (mr *MockisInterfaceSpec_IfInfoMockRecorder) Size() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Size", reflect.TypeOf((*MockisInterfaceSpec_IfInfo)(nil).Size))
}

// MockisEnicResponseInfo_EnicTypeInfo is a mock of isEnicResponseInfo_EnicTypeInfo interface
type MockisEnicResponseInfo_EnicTypeInfo struct {
	ctrl     *gomock.Controller
	recorder *MockisEnicResponseInfo_EnicTypeInfoMockRecorder
}

// MockisEnicResponseInfo_EnicTypeInfoMockRecorder is the mock recorder for MockisEnicResponseInfo_EnicTypeInfo
type MockisEnicResponseInfo_EnicTypeInfoMockRecorder struct {
	mock *MockisEnicResponseInfo_EnicTypeInfo
}

// NewMockisEnicResponseInfo_EnicTypeInfo creates a new mock instance
func NewMockisEnicResponseInfo_EnicTypeInfo(ctrl *gomock.Controller) *MockisEnicResponseInfo_EnicTypeInfo {
	mock := &MockisEnicResponseInfo_EnicTypeInfo{ctrl: ctrl}
	mock.recorder = &MockisEnicResponseInfo_EnicTypeInfoMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use
func (m *MockisEnicResponseInfo_EnicTypeInfo) EXPECT() *MockisEnicResponseInfo_EnicTypeInfoMockRecorder {
	return m.recorder
}

// isEnicResponseInfo_EnicTypeInfo mocks base method
func (m *MockisEnicResponseInfo_EnicTypeInfo) isEnicResponseInfo_EnicTypeInfo() {
	m.ctrl.Call(m, "isEnicResponseInfo_EnicTypeInfo")
}

// isEnicResponseInfo_EnicTypeInfo indicates an expected call of isEnicResponseInfo_EnicTypeInfo
func (mr *MockisEnicResponseInfo_EnicTypeInfoMockRecorder) isEnicResponseInfo_EnicTypeInfo() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "isEnicResponseInfo_EnicTypeInfo", reflect.TypeOf((*MockisEnicResponseInfo_EnicTypeInfo)(nil).isEnicResponseInfo_EnicTypeInfo))
}

// MarshalTo mocks base method
func (m *MockisEnicResponseInfo_EnicTypeInfo) MarshalTo(arg0 []byte) (int, error) {
	ret := m.ctrl.Call(m, "MarshalTo", arg0)
	ret0, _ := ret[0].(int)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// MarshalTo indicates an expected call of MarshalTo
func (mr *MockisEnicResponseInfo_EnicTypeInfoMockRecorder) MarshalTo(arg0 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "MarshalTo", reflect.TypeOf((*MockisEnicResponseInfo_EnicTypeInfo)(nil).MarshalTo), arg0)
}

// Size mocks base method
func (m *MockisEnicResponseInfo_EnicTypeInfo) Size() int {
	ret := m.ctrl.Call(m, "Size")
	ret0, _ := ret[0].(int)
	return ret0
}

// Size indicates an expected call of Size
func (mr *MockisEnicResponseInfo_EnicTypeInfoMockRecorder) Size() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Size", reflect.TypeOf((*MockisEnicResponseInfo_EnicTypeInfo)(nil).Size))
}

// MockisInterfaceStatus_IfResponseInfo is a mock of isInterfaceStatus_IfResponseInfo interface
type MockisInterfaceStatus_IfResponseInfo struct {
	ctrl     *gomock.Controller
	recorder *MockisInterfaceStatus_IfResponseInfoMockRecorder
}

// MockisInterfaceStatus_IfResponseInfoMockRecorder is the mock recorder for MockisInterfaceStatus_IfResponseInfo
type MockisInterfaceStatus_IfResponseInfoMockRecorder struct {
	mock *MockisInterfaceStatus_IfResponseInfo
}

// NewMockisInterfaceStatus_IfResponseInfo creates a new mock instance
func NewMockisInterfaceStatus_IfResponseInfo(ctrl *gomock.Controller) *MockisInterfaceStatus_IfResponseInfo {
	mock := &MockisInterfaceStatus_IfResponseInfo{ctrl: ctrl}
	mock.recorder = &MockisInterfaceStatus_IfResponseInfoMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use
func (m *MockisInterfaceStatus_IfResponseInfo) EXPECT() *MockisInterfaceStatus_IfResponseInfoMockRecorder {
	return m.recorder
}

// isInterfaceStatus_IfResponseInfo mocks base method
func (m *MockisInterfaceStatus_IfResponseInfo) isInterfaceStatus_IfResponseInfo() {
	m.ctrl.Call(m, "isInterfaceStatus_IfResponseInfo")
}

// isInterfaceStatus_IfResponseInfo indicates an expected call of isInterfaceStatus_IfResponseInfo
func (mr *MockisInterfaceStatus_IfResponseInfoMockRecorder) isInterfaceStatus_IfResponseInfo() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "isInterfaceStatus_IfResponseInfo", reflect.TypeOf((*MockisInterfaceStatus_IfResponseInfo)(nil).isInterfaceStatus_IfResponseInfo))
}

// MarshalTo mocks base method
func (m *MockisInterfaceStatus_IfResponseInfo) MarshalTo(arg0 []byte) (int, error) {
	ret := m.ctrl.Call(m, "MarshalTo", arg0)
	ret0, _ := ret[0].(int)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// MarshalTo indicates an expected call of MarshalTo
func (mr *MockisInterfaceStatus_IfResponseInfoMockRecorder) MarshalTo(arg0 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "MarshalTo", reflect.TypeOf((*MockisInterfaceStatus_IfResponseInfo)(nil).MarshalTo), arg0)
}

// Size mocks base method
func (m *MockisInterfaceStatus_IfResponseInfo) Size() int {
	ret := m.ctrl.Call(m, "Size")
	ret0, _ := ret[0].(int)
	return ret0
}

// Size indicates an expected call of Size
func (mr *MockisInterfaceStatus_IfResponseInfoMockRecorder) Size() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Size", reflect.TypeOf((*MockisInterfaceStatus_IfResponseInfo)(nil).Size))
}

// MockisInterfaceStats_Ifstats is a mock of isInterfaceStats_Ifstats interface
type MockisInterfaceStats_Ifstats struct {
	ctrl     *gomock.Controller
	recorder *MockisInterfaceStats_IfstatsMockRecorder
}

// MockisInterfaceStats_IfstatsMockRecorder is the mock recorder for MockisInterfaceStats_Ifstats
type MockisInterfaceStats_IfstatsMockRecorder struct {
	mock *MockisInterfaceStats_Ifstats
}

// NewMockisInterfaceStats_Ifstats creates a new mock instance
func NewMockisInterfaceStats_Ifstats(ctrl *gomock.Controller) *MockisInterfaceStats_Ifstats {
	mock := &MockisInterfaceStats_Ifstats{ctrl: ctrl}
	mock.recorder = &MockisInterfaceStats_IfstatsMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use
func (m *MockisInterfaceStats_Ifstats) EXPECT() *MockisInterfaceStats_IfstatsMockRecorder {
	return m.recorder
}

// isInterfaceStats_Ifstats mocks base method
func (m *MockisInterfaceStats_Ifstats) isInterfaceStats_Ifstats() {
	m.ctrl.Call(m, "isInterfaceStats_Ifstats")
}

// isInterfaceStats_Ifstats indicates an expected call of isInterfaceStats_Ifstats
func (mr *MockisInterfaceStats_IfstatsMockRecorder) isInterfaceStats_Ifstats() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "isInterfaceStats_Ifstats", reflect.TypeOf((*MockisInterfaceStats_Ifstats)(nil).isInterfaceStats_Ifstats))
}

// MarshalTo mocks base method
func (m *MockisInterfaceStats_Ifstats) MarshalTo(arg0 []byte) (int, error) {
	ret := m.ctrl.Call(m, "MarshalTo", arg0)
	ret0, _ := ret[0].(int)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// MarshalTo indicates an expected call of MarshalTo
func (mr *MockisInterfaceStats_IfstatsMockRecorder) MarshalTo(arg0 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "MarshalTo", reflect.TypeOf((*MockisInterfaceStats_Ifstats)(nil).MarshalTo), arg0)
}

// Size mocks base method
func (m *MockisInterfaceStats_Ifstats) Size() int {
	ret := m.ctrl.Call(m, "Size")
	ret0, _ := ret[0].(int)
	return ret0
}

// Size indicates an expected call of Size
func (mr *MockisInterfaceStats_IfstatsMockRecorder) Size() *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Size", reflect.TypeOf((*MockisInterfaceStats_Ifstats)(nil).Size))
}

// MockInterfaceClient is a mock of InterfaceClient interface
type MockInterfaceClient struct {
	ctrl     *gomock.Controller
	recorder *MockInterfaceClientMockRecorder
}

// MockInterfaceClientMockRecorder is the mock recorder for MockInterfaceClient
type MockInterfaceClientMockRecorder struct {
	mock *MockInterfaceClient
}

// NewMockInterfaceClient creates a new mock instance
func NewMockInterfaceClient(ctrl *gomock.Controller) *MockInterfaceClient {
	mock := &MockInterfaceClient{ctrl: ctrl}
	mock.recorder = &MockInterfaceClientMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use
func (m *MockInterfaceClient) EXPECT() *MockInterfaceClientMockRecorder {
	return m.recorder
}

// LifCreate mocks base method
func (m *MockInterfaceClient) LifCreate(ctx context.Context, in *LifRequestMsg, opts ...grpc.CallOption) (*LifResponseMsg, error) {
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "LifCreate", varargs...)
	ret0, _ := ret[0].(*LifResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// LifCreate indicates an expected call of LifCreate
func (mr *MockInterfaceClientMockRecorder) LifCreate(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "LifCreate", reflect.TypeOf((*MockInterfaceClient)(nil).LifCreate), varargs...)
}

// LifUpdate mocks base method
func (m *MockInterfaceClient) LifUpdate(ctx context.Context, in *LifRequestMsg, opts ...grpc.CallOption) (*LifResponseMsg, error) {
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "LifUpdate", varargs...)
	ret0, _ := ret[0].(*LifResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// LifUpdate indicates an expected call of LifUpdate
func (mr *MockInterfaceClientMockRecorder) LifUpdate(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "LifUpdate", reflect.TypeOf((*MockInterfaceClient)(nil).LifUpdate), varargs...)
}

// LifDelete mocks base method
func (m *MockInterfaceClient) LifDelete(ctx context.Context, in *LifDeleteRequestMsg, opts ...grpc.CallOption) (*LifDeleteResponseMsg, error) {
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "LifDelete", varargs...)
	ret0, _ := ret[0].(*LifDeleteResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// LifDelete indicates an expected call of LifDelete
func (mr *MockInterfaceClientMockRecorder) LifDelete(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "LifDelete", reflect.TypeOf((*MockInterfaceClient)(nil).LifDelete), varargs...)
}

// LifGet mocks base method
func (m *MockInterfaceClient) LifGet(ctx context.Context, in *LifGetRequestMsg, opts ...grpc.CallOption) (*LifGetResponseMsg, error) {
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "LifGet", varargs...)
	ret0, _ := ret[0].(*LifGetResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// LifGet indicates an expected call of LifGet
func (mr *MockInterfaceClientMockRecorder) LifGet(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "LifGet", reflect.TypeOf((*MockInterfaceClient)(nil).LifGet), varargs...)
}

// LifGetQState mocks base method
func (m *MockInterfaceClient) LifGetQState(ctx context.Context, in *GetQStateRequestMsg, opts ...grpc.CallOption) (*GetQStateResponseMsg, error) {
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "LifGetQState", varargs...)
	ret0, _ := ret[0].(*GetQStateResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// LifGetQState indicates an expected call of LifGetQState
func (mr *MockInterfaceClientMockRecorder) LifGetQState(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "LifGetQState", reflect.TypeOf((*MockInterfaceClient)(nil).LifGetQState), varargs...)
}

// LifSetQState mocks base method
func (m *MockInterfaceClient) LifSetQState(ctx context.Context, in *SetQStateRequestMsg, opts ...grpc.CallOption) (*SetQStateResponseMsg, error) {
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "LifSetQState", varargs...)
	ret0, _ := ret[0].(*SetQStateResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// LifSetQState indicates an expected call of LifSetQState
func (mr *MockInterfaceClientMockRecorder) LifSetQState(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "LifSetQState", reflect.TypeOf((*MockInterfaceClient)(nil).LifSetQState), varargs...)
}

// InterfaceCreate mocks base method
func (m *MockInterfaceClient) InterfaceCreate(ctx context.Context, in *InterfaceRequestMsg, opts ...grpc.CallOption) (*InterfaceResponseMsg, error) {
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "InterfaceCreate", varargs...)
	ret0, _ := ret[0].(*InterfaceResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// InterfaceCreate indicates an expected call of InterfaceCreate
func (mr *MockInterfaceClientMockRecorder) InterfaceCreate(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "InterfaceCreate", reflect.TypeOf((*MockInterfaceClient)(nil).InterfaceCreate), varargs...)
}

// InterfaceUpdate mocks base method
func (m *MockInterfaceClient) InterfaceUpdate(ctx context.Context, in *InterfaceRequestMsg, opts ...grpc.CallOption) (*InterfaceResponseMsg, error) {
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "InterfaceUpdate", varargs...)
	ret0, _ := ret[0].(*InterfaceResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// InterfaceUpdate indicates an expected call of InterfaceUpdate
func (mr *MockInterfaceClientMockRecorder) InterfaceUpdate(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "InterfaceUpdate", reflect.TypeOf((*MockInterfaceClient)(nil).InterfaceUpdate), varargs...)
}

// InterfaceDelete mocks base method
func (m *MockInterfaceClient) InterfaceDelete(ctx context.Context, in *InterfaceDeleteRequestMsg, opts ...grpc.CallOption) (*InterfaceDeleteResponseMsg, error) {
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "InterfaceDelete", varargs...)
	ret0, _ := ret[0].(*InterfaceDeleteResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// InterfaceDelete indicates an expected call of InterfaceDelete
func (mr *MockInterfaceClientMockRecorder) InterfaceDelete(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "InterfaceDelete", reflect.TypeOf((*MockInterfaceClient)(nil).InterfaceDelete), varargs...)
}

// InterfaceGet mocks base method
func (m *MockInterfaceClient) InterfaceGet(ctx context.Context, in *InterfaceGetRequestMsg, opts ...grpc.CallOption) (*InterfaceGetResponseMsg, error) {
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "InterfaceGet", varargs...)
	ret0, _ := ret[0].(*InterfaceGetResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// InterfaceGet indicates an expected call of InterfaceGet
func (mr *MockInterfaceClientMockRecorder) InterfaceGet(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "InterfaceGet", reflect.TypeOf((*MockInterfaceClient)(nil).InterfaceGet), varargs...)
}

// AddL2SegmentOnUplink mocks base method
func (m *MockInterfaceClient) AddL2SegmentOnUplink(ctx context.Context, in *InterfaceL2SegmentRequestMsg, opts ...grpc.CallOption) (*InterfaceL2SegmentResponseMsg, error) {
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "AddL2SegmentOnUplink", varargs...)
	ret0, _ := ret[0].(*InterfaceL2SegmentResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// AddL2SegmentOnUplink indicates an expected call of AddL2SegmentOnUplink
func (mr *MockInterfaceClientMockRecorder) AddL2SegmentOnUplink(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AddL2SegmentOnUplink", reflect.TypeOf((*MockInterfaceClient)(nil).AddL2SegmentOnUplink), varargs...)
}

// DelL2SegmentOnUplink mocks base method
func (m *MockInterfaceClient) DelL2SegmentOnUplink(ctx context.Context, in *InterfaceL2SegmentRequestMsg, opts ...grpc.CallOption) (*InterfaceL2SegmentResponseMsg, error) {
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "DelL2SegmentOnUplink", varargs...)
	ret0, _ := ret[0].(*InterfaceL2SegmentResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// DelL2SegmentOnUplink indicates an expected call of DelL2SegmentOnUplink
func (mr *MockInterfaceClientMockRecorder) DelL2SegmentOnUplink(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "DelL2SegmentOnUplink", reflect.TypeOf((*MockInterfaceClient)(nil).DelL2SegmentOnUplink), varargs...)
}

// MockInterfaceServer is a mock of InterfaceServer interface
type MockInterfaceServer struct {
	ctrl     *gomock.Controller
	recorder *MockInterfaceServerMockRecorder
}

// MockInterfaceServerMockRecorder is the mock recorder for MockInterfaceServer
type MockInterfaceServerMockRecorder struct {
	mock *MockInterfaceServer
}

// NewMockInterfaceServer creates a new mock instance
func NewMockInterfaceServer(ctrl *gomock.Controller) *MockInterfaceServer {
	mock := &MockInterfaceServer{ctrl: ctrl}
	mock.recorder = &MockInterfaceServerMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use
func (m *MockInterfaceServer) EXPECT() *MockInterfaceServerMockRecorder {
	return m.recorder
}

// LifCreate mocks base method
func (m *MockInterfaceServer) LifCreate(arg0 context.Context, arg1 *LifRequestMsg) (*LifResponseMsg, error) {
	ret := m.ctrl.Call(m, "LifCreate", arg0, arg1)
	ret0, _ := ret[0].(*LifResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// LifCreate indicates an expected call of LifCreate
func (mr *MockInterfaceServerMockRecorder) LifCreate(arg0, arg1 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "LifCreate", reflect.TypeOf((*MockInterfaceServer)(nil).LifCreate), arg0, arg1)
}

// LifUpdate mocks base method
func (m *MockInterfaceServer) LifUpdate(arg0 context.Context, arg1 *LifRequestMsg) (*LifResponseMsg, error) {
	ret := m.ctrl.Call(m, "LifUpdate", arg0, arg1)
	ret0, _ := ret[0].(*LifResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// LifUpdate indicates an expected call of LifUpdate
func (mr *MockInterfaceServerMockRecorder) LifUpdate(arg0, arg1 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "LifUpdate", reflect.TypeOf((*MockInterfaceServer)(nil).LifUpdate), arg0, arg1)
}

// LifDelete mocks base method
func (m *MockInterfaceServer) LifDelete(arg0 context.Context, arg1 *LifDeleteRequestMsg) (*LifDeleteResponseMsg, error) {
	ret := m.ctrl.Call(m, "LifDelete", arg0, arg1)
	ret0, _ := ret[0].(*LifDeleteResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// LifDelete indicates an expected call of LifDelete
func (mr *MockInterfaceServerMockRecorder) LifDelete(arg0, arg1 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "LifDelete", reflect.TypeOf((*MockInterfaceServer)(nil).LifDelete), arg0, arg1)
}

// LifGet mocks base method
func (m *MockInterfaceServer) LifGet(arg0 context.Context, arg1 *LifGetRequestMsg) (*LifGetResponseMsg, error) {
	ret := m.ctrl.Call(m, "LifGet", arg0, arg1)
	ret0, _ := ret[0].(*LifGetResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// LifGet indicates an expected call of LifGet
func (mr *MockInterfaceServerMockRecorder) LifGet(arg0, arg1 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "LifGet", reflect.TypeOf((*MockInterfaceServer)(nil).LifGet), arg0, arg1)
}

// LifGetQState mocks base method
func (m *MockInterfaceServer) LifGetQState(arg0 context.Context, arg1 *GetQStateRequestMsg) (*GetQStateResponseMsg, error) {
	ret := m.ctrl.Call(m, "LifGetQState", arg0, arg1)
	ret0, _ := ret[0].(*GetQStateResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// LifGetQState indicates an expected call of LifGetQState
func (mr *MockInterfaceServerMockRecorder) LifGetQState(arg0, arg1 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "LifGetQState", reflect.TypeOf((*MockInterfaceServer)(nil).LifGetQState), arg0, arg1)
}

// LifSetQState mocks base method
func (m *MockInterfaceServer) LifSetQState(arg0 context.Context, arg1 *SetQStateRequestMsg) (*SetQStateResponseMsg, error) {
	ret := m.ctrl.Call(m, "LifSetQState", arg0, arg1)
	ret0, _ := ret[0].(*SetQStateResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// LifSetQState indicates an expected call of LifSetQState
func (mr *MockInterfaceServerMockRecorder) LifSetQState(arg0, arg1 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "LifSetQState", reflect.TypeOf((*MockInterfaceServer)(nil).LifSetQState), arg0, arg1)
}

// InterfaceCreate mocks base method
func (m *MockInterfaceServer) InterfaceCreate(arg0 context.Context, arg1 *InterfaceRequestMsg) (*InterfaceResponseMsg, error) {
	ret := m.ctrl.Call(m, "InterfaceCreate", arg0, arg1)
	ret0, _ := ret[0].(*InterfaceResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// InterfaceCreate indicates an expected call of InterfaceCreate
func (mr *MockInterfaceServerMockRecorder) InterfaceCreate(arg0, arg1 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "InterfaceCreate", reflect.TypeOf((*MockInterfaceServer)(nil).InterfaceCreate), arg0, arg1)
}

// InterfaceUpdate mocks base method
func (m *MockInterfaceServer) InterfaceUpdate(arg0 context.Context, arg1 *InterfaceRequestMsg) (*InterfaceResponseMsg, error) {
	ret := m.ctrl.Call(m, "InterfaceUpdate", arg0, arg1)
	ret0, _ := ret[0].(*InterfaceResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// InterfaceUpdate indicates an expected call of InterfaceUpdate
func (mr *MockInterfaceServerMockRecorder) InterfaceUpdate(arg0, arg1 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "InterfaceUpdate", reflect.TypeOf((*MockInterfaceServer)(nil).InterfaceUpdate), arg0, arg1)
}

// InterfaceDelete mocks base method
func (m *MockInterfaceServer) InterfaceDelete(arg0 context.Context, arg1 *InterfaceDeleteRequestMsg) (*InterfaceDeleteResponseMsg, error) {
	ret := m.ctrl.Call(m, "InterfaceDelete", arg0, arg1)
	ret0, _ := ret[0].(*InterfaceDeleteResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// InterfaceDelete indicates an expected call of InterfaceDelete
func (mr *MockInterfaceServerMockRecorder) InterfaceDelete(arg0, arg1 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "InterfaceDelete", reflect.TypeOf((*MockInterfaceServer)(nil).InterfaceDelete), arg0, arg1)
}

// InterfaceGet mocks base method
func (m *MockInterfaceServer) InterfaceGet(arg0 context.Context, arg1 *InterfaceGetRequestMsg) (*InterfaceGetResponseMsg, error) {
	ret := m.ctrl.Call(m, "InterfaceGet", arg0, arg1)
	ret0, _ := ret[0].(*InterfaceGetResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// InterfaceGet indicates an expected call of InterfaceGet
func (mr *MockInterfaceServerMockRecorder) InterfaceGet(arg0, arg1 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "InterfaceGet", reflect.TypeOf((*MockInterfaceServer)(nil).InterfaceGet), arg0, arg1)
}

// AddL2SegmentOnUplink mocks base method
func (m *MockInterfaceServer) AddL2SegmentOnUplink(arg0 context.Context, arg1 *InterfaceL2SegmentRequestMsg) (*InterfaceL2SegmentResponseMsg, error) {
	ret := m.ctrl.Call(m, "AddL2SegmentOnUplink", arg0, arg1)
	ret0, _ := ret[0].(*InterfaceL2SegmentResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// AddL2SegmentOnUplink indicates an expected call of AddL2SegmentOnUplink
func (mr *MockInterfaceServerMockRecorder) AddL2SegmentOnUplink(arg0, arg1 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AddL2SegmentOnUplink", reflect.TypeOf((*MockInterfaceServer)(nil).AddL2SegmentOnUplink), arg0, arg1)
}

// DelL2SegmentOnUplink mocks base method
func (m *MockInterfaceServer) DelL2SegmentOnUplink(arg0 context.Context, arg1 *InterfaceL2SegmentRequestMsg) (*InterfaceL2SegmentResponseMsg, error) {
	ret := m.ctrl.Call(m, "DelL2SegmentOnUplink", arg0, arg1)
	ret0, _ := ret[0].(*InterfaceL2SegmentResponseMsg)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// DelL2SegmentOnUplink indicates an expected call of DelL2SegmentOnUplink
func (mr *MockInterfaceServerMockRecorder) DelL2SegmentOnUplink(arg0, arg1 interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "DelL2SegmentOnUplink", reflect.TypeOf((*MockInterfaceServer)(nil).DelL2SegmentOnUplink), arg0, arg1)
}
