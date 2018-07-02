import { Component, OnDestroy, OnInit } from '@angular/core';
import { BaseComponent } from '@app/components/base/base.component';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { ControllerService } from '@app/services/controller.service';


@Component({
  selector: 'app-networkcore',
  templateUrl: './networkcore.component.html',
  styleUrls: ['./networkcore.component.scss']
})
export class NetworkcoreComponent extends BaseComponent implements OnInit, OnDestroy {

  constructor(protected _controllerService: ControllerService,
  ) {
    super(_controllerService);
  }

  ngOnInit() {
  }

  /**
   * Component is about to exit
   */
  ngOnDestroy() {
    // publish event that AppComponent is about to exist
    this._controllerService.publish(Eventtypes.COMPONENT_DESTROY, { 'component': 'NetworkcoreComponent', 'state': Eventtypes.COMPONENT_DESTROY });
  }

}
