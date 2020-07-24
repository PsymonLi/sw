import {  ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';

import { BasecardComponent } from './basecard.component';
import { WidgetsModule } from 'web-app-framework';
import { MaterialdesignModule } from '@app/lib/materialdesign.module';
import { DsbdwidgetheaderComponent } from '@app/components/shared/dsbdwidgetheader/dsbdwidgetheader.component';
import { PrettyDatePipe } from '@app/components/shared/Pipes/PrettyDate.pipe';
import { FlexLayoutModule } from '@angular/flex-layout';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { SpinnerComponent } from '../spinner/spinner.component';
import { Router } from '@angular/router';
import { NgModuleFactoryLoader } from '@angular/core';
import { HttpClient, HttpHandler } from '@angular/common/http';
import { AuthService } from '@app/services/auth.service';
import { ControllerService } from '@app/services/controller.service';
import { LicenseService } from '@app/services/license.service';
import { LogService } from '@app/services/logging/log.service';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { MessageService } from '@app/services/message.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { ConfirmationService } from 'primeng/primeng';

const mockRouter = {
  navigateByUrl: jasmine.createSpy('navigateByUrl')
};

describe('HerocardComponent', () => {
  const stats = ['first', 'second', 'third'];
  let component: BasecardComponent;
  let fixture: ComponentFixture<BasecardComponent>;
  let uiconfigService: UIConfigsService;

  configureTestSuite(() => {
     TestBed.configureTestingModule({
      declarations: [
        BasecardComponent,
        DsbdwidgetheaderComponent,
        PrettyDatePipe,
        SpinnerComponent,
      ],
      imports: [
        WidgetsModule,
        MaterialdesignModule,
        NoopAnimationsModule,
        FlexLayoutModule
      ],
      providers: [
        AuthService,
        ConfirmationService,
        ControllerService,
        HttpClient,
        HttpHandler,
        LicenseService,
        LogPublishersService,
        LogService,
        MessageService,
        NgModuleFactoryLoader,
        UIConfigsService,
        { provide: Router, useValue: mockRouter },
      ]
    }); });

  beforeEach(() => {
    uiconfigService = TestBed.get(UIConfigsService);
    spyOn(uiconfigService, 'isFeatureEnabled').and.returnValue(false);
    fixture = TestBed.createComponent(BasecardComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
