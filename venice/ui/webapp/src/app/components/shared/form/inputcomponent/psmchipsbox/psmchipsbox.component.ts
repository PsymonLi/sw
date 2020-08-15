import { Component, forwardRef, Input, OnInit, ViewEncapsulation, ChangeDetectionStrategy, ChangeDetectorRef, ElementRef, Injector, Type, AfterViewInit } from '@angular/core';
import { NG_VALUE_ACCESSOR, NgControl, FormControl } from '@angular/forms';
import { AbstractControl, ValidatorFn, ValidationErrors } from '@angular/forms';
import { Utility } from '@app/common/Utility';
import { ChipsComponent } from '@app/components/shared/chips/chips.component';
import { DomHandler } from 'primeng/api';

/**
 * This componet is a wrap on app-chips
 * it adds form binding to the from control
 * it taks one validFn for each chip and validate all chips
 *
 */

@Component({
  selector: 'app-psmchipsbox',
  templateUrl: './psmchipsbox.component.html',
  styleUrls: ['./psmchipsbox.component.scss'],
  encapsulation: ViewEncapsulation.None,
  providers: [{
    provide: NG_VALUE_ACCESSOR,
    useExisting: forwardRef(() => PsmChipsBoxComponent),
    multi: true
  }]
})

export class PsmChipsBoxComponent extends ChipsComponent  implements OnInit, AfterViewInit {
  boxId: string = '';
  ngControl: NgControl;
  formCtrl: FormControl = new FormControl();

  isRequiredField: boolean = false;
  isFocused: boolean = false;
  originalTooltip: string = '';
  tooltipClass: string = 'global-info-tooltip';

  defaultSpanClass: string = 'psm-form-chips-box-container';
  defaultStyleClass: string = 'psm-form-chips-box';
  defaultIconSpanClass: string = 'psm-form-input-iconSpan';

  @Input() id: string;
  @Input() itemErrorMsg: string = '';
  @Input() spanClass: string = '';
  @Input() boxStyleClass: string = '';
  @Input() label: string = '';
  @Input() toolTip: string = '';
  @Input() showRequired: boolean;
  @Input() itemValidator: (item: string) => boolean = (item: string) => true;

  constructor(protected inj: Injector, el: ElementRef, protected cdr: ChangeDetectorRef, domHandler: DomHandler) {
    super(el, domHandler);
  }

  ngOnInit() {
    super.ngOnInit();
    this.boxId = this.id ? this.id + '-box' :
        this.constructor.name + '-' + Utility.s4() + '-' + Utility.s4();
  }

  ngAfterViewInit() {
    super.ngAfterViewInit();
    this.ngControl = this.inj.get<NgControl>(NgControl as Type<NgControl>);
    if (this.ngControl && this.ngControl.control) {
      this.formCtrl = this.ngControl.control as FormControl;
      if (this.formCtrl.validator) {
        const validator = this.formCtrl.validator({} as AbstractControl);
        if (validator && validator.required) {
          this.isRequiredField = true;
        }
      } else {
        this.formCtrl.setValidators([this.isFieldValid()]);
      }
      const formCtrl = this.formCtrl as any;
      if (!this.toolTip && formCtrl._venice_sdk && formCtrl._venice_sdk.description) {
        this.toolTip = formCtrl._venice_sdk.description + '\nHit enter or space key to add more.';
      }
      this.originalTooltip = this.toolTip;
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
    if (this.label) {
      clsList += ' ui-float-label';
    }
    if (this.isFocused) {
      clsList += ' psm-chips-focused';
    }
    if (!this.isFieldEmpty()) {
      clsList += ' psm-chips-noneEmpty';
    }
    return clsList;
  }

  showRequiredIcon(): boolean {
    if (!Utility.isValueOrArrayEmpty(this.formCtrl.value)) {
      return false;
    }
    if (this.showRequired === true || this.showRequired === false) {
      return this.showRequired;
    }
    return this.isRequiredField;
  }

  isFieldValid(): ValidatorFn {
    return (control: AbstractControl): ValidationErrors | null => {
      const arr: string[] = control.value;
      if (!arr) {
        return null;
      }
      for (let i = 0; i < arr.length; i++) {
        if (arr[i] && !this.itemValidator(arr[i])) {
          this.tooltipClass = 'global-error-tooltip';
          this.toolTip = this.itemErrorMsg;
          return {
            field: {
              required: false,
              message: this.itemErrorMsg
            }
          };
        }
      }
      this.tooltipClass = 'global-info-tooltip';
      this.toolTip = this.originalTooltip;
      return null;
    };
  }

  onChipsFocus() {
    this.isFocused = true;
  }

  onChipsBlur() {
    this.isFocused = false;
  }

  isFieldEmpty(): boolean {
    return Utility.isValueOrArrayEmpty(this.formCtrl.value);
  }
}
