package minio

import (
	"context"
	"crypto/rand"
	"encoding/base32"
	"fmt"
	"strings"
	"time"

	"github.com/gogo/protobuf/types"
	"github.com/pkg/errors"
	uuid "github.com/satori/go.uuid"

	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/log"
)

const defaultRetries = 3
const defaultWaitTime = 3 * time.Second

//CredentialsManager is an interface for minio credentials manager
type CredentialsManager interface {
	//GetCredentials fetches minio credentials from sources specific to implementations
	GetCredentials() (*Credentials, error)

	//CreateCredentials generates minio credentials and persists them to implementation specific backend
	CreateCredentials() (*Credentials, error)
}

//Credentials is a concrete type to hold minio credentials
type Credentials struct {
	AccessKey string
	SecretKey string
}

//APIServerBasedCredsManager is an implementation of CredentialsManager that manages minio credentials via APIServer
type APIServerBasedCredsManager struct {
	apiClient     cluster.ClusterV1Interface
	retryCount    int
	retryInterval time.Duration
}

// Option provides optional parameters to the CredentialManager constructor
type Option func(cp CredentialsManager)

//NewAPIServerBasedCredsManager creates a new instance of API server based credentials manager
func NewAPIServerBasedCredsManager(apiClient cluster.ClusterV1Interface, options ...Option) *APIServerBasedCredsManager {
	apiServerBasedCredsMgr := &APIServerBasedCredsManager{
		apiClient:     apiClient,
		retryCount:    defaultRetries,
		retryInterval: defaultWaitTime,
	}
	for _, o := range options {
		o(apiServerBasedCredsMgr)
	}
	return apiServerBasedCredsMgr
}

//NewCredentialsManager creates a new instance of API server based credentials manager
func NewCredentialsManager(clientName string, resolver resolver.Interface) (*APIServerBasedCredsManager, error) {
	// create the api client
	l := log.WithContext("Pkg", "objstoreClient")
	b := balancer.New(resolver)
	apiClient, err := apiclient.NewGrpcAPIClient(clientName, globals.APIServer, l, rpckit.WithBalancer(b))
	if err != nil {
		log.Errorf("Failed to connect to api gRPC server [%s]\n", globals.APIServer)
		b.Close()
		return nil, err
	}
	return NewAPIServerBasedCredsManager(apiClient.ClusterV1()), nil
}

//WithAPIRetries sets the retry count for API server related calls
func WithAPIRetries(retryCount int, retryInterval time.Duration) Option {
	return func(cp CredentialsManager) {
		if inst, ok := cp.(*APIServerBasedCredsManager); ok {
			inst.retryCount = retryCount
			inst.retryInterval = retryInterval
		}
	}
}

//GetCredentials fetches minio credentials from APIServer
func (credsManager *APIServerBasedCredsManager) GetCredentials() (*Credentials, error) {
	var err error
	var credentials *cluster.Credentials
	objectMeta := &api.ObjectMeta{
		Name: globals.MinioCredentialsObjectName,
	}
	for attempt := 0; attempt < credsManager.retryCount; attempt++ {
		select {
		case <-time.After(credsManager.retryInterval):
			ctx, cancel := context.WithTimeout(context.Background(), credsManager.retryInterval)
			credentials, err = credsManager.apiClient.Credentials().Get(ctx, objectMeta)
			if err != nil {
				log.Errorf("Attempt#%d, failed to get MINIO credentials from API server, error: %s", attempt+1, err.Error())
				cancel()
				continue
			}
			cancel()
			return GetMinioKeys(credentials)
		}
	}
	return nil, errors.Wrap(err, "Failed to get MINIO credentials from API server")
}

//CreateCredentials creates minio credentials if there isn't one already existing
func (credsManager *APIServerBasedCredsManager) CreateCredentials() (*Credentials, error) {
	var err, getErr, mappingErr error
	var createdCredentials *cluster.Credentials
	var minioKeys *Credentials
	credentials, err := GenerateObjectStoreCredentials()
	if err != nil {
		return nil, errors.Wrap(err, "failed to generate minio credentials object")
	}
	for attempt := 0; attempt < credsManager.retryCount; attempt++ {
		select {
		case <-time.After(credsManager.retryInterval):
			ctx, cancel := context.WithTimeout(context.Background(), credsManager.retryInterval)
			createdCredentials, err = credsManager.apiClient.Credentials().Create(ctx, credentials)
			if err != nil {
				if !strings.Contains(err.Error(), "AlreadyExists") {
					log.Errorf("Attempt#%d, failed to create Credentials for object store, error: %v", attempt+1, err)
					cancel()
					continue
				}
				log.Infof("Credentials with name: %s already exist", credentials.Name)
				minioKeys, getErr = credsManager.GetCredentials()
				if getErr != nil {
					log.Errorf("Unable to get existing Minio keys from api-server")
					cancel()
					return nil, errors.Wrap(getErr, "Unable to get existing Minio keys from api-server")
				}
			} else {
				minioKeys, mappingErr = GetMinioKeys(createdCredentials)
				if mappingErr != nil {
					log.Errorf("Unable to extract Minio keys from Credentials object")
					cancel()
					return nil, errors.Wrap(mappingErr, "Unable to extract Minio keys from Credentials object")
				}
			}
			cancel()
			return minioKeys, err
		}
	}
	return nil, errors.Wrap(err, "failed to create Credentials for object store")
}

// GenerateObjectStoreCredentials creates a Credentials object using randomly generated minio access/secret keys
func GenerateObjectStoreCredentials() (*cluster.Credentials, error) {
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
	//accessKey, err := generateRandomKey()
	//if err != nil {
	//	return nil, errors.Wrap(err, "unable to generate random access key")
	//}
	//secretKey, err := generateRandomKey()
	//if err != nil {
	//	return nil, errors.Wrap(err, "unable to generate random secret key")
	//}
	// TODO: This is a temporary fix to unblock QA, will be reverted to random keys once upgrade issue is resolved.
	accessKey := "miniokey"
	secretKey := "minio0523"
	credentials.Spec = cluster.CredentialsSpec{
		KeyValuePairs: []cluster.KeyValue{
			{
				Key:   globals.MinioAccessKeyName,
				Value: []byte(accessKey),
			},
			{
				Key:   globals.MinioSecretKeyName,
				Value: []byte(secretKey),
			},
		},
	}
	return credentials, nil
}

func generateRandomKey() (string, error) {
	keyBytes := make([]byte, 32)
	_, err := rand.Read(keyBytes)
	if err != nil {
		return "", err
	}
	treatedForMinio := base32.StdEncoding.WithPadding(base32.NoPadding).EncodeToString(keyBytes)
	return treatedForMinio, nil
}

// GetMinioKeys extracts minio access, secret keys from Credentials object
func GetMinioKeys(credentials *cluster.Credentials) (creds *Credentials, err error) {
	var accessKey, secretKey string
	for _, kvPair := range credentials.Spec.KeyValuePairs {
		switch kvPair.Key {
		case globals.MinioAccessKeyName:
			accessKey = string(kvPair.Value)
		case globals.MinioSecretKeyName:
			secretKey = string(kvPair.Value)
		default:
			return nil, fmt.Errorf("invalid key found: %s, expected to be %s or %s", kvPair.Key, globals.MinioSecretKeyName, globals.MinioAccessKeyName)
		}
	}
	if accessKey == "" || secretKey == "" {
		return nil, errors.New("access key or secret key can not be blank")
	}
	return &Credentials{
		AccessKey: accessKey,
		SecretKey: secretKey,
	}, nil
}

//ValidateObjectStoreCredentials validates credentials object has the necessary key materials needed to access minio
func ValidateObjectStoreCredentials(credentials *cluster.Credentials) error {
	if credentials == nil {
		return errors.New("credentials can not be nil")
	}
	_, err := GetMinioKeys(credentials)
	return err
}
