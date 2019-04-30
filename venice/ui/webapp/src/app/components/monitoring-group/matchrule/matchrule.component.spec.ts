import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { MatchruleComponent } from './matchrule.component';
import { MaterialdesignModule } from '@app/lib/materialdesign.module';
import { PrimengModule } from '@app/lib/primeng.module';
import { ReactiveFormsModule, FormsModule } from '@angular/forms';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { ControllerService } from '@app/services/controller.service';
import { MessageService } from '@app/services/message.service';
import { LogService } from '@app/services/logging/log.service';
import { ConfirmationService } from 'primeng/primeng';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { MatIconRegistry } from '@angular/material';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { RouterTestingModule } from '@angular/router/testing';
import { AuthService } from '@app/services/generated/auth.service';

describe('MatchruleComponent', () => {
  let component: MatchruleComponent;
  let fixture: ComponentFixture<MatchruleComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [MatchruleComponent],
      imports: [
        MaterialdesignModule,
        RouterTestingModule,
        PrimengModule,
        ReactiveFormsModule,
        FormsModule,
        NoopAnimationsModule,
      ],
      providers: [
        ControllerService,
        ConfirmationService,
        LogService,
        LogPublishersService,
        MatIconRegistry,
        UIConfigsService,
        MessageService,
        AuthService,
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(MatchruleComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
