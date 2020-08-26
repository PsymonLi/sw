import { HttpClientTestingModule } from '@angular/common/http/testing';
import { Component } from '@angular/core';
import {  ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';
import { RouterTestingModule } from '@angular/router/testing';
import { DsbdwidgetheaderComponent } from '@app/components/shared/dsbdwidgetheader/dsbdwidgetheader.component';
import { MaterialdesignModule } from '@app/lib/materialdesign.module';
import { PrimengModule } from '@app/lib/primeng.module';
import { ControllerService } from '@app/services/controller.service';
import { ConfirmationService } from 'primeng';
import { WorkloadService } from '@app/services/workload.service';
import { WidgetsModule } from 'web-app-framework';

import { DsbdworkloadComponent } from './dsbdworkload.component';
import { LogService } from '@app/services/logging/log.service';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { PrettyDatePipe } from '@app/components/shared/Pipes/PrettyDate.pipe';
import { MessageService } from '@app/services/message.service';

@Component({
  template: ''
})
class DummyComponent { }

describe('DsbdworkloadComponent', () => {
  let component: DsbdworkloadComponent;
  let fixture: ComponentFixture<DsbdworkloadComponent>;

  configureTestSuite(() => {
     TestBed.configureTestingModule({
      declarations: [DsbdworkloadComponent, DsbdwidgetheaderComponent, DummyComponent, PrettyDatePipe],
      imports: [
        RouterTestingModule.withRoutes([
          { path: 'login', component: DummyComponent }
        ]),
        WidgetsModule,
        PrimengModule,
        MaterialdesignModule,
        HttpClientTestingModule
      ],
      providers: [
        ControllerService,
        ConfirmationService,
        LogService,
        LogPublishersService,
        WorkloadService,
        MessageService
      ]
    });
      });

  beforeEach(() => {
    fixture = TestBed.createComponent(DsbdworkloadComponent);
    component = fixture.componentInstance;
    component.id = 'test_id';
    component.background_img = {
      url: 'test.com'
    };
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
