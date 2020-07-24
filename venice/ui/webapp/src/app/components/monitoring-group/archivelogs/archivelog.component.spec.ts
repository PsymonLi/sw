/**-----
 Angular imports
 ------------------*/
import { HttpClientTestingModule } from '@angular/common/http/testing';
import { Component } from '@angular/core';
import {  ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';
import { MatIconRegistry } from '@angular/material';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { RouterTestingModule } from '@angular/router/testing';
import { SharedModule } from '@app/components/shared/shared.module';
/**-----
 Venice web-app imports
 ------------------*/
import { ControllerService } from '@app/services/controller.service';
import { ConfirmationService } from 'primeng/primeng';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { LogService } from '@app/services/logging/log.service';
import { MaterialdesignModule } from '@lib/materialdesign.module';
import { PrimengModule } from '@lib/primeng.module';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { LicenseService } from '@app/services/license.service';
import { EventsService } from '@app/services/events.service';
import { SearchService } from '@app/services/generated/search.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { MessageService } from '@app/services/message.service';
import { WidgetsModule } from 'web-app-framework';

import { ArchivelogComponent } from './archivelog.component';
import { AuthService } from '@app/services/auth.service';

describe('ArchivelogComponent', () => {
  let component: ArchivelogComponent;
  let fixture: ComponentFixture<ArchivelogComponent>;

  configureTestSuite(() => {
    TestBed.configureTestingModule({
      declarations: [ArchivelogComponent],
      imports: [
        FormsModule,
        ReactiveFormsModule,
        NoopAnimationsModule,
        SharedModule,
        HttpClientTestingModule,
        PrimengModule,
        MaterialdesignModule,
        RouterTestingModule,
        WidgetsModule
      ],
      providers: [
        ControllerService,
        UIConfigsService,
        LicenseService,
        AuthService,
        ConfirmationService,
        LogService,
        LogPublishersService,
        MatIconRegistry,
        AuthService,
        EventsService,
        SearchService,
        MonitoringService,
        MessageService
      ]
    });
      });

  beforeEach(() => {
    fixture = TestBed.createComponent(ArchivelogComponent);
    component = fixture.componentInstance;
  });

  it('should create', () => {
    fixture.detectChanges();
    expect(component).toBeTruthy();
  });

});
