// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

package credentials

import (
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/x509"
	"crypto/x509/pkix"
	"fmt"
	"net"
	"os"
	"path"

	"github.com/pkg/errors"

	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/certs"
	"github.com/pensando/sw/venice/utils/log"
)

const (
	// These file names need to match those in github.com/minio/minio/cmd/config-dir.go
	httpsKeyFileName            = "private.key"
	httpsCertFileName           = "public.crt"
	httpsCaCertFileNameTemplate = "public%d.crt"
)

// GenVosHTTPSAuth generates the credentials for Minio backend to authenticate itself to clients.
func GenVosHTTPSAuth(node, dir string, csrSigner certs.CSRSigner, caTrustChain, trustRoots []*x509.Certificate) error {
	if csrSigner == nil {
		return errors.New("Cannot generate VOS credentials without a CSR signer")
	}
	// As of 10/2018 Minio only supports P-256
	privateKey, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	if err != nil {
		return errors.Wrapf(err, "error generating private key")
	}
	csr, err := certs.CreateCSR(privateKey, &pkix.Name{CommonName: node}, []string{node, globals.Vos}, []net.IP{})
	if err != nil {
		return errors.Wrapf(err, "error generating csr")
	}
	cert, err := csrSigner(csr)
	if err != nil {
		return errors.Wrapf(err, "error generating certificate")
	}

	caDir := path.Join(dir, "CAs")
	perm := os.FileMode(0700)
	err = os.MkdirAll(caDir, perm)
	if err != nil {
		return errors.Wrapf(err, "Error creating directory: %v with permissions: %v", dir, perm)
	}

	certsPath := path.Join(dir, httpsCertFileName)
	err = certs.SaveCertificates(certsPath, append([]*x509.Certificate{cert}, caTrustChain...))
	if err != nil {
		return err
	}

	keyPath := path.Join(dir, httpsKeyFileName)
	err = certs.SavePrivateKey(keyPath, privateKey) // Minio only supports PKCS-1
	if err != nil {
		return err
	}

	caCertsPathTemplate := path.Join(caDir, httpsCaCertFileNameTemplate)
	for i, trustRoot := range trustRoots {
		filePath := fmt.Sprintf(caCertsPathTemplate, i+1)
		err = certs.SaveCertificate(filePath, trustRoot)
		if err != nil {
			log.Errorf("Error storing trust root %s: %v", filePath, err)
		}
	}

	return nil
}

// GenVosAuth generate credentials for Vos instances backed by Minio
func GenVosAuth(node string, csrSigner certs.CSRSigner, caTrustChain, trustRoots []*x509.Certificate) error {
	err := GenVosHTTPSAuth(node, globals.VosHTTPSAuthDir, csrSigner, caTrustChain, trustRoots)
	if err != nil {
		return errors.Wrapf(err, "Error generating VOS node credentials")
	}
	return nil
}

// RemoveVosAuth removes Vos instance credentials
func RemoveVosAuth() error {
	return os.RemoveAll(globals.VosHTTPSAuthDir)
}
