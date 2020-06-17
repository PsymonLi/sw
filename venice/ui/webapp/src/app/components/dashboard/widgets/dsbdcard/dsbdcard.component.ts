import { Component, OnInit, Input } from '@angular/core';

interface DisplayData {
  title: string;
  alerts: number[];
  rows: any[];
}

@Component({
  selector: 'app-dsbdcard',
  templateUrl: './dsbdcard.component.html',
  styleUrls: ['./dsbdcard.component.scss']
})
export class DsbdcardComponent implements OnInit {

  @Input() displayData: DisplayData = {
    title: '', alerts: [0, 0, 0],
    rows: [{
      title: '',
      value: '',
      unit: ''
    }, {
      title: '',
      value: '',
      unit: ''
    }, {
      title: '',
      value: '',
      unit: ''
    }, {
      title: '',
      value: '',
      unit: ''
    }]
  };

  constructor() { }

  ngOnInit() {
  }

}
