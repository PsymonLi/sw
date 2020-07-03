import { async, ComponentFixture, TestBed } from '@angular/core/testing';
import { SharedModule } from '@app/components/shared/shared.module';
import { DsbdcardComponent } from './dsbdcard.component';

describe('DsbdcardComponent', () => {
  let component: DsbdcardComponent;
  let fixture: ComponentFixture<DsbdcardComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ DsbdcardComponent ],
      imports: [
        SharedModule
      ],
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DsbdcardComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
