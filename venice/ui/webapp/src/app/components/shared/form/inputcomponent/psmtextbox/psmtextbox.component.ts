import {
  Component, Input, ViewEncapsulation, OnInit, forwardRef, Injector,
  ElementRef, ChangeDetectionStrategy, ChangeDetectorRef, AfterViewInit
} from '@angular/core';
import { NG_VALUE_ACCESSOR, ControlValueAccessor } from '@angular/forms';
import { FormInputComponent } from '../forminput.component';

@Component({
  selector: 'app-psmtextbox',
  templateUrl: './psmtextbox.component.html',
  styleUrls: ['./psmtextbox.component.scss'],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush,
  providers: [
    {
      provide: NG_VALUE_ACCESSOR,
      useExisting: forwardRef(() => PsmTextBoxComponent),
      multi: true
    }
  ]
})

export class PsmTextBoxComponent extends FormInputComponent implements OnInit, AfterViewInit, ControlValueAccessor {

  @Input() autoTrim: boolean = false;
  @Input() convertToList: boolean = false;

  protected defaultSpanClass: string = 'psm-form-text-box-container';
  protected defaultComponentClass: string = 'psm-form-text-box';

  constructor(protected inj: Injector, protected el: ElementRef, protected cdr: ChangeDetectorRef) {
    super(inj, el, cdr);
  }

  ngOnInit() {
    super.ngOnInit();
  }

  ngAfterViewInit() {
    super.ngAfterViewInit();
  }

  protected isEqual(val: any) {
    if (this.convertToList) {
      if (!this.innerValue && !val) {
        return true;
      }
      if (this.innerValue && !val) {
        return false;
      }
      if (!this.innerValue && val) {
        return false;
      }
      return this.innerValue.join(',') === val.join(',');
    }
    return this.innerValue === val;
  }

  // if user choose all value, then disable all other options
  protected setOutputValue(val: any): any {
    let newVal = super.setOutputValue(val);
    if (this.convertToList && Array.isArray(newVal)) {
      newVal = newVal.join(',');
    }
    return newVal;
  }

  protected setInputValue(val: any): any {
    let newVal: any = super.setInputValue(val);
    if (this.convertToList) {
      newVal = newVal.split(',');
      if (this.autoTrim && newVal && newVal.length > 0) {
        newVal = newVal.map(item => item.trim());
      }
    } else if (this.autoTrim && val) {
      newVal = newVal.trim();
    }
    return newVal;
  }
}
