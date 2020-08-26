/**-----
 Angular imports
 ------------------*/
import { ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';
import { RouterTestingModule } from '@angular/router/testing';
import { HttpClientTestingModule } from '@angular/common/http/testing';
import { Component } from '@angular/core';

/**-----
 Venice UI -  imports
 ------------------*/
import { ClusterGroupComponent } from './cluster-group.component';
import { ControllerService } from '@app/services/controller.service';
import { ConfirmationService } from 'primeng';

/**-----
 Third-parties imports
 ------------------*/
import { MatIconRegistry } from '@angular/material/icon';
import { LogService } from '@app/services/logging/log.service';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { MessageService } from '@app/services/message.service';

@Component({
  template: ''
})
class DummyComponent { }

describe('ClusterGroupComponent', () => {
  let component: ClusterGroupComponent;
  let fixture: ComponentFixture<ClusterGroupComponent>;

  configureTestSuite(() => {
     TestBed.configureTestingModule({
      declarations: [
        ClusterGroupComponent,
        DummyComponent
      ],
      imports: [
        RouterTestingModule.withRoutes([
          { path: 'login', component: DummyComponent }
        ]),
        HttpClientTestingModule
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
    fixture = TestBed.createComponent(ClusterGroupComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
