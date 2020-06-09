import { AfterViewInit, Component, EventEmitter, Input, OnChanges, OnDestroy, OnInit, Output, Renderer2, SimpleChanges, TemplateRef, ViewChild, ViewEncapsulation } from '@angular/core';
import { FormArray } from '@angular/forms';
import { ActivatedRoute } from '@angular/router';
import { Subscription } from 'rxjs';
import { Table } from 'primeng/table';
import { Animations } from '@app/animations';
import { Utility } from '@app/common/Utility';
import { ControllerService } from '@app/services/controller.service';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { IApiStatus } from '@sdk/v1/models/generated/search';
import { BaseComponent } from '@app/components/base/base.component';
import { AdvancedSearchComponent } from '../advanced-search/advanced-search.component';
import { CustomExportMap, TableCol } from '../tableviewedit';
import { TableUtility } from '../tableviewedit/tableutility';
import { TableMenuItem } from '../tableheader/tableheader.component';
import { RepeaterData } from 'web-app-framework';

@Component({
    selector: 'app-pentable',
    templateUrl: './pentable.component.html',
    styleUrls: ['./pentable.component.scss'],
    encapsulation: ViewEncapsulation.None,
    animations: [Animations]
})
export class PentableComponent extends BaseComponent implements AfterViewInit, OnChanges, OnDestroy, OnInit {
  @ViewChild('primengTable') table: Table;
  @ViewChild('advancedSearchComponent') advancedSearchComponent: AdvancedSearchComponent;

  @Input() actionTemplate: TemplateRef<any>;
  @Input() bodyTemplate: TemplateRef<any>;
  @Input() captionTemplate: TemplateRef<any>;
  @Input() columns: TableCol[] = [];
  @Input() createTemplate: TemplateRef<any>;
  @Input() data: any[] = [];
  @Input() dataKey: string = 'meta.uuid';
  @Input() enableCheckbox: boolean;
  @Input() expandTemplate: TemplateRef<any>;
  @Input() exportFilename: string = 'Pensando';
  @Input() exportMap: CustomExportMap = {};
  @Input() loading: boolean;
  @Input() numRows: number = 25;
  @Input() resizableColumns: boolean = true;
  @Input() rowHeight: number = 0;
  @Input() scrollable: boolean = false;
  @Input() searchable: boolean = false;
  @Input() searchCols: TableCol[] = [];
  @Input() searchCustomQueryOptions: RepeaterData[] = [];
  @Input() searchFormArray = new FormArray([]);
  @Input() searchKind: string;
  @Input() searchMultiSelectFields: Array<string> = [];
  @Input() sortField: string = 'meta.mod-time';
  @Input() sortOrder: number = -1;

  @Output() operationOnMultiRecordsComplete: EventEmitter<any> = new EventEmitter<any>();
  @Output() rowClickEmitter: EventEmitter<any> = new EventEmitter<any>();
  @Output() rowSelectedEmitter: EventEmitter<any> = new EventEmitter<any>();
  @Output() rowUnselectedEmitter: EventEmitter<any> = new EventEmitter<any>();
  @Output() searchCancelledEmitter: EventEmitter<any> = new EventEmitter<any>();
  @Output() searchEmitter: EventEmitter<any> = new EventEmitter<any>();

  colMouseMoveUnlisten: () => void;
  colMouseUpUnlisten: () => void;
  creatingMode: boolean = false;
  expandedRowData: any;
  filter: string;
  first: number = 0;
  hoveredRowID: string;
  rowsPerPageOptions: number[] = [10, 25, 50, 100];
  defaultRows: number = Math.min(...this.rowsPerPageOptions);
  scrollHeight: string = `100%`;
  searchInitialized: boolean = false;
  selectedColumns: TableCol[] = [];
  selectedDataObjects: any[] = [];
  showRowExpand: boolean = false;
  subscriptions: Subscription[] = [];
  tableMenuItems: TableMenuItem[] = [
    {
      text: 'Export To CSV',
      onClick: () => {
        this.exportTableDataCSV();
      }
    },
    {
      text: 'Export To JSON',
      onClick: () => {
        this.exportTableDataJSON();
      }
    }
  ];

  constructor(private _route: ActivatedRoute, protected controllerService: ControllerService, protected renderer: Renderer2) {
    super(controllerService);
  }

  ngOnInit() {
    this.selectedColumns = this.columns;
    this.controllerService.publish(Eventtypes.COMPONENT_INIT, {
      'component': this.getClassName(), 'state':
        Eventtypes.COMPONENT_INIT
    });

    if (this.searchable) {
      const sub = this._route.queryParams.subscribe(params => {
        if (params.hasOwnProperty('filter')) {
          this.filter = params['filter'];
        } else {
          this.filter = null;
        }
      });
      this.subscriptions.push(sub);
    }
  }

  ngAfterViewInit() {
    this.resizeTable();
  }

  ngOnChanges(change: SimpleChanges) {
    if (this.searchable && this.filter) {
      // if there is a "filter" query param:
      // - wait until data finishes loading (component init)
      // - if data has been updated, emit search results for parent to update filtered list
      if ((change.loading && change.loading.previousValue && !change.loading.currentValue) ||
          (change.data && !this.loading && !Utility.getLodash().isEqual(change.data.previousValue, change.data.currentValue))) {
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
      }
    }
  }

  ngOnDestroy() {
    this.subscriptions.forEach(sub => {
      sub.unsubscribe();
    });
    this.controllerService.publish(Eventtypes.COMPONENT_DESTROY, {
      'component': this.getClassName(), 'state':
        Eventtypes.COMPONENT_DESTROY
    });
  }

  reset() {
    this.first = 0;
  }

  createNewObject() {
    // If a row is expanded, we shouldnt be able to open a create new policy form
    if (!this.showRowExpand) {
      this.creatingMode = true;
      this.setPaginatorDisabled(true);
    }
  }

  creationFormClose() {
    this.creatingMode = false;
    this.controllerService.removeToaster(Utility.CREATE_FAILED_SUMMARY);
    this.setPaginatorDisabled(false);
  }

  // TODO: cleanup duplicate code from tableviewedit
  exportTableDataCSV() {
    TableUtility.exportTableCSV(this.columns, this.data, this.exportFilename, this.exportMap);
    this.controllerService.invokeInfoToaster('File Exported', this.exportFilename + '.csv');
  }
  exportTableDataJSON() {
    TableUtility.exportTableJSON(this.columns, this.data, this.exportFilename, this.exportMap);
    this.controllerService.invokeInfoToaster('FileExported', this.exportFilename + '.json');
  }

  getCellWidth(width: any) {
    if (Number.isFinite(width)) {
      return `${width}%`;
    } else {
      return width;
    }
  }

  getRowID(rowData: any): string {
    return Utility.getLodash().get(rowData, this.dataKey);
  }

  getSearchText(): string {
    const search = this.advancedSearchComponent.showAdvancedPanel ? null : this.advancedSearchComponent.search;
    const generalSearch = this.advancedSearchComponent.showAdvancedPanel ? this.advancedSearchComponent.generalSearch : null;

    return search || generalSearch || null;
  }

  handleDisabledEvents(event: Event) {
    event.stopPropagation();
  }

  isDisabled(): boolean {
    return this.creatingMode || this.showRowExpand;
  }

  isShowRowExpand(): boolean {
    return this.showRowExpand;
  }

  onColumnResize() {
    const $ = Utility.getJQuery();
    const resizerHelper = $('.ui-column-resizer-helper');
    const resizeHelperDisplay = resizerHelper.css('display');

    if (!resizeHelperDisplay || resizeHelperDisplay === 'none') {
      return;
    }

    const pTableTop = $('p-table > .ui-table').offset().top;

    const tableWrapper = $('.ui-table-wrapper');
    const tableWrapperTop = tableWrapper.offset().top;
    const tableWrapperHeight = tableWrapper.outerHeight();

    resizerHelper.css({
      'top': `${tableWrapperTop - pTableTop}px`,
      'height': `${tableWrapperHeight}px`
    });
  }

  /**
   * This api serves html template
   * @param $event
   */
  onColumnSelectChange($event) {
    const newColumns = $event.value;
    if (newColumns.length > 0) {
      const fieldname = 'field';
      this.selectedColumns = newColumns.sort((a: TableCol, b: TableCol) => {
        const aIndex = this.columns.findIndex((col: TableCol) => col[fieldname] === a[fieldname]);
        const bIndex = this.columns.findIndex((col: TableCol) => col[fieldname] === b[fieldname]);
        return aIndex - bIndex;
      });
    } else {
      this.selectedColumns = newColumns;
    }
  }

  onDeleteRecord(event, object, headerMsg, successMsg, deleteAction, postDeleteAction?) {
    if (event) {
      event.stopPropagation();
    }
    // Should not be able to delete any record while we are editing
    if (this.showRowExpand) {
      return;
    }
    this.controllerService.invokeConfirm({
      header: headerMsg,
      message: 'This action cannot be reversed',
      acceptLabel: 'Delete',
      accept: () => {
        const sub = deleteAction(object).subscribe(
          (response) => {
            this.controllerService.invokeSuccessToaster(Utility.DELETE_SUCCESS_SUMMARY, successMsg);
            if (postDeleteAction) {
              postDeleteAction();
            }
          },
          (error) => {
            if (error.body instanceof Error) {
              console.error('Service returned code: ' + error.statusCode + ' data: ' + <Error>error.body);
            } else {
              console.error('Service returned code: ' + error.statusCode + ' data: ' + <IApiStatus>error.body);
            }
            this.controllerService.invokeRESTErrorToaster(Utility.DELETE_FAILED_SUMMARY, error);
          }
        );
        this.subscriptions.push(sub);
      }
    });
  }

  onSearch() {
    this.controllerService.navigate([], {
      queryParams: {
        filter: this.getSearchText(),
      },
    });
    this.searchEmitter.emit(null);
  }

  onSearchCancelled() {
    this.controllerService.navigate([], { queryParams: null });
    this.searchCancelledEmitter.emit();
  }

  onThMouseDown(event) {
    if (event.target.classList.contains('ui-column-resizer')) {
      this.colMouseMoveUnlisten = this.renderer.listen('document', 'mousemove', () => {
        this.onColumnResize();
      });

      this.colMouseUpUnlisten = this.renderer.listen('document', 'mouseup', () => {
        if (this.colMouseMoveUnlisten) {
          this.colMouseMoveUnlisten();
        }
        this.colMouseUpUnlisten();
      });
    }
  }

  resetHover(rowData) {
    // We check if the  row that we are leaving
    // is the row that is saved so that if the rowhover
    // fires for another row before this leave we don't unset it.
    if (this.hoveredRowID === this.getRowID(rowData)) {
      this.hoveredRowID = null;
    }
  }

  resizeTable() {
    // only resize if we have scrollable table
    if (!this.scrollable) {
      return;
    }

    const $ = Utility.getJQuery();

    // set header width, account for scrollbar in table body, to align column widths with body widths
    const bodyWidth = parseInt($('.pentable-widget .ui-table-scrollable-body').css('width'), 10);
    const bodyTableWidth = parseInt($('.pentable-widget .ui-table-scrollable-body-table').css('width'), 10);
    const scrollWidth = bodyWidth - bodyTableWidth;

    $('.pentable-widget .ui-table-scrollable-header-box').css('margin-right', `${scrollWidth}px`);
  }

  rowClick($event, rowData) {
    $event['rowData'] = rowData;
    this.rowClickEmitter.emit($event);
  }

  rowExpandAnimationComplete(rowData) {
    if (!this.showRowExpand) {
      this.table.toggleRow(rowData);
    }
  }

  rowHover(rowData: any) {
    if (!this.loading && !this.isDisabled()) {
      this.hoveredRowID = this.getRowID(rowData);
    }
  }

  rowSelected(event) {
    this.rowSelectedEmitter.emit(event);
  }

  rowUnselected(event) {
    this.rowUnselectedEmitter.emit(event);
  }

  setPaginatorDisabled(disable) {
    const $ = Utility.getJQuery();
    const className = 'ui-paginator-element-disabled';
    if (disable) {
      $('.ui-paginator-element').addClass(className);
      $('.ui-paginator p-dropdown > .ui-dropdown').addClass(className);
    } else {
      $('.ui-paginator-element').removeClass(className);
      $('.ui-paginator p-dropdown > .ui-dropdown').removeClass(className);
    }
  }

  toggleRow(rowData: any, event?: Event) {
    this.expandedRowData = this.showRowExpand ? null : rowData;
    this.showRowExpand = !this.showRowExpand;
    this.setPaginatorDisabled(this.showRowExpand);

    // for expanding, toggle row right away so slide open can be seen
    // for closing, toggle row after rowExpandAnimationComplete so slide close can be seen
    if (this.showRowExpand) {
      this.table.toggleRow(rowData, event);
    }
  }

}
