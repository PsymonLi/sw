import {
  Component, Input, ViewEncapsulation, OnInit, forwardRef, Injector,
  ElementRef, ChangeDetectionStrategy, ChangeDetectorRef, AfterViewInit, TemplateRef
} from '@angular/core';
import { NG_VALUE_ACCESSOR, ControlValueAccessor } from '@angular/forms';
import { FormInputComponent } from '../forminput.component';
import { SelectItem } from 'primeng/api';

@Component({
  selector: 'app-psmlistbox',
  templateUrl: './psmlistbox.component.html',
  styleUrls: ['./psmlistbox.component.scss'],
  encapsulation: ViewEncapsulation.None,
  providers: [
    {
      provide: NG_VALUE_ACCESSOR,
      useExisting: forwardRef(() => PsmListBoxComponent),
      multi: true
    }
  ]
})

export class PsmListBoxComponent extends FormInputComponent implements OnInit, AfterViewInit, ControlValueAccessor {

  protected defaultSpanClass: string = 'psm-form-list-box-container';
  protected defaultComponentClass: string = 'psm-form-list-box';

  @Input() multiple: boolean = false;
  @Input() checkbox: boolean = false;
  @Input() showToggleAll: boolean = false;
  @Input() convertListToString: boolean = false;
  @Input() listStyle: string;
  @Input() filter: string;
  @Input() filterMode: string;
  @Input() filterValue: string;
  @Input() options: SelectItem[] = [];
  @Input() optionItemTemplate: TemplateRef<any>;

  constructor(protected inj: Injector, protected el: ElementRef, protected cdr: ChangeDetectorRef) {
    super(inj, el, cdr);
  }

  ngOnInit() {
    super.ngOnInit();
  }

  ngAfterViewInit() {
    super.ngAfterViewInit();
  }

  isEqual(val: any) {
    if (this.multiple) {
      if (!this.innerValue && !val) {
        return true;
      }
      if (this.innerValue && !val) {
        return false;
      }
      if (!this.innerValue && val) {
        return false;
      }
      this.innerValue.sort((a, b) => a > b ? 1 : -1);
      val.sort((a, b) => a > b ? 1 : -1);
      return this.innerValue.join(',') === val.join(',');
    }
    return this.innerValue === val;
  }

  // convert value type from selectitem to selectItem.value
  // the value from input box to data model
  setOutputValue(val: any): any {
    const newVal = super.setOutputValue(val);
    if (!newVal) {
      return newVal;
    }
    if (this.multiple && this.convertListToString) {
      return newVal.join(',');
    }
    return newVal;
  }

  // convert value from selectItem.value to selectItem
  // if selectItem can not be found, value becomes null
  setInputValue(val: any): any {
    let newVal: any = super.setInputValue(val);
    if (!newVal) {
      return newVal;
    }
    if (this.multiple && this.convertListToString && !Array.isArray(newVal)) {
      newVal = newVal.split(',');
    }
    return newVal;
  }

}
