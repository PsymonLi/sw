import { ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { RouterTestingModule } from '@angular/router/testing';
import { MaterialdesignModule } from '@app/lib/materialdesign.module';
import { PsmToggleComponent } from './psmtoggle.component';
import { PrimengModule } from '@app/lib/primeng.module';
import { FormsModule, ReactiveFormsModule, NgControl } from '@angular/forms';
import { HttpClientTestingModule } from '@angular/common/http/testing';

describe('PsmToggleComponent', () => {
  let component: PsmToggleComponent;
  let fixture: ComponentFixture<PsmToggleComponent>;

  configureTestSuite(() => {
     TestBed.configureTestingModule({
      declarations: [PsmToggleComponent],
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
    fixture = TestBed.createComponent(PsmToggleComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
