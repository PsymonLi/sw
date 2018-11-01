import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { PrimengModule } from '@lib/primeng.module';
import { MaterialdesignModule } from '@lib/materialdesign.module';
import { FlexLayoutModule } from '@angular/flex-layout';

/**-----
 Venice UI -  imports
 ------------------*/
import { NaplesComponent } from './naples.component';
import { NaplesRoutingModule } from './naples.route';
import { SharedModule } from '@app/components/shared//shared.module';
import { NaplesdetailComponent } from './naplesdetail/naplesdetail.component';
// import { NodedetailComponent } from './nodedetail/nodedetail.component';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,

    PrimengModule,
    FlexLayoutModule,
    MaterialdesignModule,

    NaplesRoutingModule,
    SharedModule
  ],
  declarations: [NaplesComponent, NaplesdetailComponent]
})
export class NaplesModule { }
