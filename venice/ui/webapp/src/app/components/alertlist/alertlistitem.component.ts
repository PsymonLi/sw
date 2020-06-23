import { Component, EventEmitter, Input, OnChanges, OnDestroy, OnInit, Output, ViewEncapsulation, ChangeDetectionStrategy, ChangeDetectorRef } from '@angular/core';
import { Utility } from '@app/common/Utility';
import { MonitoringAlert } from '@sdk/v1/models/generated/monitoring';
import { BaseComponent } from '../base/base.component';

/**
 *  an alert is like
 *  {
            "kind": "Alert",
            "api-version": "v1",
            "meta": {
                "name": "18194946-dce3-4b06-9bdc-edaf9bbf3821",
                "tenant": "default",
                "namespace": "default",
                "generation-id": "1",
                "resource-version": "7914160",
                "uuid": "24564f1d-055c-419b-8156-ebcac09026f5",
                "creation-time": "2020-06-21T20:59:36.296822038Z",
                "mod-time": "2020-06-22T18:49:22.615605378Z",
                "self-link": "/configs/monitoring/v1/tenant/default/alerts/18194946-dce3-4b06-9bdc-edaf9bbf3821"
            },
            "spec": {
                "state": "open"
            },
            "status": {
                "severity": "critical",
                "source": {
                    "component": "pen-spyglass",
                    "node-name": "10.30.5.175"
                },
                "event-uri": "/events/v1/events/fc2445eb-508a-4fc9-8522-3f1e842cf05b",
                "object-ref": {
                    "tenant": "default",
                    "kind": "Tenant",
                    "name": "default"
                },
                "message": "Flow logs rate limited at the PSM.  This error can occur if flow logs throttling has been applied —  when the number of flow log records across the PSM cluster is higher than the maximum number of records that can be published within a specific timeframe",
                "reason": {
                    "matched-requirements": [
                        {
                            "key": "Severity",
                            "operator": "equals",
                            "values": [
                                "critical"
                            ],
                            "observed-value": "critical"
                        }
                    ],
                    "alert-policy-id": "default-event-based-alerts/74c47a50-b18e-4f1c-ac56-92d3dbfd9ed2"
                },
                "acknowledged": null,
                "resolved": null,
                "total-hits": 22
            }
        },
 */

@Component({
  selector: 'app-alertlistitem',
  templateUrl: './alertlistitem.component.html',
  styleUrls: ['./alertlistitem.component.scss'],
  encapsulation: ViewEncapsulation.None,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class AlertlistitemComponent extends BaseComponent implements OnInit, OnDestroy, OnChanges {
  public static ALERT_SOURCE_NAME_LINK: string = 'alertSourceNameLink';
  @Input() data: MonitoringAlert;

  @Output() alertClick: EventEmitter<any> = new EventEmitter();
  @Output() alertSourceNameClick: EventEmitter<any> = new EventEmitter();


  alert:  MonitoringAlert;
  constructor(protected cdr: ChangeDetectorRef) {
    super(null, null);
  }

  ngOnInit() {
  }

  ngOnDestroy() {
  }

  ngOnChanges() {
    if (this.data) {
      // add a UI_FIELD_PROPERTY to avoid constant function running in html template.
      this.alert = this.data;
      this.alert[AlertlistitemComponent.ALERT_SOURCE_NAME_LINK] = this.getAlertItemSourceNameLink();
    }
    this.cdr.detectChanges();
  }

  /**
   * build css class name for alert.
   * css file has rules like
   * .alertlistitem-icon-warning {..}
   * .alertlistitem-icon-info { .. }
   */
  getAlertItemIconClass() {
    return 'alertlistitem-icon-common alertlistitem-icon-' + this.getAlertItemSeverity();
  }

  getAlertItemSeverity(): string {
    let severity = this.alert.status.severity ?  this.alert.status.severity.toString () : '';
    severity = severity.toLowerCase();
    return severity;
  }

  getAlertItemIconTooltip(): string {
    return 'Alert policy ' + this.alert.status.reason['alert-policy-id'] + ' reports ' + this.alert.status.message;
  }

  onAlertItemIconClick($event) {
    event.stopPropagation();
    event.preventDefault();
    const delimiter = '<br/>';
    const msg = this.getAlertJSON2String(this.alert);
    const severity = this.getAlertItemSeverity();
    Utility.getInstance().getControllerService().invokeConfirm({
      icon: 'pi pi-info-circle',
      header: 'Alert [' + this.alert.meta.name + '] ',
      message: 'Status: ' + delimiter + msg,
      acceptLabel: 'Close',
      acceptVisible: true,
      rejectVisible: false,
      accept: () => {
        // When a primeng alert is created, it tries to "focus" on a button, not adding a button returns an error.
        // So we create a button but hide it later.
      }
    });
  }

  getAlertJSON2String(alert: MonitoringAlert): string {
    const obj = Utility.trimUIFields(alert.status.getModelValues());
    const list = [];
    Utility.traverseNodeJSONObject(obj, 0, list, this);
    return list.join('<br/>');
  }

  onAlertItemClick($event) {
    // comment this line out for now as we are not updating alert in alertlist panel.
    // this.alert.check = !this.alert.check;
    this.alertClick.emit(this.alert);
  }

  onAlertItemSourceNameClick() {
    this.alertSourceNameClick.emit();
  }

  getAlertItemSourceNameLink(): string {
    return Utility.getAlertSourceLink(this.alert);
  }

  getAlertItemSourceNameTooltip(): string {
    const cat = this.alert.status['object-ref'].kind;
    return 'Go to ' + (cat ? cat.toLowerCase() + ' ' : '') + 'details page';
  }
}
