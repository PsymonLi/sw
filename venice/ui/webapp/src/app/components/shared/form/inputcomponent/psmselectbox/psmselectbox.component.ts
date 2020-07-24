import {
  Component, Input, ViewEncapsulation, OnInit, forwardRef, Injector,
  ElementRef, ChangeDetectionStrategy, ChangeDetectorRef, AfterViewInit, OnChanges, SimpleChanges
} from '@angular/core';
import { NG_VALUE_ACCESSOR, ControlValueAccessor } from '@angular/forms';
import { FormInputComponent } from '../forminput.component';
import { SelectItem } from 'primeng/api';

@Component({
  selector: 'app-psmselectbox',
  templateUrl: './psmselectbox.component.html',
  styleUrls: ['./psmselectbox.component.scss'],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush,
  providers: [
    {
      provide: NG_VALUE_ACCESSOR,
      useExisting: forwardRef(() => PsmSelectBoxComponent),
      multi: true
    }
  ]
})

export class PsmSelectBoxComponent extends FormInputComponent implements OnInit, OnChanges, AfterViewInit, ControlValueAccessor {

  allOptions: SelectItem[] = [];
  protected defaultSpanClass: string = 'psm-form-select-box-container';
  protected defaultComponentClass: string = 'psm-form-select-box';

  @Input() editable: boolean = false;
  @Input() supportClear: boolean = false;
  @Input() options: SelectItem[];
  @Input() addEmptyOption: boolean = false;
  @Input() emptyOptionLabel: string = '';
  @Input() formatOptionLabel: (option: any) => string =
      (option: any) => option.label

  constructor(protected inj: Injector, protected el: ElementRef, protected cdr: ChangeDetectorRef) {
    super(inj, el, cdr);
  }

  ngOnInit() {
    super.ngOnInit();
  }

  ngAfterViewInit() {
    super.ngAfterViewInit();
  }

  getSpanClasses(): string {
    let clsList = super.getSpanClasses();
    if (this.editable) {
      clsList += ' psm-select-editable';
    }
    return clsList;
  }

  ngOnChanges(changes: SimpleChanges) {
    if (changes.options) {
      if (this.addEmptyOption) {
        this.allOptions = [{
          label: this.emptyOptionLabel,
          value: null
        }, ...this.options];
      } else {
        this.allOptions = [...this.options];
      }
    }
  }

  resetValue() {
    this.writeValue(null);
  }
}
