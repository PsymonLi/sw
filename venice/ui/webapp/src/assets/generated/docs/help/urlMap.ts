// Mapping from UI url to doc ID to show on URL load
export const UrlMap: { [url: string]: string } = {
  '/dashboard': 'dashboard',
  '/cluster/cluster': 'cluster',
  '/cluster/cluster/:id': 'clusternode',
  '/cluster/dscs': 'dscs',
  '/cluster/dscs/:id': 'dscdetails',
  '/cluster/hosts': 'hosts',
  '/monitoring/alertsevents': 'alertsevents',
  '/monitoring/alertsevents/alertpolicies': 'alertsevents',
  '/monitoring/alertsevents/alertdestinations': 'alertsevents',
  '/monitoring/alertsevents/eventpolicy': 'alertsevents',
  '/monitoring/auditevents' : 'auditevents',
  '/monitoring/fwlogs': 'fwlogs',
  '/monitoring/fwlogpolicies': 'firewalllogpolicy',
  '/monitoring/metrics': 'metrics',  
  '/security/securityapps': 'apps',
  '/security/sgpolicies': 'policies',
  '/security/sgpolicies/:id': 'policies',
  '/security/securitygroups': 'securitygroups',  
  '/troubleshoot/flowexport': 'flowexport',
  '/troubleshoot/mirrorsessions': 'mirrorsessions',
  '/workload': 'workload',
  '/admin/authpolicy' : 'authpolicy',
  '/admin/upgrade/rollouts': 'systemupgrade',
  '/admin/users': 'localuser',
  '/admin/techsupport': 'techsupport',
  '/admin/snapshots': 'snapshots',
  '/admin/certificate': 'updateservercertificate'
}
