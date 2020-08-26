import { HttpClientTestingModule } from '@angular/common/http/testing';
import { async, ComponentFixture, TestBed } from '@angular/core/testing';
import { MatIconModule } from '@angular/material/icon';
import { MatMenuModule } from '@angular/material/menu';
import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
import { RouterTestingModule } from '@angular/router/testing';
import { SharedModule } from '@app/components/shared/shared.module';
import { AuthService } from '@app/services/auth.service';
import { ControllerService } from '@app/services/controller.service';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { SearchService } from '@app/services/generated/search.service';
import { LicenseService } from '@app/services/license.service';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { LogService } from '@app/services/logging/log.service';
import { MessageService } from '@app/services/message.service';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { DialogModule } from 'primeng/dialog';
import { ConfirmationService } from 'primeng';
import { RrhealthcardComponent } from './rrhealthcard.component';
import { MatTooltipModule } from '@angular/material/tooltip';

describe('RrhealthcardComponent', () => {
  let component: RrhealthcardComponent;
  let fixture: ComponentFixture<RrhealthcardComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [
        RrhealthcardComponent
      ],
      imports: [
        MatIconModule,
        MatMenuModule,
        RouterTestingModule,
        HttpClientTestingModule,
        BrowserAnimationsModule,
        DialogModule,
        SharedModule,
        MatTooltipModule,
      ],
      providers: [
        ControllerService,
        UIConfigsService,
        MonitoringService,
        SearchService,
        LogService,
        LogPublishersService,
        MessageService,
        ConfirmationService,
        AuthService,
        LicenseService,
      ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RrhealthcardComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
