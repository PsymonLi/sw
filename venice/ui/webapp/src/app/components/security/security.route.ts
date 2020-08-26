import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

const routes: Routes = [
  {
    path: '',
    redirectTo: 'sgpolicies',
    pathMatch: 'full'
  },
  {
    path: 'sgpolicies',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/security/sgpolicies/sgpolicies.module').then(m => m.SgpoliciesModule)
      }
    ]
  },
  {
    path: 'securityapps',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/security/securityapps/securityapps.module').then(m => m.SecurityappsModule)
      }
    ]
  },
  {
    path: 'securitygroups',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/security/securitygroups/securitygroups.module').then(m => m.SecuritygroupsModule)
      }
    ]
  },
  {
    path: 'firewallprofiles',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/security/firewallprofiles/firewallprofiles.module').then(m => m.FirewallprofilesModule)
      }
    ]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class SecurityRoutingModule { }
