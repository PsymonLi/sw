import { Component, Input, EventEmitter, Output, ChangeDetectionStrategy } from '@angular/core';

@Component({
  selector: 'app-inlinebuttons',
  templateUrl: './inlinebuttons.component.html',
  styleUrls: ['./inlinebuttons.component.scss']
})
export class InlineButtonsComponent {
  @Input() saveTooltip: string;
  @Input() saveButtonClass: string;
  @Input() form: any;
  @Output() saveFunc = new EventEmitter();
  @Output() cancelFunc = new EventEmitter();

  save() {
    if (this.form) {
      this.form.saveObject();
    } else {
      this.saveFunc.emit();
    }
  }

  cancel() {
    if (this.form) {
      this.form.cancelObject();
    } else {
      this.cancelFunc.emit();
    }
  }
 }
