import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';
import { AuthGuard } from '@app/services/authguard.service';
import { SearchresultComponent } from '@app/components/search/searchresult/searchresult.component';
import { RouteGuard } from '@app/services/routeguard.service';
import { UIConfigsResolver } from '@app/services/uiconfigs.service';
import { AppcontentComponent } from '@app/appcontent.component';
import { MissingpageComponent } from './widgets/missingpage/missingpage.component';
import { RolloutstatusComponent } from './components/admin/systemupgrade/rollouts/rolloutstatus/rolloutstatus.component';
import { LoginComponent } from './components/login/login.component';


/**
 * This is the application route configuration file.
 * We are uing lazy-loading in route configuration.
 */

export const routes: Routes = [
  {
    path: 'login',
    component: LoginComponent
  },
  {
    path: '',
    pathMatch: 'full',
    redirectTo: '/login'
  },
  {
    path: '',
    // AuthGuard will be called if someone tries to enter any of these
    // subroutes for the first time. If they switch between child routes
    // (no matter how deep), RouteGuard will be called to determine whether the
    // destination route is allowed
    canActivate: [AuthGuard],
    canActivateChild: [RouteGuard],
    component: AppcontentComponent,
    resolve: {
      configResolver: UIConfigsResolver
    },
    children: [
      {
        path: 'dashboard',
        loadChildren: () => import('@components/dashboard/dashboard.module').then(m => m.DashboardModule),
      },
      {
        path: 'workload',
        loadChildren: () => import('@components/workload/workload.module').then(m => m.WorkloadModule),
      },
      {
        path: 'network',
        loadChildren: () => import('@components/network/network.module').then(m => m.NetworkModule),
      },
      {
        path: 'monitoring',
        loadChildren: () => import('@components/monitoring-group/monitoring-group.module').then(m => m.MonitoringGroupModule),
      },
      {
        path: 'troubleshoot',
        loadChildren: () => import('@components/troubleshoot-group/troubleshoot-group.module').then(m => m.TroubleshootGroupModule),
      },
      {
        path: 'admin',
        loadChildren: () => import('@components/admin/admin.module').then(m => m.AdminModule),
      },
      {
        path: 'settings',
        loadChildren: () => import('@components/settings-group/settings-group.module').then(m => m.SettingsGroupModule),
      },
      {
        path: 'security',
        loadChildren: () => import('@components/security/security.module').then(m => m.SecurityModule),
      },
      {
        path: 'network',
        loadChildren: () => import('@components/network/network.module').then(m => m.NetworkModule),
      },
      {
        path: 'cluster',
        loadChildren: () => import('@components/cluster-group/cluster-group.module').then(m => m.ClusterGroupModule),
      },
      {
        path: 'controller',
        loadChildren: () => import('@components/controller/controller.module').then(m => m.ControllerModule),
      },
      {
        path: 'searchresult',
        component: SearchresultComponent,
      },
      {
        path: 'maintenance',
        component: RolloutstatusComponent,
      },
      {
        path: '**',
        component: MissingpageComponent
      }
    ]
  },
  {
    path: '**',
    redirectTo: '/login'
  }

];

@NgModule({
  imports: [
    RouterModule.forRoot(
      routes,
    )
  ],
  exports: [
    RouterModule
  ]
})
export class AppRoutingModule { }
