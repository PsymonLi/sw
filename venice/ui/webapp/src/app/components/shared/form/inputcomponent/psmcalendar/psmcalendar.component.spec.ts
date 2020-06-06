import { ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { RouterTestingModule } from '@angular/router/testing';
import { MaterialdesignModule } from '@app/lib/materialdesign.module';
import { PsmCalendarComponent } from './psmcalendar.component';
import { PrimengModule } from '@app/lib/primeng.module';
import { FormsModule, ReactiveFormsModule, NgControl } from '@angular/forms';
import { HttpClientTestingModule } from '@angular/common/http/testing';

describe('PsmCalendarComponent', () => {
  let component: PsmCalendarComponent;
  let fixture: ComponentFixture<PsmCalendarComponent>;

  configureTestSuite(() => {
     TestBed.configureTestingModule({
      declarations: [PsmCalendarComponent],
      imports: [
        RouterTestingModule,
        HttpClientTestingModule,
        PrimengModule,
        FormsModule,
        MaterialdesignModule,
        ReactiveFormsModule,
        NoopAnimationsModule,
      ],
      providers: [NgControl]
    });
  });

  beforeEach(() => {
    fixture = TestBed.createComponent(PsmCalendarComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
