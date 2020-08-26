import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

const routes: Routes = [
  {
    path: '',
    redirectTo: 'authpolicy',
    pathMatch: 'full'
  },
  {
    path: 'authpolicy',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/admin/authpolicy/authpolicy.module').then(m => m.AuthpolicyModule)
      }
    ]
  },
  {
    path: 'users',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/admin/users/users.module').then(m => m.UsersModule)
      }
    ]
  },
  {
    path: 'upgrade',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/admin/systemupgrade/systemupgrade.module').then(m => m.SystemupgradeModule)
      }
    ]
  },
  {
    path: 'techsupport',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/admin/techsupport/techsupport.module').then(m => m.TechsupportModule)
      }
    ]
  },
  {
    path: 'snapshots',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/admin/snapshots/snapshots.module').then(m => m.SnapshotsModule)
      }
    ]
  },
  {
    path: 'certificate',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/admin/updatecluster-tls/updatecluster-tls.module').then(m => m.UpdateclusterTLSModule)
      }
    ]
  },
  {
    path: 'api',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/admin/api-sample/api-sample.module').then(m => m.ApiSampleModule)
      }
    ]
  }
  ];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class AdminRoutingModule { }
