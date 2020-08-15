import {
  Component, Input, ViewEncapsulation, OnInit, forwardRef, Injector,
  ElementRef, ChangeDetectionStrategy, ChangeDetectorRef, AfterViewInit
} from '@angular/core';
import { NG_VALUE_ACCESSOR, ControlValueAccessor } from '@angular/forms';
import { FormInputComponent } from '../forminput.component';

@Component({
  selector: 'app-psmtoggle',
  templateUrl: './psmtoggle.component.html',
  styleUrls: ['./psmtoggle.component.scss'],
  encapsulation: ViewEncapsulation.None,
  providers: [
    {
      provide: NG_VALUE_ACCESSOR,
      useExisting: forwardRef(() => PsmToggleComponent),
      multi: true
    }
  ]
})

export class  PsmToggleComponent extends FormInputComponent implements OnInit, AfterViewInit, ControlValueAccessor {

  protected defaultSpanClass: string = 'psm-form-toggle-container';
  protected defaultComponentClass: string = 'psm-form-toggle';

  constructor(protected inj: Injector, protected el: ElementRef, protected cdr: ChangeDetectorRef) {
    super(inj, el, cdr);
  }

  ngOnInit() {
    super.ngOnInit();
  }

  ngAfterViewInit() {
    super.ngAfterViewInit();
  }
}
