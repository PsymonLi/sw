import { ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { RouterTestingModule } from '@angular/router/testing';
import { MaterialdesignModule } from '@app/lib/materialdesign.module';
import { PsmMultiSelectComponent } from './psmmultiselectbox.component';
import { PrimengModule } from '@app/lib/primeng.module';
import { FormsModule, ReactiveFormsModule, NgControl } from '@angular/forms';
import { HttpClientTestingModule } from '@angular/common/http/testing';

describe('PsmMultiSelectComponent', () => {
  let component: PsmMultiSelectComponent;
  let fixture: ComponentFixture<PsmMultiSelectComponent>;

  configureTestSuite(() => {
     TestBed.configureTestingModule({
      declarations: [PsmMultiSelectComponent],
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
    fixture = TestBed.createComponent(PsmMultiSelectComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
