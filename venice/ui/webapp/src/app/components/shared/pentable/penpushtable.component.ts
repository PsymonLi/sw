import { ViewEncapsulation, ChangeDetectionStrategy, Component, ChangeDetectorRef, Renderer2, OnInit, OnChanges, SimpleChanges, ElementRef } from '@angular/core';
import { PentableComponent } from './pentable.component';
import { Animations } from '@app/animations';
import { ControllerService } from '@app/services/controller.service';
import { ActivatedRoute } from '@angular/router';
import { BaseModel } from '@sdk/v1/models/generated/basemodel/base-model';
import { CreationForm } from '../tableviewedit/tableviewedit.component';
import { UIConfigsService } from '@app/services/uiconfigs.service';
import { AbstractControl } from '@angular/forms';
import { Utility } from '@app/common/Utility';

export abstract class CreationPushForm<I, T extends BaseModel> extends CreationForm<I, T> implements OnChanges {
  constructor(protected controllerService: ControllerService,
      protected uiconfigsSerivce: UIConfigsService,
      protected cdr: ChangeDetectorRef,
      protected objConstructor: any = null) {
    super(controllerService, uiconfigsSerivce);
  }

  ngOnChanges(changes: SimpleChanges) {
    this.cdr.markForCheck();
  }

  isFieldEmpty(field: AbstractControl): boolean {
    return Utility.isValueOrArrayEmpty(field.value);
  }
}
