import { async, ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';
import { RrstatusComponent } from './rrstatus.component';
import { ClusterService } from '@app/services/generated/cluster.service';
import { ControllerService } from '@app/services/controller.service';
import { MatIconRegistry } from '@angular/material';
import { MaterialdesignModule } from '@app/lib/materialdesign.module';
import { HttpClientTestingModule } from '@angular/common/http/testing';
import { RouterTestingModule } from '@angular/router/testing';
import { MessageService } from '@app/services/message.service';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { LogService } from '@app/services/logging/log.service';
import { ConfirmationService } from 'primeng/api';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { LicenseService } from '@app/services/license.service';
import { AuthService } from '@app/services/auth.service';
import { RoutingHealth } from '@sdk/v1/models/generated/routing';
import { PrettyDatePipe } from '@app/components/shared/Pipes/PrettyDate.pipe.js';


describe('RrstatusComponent', () => {
  let component: RrstatusComponent;
  let fixture: ComponentFixture<RrstatusComponent>;
  const health =  {
    'kind': '',
    'meta': {
      'name': '',
      'generation-id': '',
      'creation-time': '',
      'mod-time': ''
    },
    'spec': {},
    'status': {
      'router-id': '192.168.30.12',
      'internal-peers': {
        'configured': 2,
        'established': 2
      },
      'external-peers': {
        'configured': 3,
        'established': 0
      },
      'unexpected-peers': 0
    }
  };
  configureTestSuite(() => {
    TestBed.configureTestingModule({
      declarations: [
        RrstatusComponent,
        PrettyDatePipe
      ],
      imports: [
        RouterTestingModule.withRoutes([]),
        HttpClientTestingModule,
        MaterialdesignModule,
      ],
      providers: [
        ControllerService,
        ClusterService,
        MatIconRegistry,
        LogService,
        LogPublishersService,
        MessageService,
        ConfirmationService,
        UIConfigsService,
        LicenseService,
        AuthService
      ]
    });
  });
  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ RrstatusComponent ]
    })
    .compileComponents();
  }));
  beforeEach(() => {
    fixture = TestBed.createComponent(RrstatusComponent);
    component = fixture.componentInstance;
    component.nodeHealth = new RoutingHealth(health);
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
