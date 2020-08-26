import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { ToprulescardComponent } from './toprulescard.component';
import { SharedModule } from '@app/components/shared/shared.module';
import { ChartModule } from 'primeng/chart';
import { ControllerService } from '@app/services/controller.service';
import { RouterTestingModule } from '@angular/router/testing';
import { LogService } from '@app/services/logging/log.service';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { MessageService } from '@app/services/message.service';
import { ConfirmationService } from 'primeng';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { AuthService } from '@app/services/auth.service';
import { LicenseService } from '@app/services/license.service';
import { SecurityService } from '@app/services/generated/security.service';
import { MaterialdesignModule } from '@lib/materialdesign.module';

describe('ToprulescardComponent', () => {
  let component: ToprulescardComponent;
  let fixture: ComponentFixture<ToprulescardComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [
        ToprulescardComponent,
      ],
      imports: [
        SharedModule,
        ChartModule,
        RouterTestingModule,
        MaterialdesignModule,
      ],
      providers: [
        ControllerService,
        LogService,
        LogPublishersService,
        MessageService,
        ConfirmationService,
        UIConfigsService,
        AuthService,
        LicenseService,
        SecurityService,
      ],
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(ToprulescardComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
