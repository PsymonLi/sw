package minio

import (
	"context"
	"crypto/tls"
	"encoding/json"
	"fmt"
	"net/http"
	"time"

	"github.com/minio/minio/pkg/madmin"

	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/objstore/minio"
	"github.com/pensando/sw/venice/utils/resolver"
)

const (
	maxRetry = 3
)

// Option provides optional parameters to the client constructor
type Option func(c *adminClient)

// WithCredentialsManager tbd
func WithCredentialsManager(credentialsManager minio.CredentialsManager) Option {
	return func(c *adminClient) {
		c.credentialsManager = credentialsManager
	}
}

// WithTLSConfig provides a custom TLS configuration for the client to use when
// contacting the backend. Can be used to provide custom trust roots, server names,
// cipher configs, etc.
func WithTLSConfig(tlsConfig *tls.Config) Option {
	return func(c *adminClient) {
		c.tlsConfig = tlsConfig
	}
}

// AdminClient represents the admin tasks that can performed on the minio cluster
type AdminClient interface {
	ClusterInfo(ctx context.Context) (string, error)
	DataUsageInfo(ctx context.Context) (string, error)
	ServiceRestart(ctx context.Context) error
	IsClusterOnline(ctx context.Context) (bool, error)
}

type adminClient struct {
	r                  resolver.Interface
	mAdminC            *madmin.AdminClient
	tlsConfig          *tls.Config
	credentialsManager minio.CredentialsManager
}

// NewAdminClient creates a new Admin client
func NewAdminClient(rslvr resolver.Interface, opts ...Option) (AdminClient, error) {
	c := &adminClient{
		r: rslvr,
	}

	for _, o := range opts {
		o(c)
	}

	if err := c.connect(""); err != nil {
		return nil, err
	}

	return c, nil
}

// NewPinnedAdminClient creates a new Admin client for a particular url
func NewPinnedAdminClient(url string, opts ...Option) (AdminClient, error) {
	c := &adminClient{}

	for _, o := range opts {
		o(c)
	}

	if err := c.connect(url); err != nil {
		return nil, err
	}

	return c, nil
}

func (c *adminClient) connect(url string) error {
	if c.credentialsManager == nil {
		panic("minio credentails must be provided")
	}

	creds, err := c.credentialsManager.GetCredentials()
	if err != nil {
		return err
	}

	helper := func(url string) error {
		for i := 0; i < maxRetry; i++ {
			log.Infof("connecting admin to {%s} %s %v", globals.VosMinio, url, c.tlsConfig != nil)
			c.mAdminC, err = madmin.New(url, creds.AccessKey, creds.SecretKey, c.tlsConfig != nil)
		}
		if err != nil {
			log.Errorf("error in creating minio admin client url %s, err %v", url, err)
			return err
		}
		return nil
	}

	if url != "" {
		err := helper(url)
		if err != nil {
			return err
		}
	} else {
		// API requests are secure (HTTPS) if secure=true and insecure (HTTPS) otherwise.
		// New returns an MinIO Admin client object.
		// connect and check
		addrs, err := c.getObjStoreAddr()
		if err != nil {
			return err
		}

		for _, url := range addrs {
			err = helper(url)
			if err != nil {
				continue
			}
		}
		if err != nil {
			return err
		}
	}

	if c.tlsConfig != nil {
		tr := &http.Transport{
			TLSClientConfig:    c.tlsConfig,
			DisableCompression: true,
		}
		c.mAdminC.SetCustomTransport(tr)
	}

	return nil
}

// ClusterInfo returns json serialized Cluster Info
func (c *adminClient) ClusterInfo(ctx context.Context) (string, error) {
	st, err := c.mAdminC.ServerInfo(ctx)
	if err != nil {
		return "", err
	}
	temp, err := json.Marshal(st)
	if err != nil {
		return "", err
	}
	return string(temp[:]), nil
}

// DataUsageInfo returns json serialized DataUsageInfo
func (c *adminClient) DataUsageInfo(ctx context.Context) (string, error) {
	st, err := c.mAdminC.DataUsageInfo(ctx)
	if err != nil {
		return "", err
	}
	temp, err := json.Marshal(st)
	if err != nil {
		return "", err
	}
	return string(temp[:]), nil
}

// ServiceRestart restarts all the connected Minio instances in the cluster
func (c *adminClient) ServiceRestart(ctx context.Context) error {
	return c.mAdminC.ServiceRestart(ctx)
}

// IsClusterOnline check is the cluster is functional, otherwise returns an error
func (c *adminClient) IsClusterOnline(ctx context.Context) (bool, error) {
	st, err := c.mAdminC.ServerInfo(ctx)
	if err != nil {
		return false, err
	}
	return st.Mode == "online", nil
}

// getObjStoreAddr get object store URLs from the resolver
func (c *adminClient) getObjStoreAddr() ([]string, error) {
	for i := 0; i < maxRetry; i++ {
		addr := c.r.GetURLs(globals.VosMinio)
		if len(addr) > 0 {
			return addr, nil
		}
		time.Sleep(time.Second * 1)
		log.Warnf("failed to get {%s} url, retrying", globals.VosMinio)
	}

	return []string{}, fmt.Errorf("failed to get object store url from resolver")
}
