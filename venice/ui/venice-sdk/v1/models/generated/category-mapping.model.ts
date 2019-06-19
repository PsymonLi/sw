import { AuthUser } from './auth';
import { AuthAuthenticationPolicy } from './auth';
import { AuthRole } from './auth';
import { AuthRoleBinding } from './auth';
import { AuthUserPreference } from './auth';
import { ClusterCluster } from './cluster';
import { ClusterNode } from './cluster';
import { ClusterHost } from './cluster';
import { ClusterSmartNIC } from './cluster';
import { ClusterTenant } from './cluster';
import { ClusterVersion } from './cluster';
import { DiagnosticsModule } from './diagnostics';
import { MonitoringEventPolicy } from './monitoring';
import { MonitoringStatsPolicy } from './monitoring';
import { MonitoringFwlogPolicy } from './monitoring';
import { MonitoringFlowExportPolicy } from './monitoring';
import { MonitoringAlert } from './monitoring';
import { MonitoringAlertPolicy } from './monitoring';
import { MonitoringAlertDestination } from './monitoring';
import { MonitoringMirrorSession } from './monitoring';
import { MonitoringTechSupportRequest } from './monitoring';
import { EventsEvent } from './events';
import { AuditEvent } from './audit';
import { NetworkNetwork } from './network';
import { NetworkService } from './network';
import { NetworkLbPolicy } from './network';
import { NetworkVirtualRouter } from './network';
import { NetworkNetworkInterface } from './network';
import { ObjstoreBucket } from './objstore';
import { ObjstoreObject } from './objstore';
import { RolloutRollout } from './rollout';
import { RolloutRolloutAction } from './rollout';
import { SecuritySecurityGroup } from './security';
import { SecuritySGPolicy } from './security';
import { SecurityApp } from './security';
import { SecurityFirewallProfile } from './security';
import { SecurityCertificate } from './security';
import { SecurityTrafficEncryptionPolicy } from './security';
import { StagingBuffer } from './staging';
import { WorkloadEndpoint } from './workload';
import { WorkloadWorkload } from './workload';

interface CatMap {
  [key: string]: {
    [key: string]: {
      instance: any,
      scopes: string[],
      actions: string[],
    }
  }
}

export const CategoryMapping: CatMap  = {
  "Auth" : {
    "User" : {
      instance: new AuthUser(),
      scopes: [ 'tenant', ] ,
      actions:  [ 'PasswordChange',  'PasswordReset',  'IsAuthorized', ] ,
    },
    "AuthenticationPolicy" : {
      instance: new AuthAuthenticationPolicy(),
      scopes: [ 'cluster', ] ,
      actions:  [ 'LdapConnectionCheck',  'LdapBindCheck',  'TokenSecretGenerate', ] ,
    },
    "Role" : {
      instance: new AuthRole(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "RoleBinding" : {
      instance: new AuthRoleBinding(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "UserPreference" : {
      instance: new AuthUserPreference(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
  },
  "Cluster" : {
    "Cluster" : {
      instance: new ClusterCluster(),
      scopes: [ 'cluster', ] ,
      actions:  [ 'AuthBootstrapComplete',  'UpdateTLSConfig', ] ,
    },
    "Node" : {
      instance: new ClusterNode(),
      scopes: [ 'cluster', ] ,
      actions:  [] ,
    },
    "Host" : {
      instance: new ClusterHost(),
      scopes: [ 'cluster', ] ,
      actions:  [] ,
    },
    "SmartNIC" : {
      instance: new ClusterSmartNIC(),
      scopes: [ 'cluster', ] ,
      actions:  [] ,
    },
    "Tenant" : {
      instance: new ClusterTenant(),
      scopes: [ 'cluster', ] ,
      actions:  [] ,
    },
    "Version" : {
      instance: new ClusterVersion(),
      scopes: [ 'cluster', ] ,
      actions:  [] ,
    },
  },
  "Diagnostics" : {
    "Module" : {
      instance: new DiagnosticsModule(),
      scopes: [ 'cluster', ] ,
      actions:  [ 'Debug', ] ,
    },
  },
  "Monitoring" : {
    "EventPolicy" : {
      instance: new MonitoringEventPolicy(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "StatsPolicy" : {
      instance: new MonitoringStatsPolicy(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "FwlogPolicy" : {
      instance: new MonitoringFwlogPolicy(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "FlowExportPolicy" : {
      instance: new MonitoringFlowExportPolicy(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "Alert" : {
      instance: new MonitoringAlert(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "AlertPolicy" : {
      instance: new MonitoringAlertPolicy(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "AlertDestination" : {
      instance: new MonitoringAlertDestination(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "MirrorSession" : {
      instance: new MonitoringMirrorSession(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "TechSupportRequest" : {
      instance: new MonitoringTechSupportRequest(),
      scopes: [ 'cluster', ] ,
      actions:  [] ,
    },
    "Event" : {
      instance: new EventsEvent(),
      scopes: [] ,
      actions:  [] ,
    },
    "AuditEvent" : {
      instance: new AuditEvent(),
      scopes: [] ,
      actions:  [] ,
    },
  },
  "Network" : {
    "Network" : {
      instance: new NetworkNetwork(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "Service" : {
      instance: new NetworkService(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "LbPolicy" : {
      instance: new NetworkLbPolicy(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "VirtualRouter" : {
      instance: new NetworkVirtualRouter(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "NetworkInterface" : {
      instance: new NetworkNetworkInterface(),
      scopes: [ 'cluster', ] ,
      actions:  [] ,
    },
  },
  "Objstore" : {
    "Bucket" : {
      instance: new ObjstoreBucket(),
      scopes: [ 'cluster', ] ,
      actions:  [] ,
    },
    "Object" : {
      instance: new ObjstoreObject(),
      scopes: [ 'tenant',  'namespace', ] ,
      actions:  [] ,
    },
  },
  "Rollout" : {
    "Rollout" : {
      instance: new RolloutRollout(),
      scopes: [ 'cluster', ] ,
      actions:  [ 'CreateRollout',  'UpdateRollout',  'StopRollout',  'RemoveRollout', ] ,
    },
    "RolloutAction" : {
      instance: new RolloutRolloutAction(),
      scopes: [ 'cluster', ] ,
      actions:  [] ,
    },
  },
  "Security" : {
    "SecurityGroup" : {
      instance: new SecuritySecurityGroup(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "SGPolicy" : {
      instance: new SecuritySGPolicy(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "App" : {
      instance: new SecurityApp(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "FirewallProfile" : {
      instance: new SecurityFirewallProfile(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "Certificate" : {
      instance: new SecurityCertificate(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "TrafficEncryptionPolicy" : {
      instance: new SecurityTrafficEncryptionPolicy(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
  },
  "Staging" : {
    "Buffer" : {
      instance: new StagingBuffer(),
      scopes: [ 'tenant', ] ,
      actions:  [ 'commit',  'clear', ] ,
    },
  },
  "Workload" : {
    "Endpoint" : {
      instance: new WorkloadEndpoint(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
    "Workload" : {
      instance: new WorkloadWorkload(),
      scopes: [ 'tenant', ] ,
      actions:  [] ,
    },
  },
}