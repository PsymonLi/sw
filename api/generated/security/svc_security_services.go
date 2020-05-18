// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package security is a auto generated package.
Input file: svc_security.proto
*/
package security

import (
	"context"

	"github.com/pensando/sw/api"
)

// Dummy definitions to suppress nonused warnings
var _ api.ObjectMeta

// ServiceSecurityV1Client  is the client interface for the service.
type ServiceSecurityV1Client interface {
	AutoWatchSvcSecurityV1(ctx context.Context, in *api.AggWatchOptions) (SecurityV1_AutoWatchSvcSecurityV1Client, error)

	AutoAddApp(ctx context.Context, t *App) (*App, error)
	AutoAddCertificate(ctx context.Context, t *Certificate) (*Certificate, error)
	AutoAddFirewallProfile(ctx context.Context, t *FirewallProfile) (*FirewallProfile, error)
	AutoAddNetworkSecurityPolicy(ctx context.Context, t *NetworkSecurityPolicy) (*NetworkSecurityPolicy, error)
	AutoAddSecurityGroup(ctx context.Context, t *SecurityGroup) (*SecurityGroup, error)
	AutoAddTrafficEncryptionPolicy(ctx context.Context, t *TrafficEncryptionPolicy) (*TrafficEncryptionPolicy, error)
	AutoDeleteApp(ctx context.Context, t *App) (*App, error)
	AutoDeleteCertificate(ctx context.Context, t *Certificate) (*Certificate, error)
	AutoDeleteFirewallProfile(ctx context.Context, t *FirewallProfile) (*FirewallProfile, error)
	AutoDeleteNetworkSecurityPolicy(ctx context.Context, t *NetworkSecurityPolicy) (*NetworkSecurityPolicy, error)
	AutoDeleteSecurityGroup(ctx context.Context, t *SecurityGroup) (*SecurityGroup, error)
	AutoDeleteTrafficEncryptionPolicy(ctx context.Context, t *TrafficEncryptionPolicy) (*TrafficEncryptionPolicy, error)
	AutoGetApp(ctx context.Context, t *App) (*App, error)
	AutoGetCertificate(ctx context.Context, t *Certificate) (*Certificate, error)
	AutoGetFirewallProfile(ctx context.Context, t *FirewallProfile) (*FirewallProfile, error)
	AutoGetNetworkSecurityPolicy(ctx context.Context, t *NetworkSecurityPolicy) (*NetworkSecurityPolicy, error)
	AutoGetSecurityGroup(ctx context.Context, t *SecurityGroup) (*SecurityGroup, error)
	AutoGetTrafficEncryptionPolicy(ctx context.Context, t *TrafficEncryptionPolicy) (*TrafficEncryptionPolicy, error)
	AutoLabelApp(ctx context.Context, t *api.Label) (*App, error)
	AutoLabelCertificate(ctx context.Context, t *api.Label) (*Certificate, error)
	AutoLabelFirewallProfile(ctx context.Context, t *api.Label) (*FirewallProfile, error)
	AutoLabelNetworkSecurityPolicy(ctx context.Context, t *api.Label) (*NetworkSecurityPolicy, error)
	AutoLabelSecurityGroup(ctx context.Context, t *api.Label) (*SecurityGroup, error)
	AutoLabelTrafficEncryptionPolicy(ctx context.Context, t *api.Label) (*TrafficEncryptionPolicy, error)
	AutoListApp(ctx context.Context, t *api.ListWatchOptions) (*AppList, error)
	AutoListCertificate(ctx context.Context, t *api.ListWatchOptions) (*CertificateList, error)
	AutoListFirewallProfile(ctx context.Context, t *api.ListWatchOptions) (*FirewallProfileList, error)
	AutoListNetworkSecurityPolicy(ctx context.Context, t *api.ListWatchOptions) (*NetworkSecurityPolicyList, error)
	AutoListSecurityGroup(ctx context.Context, t *api.ListWatchOptions) (*SecurityGroupList, error)
	AutoListTrafficEncryptionPolicy(ctx context.Context, t *api.ListWatchOptions) (*TrafficEncryptionPolicyList, error)
	AutoUpdateApp(ctx context.Context, t *App) (*App, error)
	AutoUpdateCertificate(ctx context.Context, t *Certificate) (*Certificate, error)
	AutoUpdateFirewallProfile(ctx context.Context, t *FirewallProfile) (*FirewallProfile, error)
	AutoUpdateNetworkSecurityPolicy(ctx context.Context, t *NetworkSecurityPolicy) (*NetworkSecurityPolicy, error)
	AutoUpdateSecurityGroup(ctx context.Context, t *SecurityGroup) (*SecurityGroup, error)
	AutoUpdateTrafficEncryptionPolicy(ctx context.Context, t *TrafficEncryptionPolicy) (*TrafficEncryptionPolicy, error)

	AutoWatchSecurityGroup(ctx context.Context, in *api.ListWatchOptions) (SecurityV1_AutoWatchSecurityGroupClient, error)
	AutoWatchNetworkSecurityPolicy(ctx context.Context, in *api.ListWatchOptions) (SecurityV1_AutoWatchNetworkSecurityPolicyClient, error)
	AutoWatchApp(ctx context.Context, in *api.ListWatchOptions) (SecurityV1_AutoWatchAppClient, error)
	AutoWatchFirewallProfile(ctx context.Context, in *api.ListWatchOptions) (SecurityV1_AutoWatchFirewallProfileClient, error)
	AutoWatchCertificate(ctx context.Context, in *api.ListWatchOptions) (SecurityV1_AutoWatchCertificateClient, error)
	AutoWatchTrafficEncryptionPolicy(ctx context.Context, in *api.ListWatchOptions) (SecurityV1_AutoWatchTrafficEncryptionPolicyClient, error)
}

// ServiceSecurityV1Server is the server interface for the service.
type ServiceSecurityV1Server interface {
	AutoWatchSvcSecurityV1(in *api.AggWatchOptions, stream SecurityV1_AutoWatchSvcSecurityV1Server) error

	AutoAddApp(ctx context.Context, t App) (App, error)
	AutoAddCertificate(ctx context.Context, t Certificate) (Certificate, error)
	AutoAddFirewallProfile(ctx context.Context, t FirewallProfile) (FirewallProfile, error)
	AutoAddNetworkSecurityPolicy(ctx context.Context, t NetworkSecurityPolicy) (NetworkSecurityPolicy, error)
	AutoAddSecurityGroup(ctx context.Context, t SecurityGroup) (SecurityGroup, error)
	AutoAddTrafficEncryptionPolicy(ctx context.Context, t TrafficEncryptionPolicy) (TrafficEncryptionPolicy, error)
	AutoDeleteApp(ctx context.Context, t App) (App, error)
	AutoDeleteCertificate(ctx context.Context, t Certificate) (Certificate, error)
	AutoDeleteFirewallProfile(ctx context.Context, t FirewallProfile) (FirewallProfile, error)
	AutoDeleteNetworkSecurityPolicy(ctx context.Context, t NetworkSecurityPolicy) (NetworkSecurityPolicy, error)
	AutoDeleteSecurityGroup(ctx context.Context, t SecurityGroup) (SecurityGroup, error)
	AutoDeleteTrafficEncryptionPolicy(ctx context.Context, t TrafficEncryptionPolicy) (TrafficEncryptionPolicy, error)
	AutoGetApp(ctx context.Context, t App) (App, error)
	AutoGetCertificate(ctx context.Context, t Certificate) (Certificate, error)
	AutoGetFirewallProfile(ctx context.Context, t FirewallProfile) (FirewallProfile, error)
	AutoGetNetworkSecurityPolicy(ctx context.Context, t NetworkSecurityPolicy) (NetworkSecurityPolicy, error)
	AutoGetSecurityGroup(ctx context.Context, t SecurityGroup) (SecurityGroup, error)
	AutoGetTrafficEncryptionPolicy(ctx context.Context, t TrafficEncryptionPolicy) (TrafficEncryptionPolicy, error)
	AutoLabelApp(ctx context.Context, t api.Label) (App, error)
	AutoLabelCertificate(ctx context.Context, t api.Label) (Certificate, error)
	AutoLabelFirewallProfile(ctx context.Context, t api.Label) (FirewallProfile, error)
	AutoLabelNetworkSecurityPolicy(ctx context.Context, t api.Label) (NetworkSecurityPolicy, error)
	AutoLabelSecurityGroup(ctx context.Context, t api.Label) (SecurityGroup, error)
	AutoLabelTrafficEncryptionPolicy(ctx context.Context, t api.Label) (TrafficEncryptionPolicy, error)
	AutoListApp(ctx context.Context, t api.ListWatchOptions) (AppList, error)
	AutoListCertificate(ctx context.Context, t api.ListWatchOptions) (CertificateList, error)
	AutoListFirewallProfile(ctx context.Context, t api.ListWatchOptions) (FirewallProfileList, error)
	AutoListNetworkSecurityPolicy(ctx context.Context, t api.ListWatchOptions) (NetworkSecurityPolicyList, error)
	AutoListSecurityGroup(ctx context.Context, t api.ListWatchOptions) (SecurityGroupList, error)
	AutoListTrafficEncryptionPolicy(ctx context.Context, t api.ListWatchOptions) (TrafficEncryptionPolicyList, error)
	AutoUpdateApp(ctx context.Context, t App) (App, error)
	AutoUpdateCertificate(ctx context.Context, t Certificate) (Certificate, error)
	AutoUpdateFirewallProfile(ctx context.Context, t FirewallProfile) (FirewallProfile, error)
	AutoUpdateNetworkSecurityPolicy(ctx context.Context, t NetworkSecurityPolicy) (NetworkSecurityPolicy, error)
	AutoUpdateSecurityGroup(ctx context.Context, t SecurityGroup) (SecurityGroup, error)
	AutoUpdateTrafficEncryptionPolicy(ctx context.Context, t TrafficEncryptionPolicy) (TrafficEncryptionPolicy, error)

	AutoWatchSecurityGroup(in *api.ListWatchOptions, stream SecurityV1_AutoWatchSecurityGroupServer) error
	AutoWatchNetworkSecurityPolicy(in *api.ListWatchOptions, stream SecurityV1_AutoWatchNetworkSecurityPolicyServer) error
	AutoWatchApp(in *api.ListWatchOptions, stream SecurityV1_AutoWatchAppServer) error
	AutoWatchFirewallProfile(in *api.ListWatchOptions, stream SecurityV1_AutoWatchFirewallProfileServer) error
	AutoWatchCertificate(in *api.ListWatchOptions, stream SecurityV1_AutoWatchCertificateServer) error
	AutoWatchTrafficEncryptionPolicy(in *api.ListWatchOptions, stream SecurityV1_AutoWatchTrafficEncryptionPolicyServer) error
}
