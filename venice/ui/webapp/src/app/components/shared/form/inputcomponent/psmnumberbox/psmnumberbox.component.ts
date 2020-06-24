import {
  Component, Input, ViewEncapsulation, OnInit, forwardRef, Injector,
  ElementRef, ChangeDetectionStrategy, ChangeDetectorRef, AfterViewInit
} from '@angular/core';
import { NG_VALUE_ACCESSOR, ControlValueAccessor } from '@angular/forms';
import { FormInputComponent } from '../forminput.component';

@Component({
  selector: 'app-psmnumberbox',
  templateUrl: './psmnumberbox.component.html',
  styleUrls: ['./psmnumberbox.component.scss'],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush,
  providers: [
    {
      provide: NG_VALUE_ACCESSOR,
      useExisting: forwardRef(() => PsmNumberBoxComponent),
      multi: true
    }
  ]
})

export class PsmNumberBoxComponent extends FormInputComponent implements OnInit, AfterViewInit, ControlValueAccessor {
  @Input() min: number;
  @Input() max: number;
  @Input() step: number = 1;

  protected defaultSpanClass: string = 'psm-form-number-box-container';
  protected defaultComponentClass: string = 'psm-form-number-box';

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
