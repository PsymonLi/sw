import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { ChipsComponent } from './chips.component';
import { MaterialdesignModule } from '@app/lib/materialdesign.module';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { ChipsModule, AutoCompleteModule } from 'primeng';

describe('ChipsComponent', () => {
  let component: ChipsComponent;
  let fixture: ComponentFixture<ChipsComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ ChipsComponent ],
      imports: [
        MaterialdesignModule,
        NoopAnimationsModule,
        ChipsModule,
        AutoCompleteModule,
      ],
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(ChipsComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
