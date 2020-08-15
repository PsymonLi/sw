import {
  Component, Input, ViewEncapsulation, OnInit, forwardRef, Injector,
  ElementRef, ChangeDetectionStrategy, ChangeDetectorRef, AfterViewInit, TemplateRef
} from '@angular/core';
import { NG_VALUE_ACCESSOR, ControlValueAccessor } from '@angular/forms';
import { FormInputComponent } from '../forminput.component';
import { SelectItem } from 'primeng/api';
import { Utility } from '@app/common/Utility';

@Component({
  selector: 'app-psmpicklist',
  templateUrl: './psmpicklist.component.html',
  styleUrls: ['./psmpicklist.component.scss'],
  encapsulation: ViewEncapsulation.None,
  providers: [
    {
      provide: NG_VALUE_ACCESSOR,
      useExisting: forwardRef(() => PsmPickListComponent),
      multi: true
    }
  ]
})

export class PsmPickListComponent extends FormInputComponent implements OnInit, AfterViewInit, ControlValueAccessor {

  protected defaultSpanClass: string = 'psm-form-picklist-box-container';
  protected defaultComponentClass: string = 'psm-form-picklist-box';

  @Input() options: SelectItem[] = [];
  @Input() sourceHeader: string;
  @Input() targetHeader: string;
  @Input() filterBy: string;
  @Input() showSourceFilter: boolean = true;
  @Input() showTargetFilter: boolean = true;
  @Input() dragdrop: boolean = false;
  @Input() dragdropScope: string;
  @Input() listStyle: string;
  @Input() responsive: boolean = false;
  @Input() showSourceControls: boolean = true;
  @Input() showTargetControls: boolean = true;
  @Input() sourceFilterPlaceholder: string;
  @Input() targetFilterPlaceholder: string;
  @Input() optionItemTemplate: TemplateRef<any>;

 target: SelectItem[] = [];
 source: SelectItem[] = [];


  constructor(protected inj: Injector, protected el: ElementRef, protected cdr: ChangeDetectorRef) {
    super(inj, el, cdr);
  }

  ngOnInit() {
    super.ngOnInit();
    this.source = [...this.options];
  }

  ngAfterViewInit() {
    super.ngAfterViewInit();
  }

  onDataChanged() {
    const newValue = this.target.map(item => item.value);
    this.innerValue = newValue;
    this.onValueChange(newValue);
    this.change.emit(newValue);
    this.onTouched();
  }

  writeValue(newVal: any): void {
    this.innerValue = newVal;
    if (!Utility.isValueOrArrayEmpty(newVal)) {
      this.source = [];
      this.target = [];
      this.options.forEach(option => {
        if (newVal.includes(option.value)) {
          this.target.push(option);
        } else {
          this.source.push(option);
        }
      });
    }
  }

}
