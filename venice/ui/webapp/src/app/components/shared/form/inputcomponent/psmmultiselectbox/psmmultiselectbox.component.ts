import {
  Component, Input, ViewEncapsulation, OnInit, OnChanges, SimpleChanges,
  forwardRef, Injector, ElementRef, ChangeDetectionStrategy,
  ChangeDetectorRef, AfterViewInit
} from '@angular/core';
import { NG_VALUE_ACCESSOR, ControlValueAccessor } from '@angular/forms';
import { FormInputComponent } from '../forminput.component';
import { SelectItem } from 'primeng/api';

@Component({
  selector: 'app-psmmultiselectbox',
  templateUrl: './psmmultiselectbox.component.html',
  styleUrls: ['./psmmultiselectbox.component.scss'],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush,
  providers: [
    {
      provide: NG_VALUE_ACCESSOR,
      useExisting: forwardRef(() => PsmMultiSelectComponent),
      multi: true
    }
  ]
})

export class PsmMultiSelectComponent extends FormInputComponent implements OnInit, OnChanges, AfterViewInit, ControlValueAccessor {
  @Input() options: SelectItem[];
  @Input() allOption: SelectItem;
  @Input() filter: boolean = false;
  @Input() showHeader: boolean = false;
  @Input() showToggleAll: boolean = false;

  allOptions: SelectItem[] = [];
  protected defaultSpanClass: string = 'psm-form-multi-select-container';
  protected defaultComponentClass: string = 'psm-form-multi-select-box';

  constructor(protected inj: Injector, protected el: ElementRef, protected cdr: ChangeDetectorRef) {
    super(inj, el, cdr);
  }

  ngOnInit() {
    super.ngOnInit();
  }

  ngAfterViewInit() {
    super.ngAfterViewInit();
  }

  ngOnChanges(changes: SimpleChanges) {
    if (changes.options) {
      if (this.allOption) {
        this.allOptions = [this.allOption, ...this.options];
      } else {
        this.allOptions = [...this.options];
      }
    }
  }

  disableAllOtherOptions() {
    this.allOptions = this.allOptions.map(option => {
      if (option.value !== this.allOption.value) {
        option.disabled = true;
      }
      return option;
    });
  }

  enableAllOtherOptions() {
    this.allOptions = this.allOptions.map(option => {
      option.disabled = false;
      return option;
    });
  }

  // this method compared the current date and the new date about
  // to write into the box.
  protected isEqual(val: string[]) {
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

  // if user choose all value, then disable all other options
  protected setOutputValue(val: string[]): any {
    let newVal: string[] = super.setOutputValue(val);
    if (this.allOption && newVal && newVal.length > 1 &&
        newVal.indexOf(this.allOption.value) > -1) {
      this.innerValue = [this.allOption.value];
      newVal = this.innerValue;
    }
    if (this.allOption) {
      if (newVal && newVal.length === 1 &&
          newVal[0] === this.allOption.value) {
        this.disableAllOtherOptions();
      } else {
        this.enableAllOtherOptions();
      }
    }
    return newVal;
  }

  protected setInputValue(val: string[]): any {
    const newVal: string[] = super.setInputValue(val);
    if (this.allOption && newVal && newVal.length > 0 &&
        newVal.indexOf(this.allOption.value) > -1) {
      this.disableAllOtherOptions();
    }
    return newVal;
  }
}
