import { Component, OnChanges, OnDestroy, OnInit } from '@angular/core';
import { Icon } from '@app/models/frontend/shared/icon.interface';

@Component({
  selector: 'app-softwareversion',
  templateUrl: './softwareversion.component.html',
  styleUrls: ['./softwareversion.component.scss']
})
export class SoftwareversionComponent implements OnInit, OnDestroy, OnChanges {
  title = 'Software Version';
  content = 'Version 1.3.0';
  upgrademessage = 'UPDATE VER 1.3.3 AVAILABLE';
  features: any[] = [
    {
      name: 'Enterprise License',
      detail: 'Up to 5000 Naples'
    },
    {
      name: 'Premium support',
      detail: 'Support is available 2x24x365'
    },
    {
      name: 'Private cloud',
      detail: 'Pensando hosts client data center'
    },
    {
      name: 'Data analysis',
      detail: 'Pensando runs data analysis'
    }
  ];
  menuItems: any[] = [];
  icon: Icon = {
    margin: {
      top: '8px',
      left: '10px'
    },
    matIcon: 'system_update_alt'
  };

  constructor() { }

  ngOnInit() {

  }

  ngOnDestroy() {
  }

  ngOnChanges() {
  }

  onSoftwareFeatureClick($event, feature) {
    const obj = {
      event: $event,
      feature: feature
    };
    console.log('softwareversion _ onFeatureClick', obj);
  }

  onIconClick($event, id) {
    console.log('softwareversion _ onIconClick()', id, $event);
  }
}
