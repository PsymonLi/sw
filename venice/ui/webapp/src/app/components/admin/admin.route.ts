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
        loadChildren: '@app/components/admin/authpolicy/authpolicy.module#AuthpolicyModule'
      }
    ]
  },
  {
    path: 'users',
    children: [
      {
        path: '',
        loadChildren: '@app/components/admin/users/users.module#UsersModule'
      }
    ]
  },
  {
    path: 'upgrade',
    children: [
      {
        path: '',
        loadChildren: '@app/components/admin/systemupgrade/systemupgrade.module#SystemupgradeModule'
      }
    ]
  },
  {
    path: 'techsupport',
    children: [
      {
        path: '',
        loadChildren: '@app/components/monitoring-group/techsupport/techsupport.module#TechsupportModule'
      }
    ]
  },
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class AdminRoutingModule { }
