import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';


const routes: Routes = [
  {
    path: '',
    redirectTo: 'vcenter',
    pathMatch: 'full'
  },
  {
    path: 'vcenter',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/controller/vcenterIntegrations/vcenterIntegrations.module').then(m => m.VcenterIntegrationsModule)
      }
    ]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class ControllerRoutingModule { }
