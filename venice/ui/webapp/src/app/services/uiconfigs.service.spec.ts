import { TestBed, inject } from '@angular/core/testing';

import { UIConfigsService } from './uiconfigs.service';
import { RouterTestingModule } from '@angular/router/testing';
import { HttpClientTestingModule } from '@angular/common/http/testing';
import { ControllerService } from './controller.service';
import { ConfirmationService } from 'primeng/api';
import { LogService } from './logging/log.service';
import { LogPublishersService } from './logging/log-publishers.service';
import { MatIconRegistry } from '@angular/material';
import { MessageService } from './message.service';
import { AuthService } from './generated/auth.service';

describe('UiconfigsService', () => {
  beforeEach(() => {
    TestBed.configureTestingModule({
      imports: [
        HttpClientTestingModule,
        RouterTestingModule
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
    });
  });

  it('should be created', inject([UIConfigsService], (service: UIConfigsService) => {
    expect(service).toBeTruthy();
  }));
});
