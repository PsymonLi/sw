import { ComponentFixture, TestBed } from '@angular/core/testing';
import { configureTestSuite } from 'ng-bullet';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { RouterTestingModule } from '@angular/router/testing';
import { MaterialdesignModule } from '@app/lib/materialdesign.module';
import { PsmNumberBoxComponent } from './psmnumberbox.component';
import { PrimengModule } from '@app/lib/primeng.module';
import { FormsModule, ReactiveFormsModule, NgControl } from '@angular/forms';
import { HttpClientTestingModule } from '@angular/common/http/testing';

describe('PsmNumberBoxComponent', () => {
  let component: PsmNumberBoxComponent;
  let fixture: ComponentFixture<PsmNumberBoxComponent>;

  configureTestSuite(() => {
     TestBed.configureTestingModule({
      declarations: [PsmNumberBoxComponent],
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
    fixture = TestBed.createComponent(PsmNumberBoxComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
