import { ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { RouterTestingModule } from '@angular/router/testing';
import { MaterialdesignModule } from '@app/lib/materialdesign.module';
import { PsmChipsBoxComponent } from './psmchipsbox.component';
import { PrimengModule } from '@app/lib/primeng.module';
import { FormsModule, ReactiveFormsModule, NgControl } from '@angular/forms';
import { HttpClientTestingModule } from '@angular/common/http/testing';
import { ChipsComponent } from '@app/components/shared/chips/chips.component';
import { DomHandler } from 'primeng/api';

describe('PsmChipsBoxComponent', () => {
  let component: PsmChipsBoxComponent;
  let fixture: ComponentFixture<PsmChipsBoxComponent>;

  configureTestSuite(() => {
     TestBed.configureTestingModule({
      declarations: [ChipsComponent, PsmChipsBoxComponent],
      imports: [
        RouterTestingModule,
        HttpClientTestingModule,
        PrimengModule,
        FormsModule,
        MaterialdesignModule,
        ReactiveFormsModule,
        NoopAnimationsModule,
      ],
      providers: [DomHandler, NgControl]
    });
  });

  beforeEach(() => {
    fixture = TestBed.createComponent(PsmChipsBoxComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
