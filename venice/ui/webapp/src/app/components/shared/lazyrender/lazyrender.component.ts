import { Component, OnInit, ContentChild, AfterContentInit, Input, Output, EventEmitter, ElementRef, SimpleChanges, OnChanges, AfterViewInit, HostListener, IterableDiffers, IterableDiffer, DoCheck } from '@angular/core';
import { Table } from 'primeng/table';
import { ArrayChunkUtility } from '@app/common/ArrayChunkUtility';
import { Utility } from '@app/common/Utility';
import { LazyLoadEvent } from 'primeng/primeng';
import { HttpEventUtility } from '@app/common/HttpEventUtility';

/**
 * This component is meant to be used as a wrapper around a primeng
 * table. It uses primeng's infinite scrolling capabilities to lazy render
 * large data sets. This component will look for a child primeng table, and set
 * all the properties that are needed for infinite scroll.
 *
 * When there is new data (binding of data changes) there are two possible behaviors:
 * 1. If the user is at the top of the page, we switch to the new data instantly
 * 2. If the user has scrolled at all, we leave the old data,
 *    but display a new data button. When the user clicks this button,
 *    we switch to the new data and scroll the user to the top of the table
 *
 * Note: This component doesn't support any kind of filtering. Filtering should be done
 * in the parent component and replace the data input binding with the new filtered
 * data set. See shared/alertsevents component
 *
 * See sgpolicy component and shared/alertsevents compononet for an example of how
 * to use this component.
 */
@Component({
  selector: 'app-lazyrender',
  templateUrl: './lazyrender.component.html',
  styleUrls: ['./lazyrender.component.scss'],
})
export class LazyrenderComponent implements OnInit, AfterContentInit, OnChanges, AfterViewInit, DoCheck {
  @ContentChild(Table) primengTable: Table;

  // The entire data set, will only render a subsection of it at a time
  @Input() data: any[] = [];

  // Primeng needs the height of each row to be fixed and known beforehand
  @Input() virtualRowHeight: number = 36;

  // Number of rows to display in the table
  // Needed since the initial call to load scroll is made
  // before we hook onto it
  @Input() rows: number = 28;

  // Whether or not to check the array objects on each
  // ngDoCheck cycle
  @Input() runDoCheck: boolean = false;

  // Emits when the displayed data set switches to the updated data
  @Output() dataUpdate: EventEmitter<boolean> = new EventEmitter<boolean>();

  // Emits the data entries that are currently loaded
  @Output() loadedData: EventEmitter<ReadonlyArray<any>> = new EventEmitter<ReadonlyArray<any>>();

  // The array that is given to primeng to render
  dataLazy: ReadonlyArray<any> = [];

  // Utility class that holds all the data, and lets us easily request chunks
  dataChunkUtility = new ArrayChunkUtility([], false);

  // Whether or not we have a new set of data ready to update to
  hasUpdate: boolean = false;

  // Flag we set once the viewInit hook has been called
  // Once this is done we know we can resize the table.
  viewInitComplete: boolean = false;

  // Holds the refernce to the interval that checks table sizing
  resizeInterval: NodeJS.Timer;

  // Holds the refernce to the timeout that resize table uses
  resizeTimeout: NodeJS.Timer;

  // Differs object for determining if data has changes
  arrayDiffers: IterableDiffer<any>;

  // Flag to determine whether we allow the table to automatically update when
  // user is scrolled to the top of the table.
  isFrozen: boolean = false;

  constructor(protected elRef: ElementRef,
    protected _iterableDiffers: IterableDiffers) {
    this.arrayDiffers = _iterableDiffers.find([]).create(HttpEventUtility.trackBy);
  }

  ngOnInit() {
    this.dataChunkUtility.updateData(this.data, true);
    this.dataLazy = this.dataChunkUtility.requestChunk(0, this.rows * 2);
    this.setTableValues();
  }

  freezeTable() {
    this.isFrozen = true;
  }

  unfreezeTable() {
    this.isFrozen = false;
    if (this.viewInitComplete && this.getTableScroll() === 0) {
      this.resetTableView();
    }
  }

  /**
   * We check if any of the objects in the array have changed
   * This kind of detection is not automatically done by angular
   * To improve efficiency, we check only the name and last mod time
   * (see trackBy function) instead of checking every object field.
   */
  ngDoCheck() {
    if (!this.runDoCheck) {
      return;
    }
    const changes = this.arrayDiffers.diff(this.data);
    if (changes) {
      this.onData(this.data);
    }
  }

  ngOnChanges(changes: SimpleChanges) {
    if (changes.data) {
      this.onData(changes.data.currentValue);
    }
  }

  onData(data) {
    this.dataChunkUtility.updateData(data, false);
    // Auto update array if blank OR if we are at the top of the table (scroll is 0)
    // We skip this if view hasn't been initialized yet
    if (this.viewInitComplete) {
      if (!this.isFrozen && this.getTableScroll() === 0) {
        this.resetTableView();
      } else {
        this.hasUpdate = true;
      }
    }
  }

  // Sets the data to render, as well as the total records value
  // so that primeng adjusts the scrollbar
  setTableValues() {
    if (this.primengTable != null) {
      this.primengTable.value = this.dataLazy as any[];
      this.primengTable.totalRecords = this.dataChunkUtility.length;
      this.loadedData.emit(this.dataLazy);
    }
  }

  ngAfterViewInit() {
    this.viewInitComplete = true;
    // Need to put into next cycle to prevent primeNG overriding
    // putting longer delay so that the rest of the application
    // finishes rendering. Currently causing calculations to be ~1 pixel off
    // and displaying an uneccessary scrollbar
    this.resizeInterval = setInterval(() => {
      this.resizeTable(500);
    }, 1000);
  }

  stopResizeTableInterval() {
    clearInterval(this.resizeInterval);
  }

  /**
   * PrimeNG is currently not height responsive when using virtual scroll
   * It's also calculating it's initial height before Flex Layout has time to take effect
   * Whenever the page resizes we manually calculate and set the height.
   *
   * The host listener binding allows this function to fire
   * whenever the window resizes
   *
   * TODO: Setup an rxjs throttle on the window resize event for better performance
   *
   * Delay is the ms to wait before calculating the new dimensions
   */
  @HostListener('window:resize')
  resizeTable(delay: number = 0) {
    // Set table to be container minus header
    if (this.resizeTimeout != null) {
      clearTimeout(this.resizeTimeout);
      this.resizeTimeout = null;
    }
    this.resizeTimeout = setTimeout(() => {
      const $ = Utility.getJQuery();
      const containerHeight = $('.lazyrender-container').outerHeight();
      let headerHeight = $('.lazyrender-container .ui-table-caption').outerHeight();
      const tableBodyHeader = $('.lazyrender-container .ui-table-scrollable-header').outerHeight();
      // If there is no caption element, outerHeight will return undefined
      if (headerHeight == null) {
        headerHeight = 0;
      }
      const newHeight = containerHeight - headerHeight - tableBodyHeader;
      $('.lazyrender-container .ui-table-scrollable-body').css('max-height', newHeight + 'px');
      // Setting height as well since sometimes the auto calculation is off by 1 px
      $('.lazyrender-container .ui-table-scrollable-body').css('height', newHeight + 'px');
    }, delay);
  }

  /**
   * Angular hook, content child will be ready in this hook
   */
  ngAfterContentInit() {
    if (this.primengTable != null) {
      // Setting attributes needed for virtual scroll
      this.primengTable.onLazyLoad.subscribe(event => {
        this.tableScrollLoad(event);
      });
      this.primengTable.rows = this.rows;
      this.primengTable.virtualRowHeight = this.virtualRowHeight;
      this.primengTable.scrollHeight = '100%';
      this.primengTable.virtualScroll = true;
      this.primengTable.virtualScrollDelay = 100;
      this.primengTable.lazy = true;
      this.primengTable.scrollable = true;

      // Set values, will be blank array at this point
      // since input data hasn't loaded yet
      this.setTableValues();
    }
  }

  /**
   * Gets the requested data chunk, and gives it to Primeng
   * Attached to the onLazyLoad hook in ngAfterContentInit
   *
   * Note: does not support filtering requests
   *
   * @param event The event request passed in by Primeng
   */
  tableScrollLoad(event: LazyLoadEvent) {
    this.dataChunkUtility.sort(event.sortField, event.sortOrder);
    this.dataLazy = this.dataChunkUtility.requestChunk(event.first, event.first + event.rows);
    this.setTableValues();
  }

  /**
   * Returns the table's current scroll amount
   */
  getTableScroll() {
    return this.elRef.nativeElement.querySelector('.ui-table-scrollable-body').scrollTop;
  }

  scrollToRowNumber(rowNum) {
    const scrollAmount = rowNum * this.virtualRowHeight;
    this.elRef.nativeElement.querySelector('.ui-table-scrollable-body').scroll(0, scrollAmount);
  }

  /**
   * Switches to use new data and scrolls the table to the top
   */
  resetTableView() {
    this.dataChunkUtility.switchToNewData();
    this.hasUpdate = false;
    // If we are scrolled a lot, the scroll to the top will trigger the table
    // to make a new request. If we are near the top though, it won't trigger so we
    // must load the new values
    this.dataLazy = this.dataChunkUtility.getLastRequestedChunk();
    this.dataUpdate.emit(true);
    this.setTableValues();
    this.elRef.nativeElement.querySelector('.ui-table-scrollable-body').scroll(0, 0);
  }

  // Returns the data that is currently being displayed in the table
  getCurrentDataLength(): number {
    return this.dataChunkUtility.currentDataArray.length;
  }
}
