import { Component } from '@angular/core';
import {  ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';
import { FormsModule } from '@angular/forms';
import { MatIconRegistry } from '@angular/material/icon';
import { RouterTestingModule } from '@angular/router/testing';
import { SharedModule } from '@app/components/shared/shared.module';
import { PrimengModule } from '@app/lib/primeng.module';
import { ControllerService } from '@app/services/controller.service';
import { ConfirmationService } from 'primeng';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { LogService } from '@app/services/logging/log.service';
import { MessageService } from '@app/services/message.service';
import { WidgetsModule } from 'web-app-framework';
import { SearchresultComponent } from './searchresult.component';


@Component({
  template: ''
})
class DummyComponent { }

describe('SearchresultComponent', () => {
  let component: SearchresultComponent;
  let fixture: ComponentFixture<SearchresultComponent>;

  configureTestSuite(() => {
    TestBed.configureTestingModule({
      declarations: [SearchresultComponent, DummyComponent
      ],
      imports: [
        RouterTestingModule.withRoutes([
          { path: 'login', component: DummyComponent }
        ]),
        FormsModule,
        SharedModule,
        WidgetsModule,
        PrimengModule
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
    fixture = TestBed.createComponent(SearchresultComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
