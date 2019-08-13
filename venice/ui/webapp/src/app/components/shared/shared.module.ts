import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { PagebodyComponent } from './pagebody/pagebody.component';
import { FlexLayoutModule } from '@angular/flex-layout';
import { SpinnerComponent } from './spinner/spinner.component';
import { DsbdwidgetheaderComponent } from './dsbdwidgetheader/dsbdwidgetheader.component';
import { ModalheaderComponent } from './modal/modalheader/modalheader.component';
import { ModalbodyComponent } from './modal/modalbody/modalbody.component';
import { ModalitemComponent } from './modal/modalitem/modalitem.component';
import { ModalcontentComponent } from './modal/modalcontent/modalcontent.component';
import { ModalwidgetComponent } from './modal/modalwidget/modalwidget.component';

import { MaterialdesignModule } from '@lib/materialdesign.module';
import { TableheaderComponent } from '@app/components/shared/tableheader/tableheader.component';
import { LazyrenderComponent } from './lazyrender/lazyrender.component';
import { PrettyDatePipe } from '@app/components/shared/Pipes/PrettyDate.pipe';
import { AlertseventsComponent } from '@app/components/shared/alertsevents/alertsevents.component';
import { PrimengModule } from '@app/lib/primeng.module';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';
import { WhitespaceTrimDirective } from '@app/components/shared/directives/whitespacetrim.directive';
import { FieldselectorComponent } from './fieldselector/fieldselector.component';
import { HerocardComponent } from './herocard/herocard.component';
import { WidgetsModule } from 'web-app-framework';
import { SorticonComponent } from './sorticon/sorticon.component';
import { BasecardComponent } from './basecard/basecard.component';
import { LinegraphComponent } from './linegraph/linegraph.component';
import { FlipComponent } from './flip/flip.component';
import { ErrorTooltipDirective } from './directives/errorTooltip.directive';
import { TablevieweditHTMLComponent } from './tableviewedit/tableviewedit.component';
import { RoleGuardDirective } from './directives/roleGuard.directive';
import { LabeleditorComponent } from './labeleditor/labeleditor.component';
import { AdvancedSearchComponent } from './advanced-search/advanced-search.component';
import { TimeRangeComponent } from './timerange/timerange.component';
import { UIChartComponent } from './primeng-chart/chart';
import { OrderedlistComponent } from './orderedlist/orderedlist.component';
import { DragDropModule } from '@angular/cdk/drag-drop';
import { ChipsComponent } from './chips/chips.component';
import { DomHandler } from 'primeng/api';

@NgModule({
  imports: [
    CommonModule,
    FlexLayoutModule,
    MaterialdesignModule,
    PrimengModule,
    FormsModule,
    ReactiveFormsModule,
    FormsModule,
    WidgetsModule,
    DragDropModule
  ],
  declarations: [PagebodyComponent,
    SpinnerComponent,
    DsbdwidgetheaderComponent,
    ModalheaderComponent,
    ModalbodyComponent,
    ModalitemComponent,
    ModalcontentComponent,
    ModalwidgetComponent,
    TableheaderComponent,
    LazyrenderComponent,
    PrettyDatePipe,
    AlertseventsComponent,
    WhitespaceTrimDirective,
    FieldselectorComponent,
    HerocardComponent,
    SorticonComponent,
    BasecardComponent,
    LinegraphComponent,
    FlipComponent,
    ErrorTooltipDirective,
    TablevieweditHTMLComponent,
    RoleGuardDirective,
    LabeleditorComponent,
    AdvancedSearchComponent,
    TimeRangeComponent,
    UIChartComponent,
    OrderedlistComponent,
    ChipsComponent
  ],
  exports: [
    PagebodyComponent,
    SpinnerComponent,
    DsbdwidgetheaderComponent,
    ModalheaderComponent,
    ModalbodyComponent,
    ModalitemComponent,
    ModalcontentComponent,
    ModalwidgetComponent,
    TableheaderComponent,
    LazyrenderComponent,
    PrettyDatePipe,
    AlertseventsComponent,
    WhitespaceTrimDirective,
    FieldselectorComponent,
    HerocardComponent,
    SorticonComponent,
    BasecardComponent,
    LinegraphComponent,
    FlipComponent,
    ErrorTooltipDirective,
    TablevieweditHTMLComponent,
    RoleGuardDirective,
    AdvancedSearchComponent,
    LabeleditorComponent,
    TimeRangeComponent,
    UIChartComponent,
    OrderedlistComponent,
    ChipsComponent
  ],
  providers: [
    DomHandler
  ]
})
export class SharedModule { }
