import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';

const routes: Routes = [
  {
    path: '',
    redirectTo: 'flowexport',
    pathMatch: 'full'
  },
  {
    path: 'flowexport',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/troubleshoot-group/flowexport/flowexport.module').then(m => m.FlowexportModule)
      }
    ]
  },
  {
    path: 'troubleshooting',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/troubleshoot-group/troubleshooting/troubleshooting.module').then(m => m.TroubleshootingModule)
      }
    ]
  },
  {
    path: 'mirrorsessions',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/troubleshoot-group/mirrorsessions/mirrorsessions.module').then(m => m.MirrorsessionsModule)
      }
    ]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class TroubleshootRoutingModule { }
