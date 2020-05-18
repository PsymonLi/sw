// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package cluster is a auto generated package.
Input file: svc_cluster.proto
*/
package cluster

import (
	"context"

	"github.com/pensando/sw/api"
)

// Dummy definitions to suppress nonused warnings
var _ api.ObjectMeta

// ServiceClusterV1Client  is the client interface for the service.
type ServiceClusterV1Client interface {
	AutoWatchSvcClusterV1(ctx context.Context, in *api.AggWatchOptions) (ClusterV1_AutoWatchSvcClusterV1Client, error)

	AuthBootstrapComplete(ctx context.Context, t *ClusterAuthBootstrapRequest) (*Cluster, error)
	AutoAddCluster(ctx context.Context, t *Cluster) (*Cluster, error)
	AutoAddConfigurationSnapshot(ctx context.Context, t *ConfigurationSnapshot) (*ConfigurationSnapshot, error)
	AutoAddDSCProfile(ctx context.Context, t *DSCProfile) (*DSCProfile, error)
	AutoAddDistributedServiceCard(ctx context.Context, t *DistributedServiceCard) (*DistributedServiceCard, error)
	AutoAddHost(ctx context.Context, t *Host) (*Host, error)
	AutoAddLicense(ctx context.Context, t *License) (*License, error)
	AutoAddNode(ctx context.Context, t *Node) (*Node, error)
	AutoAddSnapshotRestore(ctx context.Context, t *SnapshotRestore) (*SnapshotRestore, error)
	AutoAddTenant(ctx context.Context, t *Tenant) (*Tenant, error)
	AutoAddVersion(ctx context.Context, t *Version) (*Version, error)
	AutoDeleteCluster(ctx context.Context, t *Cluster) (*Cluster, error)
	AutoDeleteConfigurationSnapshot(ctx context.Context, t *ConfigurationSnapshot) (*ConfigurationSnapshot, error)
	AutoDeleteDSCProfile(ctx context.Context, t *DSCProfile) (*DSCProfile, error)
	AutoDeleteDistributedServiceCard(ctx context.Context, t *DistributedServiceCard) (*DistributedServiceCard, error)
	AutoDeleteHost(ctx context.Context, t *Host) (*Host, error)
	AutoDeleteLicense(ctx context.Context, t *License) (*License, error)
	AutoDeleteNode(ctx context.Context, t *Node) (*Node, error)
	AutoDeleteSnapshotRestore(ctx context.Context, t *SnapshotRestore) (*SnapshotRestore, error)
	AutoDeleteTenant(ctx context.Context, t *Tenant) (*Tenant, error)
	AutoDeleteVersion(ctx context.Context, t *Version) (*Version, error)
	AutoGetCluster(ctx context.Context, t *Cluster) (*Cluster, error)
	AutoGetConfigurationSnapshot(ctx context.Context, t *ConfigurationSnapshot) (*ConfigurationSnapshot, error)
	AutoGetDSCProfile(ctx context.Context, t *DSCProfile) (*DSCProfile, error)
	AutoGetDistributedServiceCard(ctx context.Context, t *DistributedServiceCard) (*DistributedServiceCard, error)
	AutoGetHost(ctx context.Context, t *Host) (*Host, error)
	AutoGetLicense(ctx context.Context, t *License) (*License, error)
	AutoGetNode(ctx context.Context, t *Node) (*Node, error)
	AutoGetSnapshotRestore(ctx context.Context, t *SnapshotRestore) (*SnapshotRestore, error)
	AutoGetTenant(ctx context.Context, t *Tenant) (*Tenant, error)
	AutoGetVersion(ctx context.Context, t *Version) (*Version, error)
	AutoLabelCluster(ctx context.Context, t *api.Label) (*Cluster, error)
	AutoLabelConfigurationSnapshot(ctx context.Context, t *api.Label) (*ConfigurationSnapshot, error)
	AutoLabelDSCProfile(ctx context.Context, t *api.Label) (*DSCProfile, error)
	AutoLabelDistributedServiceCard(ctx context.Context, t *api.Label) (*DistributedServiceCard, error)
	AutoLabelHost(ctx context.Context, t *api.Label) (*Host, error)
	AutoLabelLicense(ctx context.Context, t *api.Label) (*License, error)
	AutoLabelNode(ctx context.Context, t *api.Label) (*Node, error)
	AutoLabelSnapshotRestore(ctx context.Context, t *api.Label) (*SnapshotRestore, error)
	AutoLabelTenant(ctx context.Context, t *api.Label) (*Tenant, error)
	AutoLabelVersion(ctx context.Context, t *api.Label) (*Version, error)
	AutoListCluster(ctx context.Context, t *api.ListWatchOptions) (*ClusterList, error)
	AutoListConfigurationSnapshot(ctx context.Context, t *api.ListWatchOptions) (*ConfigurationSnapshotList, error)
	AutoListDSCProfile(ctx context.Context, t *api.ListWatchOptions) (*DSCProfileList, error)
	AutoListDistributedServiceCard(ctx context.Context, t *api.ListWatchOptions) (*DistributedServiceCardList, error)
	AutoListHost(ctx context.Context, t *api.ListWatchOptions) (*HostList, error)
	AutoListLicense(ctx context.Context, t *api.ListWatchOptions) (*LicenseList, error)
	AutoListNode(ctx context.Context, t *api.ListWatchOptions) (*NodeList, error)
	AutoListSnapshotRestore(ctx context.Context, t *api.ListWatchOptions) (*SnapshotRestoreList, error)
	AutoListTenant(ctx context.Context, t *api.ListWatchOptions) (*TenantList, error)
	AutoListVersion(ctx context.Context, t *api.ListWatchOptions) (*VersionList, error)
	AutoUpdateCluster(ctx context.Context, t *Cluster) (*Cluster, error)
	AutoUpdateConfigurationSnapshot(ctx context.Context, t *ConfigurationSnapshot) (*ConfigurationSnapshot, error)
	AutoUpdateDSCProfile(ctx context.Context, t *DSCProfile) (*DSCProfile, error)
	AutoUpdateDistributedServiceCard(ctx context.Context, t *DistributedServiceCard) (*DistributedServiceCard, error)
	AutoUpdateHost(ctx context.Context, t *Host) (*Host, error)
	AutoUpdateLicense(ctx context.Context, t *License) (*License, error)
	AutoUpdateNode(ctx context.Context, t *Node) (*Node, error)
	AutoUpdateSnapshotRestore(ctx context.Context, t *SnapshotRestore) (*SnapshotRestore, error)
	AutoUpdateTenant(ctx context.Context, t *Tenant) (*Tenant, error)
	AutoUpdateVersion(ctx context.Context, t *Version) (*Version, error)
	Restore(ctx context.Context, t *SnapshotRestore) (*SnapshotRestore, error)
	Save(ctx context.Context, t *ConfigurationSnapshotRequest) (*ConfigurationSnapshot, error)
	UpdateTLSConfig(ctx context.Context, t *UpdateTLSConfigRequest) (*Cluster, error)

	AutoWatchCluster(ctx context.Context, in *api.ListWatchOptions) (ClusterV1_AutoWatchClusterClient, error)
	AutoWatchNode(ctx context.Context, in *api.ListWatchOptions) (ClusterV1_AutoWatchNodeClient, error)
	AutoWatchHost(ctx context.Context, in *api.ListWatchOptions) (ClusterV1_AutoWatchHostClient, error)
	AutoWatchDistributedServiceCard(ctx context.Context, in *api.ListWatchOptions) (ClusterV1_AutoWatchDistributedServiceCardClient, error)
	AutoWatchTenant(ctx context.Context, in *api.ListWatchOptions) (ClusterV1_AutoWatchTenantClient, error)
	AutoWatchVersion(ctx context.Context, in *api.ListWatchOptions) (ClusterV1_AutoWatchVersionClient, error)
	AutoWatchConfigurationSnapshot(ctx context.Context, in *api.ListWatchOptions) (ClusterV1_AutoWatchConfigurationSnapshotClient, error)
	AutoWatchSnapshotRestore(ctx context.Context, in *api.ListWatchOptions) (ClusterV1_AutoWatchSnapshotRestoreClient, error)
	AutoWatchLicense(ctx context.Context, in *api.ListWatchOptions) (ClusterV1_AutoWatchLicenseClient, error)
	AutoWatchDSCProfile(ctx context.Context, in *api.ListWatchOptions) (ClusterV1_AutoWatchDSCProfileClient, error)
}

// ServiceClusterV1Server is the server interface for the service.
type ServiceClusterV1Server interface {
	AutoWatchSvcClusterV1(in *api.AggWatchOptions, stream ClusterV1_AutoWatchSvcClusterV1Server) error

	AuthBootstrapComplete(ctx context.Context, t ClusterAuthBootstrapRequest) (Cluster, error)
	AutoAddCluster(ctx context.Context, t Cluster) (Cluster, error)
	AutoAddConfigurationSnapshot(ctx context.Context, t ConfigurationSnapshot) (ConfigurationSnapshot, error)
	AutoAddDSCProfile(ctx context.Context, t DSCProfile) (DSCProfile, error)
	AutoAddDistributedServiceCard(ctx context.Context, t DistributedServiceCard) (DistributedServiceCard, error)
	AutoAddHost(ctx context.Context, t Host) (Host, error)
	AutoAddLicense(ctx context.Context, t License) (License, error)
	AutoAddNode(ctx context.Context, t Node) (Node, error)
	AutoAddSnapshotRestore(ctx context.Context, t SnapshotRestore) (SnapshotRestore, error)
	AutoAddTenant(ctx context.Context, t Tenant) (Tenant, error)
	AutoAddVersion(ctx context.Context, t Version) (Version, error)
	AutoDeleteCluster(ctx context.Context, t Cluster) (Cluster, error)
	AutoDeleteConfigurationSnapshot(ctx context.Context, t ConfigurationSnapshot) (ConfigurationSnapshot, error)
	AutoDeleteDSCProfile(ctx context.Context, t DSCProfile) (DSCProfile, error)
	AutoDeleteDistributedServiceCard(ctx context.Context, t DistributedServiceCard) (DistributedServiceCard, error)
	AutoDeleteHost(ctx context.Context, t Host) (Host, error)
	AutoDeleteLicense(ctx context.Context, t License) (License, error)
	AutoDeleteNode(ctx context.Context, t Node) (Node, error)
	AutoDeleteSnapshotRestore(ctx context.Context, t SnapshotRestore) (SnapshotRestore, error)
	AutoDeleteTenant(ctx context.Context, t Tenant) (Tenant, error)
	AutoDeleteVersion(ctx context.Context, t Version) (Version, error)
	AutoGetCluster(ctx context.Context, t Cluster) (Cluster, error)
	AutoGetConfigurationSnapshot(ctx context.Context, t ConfigurationSnapshot) (ConfigurationSnapshot, error)
	AutoGetDSCProfile(ctx context.Context, t DSCProfile) (DSCProfile, error)
	AutoGetDistributedServiceCard(ctx context.Context, t DistributedServiceCard) (DistributedServiceCard, error)
	AutoGetHost(ctx context.Context, t Host) (Host, error)
	AutoGetLicense(ctx context.Context, t License) (License, error)
	AutoGetNode(ctx context.Context, t Node) (Node, error)
	AutoGetSnapshotRestore(ctx context.Context, t SnapshotRestore) (SnapshotRestore, error)
	AutoGetTenant(ctx context.Context, t Tenant) (Tenant, error)
	AutoGetVersion(ctx context.Context, t Version) (Version, error)
	AutoLabelCluster(ctx context.Context, t api.Label) (Cluster, error)
	AutoLabelConfigurationSnapshot(ctx context.Context, t api.Label) (ConfigurationSnapshot, error)
	AutoLabelDSCProfile(ctx context.Context, t api.Label) (DSCProfile, error)
	AutoLabelDistributedServiceCard(ctx context.Context, t api.Label) (DistributedServiceCard, error)
	AutoLabelHost(ctx context.Context, t api.Label) (Host, error)
	AutoLabelLicense(ctx context.Context, t api.Label) (License, error)
	AutoLabelNode(ctx context.Context, t api.Label) (Node, error)
	AutoLabelSnapshotRestore(ctx context.Context, t api.Label) (SnapshotRestore, error)
	AutoLabelTenant(ctx context.Context, t api.Label) (Tenant, error)
	AutoLabelVersion(ctx context.Context, t api.Label) (Version, error)
	AutoListCluster(ctx context.Context, t api.ListWatchOptions) (ClusterList, error)
	AutoListConfigurationSnapshot(ctx context.Context, t api.ListWatchOptions) (ConfigurationSnapshotList, error)
	AutoListDSCProfile(ctx context.Context, t api.ListWatchOptions) (DSCProfileList, error)
	AutoListDistributedServiceCard(ctx context.Context, t api.ListWatchOptions) (DistributedServiceCardList, error)
	AutoListHost(ctx context.Context, t api.ListWatchOptions) (HostList, error)
	AutoListLicense(ctx context.Context, t api.ListWatchOptions) (LicenseList, error)
	AutoListNode(ctx context.Context, t api.ListWatchOptions) (NodeList, error)
	AutoListSnapshotRestore(ctx context.Context, t api.ListWatchOptions) (SnapshotRestoreList, error)
	AutoListTenant(ctx context.Context, t api.ListWatchOptions) (TenantList, error)
	AutoListVersion(ctx context.Context, t api.ListWatchOptions) (VersionList, error)
	AutoUpdateCluster(ctx context.Context, t Cluster) (Cluster, error)
	AutoUpdateConfigurationSnapshot(ctx context.Context, t ConfigurationSnapshot) (ConfigurationSnapshot, error)
	AutoUpdateDSCProfile(ctx context.Context, t DSCProfile) (DSCProfile, error)
	AutoUpdateDistributedServiceCard(ctx context.Context, t DistributedServiceCard) (DistributedServiceCard, error)
	AutoUpdateHost(ctx context.Context, t Host) (Host, error)
	AutoUpdateLicense(ctx context.Context, t License) (License, error)
	AutoUpdateNode(ctx context.Context, t Node) (Node, error)
	AutoUpdateSnapshotRestore(ctx context.Context, t SnapshotRestore) (SnapshotRestore, error)
	AutoUpdateTenant(ctx context.Context, t Tenant) (Tenant, error)
	AutoUpdateVersion(ctx context.Context, t Version) (Version, error)
	Restore(ctx context.Context, t SnapshotRestore) (SnapshotRestore, error)
	Save(ctx context.Context, t ConfigurationSnapshotRequest) (ConfigurationSnapshot, error)
	UpdateTLSConfig(ctx context.Context, t UpdateTLSConfigRequest) (Cluster, error)

	AutoWatchCluster(in *api.ListWatchOptions, stream ClusterV1_AutoWatchClusterServer) error
	AutoWatchNode(in *api.ListWatchOptions, stream ClusterV1_AutoWatchNodeServer) error
	AutoWatchHost(in *api.ListWatchOptions, stream ClusterV1_AutoWatchHostServer) error
	AutoWatchDistributedServiceCard(in *api.ListWatchOptions, stream ClusterV1_AutoWatchDistributedServiceCardServer) error
	AutoWatchTenant(in *api.ListWatchOptions, stream ClusterV1_AutoWatchTenantServer) error
	AutoWatchVersion(in *api.ListWatchOptions, stream ClusterV1_AutoWatchVersionServer) error
	AutoWatchConfigurationSnapshot(in *api.ListWatchOptions, stream ClusterV1_AutoWatchConfigurationSnapshotServer) error
	AutoWatchSnapshotRestore(in *api.ListWatchOptions, stream ClusterV1_AutoWatchSnapshotRestoreServer) error
	AutoWatchLicense(in *api.ListWatchOptions, stream ClusterV1_AutoWatchLicenseServer) error
	AutoWatchDSCProfile(in *api.ListWatchOptions, stream ClusterV1_AutoWatchDSCProfileServer) error
}
