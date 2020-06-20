import {  ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';

import { HostCardComponent } from './hostcard.component';
import { PrimengModule } from '@app/lib/primeng.module';
import { MaterialdesignModule } from '@app/lib/materialdesign.module';
import { RouterTestingModule } from '@angular/router/testing';
import { HttpClientTestingModule } from '@angular/common/http/testing';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { SharedModule } from '@app/components/shared/shared.module';
import { ControllerService } from '@app/services/controller.service';
import { ClusterService } from '@app/services/generated/cluster.service';
import { MessageService } from '@app/services/message.service';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { LogService } from '@app/services/logging/log.service';
import { ConfirmationService } from 'primeng/api';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { LicenseService } from '@app/services/license.service';
import { AuthService } from '@app/services/auth.service';

describe('HostCardComponent', () => {
  let component: HostCardComponent;
  let fixture: ComponentFixture<HostCardComponent>;

  configureTestSuite(() => {
     TestBed.configureTestingModule({
      declarations: [HostCardComponent],
      imports: [
        PrimengModule,
        MaterialdesignModule,
        RouterTestingModule,
        HttpClientTestingModule,
        NoopAnimationsModule,
        SharedModule
      ],
      providers: [
        ControllerService,
        ClusterService,
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

  beforeEach(() => {
    fixture = TestBed.createComponent(HostCardComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
