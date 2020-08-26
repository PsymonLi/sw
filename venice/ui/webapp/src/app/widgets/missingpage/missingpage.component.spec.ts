import {  ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';

import { MissingpageComponent } from './missingpage.component';
import { ControllerService } from '@app/services/controller.service';
import { ConfirmationService } from 'primeng';
import { SharedModule } from '@app/components/shared/shared.module';
import { MatIconRegistry } from '@angular/material/icon';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { LogService } from '@app/services/logging/log.service';
import { RouterTestingModule } from '@angular/router/testing';
import { MessageService } from '@app/services/message.service';

describe('MissingpageComponent', () => {
  let component: MissingpageComponent;
  let fixture: ComponentFixture<MissingpageComponent>;

  configureTestSuite(() => {
     TestBed.configureTestingModule({
      declarations: [MissingpageComponent],
      imports: [
        SharedModule,
        RouterTestingModule,
      ],
      providers: [
        ControllerService,
        ConfirmationService,
        LogService,
        LogPublishersService,
        MatIconRegistry,
        MessageService
      ]
    });
      });

  beforeEach(() => {
    fixture = TestBed.createComponent(MissingpageComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
