import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';
import { SystemupgradeComponent } from '@app/components/admin/systemupgrade/systemupgrade.component';


const routes: Routes = [
  {
    path: '',
    redirectTo: 'rollouts',
    pathMatch: 'full'
  },
  {
    path: 'imageupload',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/admin/systemupgrade/imageupload/imageupload.module').then(m => m.ImageuploadModule)
      }
    ]
  },
  {
    path: 'rollouts',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/admin/systemupgrade/rollouts/rollouts.module').then(m => m.RolloutsModule)
      }
    ]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class SystemupgradeRoutingModule { }
