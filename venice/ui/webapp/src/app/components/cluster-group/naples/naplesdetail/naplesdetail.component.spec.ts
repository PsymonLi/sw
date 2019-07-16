import {  ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';

import { ControllerService } from '@app/services/controller.service';
import { ConfirmationService } from 'primeng/primeng';
import { LogService } from '@app/services/logging/log.service';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { MatIconRegistry } from '@angular/material';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { MessageService } from '@app/services/message.service';
import { MetricsqueryService } from '@app/services/metricsquery.service';
import { ClusterService } from '@app/services/generated/cluster.service';
import { HttpClientTestingModule } from '@angular/common/http/testing';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { SharedModule } from '@app/components/shared/shared.module';
import { MaterialdesignModule } from '@app/lib/materialdesign.module';
import { PrimengModule } from '@app/lib/primeng.module';
import { ActivatedRoute } from '@angular/router';
import { BehaviorSubject, ReplaySubject, Observable } from 'rxjs';
import { RouterLinkStubDirective } from '@app/common/RouterLinkStub.directive.spec';
import { RouterTestingModule } from '@angular/router/testing';
import { AlerttableService } from '@app/services/alerttable.service';
import { SearchService } from '@app/services/generated/search.service';
import { EventsService } from '@app/services/events.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { NaplesdetailComponent } from './naplesdetail.component';
import { Utility } from '@app/common/Utility';
import { TestingUtility } from '@app/common/TestingUtility';
import { By } from '@angular/platform-browser';
import { IClusterSmartNIC, ClusterSmartNIC, ClusterSmartNICStatus_admission_phase_uihint } from '@sdk/v1/models/generated/cluster';
import { AuthService } from '@app/services/auth.service';
import { UIRolePermissions } from '@sdk/v1/models/generated/UI-permissions-enum';

class MockActivatedRoute extends ActivatedRoute {
  id = '4444.4444.0002';
  paramObserver = new BehaviorSubject<any>({ id: this.id });
  snapshot: any = { url: ['cluster', 'naples', '4444.4444.0002'] };

  constructor() {
    super();
    this.params = this.paramObserver.asObservable();
  }

  setId(id) {
    this.id = id;
    this.paramObserver.next({ id: this.id });
  }
}

describe('NaplesdetailComponent', () => {
  const _ = Utility.getLodash();

  let component: NaplesdetailComponent;
  let fixture: ComponentFixture<NaplesdetailComponent>;
  let testingUtility: TestingUtility;
  let naplesWatchSpy;
  let naplesGetSpy;
  let naplesObserver;
  let naples1;
  let naples2;
  let naples3;

  function verifyMeta(naples: IClusterSmartNIC) {
    const fields = fixture.debugElement.queryAll(By.css('.naplesdetail-node-value'));
    expect(fields.length).toBe(8); // there are 8 columns defined in naplesdetail.c.ts
    if (naples.status['primary-mac'] != null) {
      expect(fields[0].nativeElement.textContent).toContain(naples.status['primary-mac']);
    } else {
      expect(fields[0].nativeElement.textContent.trim()).toBe('');
    }
    if (naples.spec['ip-config'] != null && naples.spec['ip-config']['ip-address'] != null) {
      expect(fields[1].nativeElement.textContent).toContain(naples.spec['ip-config']['ip-address']);
    } else {
      expect(fields[1].nativeElement.textContent.trim()).toBe('');
    }
    if (naples.spec['id'] != null) {
      expect(fields[2].nativeElement.textContent).toContain(naples.spec['id']);
    } else {
      expect(fields[2].nativeElement.textContent.trim()).toBe('');
    }
    if (naples.status['admission-phase'] != null) {
      expect(fields[3].nativeElement.textContent).toContain(ClusterSmartNICStatus_admission_phase_uihint[naples.status['admission-phase']]);
      const icon = fields[3].query(By.css('.naplesdetail-phase-icon'));
      if (naples.status['admission-phase'] !== 'ADMITTED') {
        expect(icon).toBeTruthy();
        if (naples.status['admission-phase'] === 'PENDING') {
          expect(icon.nativeElement.textContent).toContain('notifications');
        } else if (naples.status['admission-phase'] !== 'REJECTED') {
          expect(icon.nativeElement.textContent).toContain('error');
        }
      } else {
        expect(icon).toBeNull();
      }
    } else {
      expect(fields[3].nativeElement.textContent.trim()).toBe('');
    }
    if (naples.status['serial-num'] != null) {
      expect(fields[5].nativeElement.textContent).toContain(naples.status['smartNicVersion']);
    } else {
      expect(fields[5].nativeElement.textContent.trim()).toBe('');
    }
  }

  function verifyServiceCalls(naplesName) {
    expect(naplesWatchSpy).toHaveBeenCalled();
    const calledObj = naplesWatchSpy.calls.mostRecent().args[0];
    expect(_.isEqual({ 'field-selector': 'meta.name=' + naplesName }, calledObj)).toBeTruthy('Incorrect selector for ' + naplesName);

    expect(naplesGetSpy).toHaveBeenCalled();
    expect(naplesGetSpy).toHaveBeenCalledWith(naplesName + ':');
  }

  function getOverlay() {
    return fixture.debugElement.query(By.css('.naplesdetail-overlay'));
  }

  function getMissingPolicyIcon() {
    return fixture.debugElement.query(By.css('.naplesdetail-missing-node'));
  }

  function getDeletedPolicyIcon() {
    return fixture.debugElement.query(By.css('.naplesdetail-deleted-node'));
  }

  function getOverlayText() {
    return fixture.debugElement.query(By.css('.naplesdetail-overlay-text'));
  }

  function getOverlayButtons() {
    return fixture.debugElement.queryAll(By.css('.naplesdetail-overlay-button'));
  }

  configureTestSuite(() => {
     TestBed.configureTestingModule({
      declarations: [NaplesdetailComponent, RouterLinkStubDirective],
      imports: [
        RouterTestingModule,
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
        MatIconRegistry,
        UIConfigsService,
        MessageService,
        ClusterService,
        AlerttableService,
        MetricsqueryService,
        SearchService,
        EventsService,
        AuthService,
        MonitoringService,
        {
          provide: ActivatedRoute,
          useClass: MockActivatedRoute
        },
        MessageService
      ]
    });
      });

  beforeEach(() => {
    fixture = TestBed.createComponent(NaplesdetailComponent);
    component = fixture.componentInstance;
    testingUtility = new TestingUtility(fixture);
    const clusterService = TestBed.get(ClusterService);

    naples1 = {
      'kind': 'SmartNIC',
      'api-version': 'v1',
      'meta': {
        'name': '4444.4444.0002',
        'generation-id': '1',
        'resource-version': '34507',
        'uuid': '11b1912f-c513-4f19-8183-5e17e60f024f',
        'creation-time': '2018-12-12T20:03:01.157939359Z',
        'mod-time': '2018-12-13T01:21:16.38343108Z',
        'self-link': '/configs/cluster/v1/smartnics/4444.4444.0002'
      },
      'spec': {
        'admit': true,
        'id': 'naples1-host',
        'mgmt-mode': 'NETWORK'
      },
      'status': {
        'admission-phase': 'ADMITTED',
        'host': 'naples-host-1',
        'conditions': [
          {
            'type': 'HEALTHY',
            'status': 'TRUE',
            'last-transition-time': '2018-12-13T01:21:16Z'
          }
        ],
        'serial-num': '0x0123456789ABCDEFghijk',
        'primary-mac': '4444.4444.0002',
        'smartNicVersion': '1.0E'
      }
    };

    naples2 = new ClusterSmartNIC({
      'kind': 'SmartNIC',
      'api-version': 'v1',
      'meta': {
        'name': '3333.3333.0002',
        'generation-id': '1',
        'resource-version': '34507',
        'uuid': '11b1912f-c513-4f19-8183-5e17e60f024f',
        'creation-time': '2018-12-12T20:03:01.157939359Z',
        'mod-time': '2018-12-13T01:21:16.38343108Z',
        'self-link': '/configs/cluster/v1/smartnics/4444.4444.0002'
      },
      'spec': {},
      'status': {
        'host': 'naples-host-1',
        'admission-phase': 'PENDING'
      }
    });

    naples3 = new ClusterSmartNIC({
      'kind': 'SmartNIC',
      'api-version': 'v1',
      'meta': {
        'name': '3333.3333.0003',
        'generation-id': '1',
        'resource-version': '34507',
        'uuid': '11b1912f-c513-4f19-8183-5e17e60f024f',
        'creation-time': '2018-12-12T20:03:01.157939359Z',
        'mod-time': '2018-12-13T01:21:16.38343108Z',
        'self-link': '/configs/cluster/v1/smartnics/4444.4444.0002'
      },
      'spec': {},
      'status': {
        'host': 'naples-host-3',
        'admission-phase': 'REJECTED'
      }
    });

    naplesObserver = new BehaviorSubject({
      events: [
        {
          type: 'Created',
          object: naples1
        }
      ]
    });
    naplesWatchSpy = spyOn(clusterService, 'WatchSmartNIC').and.returnValue(
      naplesObserver
    );
    naplesGetSpy = spyOn(clusterService, 'GetSmartNIC').and.returnValue(
      new BehaviorSubject({
        body: naples2
      })
    );
  });

  afterEach(() => {
    TestBed.resetTestingModule();
  });

  it('should display missing policy overlay and deleted policy overlay', () => {
    // change param id
    const mockActivatedRoute: MockActivatedRoute = TestBed.get(ActivatedRoute);
    mockActivatedRoute.setId('3333.3333.0002');
    const policyWatchObserver = new ReplaySubject();
    const policyGetObserver = new Observable((observable) => {
      observable.error({ body: null, statusCode: 400 });
    });
    naplesWatchSpy.and.returnValue(
      policyWatchObserver
    );
    naplesGetSpy.and.returnValue(
      policyGetObserver
    );

    fixture.detectChanges();
    verifyServiceCalls('3333.3333.0002');

    // View should now be of missing overlay, and data should be cleared
    expect(getOverlay()).toBeTruthy();
    expect(getMissingPolicyIcon()).toBeTruthy();
    expect(getOverlayText().nativeElement.textContent).toContain('3333.3333.0002 does not exist');
    let buttons = getOverlayButtons();
    expect(buttons.length).toBe(2);
    expect(buttons[0].nativeElement.textContent).toContain('NAPLES OVERVIEW');
    expect(buttons[1].nativeElement.textContent).toContain('HOMEPAGE');

    // Add object
    policyWatchObserver.next({
      events: [
        {
          type: 'Created',
          object: naples2
        }
      ]
    });

    fixture.detectChanges();
    // overlay should be gone
    expect(getOverlay()).toBeNull();
    verifyMeta(naples2);

    // Delete policy
    policyWatchObserver.next({
      events: [
        {
          type: 'Deleted',
          object: naples2
        }
      ]
    });

    fixture.detectChanges();
    expect(getOverlay()).toBeTruthy();
    expect(getDeletedPolicyIcon()).toBeTruthy();
    expect(getOverlayText().nativeElement.textContent).toContain('3333.3333.0002 has been deleted');
    buttons = getOverlayButtons();
    expect(buttons.length).toBe(2);
    expect(buttons[0].nativeElement.textContent).toContain('NAPLES OVERVIEW');
    expect(buttons[1].nativeElement.textContent).toContain('HOMEPAGE');

    // Clicking homepage button
    spyOn(component, 'routeToHomepage');
    testingUtility.sendClick(buttons[1]);
    expect(component.routeToHomepage).toHaveBeenCalled();

    // find DebugElements with an attached RouterLinkStubDirective
    const linkDes = fixture.debugElement
      .queryAll(By.directive(RouterLinkStubDirective));

    // get attached link directive instances
    // using each DebugElement's injector
    const routerLinks = linkDes.map(de => de.injector.get(RouterLinkStubDirective));
    expect(routerLinks.length).toBe(2, 'should have 2 routerLinks');
    expect(routerLinks[1].linkParams).toBe('../'); // VS-639

    testingUtility.sendClick(buttons[0]);
    expect(routerLinks[1].navigatedTo).toBe('../'); // VS-639

  });



  it('should rerender when user navigates to same page with different id and use field selectors', () => {
    fixture.detectChanges();
    verifyMeta(naples1);
    verifyServiceCalls('4444.4444.0002');

    // change param id
    let mockActivatedRoute: MockActivatedRoute = TestBed.get(ActivatedRoute);
    naplesWatchSpy.and.returnValue(
      new BehaviorSubject({
        events: [
          {
            type: 'Created',
            object: naples3
          }
        ]
      })
    );
    mockActivatedRoute.setId('3333.3333.0003');


    // View should now be of naples3
    fixture.detectChanges();
    verifyMeta(naples3);
    verifyServiceCalls('3333.3333.0003');

    // change param id
    naplesWatchSpy.and.returnValue(
      new BehaviorSubject({
        events: [
          {
            type: 'Created',
            object: naples2
          }
        ]
      })
    );
    mockActivatedRoute.setId('3333.3333.0002');


    // View should now be of naples2
    fixture.detectChanges();
    verifyMeta(naples2);
    verifyServiceCalls('3333.3333.0002');

    // change param id
    const policyWatchObserver = new ReplaySubject();
    const policyGetObserver = new Observable((observable) => {
      observable.error({ body: null, statusCode: 400 });
    });
    naplesWatchSpy.and.returnValue(
      policyWatchObserver
    );
    naplesGetSpy.and.returnValue(
      policyGetObserver
    );
    mockActivatedRoute = TestBed.get(ActivatedRoute);
    mockActivatedRoute.setId('policy3');

    verifyServiceCalls('policy3');

    // View should now be of missing overlay, and data should be cleared
    fixture.detectChanges();
    const emptyNaples = new ClusterSmartNIC();
    emptyNaples.status['admission-phase'] = null;
    verifyMeta(emptyNaples);
    expect(getOverlay()).toBeTruthy();

  });

  describe('RBAC', () => {
    it('no permission', () => {
    fixture.detectChanges();
      // metrics should be visible
      const cards = fixture.debugElement.queryAll(By.css('app-herocard'));
      expect(cards.length).toBe(3);
    });

  });

});
