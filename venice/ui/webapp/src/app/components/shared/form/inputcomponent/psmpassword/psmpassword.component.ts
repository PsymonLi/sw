import {
  Component, Input, ViewEncapsulation, OnInit, forwardRef, Injector,
  ElementRef, ChangeDetectionStrategy, ChangeDetectorRef, AfterViewInit
} from '@angular/core';
import { NG_VALUE_ACCESSOR, ControlValueAccessor } from '@angular/forms';
import { FormInputComponent } from '../forminput.component';

@Component({
  selector: 'app-psmpassword',
  templateUrl: './psmpassword.component.html',
  styleUrls: ['./psmpassword.component.scss'],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush,
  providers: [
    {
      provide: NG_VALUE_ACCESSOR,
      useExisting: forwardRef(() => PsmPasswordComponent),
      multi: true
    }
  ]
})

export class  PsmPasswordComponent extends FormInputComponent implements OnInit, AfterViewInit, ControlValueAccessor {

  protected defaultSpanClass: string = 'psm-form-password-container';
  protected defaultComponentClass: string = 'psm-form-password';

  @Input() passwordMask: string = null;

  isPasswordMasked: boolean = false;

  constructor(protected inj: Injector, protected el: ElementRef, protected cdr: ChangeDetectorRef) {
    super(inj, el, cdr);
  }

  ngOnInit() {
    super.ngOnInit();
    if (this.passwordMask) {
      this.isPasswordMasked = true;
    }
  }

  ngAfterViewInit() {
    super.ngAfterViewInit();
  }

  getSpanClasses(): string {
    let classList: string = super.getSpanClasses();
    if (this.isPasswordMasked) {
      classList += ' psm-form-password-masked';
    }
    return classList;
  }
}
