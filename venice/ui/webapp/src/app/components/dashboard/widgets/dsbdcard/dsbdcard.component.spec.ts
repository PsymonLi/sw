import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DsbdcardComponent } from './dsbdcard.component';

describe('DsbdcardComponent', () => {
  let component: DsbdcardComponent;
  let fixture: ComponentFixture<DsbdcardComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ DsbdcardComponent ]
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
