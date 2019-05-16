import {  ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';

import { WorkloadsComponent } from './workloads.component';
import { PrimengModule } from '@app/lib/primeng.module';
import { MaterialdesignModule } from '@app/lib/materialdesign.module';
import { RouterTestingModule } from '@angular/router/testing';
import { HttpClientTestingModule } from '@angular/common/http/testing';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { SharedModule } from '@app/components/shared/shared.module';

describe('WorkloadsComponent', () => {
  let component: WorkloadsComponent;
  let fixture: ComponentFixture<WorkloadsComponent>;

  configureTestSuite(() => {
     TestBed.configureTestingModule({
      declarations: [WorkloadsComponent],
      imports: [
        PrimengModule,
        MaterialdesignModule,
        RouterTestingModule,
        HttpClientTestingModule,
        NoopAnimationsModule,
        SharedModule
      ],
    });
      });

  beforeEach(() => {
    fixture = TestBed.createComponent(WorkloadsComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
