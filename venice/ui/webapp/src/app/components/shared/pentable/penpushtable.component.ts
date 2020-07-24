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

@Component({
    selector: 'app-penpushtable',
    templateUrl: './pentable.component.html',
    styleUrls: ['./pentable.component.scss'],
    encapsulation: ViewEncapsulation.None,
    changeDetection: ChangeDetectionStrategy.OnPush,
    animations: [Animations]
})
export class PenPushTableComponent extends PentableComponent {

  constructor(protected _route: ActivatedRoute,
      protected cdr: ChangeDetectorRef,
      protected controllerService: ControllerService,
      protected nativeElementRef: ElementRef,
      protected renderer: Renderer2) {
    super(_route, controllerService, nativeElementRef, renderer);
  }

  createNewObject() {
    super.createNewObject();
    this.cdr.detectChanges();
    // scroll to top if user click cerate button
    if (this.nativeElementRef && this.nativeElementRef.nativeElement) {
      const domElemnt = this.nativeElementRef.nativeElement;
      if (domElemnt.parentNode && domElemnt.parentNode.parentNode) {
        domElemnt.parentNode.parentNode.scrollTop = 0;
      }
    }
  }
}

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
