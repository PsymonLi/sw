import { Component, Input, ViewEncapsulation, OnInit, OnChanges, SimpleChanges, forwardRef, Injector, ElementRef, ChangeDetectionStrategy, ChangeDetectorRef, AfterViewInit } from '@angular/core';
import { NG_VALUE_ACCESSOR, ControlValueAccessor, FormControl, NgControl } from '@angular/forms';
import { Utility } from '@app/common/Utility';
import { FormInputComponent } from '../forminput.component';

@Component({
  selector: 'app-psmcalendar',
  templateUrl: './psmcalendar.component.html',
  styleUrls: ['./psmcalendar.component.scss'],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush,
  providers: [
    {
      provide: NG_VALUE_ACCESSOR,
      useExisting: forwardRef(() => PsmCalendarComponent),
      multi: true
    }
  ]
})

export class PsmCalendarComponent extends FormInputComponent implements OnInit, OnChanges, AfterViewInit, ControlValueAccessor {
  @Input() isUtc: boolean = true;
  @Input() minDate: Date = null;
  @Input() maxDate: Date = null;
  @Input() defaultDate: Date = null;

  protected defaultSpanClass: string = 'psm-form-calender-container';
  protected defaultComponentClass: string = 'psm-form-calender-box';

  ngControl: NgControl;
  innerValue: any;

  // Function to call when the rating changes.
  onChange = (val: any) => {};
  // Function to call when the input is touched (when a star is clicked).
  onTouched = () => {};

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
    if (changes.minDate && this.isUtc && this.minDate) {
      this.minDate = Utility.convertLocalTimeToUTCTime(this.minDate);
    }
    if (changes.maxDate && this.isUtc && this.maxDate) {
      this.maxDate = Utility.convertLocalTimeToUTCTime(this.maxDate);
    }
    if (changes.defaultDate && this.isUtc && this.defaultDate) {
      this.defaultDate = Utility.convertLocalTimeToUTCTime(this.defaultDate);
    }
  }

  // this method compared the current date and the new date about
  // to write into the box.
  protected isEqual(val: Date) {
    if (!this.innerValue && !val) {
      return true;
    }
    if (this.innerValue && !val) {
      return false;
    }
    if (!this.innerValue && val) {
      return false;
    }
    return this.innerValue.getTime() === val.getTime();
  }

  // this methode to give all the children component class a way to change
  // the value from input box to data model
  protected setOutputValue(val: any): any {
    if (!val || !this.isUtc) {
      return val;
    }
    return Utility.convertUTCTimeToLocalTime(val);
  }

  // add time zone diff to the value to make the calendar shows utc time
  // by a local calendar
  protected setInputValue(val: any): any {
    if (!val || !this.isUtc) {
      return val;
    }
    let newDate = val;
    if (typeof val === 'string') {
      newDate = new Date(val);
    }
    return Utility.convertLocalTimeToUTCTime(newDate);
  }

  // Allows Angular to register a function to call when the model changes.
  // Save the function as a property to call later here.
  registerOnChange(fn: (rating: number) => void): void {
    this.onChange = fn;
  }

  // Allows Angular to register a function to call when the input has been touched.
  // Save the function as a property to call later here.
  registerOnTouched(fn: () => void): void {
    this.onTouched = fn;
  }

}
