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
export class PenPushTableComponent extends PentableComponent implements OnChanges {

  constructor(protected _route: ActivatedRoute,
      protected cdr: ChangeDetectorRef,
      protected controllerService: ControllerService,
      protected nativeElementRef: ElementRef,
      protected renderer: Renderer2) {
    super(_route, controllerService, nativeElementRef, renderer);
  }

  createNewObject() {
    super.createNewObject();
    this.refreshGui();
    // scroll to top if user click cerate button
    if (this.nativeElementRef && this.nativeElementRef.nativeElement) {
      const domElemnt = this.nativeElementRef.nativeElement;
      if (domElemnt.parentNode && domElemnt.parentNode.parentNode) {
        domElemnt.parentNode.parentNode.scrollTop = 0;
      }
    }
  }

  ngOnChanges(change: SimpleChanges) {
    if (this.searchable && !this.filterSub) {
      this.filterSub = this._route.queryParams.subscribe(params => {
        if (params.hasOwnProperty('filter')) {
          this.filter = params['filter'];
        } else {
          this.filter = null;
        }
      });
      this.subscriptions.push(this.filterSub);
    }

    if (this.searchable && this.filter) {
      // if there is a "filter" query param:
      // - wait until data finishes loading (component init)
      // - if data has been updated, emit search results for parent to update filtered list
      const loadingCompleted = change.loading && change.loading.previousValue && !change.loading.currentValue;
      let dataChanged = false;
      if (change.data && !this.loading) {
        const _ = Utility.getLodash();
        const sortFieldArr = this.sortField.split('.');
        const prevDataSorted = _.sortBy(change.data.previousValue, data => _.get(data, sortFieldArr));
        const currDataSorted = _.sortBy(change.data.currentValue, data => _.get(data, sortFieldArr));
        dataChanged = prevDataSorted.length !== currDataSorted.length || currDataSorted.some((data, idx) => {
          const prevData = prevDataSorted[idx];
          if (data && data.meta && prevData && prevData.meta) {
            return data.meta.uuid !== prevData.meta.uuid;
          }
          return false;
        });
      }

      if (loadingCompleted || dataChanged) {
        // emit search based on query params after current cycle
        setTimeout(() => {
          if (!this.searchInitialized) {
            this.advancedSearchComponent.search = this.filter;
            this.advancedSearchComponent.generalSearch = this.filter;
            this.searchInitialized = true;
          }
          // if search text does not match filter, user is editing. clear out filter and let data update naturally.
          // TODO: implement checking filter value in advanced search component after adding support for advanced search fields in pentable.
          if (this.searchInitialized && this.getSearchText() !== this.filter) {
            this.controllerService.navigate([], {
              queryParams: {
                filter: null,
              },
            });
            return;
          }
          this.searchEmitter.emit(null);
        }, 0);
        this.reset();
      }
    }

    if (change.data) {
      setTimeout(() => {
        this.isPageSelected();
        this.refreshGui();
        if (this.receiveSelectedDataUpdate) {
          this.updateSelectedDataObjects();
        }
      }, 0);
    }
  }

  refreshGui() {
    this.cdr.detectChanges();
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
