import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';
import { FwlogsComponent } from './fwlogs.component';


const routes: Routes = [
  {
    path: '',
    component: FwlogsComponent
  },
  {
    path: 'fwlogpolicies',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/monitoring-group/fwlogs/fwlogpolicies/fwlogpolicies.module').then(m => m.FwlogpoliciesModule)
      }
    ]
  },
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class FwlogsRoutingModule { }
