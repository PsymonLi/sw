import { HttpClientTestingModule } from '@angular/common/http/testing';
import { Component } from '@angular/core';
import { async, ComponentFixture, TestBed } from '@angular/core/testing';
import { FormsModule } from '@angular/forms';
import { MatIconRegistry } from '@angular/material';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { RouterTestingModule } from '@angular/router/testing';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { LogService } from '@app/services/logging/log.service';
import { MessageService } from '@app/services/message.service';
import { MetricsqueryService } from '@app/services/metricsquery.service';
import { SharedModule } from '@components/shared/shared.module';
import { PrimengModule } from '@lib/primeng.module';
import { ConfirmationService } from 'primeng/primeng';
import { WidgetsModule } from 'web-app-framework';
import { TelemetryComponent } from './telemetry.component';
import { MaterialdesignModule } from '@app/lib/materialdesign.module';
import { ControllerService } from '@app/services/controller.service';
import { configureTestSuite } from 'ng-bullet';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { AuthService } from '@app/services/auth.service';


@Component({
  template: ''
})
class DummyComponent { }

describe('TelemetryComponent', () => {
  let component: TelemetryComponent;
  let fixture: ComponentFixture<TelemetryComponent>;

  configureTestSuite(() => {
    TestBed.configureTestingModule({
      declarations: [
        TelemetryComponent,
        DummyComponent,
      ],
      imports: [
        RouterTestingModule.withRoutes([
          { path: 'login', component: DummyComponent }
        ]),
        FormsModule,
        HttpClientTestingModule,
        WidgetsModule,
        PrimengModule,
        MaterialdesignModule,
        NoopAnimationsModule,
        SharedModule
      ],
      providers: [
        ControllerService,
        ConfirmationService,
        MatIconRegistry,
        LogService,
        LogPublishersService,
        MessageService,
        MetricsqueryService,
        UIConfigsService,
        AuthService,
      ]
    });
  });

  beforeEach(() => {
    fixture = TestBed.createComponent(TelemetryComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
