import { Component, OnInit, ViewEncapsulation, Input, EventEmitter, Output, ChangeDetectorRef, AfterContentChecked, OnChanges, SimpleChanges } from '@angular/core';
import { InterfaceStats } from '../naplesdetail.component';
import { Utility } from '@app/common/Utility';
import { NetworkNetworkInterface, NetworkNetworkInterfaceSpec_type } from '@sdk/v1/models/generated/network';
import { ViewRef_ } from '@angular/core/src/view';

@Component({
  selector: 'app-naplesdetail-ifstats',
  templateUrl: 'naplesdetailifstats.component.html',
  styleUrls: ['naplesdetailifstats.component.scss'],
  encapsulation: ViewEncapsulation.None
})
export class NaplesdetailIfstatsComponent implements OnInit, AfterContentChecked , OnChanges {
  @Input() interfaceStats: InterfaceStats[];
  @Input() networkInterfaces: NetworkNetworkInterface[];
  @Input() mouseOverInterface: string = '';
  @Output() collapseExpandClickEmit: EventEmitter<string> = new EventEmitter();

  tabMode: string = 'status';
  ifs: InterfaceStats = null;
  uplinkNetworkInterface: NetworkNetworkInterface = null;

  constructor(private cdr: ChangeDetectorRef) {
   }

  ngOnChanges(changes: SimpleChanges): void {
    if (changes.mouseOverInterface) {
       this.getUplinkIpConfig();
    }
  }

  ngOnInit() {
  }

  getStats() {
    if (this.interfaceStats && this.mouseOverInterface) {
      this.ifs = this.interfaceStats.find(item => item.ifname === this.mouseOverInterface);
    }
    return this.ifs;
  }

  expandInterfaceTable() {
    this.collapseExpandClickEmit.emit('expand');
  }

  displayStatsNumber (bytes: number): string {
    if (bytes === -1) {
      return '';
    }

    if (bytes === -1000) {
      return 'N/A';
    }

    return Utility.formatBytes(bytes, 2);
  }

  displayStatsCount (count: number): string {
    if (count === null || count === undefined) {
      return '';
    }
    if (count === -1) {
      return '';
    }

    if (count === -1000) {
      return 'N/A';
    }

    return count.toString();
  }

  getUplinkIpConfig(): NetworkNetworkInterface {
    let networkInterface = null;
    if (this.networkInterfaces && this.mouseOverInterface) {
      networkInterface = this.networkInterfaces.find(item => item.meta.name === this.mouseOverInterface);
      if (networkInterface && networkInterface.spec.type === NetworkNetworkInterfaceSpec_type['uplink-eth']) {
        if (!(networkInterface.status['if-uplink-status']['ip-config']['ip-address'] === null) || !(networkInterface.status['if-uplink-status']['lldp-neighbor']['chassis-id'] === null)) {
          this.uplinkNetworkInterface = networkInterface;
          return networkInterface;
        }
      } else {
        networkInterface = null;
      }
    }
    this.showStatusTab();
    this.uplinkNetworkInterface = networkInterface;
    return networkInterface;
  }

  showUplinkIPConfig() {
    if (this.uplinkNetworkInterface) {
      this.tabMode = 'uplinkip';
    }
  }
  showUplinkLldpNeighbour() {
    if (this.uplinkNetworkInterface) {
      this.tabMode = 'lldp';
    }
  }
  showStatusTab() {
    this.tabMode = 'status';
  }
  ngAfterContentChecked(): void {
    this.cdr.detectChanges();
  }
}
