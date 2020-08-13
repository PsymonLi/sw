package minio

import (
	"errors"
	"strings"
	"testing"
	"time"

	"github.com/coreos/etcd/pkg/testutil"
	"github.com/gogo/protobuf/types"
	"github.com/golang/mock/gomock"
	uuid "github.com/satori/go.uuid"

	"github.com/pensando/sw/api/mock"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/testutils"
)

func TestCredentialsManagerConstructor(t *testing.T) {
	mockCtrl := gomock.NewController(t)
	defer mockCtrl.Finish()
	mockAPIClient := mock.NewMockClusterV1Interface(mockCtrl)

	credsManager := NewAPIServerBasedCredsManager(mockAPIClient, WithAPIRetries(2, 2*time.Second))
	testutils.Assert(t, credsManager != nil, "Constructor should not return a nil")
	testutils.AssertEquals(t, 2, credsManager.retryCount, "retries must match provided count")
	testutils.AssertEquals(t, time.Second*2, credsManager.retryInterval, "retry interval must match provided value")
}

func TestAPIServerBasedCredsManager_GetCredentials(t *testing.T) {
	mockCtrl := gomock.NewController(t)
	defer mockCtrl.Finish()

	expectedObjMeta := &api.ObjectMeta{
		Name: globals.MinioCredentialsObjectName,
	}
	//happy case
	mockAPIClient, mockCredentialInterface := setupMocks(mockCtrl, 1)
	expectedCredsObj := getValidObjectStoreCreds()
	mockCredentialInterface.EXPECT().Get(gomock.Any(), gomock.Eq(expectedObjMeta)).Return(expectedCredsObj, nil).Times(1)
	toTest := &APIServerBasedCredsManager{apiClient: mockAPIClient, retryCount: 1, retryInterval: time.Millisecond}
	expectedMinioCreds := &Credentials{
		AccessKey: getValue(expectedCredsObj, globals.MinioAccessKeyName),
		SecretKey: getValue(expectedCredsObj, globals.MinioSecretKeyName),
	}
	actualMinioCreds, err := toTest.GetCredentials()
	testutils.AssertOk(t, err, "Unexpected error when fetching credentials")
	testutils.AssertEquals(t, expectedMinioCreds, actualMinioCreds, "credentials manager returned unexpected credentials")

	//happy case with retries
	mockAPIClient, mockCredentialInterface = setupMocks(mockCtrl, 2)
	expectedCredsObj = getValidObjectStoreCreds()
	mockCredentialInterface.EXPECT().Get(gomock.Any(), gomock.Eq(expectedObjMeta)).Return(nil, errors.New("test error message")).Times(1)
	mockCredentialInterface.EXPECT().Get(gomock.Any(), gomock.Eq(expectedObjMeta)).Return(expectedCredsObj, nil).Times(1)
	toTest = &APIServerBasedCredsManager{apiClient: mockAPIClient, retryCount: 2, retryInterval: time.Millisecond}
	expectedMinioCreds = &Credentials{
		AccessKey: getValue(expectedCredsObj, globals.MinioAccessKeyName),
		SecretKey: getValue(expectedCredsObj, globals.MinioSecretKeyName),
	}
	actualMinioCreds, err = toTest.GetCredentials()
	testutils.AssertOk(t, err, "Unexpected error when fetching credentials")
	testutils.AssertEquals(t, expectedMinioCreds, actualMinioCreds, "credentials manager returned unexpected credentials")

	//invalid credentials object is found
	mockAPIClient, mockCredentialInterface = setupMocks(mockCtrl, 1)
	expectedCredsObj = getObjectStoreCredsWithoutAccessKey()
	mockCredentialInterface.EXPECT().Get(gomock.Any(), gomock.Eq(expectedObjMeta)).Return(expectedCredsObj, nil).Times(1)
	toTest = &APIServerBasedCredsManager{apiClient: mockAPIClient, retryCount: 1, retryInterval: time.Millisecond}
	actualMinioCreds, err = toTest.GetCredentials()
	testutil.AssertNil(t, actualMinioCreds)
	testutils.AssertError(t, err, "credential manager expected to return error")

	// apiclient returns error
	mockAPIClient, mockCredentialInterface = setupMocks(mockCtrl, 1)
	testErrorMsg := "test error message"
	mockCredentialInterface.EXPECT().Get(gomock.Any(), gomock.Eq(expectedObjMeta)).Return(nil, errors.New(testErrorMsg)).Times(1)
	toTest = &APIServerBasedCredsManager{apiClient: mockAPIClient, retryCount: 1, retryInterval: time.Millisecond}
	actualMinioCreds, err = toTest.GetCredentials()
	testutil.AssertNil(t, actualMinioCreds)
	testutils.AssertError(t, err, "credential manager expected to return error")
	testutils.Assert(t, strings.Contains(err.Error(), testErrorMsg), "GetCredentials failed with unexpected error")

	// apiclient returns error with retries
	mockAPIClient, mockCredentialInterface = setupMocks(mockCtrl, 3)
	mockCredentialInterface.EXPECT().Get(gomock.Any(), gomock.Eq(expectedObjMeta)).Return(nil, errors.New(testErrorMsg)).Times(3)
	toTest = &APIServerBasedCredsManager{apiClient: mockAPIClient, retryCount: 3, retryInterval: time.Millisecond}
	actualMinioCreds, err = toTest.GetCredentials()
	testutil.AssertNil(t, actualMinioCreds)
	testutils.AssertError(t, err, "credential manager expected to return error")
	testutils.Assert(t, strings.Contains(err.Error(), testErrorMsg), "GetCredentials failed with unexpected error")
}

func TestApiServerBasedCredsManager_CreateCredentials(t *testing.T) {
	mockCtrl := gomock.NewController(t)
	defer mockCtrl.Finish()

	expectedObjMeta := &api.ObjectMeta{
		Name: globals.MinioCredentialsObjectName,
	}
	//happy case
	expectedCredsObj := getValidObjectStoreCreds()
	expectedMinioCreds := &Credentials{
		AccessKey: getValue(expectedCredsObj, globals.MinioAccessKeyName),
		SecretKey: getValue(expectedCredsObj, globals.MinioSecretKeyName),
	}
	mockAPIClient, mockCredentialInterface := setupMocks(mockCtrl, 1)
	mockCredentialInterface.EXPECT().Create(gomock.Any(), gomock.Any()).Return(expectedCredsObj, nil).Times(1)
	mockCredentialInterface.EXPECT().Get(gomock.Any(), gomock.Eq(expectedObjMeta)).Return(expectedCredsObj, nil).Times(0)
	toTest := &APIServerBasedCredsManager{apiClient: mockAPIClient, retryCount: 1, retryInterval: time.Millisecond}

	actualMinioCreds, err := toTest.CreateCredentials()
	testutils.AssertOk(t, err, "Unexpected error when creating credentials")
	testutils.AssertEquals(t, expectedMinioCreds, actualMinioCreds, "credentials create op returned unexpected credentials")

	//happy case with retries
	mockAPIClient, mockCredentialInterface = setupMocks(mockCtrl, 2)
	mockCredentialInterface.EXPECT().Create(gomock.Any(), gomock.Any()).Return(nil, errors.New("test error message")).Times(1)
	mockCredentialInterface.EXPECT().Create(gomock.Any(), gomock.Any()).Return(expectedCredsObj, nil).Times(1)
	toTest = &APIServerBasedCredsManager{apiClient: mockAPIClient, retryCount: 2, retryInterval: time.Millisecond}

	actualMinioCreds, err = toTest.CreateCredentials()
	testutils.AssertOk(t, err, "Unexpected error when creating credentials")
	testutils.AssertEquals(t, expectedMinioCreds, actualMinioCreds, "credentials manager returned unexpected credentials")

	//credentials already exist error is not retried
	mockAPIClient, mockCredentialInterface = setupMocks(mockCtrl, 2)
	mockCredentialInterface.EXPECT().Create(gomock.Any(), gomock.Any()).Return(nil, errors.New("AlreadyExists")).Times(1)
	mockCredentialInterface.EXPECT().Get(gomock.Any(), gomock.Eq(expectedObjMeta)).Return(expectedCredsObj, nil).Times(1)
	toTest = &APIServerBasedCredsManager{apiClient: mockAPIClient, retryCount: 2, retryInterval: time.Millisecond}
	actualMinioCreds, err = toTest.CreateCredentials()
	testutil.AssertNotNil(t, actualMinioCreds)
	testutils.AssertEquals(t, expectedMinioCreds, actualMinioCreds, "Create op returns already existing credentials")
	testutils.AssertError(t, err, "credential creation expected to return error")
	testutils.Assert(t, strings.Contains(err.Error(), "AlreadyExists"), "CreateCredentials failed with unexpected error")

	//case with retries ending with already exists error
	mockAPIClient, mockCredentialInterface = setupMocks(mockCtrl, 3)
	mockCredentialInterface.EXPECT().Create(gomock.Any(), gomock.Any()).Return(nil, errors.New("test error message")).Times(1)
	mockCredentialInterface.EXPECT().Create(gomock.Any(), gomock.Any()).Return(expectedCredsObj, errors.New("AlreadyExists")).Times(1)
	mockCredentialInterface.EXPECT().Get(gomock.Any(), gomock.Eq(expectedObjMeta)).Return(expectedCredsObj, nil).Times(1)
	toTest = &APIServerBasedCredsManager{apiClient: mockAPIClient, retryCount: 2, retryInterval: time.Millisecond}

	actualMinioCreds, err = toTest.CreateCredentials()
	testutil.AssertNotNil(t, actualMinioCreds)
	testutils.AssertEquals(t, expectedMinioCreds, actualMinioCreds, "Create op returns already existing credentials")
	testutils.AssertError(t, err, "credential creation expected to return error")
	testutils.Assert(t, strings.Contains(err.Error(), "AlreadyExists"), "CreateCredentials failed with unexpected error")

	// apiclient returns error
	mockAPIClient, mockCredentialInterface = setupMocks(mockCtrl, 1)
	testErrorMsg := "test error message"
	mockCredentialInterface.EXPECT().Create(gomock.Any(), gomock.Any()).Return(nil, errors.New(testErrorMsg)).Times(1)
	toTest = &APIServerBasedCredsManager{apiClient: mockAPIClient, retryCount: 1, retryInterval: time.Millisecond}
	actualMinioCreds, err = toTest.CreateCredentials()
	testutil.AssertNil(t, actualMinioCreds)
	testutils.AssertError(t, err, "credential creation expected to return error")
	testutils.Assert(t, strings.Contains(err.Error(), testErrorMsg), "CreateCredentials failed with unexpected error")

	// apiclient returns error with retries
	mockAPIClient, mockCredentialInterface = setupMocks(mockCtrl, 3)
	mockCredentialInterface.EXPECT().Create(gomock.Any(), gomock.Any()).Return(nil, errors.New(testErrorMsg)).Times(3)
	toTest = &APIServerBasedCredsManager{apiClient: mockAPIClient, retryCount: 3, retryInterval: time.Millisecond}
	actualMinioCreds, err = toTest.CreateCredentials()
	testutil.AssertNil(t, actualMinioCreds)
	testutils.AssertError(t, err, "credential creation expected to return error")
	testutils.Assert(t, strings.Contains(err.Error(), testErrorMsg), "CreateCredentials failed with unexpected error")
}

func setupMocks(mockCtrl *gomock.Controller, expectedInvocationCount int) (*mock.MockClusterV1Interface, *mock.MockClusterV1CredentialsInterface) {
	mockCredsInterface := mock.NewMockClusterV1CredentialsInterface(mockCtrl)
	mockAPIClient := mock.NewMockClusterV1Interface(mockCtrl)
	mockAPIClient.EXPECT().Credentials().Return(mockCredsInterface).Times(expectedInvocationCount)
	return mockAPIClient, mockCredsInterface
}

func TestValidateObjectStoreCredentials(t *testing.T) {
	creds := getValidObjectStoreCreds()
	testutils.AssertOk(t, ValidateObjectStoreCredentials(creds), "validator should not return an error")

	creds.Spec.KeyValuePairs = append(creds.Spec.KeyValuePairs, cluster.KeyValue{
		Key:   "randomKey",
		Value: []byte("randomValue"),
	})
	testutils.AssertError(t, ValidateObjectStoreCredentials(creds), "validator should return error when unexpected key is found")

	invalidCreds := getObjectStoreCredsWithoutAccessKey()
	testutils.AssertError(t, ValidateObjectStoreCredentials(invalidCreds), "validator should return error when access key is not present")

	invalidCreds = getObjectStoreCredsWithoutSecretKey()
	testutils.AssertError(t, ValidateObjectStoreCredentials(invalidCreds), "validator should return error when secret key is not present")

	invalidCreds = getObjectStoreCredsWithImproperOldKeys()
	testutils.AssertError(t, ValidateObjectStoreCredentials(invalidCreds), "validator should return error when old secret key is not present")

	testutils.AssertError(t, ValidateObjectStoreCredentials(nil), "validator should return error when credentials obj is nil")
}

func TestObjectStoreCredentialCreation(t *testing.T) {
	credentials1, err := GenerateObjectStoreCredentials()
	testutils.AssertOk(t, err, "unable to create credentials")
	testutils.AssertOk(t, ValidateObjectStoreCredentials(credentials1), "invalid credentials created")

	credentials2, err := GenerateObjectStoreCredentials()
	testutils.AssertOk(t, err, "unable to create credentials")

	minioKeys1, err := GetMinioKeys(credentials1)
	testutils.AssertOk(t, err, "minio key extraction should have succeeded")
	minioKeys2, err := GetMinioKeys(credentials2)
	testutils.AssertOk(t, err, "minio key extraction should have succeeded")

	testutils.Assert(t, minioKeys1 != minioKeys2, "Minio keys should be randomly generated, they aren't expected to be equal")
}

func getValue(credentials *cluster.Credentials, key string) string {
	for _, kv := range credentials.Spec.KeyValuePairs {
		if kv.Key == key {
			return string(kv.Value)
		}
	}
	return ""
}

func getValidObjectStoreCreds() *cluster.Credentials {
	credentials := &cluster.Credentials{}
	c, _ := types.TimestampProto(time.Now())
	credentials.Defaults("all")
	credentials.APIVersion = "v1"
	credentials.ObjectMeta = api.ObjectMeta{
		Name:         globals.MinioCredentialsObjectName,
		GenerationID: "1",
		UUID:         uuid.NewV4().String(),
		CreationTime: api.Timestamp{
			Timestamp: *c,
		},
		ModTime: api.Timestamp{
			Timestamp: *c,
		},
	}
	credentials.SelfLink = credentials.MakeKey("cluster")
	minioAccessKey := uuid.NewV4().String()
	minioSecretKey := uuid.NewV4().String()
	credentials.Spec = cluster.CredentialsSpec{
		KeyValuePairs: []cluster.KeyValue{
			{
				Key:   globals.MinioAccessKeyName,
				Value: []byte(minioAccessKey),
			},
			{
				Key:   globals.MinioSecretKeyName,
				Value: []byte(minioSecretKey),
			},
		},
	}
	return credentials
}

func getObjectStoreCredsWithImproperOldKeys() *cluster.Credentials {
	credentials := &cluster.Credentials{}
	c, _ := types.TimestampProto(time.Now())
	credentials.Defaults("all")
	credentials.APIVersion = "v1"
	credentials.ObjectMeta = api.ObjectMeta{
		Name:         globals.MinioCredentialsObjectName,
		GenerationID: "1",
		UUID:         uuid.NewV4().String(),
		CreationTime: api.Timestamp{
			Timestamp: *c,
		},
		ModTime: api.Timestamp{
			Timestamp: *c,
		},
	}
	credentials.SelfLink = credentials.MakeKey("cluster")
	minioAccessKey := uuid.NewV4().String()
	minioSecretKey := uuid.NewV4().String()
	credentials.Spec = cluster.CredentialsSpec{
		KeyValuePairs: []cluster.KeyValue{
			{
				Key:   globals.MinioAccessKeyName,
				Value: []byte(minioAccessKey),
			},
			{
				Key:   globals.MinioSecretKeyName,
				Value: []byte(minioSecretKey),
			},
			{
				Key:   globals.MinioOldAccessKeyName,
				Value: []byte("miniokey"),
			},
			// no old-secret key specified
		},
	}
	return credentials
}

func getObjectStoreCredsWithoutAccessKey() *cluster.Credentials {
	credentials := &cluster.Credentials{}
	c, _ := types.TimestampProto(time.Now())
	credentials.Defaults("all")
	credentials.APIVersion = "v1"
	credentials.ObjectMeta = api.ObjectMeta{
		Name:         globals.MinioCredentialsObjectName,
		GenerationID: "1",
		UUID:         uuid.NewV4().String(),
		CreationTime: api.Timestamp{
			Timestamp: *c,
		},
		ModTime: api.Timestamp{
			Timestamp: *c,
		},
	}
	credentials.SelfLink = credentials.MakeKey("cluster")
	minioSecretKey := uuid.NewV4().String()
	credentials.Spec = cluster.CredentialsSpec{
		KeyValuePairs: []cluster.KeyValue{
			{
				Key:   globals.MinioSecretKeyName,
				Value: []byte(minioSecretKey),
			},
		},
	}
	return credentials
}

func getObjectStoreCredsWithoutSecretKey() *cluster.Credentials {
	credentials := &cluster.Credentials{}
	c, _ := types.TimestampProto(time.Now())
	credentials.Defaults("all")
	credentials.APIVersion = "v1"
	credentials.ObjectMeta = api.ObjectMeta{
		Name:         globals.MinioCredentialsObjectName,
		GenerationID: "1",
		UUID:         uuid.NewV4().String(),
		CreationTime: api.Timestamp{
			Timestamp: *c,
		},
		ModTime: api.Timestamp{
			Timestamp: *c,
		},
	}
	credentials.SelfLink = credentials.MakeKey("cluster")
	minioAccessKey := uuid.NewV4().String()
	credentials.Spec = cluster.CredentialsSpec{
		KeyValuePairs: []cluster.KeyValue{
			{
				Key:   globals.MinioAccessKeyName,
				Value: []byte(minioAccessKey),
			},
		},
	}
	return credentials
}
