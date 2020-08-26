/**-----
 Angular imports
 ------------------*/
import {  ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';
import { By } from '@angular/platform-browser';
import { RouterTestingModule } from '@angular/router/testing';
import { HttpClientTestingModule } from '@angular/common/http/testing';
import { Component, DebugElement } from '@angular/core';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { MatIconRegistry } from '@angular/material/icon';

/**-----
 Venice UI -  imports
 ------------------*/
import { ControllerService } from '@app/services/controller.service';
import { ConfirmationService } from 'primeng';
import { ClusterService } from '@app/services/generated/cluster.service';
import { MaterialdesignModule } from '@lib/materialdesign.module';
import { SharedModule } from '@app/components/shared/shared.module';
import { PrimengModule } from '@app/lib/primeng.module';
import { LogService } from '@app/services/logging/log.service';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';

import { SearchsuggestionsComponent } from './searchsuggestions.component';
import { MessageService } from '@app/services/message.service';

@Component({
  template: ''
})
class DummyComponent { }

describe('SearchsuggestionsComponent', () => {
  let component: SearchsuggestionsComponent;
  let fixture: ComponentFixture<SearchsuggestionsComponent>;

  configureTestSuite(() => {
    TestBed.configureTestingModule({
      declarations: [SearchsuggestionsComponent, DummyComponent],
      imports: [
        RouterTestingModule.withRoutes([
          { path: 'login', component: DummyComponent }
        ]),
        HttpClientTestingModule,
        NoopAnimationsModule,
        SharedModule,
        MaterialdesignModule,
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
    fixture = TestBed.createComponent(SearchsuggestionsComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
