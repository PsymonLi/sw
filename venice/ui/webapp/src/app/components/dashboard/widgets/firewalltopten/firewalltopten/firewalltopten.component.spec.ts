import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { FirewalltoptenComponent } from './firewalltopten.component';
import { BasecardComponent } from '@app/components/shared/basecard/basecard.component';
import { ChartModule } from 'primeng/chart';
import { MatIconModule } from '@angular/material/icon';
import { MatMenuModule } from '@angular/material/menu';
import { DsbdwidgetheaderComponent } from '@app/components/shared/dsbdwidgetheader/dsbdwidgetheader.component';
import { SpinnerComponent } from '@app/components/shared/spinner/spinner.component';
import { PrettyDatePipe } from '@app/components/shared/Pipes/PrettyDate.pipe';
import { FwlogService } from '@app/services/generated/fwlog.service';
import { HttpClientModule } from '@angular/common/http';
import { ControllerService } from '@app/services/controller.service';
import { Router } from '@angular/router';
import { RouterTestingModule } from '@angular/router/testing';
import { LogService } from '@app/services/logging/log.service';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { MessageService } from '@app/services/message.service';
import { ConfirmationService } from 'primeng';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { AuthService } from '@app/services/auth.service';
import { LicenseService } from '@app/services/license.service';
import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
import { SecurityService } from '@app/services/generated/security.service';
import { ToprulescardComponent } from '@app/components/dashboard/widgets/firewalltopten/toprulescard/toprulescard.component';
import { SharedModule } from '@components/shared/shared.module';
import { MetricsqueryService } from '@app/services/metricsquery.service';

describe('FirewalltoptenComponent', () => {
  let component: FirewalltoptenComponent;
  let fixture: ComponentFixture<FirewalltoptenComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [
        FirewalltoptenComponent,
        ToprulescardComponent,
      ],
      imports: [
        ChartModule,
        MatIconModule,
        MatMenuModule,
        HttpClientModule,
        RouterTestingModule,
        BrowserAnimationsModule,
        SharedModule,
      ],
      providers: [
        FwlogService,
        ControllerService,
        LogService,
        LogPublishersService,
        MessageService,
        ConfirmationService,
        UIConfigsService,
        AuthService,
        LicenseService,
        SecurityService,
        MetricsqueryService,
      ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(FirewalltoptenComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
