/**-----
 Angular imports
 ------------------*/
import { HttpClientTestingModule } from '@angular/common/http/testing';
import { async, ComponentFixture, TestBed } from '@angular/core/testing';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';
import { MatIconRegistry } from '@angular/material';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { RouterTestingModule } from '@angular/router/testing';
import { SharedModule } from '@app/components/shared/shared.module';
/**-----
 Venice web-app imports
 ------------------*/
import { ControllerService } from '@app/services/controller.service';
import { ConfirmationService } from 'primeng/primeng';
import { LogPublishersService } from '@app/services/logging/log-publishers.service';
import { LogService } from '@app/services/logging/log.service';
import { MaterialdesignModule } from '@lib/materialdesign.module';
import { PrimengModule } from '@lib/primeng.module';
import { MonitoringService } from '@app/services/generated/monitoring.service';
import { DestinationpolicyComponent } from './destinations.component';
import { NewdestinationComponent } from './newdestination/newdestination.component';
import { MessageService } from '@app/services/message.service';
import { MonitoringGroupModule } from '@app/components/monitoring-group/monitoring-group.module';
import { TestTablevieweditRBAC } from '@app/components/shared/tableviewedit/tableviewedit.component.spec';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { AuthService } from '@app/services/auth.service';
import { MonitoringAlertDestination } from '@sdk/v1/models/generated/monitoring';


describe('DestinationpolicyComponent', () => {
  let component: DestinationpolicyComponent;
  let fixture: ComponentFixture<DestinationpolicyComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [DestinationpolicyComponent, NewdestinationComponent],
      imports: [
        FormsModule,
        ReactiveFormsModule,
        NoopAnimationsModule,
        SharedModule,
        HttpClientTestingModule,
        PrimengModule,
        MaterialdesignModule,
        RouterTestingModule,
        MonitoringGroupModule
      ],
      providers: [
        ControllerService,
        UIConfigsService,
        AuthService,
        ConfirmationService,
        LogService,
        LogPublishersService,
        MatIconRegistry,
        MonitoringService,
        MessageService
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DestinationpolicyComponent);
    component = fixture.componentInstance;
  });

  it('should create', () => {
    fixture.detectChanges();
    expect(component).toBeTruthy();
  });

  describe('RBAC', () => {
    const testHelper = new TestTablevieweditRBAC('monitoringalertdestination');

    beforeEach(() => {
      component.isActiveTab = true;
      component.dataObjects = [new MonitoringAlertDestination()];
      testHelper.fixture = fixture;
    });

    testHelper.runTests();
  });
});
