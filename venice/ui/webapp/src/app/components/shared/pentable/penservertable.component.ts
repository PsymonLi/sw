import { ViewEncapsulation, ChangeDetectionStrategy, Component, ChangeDetectorRef, Renderer2, ElementRef, Input } from '@angular/core';
import { PentableComponent } from './pentable.component';
import { Animations } from '@app/animations';
import { ControllerService } from '@app/services/controller.service';
import { ActivatedRoute } from '@angular/router';
import { LazyLoadEvent } from 'primeng/primeng';

@Component({
    selector: 'app-penservertable',
    templateUrl: './pentable.component.html',
    styleUrls: ['./pentable.component.scss'],
    encapsulation: ViewEncapsulation.None,
    changeDetection: ChangeDetectionStrategy.OnPush,
    animations: [Animations]
})
export class PenServerTableComponent extends PentableComponent {

    @Input() totalRecords: number = 0;

    numRows: number = 100; // Keeping fixed for now
    rowsPerPageOptions: number[] = null;

    constructor(protected _route: ActivatedRoute,
        protected cdr: ChangeDetectorRef,
        protected controllerService: ControllerService,
        protected nativeElementRef: ElementRef,
        protected renderer: Renderer2) {
        super(_route, cdr, controllerService, nativeElementRef, renderer);
        this.serverMode = true; // temp for auditevents
        this.componentInit = true; // temp for auditevents
    }

    getPaginatorText(): string {
        let text = '';
        if (this.first + this.numRows >= this.totalRecords) {
            if (this.first + this.numRows === this.totalRecords || this.first + this.data.length === this.totalRecords) {
                text = `Showing ${this.first + 1} to ${this.totalRecords} of ${this.totalRecords} records`;
            } else {
                text = `From ${this.first + 1} to ${this.totalRecords} of ${this.totalRecords} records (${this.data.length} on page after dedupe)`;
            }
        } else if (this.data.length < this.numRows) {
                text = `From ${this.first + 1} to ${this.first + this.numRows} of ${this.totalRecords} records (${this.data.length} on page after dedupe)`;
        } else {
            text = `Showing ${this.first + 1} to ${this.first + this.numRows} of ${this.totalRecords} records`;
        }
        return text;
    }

    onLazyLoad(event: LazyLoadEvent) {
        this.lazyLoadEvent.emit(event);
    }

    resetLazyLoadEvent() {
        this.table.reset();
        this.reset();
    }
}
