import {Input, OnInit, Injector, ViewChild, ElementRef, Type, ChangeDetectorRef, AfterViewInit, OnDestroy } from '@angular/core';
import { ControlValueAccessor, NgControl, AbstractControl } from '@angular/forms';
import { Subscription } from 'rxjs';
import { Utility } from '@app/common/Utility';

export class FormInputComponent implements OnInit, AfterViewInit, OnDestroy, ControlValueAccessor {
  protected defaultSpanClass: string = 'psm-form-input-container';
  protected defaultComponentClass: string = 'psm-form-input-box';

  boxId: string = '';
  isRequiredField: boolean = false;
  disabled: boolean = false;
  subscriptions: Subscription[] = [];

  ngControl: NgControl;
  innerValue: any;

  @ViewChild('componentbox', {read: ElementRef}) inputBox: ElementRef;
  @Input() label: string;
  @Input() id: string;
  @Input() showRequired: boolean;
  @Input() showRequiredOnEmpty: boolean;
  @Input() spanClass: string;
  @Input() componentClass: string;
  @Input() componentStyleClass: string;
  @Input() componentInputStyleClass: string;
  // this input function let each component instance to change value before
  // put the value into input box
  @Input() processInput: (val: any) => any = (val) => val;
  // this input function let each component instance to change value before
  // put the value into data model
  @Input() processOutput: (val: any) => any = (val) => val;

  // Function to call when the rating changes.
  onChange = (val: any) => {};
  // Function to call when the input is touched (when a star is clicked).
  onTouched = () => {};

  constructor(protected inj: Injector, protected el: ElementRef, protected cdr: ChangeDetectorRef) {
  }

  ngOnInit() {
    this.ngControl = this.inj.get<NgControl>(NgControl as Type<NgControl>);
    this.boxId = this.id ? this.id + '-box' :
        this.constructor.name + '-' + Utility.s4() + '-' + Utility.s4();
  }

  ngAfterViewInit() {
    setTimeout(() => {this.cdr.detectChanges(); }, 0);
    if (this.ngControl && this.ngControl.control) {
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

  getSpanClasses(): string {
    let clsList = this.defaultSpanClass;
    if (this.spanClass) {
      clsList += ' ' + this.spanClass;
    }
    if (this.label)  {
      clsList += ' ui-float-label';
    }
    if (this.showRequiredIcon()) {
      clsList += ' psm-required-box';
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
    if (this.showRequired === true || this.showRequired === false) {
      return this.showRequired;
    }
    if (this.showRequiredOnEmpty === true || this.isRequiredField) {
      return !this.ngControl.value;
    }
    return false;
  }

  // when the innerValue changes, use this.onChange to update data model.
  // processOutput is for instance and setOutputValue is for class
  onInnerCtrlChange() {
    this.onChange(this.processOutput(this.setOutputValue(this.innerValue)));
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
    this.onChange = fn;
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
