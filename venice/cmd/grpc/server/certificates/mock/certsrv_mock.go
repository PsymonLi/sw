// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package mock

import (
	"bytes"
	"crypto"
	"crypto/x509"
	"fmt"
	"net"
	"sync/atomic"

	"github.com/pkg/errors"
	context "golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/stats"

	"github.com/pensando/sw/venice/cmd/grpc/server/certificates/certapi"
	"github.com/pensando/sw/venice/utils/certs"
	"github.com/pensando/sw/venice/utils/log"
)

type statsHandler struct {
	certSrv *CertSrv
}

// TagRPC can attach some information to the given RPC context. Unused.
func (*statsHandler) TagRPC(c context.Context, t *stats.RPCTagInfo) context.Context {
	return c
}

// HandleRPC updates RPC-level stats. Unused.
func (*statsHandler) HandleRPC(context.Context, stats.RPCStats) {
}

// TagConn can attach some information to the given connection context. Unused.
func (*statsHandler) TagConn(c context.Context, t *stats.ConnTagInfo) context.Context {
	return c
}

// HandleRPC updates connection-level stats. Used to track active and completed connections.
func (h *statsHandler) HandleConn(c context.Context, s stats.ConnStats) {
	switch v := s.(type) {
	case *stats.ConnBegin:
		h.certSrv.incrementConnBegin()
	case *stats.ConnEnd:
		h.certSrv.incrementConnEnd()
	default:
		panic(fmt.Sprintf("Unknown ConnStats type: %T", v))
	}
}

// CertSrv is a mock instance of the CMD certificates server
type CertSrv struct {
	RPCServer   *grpc.Server
	listenURL   string
	privateKey  crypto.PrivateKey
	certificate *x509.Certificate
	trustChain  []*x509.Certificate
	trustRoots  []*x509.Certificate
	// counters
	rpcSuccess uint64
	rpcError   uint64
	connBegin  uint64
	connEnd    uint64
}

func (c *CertSrv) incrementRPCSuccess() {
	atomic.AddUint64(&c.rpcSuccess, 1)
}

func (c *CertSrv) incrementRPCError() {
	atomic.AddUint64(&c.rpcError, 1)
}

func (c *CertSrv) incrementConnBegin() {
	atomic.AddUint64(&c.connBegin, 1)
}

func (c *CertSrv) incrementConnEnd() {
	atomic.AddUint64(&c.connEnd, 1)
}

// SignCertificateRequest is the handler for the RPC method with the same name
// It receives a CSR and returns a signed certificate
func (c *CertSrv) SignCertificateRequest(ctx context.Context, req *certapi.CertificateSignReq) (*certapi.CertificateSignResp, error) {
	var err error

	// make sure counters are updated on return
	defer func() {
		if err != nil {
			c.incrementRPCError()
		} else {
			c.incrementRPCSuccess()
		}
	}()

	// Only check that CSR is syntactically valid
	csr, err := x509.ParseCertificateRequest(req.GetCsr())
	if err != nil {
		log.Errorf("Received invalid certificate request, error: %v", err)
		return nil, errors.Wrapf(err, "Invalid certificate request")
	}

	err = csr.CheckSignature()
	if err != nil {
		log.Errorf("Received CSR with invalid signature, error: %v", err)
		return nil, errors.Wrapf(err, "CSR has invalid signature")
	}

	cert, err := certs.SignCSRwithCA(csr, c.certificate, c.privateKey, certs.WithValidityDays(7))
	if err != nil {
		log.Errorf("Error signing CSR: %v", err)
		return nil, errors.Wrapf(err, "Error signing CSR")
	}

	return &certapi.CertificateSignResp{
		Certificate: &certapi.Certificate{Certificate: cert.Raw},
	}, nil
}

// GetTrustRoots is the handler for the RPC method that returns trust roots
// It returns an array of trusted certificates
func (c *CertSrv) GetTrustRoots(ctx context.Context, empty *certapi.Empty) (*certapi.TrustRoots, error) {
	certs := make([]*certapi.Certificate, 0)
	for _, c := range c.trustRoots {
		certs = append(certs, &certapi.Certificate{Certificate: c.Raw})
	}
	c.incrementRPCSuccess()
	return &certapi.TrustRoots{Certificates: certs}, nil
}

// GetCaTrustChain is the handler for the RPC method that returns the CA trust chain
func (c *CertSrv) GetCaTrustChain(ctx context.Context, empty *certapi.Empty) (*certapi.CaTrustChain, error) {
	certs := make([]*certapi.Certificate, 0)
	for _, c := range c.trustChain {
		certs = append(certs, &certapi.Certificate{Certificate: c.Raw})
	}
	c.incrementRPCSuccess()
	return &certapi.CaTrustChain{Certificates: certs}, nil
}

// NewCertSrv returns a mock instance of the CMD certificates server that uses passed-in keys and certs
func NewCertSrv(listenURL, certPath, keyPath, rootsPath string) (*CertSrv, error) {
	if listenURL == "" {
		return nil, errors.New("ListenURL parameter is required")
	}

	key, err := certs.ReadPrivateKey(keyPath)
	if err != nil {
		return nil, errors.Wrapf(err, "Error reading private key from %s", keyPath)
	}

	caCert, err := certs.ReadCertificate(certPath)
	if err != nil {
		return nil, errors.Wrapf(err, "Error reading certificate from %s", keyPath)
	}

	// create the controller instance
	ctrler := &CertSrv{
		privateKey:  key,
		certificate: caCert,
	}

	trustRoots, err := certs.ReadCertificates(rootsPath)
	if err != nil {
		return nil, errors.Wrapf(err, "Error reading trust roots from %s", rootsPath)
	}

	if certs.IsSelfSigned(caCert) {
		ctrler.trustRoots = append(ctrler.trustRoots, caCert)
		ctrler.trustChain = append(ctrler.trustChain, caCert)
	}

	for _, c := range trustRoots {
		ctrler.trustRoots = append(ctrler.trustRoots, c)
		// assume at most 2-level hierarchy
		if !certs.IsSelfSigned(caCert) && bytes.Equal(c.RawSubject, caCert.RawIssuer) {
			ctrler.trustChain = append(ctrler.trustChain, caCert, c)
		}
	}
	if len(ctrler.trustChain) < 1 {
		return nil, errors.Wrapf(err, "Could not establish CA trust chain")
	}

	// Do not use rpckit to avoid circular dependencies
	listener, err := net.Listen("tcp", listenURL)
	if err != nil {
		return nil, errors.Wrapf(err, "Could start listening at %s", listenURL)
	}
	server := grpc.NewServer(grpc.StatsHandler(&statsHandler{certSrv: ctrler}))
	certapi.RegisterCertificatesServer(server, ctrler)
	ctrler.RPCServer = server
	ctrler.listenURL = listener.Addr().String()
	go server.Serve(listener)

	return ctrler, err
}

// GetListenURL returns the URL the server is listening on
func (c *CertSrv) GetListenURL() string {
	return c.listenURL
}

// Stop stops the gRPC server and performs cleanup
func (c *CertSrv) Stop() error {
	c.RPCServer.Stop()
	return nil
}

// GetRPCSuccessCount returns rpcSuccess count
func (c *CertSrv) GetRPCSuccessCount() uint64 {
	return atomic.LoadUint64(&c.rpcSuccess)
}

// GetRPCErrorCount returns rpcError count
func (c *CertSrv) GetRPCErrorCount() uint64 {
	return atomic.LoadUint64(&c.rpcError)
}

// ClearRPCCounts clears rpcSuccess and rpcError counters
func (c *CertSrv) ClearRPCCounts() {
	atomic.StoreUint64(&c.rpcSuccess, 0)
	atomic.StoreUint64(&c.rpcError, 0)
}

// GetActiveConnCount returns the number of active connections
func (c *CertSrv) GetActiveConnCount() uint64 {
	// Currently there is no function in sync/atomic to subtract two atomic variables.
	// In theory connEnd could be incremented after connBegin has been loaded, resulting
	// in a negative value. It should not happen, but it's better to handle it just in case.
	connBegin := atomic.LoadUint64(&c.connBegin)
	connEnd := atomic.LoadUint64(&c.connEnd)
	if connEnd > connBegin {
		return connEnd - connBegin
	}
	return 0
}

// GetCompletedConnCount returns the total number of connections that have been completed
func (c *CertSrv) GetCompletedConnCount() uint64 {
	return atomic.LoadUint64(&c.connEnd)
}
