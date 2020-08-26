import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';
import { NaplesComponent } from './naples/naples.component';


const routes: Routes = [
  {
    path: '',
    redirectTo: 'cluster',
    pathMatch: 'full'
  },
  {
    path: 'cluster',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/cluster-group/cluster/cluster.module').then(m => m.ClusterModule)
      }
    ]
  },
  {
    path: 'dscs',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/cluster-group/naples/naples.module').then(m => m.NaplesModule)
      }
    ]
  },
  {
    path: 'dscprofiles',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/cluster-group/dscprofiles/dscprofiles.module').then(m => m.DscprofilesModule)
      }
    ]
  },
  {
    path: 'networkinterfaces',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/cluster-group/networkinterfaces/networkinterfaces.module').then(m => m.NetworkinterfacesModule)
      }
    ]
  },
  {
    path: 'hosts',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/cluster-group/hosts/hosts.module').then(m => m.HostsModule)
      }
    ]
  },
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class ClusterGroupRoutingModule { }
