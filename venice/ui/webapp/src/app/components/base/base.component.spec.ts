import { async, ComponentFixture, TestBed } from '@angular/core/testing';
import { RouterTestingModule } from '@angular/router/testing';
import { HttpClientTestingModule } from '@angular/common/http/testing';

import { MatIconRegistry } from '@angular/material';
import { ControllerService } from '../../services/controller.service';
import { BaseComponent } from './base.component';
import { LogService } from '@app/services/logging/log.service';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { ConfirmationService } from 'primeng/primeng';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { MessageService } from '@app/services/message.service';
import { AuthService } from '@app/services/auth.service';

describe('BaseComponent', () => {
  let component: BaseComponent;
  let fixture: ComponentFixture<BaseComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [BaseComponent],
      imports: [
        RouterTestingModule,
        HttpClientTestingModule,
      ],
      providers: [
        ControllerService,
        ConfirmationService,
        LogService,
        LogPublishersService,
        MatIconRegistry,
        UIConfigsService,
        MessageService,
        AuthService
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(BaseComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
