import {
  Component, Input, ViewEncapsulation, OnInit, forwardRef, Injector,
  ElementRef, ChangeDetectionStrategy, ChangeDetectorRef, AfterViewInit
} from '@angular/core';
import { NG_VALUE_ACCESSOR, ControlValueAccessor } from '@angular/forms';
import { FormInputComponent } from '../forminput.component';
import { SelectItem } from 'primeng/api';

@Component({
  selector: 'app-psmradiobuttons',
  templateUrl: './psmradiobuttons.component.html',
  styleUrls: ['./psmradiobuttons.component.scss'],
  encapsulation: ViewEncapsulation.None,
  providers: [
    {
      provide: NG_VALUE_ACCESSOR,
      useExisting: forwardRef(() => PsmRadioButtonsComponent),
      multi: true
    }
  ]
})

export class PsmRadioButtonsComponent extends FormInputComponent implements OnInit, AfterViewInit, ControlValueAccessor {

  protected defaultSpanClass: string = 'psm-form-radio-buttons-container';
  protected defaultComponentClass: string = 'psm-form-radio-buttons';

  @Input() options: SelectItem[];
  @Input() isSmallSize: boolean = false;
  @Input() layout: string = 'horizontal';

  constructor(protected inj: Injector, protected el: ElementRef, protected cdr: ChangeDetectorRef) {
    super(inj, el, cdr);
  }

  ngOnInit() {
    super.ngOnInit();
    if (this.isSmallSize) {
      this.defaultComponentClass = 'psm-form-radio-buttons-smallsize';
    }
  }

  ngAfterViewInit() {
    super.ngAfterViewInit();
  }
}
