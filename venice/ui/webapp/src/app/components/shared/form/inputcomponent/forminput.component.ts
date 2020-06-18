import {Input, OnInit, Injector, ViewChild, ElementRef, Type, ChangeDetectorRef, AfterViewInit, OnDestroy, Output, EventEmitter } from '@angular/core';
import { ControlValueAccessor, NgControl, AbstractControl } from '@angular/forms';
import { Subscription } from 'rxjs';
import { Utility } from '@app/common/Utility';

export class FormInputComponent implements OnInit, AfterViewInit, OnDestroy, ControlValueAccessor {
  protected defaultIconSpanClass: string = 'psm-form-input-iconSpan';
  protected defaultSpanClass: string = 'psm-form-input-container';
  protected defaultComponentClass: string = 'psm-form-input-box';

  boxId: string = '';
  currentTooltipClass: string;
  isRequiredField: boolean = false;
  disabled: boolean = false;
  subscriptions: Subscription[] = [];

  ngControl: NgControl;
  innerValue: any;

  @Output() change: EventEmitter<any> = new EventEmitter<any>();
  @Output() focus: EventEmitter<any> = new EventEmitter<any>();
  @Output() blur: EventEmitter<any> = new EventEmitter<any>();
  @Output() click: EventEmitter<any> = new EventEmitter<any>();

  @Input() label: string;
  @Input() id: string;
  @Input() showRequired: boolean;
  @Input() showRequiredOnEmpty: boolean;
  @Input() hintTooltip: string;
  @Input() errorTooltip: string;
  @Input() spanClass: string;
  @Input() componentClass: string;
  @Input() componentStyleClass: string;
  @Input() componentInputStyleClass: string;
  @Input() tooltipClass: string = 'global-info-tooltip';
  @Input() errorTooltipClass: string = 'global-error-tooltip';


  // this input function let each component instance to change value before
  // put the value into input box
  @Input() processInput: (val: any) => any = (val) => val;
  // this input function let each component instance to change value before
  // put the value into data model
  @Input() processOutput: (val: any) => any = (val) => val;

  // Function to call when the value changes.
  onValueChange = (val: any) => {};
  // Function to call when the input is touched (when a star is clicked).
  onTouched = () => {};

  constructor(protected inj: Injector, protected el: ElementRef, protected cdr: ChangeDetectorRef) {
  }

  ngOnInit() {
    this.ngControl = this.inj.get<NgControl>(NgControl as Type<NgControl>);
    this.boxId = this.id ? this.id + '-box' :
        this.constructor.name + '-' + Utility.s4() + '-' + Utility.s4();
    this.currentTooltipClass = this.tooltipClass;
  }

  ngAfterViewInit() {
    setTimeout(() => {this.cdr.detectChanges(); }, 0);
    if (this.ngControl && this.ngControl.control) {
      this.disabled = this.ngControl.disabled;
      const sub = this.ngControl.valueChanges.subscribe(() => {
        if (this.disabled !== this.ngControl.disabled) {
          this.disabled = this.ngControl.disabled;
        }
        this.cdr.detectChanges();
      });
      this.subscriptions.push(sub);
      if (this.ngControl.control.validator) {
        const validator = this.ngControl.control.validator({}as AbstractControl);
        if (validator && validator.required) {
          this.isRequiredField = true;
        }
      }
    }
  }

  getIconSpanClasses(): string {
    let clsList = this.defaultIconSpanClass;
    if (this.showRequiredIcon()) {
      clsList += ' psm-required-box';
    }
    return clsList;
  }

  getSpanClasses(): string {
    let clsList = this.defaultSpanClass;
    if (this.spanClass) {
      clsList += ' ' + this.spanClass;
    }
    if (this.label)  {
      clsList += ' ui-float-label';
    }
    if (this.ngControl && this.ngControl.control &&
        this.ngControl.touched && this.ngControl.invalid &&
        this.ngControl.dirty) {
      clsList += ' psm-invalid-box';
    }
    return clsList;
  }

  getComponentClass(): string {
    let clsList = this.defaultComponentClass;
    if (this.componentClass) {
      clsList += ' ' + this.componentClass;
    }
    return clsList;
  }

  showRequiredIcon(): boolean {
    if (!Utility.isValueOrArrayEmpty(this.ngControl.value)) {
      return false;
    }
    if (this.showRequired === true || this.showRequired === false) {
      return this.showRequired;
    }
    return this.showRequiredOnEmpty === true || this.isRequiredField;
  }

  getTooltip(): string {
    if (this.hintTooltip) {
      this.currentTooltipClass = this.tooltipClass;
      return this.hintTooltip;
    }
    if (this.ngControl && this.ngControl.control &&
        this.ngControl.touched && this.ngControl.invalid &&
        this.ngControl.dirty) {
      this.currentTooltipClass = this.errorTooltipClass;
      if (this.errorTooltip) {
        return this.errorTooltip;
      }
      const msgs = [];
      for (const key in this.ngControl.errors) {
        if (this.ngControl.errors.hasOwnProperty(key)) {
          const error = this.ngControl.errors[key];
          if (error.message != null) {
            msgs.push(error.message);
          }
        }
      }
      return msgs.join('\n');
    }
    if (this.ngControl && this.ngControl.control) {
      this.currentTooltipClass = this.tooltipClass;
      const desc = [];
      const customControl: any = this.ngControl.control;
      if (customControl && customControl._venice_sdk && customControl._venice_sdk.description) {
        desc.push(customControl._venice_sdk.description);
      }
      if (customControl && customControl._venice_sdk && customControl._venice_sdk.hint) {
        desc.push('Ex. ' + customControl._venice_sdk.hint);
      }
      return desc.join('\n');
    }
  }

  onBoxClick(event) {
    this.click.emit(event);
  }

  onBoxFocus(event) {
    this.focus.emit(event);
  }

  onBoxBlur(event) {
    this.onTouched();
    this.blur.emit(event);
  }

  // when the innerValue changes, use this.onValueChange to update data model.
  // processOutput is for instance and setOutputValue is for class
  onInnerCtrlChange() {
    const newVal = this.processOutput(this.setOutputValue(this.innerValue));
    this.onValueChange(newVal);
    this.change.emit(newVal);
    this.onTouched();
  }

  // write the value from data model to the input box
  // processInput is for instance and setInputValue is for class
  writeValue(val: any): void {
    const newVal = this.processInput(this.setInputValue(val));
    if (!this.isEqual(val)) {
      this.innerValue = newVal;
    }
    setTimeout(() => {this.cdr.detectChanges(); }, 0);
  }

  // this method compared the current value and the new value about
  // to write into the box. expect this methode to be overridden if
  // val is object (for example, Date Object)
  protected isEqual(val: any) {
    return this.innerValue === val;
  }

  // this methode to give all the children component class a way to change
  // the value from data model before the value puts into input box
  protected setInputValue(val: any): any {
    return val;
  }

  // this methode to give all the children component class a way to change
  // the value from input box to data model
  protected setOutputValue(val: any): any {
    return val;
  }

  // Allows Angular to register a function to call when the model changes.
  // Save the function as a property to call later here.
  registerOnChange(fn: (rating: number) => void): void {
    this.onValueChange = fn;
  }

  // Allows Angular to register a function to call when the input has been touched.
  // Save the function as a property to call later here.
  registerOnTouched(fn: () => void): void {
    this.onTouched = fn;
  }

  ngOnDestroy() {
    this.subscriptions.forEach(s => {
      s.unsubscribe();
    });
  }
}
