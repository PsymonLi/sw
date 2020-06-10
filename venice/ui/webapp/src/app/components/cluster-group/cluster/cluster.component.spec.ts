/**-----
 Angular imports
 ------------------*/
import {  ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';
import { By } from '@angular/platform-browser';
import { RouterTestingModule } from '@angular/router/testing';
import { HttpClientTestingModule } from '@angular/common/http/testing';
import { Component, DebugElement } from '@angular/core';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { MatIconRegistry } from '@angular/material';


/**-----
 Venice UI -  imports
 ------------------*/
import { ClusterComponent } from './cluster.component';
import { ControllerService } from '@app/services/controller.service';
import { ConfirmationService } from 'primeng/primeng';
import { ClusterService } from '@app/services/generated/cluster.service';
import { NetworkService } from '@app/services/generated/network.service';
import { MaterialdesignModule } from '@lib/materialdesign.module';
import { SharedModule } from '@app/components/shared/shared.module';
import { PrimengModule } from '@app/lib/primeng.module';
import { LogService } from '@app/services/logging/log.service';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { ClusterCluster, ClusterNode } from '@sdk/v1/models/generated/cluster';
import { EventsService } from '@app/services/events.service';
import { AlerttableService } from '@app/services/alerttable.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { LicenseService } from '@app/services/license.service';
import { SearchService } from '@app/services/generated/search.service';
import { BehaviorSubject } from 'rxjs';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { MessageService } from '@app/services/message.service';
import { MetricsqueryService } from '@app/services/metricsquery.service';
import { DiagnosticsService } from '@app/services/generated/diagnostics.service';

import { TestingUtility } from '@app/common/TestingUtility';
import {FormsModule, ReactiveFormsModule} from '@angular/forms';
import { AuthService } from '@app/services/auth.service';
import { AuthService as AuthServiceGen } from '@app/services/generated/auth.service';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';
import { ClusterupdateComponent } from './clusterupdate/clusterupdate.component';

@Component({
  template: ''
})
class DummyComponent { }

describe('ClusterComponent', () => {
  let component: ClusterComponent;
  let fixture: ComponentFixture<ClusterComponent>;

  configureTestSuite(() => {
     TestBed.configureTestingModule({
      declarations: [
        ClusterComponent,
        DummyComponent,
        ClusterupdateComponent
      ],
      imports: [
        RouterTestingModule.withRoutes([
          { path: 'login', component: DummyComponent }
        ]),
        FormsModule,
        ReactiveFormsModule,
        HttpClientTestingModule,
        NoopAnimationsModule,
        SharedModule,
        MaterialdesignModule,
        PrimengModule,
      ],
      providers: [
        ControllerService,
        ConfirmationService,
        LogService,
        LogPublishersService,
        ClusterService,
        NetworkService,
        MatIconRegistry,
        AlerttableService,
        EventsService,
        UIConfigsService,
        LicenseService,
        SearchService,
        MonitoringService,
        MessageService,
        MetricsqueryService,
        MessageService,
        AuthService,
        AuthServiceGen,
        DiagnosticsService
      ]
    });
      });

  beforeEach(() => {
    fixture = TestBed.createComponent(ClusterComponent);
    component = fixture.componentInstance;
    component.cluster = new ClusterCluster({ 'kind': 'Cluster', 'api-version': 'v1', 'meta': { 'name': 'testCluster', 'tenant': 'default', 'namespace': 'default', 'resource-version': '454', 'uuid': '88f8baa7-7a8e-4a2a-9780-c23d2a217b5d', 'creation-time': '2018-06-13T17:50:29.117538562Z', 'mod-time': '2018-06-13T17:50:44.783819163Z', 'self-link': '/v1/cluster/cluster/testCluster' }, 'spec': { 'quorum-nodes': ['node1', 'node2', 'node3'], 'virtual-ip': '192.168.30.10', 'ntp-servers': ['1.pool.ntp.org', '2.pool.ntp.org'], 'auto-admit-dscs': true }, 'status': { 'leader': 'node1', 'last-leader-transition-time': '2018-06-13T17:50:44.783544086Z' } });
    const events = [];

    const node5 = new ClusterNode({ 'kind': 'Node', 'api-version': 'v1', 'meta': { 'name': 'node5', 'tenant': 'default', 'resource-version': '6957', 'uuid': '199e7e27-a191-4332-9e28-e3c1f4445ddf', 'creation-time': '2018-06-13T18:22:30.542580696Z', 'mod-time': '2018-06-13T18:22:30.607773739Z', 'self-link': '/v1/cluster/nodes/node5' }, 'spec': {}, 'status': { 'phase': 'joined', 'quorum': false } });
    events.push({ type: 'Created', object: node5 });

    const node4 = new ClusterNode({ 'kind': 'Node', 'api-version': 'v1', 'meta': { 'name': 'node4', 'tenant': 'default', 'resource-version': '6918', 'uuid': 'ae07bb01-b50c-4941-9be8-133766b3725e', 'creation-time': '2018-06-13T18:22:28.189909722Z', 'mod-time': '2018-06-13T18:22:28.25393029Z', 'self-link': '/v1/cluster/nodes/node4' }, 'spec': {}, 'status': { 'phase': 'joined', 'quorum': false } });
    events.push({ type: 'Created', object: node4 });

    const node3 = new ClusterNode({ 'kind': 'Node', 'api-version': 'v1', 'meta': { 'name': 'node3', 'tenant': 'default', 'namespace': 'default', 'resource-version': '5', 'uuid': 'cdf2c24c-3afb-417f-a615-6a4cbd77f2c4', 'creation-time': '2018-06-13T17:50:29.117712981Z', 'mod-time': '2018-06-13T17:50:29.117712981Z', 'self-link': '/v1/cluster/nodes/node3' }, 'spec': {}, 'status': { 'phase': 'joined', 'quorum': true } });
    events.push({ type: 'Created', object: node3 });

    const node2 = new ClusterNode({ 'kind': 'Node', 'api-version': 'v1', 'meta': { 'name': 'node2', 'tenant': 'default', 'namespace': 'default', 'resource-version': '5', 'uuid': '1d96fc5c-7701-4d8d-ada8-3ec160a50138', 'creation-time': '2018-06-13T17:50:29.117702613Z', 'mod-time': '2018-06-13T17:50:29.117702613Z', 'self-link': '/v1/cluster/nodes/node2' }, 'spec': {}, 'status': { 'phase': 'joined', 'quorum': true } });
    events.push({ type: 'Created', object: node2 });

    const node1 = new ClusterNode({ 'kind': 'Node', 'api-version': 'v1', 'meta': { 'name': 'node1', 'tenant': 'default', 'namespace': 'default', 'resource-version': '5', 'uuid': '6a000b60-949b-4338-bd7e-8e750a9a8edb', 'creation-time': '2018-06-13T17:50:29.117624431Z', 'mod-time': '2018-06-13T17:50:29.117624431Z', 'self-link': '/v1/cluster/nodes/node1' }, 'spec': {}, 'status': { 'phase': 'joined', 'quorum': true } });
    events.push({ type: 'Created', object: node1 });

    const service = TestBed.get(ClusterService);
    const ret = {
      events: events
    };
    spyOn(service, 'WatchNode').and.returnValue(
      new BehaviorSubject(ret)
    );
  });

  it('cluster should have right values', () => {
    TestingUtility.setAllPermissions();
    fixture.detectChanges();
    // Cluster icon should be visible
    const clusterDe: DebugElement = fixture.debugElement;
    const clusterHe: HTMLElement = clusterDe.nativeElement;
    expect(clusterDe.queryAll(By.css('.pagebody-icon')).length).toBe(1, 'More than pagebody icon was present');
    expect(clusterHe.querySelector('div.pagebody-headercontent').textContent).toContain('Cluster Overview', 'header title was not cluster overview');

    // Cluster values
    expect(
      TestingUtility.isDateDisplayCorrect('2018-06-13T17:50:44.783544086Z',
        clusterDe.query(By.css('div.cluster-details-panel-last-leader-transition-time'))
      ))
      .toBeTruthy();
    expect(clusterHe.querySelector('div.cluster-details-panel-cluster-name').textContent).toContain('testCluster');
    expect(clusterHe.querySelector('div.cluster-details-panel-ntp-servers').textContent).toContain('1.pool.ntp.org2.pool.ntp.org');
    // expect(clusterHe.querySelector('div.cluster-details-panel-virtual-ip').textContent).toContain('192.168.30.10');
    expect(clusterHe.querySelector('div.cluster-details-panel-auto-admit-nics').textContent).toContain('yes');
  });

  describe('RBAC', () => {
      let toolbarSpy: jasmine.Spy;

      beforeEach(() => {
        TestingUtility.removeAllPermissions();
        const controllerService = TestBed.get(ControllerService);
        toolbarSpy = spyOn(controllerService, 'setToolbarData');
      });

      it('alertsevents table', () => {
        // both permissions
        TestingUtility.addPermissions([UIRolePermissions.monitoringalert_read, UIRolePermissions.eventsevent_read]);
        fixture.detectChanges();
        expect(fixture.debugElement.queryAll(By.css('app-shared-alertsevents')).length).toBe(1, 'alertsevents table should be present');

        TestingUtility.removeAllPermissions();
        TestingUtility.updateRoleGuards();
        fixture.detectChanges();

        // no permissions
        expect(fixture.debugElement.queryAll(By.css('app-shared-alertsevents')).length).toBe(0, 'alertsevents table should not be present');

        TestingUtility.addPermissions([UIRolePermissions.eventsevent_read]);
        TestingUtility.updateRoleGuards();
        fixture.detectChanges();

        // events only
        expect(fixture.debugElement.queryAll(By.css('app-shared-alertsevents')).length).toBe(1, 'alertsevents table should be present');

        TestingUtility.removeAllPermissions();
        TestingUtility.addPermissions([UIRolePermissions.monitoringalert_read]);
        TestingUtility.updateRoleGuards();
        fixture.detectChanges();

        // alerts only
        expect(fixture.debugElement.queryAll(By.css('app-shared-alertsevents')).length).toBe(1, 'alertsevents table should be present');
      });
  });


});
