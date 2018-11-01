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
        loadChildren: '@app/components/cluster-group/cluster/cluster.module#ClusterModule'
      }
    ]
  },
  {
    path: 'naples',
    children: [
      {
        path: '',
        loadChildren: '@app/components/cluster-group/naples/naples.module#NaplesModule'
      }
    ]
  },
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class ClusterGroupRoutingModule { }
