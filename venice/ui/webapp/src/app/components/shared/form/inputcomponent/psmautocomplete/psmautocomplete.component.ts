import {
  Component, Input, ViewEncapsulation, OnInit, forwardRef, Injector,
  ElementRef, ChangeDetectionStrategy, ChangeDetectorRef, AfterViewInit, Output
} from '@angular/core';
import { NG_VALUE_ACCESSOR, ControlValueAccessor } from '@angular/forms';
import { FormInputComponent } from '../forminput.component';

@Component({
  selector: 'app-psmautocomplete',
  templateUrl: './psmautocomplete.component.html',
  styleUrls: ['./psmautocomplete.component.scss'],
  encapsulation: ViewEncapsulation.None,
  providers: [
    {
      provide: NG_VALUE_ACCESSOR,
      useExisting: forwardRef(() => PsmAutoCompleteComponent),
      multi: true
    }
  ]
})

export class PsmAutoCompleteComponent extends FormInputComponent implements OnInit, AfterViewInit, ControlValueAccessor {

  @Input() autoTrim: boolean = false;
  @Input() suggestions: string[] = [];
  @Input() completeMethod: (event: any) => void = null;

  protected defaultSpanClass: string = 'psm-form-autocomplete-container';
  protected defaultComponentClass: string = 'psm-form-autocomplete';

  constructor(protected inj: Injector, protected el: ElementRef, protected cdr: ChangeDetectorRef) {
    super(inj, el, cdr);
  }

  ngOnInit() {
    super.ngOnInit();
  }

  ngAfterViewInit() {
    super.ngAfterViewInit();
  }

  protected setOutputValue(val: any): any {
    let newVal = super.setOutputValue(val);
    if (this.autoTrim && val) {
      newVal = newVal.trim();
    }
    return newVal;
  }
}
