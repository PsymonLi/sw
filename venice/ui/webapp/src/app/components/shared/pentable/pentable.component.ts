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
import { SelectItem } from 'primeng/api';

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

  @Output() archiveQueryEmitter: EventEmitter<any> = new EventEmitter<any>();
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
  filterSub: Subscription;
  first: number = 0;
  hoveredRowID: string;
  rowsPerPageOptions: number[] = [10, 25, 50, 100];
  defaultRows: number = Math.min(...this.rowsPerPageOptions);
  pageSelected: boolean = false;
  scrollHeight: string = `100%`;
  searchInitialized: boolean = false;
  selectedColumns: TableCol[] = [];
  selectedDataObjects: any[] = [];
  selectedDataObjectsKeySet: Set<any> = new Set<any>();
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
    },
  ];
  selectAllMode: boolean = false;
  selectOptions: SelectItem[] = [
    {label: 'Select by page', value: 'page'},
    {label: 'Select all', value: 'all'},
  ];

  constructor(private _route: ActivatedRoute, protected controllerService: ControllerService, protected renderer: Renderer2) {
    super(controllerService);
  }

  debug(text: string = 'Pen table is rendering ......') {
    console.warn(text);
  }

  ngOnInit() {
    this.selectedColumns = this.columns;
  }

  ngAfterViewInit() {
    this.resizeTable();
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
      }, 0);
    }
  }

  genDataObjectsSetByKey(objects: any[]) {
    const selKey = new Set<any>();
    objects.forEach(obj => {
      selKey.add(Utility.getObjectValueByPropertyPath(obj, this.dataKey, false));
    });
    return selKey;
  }

  getArchiveQuery(event) {
    this.archiveQueryEmitter.emit(event);
  }

  isSuperset(set: any, subset: any) {
    const subsetArr = Array.from(subset.values());
    for (const ele of subsetArr) {
        if (!set.has(ele)) {
            return false;
        }
    }
    return true;
  }

  setUnion(setA: any, setB: any) {
    const union = new Set(setA);
    setB.forEach(ele => {
      union.add(ele);
    });
    return union;
  }

  setDifference(setA: any, setB: any) {
    const diff = new Set(setA);
    setB.forEach(ele => {
      diff.delete(ele);
    });
    return diff;
  }

  ngOnDestroy() {
    this.subscriptions.forEach(sub => {
      sub.unsubscribe();
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
    this.controllerService.invokeInfoToaster('File Exported', this.exportFilename + '.json');
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

  /**
   * Current page data objects would be pushed into selected data objects when table header checkbox is checked, otherwise would be removed from selected data objects
   * @param checked Tells if table header checkbox is checked or not
   */
  onHeaderCheckboxToggle(checked) {
    const pageDataObjects = this.data.slice(this.first, this.first + this.numRows);
    const pageDataObjectsKeySet = this.genDataObjectsSetByKey(pageDataObjects);
    const newDataObjectsSet = checked ? this.setUnion(this.selectedDataObjectsKeySet, pageDataObjectsKeySet) : this.setDifference(this.selectedDataObjectsKeySet, pageDataObjectsKeySet);
    const newDataObjects: any[] = [];
    const tempObj = pageDataObjects.concat(this.selectedDataObjects);
    newDataObjectsSet.forEach(ele => {
      for (let i = 0; i < tempObj.length; i++) {
        if (ele === Utility.getObjectValueByPropertyPath(tempObj[i], this.dataKey, false)) {
          newDataObjects.push(tempObj[i]);
          break;
        }
      }
    });
    this.selectedDataObjects = newDataObjects;
    setTimeout(() => {
      this.isPageSelected();
    }, 0);
  }

  onPage(event) {
    this.first = event.first;
    this.numRows = event.rows;
    this.isPageSelected();
  }

  onSort() {
    this.reset();
  }

  /**
   * Table Header checkbox would be checked if all data objects in current page are selected
   */
  isPageSelected() {
    this.selectedDataObjectsKeySet = this.genDataObjectsSetByKey(this.selectedDataObjects);
    const pageDataObjects = this.data.slice(this.first, this.first + this.numRows);
    // When page is loading its length can be 0
    if (pageDataObjects.length > 0) {
      const objKeySet = this.genDataObjectsSetByKey(pageDataObjects);
      this.pageSelected = this.isSuperset(this.selectedDataObjectsKeySet, objKeySet);
    }
  }

  onSelectModeChange($event) {
    this.selectedDataObjects = [];
    if ($event.value === 'all') {
      this.selectAllMode = true;
    } else {
      this.selectAllMode = false;
    }
    this.isPageSelected();
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
    this.reset();
    this.searchCancelledEmitter.emit();
  }

  clearSearch() {
    if (this.advancedSearchComponent) {
      this.advancedSearchComponent.clearSearchForm();
    }
    this.controllerService.navigate([], { queryParams: null });
    this.reset();
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
    this.isPageSelected();
    this.rowSelectedEmitter.emit(event);
  }

  rowUnselected(event) {
    this.isPageSelected();
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
