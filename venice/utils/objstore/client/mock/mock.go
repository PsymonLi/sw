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

// ListBuckets mocks base method
func (m *MockobjStoreBackend) ListBuckets(ctx context.Context) ([]string, error) {
	ret := m.ctrl.Call(m, "ListBuckets", ctx)
	ret0, _ := ret[0].([]string)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// ListBuckets indicates an expected call of ListBuckets
func (mr *MockobjStoreBackendMockRecorder) ListBuckets(ctx interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "ListBuckets", reflect.TypeOf((*MockobjStoreBackend)(nil).ListBuckets), ctx)
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

// PutObjectRateLimiter mocks base method
func (m *MockobjStoreBackend) PutObjectRateLimiter(ctx context.Context, objectName string, reader io.Reader, userMeta map[string]string, size uint64) (int64, error) {
	ret := m.ctrl.Call(m, "PutObjectRateLimiter", ctx, objectName, reader, userMeta, size)
	ret0, _ := ret[0].(int64)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// PutObjectRateLimiter indicates an expected call of PutObjectRateLimiter
func (mr *MockobjStoreBackendMockRecorder) PutObjectRateLimiter(ctx, objectName, reader, userMeta, size interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "PutObjectRateLimiter", reflect.TypeOf((*MockobjStoreBackend)(nil).PutObjectRateLimiter), ctx, objectName, reader, userMeta, size)
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
func (m *MockobjStoreBackend) PutObjectExplicit(ctx context.Context, serviceName, objectName string, reader io.Reader, size int64, metaData map[string]string, contentType string) (int64, error) {
	ret := m.ctrl.Call(m, "PutObjectExplicit", ctx, serviceName, objectName, reader, size, metaData, contentType)
	ret0, _ := ret[0].(int64)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// PutObjectExplicit indicates an expected call of PutObjectExplicit
func (mr *MockobjStoreBackendMockRecorder) PutObjectExplicit(ctx, serviceName, objectName, reader, size, metaData, contentType interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "PutObjectExplicit", reflect.TypeOf((*MockobjStoreBackend)(nil).PutObjectExplicit), ctx, serviceName, objectName, reader, size, metaData, contentType)
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

// GetObjectExplicit mocks base method
func (m *MockobjStoreBackend) GetObjectExplicit(ctx context.Context, serviceName, objectName string) (io.ReadCloser, error) {
	ret := m.ctrl.Call(m, "GetObjectExplicit", ctx, serviceName, objectName)
	ret0, _ := ret[0].(io.ReadCloser)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetObjectExplicit indicates an expected call of GetObjectExplicit
func (mr *MockobjStoreBackendMockRecorder) GetObjectExplicit(ctx, serviceName, objectName interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetObjectExplicit", reflect.TypeOf((*MockobjStoreBackend)(nil).GetObjectExplicit), ctx, serviceName, objectName)
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

// ListObjectsExplicit mocks base method
func (m *MockobjStoreBackend) ListObjectsExplicit(serviceName, prefix string, recursive bool) ([]string, error) {
	ret := m.ctrl.Call(m, "ListObjectsExplicit", serviceName, prefix, recursive)
	ret0, _ := ret[0].([]string)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// ListObjectsExplicit indicates an expected call of ListObjectsExplicit
func (mr *MockobjStoreBackendMockRecorder) ListObjectsExplicit(serviceName, prefix, recursive interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "ListObjectsExplicit", reflect.TypeOf((*MockobjStoreBackend)(nil).ListObjectsExplicit), serviceName, prefix, recursive)
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

// RemoveObjectsWithContext mocks base method
func (m *MockobjStoreBackend) RemoveObjectsWithContext(ctx context.Context, bucketName string, objectsCh <-chan string) <-chan interface{} {
	ret := m.ctrl.Call(m, "RemoveObjectsWithContext", ctx, bucketName, objectsCh)
	ret0, _ := ret[0].(<-chan interface{})
	return ret0
}

// RemoveObjectsWithContext indicates an expected call of RemoveObjectsWithContext
func (mr *MockobjStoreBackendMockRecorder) RemoveObjectsWithContext(ctx, bucketName, objectsCh interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "RemoveObjectsWithContext", reflect.TypeOf((*MockobjStoreBackend)(nil).RemoveObjectsWithContext), ctx, bucketName, objectsCh)
}

// SetServiceLifecycleWithContext mocks base method
func (m *MockobjStoreBackend) SetServiceLifecycleWithContext(ctx context.Context, serviceName string, enabled bool, prefix string, days int) error {
	ret := m.ctrl.Call(m, "SetServiceLifecycleWithContext", ctx, serviceName, enabled, prefix, days)
	ret0, _ := ret[0].(error)
	return ret0
}

// SetServiceLifecycleWithContext indicates an expected call of SetServiceLifecycleWithContext
func (mr *MockobjStoreBackendMockRecorder) SetServiceLifecycleWithContext(ctx, serviceName, enabled, prefix, days interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "SetServiceLifecycleWithContext", reflect.TypeOf((*MockobjStoreBackend)(nil).SetServiceLifecycleWithContext), ctx, serviceName, enabled, prefix, days)
}

// SelectObjectContentExplicit mocks base method
func (m *MockobjStoreBackend) SelectObjectContentExplicit(ctx context.Context, serviceName, objectName, sqlExpression string, inputSerializationType client.InputSerializationType, outputSerializationType client.OutputSerializationType) (io.ReadCloser, error) {
	ret := m.ctrl.Call(m, "SelectObjectContentExplicit", ctx, serviceName, objectName, sqlExpression, inputSerializationType, outputSerializationType)
	ret0, _ := ret[0].(io.ReadCloser)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// SelectObjectContentExplicit indicates an expected call of SelectObjectContentExplicit
func (mr *MockobjStoreBackendMockRecorder) SelectObjectContentExplicit(ctx, serviceName, objectName, sqlExpression, inputSerializationType, outputSerializationType interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "SelectObjectContentExplicit", reflect.TypeOf((*MockobjStoreBackend)(nil).SelectObjectContentExplicit), ctx, serviceName, objectName, sqlExpression, inputSerializationType, outputSerializationType)
}

// FPutObjectExplicit mocks base method
func (m *MockobjStoreBackend) FPutObjectExplicit(ctx context.Context, serviceName, objectName, filePath string, metaData map[string]string, contentType string) (int64, error) {
	ret := m.ctrl.Call(m, "FPutObjectExplicit", ctx, serviceName, objectName, filePath, metaData, contentType)
	ret0, _ := ret[0].(int64)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// FPutObjectExplicit indicates an expected call of FPutObjectExplicit
func (mr *MockobjStoreBackendMockRecorder) FPutObjectExplicit(ctx, serviceName, objectName, filePath, metaData, contentType interface{}) *gomock.Call {
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "FPutObjectExplicit", reflect.TypeOf((*MockobjStoreBackend)(nil).FPutObjectExplicit), ctx, serviceName, objectName, filePath, metaData, contentType)
}
