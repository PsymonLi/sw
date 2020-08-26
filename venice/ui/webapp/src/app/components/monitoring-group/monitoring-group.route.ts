import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';

const routes: Routes = [
  {
    path: '',
    redirectTo: 'alertsevents',
    pathMatch: 'full'
  },
  {
    path: 'alertsevents',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/monitoring-group/alertsevents/alertseventspage.module').then(m => m.AlertsEventsPageModule)
      },
      {
        path: 'alert/:alertname',
        loadChildren: () => import('@app/components/monitoring-group/alertsevents/alertseventspage.module').then(m => m.AlertsEventsPageModule)
      },
      {
        path: 'event/:eventname',
        loadChildren: () => import('@app/components/monitoring-group/alertsevents/alertseventspage.module').then(m => m.AlertsEventsPageModule)
      },
      {
        path: 'alert',
        loadChildren: () => import('@app/components/monitoring-group/alertsevents/alertseventspage.module').then(m => m.AlertsEventsPageModule)
      },
      {
        path: 'event',
        loadChildren: () => import('@app/components/monitoring-group/alertsevents/alertseventspage.module').then(m => m.AlertsEventsPageModule)
      }
    ]
  },
  {
    path: 'fwlogs',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/monitoring-group/fwlogs/fwlogs.module').then(m => m.FwlogsModule)
      }
    ]
  },
  {
    path: 'archive',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/monitoring-group/archivelogs/archivelog.module').then(m => m.ArchivelogModule)
      }
    ]
  },
  {
    path: 'auditevents',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/monitoring-group/auditevents/auditevents.module').then(m => m.AuditeventsModule)
      }
    ]
  },
  {
    path: 'metrics',
    children: [
      {
        path: '',
        loadChildren: () => import('@app/components/monitoring-group/telemetry/telemetry.module').then(m => m.TelemetryModule)
      }
    ]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class MonitoringRoutingModule { }
