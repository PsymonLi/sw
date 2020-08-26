import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { AlertstableComponent } from './alertstable.component';
import { MaterialdesignModule } from '@lib/materialdesign.module';
import { configureTestSuite } from 'ng-bullet';
/**-----
 Angular imports
 ------------------*/
//  import { async, ComponentFixture, TestBed } from '@angular/core/testing';
 import { HttpClientTestingModule } from '@angular/common/http/testing';
//  import { configureTestSuite } from 'ng-bullet';
 import { FormsModule, ReactiveFormsModule } from '@angular/forms';
 import { MatIconRegistry } from '@angular/material/icon';
 import { NoopAnimationsModule } from '@angular/platform-browser/animations';
 import { RouterTestingModule } from '@angular/router/testing';
 import { AlerttableService } from '@app/services/alerttable.service';
 import { SharedModule } from '@app/components/shared/shared.module';

 /**-----
  Venice web-app imports
  ------------------*/
 import { ControllerService } from '@app/services/controller.service';
 import { ConfirmationService } from 'primeng';
 import { LogPublishersService } from '@app/services/logging/log-publishers.service';
 import { LogService } from '@app/services/logging/log.service';
//  import { MaterialdesignModule } from '@lib/materialdesign.module';
 import { PrimengModule } from '@lib/primeng.module';
 import { EventsService } from '@app/services/events.service';
 import { EventsService as EventsServiceGen } from '@app/services/generated/events.service';
 import { SearchService } from '@app/services/generated/search.service';
 import { UIConfigsService } from '@app/services/uiconfigs.service';
import { LicenseService } from '@app/services/license.service';
 import { MonitoringService } from '@app/services/generated/monitoring.service';
 import { MessageService } from '@app/services/message.service';
 import { AuthService } from '@app/services/auth.service';
//  import { EventstableComponent } from './eventstable.component';


describe('AlertstableComponent', () => {
  let component: AlertstableComponent;
  let fixture: ComponentFixture<AlertstableComponent>;

  configureTestSuite(() => {
    TestBed.configureTestingModule({
     declarations: [],
     imports: [
       RouterTestingModule.withRoutes([]),
       FormsModule,
       ReactiveFormsModule,
       NoopAnimationsModule,
       HttpClientTestingModule,
       PrimengModule,
       MaterialdesignModule,
       SharedModule,
     ],
     providers: [
       ControllerService,
       ConfirmationService,
       LogService,
       LogPublishersService,
       AlerttableService,
       MatIconRegistry,
       EventsService,
       EventsServiceGen,
       AlerttableService,
       SearchService,
       UIConfigsService,
       LicenseService,
       AuthService,
       MonitoringService,
       MessageService
     ]
   });
     });

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ AlertstableComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(AlertstableComponent);
    component = fixture.componentInstance;
  });

  it('should create', () => {
    fixture.detectChanges();
    expect(component).toBeTruthy();
  });
});
