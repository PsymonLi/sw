// Code generated by MockGen. DO NOT EDIT.
// Source: ../objstore.go

// Package mock is a generated GoMock package.
package mock

import (
	context "context"
	io "io"
	reflect "reflect"

	gomock "github.com/golang/mock/gomock"

	client "github.com/pensando/sw/venice/utils/objstore/minio/client"
)

// MockobjStoreBackend is a mock of objStoreBackend interface
type MockobjStoreBackend struct {
	ctrl     *gomock.Controller
	recorder *MockobjStoreBackendMockRecorder
}

// MockobjStoreBackendMockRecorder is the mock recorder for MockobjStoreBackend
type MockobjStoreBackendMockRecorder struct {
	mock *MockobjStoreBackend
}

// NewMockobjStoreBackend creates a new mock instance
func NewMockobjStoreBackend(ctrl *gomock.Controller) *MockobjStoreBackend {
	mock := &MockobjStoreBackend{ctrl: ctrl}
	mock.recorder = &MockobjStoreBackendMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use
func (m *MockobjStoreBackend) EXPECT() *MockobjStoreBackendMockRecorder {
	return m.recorder
}

// PutObject mocks base method
func (m *MockobjStoreBackend) PutObject(ctx context.Context, objectName string, reader io.Reader, userMeta map[string]string) (int64, error) {
	ret := m.ctrl.Call(m, "PutObject", ctx, objectName, reader, userMeta)
	ret0, _ := ret[0].(int64)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// PutObject indicates an expected call of PutObject
func (mr *MockobjStoreBackendMockRecorder) PutObject(ctx, objectName, reader, userMeta interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "PutObject", reflect.TypeOf((*MockobjStoreBackend)(nil).PutObject), ctx, objectName, reader, userMeta)
}

// PutObjectOfSize mocks base method
func (m *MockobjStoreBackend) PutObjectOfSize(ctx context.Context, objectName string, reader io.Reader, size int64, userMeta map[string]string) (int64, error) {
	ret := m.ctrl.Call(m, "PutObjectOfSize", ctx, objectName, reader, size, userMeta)
	ret0, _ := ret[0].(int64)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// PutObjectOfSize indicates an expected call of PutObjectOfSize
func (mr *MockobjStoreBackendMockRecorder) PutObjectOfSize(ctx, objectName, reader, size, userMeta interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "PutObjectOfSize", reflect.TypeOf((*MockobjStoreBackend)(nil).PutObjectOfSize), ctx, objectName, reader, size, userMeta)
}

// PutObjectExplicit mocks base method
func (m *MockobjStoreBackend) PutObjectExplicit(ctx context.Context, serviceName, objectName string, reader io.Reader, size int64, metaData map[string]string) (int64, error) {
	ret := m.ctrl.Call(m, "PutObjectExplicit", ctx, serviceName, objectName, reader, size, metaData)
	ret0, _ := ret[0].(int64)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// PutObjectExplicit indicates an expected call of PutObjectExplicit
func (mr *MockobjStoreBackendMockRecorder) PutObjectExplicit(ctx, serviceName, objectName, reader, size, metaData interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "PutObjectExplicit", reflect.TypeOf((*MockobjStoreBackend)(nil).PutObjectExplicit), ctx, serviceName, objectName, reader, size, metaData)
}

// GetObject mocks base method
func (m *MockobjStoreBackend) GetObject(ctx context.Context, objectName string) (io.ReadCloser, error) {
	ret := m.ctrl.Call(m, "GetObject", ctx, objectName)
	ret0, _ := ret[0].(io.ReadCloser)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetObject indicates an expected call of GetObject
func (mr *MockobjStoreBackendMockRecorder) GetObject(ctx, objectName interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetObject", reflect.TypeOf((*MockobjStoreBackend)(nil).GetObject), ctx, objectName)
}

// StatObject mocks base method
func (m *MockobjStoreBackend) StatObject(objectName string) (*client.ObjectStats, error) {
	ret := m.ctrl.Call(m, "StatObject", objectName)
	ret0, _ := ret[0].(*client.ObjectStats)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// StatObject indicates an expected call of StatObject
func (mr *MockobjStoreBackendMockRecorder) StatObject(objectName interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "StatObject", reflect.TypeOf((*MockobjStoreBackend)(nil).StatObject), objectName)
}

// ListObjects mocks base method
func (m *MockobjStoreBackend) ListObjects(prefix string) ([]string, error) {
	ret := m.ctrl.Call(m, "ListObjects", prefix)
	ret0, _ := ret[0].([]string)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// ListObjects indicates an expected call of ListObjects
func (mr *MockobjStoreBackendMockRecorder) ListObjects(prefix interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "ListObjects", reflect.TypeOf((*MockobjStoreBackend)(nil).ListObjects), prefix)
}

// RemoveObjects mocks base method
func (m *MockobjStoreBackend) RemoveObjects(prefix string) error {
	ret := m.ctrl.Call(m, "RemoveObjects", prefix)
	ret0, _ := ret[0].(error)
	return ret0
}

// RemoveObjects indicates an expected call of RemoveObjects
func (mr *MockobjStoreBackendMockRecorder) RemoveObjects(prefix interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "RemoveObjects", reflect.TypeOf((*MockobjStoreBackend)(nil).RemoveObjects), prefix)
}

// RemoveObject mocks base method
func (m *MockobjStoreBackend) RemoveObject(path string) error {
	ret := m.ctrl.Call(m, "RemoveObject", path)
	ret0, _ := ret[0].(error)
	return ret0
}

// RemoveObject indicates an expected call of RemoveObject
func (mr *MockobjStoreBackendMockRecorder) RemoveObject(path interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "RemoveObject", reflect.TypeOf((*MockobjStoreBackend)(nil).RemoveObject), path)
}
